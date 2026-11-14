/* NIC-Strelka — the sensor's samples from the FLPR to the main core: 32 B of head, then 1 024
 * samples of 16 B, in RAM both cores see, outside either one's own (FIRMWARE.md, The sensor on
 * the FLPR). The FLPR writes the samples and its fields of the head, the main core its fields
 * and the count it has read; neither writes the other's. Plain C, checked on the host
 * (tests/test_ring.c). */
#ifndef STRELKA_SENSOR_RING_H
#define STRELKA_SENSOR_RING_H

#include <stddef.h>
#include <stdint.h>

#define RING_N           1024u     /* samples; a power of two, 32 ms at 32 kHz */

/* byte 0, the FLPR's state */
#define RING_OFF         0         /* running, the sensor not set */
#define RING_SET         1         /* set: pin 9 may be driven */
#define RING_RUN         2         /* measuring */
#define RING_FAULT       3
/* byte 1 before the FLPR has looked for the sensor: the main core writes it before the FLPR runs */
#define RING_NOT_YET     0xFF
/* byte 2, the main core's command */
#define RING_CMD_NONE    0
#define RING_CMD_SET     1
#define RING_CMD_START   2
#define RING_CMD_STOP    3

struct ring_head {                      /* bytes, the writer */
	volatile uint8_t state;         /* 0, FLPR */
	volatile uint8_t sensor;        /* 1, FLPR: WHO_AM_I, 0 none, RING_NOT_YET */
	volatile uint8_t cmd;           /* 2, main */
	volatile uint8_t cmd_seq;       /* 3, main: one up with every command */
	volatile uint8_t done_seq;      /* 4, FLPR: the last command's, once done */
	volatile int8_t temp_half;      /* 5, FLPR: ½ °C from 25 °C */
	volatile uint16_t rate_hz;      /* 6-7, main: the sensor's rate */
	volatile uint8_t n;             /* 8, main: the samples a step, which the LSM6DSV320X's own
					 * rate and filters follow */
	volatile uint8_t gyro_125;      /* 9, main: ±°/s / 125, 2 to 32 */
	volatile uint16_t lost;         /* 10-11, FLPR: samples dropped, the ring full */
	volatile uint32_t written;      /* 12-15, FLPR */
	volatile uint32_t read;         /* 16-19, main */
	volatile uint16_t missed;       /* 20-21, FLPR: data-readies passed between two reads */
	volatile uint16_t acc_g;        /* 22-23, main: ±g, up to 320 on the hoof's sensor */
	volatile uint16_t hg;           /* 24-25, main: both channels kept, the high-g's ±g; 0 none */
	uint8_t spare[6];               /* 26-31 */
};

struct ring_sample {
	uint32_t stamp;                 /* the data-ready, TIMER22's capture, 16 MHz ticks */
	int16_t a[3];                   /* the accelerometer x, y, z, as the sensor gives them */
	int16_t g[3];                   /* the gyro x, y, z */
	int16_t hg[3];                  /* the high-g channel's x, y, z, both kept; else zeros */
	uint16_t pad;
};

struct ring {
	struct ring_head head;
	struct ring_sample s[RING_N];
};

_Static_assert(sizeof(struct ring_head) == 32, "the head is 32 B");
_Static_assert(offsetof(struct ring_head, rate_hz) == 6, "rate at 6");
_Static_assert(offsetof(struct ring_head, lost) == 10, "lost at 10");
_Static_assert(offsetof(struct ring_head, written) == 12, "written at 12");
_Static_assert(offsetof(struct ring_head, read) == 16, "read at 16");
_Static_assert(offsetof(struct ring_head, missed) == 20, "missed at 20");
_Static_assert(offsetof(struct ring_head, acc_g) == 22, "the accelerometer's range at 22");
_Static_assert(offsetof(struct ring_head, hg) == 24, "the high-g's range at 24");
_Static_assert(sizeof(struct ring_sample) == 24, "a sample is 24 B");

/* the fence between the data and the count that publishes it, on either core */
#ifndef RING_FENCE
#define RING_FENCE() __sync_synchronize()
#endif

/* the FLPR: a sample in, or counted as lost when the main core has not read for 1 024 */
static inline int ring_push(struct ring *r, const struct ring_sample *s)
{
	uint32_t w = r->head.written;

	if (w - r->head.read >= RING_N) {
		r->head.lost++;
		return -1;
	}
	r->s[w & (RING_N - 1)] = *s;
	RING_FENCE();
	r->head.written = w + 1;
	return 0;
}

/* the main core: the next sample out, 1 if there was one */
static inline int ring_pop(struct ring *r, struct ring_sample *s)
{
	uint32_t rd = r->head.read;

	if (rd == r->head.written) {
		return 0;
	}
	RING_FENCE();
	*s = r->s[rd & (RING_N - 1)];
	RING_FENCE();
	r->head.read = rd + 1;
	return 1;
}

/* the temperature in ½ °C from 25 °C, −39 to +88,5 °C, from and to 0,01 °C */
static inline int8_t ring_temp_half(int32_t centi)
{
	int32_t h = (centi - 2500) / 50;

	return (int8_t)(h < -128 ? -128 : h > 127 ? 127 : h);
}

static inline int16_t ring_temp_centi(int8_t half)
{
	return (int16_t)(2500 + half * 50);
}

/* a 32-bit stamp of TIMER22 into the 64-bit count, taken soon after it: the wrap is 268 s */
static inline uint64_t ring_extend(uint64_t now, uint32_t stamp)
{
	return now - (uint32_t)((uint32_t)now - stamp);
}

#endif
