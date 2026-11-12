/* NIC-Strelka — Strelka's addition to MCUboot: the VBUS gate. MCUboot's wait for a command on
 * the USB serial port runs only with VBUS there, so a unit off the cable boots at once and one on
 * it waits its seconds (FIRMWARE.md, USB). MCUboot has no hook in that wait, so the question is put
 * to it by the patch in firmware/patches/, which calls strelka_wait_for_dfu() below. The board's
 * power is the PMIC's, nothing for the bootloader to hold (FIRMWARE.md, The PMIC).
 *
 * VBUS is read from the nRF54LM20A's USB regulator, VREGUSB (its datasheet v1.0, 5.9.1): started,
 * it raises VBUSDETECTED when VBUS is in range. The registers are written raw, so that nothing
 * but MCUboot's own build is needed; the regulator is stopped again, the serial port's driver
 * starts it for itself. */
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#ifdef CONFIG_STRELKA_VBUS_GATE
#define VREGUSB_BASE           0x50121000UL      /* VREGUSB : S */
#define VREGUSB_TASKS_START    (*(volatile uint32_t *)(VREGUSB_BASE + 0x000))
#define VREGUSB_TASKS_STOP     (*(volatile uint32_t *)(VREGUSB_BASE + 0x004))
#define VREGUSB_EVENTS_VBUSDETECTED (*(volatile uint32_t *)(VREGUSB_BASE + 0x104))
#define VREGUSB_EVENTS_VBUSREMOVED  (*(volatile uint32_t *)(VREGUSB_BASE + 0x110))
/* the detector's state, as Zephyr's VREGUSB driver reads it (regulator_nrf_vregusb.c): with VBUS
 * already there at the start no VBUSDETECTED event fires, and that is the gate's own case */
#define VREGUSB_STATUS              (*(volatile uint32_t *)(VREGUSB_BASE + 0x400))
#define VREGUSB_STATUS_VBUSDETECTED BIT(2)

#define SETTLE_US 2000           /* the detector's settling after the start; generous, once */

static bool vbus_present(void)
{
	bool present;

	VREGUSB_EVENTS_VBUSDETECTED = 0;
	VREGUSB_EVENTS_VBUSREMOVED = 0;
	VREGUSB_TASKS_START = 1;
	k_busy_wait(SETTLE_US);
	present = VREGUSB_EVENTS_VBUSDETECTED != 0 ||
		  (VREGUSB_STATUS & VREGUSB_STATUS_VBUSDETECTED) != 0;
	VREGUSB_TASKS_STOP = 1;
	return present;
}

/* asked by MCUboot's patched wait-for-DFU block: wait only with VBUS there */
bool strelka_wait_for_dfu(void)
{
	return vbus_present();
}
#endif
