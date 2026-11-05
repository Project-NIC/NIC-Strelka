/* NIC-Strelka — the sensor from the main core's side while the FLPR reads it (FIRMWARE.md, The
 * sensor on the FLPR): imu.c's calls, answered by the ring's head and the commands written into
 * it, and the samples taken out of the ring with their stamps made 64-bit (common/sensor_ring.h).
 * The FLPR's own program is firmware/flpr. Built against nRF Connect SDK v3.4.0, not run on
 * hardware: the kit decides it. */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include "imu.h"
#include "imu_tables.h"
#include "sensor_ring.h"
#include "clock.h"

#define RING_NODE DT_NODELABEL(sensor_ring)
BUILD_ASSERT(DT_REG_SIZE(RING_NODE) >= sizeof(struct ring), "the ring's region holds it");
static struct ring *const r = (struct ring *)DT_REG_ADDR(RING_NODE);

static uint8_t kind;

/* the head cleared before the FLPR runs: Nordic's launcher starts it at POST_KERNEL */
static int ring_clear(void)
{
	memset((void *)&r->head, 0, sizeof(r->head));
	r->head.sensor = RING_NOT_YET;
	return 0;
}
SYS_INIT(ring_clear, PRE_KERNEL_1, 0);

/* a command written, its number last; done when the FLPR echoes the number */
static int send(uint8_t cmd)
{
	uint8_t seq = (uint8_t)(r->head.cmd_seq + 1);

	r->head.cmd = cmd;
	RING_FENCE();
	r->head.cmd_seq = seq;
	for (int ms = 0; ms < 200; ms++) {
		if (r->head.done_seq == seq) {
			RING_FENCE();
			return r->head.state == RING_FAULT ? -EIO : 0;
		}
		k_msleep(1);
	}
	return -ETIMEDOUT;
}

int imu_init(void)
{
	for (int ms = 0; ms < 500 && r->head.sensor == RING_NOT_YET; ms++) {
		k_msleep(1);
	}
	RING_FENCE();
	kind = r->head.sensor == RING_NOT_YET ? 0 : imu_kind(r->head.sensor);
	return kind ? 0 : -ENODEV;
}

uint8_t imu_sensor(void)
{
	return kind;
}

uint8_t imu_whoami(void)
{
	return r->head.sensor == RING_NOT_YET ? 0 : r->head.sensor;
}

uint16_t imu_acc_g_max(void)
{
	return imu_acc_g_max_of(kind);
}

bool imu_on_grid(void)
{
	return imu_on_grid_of(kind);
}

uint16_t imu_sensor_hz(uint16_t rate_hz)
{
	return imu_sensor_hz_of(kind, rate_hz);
}

uint16_t imu_acc_full_scale(uint16_t acc_g)
{
	return imu_acc_full_scale_of(kind, acc_g);
}

uint16_t imu_gyro_dps_max(void)
{
	return imu_gyro_dps_max_of(kind);
}

uint16_t imu_gyro_full_scale(uint16_t gyro_dps)
{
	return imu_gyro_full_scale_of(kind, gyro_dps);
}

bool imu_rate_ok(uint16_t rate_hz)
{
	return imu_rate_ok_of(kind, rate_hz);
}

bool imu_ranges_ok(uint16_t acc_g, uint16_t gyro_dps)
{
	return imu_ranges_ok_of(kind, acc_g, gyro_dps);
}

int imu_configure(const struct imu_config *cfg)
{
	if (!imu_rate_ok_of(kind, cfg->rate_hz) ||
	    !imu_ranges_ok_of(kind, cfg->acc_g, cfg->gyro_dps)) {
		return -EINVAL;
	}
	r->head.rate_hz = cfg->rate_hz;
	r->head.acc_g = cfg->acc_g;
	r->head.gyro_125 = (uint8_t)(cfg->gyro_dps / 125u);
	return send(RING_CMD_SET);
}

int imu_start(void)
{
	return send(RING_CMD_START);
}

int imu_stop(void)
{
	return send(RING_CMD_STOP);
}

int imu_read_step(struct strelka_step *step, bool *acc_out, bool *gyro_out)
{
	ARG_UNUSED(step);
	ARG_UNUSED(acc_out);
	ARG_UNUSED(gyro_out);
	return -ENOTSUP;    /* the FLPR reads: imu_ring_pop */
}

int16_t imu_last_temp(void)
{
	return 0;           /* the register's raw value stays on the FLPR */
}

int16_t imu_temp_centi(void)
{
	return ring_temp_centi(r->head.temp_half);
}

int imu_read_fifo(struct strelka_step *steps, int max_steps)
{
	ARG_UNUSED(steps);
	ARG_UNUSED(max_steps);
	return -ENOTSUP;
}

int imu_ring_pop(struct strelka_step *step, uint64_t *stamp, bool *acc_out, bool *gyro_out)
{
	struct ring_sample s;

	if (!ring_pop(r, &s)) {
		return 0;
	}
	*stamp = ring_extend(clock_now(), s.stamp);
	*acc_out = *gyro_out = false;
	for (int i = 0; i < 3; i++) {
		step->a[i] = s.a[i];
		step->g[i] = s.g[i];
		if (s.a[i] == INT16_MAX || s.a[i] == INT16_MIN) {
			*acc_out = true;
		}
		if (s.g[i] == INT16_MAX || s.g[i] == INT16_MIN) {
			*gyro_out = true;
		}
	}
	return 1;
}

uint32_t imu_dropped(void)
{
	return (uint32_t)r->head.lost + r->head.missed;
}
