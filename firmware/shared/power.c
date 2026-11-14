/* NIC-Strelka — the unit's power (power.h). A mark in the nRF54LM20A's GPREGRET[1], a register of
 * POWER kept through a soft reset and through MCUboot, which leaves it alone, says why the unit
 * restarted itself: switched off on the cable, to idle; or after the factory settings. Any other
 * restart off the cable — a soft reset, the watchdog's, a lockup, the hwinfo driver's reasons —
 * keeps the unit on, so that an update's swap, a set given over the network or a hang caught by
 * the watchdog never switch it off. The watchdog, WDT31, runs on through a soft reset (the
 * datasheet v1.0, table 22): MCUboot feeds it (sysbuild/mcuboot.conf); a timer reloads each
 * register enabled for the first minute of the start at most, until main installs its own, so
 * that a start that hangs is still reset; the idle and ship mode's tries feed it from their own
 * loop, so that a hang there is caught too, and the idle marks itself, so that a reset in it
 * comes back to it. Built against nRF Connect SDK v3.4.0, not run on hardware: the kit decides
 * it. */
#include <zephyr/kernel.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_power.h>
#include <hal/nrf_wdt.h>
#include "gauge.h"
#include "light.h"
#include "pmic.h"
#include "power.h"
#include "store.h"

LOG_MODULE_REGISTER(power, LOG_LEVEL_INF);

#define MARK_OFF_ON_CABLE  0x5Au /* switched off on the cable: to idle */
#define MARK_FACTORY       0x5Cu /* after the factory settings: on, the magnets' hold over */
#define KEEP_EVERY_S       3600  /* the gauge's state kept once an hour */
#define BOOT_MS            300   /* the start before the application runs, off the cable */
#define BOOT_ON_CABLE_MS   3300  /* and on it, MCUboot's 3 s for a command */
#define SHIP_TRIES         10    /* a second each, then System OFF */
#define FEED_TICKS_MAX     60    /* the timer's feeding, a second each, at the most */

static struct gpio_callback cb;
static atomic_t a_pressed;
static volatile int64_t a_pressed_at;
static int64_t a_since;
static uint8_t mark;
static uint8_t kept[GAUGE_STATE_MAX];
static K_MUTEX_DEFINE(kept_lock);

static void on_pmic(const struct device *dev, struct gpio_callback *c, uint32_t events)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(c);
	if (events & BIT(NPM13XX_EVENT_SHIPHOLD_PRESS)) {
		a_pressed_at = k_uptime_get();
		atomic_set(&a_pressed, 1);
	}
	/* the cable's going is read from VBUS itself, the event only waking the idle sooner */
}

/* the watchdog, if it is running: each enabled register reloaded; registers alone, so from the
 * timer's interrupt as well */
static void wdt_keep(void)
{
	if (!nrf_wdt_started_check(NRF_WDT31)) {
		return;
	}
	for (int rr = 0; rr < NRF_WDT_CHANNEL_NUMBER; rr++) {
		if (nrf_wdt_reload_request_enable_check(NRF_WDT31, (nrf_wdt_rr_register_t)rr)) {
			nrf_wdt_reload_request_set(NRF_WDT31, (nrf_wdt_rr_register_t)rr);
		}
	}
}

static atomic_t feed_ticks;

static void feed_tick(struct k_timer *t)
{
	if (atomic_inc(&feed_ticks) >= FEED_TICKS_MAX) {
		k_timer_stop(t);         /* a start this long has hung: the watchdog may reset it */
		return;
	}
	wdt_keep();
}
static K_TIMER_DEFINE(feed, feed_tick, NULL);

void power_watchdog_taken(void)
{
	k_timer_stop(&feed);
}

void power_keep(void)
{
	size_t n;

	k_mutex_lock(&kept_lock, K_FOREVER);
	n = gauge_state(kept, sizeof(kept));
	if (n > 0) {
		store_gauge(kept, n);
	}
	k_mutex_unlock(&kept_lock);
}

static FUNC_NORETURN void ship(void)
{
	k_timer_stop(&feed);
	power_keep();
	light_off_now();
	if (!pmic_present()) {
		LOG_WRN("no PMIC: System OFF, the development kit's off");
		sys_poweroff();
	}
	for (int tries = 0;; tries++) {
		wdt_keep();
		/* never with the cable in (7.4): then the idle, by a restart that keeps the mark */
		if (pmic_vbus()) {
			nrf_power_gpregret_set(NRF_POWER, 1, MARK_OFF_ON_CABLE);
			sys_reboot(SYS_REBOOT_COLD);
		}
		if (tries >= SHIP_TRIES) {
			LOG_ERR("ship mode not taken: System OFF");
			sys_poweroff();
		}
		pmic_ship();
		/* still running: the write did not take */
		k_msleep(1000);
	}
}

/* on the cable's power, switched off: the PMIC's light, the gauge every second; the watchdog fed
 * from this loop, and the mark set again, so that a reset here comes back here */
static void idle_on_cable(bool show_cell)
{
	LOG_INF("on the cable, not switched on");
	k_timer_stop(&feed);
	nrf_power_gpregret_set(NRF_POWER, 1, MARK_OFF_ON_CABLE);
	wdt_keep();
	if (show_cell) {
		light_show_cell(gauge_pct());
		k_msleep(2500);
	}
	light_set(LIGHT_CHARGER);
	for (unsigned s = 0;; s++) {
		wdt_keep();
		k_msleep(1000);
		gauge_service();
		if (!pmic_vbus()) {
			ship();
		}
		if (atomic_set(&a_pressed, 0) != 0 && pmic_magnet_a_held()) {
			a_since = a_pressed_at;
			nrf_power_gpregret_set(NRF_POWER, 1, 0);
			atomic_set(&feed_ticks, 0);
			k_timer_start(&feed, K_NO_WAIT, K_MSEC(1000));
			return;          /* magnet A: switched on */
		}
		if (s % KEEP_EVERY_S == KEEP_EVERY_S - 1) {
			power_keep();
		}
	}
}

void power_start(bool cable_on)
{
	uint32_t cause = 0;
	bool restarted;

	mark = (uint8_t)nrf_power_gpregret_get(NRF_POWER, 1);
	nrf_power_gpregret_set(NRF_POWER, 1, 0);
	if (hwinfo_get_reset_cause(&cause) == 0) {
		hwinfo_clear_reset_cause();
	}
	/* a reset the unit did not ask to be off by: its own, the watchdog's, a lockup, the reset
	 * pin's or the debugger's, the SWD pads' */
	restarted = (cause & (RESET_SOFTWARE | RESET_WATCHDOG | RESET_CPU_LOCKUP | RESET_PIN |
			      RESET_DEBUG)) != 0;
	k_timer_start(&feed, K_NO_WAIT, K_MSEC(1000));
	if (!pmic_present()) {
		LOG_WRN("no PMIC answering: switched on as it is, the development kit's way");
		return;
	}
	a_since = k_uptime_get() - (pmic_vbus() ? BOOT_ON_CABLE_MS : BOOT_MS);
	pmic_listen(&cb, on_pmic, BIT(NPM13XX_EVENT_SHIPHOLD_PRESS) |
		    BIT(NPM13XX_EVENT_VBUS_REMOVED));
	if (mark == MARK_OFF_ON_CABLE) {
		if (!pmic_vbus()) {
			ship();          /* the cable left during the restart */
		}
		idle_on_cable(false);
		return;
	}
	if (pmic_magnet_a_held()) {
		return;                  /* magnet A, still there since it took the PMIC out */
	}
	if (restarted || mark == MARK_FACTORY) {
		LOG_INF("restarted, staying on (reset cause 0x%08x)", cause);
		return;
	}
	if (!pmic_vbus()) {
		ship();                  /* a magnet brushed past, and no cable */
	}
	if (cable_on) {
		return;                  /* Povodok: the cable switches it on */
	}
	idle_on_cable(true);
}

bool power_after_factory(void)
{
	return mark == MARK_FACTORY;
}

int64_t power_a_since(void)
{
	return a_since;
}

FUNC_NORETURN void power_restart(bool factory)
{
	if (!factory) {
		power_keep();    /* after the factory settings the gauge's state stays forgotten */
	}
	light_off_now();
	nrf_power_gpregret_set(NRF_POWER, 1, factory ? MARK_FACTORY : 0u);
	sys_reboot(SYS_REBOOT_COLD);
}

FUNC_NORETURN void power_off(void)
{
	if (!pmic_present() || !pmic_vbus()) {
		ship();
	}
	power_keep();
	light_off_now();
	nrf_power_gpregret_set(NRF_POWER, 1, MARK_OFF_ON_CABLE);
	sys_reboot(SYS_REBOOT_COLD);
}
