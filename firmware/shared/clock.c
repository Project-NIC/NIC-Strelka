#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <hal/nrf_timer.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_gpio.h>
#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#include <gpiote_nrfx.h>
#include <nrfx.h>
#include <nrfx_power.h>
#include <hal/nrf_clock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include "loop.h"
#include "timetable.h"
#include "clock.h"

LOG_MODULE_REGISTER(clock, LOG_LEVEL_INF);

/* TIMER21 makes CLKIN, TIMER22 stamps; both in the peripheral domain with GPIOTE20, which serves
 * P1 and P3, and DPPIC20. MPSL, while it runs, holds TIMER10, TIMER20 and channel 0 of DPPIC10,
 * DPPIC20, PPIB11 and PPIB21; ESB, while it runs, TIMER10 and the channels it takes through
 * nrfx_gppi (FIRMWARE.md, The pins): the channels here are taken through nrfx_gppi as well, so
 * that none meets another's. GPIOTE20 is Zephyr's GPIO driver's too: its channels are taken through nrfx, and the data-ready's interrupt
 * comes through nrfx's handler on the line the application core's interrupt group uses —
 * GPIOTE20_1 for the secure core — not through a line of this file's own. */
#define T_CLKIN      NRF_TIMER21
#define T_STAMP      NRF_TIMER22
#define GPIOTE       NRF_GPIOTE20
#define DPPI         NRF_DPPIC20
/* the nrfx instance Zephyr's GPIO driver defines for the node, shared with it */
#define gpiote       GPIOTE_NRFX_INST_BY_NODE(DT_NODELABEL(gpiote20))
/* the pins from the sensor's node in the overlay, their one home: CLKIN P3.00, INT1 P3.01. Povodok
 * has no sensor: its clock is the stamps and the grid alone, CLKIN and the data-ready left out */
#define IMU_NODE     DT_ALIAS(imu)
#define HAS_SENSOR   DT_NODE_EXISTS(IMU_NODE)
#if HAS_SENSOR
#define DT_PIN(prop) NRF_GPIO_PIN_MAP(DT_PROP(DT_GPIO_CTLR(IMU_NODE, prop), port), \
				      DT_GPIO_PIN(IMU_NODE, prop))
#define PIN_CLKIN    DT_PIN(clkin_gpios)
static bool clkin_pin_set;             /* the pin an output of GPIOTE, from clock_clkin_start */
#define PIN_DR       DT_PIN(int_gpios)
BUILD_ASSERT((DT_PROP(DT_GPIO_CTLR(IMU_NODE, clkin_gpios), port) == 3 ||
	      DT_PROP(DT_GPIO_CTLR(IMU_NODE, clkin_gpios), port) == 1) &&
	     (DT_PROP(DT_GPIO_CTLR(IMU_NODE, int_gpios), port) == 3 ||
	      DT_PROP(DT_GPIO_CTLR(IMU_NODE, int_gpios), port) == 1),
	     "CLKIN and INT1 on P1 or P3, the ports of GPIOTE20 and the peripheral domain's timers");
#endif
#if HAS_SENSOR
static uint8_t ch_clkin;  /* GPIOTE channels from nrfx: the CLKIN toggle, the data-ready edge */
static uint8_t ch_dr;
#endif
/* DPPIC20's channels, from nrfx_gppi at clock_init */
static uint8_t dppi_toggle;  /* TIMER21 COMPARE[0,1] -> GPIOTE OUT toggle */
static uint8_t dppi_dr;      /* GPIOTE IN -> TIMER22 CAPTURE[0] */
#define CC_STAMP_DR     0
#define CC_STAMP_SYNC   1  /* 2 the radio's TXEN compare, 3 the wrap, 4 clock_now's capture */
#define RX_END_DELAY_TICKS 0   /* a receiver's END after the packet's last bit: the kit measures
				* it, its mean taken out here (FIRMWARE.md, The sensor and its
				* clock) */

static volatile uint32_t stamp_high;    /* TIMER22's 32-bit count, extended */
static struct loop loop;
static bool steered;
static uint32_t sample_period_ticks = 16000;   /* 1 kHz */
static uint64_t origin;                 /* the set's period 0 on the local timer */
static bool origin_known;
static volatile int32_t pending_ticks;  /* ticks to add to CLKIN's periods, signed */
/* pin 9 as the LSM6DSV16X's reference instead of CLKIN: a rising edge every second period of the
 * set, 40 ms, toggled at its half, the shortest reference from 200 Hz up (AN5763 Rev 4, 3.3,
 * table 15; FIRMWARE.md, The sensor and its clock) */
#define CLOCK_REF_HALF  CLOCK_PERIOD_TICKS
static bool ref_mode;
static void (*data_ready_cb)(uint64_t stamp);

/* a stamp captured in the past, within half the timer's round of 268 s, placed against now:
 * one taken just before the wrap and read just after belongs to the round before */
static uint64_t extend(uint32_t low)
{
	uint64_t now = clock_now();
	uint64_t t = (now & ~(uint64_t)0xFFFFFFFFu) | low;

	if (low > (uint32_t)now) {
		t -= (uint64_t)1 << 32;
	}
	return t;
}

ISR_DIRECT_DECLARE(timer22_isr)
{
	if (nrf_timer_event_check(T_STAMP, NRF_TIMER_EVENT_COMPARE3)) {
		nrf_timer_event_clear(T_STAMP, NRF_TIMER_EVENT_COMPARE3);
		stamp_high++;                   /* COMPARE[3] at 0: the 32-bit wrap */
	}
	return 1;
}

#if HAS_SENSOR
/* nrfx's handler for the data-ready pin, in the GPIOTE interrupt: the edge's stamp, captured in
 * hardware before this runs, goes with it */
static void data_ready_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t trigger, void *ctx)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(trigger);
	ARG_UNUSED(ctx);
	if (data_ready_cb) {
		data_ready_cb(clock_last_data_ready());
	}
}
#endif

ISR_DIRECT_DECLARE(timer21_isr)
{
	/* a corrected period has ended: back to the plain half period */
	if (nrf_timer_event_check(T_CLKIN, NRF_TIMER_EVENT_COMPARE1)) {
		nrf_timer_event_clear(T_CLKIN, NRF_TIMER_EVENT_COMPARE1);
		int32_t p = pending_ticks;

		if (ref_mode) {
			/* the reference's period takes every tick asked since the last at once */
			nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1,
					 (uint32_t)((int32_t)(2 * CLOCK_REF_HALF) + p));
			pending_ticks = 0;
			if (p == 0) {
				nrf_timer_int_disable(T_CLKIN, NRF_TIMER_INT_COMPARE1_MASK);
			}
		} else if (p > 0) {
			nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1, 2 * LOOP_CLKIN_HALF + 1);
			pending_ticks = p - 1;
		} else if (p < 0) {
			nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1, 2 * LOOP_CLKIN_HALF - 1);
			pending_ticks = p + 1;
		} else {
			nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1, 2 * LOOP_CLKIN_HALF);
			nrf_timer_int_disable(T_CLKIN, NRF_TIMER_INT_COMPARE1_MASK);
		}
	}
	return 1;
}

/* The 32 MHz crystal, held for as long as the unit keeps the set's time and never let go between
 * the radio's turns (FIRMWARE.md, The sensor and its clock): without the request the 16 MHz
 * peripheral clock runs on the internal oscillator, -10 to +6 % (nRF54LM20A datasheet v1.0,
 * 5.4.1 and 11.4.1). Constant Latency mode for as long, on every chip: the peripheral domain kept
 * clocked while the SYNC is awaited, so that its stamp through the bridge keeps the 1 to 2 cycles
 * of the domains' synchronisers and no wake-up's microseconds (Nordic's answer, DevZone 129531);
 * it is also erratum 20's mend, which ESB would apply with its clock. CONFIG_ESB_CLOCK_INIT is off,
 * so erratum 39's, the PLL kept running, is applied here too (the SDK's
 * nrf/subsys/esb/esb_workarounds.c and esb_glue.c). */
static struct onoff_client hfxo_cli;

static int hfxo_hold(void)
{
	struct onoff_manager *mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	int err, res;

	if (!mgr) {
		return -ENODEV;
	}
	sys_notify_init_spinwait(&hfxo_cli.notify);
	err = onoff_request(mgr, &hfxo_cli);
	if (err < 0) {
		return err;
	}
	do {
		err = sys_notify_fetch_result(&hfxo_cli.notify, &res);
		if (!err && res) {
			return res;
		}
		if (err == -EAGAIN) {
			k_yield();
		}
	} while (err == -EAGAIN);
	(void)nrfx_power_constlat_mode_request();
	if (NRF_ERRATA_DYNAMIC_CHECK(54L, 39)) {
		nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_PLLSTART);
	}
	return err;
}

/* a channel of DPPIC20 from nrfx_gppi, which ESB takes its own from too */
static int dppi_take(uint8_t *ch)
{
	int c = nrfx_gppi_channel_alloc(nrfx_gppi_domain_id_get((uint32_t)DPPI));

	if (c < 0) {
		return c;
	}
	*ch = (uint8_t)c;
	return 0;
}

/* the radio's END reaches TIMER22's CAPTURE[1] across the domains, PPIB11 to PPIB21: the link
 * makes the route with nrfx_gppi_conn_alloc() from RADIO.END to this task, which takes every
 * channel and the bridge's own. TIMER22 the one timer of the set's time, as Nordic advises, not
 * TIMER10 beside it, which ESB takes again at every turn; the route some 250 to 500 ns, its
 * variation 1 to 2 cycles of 16 MHz, some 125 ns (Nordic's answer, DevZone 129531); the kit
 * measures it and which event of the SYNC — END, ADDRESS or the initial SYNC — varies least */
uint32_t clock_sync_capture_task(void)
{
	return nrf_timer_task_address_get(T_STAMP, NRF_TIMER_TASK_CAPTURE1);
}

int clock_init(void)
{
	int err = hfxo_hold();

	if (err) {
		LOG_ERR("the crystal not started: %d", err);
		return err;
	}
	LOG_INF("the crystal held");

	if (dppi_take(&dppi_toggle) || dppi_take(&dppi_dr)) {
		LOG_ERR("no DPPIC20 channel free");
		return -EBUSY;
	}

	/* the stamp timer: 16 MHz, 32 bits, free-running; the wrap counted on COMPARE[3] at 0 */
	nrf_timer_mode_set(T_STAMP, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(T_STAMP, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_prescaler_set(T_STAMP, NRF_TIMER_FREQ_16MHz);
	nrf_timer_cc_set(T_STAMP, NRF_TIMER_CC_CHANNEL3, 0);
	nrf_timer_int_enable(T_STAMP, NRF_TIMER_INT_COMPARE3_MASK);
	IRQ_DIRECT_CONNECT(TIMER22_IRQn, 1, timer22_isr, 0);
	irq_enable(TIMER22_IRQn);
	nrf_timer_task_trigger(T_STAMP, NRF_TIMER_TASK_START);

#if HAS_SENSOR
	/* GPIOTE20 is initialised by Zephyr's GPIO driver, its interrupt line connected there; the
	 * channels for the data-ready edge and the CLKIN toggle are taken from it */
	if (!nrfx_gpiote_init_check(&gpiote)) {
		LOG_ERR("GPIOTE20 not initialised: the GPIO driver is wanted");
		return -ENODEV;
	}
	/* this nrfx answers 0 on success and -errno otherwise */
	if (nrfx_gpiote_channel_alloc(&gpiote, &ch_dr) != 0 ||
	    nrfx_gpiote_channel_alloc(&gpiote, &ch_clkin) != 0) {
		LOG_ERR("no GPIOTE20 channel free");
		return -EBUSY;
	}
	/* the data-ready edge into CAPTURE[0] through its GPIOTE event and the DPPI; nrfx's
	 * handler for the interrupt */
	{
		static const nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_NOPULL;
		const nrfx_gpiote_trigger_config_t trigger = {
			.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
			.p_in_channel = &ch_dr,
		};
		const nrfx_gpiote_handler_config_t handler = {
			.handler = data_ready_handler,
			.p_context = NULL,
		};
		const nrfx_gpiote_input_pin_config_t input = {
			.p_pull_config = &pull,
			.p_trigger_config = &trigger,
			.p_handler_config = &handler,
		};

		if (nrfx_gpiote_input_configure(&gpiote, PIN_DR, &input) != 0) {
			return -EIO;
		}
	}
	nrf_gpiote_publish_set(GPIOTE, nrf_gpiote_in_event_get(ch_dr), dppi_dr);
	nrf_timer_subscribe_set(T_STAMP, NRF_TIMER_TASK_CAPTURE0, dppi_dr);
	nrf_dppi_channels_enable(DPPI, BIT(dppi_dr));
	nrfx_gpiote_trigger_enable(&gpiote, PIN_DR, true);
#endif

	/* CLKIN: TIMER21 toggles the pin at COMPARE[0] (the half) and COMPARE[1] (the period, which
	 * also clears the count): 250 and 500 ticks of 16 MHz, 32 kHz. The pin is configured and
	 * driven only once the sensor is set to take it, clock_clkin_start; until then, and on the
	 * LSM6DSV320X for good, it stays an input against the sensor's own INT2 */
	nrf_timer_mode_set(T_CLKIN, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(T_CLKIN, NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_prescaler_set(T_CLKIN, NRF_TIMER_FREQ_16MHz);
	nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL0, LOOP_CLKIN_HALF);
	nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1, 2 * LOOP_CLKIN_HALF);
	nrf_timer_shorts_enable(T_CLKIN, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK);
	nrf_timer_publish_set(T_CLKIN, NRF_TIMER_EVENT_COMPARE0, dppi_toggle);
	nrf_timer_publish_set(T_CLKIN, NRF_TIMER_EVENT_COMPARE1, dppi_toggle);
#if HAS_SENSOR
	nrf_gpiote_subscribe_set(GPIOTE, nrf_gpiote_out_task_get(ch_clkin), dppi_toggle);
#endif
	nrf_dppi_channels_enable(DPPI, BIT(dppi_toggle));
	IRQ_DIRECT_CONNECT(TIMER21_IRQn, 1, timer21_isr, 0);
	irq_enable(TIMER21_IRQn);
	loop_init(&loop, true, sample_period_ticks);
	return 0;
}

void clock_on_data_ready(void (*cb)(uint64_t stamp))
{
	data_ready_cb = cb;
}

void clock_data_ready_irq(bool on)
{
#if HAS_SENSOR
	/* the event, and with it the capture, stays on; the interrupt alone comes and goes */
	nrfx_gpiote_trigger_enable(&gpiote, PIN_DR, on);
#else
	ARG_UNUSED(on);
#endif
}

void clock_pin9_reference(bool on)
{
	ref_mode = on;
	nrf_timer_bit_width_set(T_CLKIN, on ? NRF_TIMER_BIT_WIDTH_32 : NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL0, on ? CLOCK_REF_HALF : LOOP_CLKIN_HALF);
	nrf_timer_cc_set(T_CLKIN, NRF_TIMER_CC_CHANNEL1,
			 on ? 2 * CLOCK_REF_HALF : 2 * LOOP_CLKIN_HALF);
}

void clock_clkin_start(bool steer)
{
	steered = steer;
	loop_init(&loop, steer, sample_period_ticks);
	pending_ticks = 0;
#if HAS_SENSOR
	if (!clkin_pin_set) {
		const nrfx_gpiote_output_config_t output = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;
		const nrfx_gpiote_task_config_t task = {
			.task_ch = ch_clkin,
			.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
			.init_val = NRF_GPIOTE_INITIAL_VALUE_LOW,
		};

		clkin_pin_set = nrfx_gpiote_output_configure(&gpiote, PIN_CLKIN, &output, &task) == 0;
	}
	nrfx_gpiote_out_task_enable(&gpiote, PIN_CLKIN);
#endif
	nrf_timer_task_trigger(T_CLKIN, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(T_CLKIN, NRF_TIMER_TASK_START);
}

void clock_off_grid(void)
{
	/* nothing on pin 9, which stays the sensor's own; the loop steers nothing, and the set's
	 * time is still taken from every SYNC, the samples put onto the grid by their stamps */
	steered = false;
	loop_init(&loop, false, sample_period_ticks);
	pending_ticks = 0;
}

int64_t clock_since_origin(uint64_t stamp)
{
	return (int64_t)(stamp - origin);
}

uint64_t clock_grid_stamp(int64_t index)
{
	return origin + (uint64_t)index * sample_period_ticks;
}

void clock_clkin_stop(void)
{
	nrf_timer_task_trigger(T_CLKIN, NRF_TIMER_TASK_STOP);
#if HAS_SENSOR
	nrfx_gpiote_out_task_disable(&gpiote, PIN_CLKIN);
	if (clkin_pin_set) {
		nrfx_gpiote_pin_uninit(&gpiote, PIN_CLKIN);   /* an input again */
		clkin_pin_set = false;
	}
#endif
}

uint64_t clock_now(void)
{
	/* CAPTURE[4]: COMPARE[2] is the radio's, the instant a send's TXEN fires. A wrap whose
	 * interrupt has not yet run — a higher one holding it off — is counted here */
	unsigned int key = irq_lock();
	uint32_t hi, low;

	nrf_timer_task_trigger(T_STAMP, NRF_TIMER_TASK_CAPTURE4);
	low = nrf_timer_cc_get(T_STAMP, NRF_TIMER_CC_CHANNEL4);
	hi = stamp_high;
	if (nrf_timer_event_check(T_STAMP, NRF_TIMER_EVENT_COMPARE3) && low < 0x80000000u) {
		hi++;
	}
	irq_unlock(key);
	return ((uint64_t)hi << 32) | low;
}

uint64_t clock_last_data_ready(void)
{
	return extend(nrf_timer_cc_get(T_STAMP, NRF_TIMER_CC_CHANNEL0));
}

uint64_t clock_last_sync_end(void)
{
	return extend(nrf_timer_cc_get(T_STAMP, NRF_TIMER_CC_CHANNEL1));
}

void clock_set_sample_period(uint32_t ticks)
{
	sample_period_ticks = ticks;
	loop.sample_ticks = ticks;
}

uint64_t clock_period_start(uint32_t period)
{
	return origin + (uint64_t)period * CLOCK_PERIOD_TICKS;
}

bool clock_origin_known(void)
{
	return origin_known;
}

void clock_sync(bool heard, uint64_t end_local, uint32_t period, uint8_t povodok)
{
	int64_t off = 0;
	int32_t ticks;

	if (heard) {
		/* the SYNC names its Povodok, so the period's start is known: its slot's start and
		 * its time on the air before the END, the receiver's delay taken out */
		uint64_t start = end_local - RX_END_DELAY_TICKS -
				 (uint64_t)(TT_SYNC_AT_US(povodok) + TT_SYNC_AIR_US) * 16u;

		origin = start - (uint64_t)period * CLOCK_PERIOD_TICKS;
		origin_known = true;
		off = (int64_t)(clock_last_data_ready() - origin);
	}
	ticks = loop_period(&loop, heard, off, period);
	if (ticks != 0 && steered) {
		unsigned int key = irq_lock();   /* timer21_isr counts the same ticks down */

		pending_ticks += ticks;
		nrf_timer_int_enable(T_CLKIN, NRF_TIMER_INT_COMPARE1_MASK);
		irq_unlock(key);
	}
}

bool clock_locked(void)
{
	return loop_locked(&loop);
}

int64_t clock_sample_index(uint64_t stamp)
{
	/* the samples stand on the grid, steered there: the nearest grid point is the sample's */
	if (!origin_known || (int64_t)(stamp - origin) < 0) {
		return -1;
	}
	return (int64_t)((stamp - origin + sample_period_ticks / 2) / sample_period_ticks);
}

uint32_t clock_samples_a_period(void)
{
	return CLOCK_PERIOD_TICKS / sample_period_ticks;
}

int32_t clock_grid_offset_ticks(uint64_t stamp)
{
	return (int32_t)loop_centre((int64_t)(stamp - origin), sample_period_ticks);
}
