/* NIC-Strelka — the fuel gauge (gauge.h). The library expects a current positive while the cell
 * discharges, Zephyr's sensor the opposite, as the SDK's sample turns it. Its calls are made under
 * one lock, the library being reached from the main loop, the sensor's thread and the work queue.
 * Built against nRF Connect SDK v3.4.0, not run on hardware. */
#include <math.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm13xx_charger.h>
#include <zephyr/logging/log.h>
#include <nrf_fuel_gauge.h>
#include "gauge.h"
#include "pmic.h"

LOG_MODULE_REGISTER(gauge, LOG_LEVEL_INF);

/* the cell's model: the SDK sample's until the cell is profiled (gauge.h) */
static const struct battery_model model = {
#include "battery_model.inc"
};

/* BCHGCHARGESTATUS (the nPM1300's product specification v1.1, 6.2.14) */
#define CHG_COMPLETE  BIT(1)
#define CHG_TRICKLE   BIT(2)
#define CHG_CC        BIT(3)
#define CHG_CV        BIT(4)

static bool running;
static float soc, volts, celsius;
static int32_t status_seen = -1;
static bool vbus_seen;
static int64_t last_ms;
static unsigned under_s;         /* seconds under the end voltage, in a row */
static K_MUTEX_DEFINE(lock);

#define EMPTY_S 10

static float fl(const struct sensor_value *v)
{
	return (float)v->val1 + (float)v->val2 / 1000000.0f;
}

static int read(float *v, float *i, float *t, int32_t *status)
{
	int32_t error;
	int rc = pmic_measure(v, i, t, status, &error);

	*i = -*i;
	return rc;
}

static void tell_charge(int32_t status)
{
	union nrf_fuel_gauge_ext_state_info_data info;

	if (status & CHG_COMPLETE) {
		info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_COMPLETE;
	} else if (status & CHG_TRICKLE) {
		info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_TRICKLE;
	} else if (status & CHG_CC) {
		/* the charge may be held under its setting by the USB input's limit */
		info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CC_LIMITED;
	} else if (status & CHG_CV) {
		info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_CV;
	} else {
		info.charge_state = NRF_FUEL_GAUGE_CHARGE_STATE_IDLE;
	}
	nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_STATE_CHANGE, &info);
}

int gauge_init(const uint8_t *state, size_t len)
{
	struct nrf_fuel_gauge_init_parameters p =
		NRF_FUEL_GAUGE_DEFAULT_INIT_PARAMETERS_SECONDARY(0.0f, 0.0f, 0.0f, &model);
	struct sensor_value x;
	float charge_a;
	int32_t status;

	if (!pmic_present() || read(&p.v0, &p.i0, &p.t0, &status) != 0) {
		return -ENODEV;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (state != NULL && len == nrf_fuel_gauge_state_size &&
	    nrf_fuel_gauge_state_compatible_check(state, len) == 0) {
		p.state = state;
		p.state_size = len;
	}
	if (nrf_fuel_gauge_init(&p, NULL) != 0) {
		k_mutex_unlock(&lock);
		return -EINVAL;
	}
	/* the charge's own current and its end, a tenth of it (the devicetree's charger node) */
	sensor_channel_get(pmic_charger(), SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, &x);
	charge_a = fl(&x);
	nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_CHARGE_CURRENT_LIMIT,
		&(union nrf_fuel_gauge_ext_state_info_data){ .charge_current_limit = charge_a });
	nrf_fuel_gauge_ext_state_update(NRF_FUEL_GAUGE_EXT_STATE_INFO_TERM_CURRENT,
		&(union nrf_fuel_gauge_ext_state_info_data){ .charge_term_current = charge_a / 10.0f });
	tell_charge(status);
	status_seen = status;
	volts = p.v0;
	celsius = p.t0;
	nrf_fuel_gauge_soc_get(&soc);
	last_ms = k_uptime_get();
	running = true;
	k_mutex_unlock(&lock);
	LOG_INF("fuel gauge %s, %s state", nrf_fuel_gauge_version, p.state ? "a kept" : "a new");
	return 0;
}

bool gauge_present(void)
{
	return running;
}

void gauge_service(void)
{
	struct nrf_fuel_gauge_state_info info;
	float v, i, t;
	int32_t status;
	int64_t now = k_uptime_get();
	bool vbus = pmic_vbus();

	if (!running || read(&v, &i, &t, &status) != 0) {
		return;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (vbus != vbus_seen) {
		nrf_fuel_gauge_ext_state_update(vbus ? NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_CONNECTED
						     : NRF_FUEL_GAUGE_EXT_STATE_INFO_VBUS_DISCONNECTED,
						NULL);
		vbus_seen = vbus;
	}
	if (status != status_seen) {
		tell_charge(status);
		status_seen = status;
	}
	{
		float s;

		if (nrf_fuel_gauge_process(v, i, t, (float)(now - last_ms) / 1000.0f, &s, &info) == 0) {
			soc = s;
		}
	}
	last_ms = now;
	volts = v;
	celsius = t;
	under_s = v * 1000.0f < (float)CONFIG_STRELKA_CELL_EMPTY_MV ? under_s + 1u : 0u;
	k_mutex_unlock(&lock);
}

uint8_t gauge_pct(void)
{
	float s = soc < 0.0f ? 0.0f : soc > 100.0f ? 100.0f : soc;

	return (uint8_t)lroundf(s);
}

uint16_t gauge_mv(void)
{
	return (uint16_t)lroundf(volts * 1000.0f);
}

int16_t gauge_temp_centi(void)
{
	return (int16_t)lroundf(celsius * 100.0f);
}

bool gauge_empty(void)
{
	return running && under_s >= EMPTY_S;
}

size_t gauge_state(uint8_t *buf, size_t size)
{
	int rc;

	if (!running || size < nrf_fuel_gauge_state_size) {
		return 0;
	}
	k_mutex_lock(&lock, K_FOREVER);
	rc = nrf_fuel_gauge_state_get(buf, size);
	k_mutex_unlock(&lock);
	return rc == 0 ? nrf_fuel_gauge_state_size : 0;
}
