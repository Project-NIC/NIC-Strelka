#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/net/wifi.h>
#include <zephyr/logging/log.h>
#include "ble_prov.h"
#include "phone_net.h"

LOG_MODULE_REGISTER(ble_prov, LOG_LEVEL_INF);

/* Strelka's own 128-bit base, the service and its characteristics in the third group (PHONE.md) */
#define UUID_BASE(x) BT_UUID_128_ENCODE(0x5354524cu, 0x4b41u, (x), 0x8000u, 0x00805f9b34fbull)
static const struct bt_uuid_128 uuid_service = BT_UUID_INIT_128(UUID_BASE(0x0001));
static const struct bt_uuid_128 uuid_name = BT_UUID_INIT_128(UUID_BASE(0x0002));
static const struct bt_uuid_128 uuid_password = BT_UUID_INIT_128(UUID_BASE(0x0003));
static const struct bt_uuid_128 uuid_join = BT_UUID_INIT_128(UUID_BASE(0x0004));
static const struct bt_uuid_128 uuid_outcome = BT_UUID_INIT_128(UUID_BASE(0x0005));

enum outcome { OUTCOME_NONE = 0, OUTCOME_JOINING = 1, OUTCOME_JOINED = 2, OUTCOME_FAILED = 3 };

static char ssid[WIFI_SSID_MAX_LEN];
static char password[WIFI_PSK_MAX_LEN];
static uint8_t ssid_len, password_len, outcome;
static bool running;

static ssize_t write_text(const void *buf, uint16_t len, uint16_t offset, char *out, size_t cap,
			  uint8_t *out_len)
{
	if (offset != 0 || len > cap) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	memcpy(out, buf, len);
	*out_len = (uint8_t)len;
	return len;
}

static ssize_t write_name(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn); ARG_UNUSED(attr); ARG_UNUSED(flags);
	return write_text(buf, len, offset, ssid, sizeof(ssid), &ssid_len);
}

static ssize_t write_password(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn); ARG_UNUSED(attr); ARG_UNUSED(flags);
	return write_text(buf, len, offset, password, sizeof(password), &password_len);
}

static ssize_t write_join(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	ARG_UNUSED(conn); ARG_UNUSED(attr); ARG_UNUSED(flags);
	if (offset != 0 || len != 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	if (*(const uint8_t *)buf == 1 && ssid_len) {
		outcome = phone_net_wifi_set(ssid, ssid_len, password, password_len) == 0 ?
			  OUTCOME_JOINING : OUTCOME_FAILED;
		memset(password, 0, sizeof(password));      /* kept in the settings, not here */
		password_len = 0;
	}
	return len;
}

static ssize_t read_outcome(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			    uint16_t len, uint16_t offset)
{
	uint8_t now = outcome;

	if (now == OUTCOME_JOINING && phone_net_wifi_connected()) {
		now = OUTCOME_JOINED;
	}
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &now, sizeof(now));
}

BT_GATT_SERVICE_DEFINE(strelka_prov,
	BT_GATT_PRIMARY_SERVICE(&uuid_service),
	BT_GATT_CHARACTERISTIC(&uuid_name.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_name, NULL),
	BT_GATT_CHARACTERISTIC(&uuid_password.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_password, NULL),
	BT_GATT_CHARACTERISTIC(&uuid_join.uuid, BT_GATT_CHRC_WRITE, BT_GATT_PERM_WRITE_ENCRYPT,
			       NULL, write_join, NULL),
	BT_GATT_CHARACTERISTIC(&uuid_outcome.uuid, BT_GATT_CHRC_READ, BT_GATT_PERM_READ_ENCRYPT,
			       read_outcome, NULL, NULL),
);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, UUID_BASE(0x0001)),
};

static void stop_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	ble_prov_stop();
}
static K_WORK_DELAYABLE_DEFINE(stop_work, stop_work_fn);

int ble_prov_init(void)
{
	int rc = bt_enable(NULL);

	if (rc) {
		LOG_ERR("Bluetooth LE did not start: %d", rc);
		return rc;
	}
	rc = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (rc) {
		LOG_ERR("advertising did not start: %d", rc);
		return rc;
	}
	running = true;
	k_work_reschedule(&stop_work, K_MINUTES(BLE_PROV_MINUTES));
	LOG_INF("Bluetooth LE on for %d min, for the network's name and password", BLE_PROV_MINUTES);
	return 0;
}

void ble_prov_stop(void)
{
	if (!running) {
		return;
	}
	running = false;
	k_work_cancel_delayable(&stop_work);
	(void)bt_le_adv_stop();
	(void)bt_disable();              /* the radio is the set's now: ESB and the frame */
	LOG_INF("Bluetooth LE off");
}
