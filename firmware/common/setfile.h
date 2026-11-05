/* NIC-Strelka — the set given to a Belka by its cable (FIRMWARE.md, What a unit keeps): a short
 * text file, STRELKA.SET, written on the Belka's disk by the application, or by hand, and taken by
 * the Belka when the cable leaves and it has its card back — read, kept, its bytes overwritten and
 * the file deleted, so that the key does not stay on the card:
 *
 *     # NIC-Strelka: the set of this Belka
 *     set = A1B2C3D4                          the set's address, 8 hex digits
 *     channel = 7                             the set's channel, 0–38
 *     key = 0102030405060708090A0B0C0D0E0F10  the set's key, 32 hex digits
 *     number = 5                              the Belka's number, 1–63
 *
 * The lines in any order, `#` a comment, spaces and the case of the hex digits free, every key
 * once. Plain C, checked on the host against tools/strelka_set.py (tests/test_setfile.c). */
#ifndef STRELKA_SETFILE_H
#define STRELKA_SETFILE_H

#include <stddef.h>
#include <stdint.h>

#define SETFILE_NAME  "STRELKA.SET"
#define SETFILE_MAX   512u           /* the most read of it */

struct setfile {
	uint32_t address;
	uint8_t channel;
	uint8_t key[16];
	uint8_t number;
};

/* 0 when every key is there once and in its range; -1 otherwise, *out then undefined */
int setfile_parse(const char *text, size_t len, struct setfile *out);
/* the file as the application writes it; its length, 0 when `cap` is short */
size_t setfile_write(char *out, size_t cap, const struct setfile *s);

#endif
