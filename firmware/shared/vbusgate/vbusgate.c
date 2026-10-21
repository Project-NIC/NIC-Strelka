/* NIC-Strelka — Strelka's additions to MCUboot: the hold and the VBUS gate. The hold: P1.23,
 * HOLD, is set high by the bootloader's first instructions and keeps the board on through Q1
 * before the capacitor on its gate runs down (FIRMWARE.md, The hold). The VBUS gate: MCUboot's
 * wait for a command on the USB serial port runs only with VBUS there, so a unit off the cable
 * boots at once and one on it waits its seconds (FIRMWARE.md, USB). MCUboot has no hook in that
 * wait, so the question is put to it by the patch in firmware/patches/, which calls
 * strelka_wait_for_dfu() below.
 *
 * VBUS is read from the nRF54LM20A's USB regulator, VREGUSB (its datasheet v1.0, 5.9.1): started,
 * it raises VBUSDETECTED when VBUS is in range. The registers are written raw, so that nothing
 * but MCUboot's own build is needed; the regulator is stopped again, the serial port's driver
 * starts it for itself. */
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <hal/nrf_gpio.h>

#if CONFIG_STRELKA_HOLD_PIN >= 0
/* the hold, first: HOLD high before anything else the bootloader does */
static int hold_up(void)
{
	nrf_gpio_cfg_output(CONFIG_STRELKA_HOLD_PIN);
	nrf_gpio_pin_set(CONFIG_STRELKA_HOLD_PIN);
	return 0;
}
SYS_INIT(hold_up, PRE_KERNEL_1, 0);
#endif

#ifdef CONFIG_STRELKA_VBUS_GATE
#define VREGUSB_BASE           0x50121000UL      /* VREGUSB : S */
#define VREGUSB_TASKS_START    (*(volatile uint32_t *)(VREGUSB_BASE + 0x000))
#define VREGUSB_TASKS_STOP     (*(volatile uint32_t *)(VREGUSB_BASE + 0x004))
#define VREGUSB_EVENTS_VBUSDETECTED (*(volatile uint32_t *)(VREGUSB_BASE + 0x104))
#define VREGUSB_EVENTS_VBUSREMOVED  (*(volatile uint32_t *)(VREGUSB_BASE + 0x110))

#define SETTLE_US 2000           /* the detector's settling after the start; generous, once */

static bool vbus_present(void)
{
	bool present;

	VREGUSB_EVENTS_VBUSDETECTED = 0;
	VREGUSB_EVENTS_VBUSREMOVED = 0;
	VREGUSB_TASKS_START = 1;
	k_busy_wait(SETTLE_US);
	present = VREGUSB_EVENTS_VBUSDETECTED != 0;
	VREGUSB_TASKS_STOP = 1;
	return present;
}

/* asked by MCUboot's patched wait-for-DFU block: wait only with VBUS there */
bool strelka_wait_for_dfu(void)
{
	return vbus_present();
}
#endif
