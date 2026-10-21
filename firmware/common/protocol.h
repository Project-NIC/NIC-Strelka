/* NIC-Strelka — what goes over the air and onto the card, to the byte.
 *
 * Every structure here is a page's: the inside link's packets PRINCIPLE.md's (The inside link),
 * the record's blocks RECORD.md's, the SMP header and the fragment over ESB COMMANDS.md's, the
 * live view's message PHONE.md's. Packed, little-endian but for SMP's header, which is
 * big-endian as Zephyr writes it; every size is asserted, so that a page cannot drift from the
 * code. Plain C11, nothing of Zephyr's: the host compiles it for the tests as the units do.
 */
#ifndef STRELKA_PROTOCOL_H
#define STRELKA_PROTOCOL_H

#include <stdint.h>

#define STRELKA_FW_MAJOR 0            /* the design's version the code follows; the build's hash */
#define STRELKA_FW_MINOR 7            /* tells the rest (RECORD.md, The session's head) */
#define STRELKA_FW_PATCH 0

#define PACKED __attribute__((packed))

/* ---- the set ------------------------------------------------------------------------------- */
#define STRELKA_PERIOD_US      20000u  /* one period: the SYNCs, the slots, the MISS, the PASSes */
#define STRELKA_PERIODS_A_S    50u
#define STRELKA_BELKAS_MAX     63u     /* the Belkas numbered 1–63, slot k the Belka's k */
#define STRELKA_POVODOKS_MAX   4u      /* the Povodoks numbered 0–3, a SYNC slot each */
#define STRELKA_CHANNELS       40u     /* 2402–2480 MHz by 2 MHz, numbered 0–39 */
#define STRELKA_SET_CHANNELS   39u     /* the set works on 0–38; 39, 2480 MHz, is the search's */
#define STRELKA_SEARCH_CHANNEL 39u
#define STRELKA_CHANNEL_MHZ(c) (2402u + 2u * (c))
#define STRELKA_SEARCH_ADDRESS 0x52E6B438u   /* the same in every firmware, for a new Belka */
#define STRELKA_PHY_BIT_S      2000000u      /* 2 Mbit/s on air: 4 µs a byte */
#define STRELKA_US_A_BYTE      4u

/* ---- the inside link's packet ------------------------------------------------------------ */
/* On the air: preamble 2 | address 4 | who 1 | length 1 | S0 1 | period 4 | state 2 | body |
 * CRC 3. The radio sends its fields in the order S0, LENGTH, S1 (the nRF54LM20A datasheet v1.0,
 * 8.17.1), so who rides in the radio's S0, the length in its LENGTH, Strelka's S0 in its S1 of
 * 8 bits; the address is the radio's base of 3 B and its prefix (8.17.2). In RAM the packet
 * begins at who: this structure is the radio's packet pointer's, 258 B at most, the radio's
 * limit on S0, LENGTH, S1 and the payload together (8.17.1). */
#define STRELKA_AIR_PREAMBLE_BYTES 2u   /* PCNF0.PLEN at 16 bits */
#define STRELKA_AIR_ADDRESS_BYTES  4u
#define STRELKA_AIR_CRC_BYTES      3u
#define STRELKA_HEAD_BYTES         15u  /* every byte before the body, the preamble on */
#define STRELKA_RAM_HEAD_BYTES     9u   /* who, length, S0, period, state */
#define STRELKA_BODY_MAX           249u /* the length field counts the period, the state and the
					 * body: 255 B at most */
#define STRELKA_AIR_BYTES(body)    (STRELKA_HEAD_BYTES + (body) + STRELKA_AIR_CRC_BYTES)
#define STRELKA_AIR_US(body)       (STRELKA_AIR_BYTES(body) * STRELKA_US_A_BYTE)

struct PACKED strelka_packet {
	uint8_t who;           /* bits 7–6 the sender, bits 5–0 its number */
	uint8_t length;        /* 6 + the body's bytes */
	uint8_t s0;            /* the high nibble what it carries, the low the state's item */
	uint32_t period;       /* the set's common time, the same 4 B the record carries */
	uint16_t state;        /* from a Povodok the set's, from a Belka its own */
	uint8_t body[STRELKA_BODY_MAX];
};
_Static_assert(sizeof(struct strelka_packet) == 258, "the radio's packet in RAM, 258 B at most");

#define WHO_POVODOK      0x00u      /* bits 7–6: 0 a Povodok, 2 a Belka, 1 and 3 free */
#define WHO_BELKA        0x80u
#define WHO_KIND(who)    ((who) & 0xC0u)
#define WHO_NUMBER(who)  ((who) & 0x3Fu)
#define WHO(kind, n)     ((uint8_t)((kind) | ((n) & 0x3Fu)))

/* S0's high nibble: the live view's code, or the packet's kind */
#define S0_LIVE_OFF      0x0u
#define S0_QUAT_50       0x1u
#define S0_QUAT_100      0x2u
#define S0_RAW_50        0x4u
#define S0_RAW_100       0x5u
#define S0_PASS          0xCu
#define S0_HERE          0xDu
#define S0_MISS          0xEu
#define S0_CALL          0xFu
#define S0_HIGH(s0)      ((uint8_t)((s0) >> 4))
#define S0_LOW(s0)       ((uint8_t)((s0) & 0x0Fu))
#define S0(high, low)    ((uint8_t)(((high) << 4) | ((low) & 0x0Fu)))

/* the bodies */
#define SYNC_BODY_BYTES  4u          /* the signature: AES-CCM's tag with the set's key */
#define MISS_BODY_BYTES  8u          /* a bit for each Belka missed, bit k the Belka k */
#define HERE_BODY_BYTES  8u          /* the new Belka's chip ID, little-endian */
#define PASS_EACH_BYTES  4u          /* before each missed Belka's steps: number, S0, state */
#define STEP_BYTES       12u         /* six i16: the accelerometer's x, y, z, the gyro's */
#define QUAT_BYTES       4u          /* a turn, the smallest three: 2 bits and three of 10 */
#define LIVE_STEPS_MAX   2u          /* the steps of the last whole period, 1 or 2 */
#define LIVE_BODY_MAX    (LIVE_STEPS_MAX * STEP_BYTES)
_Static_assert(STRELKA_AIR_BYTES(LIVE_BODY_MAX) == 42, "a LIVE of raw steps at 100 Hz is 42 B");
_Static_assert(STRELKA_AIR_BYTES(SYNC_BODY_BYTES) == 22, "a SYNC is 22 B");

struct PACKED strelka_step {
	int16_t a[3];
	int16_t g[3];
};
_Static_assert(sizeof(struct strelka_step) == STEP_BYTES, "a step is 12 B");

struct PACKED strelka_pass_entry {
	uint8_t belka;
	uint8_t s0;            /* the Belka's LIVE's S0: its steps and its state's item */
	uint16_t state;
	/* then its steps, as S0 says: 0, 1 or 2 quaternions or raw steps */
};
_Static_assert(sizeof(struct strelka_pass_entry) == PASS_EACH_BYTES, "a PASS's entry is 4 B");

/* ---- the set's state, in a SYNC ------------------------------------------------------------ */
#define SET_RECORD          (1u << 15)
#define SET_MARGIN(s)       (((s) >> 10) & 0x1Fu)   /* the move 0–2: 0–31 by 2 dB */
#define SET_NOISE_DBM(s)    (-100 + 2 * (int)SET_MARGIN(s))  /* the move 3: −100 to −38 dBm */
#define SET_MOVE(s)         (((s) >> 6) & 0x3u)     /* 0 none, 2 in two periods, 1 the next,
						     * 3 the measurement */
#define SET_CHANNEL(s)      ((s) & 0x3Fu)           /* the spare channel, or the one measured */
#define SET_MOVE_NONE       0u
#define SET_MOVE_NEXT       1u
#define SET_MOVE_TWO        2u
#define SET_MEASUREMENT     3u
#define SET_STATE(record, margin, move, channel) \
	((uint16_t)(((record) ? SET_RECORD : 0u) | (((margin) & 0x1Fu) << 10) | \
		    (((move) & 0x3u) << 6) | ((channel) & 0x3Fu)))

/* ---- a Belka's state, in its LIVE: one item a period ---------------------------------------- */
enum belka_item {
	ITEM_NONE = 0,         /* zeros */
	ITEM_CELL_TEMP = 1,    /* the cell u8 %; the temperature i8 °C */
	ITEM_CELL_MV = 2,      /* the cell u16 mV */
	ITEM_CARD = 3,         /* the card's free room u8 %; the blocks dropped u8 */
	ITEM_SYNCS = 4,        /* the SYNCs' mean signal i8 dBm; the SYNCs missed in the last second */
	ITEM_OFFSET = 5,       /* the clock's offset i16 ns */
	ITEM_STATE = 6,        /* the state u8; the fault u8 */
	ITEM_OUT = 7,          /* the steps out of range since the start: the accelerometer's,
				* the gyro's, u8 each */
	ITEM_CHANNEL = 8,      /* the channel measured u8; its noise i8 dBm */
	ITEM_FIRST = 1,
	ITEM_LAST = 8,
};
#define ITEM_BYTES(hi, lo)  ((uint16_t)(((uint16_t)(uint8_t)(hi) << 8) | (uint8_t)(lo)))
#define ITEM_HIGH(v)        ((uint8_t)((v) >> 8))
#define ITEM_LOW(v)         ((uint8_t)(v))

enum belka_state {
	BELKA_RECORDING = 0,
	BELKA_RECORDING_ALONE = 1,     /* without a SYNC, on its own clock */
	BELKA_RECORDING_SEARCHING = 2, /* looking for the set's channel */
	BELKA_PAUSED = 3,              /* ten seconds without a SYNC, listening */
	BELKA_WAITING = 4,             /* in the set */
	BELKA_CALIBRATING = 5,
	BELKA_GIVING = 6,              /* its record */
	BELKA_TAKING_FIRMWARE = 7,
	BELKA_ERASING = 8,
};

enum belka_fault {
	FAULT_NONE = 0,
	FAULT_NO_CARD = 1,
	FAULT_CARD_FULL = 2,
	FAULT_WRITE_FAILED = 3,
	FAULT_FS_UNREADABLE = 4,
	FAULT_DROPPED = 5,
	FAULT_SENSOR_SILENT = 6,
	FAULT_CLOCK_FALLBACK = 7,
	FAULT_CELL_LOW = 8,
	FAULT_CHARGER = 9,
	FAULT_GAUGE_SILENT = 10,
	FAULT_NO_CALIBRATION = 11,
};

/* ---- the search for new Belkas ------------------------------------------------------------- */
#define SEARCH_NUMBERS      65536u   /* the low 16 bits of SplitMix64's draw from the ID, attempt */
#define SEARCH_WINDOW_US    140u     /* a HERE of 26 B and the gap, calc/radio.py (radio-search) */
#define SEARCH_WINDOWS      138u     /* in a period after the CALL, calc/radio.py (radio-search) */
#define SEARCH_PERIODS      475u     /* a round: 65 536 numbers over 138 windows */
_Static_assert(SEARCH_WINDOWS * SEARCH_PERIODS >= SEARCH_NUMBERS, "every number has a window");
_Static_assert(STRELKA_AIR_BYTES(HERE_BODY_BYTES) == 26, "a HERE is 26 B");

/* ---- the record on the card (RECORD.md) --------------------------------------------------- */
#define REC_BLOCK_BYTES     512u
#define REC_BODY_BYTES      508u     /* before the CRC-32 */
#define REC_CRC_AT          508u
#define REC_STEPS           40u      /* steps in a data block */
#define REC_INFO_STEPS      120u     /* an information block speaks for three data blocks */
#define REC_PAYLOAD_AT      28u      /* where a data or information block's entries begin */
#define REC_TABLE_TEXT      502u     /* the set's table's text in one block */
#define REC_FREE_HEAD       365u
#define REC_STEP_MISSING    INT16_MIN /* a step lost: its six values */
#define REC_NO_SIGNAL       INT8_MIN  /* the SYNC's signal when none was heard */
#define REC_NO_POVODOK      0xFu
#define REC_VERSION         1u

enum rec_kind {
	REC_HEAD = 1,
	REC_DATA = 2,
	REC_END = 3,
	REC_TABLE = 4,
	REC_INFO = 5,
};

enum rec_why {              /* the end's why: 0 unfinished, a file cut off */
	END_UNFINISHED = 0,
	END_SYNC_STOP = 1,          /* the SYNC's state said stop */
	END_MAGNET_A = 2,           /* switched off by magnet A */
	END_CELL_LOW = 3,
	END_CARD_FULL = 4,
	END_FAULT = 5,
	END_APPLICATION = 6,        /* switched off by the application (COMMANDS.md, off) */
};

enum rec_clock {
	CLOCK_STEERED = 0,
	CLOCK_FALLBACK = 1,         /* the steps off the grid, placed by their offsets */
};

struct PACKED rec_head {
	char mark[4];           /* "STRK" */
	uint8_t version;        /* REC_VERSION */
	uint8_t kind;           /* REC_HEAD */
	uint16_t session;
	uint8_t belka;          /* 1–63 */
	uint64_t chip;          /* the chip's ID */
	uint8_t fw[3];          /* major, minor, patch */
	uint8_t build[4];       /* the short hash of the source */
	uint16_t rate_hz;       /* into memory */
	uint8_t n;              /* the samples a step */
	uint8_t acc_g;          /* ±g */
	uint16_t gyro_dps;      /* ±°/s */
	uint32_t first_period;
	uint64_t time_us;       /* µs since 1970, UTC; 0 unknown */
	uint32_t time_period;   /* the period of that time */
	int16_t tz_min;         /* minutes from UTC */
	uint8_t clock;          /* enum rec_clock */
	int16_t acc_cal[6];     /* raw readings, x up and down, y, z */
	uint16_t gyro_scale[3]; /* 32 768 for 1,000 */
	int16_t gyro_zero[3];   /* raw, at rest, warm */
	int16_t cal_temp;       /* 0,01 °C */
	uint32_t cal_time;      /* s since 1970; 0 none */
	char place[32];         /* UTF-8, zeros after */
	char name[32];          /* the file's own name */
};
_Static_assert(sizeof(struct rec_head) == 143, "the head's fields are 143 B, the rest free");

struct PACKED rec_table {
	uint8_t kind;           /* REC_TABLE */
	uint8_t part;           /* 0 first */
	uint16_t session;
	uint16_t length;        /* the whole text's */
	uint8_t text[REC_TABLE_TEXT];
};
_Static_assert(sizeof(struct rec_table) == REC_BODY_BYTES, "a table block fills its 508 B");

struct PACKED rec_data {
	uint8_t kind;           /* REC_DATA */
	uint8_t steps;          /* 40, fewer in the session's last */
	uint16_t session;
	uint32_t number;        /* one count for the data and the information blocks */
	uint32_t period;        /* of its first step */
	uint8_t place;          /* which block of the period, from 4 kHz */
	uint8_t missing;
	uint8_t acc_out;
	uint8_t gyro_out;
	int32_t offset_ns;      /* the first step's data-ready less its place on the grid */
	uint8_t free[8];
	struct strelka_step step[REC_STEPS];
};
_Static_assert(sizeof(struct rec_data) == REC_BODY_BYTES, "a data block fills its 508 B");
_Static_assert(__builtin_offsetof(struct rec_data, step) == REC_PAYLOAD_AT, "the steps at 28");

struct PACKED rec_info_entry {
	int16_t offset_ns;      /* the step's data-ready against the grid */
	int8_t signal_dbm;      /* the SYNC's; REC_NO_SIGNAL none heard */
	uint8_t who;            /* the high nibble the Povodok taken, F none; the low the SYNCs
				 * heard in the period, 0–4 */
};
_Static_assert(sizeof(struct rec_info_entry) == 4, "an information entry is 4 B");

struct PACKED rec_info {
	uint8_t kind;           /* REC_INFO */
	uint8_t steps;          /* 120, fewer in the session's last */
	uint16_t session;
	uint32_t number;
	uint32_t period;        /* of its first step */
	int16_t temp;           /* the sensor's, 0,01 °C */
	uint16_t cell_mv;
	uint8_t free[12];
	struct rec_info_entry entry[REC_INFO_STEPS];
};
_Static_assert(sizeof(struct rec_info) == REC_BODY_BYTES, "an information block fills its 508 B");
_Static_assert(__builtin_offsetof(struct rec_info, entry) == REC_PAYLOAD_AT, "the entries at 28");

struct PACKED rec_end {
	uint8_t kind;           /* REC_END */
	uint8_t why;            /* enum rec_why */
	uint16_t session;
	uint32_t steps;
	uint32_t blocks;        /* the data and the information blocks */
	uint32_t last_period;
	uint32_t dropped;
};
_Static_assert(sizeof(struct rec_end) == 20, "the end's fields are 20 B, the rest free");

/* ---- SMP, Zephyr's MCUmgr protocol (COMMANDS.md) ------------------------------------------- */
struct PACKED strelka_smp_hdr {   /* big-endian, as Zephyr writes it; Zephyr's own smp_hdr is its */
	uint8_t op_ver;         /* bits 2–0 the op, bits 4–3 the version, 1 */
	uint8_t flags;          /* zero */
	uint8_t len[2];         /* the CBOR body's length */
	uint8_t group[2];
	uint8_t seq;
	uint8_t id;
};
_Static_assert(sizeof(struct strelka_smp_hdr) == 8, "the SMP header is 8 B");
#define SMP_FRAME_MAX    2048u       /* Zephyr's default for a network transport */
#define SMP_VERSION      1u
#define SMP_OP_READ      0u
#define SMP_OP_READ_RSP  1u
#define SMP_OP_WRITE     2u
#define SMP_OP_WRITE_RSP 3u
#define SMP_OP_VER(op)   ((uint8_t)(((op) & 0x7u) | (SMP_VERSION << 3)))
#define SMP_GROUP_OS     0u
#define SMP_GROUP_IMG    1u
#define SMP_GROUP_FS     8u
#define SMP_GROUP_STRELKA 64u
#define SMP_PORT         1337u       /* Povodok's; 1337 + n the Belka numbered n */

enum smp_strelka_id {       /* Strelka's group, 64 */
	CMD_INFO = 0,
	CMD_SESSION = 1,
	CMD_SESSIONS = 2,
	CMD_CAL = 3,
	CMD_BLINK = 4,
	CMD_CARD = 5,
	CMD_OFF = 6,
	CMD_FORGET = 7,
	CMD_KEY = 8,
	CMD_JOIN = 9,
	CMD_RECORD = 10,
	CMD_LIVE = 11,
	CMD_SEARCH = 12,
	CMD_TAKE = 13,
	CMD_SET = 14,
	CMD_DROP = 15,
	CMD_AUTH = 16,
	CMD_PASSWORD = 17,
	CMD_TABLE = 18,
};

enum smp_err {              /* Zephyr's mgmt_defines.h */
	SMP_ERR_OK = 0,
	SMP_ERR_UNKNOWN = 1,
	SMP_ERR_NOMEM = 2,
	SMP_ERR_INVAL = 3,
	SMP_ERR_TIMEOUT = 4,
	SMP_ERR_NOENT = 5,
	SMP_ERR_BAD_STATE = 6,
	SMP_ERR_TOO_LARGE = 7,
	SMP_ERR_NOTSUP = 8,
	SMP_ERR_CORRUPT = 9,
	SMP_ERR_BUSY = 10,
	SMP_ERR_ACCESS = 11,
	SMP_ERR_TOO_OLD = 12,
	SMP_ERR_TOO_NEW = 13,
};

enum smp_strelka_err {      /* group 64's own, in an answer's err: {group: 64, rc} */
	STRELKA_ERR_UNREACHABLE = 1, /* the bridged target out of reach */
};

/* ---- the frame over ESB, between Povodok and a Belka (COMMANDS.md) ------------------------ */
#define ESB_PAYLOAD_MAX  252u
#define FRAG_HEAD_BYTES  5u
#define FRAG_MIC_BYTES   4u
#define FRAG_DATA_MAX    (ESB_PAYLOAD_MAX - FRAG_HEAD_BYTES - FRAG_MIC_BYTES)   /* 243 */
#define FRAG_INDEX_MAX   63u
#define FRAG_WHOLE       0u          /* byte 0, bits 7–6 */
#define FRAG_FIRST       1u
#define FRAG_MIDDLE      2u
#define FRAG_LAST        3u
#define FRAG_KIND(b)     ((uint8_t)((b) >> 6))
#define FRAG_INDEX(b)    ((uint8_t)((b) & 0x3Fu))
#define FRAG_BYTE0(kind, index) ((uint8_t)(((kind) << 6) | ((index) & 0x3Fu)))
#define NONCE_BYTES      13u         /* the set's address 4, the sender's who 1, the count 4,
				      * the index 1, the receiver's who 1, two zeros */
_Static_assert(FRAG_DATA_MAX == 243, "a fragment carries 243 B of a frame");
_Static_assert((SMP_FRAME_MAX + FRAG_DATA_MAX - 1) / FRAG_DATA_MAX <= FRAG_INDEX_MAX + 1,
	       "a frame of 2 048 B goes in 9 fragments, within the index's 64");

struct PACKED frag_head {
	uint8_t frag_index;     /* FRAG_BYTE0 */
	uint32_t count;         /* against a replay: one count a direction and a Belka */
};
_Static_assert(sizeof(struct frag_head) == FRAG_HEAD_BYTES, "a fragment's head is 5 B");

/* ---- the live view's message to the phone, over UDP (PHONE.md) ----------------------------- */
#define LIVE_MSG_KIND    1u
#define LIVE_MSG_VERSION 1u
#define UDP_MAX_BYTES    1400u

struct PACKED live_msg_head {
	uint8_t kind;           /* LIVE_MSG_KIND */
	uint8_t version;
	uint16_t session;
	uint32_t period;
	uint16_t state;         /* the set's, from the first Povodok's SYNC */
	uint8_t s0;             /* the SYNC's */
	uint8_t povodoks;       /* a bit for each Povodok heard; bit 0 the first itself */
	uint16_t povodok_state[4];
	uint8_t belkas;         /* the entries that follow */
};
_Static_assert(sizeof(struct live_msg_head) == 21, "the live message's head is 21 B");

struct PACKED live_msg_entry {
	uint8_t belka;
	uint8_t s0;             /* its LIVE's */
	uint8_t by;             /* 0 the first heard it; 1–3 the Povodok that passed it */
	uint16_t state;
	/* then its steps, as S0 says */
};
_Static_assert(sizeof(struct live_msg_entry) == 5, "a live entry is 5 B before its steps");

#endif
