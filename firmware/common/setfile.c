/* NIC-Strelka — the set given to a Belka by its cable (setfile.h). */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "protocol.h"
#include "setfile.h"

static int hex_digit(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

/* exactly `digits` hex digits into `out`, most significant first */
static bool hex(const char *s, size_t n, size_t digits, uint8_t *out)
{
	if (n != digits) {
		return false;
	}
	for (size_t i = 0; i < digits; i += 2) {
		int hi = hex_digit(s[i]), lo = hex_digit(s[i + 1]);

		if (hi < 0 || lo < 0) {
			return false;
		}
		out[i / 2] = (uint8_t)(hi << 4 | lo);
	}
	return true;
}

static bool decimal(const char *s, size_t n, unsigned max, unsigned *out)
{
	unsigned v = 0;

	if (n == 0 || n > 3) {
		return false;
	}
	for (size_t i = 0; i < n; i++) {
		if (s[i] < '0' || s[i] > '9') {
			return false;
		}
		v = v * 10 + (unsigned)(s[i] - '0');
	}
	*out = v;
	return v <= max;
}

static bool space(char c)
{
	return c == ' ' || c == '\t' || c == '\r';
}

int setfile_parse(const char *text, size_t len, struct setfile *out)
{
	unsigned seen = 0;          /* a bit a key: set, channel, key, number */
	size_t at = 0;

	memset(out, 0, sizeof(*out));
	if (len > SETFILE_MAX) {
		return -1;                       /* longer than a unit reads: never taken cut */
	}
	while (at < len) {
		size_t end = at, k0, k1, v0, v1, eq;
		unsigned v;

		while (end < len && text[end] != '\n') {
			end++;
		}
		k0 = at;
		at = end + 1;
		while (k0 < end && space(text[k0])) {
			k0++;
		}
		if (k0 == end || text[k0] == '#') {
			continue;                /* blank or a comment */
		}
		for (eq = k0; eq < end && text[eq] != '='; eq++) {
		}
		if (eq == end) {
			return -1;
		}
		for (k1 = eq; k1 > k0 && space(text[k1 - 1]); k1--) {
		}
		for (v0 = eq + 1; v0 < end && space(text[v0]); v0++) {
		}
		for (v1 = end; v1 > v0 && space(text[v1 - 1]); v1--) {
		}
#define KEY_IS(s) (k1 - k0 == sizeof(s) - 1 && memcmp(text + k0, s, sizeof(s) - 1) == 0)
		if (KEY_IS("set") && !(seen & 1)) {
			uint8_t b[4];

			if (!hex(text + v0, v1 - v0, 8, b)) {
				return -1;
			}
			out->address = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 |
				       (uint32_t)b[2] << 8 | b[3];
			seen |= 1;
		} else if (KEY_IS("channel") && !(seen & 2)) {
			if (!decimal(text + v0, v1 - v0, STRELKA_SET_CHANNELS - 1, &v)) {
				return -1;
			}
			out->channel = (uint8_t)v;
			seen |= 2;
		} else if (KEY_IS("key") && !(seen & 4)) {
			if (!hex(text + v0, v1 - v0, 32, out->key)) {
				return -1;
			}
			seen |= 4;
		} else if (KEY_IS("number") && !(seen & 8)) {
			if (!decimal(text + v0, v1 - v0, STRELKA_BELKAS_MAX, &v) || v == 0) {
				return -1;
			}
			out->number = (uint8_t)v;
			seen |= 8;
		} else {
			return -1;               /* unknown, or twice */
		}
#undef KEY_IS
	}
	return seen == 15 && out->address != 0 ? 0 : -1;
}

size_t setfile_write(char *out, size_t cap, const struct setfile *s)
{
	char key[33];
	int n;

	for (int i = 0; i < 16; i++) {
		snprintf(key + 2 * i, 3, "%02X", s->key[i]);
	}
	n = snprintf(out, cap,
		     "# NIC-Strelka: the set of this Belka, taken when its cable leaves, then wiped\n"
		     "set = %08lX\nchannel = %u\nkey = %s\nnumber = %u\n",
		     (unsigned long)s->address, s->channel, key, s->number);
	return n > 0 && (size_t)n < cap ? (size_t)n : 0;
}
