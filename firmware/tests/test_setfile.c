/* NIC-Strelka — the set given to a Belka by its cable (common/setfile.c), on the host: the file
 * tools/strelka_set.py writes, read as the firmware reads it; the firmware's own writing read back;
 * a file with a key missing, one twice, a value out of range or an unknown line refused. */
#include <string.h>
#include "test.h"
#include "setfile.h"

static const uint8_t KEY[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

static void check_reference(const struct setfile *s)
{
	CHECK(s->address == 0xA1B2C3D4u && s->channel == 7 && s->number == 5);
	CHECK(memcmp(s->key, KEY, 16) == 0);
}

static void test_python(const char *path)
{
	char text[SETFILE_MAX];
	struct setfile s;
	FILE *f = fopen(path, "rb");
	size_t n;

	CHECK(f != NULL);
	n = fread(text, 1, sizeof(text), f);
	fclose(f);
	CHECK(n > 0 && setfile_parse(text, n, &s) == 0);
	check_reference(&s);
	OK("the file Python writes, read as the Belka reads it");
}

static void test_own_and_refused(void)
{
	struct setfile s = { .address = 0xA1B2C3D4u, .channel = 7, .number = 5 }, r;
	char text[SETFILE_MAX];
	size_t n;

	memcpy(s.key, KEY, 16);
	n = setfile_write(text, sizeof(text), &s);
	CHECK(n > 0 && setfile_parse(text, n, &r) == 0);
	check_reference(&r);
	/* by hand: another order, lower case, spaces, a comment, no newline at the end */
	{
		const char *hand = "number=5\n  # a note\r\n\tkey = 0102030405060708090a0b0c0d0e0f10 \n"
				   "channel =7\nset= a1b2c3d4";

		CHECK(setfile_parse(hand, strlen(hand), &r) == 0);
		check_reference(&r);
	}
	{
		static const char *bad[] = {
			"set = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n",
			"set = A1B2C3D4\nset = A1B2C3D4\nchannel = 7\n"
			"key = 0102030405060708090A0B0C0D0E0F10\nnumber = 5\n",
			"set = A1B2C3D4\nchannel = 39\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 5\n",
			"set = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 64\n",
			"set = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F\nnumber = 5\n",
			"set = 00000000\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 5\n",
			"set = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 5\ncolour = red\n",
			"set A1B2C3D4\n",
			"set = A1B2C3D4\nchannel = 0007\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 5\n",
			"SET = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n"
			"number = 5\n",
		};

		for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
			CHECK(setfile_parse(bad[i], strlen(bad[i]), &r) != 0);
		}
	}
	{	/* a good file made longer than SETFILE_MAX by a comment: refused whole */
		static char longer[SETFILE_MAX + 2];
		const char *good = "set = A1B2C3D4\nchannel = 7\nkey = 0102030405060708090A0B0C0D0E0F10\n"
				   "number = 5\n# ";

		memset(longer, 'x', sizeof(longer) - 1);
		memcpy(longer, good, strlen(good));
		CHECK(setfile_parse(longer, SETFILE_MAX, &r) == 0);
		CHECK(setfile_parse(longer, SETFILE_MAX + 1, &r) != 0);
	}
	OK("its own writing read back, a hand's read, a key missing or twice or out of range, four "
	   "digits, a key in capitals, a file over 512 B refused");
}

int main(int argc, char **argv)
{
	CHECK(argc == 2);
	test_python(argv[1]);
	test_own_and_refused();
	return 0;
}
