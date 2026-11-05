#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <ff.h>
#include "card.h"
#include "record.h"

LOG_MODULE_REGISTER(card, LOG_LEVEL_INF);

#define DISK        "SD"
#define MOUNT       "/SD:"
#define RING_BLOCKS 256                    /* 128 KB: a quarter of the RAM (calc/params.py) */
#define BUF_NODE    DT_NODELABEL(card_buffer)
#define SYNC_MS     3000                   /* what is written saved to the card every 3 s */
#define WRITER_PRIO 10                     /* below the sensor's and the radio's threads */
#define DAY_S       86400u                 /* the room asked: a day of the record at its rate */
#define ROOM_TRIES  3                      /* that room, then a quarter of it, twice: on FAT32
					    * each try that fails reads the whole FAT */
#define FILE_CAP    (0x80000000ull - REC_BLOCK_BYTES)  /* a file under 2 GiB, Zephyr's off_t
						        * being 32 bits: the record goes on in
						        * the next part (FIRMWARE.md, The record) */

static FATFS fat_fs;
static struct fs_mount_t mnt = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = MOUNT,
};
static struct fs_file_t file;
static bool present, mounted, file_open, closing;
static uint8_t fault;
static uint32_t dropped;
static uint64_t room, written;             /* the room taken at the start, the bytes written */
static uint8_t probe[REC_BLOCK_BYTES];     /* a block read back, on the way in */
static uint16_t open_session;              /* the record being written, for its next part */
static uint8_t open_belka, part;           /* part 1 the file as named, 2 and on _2, _3 … */
static uint32_t open_rate;
static uint8_t head_copy[REC_BLOCK_BYTES]; /* the session's head, repeated at each part's start */
static bool head_kept;

/* the buffer in RAM_01, shared with the FLPR, so that either core could hold the card without
 * moving it (belka/boards/flpr.overlay; FIRMWARE.md, The sensor on the FLPR); the card's 1 MB/s
 * untouched by the few cycles' wait a running FLPR may add to RAM_01 */
BUILD_ASSERT(DT_REG_SIZE(BUF_NODE) >= RING_BLOCKS * REC_BLOCK_BYTES, "the card's buffer holds it");
static uint8_t (*const ring)[REC_BLOCK_BYTES] = (uint8_t (*)[REC_BLOCK_BYTES])DT_REG_ADDR(BUF_NODE);
static volatile uint32_t head, tail;       /* head the next free, tail the next to write */
static K_SEM_DEFINE(blocks_ready, 0, RING_BLOCKS);
static K_SEM_DEFINE(ring_empty, 0, 1);

int card_mount(void)
{
	int rc;

	if (mounted) {
		return 0;
	}
	rc = fs_mount(&mnt);
	if (rc) {
		fault = FAULT_FS_UNREADABLE;
		LOG_WRN("the card's file system unreadable (%d)", rc);
		return fault;
	}
	mounted = true;
	fault = FAULT_NONE;
	LOG_INF("the card mounted, %u MB free", card_free_mb());
	return 0;
}

int card_unmount(void)
{
	if (!mounted || file_open) {
		return file_open ? -EBUSY : 0;
	}
	fs_unmount(&mnt);
	/* FatFs's unmount lets the disk go, which the SD stack takes as its power-off and marks
	 * it uninitialised — and the USB disk class answers "not ready" for a disk in that state;
	 * so it is brought up again at once, identified and powered, for the host to have it */
	(void)disk_access_ioctl(DISK, DISK_IOCTL_CTRL_INIT, NULL);
	mounted = false;
	return 0;
}

bool card_mounted(void)
{
	return mounted;
}

/* block i of an open file read into probe, its CRC whole; FatFs's own calls, as a file of exFAT
 * may pass the 2 GiB of Zephyr's off_t */
static bool block_at(FIL *fp, uint64_t i)
{
	UINT got;

	return f_lseek(fp, (FSIZE_t)(i * REC_BLOCK_BYTES)) == FR_OK &&
	       f_read(fp, probe, REC_BLOCK_BYTES, &got) == FR_OK && got == REC_BLOCK_BYTES &&
	       rec_block_ok(probe);
}

/* what the file's own blocks must agree with, read from its head: its session, its first
 * period, and where the data begin after the table's parts */
struct own {
	uint16_t session;
	uint32_t first_period;
	uint64_t first_data;
};

/* a block of this session's file after its head: its CRC whole, its kind one of the file's, its
 * session the head's — and, so that an older file of the same number left in the same clusters
 * is not taken for it, a table's part where the table stands, a data or information block's
 * count never behind its place (blocks dropped only put it ahead) and its period not before the
 * head's, an end's count of blocks likewise (RECORD.md, The end) */
static bool ours(FIL *fp, uint64_t i, const struct own *o)
{
	uint64_t at;

	if (!block_at(fp, i) || sys_get_le16(probe + 2) != o->session) {
		return false;
	}
	at = i >= o->first_data ? i - o->first_data : 0;
	switch (probe[0]) {
	case REC_TABLE:
		return i < o->first_data && probe[1] == i - 1;
	case REC_DATA:
	case REC_INFO:
		return i >= o->first_data && sys_get_le32(probe + 4) >= at &&
		       sys_get_le32(probe + 8) >= o->first_period;
	case REC_END:
		return i >= o->first_data && sys_get_le32(probe + 8) >= at;
	default:
		return false;
	}
}

/* a file the power left in its room cut to its last block of its own: the last block looked at
 * first, the end there in every file closed; else halved down to the last one, the blocks
 * written being one run from the head and the room after them another file's or none */
static void cut_to_whole(const char *path)
{
	struct fs_file_t f;
	FIL *fp;
	uint64_t n, lo, hi;
	struct own o;
	struct rec_head head;

	fs_file_t_init(&f);
	if (fs_open(&f, path, FS_O_RDWR)) {
		return;
	}
	fp = f.filep;
	n = fp->obj.objsize / REC_BLOCK_BYTES;
	if (n < 2 || !block_at(fp, 0) || memcmp(probe, "STRK", 4) != 0) {
		goto out;                        /* no head of ours: left as it is */
	}
	memcpy(&head, probe, sizeof(head));
	o.session = head.session;
	o.first_period = head.first_period;
	o.first_data = 1;
	if (block_at(fp, 1) && probe[0] == REC_TABLE && sys_get_le16(probe + 2) == o.session &&
	    probe[1] == 0) {
		o.first_data += rec_table_blocks(sys_get_le16(probe + 4));
	}
	if (ours(fp, n - 1, &o)) {
		goto out;                        /* closed, or written to its last byte */
	}
	lo = 0;                                  /* ours */
	hi = n - 1;                              /* not */
	while (hi - lo > 1) {
		uint64_t mid = lo + (hi - lo) / 2;

		if (ours(fp, mid, &o)) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	if (f_lseek(fp, (FSIZE_t)((lo + 1) * REC_BLOCK_BYTES)) == FR_OK && f_truncate(fp) == FR_OK) {
		LOG_INF("%s cut to its %llu blocks of its own", path, (unsigned long long)(lo + 1));
	}
out:
	fs_close(&f);
}

/* at the start, after a power cut or a reset that left a record open */
static void cut_unfinished(void)
{
	static struct fs_dirent entry;
	struct fs_dir_t dir;
	char path[80];

	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, MOUNT)) {
		return;
	}
	while (fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
		size_t len = strlen(entry.name);

		if (entry.type == FS_DIR_ENTRY_FILE && len > 5 &&
		    strcmp(entry.name + len - 5, ".STRK") == 0) {
			snprintf(path, sizeof(path), MOUNT "/%s", entry.name);
			cut_to_whole(path);
		}
	}
	fs_closedir(&dir);
}

int card_init(void)
{
	int rc = disk_access_init(DISK);

	if (rc) {
		fault = FAULT_NO_CARD;
		LOG_WRN("no card (%d)", rc);
		return fault;
	}
	present = true;
	rc = card_mount();
	if (rc == 0) {
		cut_unfinished();
	}
	return rc;
}

uint8_t card_fault(void)
{
	return fault;
}

bool card_present(void)
{
	return present;
}

uint8_t card_fs(void)
{
	return present ? 2 : 0;            /* the card's format read, to come */
}

static bool free_room(struct fs_statvfs *st)
{
	return mounted && fs_statvfs(MOUNT, st) == 0;
}

/* the card's free bytes, the room taken and not yet written counted with them */
static uint64_t free_bytes(const struct fs_statvfs *st)
{
	uint64_t bytes = (uint64_t)st->f_bfree * st->f_frsize;

	if (file_open && room > written) {
		bytes += room - written;
	}
	return bytes;
}

uint8_t card_free_pct(void)
{
	struct fs_statvfs st;
	uint64_t all;

	if (!free_room(&st) || st.f_blocks == 0) {
		return 0;
	}
	all = (uint64_t)st.f_blocks * st.f_frsize;
	return (uint8_t)MIN(free_bytes(&st) * 100 / all, 100u);
}

uint32_t card_free_mb(void)
{
	struct fs_statvfs st;

	if (!free_room(&st)) {
		return 0;
	}
	return (uint32_t)(free_bytes(&st) / (1024 * 1024));
}

/* the bytes of a day's record at its rate: four blocks every 120 steps (RECORD.md, The file) */
static uint64_t day_bytes(uint32_t rate_hz)
{
	return (uint64_t)rate_hz * DAY_S * (4u * REC_BLOCK_BYTES) / REC_INFO_STEPS;
}

/* the file's room taken in one run, FatFs's f_expand (CONFIG_FS_FATFS_EXTRA_NATIVE_API): a day of
 * the record, or the card's free room if less, FAT32's limit on FAT32; a quarter of it twice more
 * where no such run is free; else none, the file growing as it goes */
static void take_room(uint32_t rate_hz)
{
	FIL *fp = file.filep;
	struct fs_statvfs st;
	uint64_t want = day_bytes(rate_hz);

	room = 0;
	if (fs_statvfs(MOUNT, &st) == 0) {
		uint64_t cluster = (uint64_t)fp->obj.fs->csize * 512u;
		uint64_t avail = (uint64_t)st.f_bfree * st.f_frsize;

		/* one cluster kept free, so that the end has its block when the card is full */
		want = MIN(want, avail > cluster ? avail - cluster : 0);
	}
	want = MIN(want, FILE_CAP);
	for (int i = 0; i < ROOM_TRIES; i++) {
		want -= want % REC_BLOCK_BYTES;
		if (want < REC_BLOCK_BYTES) {
			break;
		}
		if (f_expand(fp, (FSIZE_t)want, 1) == FR_OK) {
			room = want;
			LOG_INF("%llu MB taken for the record", (unsigned long long)(room >> 20));
			return;
		}
		want /= 4;
	}
	LOG_WRN("no run of free room: the file grows as it goes");
}

/* the file of a part opened and its room taken: part 1 as the record names it, the next parts
 * _2, _3 and on, each begun with the session's head again, so that each reads on its own */
static void part_name(char *name, size_t cap)
{
	if (part == 1) {
		snprintf(name, cap, "S%05uB%02u.STRK", open_session, open_belka);
	} else {
		snprintf(name, cap, "S%05uB%02u_%u.STRK", open_session, open_belka, part);
	}
}

bool card_recording(char *name, size_t cap, uint64_t *bytes)
{
	if (!file_open) {
		return false;
	}
	part_name(name, cap);
	*bytes = written;
	return true;
}

static int open_part(void)
{
	char path[48], name[32];
	int rc;

	part_name(name, sizeof(name));
	snprintf(path, sizeof(path), MOUNT "/%s", name);
	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
	if (rc) {
		fault = FAULT_WRITE_FAILED;
		return rc;
	}
	written = 0;
	take_room(open_rate);
	return 0;
}

/* the writer's: this part full, closed at its last byte, the next opened with the head first */
static void next_part(void)
{
	if (room > written) {
		(void)f_truncate((FIL *)file.filep);
	}
	room = 0;
	fs_close(&file);
	part++;
	if (open_part() == 0 && head_kept &&
	    fs_write(&file, head_copy, REC_BLOCK_BYTES) == REC_BLOCK_BYTES) {
		written = REC_BLOCK_BYTES;
		LOG_INF("the record goes on in part %u", part);
	} else {
		fault = FAULT_WRITE_FAILED;
	}
}

int card_open(uint16_t session, uint8_t belka, uint32_t rate_hz)
{
	int rc;

	if (!mounted || file_open) {
		return -1;
	}
	open_session = session;
	open_belka = belka;
	open_rate = rate_hz;
	part = 1;
	head_kept = false;
	rc = open_part();
	if (rc) {
		return rc;
	}
	head = tail = 0;
	dropped = 0;
	closing = false;
	fault = FAULT_NONE;                  /* a full or failed write before is this file's
					      * to find again, not the last one's */
	file_open = true;
	return 0;
}

void card_put(const uint8_t block[REC_BLOCK_BYTES])
{
	uint32_t next = (head + 1) % RING_BLOCKS;

	if (!file_open || closing) {
		return;
	}
	if (next == tail || (fault == FAULT_CARD_FULL && block[0] != REC_END)) {
		dropped++;                       /* the buffer full, the card busy too long; or
						  * the card itself full — its end still tried */
		return;
	}
	if (!head_kept && memcmp(block, "STRK", 4) == 0) {
		memcpy(head_copy, block, REC_BLOCK_BYTES);   /* for the next parts */
		head_kept = true;
	}
	memcpy(ring[head], block, REC_BLOCK_BYTES);
	head = next;
	k_sem_give(&blocks_ready);
}

uint32_t card_dropped(void)
{
	return dropped;
}

void card_close(void)
{
	if (file_open) {
		closing = true;                  /* the writer closes it once the ring is out */
		k_sem_give(&blocks_ready);
	}
}

bool card_busy(void)
{
	return file_open;
}

/* 1 every session's file deleted, 2 the card formatted; the file system is the record's
 * meanwhile (main.c refuses both while the Belka records) */
int card_erase(uint8_t what)
{
	struct fs_dir_t dir;
	static struct fs_dirent ent;
	int rc = 0;

	if (!present) {
		return -ENODEV;
	}
	if (file_open) {
		return -EBUSY;
	}
	fault = FAULT_NONE;
	if (what == 2) {
		card_unmount();
		rc = fs_mkfs(FS_FATFS, (uintptr_t)"SD:", NULL, 0);   /* FatFs wants the colon */
		card_mount();
		return rc ? -EIO : 0;
	}
	if (what != 1) {
		return -EINVAL;
	}
	if (!mounted) {
		return -ENODEV;
	}
	fs_dir_t_init(&dir);
	if (fs_opendir(&dir, MOUNT)) {
		return -EIO;
	}
	while (fs_readdir(&dir, &ent) == 0 && ent.name[0]) {
		size_t len = strlen(ent.name);
		char path[sizeof(MOUNT) + MAX_FILE_NAME + 1];

		if (ent.type == FS_DIR_ENTRY_FILE && len > 5 &&
		    strcmp(ent.name + len - 5, ".STRK") == 0) {
			snprintf(path, sizeof(path), MOUNT "/%s", ent.name);
			if (fs_unlink(path)) {
				rc = -EIO;
			}
		}
	}
	fs_closedir(&dir);
	return rc;
}

void card_set_file_allowed(bool allowed)
{
	ARG_UNUSED(allowed);               /* the file comes on the disk, which usb.c guards */
}

bool card_set_file_given(void)
{
	return false;                      /* main.c reads the disk itself */
}

static void writer(void *a, void *b, void *c)
{
	int64_t last_sync = k_uptime_get();

	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		k_sem_take(&blocks_ready, K_MSEC(SYNC_MS));
		while (file_open && tail != head) {
			ssize_t n;

			if (written + REC_BLOCK_BYTES > FILE_CAP && fault == FAULT_NONE) {
				next_part();     /* 2 GiB: the record goes on in the next file */
			}
			n = fs_write(&file, ring[tail], REC_BLOCK_BYTES);

			if (n != REC_BLOCK_BYTES) {
				/* FatFs gives a short write, not -ENOSPC, when the volume is full */
				fault = n >= 0 ? FAULT_CARD_FULL : FAULT_WRITE_FAILED;
				LOG_ERR("a write failed (%d)", (int)n);
			}
			if (n > 0) {
				written += (uint64_t)n;
			}
			tail = (tail + 1) % RING_BLOCKS;
		}
		if (tail == head) {
			k_sem_give(&ring_empty);
			if (closing && file_open) {
				if (room > written) {
					/* the room not written given back, the file cut at its
					 * last byte written, where FatFs's pointer stands */
					(void)f_truncate((FIL *)file.filep);
				}
				room = 0;
				fs_close(&file);
				file_open = false;
				closing = false;
				continue;
			}
		}
		if (file_open && k_uptime_get() - last_sync >= SYNC_MS) {
			fs_sync(&file);              /* a power cut loses 3 s at most; the file
						      * keeps its room, cut at the next start */
			last_sync = k_uptime_get();
		}
	}
}

K_THREAD_DEFINE(card_writer, 2048, writer, NULL, NULL, NULL, WRITER_PRIO, 0, 0);
