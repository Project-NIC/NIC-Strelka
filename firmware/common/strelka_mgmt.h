/* NIC-Strelka — Strelka's own SMP group, 64 (COMMANDS.md, Strelka's group): each command's
 * request read from its CBOR map and its answer written, the unit's own doings behind the
 * `strelka_ops` the unit gives. The CBOR is zcbor's; on a unit MCUmgr opens and closes the
 * answer's map round the handler and carries Zephyr's code, so this writes the map's pairs alone
 * and returns the code (mgmt_defines.h): 0 all well, else the answer is {"rc": code} — or, a
 * group's own code written here as {"err": {"group": 64, "rc": code}} and 0 returned. Plain
 * C11, compiled on the host for the tests (tests/test_smp.c, held against Python's cbor2) and
 * into both units (shared/strelka_mgmt_zephyr.c). */
#ifndef STRELKA_MGMT_H
#define STRELKA_MGMT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#include "crypto.h"
#include "protocol.h"
#include "smp.h"

/* an ops function returns Zephyr's code, 0 all well; a group's own code as STRELKA_RC(code) */
#define STRELKA_RC(code)      (256 + (code))
#define STRELKA_RC_IS_OWN(rc) ((rc) >= 256)

struct strelka_info {       /* 64/0 */
	uint64_t chip;
	uint8_t n;
	uint8_t who;            /* WHO_POVODOK or WHO_BELKA, the who byte's kind bits */
	uint8_t fw[3], build[4];
	uint8_t sensor;         /* WHO_AM_I; 0 a Povodok */
	bool card_in;
	uint32_t card_mb, card_free_mb;
	uint8_t card_fs;        /* 0 none, 1 FAT32, 2 exFAT */
	uint8_t cell_pct;
	uint16_t cell_mv;
	int16_t temp_centi;
	uint8_t state, fault;
	uint32_t cal_time;      /* s since 1970; 0 none */
};

struct strelka_session {    /* 64/1 */
	uint16_t s;
	uint16_t rate_hz;
	uint8_t n;
	uint8_t acc_g;
	uint16_t gyro_dps;
	uint8_t live;
	uint16_t stop_s;
	uint64_t t_us;
	int16_t tz_min;
	bool p_given;           /* from Povodok to the Belkas: the period of t */
	uint32_t p;
};

#define STRELKA_FILE_NAME_MAX 31
struct strelka_session_entry { /* 64/2 */
	uint16_t s;
	char file[STRELKA_FILE_NAME_MAX + 1];
	uint32_t steps, blocks, p0;
	uint8_t end;
	uint32_t bytes;
};
#define STRELKA_SESSIONS_A_PAGE 16u

struct strelka_cal {        /* 64/3, the head's fields (RECORD.md) */
	int16_t acc[6];
	uint16_t gscale[3];
	int16_t gzero[3];
	int16_t temp;
	uint32_t t;
};

struct strelka_join {       /* 64/9 */
	uint8_t n;
	uint8_t addr[4];
	uint8_t ch;
	uint8_t key[KEY_BYTES]; /* the set's key under AES-CCM with the agreed key */
	uint8_t nonce[13];
	uint8_t mic[4];
};

struct strelka_found {      /* 64/12 read */
	uint64_t chip;
	int8_t rssi;
	uint32_t ago_s;
};
#define STRELKA_FOUND_MAX 32u

struct strelka_set_belka {  /* 64/14 */
	uint8_t n;
	uint64_t chip;
	uint32_t ago;           /* periods since its last LIVE */
	uint8_t by;             /* 0 heard by the first, 1–3 passed by that Povodok */
	uint8_t state, fault;
};
struct strelka_set {
	struct strelka_set_belka belka[STRELKA_BELKAS_MAX];
	unsigned n;
	uint8_t povodoks;       /* a bit for each heard */
	uint8_t ch, spare;
};

struct strelka_table_piece { /* 64/18 */
	uint32_t off, len;
	const uint8_t *csv;
	size_t csv_len;
};
#define STRELKA_TABLE_PIECE_MAX 1900u

struct strelka_ops {        /* what the unit does; NULL for a command it has not */
	int (*info)(struct strelka_info *out, void *ctx);
	int (*session)(const struct strelka_session *s, void *ctx);
	int (*sessions)(uint32_t from, struct strelka_session_entry *list, unsigned max,
			unsigned *n, bool *more, void *ctx);
	int (*cal_write)(const struct strelka_cal *cal, void *ctx);
	int (*cal_read)(struct strelka_cal *cal, void *ctx);
	int (*blink)(uint32_t seconds, void *ctx);
	int (*card)(uint8_t what, void *ctx);            /* 1 the sessions erased, 2 formatted */
	int (*off)(void *ctx);
	int (*forget)(void *ctx);
	int (*key)(const uint8_t pub[32], uint8_t pub_out[32], void *ctx);
	int (*join)(const struct strelka_join *j, void *ctx);
	int (*record)(bool run, void *ctx);
	int (*live)(uint8_t code, void *ctx);
	int (*search)(bool run, void *ctx);
	int (*found)(struct strelka_found *list, unsigned max, unsigned *n, void *ctx);
	int (*take)(uint64_t chip, uint8_t n, void *ctx);
	int (*set)(struct strelka_set *out, void *ctx);
	int (*drop)(uint8_t n, void *ctx);
	int (*password_hash)(uint8_t hash[32], void *ctx);        /* the stored SHA-256 */
	int (*password_set)(const uint8_t hash[32], void *ctx);
	int (*table)(const struct strelka_table_piece *piece, void *ctx);
};

/* a request of group 64 handled: `dec` at the body's start, `enc` inside the answer's map;
 * `lock` the connection's, NULL where there is none (a Belka, reached through the set's signed
 * fragments alone). Returns Zephyr's code. */
int strelka_mgmt_handle(uint8_t op, uint8_t id, zcbor_state_t *dec, zcbor_state_t *enc,
			const struct strelka_ops *ops, struct smp_lock *lock, void *ctx);

/* a whole frame handled on the host and in the simulators: the lock checked, the header
 * answered, the body decoded and encoded; groups other than 64 answer 8, not supported, since
 * MCUmgr serves them on a unit. The answer's bytes written, 0 when `cap` is short or the frame
 * is not a request. */
size_t strelka_mgmt_frame(const uint8_t *frame, size_t len, uint8_t *out, size_t cap,
			  const struct strelka_ops *ops, struct smp_lock *lock, void *ctx);

#endif
