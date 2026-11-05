/* NIC-Strelka — what a unit keeps across power-ons (store.h). Built against nRF Connect SDK
 * v3.4.0, not run on hardware: the kit decides it. */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "store.h"

LOG_MODULE_REGISTER(store, LOG_LEVEL_INF);

struct store store;
static K_MUTEX_DEFINE(lock);

/* a key read whole into its place, or left as it was; a length of another build's is ignored */
static int read_into(void *dst, size_t want, size_t len, settings_read_cb read_cb, void *arg,
		     bool *loaded)
{
	if (len != want) {
		return 0;
	}
	if (read_cb(arg, dst, len) != (ssize_t)len) {
		return -EIO;
	}
	if (loaded != NULL) {
		*loaded = true;
	}
	return 0;
}

static int on_load(const char *key, size_t len, settings_read_cb read_cb, void *arg)
{
	if (strcmp(key, "set") == 0) {
		return read_into(&store.set, sizeof(store.set), len, read_cb, arg, NULL);
	}
	if (strcmp(key, "n") == 0) {
		return read_into(&store.number, sizeof(store.number), len, read_cb, arg, NULL);
	}
	if (strcmp(key, "cal") == 0) {
		return read_into(&store.cal, sizeof(store.cal), len, read_cb, arg, &store.cal_loaded);
	}
	if (strcmp(key, "gauge") == 0) {
		bool got = false;
		int rc = len <= sizeof(store.gauge)
			? read_into(store.gauge, len, len, read_cb, arg, &got) : 0;

		store.gauge_len = got ? (uint16_t)len : 0u;
		return rc;
	}
	if (strcmp(key, "pw") == 0) {
		return read_into(store.pw, sizeof(store.pw), len, read_cb, arg, &store.pw_loaded);
	}
	if (strcmp(key, "label") == 0) {
		return read_into(&store.label, sizeof(store.label), len, read_cb, arg, NULL);
	}
	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(strelka, "strelka", NULL, on_load, NULL, NULL);

int store_load(void)
{
	int rc = settings_subsys_init();

	if (rc) {
		LOG_WRN("the settings unreadable (%d): the unit as from the factory", rc);
		return rc;
	}
	rc = settings_load_subtree("strelka");
	if (store.set.channel >= STRELKA_SET_CHANNELS) {
		store.set.channel = 0;   /* a value no build writes */
	}
	LOG_INF("set %08x, channel %u, number %u%s", store.set.address, store.set.channel,
		store.number, store.cal_loaded ? ", calibrated" : "");
	return rc;
}

static int save(const char *key, const void *value, size_t len)
{
	int rc = settings_save_one(key, value, len);

	if (rc) {
		LOG_WRN("%s not saved (%d)", key, rc);
	}
	return rc;
}

int store_join(const struct store_set *set, uint8_t number)
{
	int rc;

	k_mutex_lock(&lock, K_FOREVER);
	store.set = *set;
	store.number = number;
	rc = save("strelka/set", &store.set, sizeof(store.set));
	if (rc == 0) {
		rc = save("strelka/n", &store.number, sizeof(store.number));
	}
	k_mutex_unlock(&lock);
	return rc;
}

/* a move may come a second after the last in a noisy place (link_rules.h), so the channel is
 * kept at most once a minute, the latest one; a unit started on an older one finds the set by
 * the sweep it makes when it hears no SYNC (PRINCIPLE.md, The channels) */
#define CHANNEL_KEEP_MS (60 * MSEC_PER_SEC)

int store_channel(uint8_t channel)
{
	static int64_t kept_at = -CHANNEL_KEEP_MS;
	int rc = 0;

	if (channel >= STRELKA_SET_CHANNELS) {
		return -EINVAL;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (store.set.address != 0 && store.set.channel != channel &&
	    k_uptime_get() - kept_at >= CHANNEL_KEEP_MS) {
		store.set.channel = channel;
		rc = save("strelka/set", &store.set, sizeof(store.set));
		if (rc == 0) {
			kept_at = k_uptime_get();
			LOG_INF("the set's channel %u kept", channel);
		}
	}
	k_mutex_unlock(&lock);
	return rc;
}

int store_cal(const struct strelka_cal *cal)
{
	int rc;

	k_mutex_lock(&lock, K_FOREVER);
	store.cal = *cal;
	store.cal_loaded = true;
	rc = save("strelka/cal", &store.cal, sizeof(store.cal));
	k_mutex_unlock(&lock);
	return rc;
}

int store_gauge(const uint8_t *state, size_t len)
{
	int rc = 0;

	if (len > sizeof(store.gauge)) {
		return -EINVAL;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (store.gauge_len != len || memcmp(store.gauge, state, len) != 0) {
		memcpy(store.gauge, state, len);
		store.gauge_len = (uint16_t)len;
		rc = save("strelka/gauge", store.gauge, len);
	}
	k_mutex_unlock(&lock);
	return rc;
}

int store_pw(const uint8_t hash[32])
{
	int rc;

	k_mutex_lock(&lock, K_FOREVER);
	memcpy(store.pw, hash, sizeof(store.pw));
	store.pw_loaded = true;
	rc = save("strelka/pw", store.pw, sizeof(store.pw));
	k_mutex_unlock(&lock);
	return rc;
}

int store_label(uint16_t label)
{
	int rc;

	k_mutex_lock(&lock, K_FOREVER);
	store.label = label;
	rc = label ? save("strelka/label", &store.label, sizeof(store.label))
		   : settings_delete("strelka/label");
	k_mutex_unlock(&lock);
	return rc;
}

void store_forget(void)
{
	static const char *const keys[] = {
		"strelka/set", "strelka/n", "strelka/cal", "strelka/gauge", "strelka/pw",
		"strelka/label",
	};

	k_mutex_lock(&lock, K_FOREVER);
	for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
		settings_delete(keys[i]);
	}
	memset(&store, 0, sizeof(store));
	k_mutex_unlock(&lock);
	LOG_INF("the factory settings: the set, the number, the calibration, the gauge's state "
		"and the password forgotten");
}
