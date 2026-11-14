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
	uint8_t kind;           /* a Belka's enum unit_kind; a Povodok's not sent */
	bool card_in;
	uint32_t card_mb, card_free_mb;
	uint8_t card_fs;        /* 0 none, 1 FAT32, 2 exFAT */
	uint8_t cell_pct;
	uint16_t cell_mv;
	int16_t temp_centi;
	uint8_t state, fault;
	uint32_t cal_time;      /* s since 1970; 0 none */
	uint16_t label;         /* a Povodok's: the set's own number, 1–9999; 0 none, not sent */
};

struct strelka_session {    /* 64/1 */
	uint16_t s;
	uint16_t rate_hz;
	uint8_t n;
	uint16_t acc_g;         /* ±16 or 32; 64, 128, 256 or 320 on the hoof's LSM6DSV320X */
	uint16_t gyro_dps;
	uint8_t live;
	uint16_t stop_s;        /* seconds without a SYNC that pause the record */
	uint16_t off_s;         /* seconds without a SYNC that switch a Belka off, 600 by default */
	uint64_t t_us;
	int16_t tz_min;
	bool p_given;           /* from Povodok to the Belkas: the period of t */
	bool net;               /* gathered by the network, not by the set's radio (64/1, `net`) */
	uint32_t p;
	bool units_given;       /* else every unit chosen, the head's units unknown */
	uint8_t units[UNITS_BYTES];  /* the units chosen, bit n unit n */
	uint8_t kinds[KINDS_BYTES];  /* their kinds, 2 bits a unit */
	uint16_t hg;            /* both channels kept: the high-g channel's ±g, 32–320, the
				 * accelerometer's then ±16 g at most; 0 none (RECORD.md, The groups) */
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
	uint16_t range[2];      /* the ranges it was made at, ±g and ±°/s; zeros the session's */
	int16_t hg[6];          /* the high-g channel's readings, as acc's; zeros none */
	uint16_t hg_range;      /* its range then; 0 the session's */
};

#define STRELKA_DIAG_A_PAGE 8u

struct strelka_diag_belka { /* 64/23, a Belka as Povodok knows it */
	uint8_t n;
	int8_t rssi;            /* its last LIVE at this Povodok; REC_NO_SIGNAL none */
	uint32_t ago;           /* periods since its last LIVE */
	uint16_t item[ITEM_LAST + 1];   /* its items as last heard (protocol.h) */
	bool known[ITEM_LAST + 1];
};

struct strelka_diag_band {  /* 64/23's first page: the band */
	uint8_t ch, spare;
	uint8_t margin[STRELKA_POVODOKS_MAX];   /* 0–31 by 2 dB */
	int8_t noise[STRELKA_POVODOKS_MAX][STRELKA_CHANNELS];   /* dBm; INT8_MIN not measured */
	int8_t bnoise[STRELKA_CHANNELS];        /* the Belkas' */
};

struct strelka_calibrate {  /* 64/22, a face asked */
	uint8_t face;           /* 0–5: z up, z down, x up, x down, y up, y down */
	uint16_t acc_g;         /* with face 0: the ranges, as in 64/1, and the time */
	uint16_t gyro_dps;
	uint32_t t;
	uint16_t hg;            /* both channels: the high-g's ±g, calibrated from the same faces */
};

struct strelka_give {       /* 64/19, the key opened */
	uint8_t n;              /* the Povodok's number, 0–3 */
	uint8_t addr[4];
	uint8_t ch;
	uint8_t key[KEY_BYTES];
	uint16_t label;         /* the set's own number, 1–9999; 0 none given */
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
	bool rc_known;          /* the last 64/1's answer from it, when Povodok had one */
	uint8_t rc;
	bool esb_known;         /* the Povodok it takes on ESB, from its item 8 */
	uint8_t esb;
};
struct strelka_fetch {      /* 64/21 */
	uint16_t s;             /* the session */
	unsigned n;             /* the Belkas given; 0 every one taken */
	uint8_t belka[STRELKA_BELKAS_MAX];
	uint32_t from[STRELKA_BELKAS_MAX];   /* each one's first byte, 0 when not given */
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
	int (*card)(uint8_t what, void *ctx);            /* 1 the sessions erased, 2 formatted,
							  * 3 the disk opened for writing */
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
	/* the unit's own long-term X25519 private key, made at its first start, new after its
	 * factory settings: 64/16 gives its public key, which the application pins */
	int (*identity)(uint8_t priv[32], void *ctx);
	int (*password_set)(const uint8_t hash[32], void *ctx);
	int (*table)(const struct strelka_table_piece *piece, void *ctx);
	/* 64/19: the set given to a Povodok, its key already opened under the write's own lock */
	int (*give)(const struct strelka_give *g, void *ctx);
	int (*work)(void *ctx);         /* 64/20: out of the service mode */
	int (*fetch)(const struct strelka_fetch *f, void *ctx);   /* 64/21 */
	/* 64/22: the write, a face asked; the read, its answer, after the sixth taken *done and
	 * the calibration */
	int (*calibrate)(const struct strelka_calibrate *q, void *ctx);
	int (*calibrated)(uint8_t *face, uint8_t *got, bool *done, struct strelka_cal *cal,
			  void *ctx);
	/* 64/24: the set moved to the channel given, at rest */
	int (*move)(uint8_t channel, void *ctx);
	/* 64/23: the Belkas from `from`, at most max, and the band */
	int (*diag)(uint32_t from, struct strelka_diag_band *band, struct strelka_diag_belka *list,
		    unsigned max, unsigned *n, bool *more, void *ctx);
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
