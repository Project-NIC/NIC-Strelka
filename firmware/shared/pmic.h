/* NIC-Strelka — the PMIC, Nordic's nPM1300, on Kryska the nPM1304 or the nPM1300, through the
 * SDK's own drivers: its MFD for the registers and the events on PMIC_INT, its charger as a
 * sensor, its regulators' parent for ship mode (FIRMWARE.md, The PMIC; BOARD.md, U2). Magnet A
 * is the PMIC's SHPHLD, the light its LED0, red, and LED1, green. Built against nRF Connect SDK
 * v3.4.0, not run on hardware: the kit decides it. */
#ifndef STRELKA_PMIC_H
#define STRELKA_PMIC_H

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm13xx.h>

enum pmic_charge {
	PMIC_OFF_CABLE,          /* no VBUS */
	PMIC_CHARGING,           /* trickle, constant current or constant voltage */
	PMIC_CHARGED,            /* on the cable, the charge complete or waiting */
	PMIC_CHARGE_FAULT,       /* BCHGERRREASON not 0 */
};

#define PMIC_LED_RED   0u        /* LED0, from reset the charger's error */
#define PMIC_LED_GREEN 1u        /* LED1, from reset its charge */

int pmic_init(void);
bool pmic_present(void);                 /* its MFD and charger ready */
const struct device *pmic_charger(void); /* the charger as a sensor, for the gauge */
/* an event's callback: `events` a mask of BIT(NPM13XX_EVENT_...), called from the MFD's work */
int pmic_listen(struct gpio_callback *cb, gpio_callback_handler_t handler, uint32_t events);
bool pmic_vbus(void);
/* one measurement of the cell and the charger, one caller at a time (the fetch starts the PMIC's
 * ADC): volts, amperes discharging positive as Zephyr's sensor gives them, °C, BCHGCHARGESTATUS,
 * BCHGERRREASON */
int pmic_measure(float *v, float *i, float *t, int32_t *status, int32_t *error);
bool pmic_magnet_a_held(void);           /* SHPHLDSTATUS, bit 0 low while magnet A is there */
enum pmic_charge pmic_charge(void);
/* the light: LED0 and LED1 given to the charger's own modes, or taken into the host's */
int pmic_light_to_charger(bool charger);
int pmic_led(unsigned led, bool on);     /* in the host's mode */
/* ship mode, the off: the cell parted from the board; only with the cable out (7.4). Returns only
 * if the PMIC did not take it */
int pmic_ship(void);

#endif
