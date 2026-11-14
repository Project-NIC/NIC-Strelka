/* NIC-Strelka — the FLPR's program: the sensor alone (FIRMWARE.md, The sensor on the FLPR). It
 * finds the sensor, sets it at the main core's word, and while it measures, waits for each
 * data-ready's stamp in TIMER22 — captured there in hardware, the main core's timer — reads that
 * sample over SPIM21 and lays it in the ring the main core empties (common/sensor_ring.h). No
 * interrupt of its own: the FLPR does nothing else. Built against nRF Connect SDK v3.4.0 for
 * nrf54lm20dk/nrf54lm20a/cpuflpr, not run on hardware: the kit decides it. */
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <hal/nrf_timer.h>
#include "imu.h"
#include "imu_tables.h"
#include "sensor_ring.h"

#define RING_NODE DT_NODELABEL(sensor_ring)
BUILD_ASSERT(DT_REG_SIZE(RING_NODE) >= sizeof(struct ring), "the ring's region holds it");
static struct ring *const r = (struct ring *)DT_REG_ADDR(RING_NODE);

#define T_STAMP NRF_TIMER22   /* the main core's free timer; its CC[0] the data-ready's capture */

static uint32_t period_ticks;  /* 16 MHz / the sensor's own rate */
static uint32_t last;          /* the last stamp taken */
static bool have_last;

static void command(uint8_t cmd)
{
	struct imu_config cfg = {
		.rate_hz = r->head.rate_hz,
		.acc_g = r->head.acc_g,
		.gyro_dps = (uint16_t)(r->head.gyro_125 * 125u),
		.hg = r->head.hg,
		.n = r->head.n,
	};

	switch (cmd) {
	case RING_CMD_SET:
		if (cfg.rate_hz == 0 || imu_configure(&cfg)) {
			r->head.state = RING_FAULT;
			break;
		}
		period_ticks = 16000000u / imu_sensor_hz(cfg.rate_hz, cfg.n);
		/* set while running — the session's settings — it runs on with them, as the main
		 * core's fallback does; set before the start, it waits for the start */
		r->head.state = r->head.state == RING_RUN ? RING_RUN : RING_SET;
		break;
	case RING_CMD_START:
		r->head.state = imu_start() ? RING_FAULT : RING_RUN;
		break;
	case RING_CMD_STOP:
		imu_stop();
		r->head.state = RING_SET;
		break;
	default:
		break;
	}
	/* only the data-readies after the command */
	last = nrf_timer_cc_get(T_STAMP, NRF_TIMER_CC_CHANNEL0);
	have_last = false;
}

static void take(uint32_t stamp)
{
	struct strelka_step step;
	struct ring_sample s;
	bool acc_out, gyro_out;
	int16_t hg[3];

	if (imu_read_step(&step, hg, &acc_out, &gyro_out)) {
		r->head.state = RING_FAULT;
		return;
	}
	/* a stamp more than a period and a half after the last: the samples between passed */
	if (have_last && period_ticks) {
		uint32_t n = (stamp - last + period_ticks / 2) / period_ticks;

		if (n > 1) {
			r->head.missed = (uint16_t)(r->head.missed + n - 1);
		}
	}
	last = stamp;
	have_last = true;
	s.stamp = stamp;
	memcpy(s.a, step.a, sizeof(s.a));
	memcpy(s.g, step.g, sizeof(s.g));
	memcpy(s.hg, hg, sizeof(s.hg));
	s.pad = 0;
	ring_push(r, &s);
	r->head.temp_half = ring_temp_half(imu_temp_centi());
}

int main(void)
{
	/* the head was cleared by the main core before it started this core; the sensor's byte
	 * last, so that the main core reads the rest set */
	r->head.state = RING_OFF;
	RING_FENCE();
	r->head.sensor = imu_init() ? 0 : imu_whoami();

	for (;;) {
		uint8_t seq = r->head.cmd_seq;

		if (seq != r->head.done_seq) {
			RING_FENCE();
			command(r->head.cmd);
			RING_FENCE();
			r->head.done_seq = seq;
		}
		if (r->head.state != RING_RUN) {
			k_sleep(K_MSEC(1));
			continue;
		}
		uint32_t stamp = nrf_timer_cc_get(T_STAMP, NRF_TIMER_CC_CHANNEL0);

		if (stamp != last) {
			take(stamp);
		}
	}
	return 0;
}
