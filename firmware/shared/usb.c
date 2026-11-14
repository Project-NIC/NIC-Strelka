/* After the SDK's samples/subsys/usb/common/sample_usbd_init.c. Built against nRF Connect SDK
 * v3.4.0, not run on hardware: the kit decides it, and first whether the SDK's driver bears the
 * three classes at once (STATUS.md). The driver blocks its enabling until VBUS is ready, so the
 * device is enabled on the stack's VBUS message alone, never at the start. */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>
#include "usb.h"

#ifdef CONFIG_USBD_MSC_CLASS
#include <zephyr/usb/class/usbd_msc.h>
/* the card as it is: the disk "SD" of the overlay's sdmmc node, shown only while the Belka does
 * not record, and read alone while it is in a set unless opened (FIRMWARE.md, USB) */
USBD_DEFINE_MSC_LUN(sd, "SD", "NIC", "Strelka record", "0.8");
#endif

static atomic_t protected_disk;

#ifdef CONFIG_USBD_MSC_CLASS
/* the class asks at every MODE SENSE and WRITE(10): Zephyr's weak answer replaced by the patch's
 * hook (patches/patches.yml) */
bool usbd_msc_write_protected(const char *disk)
{
	ARG_UNUSED(disk);
	return atomic_get(&protected_disk) != 0;
}
#endif

LOG_MODULE_REGISTER(usb, LOG_LEVEL_INF);

/* Zephyr's test VID/PID until Strelka has its own; the product string tells the unit */
USBD_DEVICE_DEFINE(strelka_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2fe3, 0x0001);
USBD_DESC_LANG_DEFINE(strelka_lang);
USBD_DESC_MANUFACTURER_DEFINE(strelka_mfr, "NIC");
USBD_DESC_PRODUCT_DEFINE(strelka_product, "NIC-Strelka");
USBD_DESC_CONFIG_DEFINE(strelka_cfg_desc, "on the cable");
/* self-powered, the cell being the supply; 500 mA named, in units of 2 mA, so that a port
 * gives the charger its 300 mA and the board the rest (BOARD.md, The supply) */
USBD_CONFIGURATION_DEFINE(strelka_fs_config, USB_SCD_SELF_POWERED, 250, &strelka_cfg_desc);
USBD_CONFIGURATION_DEFINE(strelka_hs_config, USB_SCD_SELF_POWERED, 250, &strelka_cfg_desc);

static bool allowed = true, vbus, enabled;
/* the flags and the stack's enable and disable under one lock: the stack's messages, main's
 * loop, MCUmgr's queue and the record's start all reach them, and a protect toggled while the
 * VBUS message is handled must not leave `enabled` out of step with the stack */
static K_MUTEX_DEFINE(usb_lock);

/* under usb_lock */
static void up_or_down(void)
{
	if (allowed && vbus && !enabled) {
		if (usbd_enable(&strelka_usbd) == 0) {
			enabled = true;
			LOG_INF("on the cable");
		}
	} else if ((!allowed || !vbus) && enabled) {
		usbd_disable(&strelka_usbd);
		enabled = false;
		if (vbus) {
			LOG_INF("the device down");
		} else {
			LOG_INF("off the cable");
		}
	}
}

static void usb_message(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	ARG_UNUSED(ctx);
	switch (msg->type) {
	case USBD_MSG_VBUS_READY:
		k_mutex_lock(&usb_lock, K_FOREVER);
		vbus = true;
		up_or_down();
		k_mutex_unlock(&usb_lock);
		break;
	case USBD_MSG_VBUS_REMOVED:
		k_mutex_lock(&usb_lock, K_FOREVER);
		vbus = false;
		up_or_down();
		k_mutex_unlock(&usb_lock);
		break;
	default:
		break;
	}
}

static int add_config(enum usbd_speed speed, struct usbd_config_node *cfg)
{
	int rc = usbd_add_configuration(&strelka_usbd, speed, cfg);

	if (rc) {
		return rc;
	}
	/* the classes here have an interface association, CDC's and NCM's: the device's class
	 * triple says so, as the SDK's sample sets it, which Windows needs to bind them */
	usbd_device_set_code_triple(&strelka_usbd, speed, USB_BCC_MISCELLANEOUS, 0x02, 0x01);
	return usbd_register_all_classes(&strelka_usbd, speed, 1, NULL);
}

int usb_init(void)
{
	int rc;

	if ((rc = usbd_add_descriptor(&strelka_usbd, &strelka_lang)) ||
	    (rc = usbd_add_descriptor(&strelka_usbd, &strelka_mfr)) ||
	    (rc = usbd_add_descriptor(&strelka_usbd, &strelka_product))) {
		LOG_ERR("descriptors %d", rc);
		return rc;
	}
	if ((rc = add_config(USBD_SPEED_FS, &strelka_fs_config)) ||
	    (usbd_caps_speed(&strelka_usbd) == USBD_SPEED_HS &&
	     (rc = add_config(USBD_SPEED_HS, &strelka_hs_config)))) {
		LOG_ERR("configuration %d", rc);
		return rc;
	}
	usbd_msg_register_cb(&strelka_usbd, usb_message);
	rc = usbd_init(&strelka_usbd);
	if (rc) {
		LOG_ERR("usbd %d", rc);
		return rc;
	}
	/* the driver detects VBUS and the stack says so: enabled then, not here */
	return 0;
}

void usb_allow(bool allow)
{
	k_mutex_lock(&usb_lock, K_FOREVER);
	allowed = allow;
	up_or_down();
	k_mutex_unlock(&usb_lock);
}

void usb_disk_protect(bool on)
{
	if ((atomic_get(&protected_disk) != 0) == on) {
		return;
	}
	atomic_set(&protected_disk, on);
	if (!IS_ENABLED(CONFIG_USBD_MSC_CLASS)) {
		return;                          /* Kryska: no disk to show anew; the flag guards
						  * its set file alone (flash_rec.c) */
	}
	k_mutex_lock(&usb_lock, K_FOREVER);
	if (enabled) {
		/* the host reads the protection at its mount: down and up, mounted anew */
		usbd_disable(&strelka_usbd);
		enabled = false;
		up_or_down();
	}
	k_mutex_unlock(&usb_lock);
}

bool usb_disk_protected(void)
{
	return atomic_get(&protected_disk) != 0;
}

bool usb_vbus(void)
{
	return vbus;
}

bool usb_enabled(void)
{
	return enabled;
}
