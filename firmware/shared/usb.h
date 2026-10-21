/* NIC-Strelka — the USB device on Zephyr's device-next stack (FIRMWARE.md, USB): the classes
 * are the overlay's and the Kconfig's — the serial port for the console and MCUboot's door on
 * both units, the disk over the card on the Belka, the network adapter, CDC-NCM, on Povodok. The
 * device comes up when the stack sees VBUS and goes when it leaves; the Belka keeps it down
 * while it records, the card being the record's then. */
#ifndef STRELKA_USB_H
#define STRELKA_USB_H

#include <stdbool.h>

int usb_init(void);
void usb_allow(bool allowed);   /* false: the device down and kept down; true: up when VBUS is */
bool usb_vbus(void);            /* VBUS present, as the stack reported it last */
bool usb_enabled(void);

#endif
