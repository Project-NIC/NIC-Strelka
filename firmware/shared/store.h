/* NIC-Strelka — what a unit keeps across power-ons, one home for the Belka and Povodok
 * (FIRMWARE.md, What a unit keeps): Zephyr's settings on ZMS in the storage partition, under
 * "strelka/". The set the unit belongs to — its address, channel and key — and the unit's
 * number, saved at the taking, the set's channel again as the set moves, once a minute at most,
 * so that a unit switched on later finds the set where it went; a Belka's calibration; the
 * fuel gauge's state; Povodok's password's hash. The factory settings forget them all and
 * keep the records. Built against nRF Connect SDK v3.4.0, not run on hardware: the kit decides
 * it. */
#ifndef STRELKA_STORE_H
#define STRELKA_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
#include "strelka_mgmt.h"
#include "gauge.h"

struct store_set {               /* "strelka/set": the set the unit belongs to; address 0 none */
	uint32_t address;
	uint8_t channel;         /* 0–38 */
	uint8_t key[KEY_BYTES];
};

struct store {
	struct store_set set;
	uint8_t number;          /* "strelka/n": a Belka's 1–63, 0 a new one; a Povodok's 0–3 */
	struct strelka_cal cal;  /* "strelka/cal", a Belka's; t 0 none */
	bool cal_loaded;
	uint8_t gauge[GAUGE_STATE_MAX];  /* "strelka/gauge": the fuel gauge's state */
	uint16_t gauge_len;              /* 0 none */
	uint8_t pw[32];          /* "strelka/pw", Povodok's: the application's password's SHA-256 */
	bool pw_loaded;
	uint16_t label;          /* "strelka/label", Povodok's: the set's own number, 1–9999; 0 none */
	uint8_t id[32];          /* "strelka/id", Povodok's own long-term X25519 private key, which
				  * the application pins (COMMANDS.md, The lock) */
	bool id_loaded;
};

/* read by every thread; written only through the calls below, one at a time */
extern struct store store;

int store_load(void);                                  /* at the start, before the rest */
int store_join(const struct store_set *set, uint8_t number);   /* the taking */
int store_channel(uint8_t channel);   /* the set's now: kept if it changed, once a minute at most */
int store_cal(const struct strelka_cal *cal);
int store_gauge(const uint8_t *state, size_t len);
int store_pw(const uint8_t hash[32]);
int store_label(uint16_t label);      /* 64/19's; 0 deletes it */
/* Povodok's own key: made at its first start and kept, new after the factory settings */
int store_identity(uint8_t priv[32]);
void store_forget(void);         /* the factory settings: every key above deleted */

#endif
