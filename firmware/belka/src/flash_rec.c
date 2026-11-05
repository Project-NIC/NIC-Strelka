/* NIC-Strelka — Kryska's memory: the record raw on Winbond's W25Q01JV on SPIM20, behind the
 * card's interface (card.h), no file system (KRYSKA.md, The record on the flash). The blocks go
 * through the card's buffer of 128 KB and a writer of its own below the sensor and the radio, as
 * on the Belka (card.c), one after another from the first erased block, found by halving at the
 * start, the sessions before it listed backwards (common/flashrec.c). The pages are programmed
 * and read through Zephyr's SPI NOR driver, which waits on the die the page is in: the status
 * read follows the instruction before it (the sheet, Rev E, 6.3.2). What the driver cannot do
 * this file does over the same SPI: each die selected by C2h and its BUSY read, after the chip
 * erase, which the driver awaits on one die alone, and before the power-down, as Linux's driver
 * does (linux-mtd, January 2025); the power-down, B9h, which both dies take, once nothing has
 * used the memory for a few seconds, and its release, ABh, before anything does. The sessions
 * are files, read alone, on a file system of Kryska's own registered in Zephyr and mounted as the
 * card's, "/SD:", so that MCUmgr's file group, the sessions' list and the application see a
 * Belka's card; STRELKA.SET alone may be written to it, the set given by the cable over SMP on
 * the serial port (FIRMWARE.md, USB), and main.c takes it as from a disk. Built against nRF
 * Connect SDK v3.4.0, not run on hardware: the kit decides it (the desk's kit.md). */
#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "card.h"
#include "flashrec.h"
#include "setfile.h"

LOG_MODULE_REGISTER(flash_rec, LOG_LEVEL_INF);

#define FLASH_NODE  DT_NODELABEL(rec_flash)
#define FLASH_BYTES (DT_PROP(FLASH_NODE, size) / 8)     /* the node gives bits */
#define BLOCKS      (FLASH_BYTES / REC_BLOCK_BYTES)
#define MOUNT       "/SD:"
#define RING_BLOCKS 256                    /* 128 KB, the card's buffer (card.c) */
#define BUF_NODE    DT_NODELABEL(card_buffer)
#define IDLE_MS     3000                   /* unused that long, the flash is put to sleep */
#define WRITER_PRIO 10                     /* below the sensor's and the radio's threads */
#define OPEN_FILES  4                      /* the download's, the hash's, the list's, the set's */
#define DIES        2
#define CMD_RDSR    0x05                   /* Read Status Register-1 (the sheet, 6.3.2) */
#define CMD_SELDIE  0xC2                   /* Software Die Select (6.3.3) */
#define CMD_DPD     0xB9                   /* Power-down, both dies (6.3.3, 6.4.34) */
#define CMD_RDPD    0xAB                   /* Release Power-down, both dies */
#define STATUS_BUSY 0x01
#define T_RES1_US   3                      /* after ABh, and after B9h, tDP (7.6) */

BUILD_ASSERT(FLASH_BYTES % REC_BLOCK_BYTES == 0, "the memory in whole blocks");
BUILD_ASSERT(FLASH_BYTES <= 0x7FFFFFFF, "the driver's offsets are off_t");
BUILD_ASSERT(DT_REG_SIZE(BUF_NODE) >= RING_BLOCKS * REC_BLOCK_BYTES, "the buffer holds it");

static const struct device *const flash = DEVICE_DT_GET(FLASH_NODE);
/* the same bus and chip select as the driver's, for the commands it has not: used only between
 * its calls, under `lock` */
static const struct spi_dt_spec bus = SPI_DT_SPEC_GET(FLASH_NODE, SPI_WORD_SET(8));

static struct flashrec index;              /* the sessions, under `lock` */
static K_MUTEX_DEFINE(lock);               /* the memory and the index: one user at a time */
static bool present, asleep, file_open, closing, erase_asked, erasing;
static uint8_t fault;
static uint32_t dropped;
static uint16_t open_session;              /* the record being written */
static uint8_t open_belka;
static uint32_t first, count;              /* its head's block, its blocks written */
static int readers;                        /* files of the record open, the flash kept awake */

static uint8_t (*const ring)[REC_BLOCK_BYTES] = (uint8_t (*)[REC_BLOCK_BYTES])DT_REG_ADDR(BUF_NODE);
static volatile uint32_t head, tail;       /* head the next free, tail the next to write */
static K_SEM_DEFINE(blocks_ready, 0, RING_BLOCKS);

/* STRELKA.SET given over SMP on the serial port, held until main.c takes it as from a disk */
static char set_text[SETFILE_MAX];
static size_t set_len;
static bool set_allowed, set_given, set_writing;

/* ---- the commands the driver has not ------------------------------------------------------- */

static int command(const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
	const struct spi_buf tb = { .buf = (void *)tx, .len = tx_len };
	const struct spi_buf_set ts = { .buffers = &tb, .count = 1 };
	const struct spi_buf rb[2] = { { .buf = NULL, .len = tx_len }, { .buf = rx, .len = rx_len } };
	const struct spi_buf_set rs = { .buffers = rb, .count = 2 };

	return rx ? spi_transceive_dt(&bus, &ts, &rs) : spi_write_dt(&bus, &ts);
}

static int select_die(uint8_t die)
{
	const uint8_t tx[2] = { CMD_SELDIE, die };

	return command(tx, sizeof(tx), NULL, 0);
}

/* both dies idle: each selected and its BUSY read until clear, as Linux's driver does; the
 * driver's own wait reads one die alone */
static int wait_dies(k_timeout_t poll)
{
	const uint8_t tx = CMD_RDSR;
	uint8_t status;
	int rc = 0;

	for (uint8_t die = 0; die < DIES && rc == 0; die++) {
		rc = select_die(die);
		while (rc == 0) {
			rc = command(&tx, 1, &status, 1);
			if (rc || !(status & STATUS_BUSY)) {
				break;
			}
			k_sleep(poll);
		}
	}
	if (rc == 0) {
		rc = select_die(0);
	}
	return rc;
}

/* under `lock`: out of the power-down before any use */
static int wake(void)
{
	const uint8_t tx = CMD_RDPD;
	int rc;

	if (!asleep) {
		return 0;
	}
	rc = command(&tx, 1, NULL, 0);
	k_busy_wait(T_RES1_US);
	asleep = rc != 0;
	return rc;
}

/* under `lock`: both dies idle, then the power-down, 1 µA (the sheet, 7.4) */
static void sleep_flash(void)
{
	const uint8_t tx = CMD_DPD;

	if (asleep) {
		return;
	}
	if (wait_dies(K_MSEC(1)) == 0 && command(&tx, 1, NULL, 0) == 0) {
		k_busy_wait(T_RES1_US);
		asleep = true;
	}
}

/* ---- the memory through the driver ---------------------------------------------------------- */

static bool read_block(void *ctx, uint32_t block, uint8_t out[REC_BLOCK_BYTES])
{
	ARG_UNUSED(ctx);
	return flash_read(flash, (off_t)block * REC_BLOCK_BYTES, out, REC_BLOCK_BYTES) == 0;
}

/* under `lock`: the memory read whole, next and the list anew */
static int scan(void)
{
	int rc = wake();

	if (rc == 0) {
		rc = flashrec_scan(&index, read_block, NULL, BLOCKS);
	}
	return rc;
}

/* ---- the file system of the record: the sessions as files, read alone ---------------------- */

struct open_file {
	bool used;
	bool set;               /* STRELKA.SET, else a session */
	bool creating;          /* STRELKA.SET being given */
	int entry;              /* the session's index in the list */
	uint32_t size;          /* bytes */
	uint32_t pos;
};

static struct open_file files[OPEN_FILES];

static const char *name_of(const char *path)
{
	const char *p = path;

	if (strncmp(p, MOUNT, sizeof(MOUNT) - 1) == 0) {
		p += sizeof(MOUNT) - 1;
	}
	while (*p == '/') {
		p++;
	}
	return p;
}

static struct open_file *slot(void)
{
	for (unsigned i = 0; i < OPEN_FILES; i++) {
		if (!files[i].used) {
			memset(&files[i], 0, sizeof(files[i]));
			files[i].used = true;
			return &files[i];
		}
	}
	return NULL;
}

static int fr_open(struct fs_file_t *filp, const char *path, fs_mode_t flags)
{
	const char *name = name_of(path);
	bool writing = (flags & (FS_O_WRITE | FS_O_CREATE | FS_O_APPEND | FS_O_TRUNC)) != 0;
	struct open_file *f;
	int rc = 0;

	if (erasing) {
		return -EBUSY;                   /* the erase holds the memory, minutes long */
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (strcmp(name, SETFILE_NAME) == 0) {
		/* the set given by the cable: written only where a disk could be, a new unit's or
		 * one opened by magnet B or 64/5 (FIRMWARE.md, USB); read back by main.c alone */
		if (flags & FS_O_CREATE) {
			if (!set_allowed || set_writing) {
				rc = -EACCES;
				goto out;
			}
			set_len = 0;
			set_given = false;
		} else if (!set_given && !set_writing) {
			rc = -ENOENT;
			goto out;
		}
		f = slot();
		if (f == NULL) {
			rc = -EMFILE;
			goto out;
		}
		f->set = true;
		f->creating = (flags & FS_O_CREATE) != 0;
		f->size = (uint32_t)set_len;
		if (f->creating) {
			set_writing = true;
		}
		filp->filep = f;
		goto out;
	}
	if (writing) {
		rc = -EROFS;
		goto out;
	}
	f = slot();
	if (f == NULL) {
		rc = -EMFILE;
		goto out;
	}
	f->entry = flashrec_find(&index, name);
	if (f->entry < 0) {
		f->used = false;
		rc = -ENOENT;
		goto out;
	}
	/* a session listed is a session closed: read while a record runs as well, the reads and
	 * the writes taking turns on the flash under the lock */
	f->size = index.list[f->entry].blocks * REC_BLOCK_BYTES;
	rc = wake();
	if (rc) {
		f->used = false;
		goto out;
	}
	readers++;
	filp->filep = f;
out:
	k_mutex_unlock(&lock);
	return rc;
}

static int fr_close(struct fs_file_t *filp)
{
	struct open_file *f = filp->filep;

	k_mutex_lock(&lock, K_FOREVER);
	if (f->set) {
		if (f->creating) {
			struct setfile sf;

			set_writing = false;
			/* given once it parses: a file cut short — the client gone, the file
			 * group closing it after its idle time — is not a set and is let go */
			set_given = set_len > 0 && setfile_parse(set_text, set_len, &sf) == 0;
			memset(&sf, 0, sizeof(sf));
			if (!set_given) {
				memset(set_text, 0, sizeof(set_text));
				set_len = 0;
			}
		}
	} else {
		readers--;
	}
	f->used = false;
	filp->filep = NULL;
	k_mutex_unlock(&lock);
	return 0;
}

static ssize_t fr_read(struct fs_file_t *filp, void *dest, size_t n)
{
	struct open_file *f = filp->filep;
	ssize_t got = 0;

	if (erasing && !f->set) {
		return -EBUSY;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (f->pos < f->size) {
		n = MIN(n, f->size - f->pos);
		if (f->set) {
			memcpy(dest, set_text + f->pos, n);
			got = (ssize_t)n;
		} else {
			off_t at = (off_t)index.list[f->entry].first * REC_BLOCK_BYTES + f->pos;

			got = wake() == 0 && flash_read(flash, at, dest, n) == 0 ? (ssize_t)n : -EIO;
		}
		if (got > 0) {
			f->pos += (uint32_t)got;
		}
	}
	k_mutex_unlock(&lock);
	return got;
}

static ssize_t fr_write(struct fs_file_t *filp, const void *src, size_t n)
{
	struct open_file *f = filp->filep;
	ssize_t put;

	if (!f->set) {
		return -EROFS;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (f->pos + n > SETFILE_MAX) {
		put = -ENOSPC;                   /* longer than any set file */
	} else {
		memcpy(set_text + f->pos, src, n);
		f->pos += (uint32_t)n;
		if (f->pos > set_len) {
			set_len = f->pos;
		}
		f->size = (uint32_t)set_len;
		put = (ssize_t)n;
	}
	k_mutex_unlock(&lock);
	return put;
}

static int fr_lseek(struct fs_file_t *filp, off_t off, int whence)
{
	struct open_file *f = filp->filep;
	int64_t pos;

	switch (whence) {
	case FS_SEEK_SET:
		pos = off;
		break;
	case FS_SEEK_CUR:
		pos = (int64_t)f->pos + off;
		break;
	case FS_SEEK_END:
		pos = (int64_t)f->size + off;
		break;
	default:
		return -EINVAL;
	}
	if (pos < 0 || pos > f->size) {
		return -EINVAL;
	}
	f->pos = (uint32_t)pos;
	return 0;
}

static off_t fr_tell(struct fs_file_t *filp)
{
	return (off_t)((struct open_file *)filp->filep)->pos;
}

static int fr_truncate(struct fs_file_t *filp, off_t length)
{
	struct open_file *f = filp->filep;

	if (!f->set || length < 0 || length > (off_t)f->size) {
		return -EROFS;
	}
	k_mutex_lock(&lock, K_FOREVER);
	set_len = (size_t)length;
	f->size = (uint32_t)set_len;
	f->pos = MIN(f->pos, f->size);
	k_mutex_unlock(&lock);
	return 0;
}

static int fr_sync(struct fs_file_t *filp)
{
	ARG_UNUSED(filp);
	return 0;
}

static int fr_opendir(struct fs_dir_t *dirp, const char *path)
{
	if (*name_of(path) != '\0') {
		return -ENOENT;                  /* the root alone */
	}
	dirp->dirp = (void *)(uintptr_t)1;       /* the next entry, 1-based */
	return 0;
}

static int fr_readdir(struct fs_dir_t *dirp, struct fs_dirent *entry)
{
	uintptr_t i = (uintptr_t)dirp->dirp - 1;
	char name[FLASHREC_NAME_MAX];

	if (erasing) {
		return -EBUSY;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (i < index.n) {
		flashrec_name(&index.list[i], name);
		strncpy(entry->name, name, sizeof(entry->name) - 1);
		entry->name[sizeof(entry->name) - 1] = '\0';
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = index.list[i].blocks * REC_BLOCK_BYTES;
		dirp->dirp = (void *)(i + 2);
	} else if (i == index.n && set_given) {
		strcpy(entry->name, SETFILE_NAME);
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = set_len;
		dirp->dirp = (void *)(i + 2);
	} else {
		entry->name[0] = '\0';           /* the end */
	}
	k_mutex_unlock(&lock);
	return 0;
}

static int fr_closedir(struct fs_dir_t *dirp)
{
	dirp->dirp = NULL;
	return 0;
}

static int fr_mount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static int fr_unmount(struct fs_mount_t *mountp)
{
	ARG_UNUSED(mountp);
	return 0;
}

static int fr_unlink(struct fs_mount_t *mountp, const char *path)
{
	int rc = -EROFS;

	ARG_UNUSED(mountp);
	k_mutex_lock(&lock, K_FOREVER);
	if (strcmp(name_of(path), SETFILE_NAME) == 0) {
		memset(set_text, 0, sizeof(set_text));    /* the key's bytes gone */
		set_len = 0;
		set_given = false;
		rc = 0;
	}
	k_mutex_unlock(&lock);
	return rc;
}

static int fr_stat(struct fs_mount_t *mountp, const char *path, struct fs_dirent *entry)
{
	const char *name = name_of(path);
	int rc = 0;

	ARG_UNUSED(mountp);
	if (erasing) {
		return -EBUSY;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (*name == '\0') {
		entry->type = FS_DIR_ENTRY_DIR;
		entry->name[0] = '\0';
		entry->size = 0;
	} else if (strcmp(name, SETFILE_NAME) == 0 && (set_given || set_writing)) {
		strcpy(entry->name, SETFILE_NAME);
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = set_len;
	} else {
		int e = flashrec_find(&index, name);

		if (e < 0) {
			rc = -ENOENT;
		} else {
			strncpy(entry->name, name, sizeof(entry->name) - 1);
			entry->name[sizeof(entry->name) - 1] = '\0';
			entry->type = FS_DIR_ENTRY_FILE;
			entry->size = index.list[e].blocks * REC_BLOCK_BYTES;
		}
	}
	k_mutex_unlock(&lock);
	return rc;
}

static int fr_statvfs(struct fs_mount_t *mountp, const char *path, struct fs_statvfs *st)
{
	ARG_UNUSED(mountp);
	ARG_UNUSED(path);
	if (erasing) {
		return -EBUSY;
	}
	k_mutex_lock(&lock, K_FOREVER);
	st->f_bsize = REC_BLOCK_BYTES;
	st->f_frsize = REC_BLOCK_BYTES;
	st->f_blocks = BLOCKS;
	st->f_bfree = BLOCKS - MIN(index.next, BLOCKS);
	k_mutex_unlock(&lock);
	return 0;
}

static const struct fs_file_system_t record_fs = {
	.open = fr_open,
	.read = fr_read,
	.write = fr_write,
	.lseek = fr_lseek,
	.tell = fr_tell,
	.truncate = fr_truncate,
	.sync = fr_sync,
	.close = fr_close,
	.opendir = fr_opendir,
	.readdir = fr_readdir,
	.closedir = fr_closedir,
	.mount = fr_mount,
	.unmount = fr_unmount,
	.unlink = fr_unlink,
	.stat = fr_stat,
	.statvfs = fr_statvfs,
};

static struct fs_mount_t mnt = {
	.type = FS_TYPE_EXTERNAL_BASE,
	.mnt_point = MOUNT,
};

/* ---- the card's interface ------------------------------------------------------------------- */

int card_init(void)
{
	int rc;

	if (!device_is_ready(flash)) {
		fault = FAULT_NO_CARD;
		LOG_WRN("the flash is not ready");
		return fault;
	}
	k_mutex_lock(&lock, K_FOREVER);
	rc = scan();
	k_mutex_unlock(&lock);
	if (rc) {
		fault = FAULT_FS_UNREADABLE;     /* the memory itself unreadable */
		LOG_WRN("the memory cannot be read");
		return fault;
	}
	rc = fs_register(FS_TYPE_EXTERNAL_BASE, &record_fs);
	if (rc == 0) {
		rc = fs_mount(&mnt);
	}
	if (rc) {
		fault = FAULT_FS_UNREADABLE;
		LOG_ERR("the record's file system %d", rc);
		return fault;
	}
	present = true;
	fault = FAULT_NONE;
	LOG_INF("the memory: %u sessions, %u MB free", index.n, card_free_mb());
	return 0;
}

int card_mount(void)
{
	return present ? 0 : fault;
}

int card_unmount(void)
{
	return 0;                                /* no host ever has it */
}

bool card_mounted(void)
{
	return present;
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
	return 0;                                /* none: the record raw */
}

uint8_t card_free_pct(void)
{
	return (uint8_t)((uint64_t)(BLOCKS - MIN(index.next, BLOCKS)) * 100 / BLOCKS);
}

uint32_t card_free_mb(void)
{
	return (uint32_t)((uint64_t)(BLOCKS - MIN(index.next, BLOCKS)) * REC_BLOCK_BYTES >> 20);
}

int card_open(uint16_t session, uint8_t belka, uint32_t rate_hz)
{
	int rc;

	ARG_UNUSED(rate_hz);                     /* no room to take: the record runs to the end */
	if (!present || file_open || erasing) {
		return -1;                       /* a file still open for a download reads on,
						  * the reads and the writes taking turns */
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (index.next + 1 >= BLOCKS) {
		fault = FAULT_CARD_FULL;         /* no room for a head and its end */
		k_mutex_unlock(&lock);
		return -1;
	}
	rc = wake();
	if (rc) {
		fault = FAULT_WRITE_FAILED;
		k_mutex_unlock(&lock);
		return -1;
	}
	fault = FAULT_NONE;                  /* a full or failed write before is this
					      * session's to find again */
	first = index.next;
	count = 0;
	open_session = session;
	open_belka = belka;
	head = tail = 0;
	dropped = 0;
	closing = false;
	file_open = true;
	k_mutex_unlock(&lock);
	return 0;
}

void card_put(const uint8_t block[REC_BLOCK_BYTES])
{
	uint32_t next = (head + 1) % RING_BLOCKS;

	if (!file_open || closing) {
		return;
	}
	if (next == tail || (fault == FAULT_CARD_FULL && block[0] != REC_END)) {
		dropped++;                       /* the buffer full, the flash busy too long; or
						  * the memory itself full — its end has its block */
		return;
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
	return file_open || erasing;
}

int card_erase(uint8_t what)
{
	if (!present) {
		return -ENODEV;
	}
	if (what != 1 && what != 2) {
		return -EINVAL;
	}
	if (file_open || erasing || readers > 0) {
		return -EBUSY;
	}
	/* every session or formatted: the same here, the whole chip erased, 200 s and up to
	 * 1 000 s (the sheet, 7.6), by the writer's thread; card_busy says while it runs, and
	 * every file call answers busy, the lock not held through it */
	fault = FAULT_NONE;
	erasing = true;
	erase_asked = true;
	k_sem_give(&blocks_ready);
	return 0;
}

bool card_recording(char *name, size_t cap, uint64_t *bytes)
{
	ARG_UNUSED(name);
	ARG_UNUSED(cap);
	ARG_UNUSED(bytes);
	return false;                            /* its sessions listed as they stand */
}

void card_set_file_allowed(bool allowed)
{
	set_allowed = allowed;
}

bool card_set_file_given(void)
{
	return set_given;
}

/* ---- the writer ----------------------------------------------------------------------------- */

/* the whole memory, minutes long: the lock is not held through it — `erasing` keeps every other
 * use of the memory out, so that MCUmgr's queue and main's loop are not held with it */
static void erase_all(void)
{
	int rc;

	k_mutex_lock(&lock, K_FOREVER);
	rc = wake();
	k_mutex_unlock(&lock);
	if (rc == 0) {
		rc = flash_erase(flash, 0, FLASH_BYTES);
	}
	if (rc == 0) {
		rc = wait_dies(K_MSEC(50));      /* the driver returned on one die's word */
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (rc == 0) {
		index.n = 0;
		index.next = 0;
		index.more = false;
		fault = FAULT_NONE;
		LOG_INF("the memory erased");
	} else {
		LOG_ERR("the erase failed (%d)", rc);
		fault = FAULT_WRITE_FAILED;
		(void)scan();                    /* what is there now */
	}
	erase_asked = false;
	erasing = false;
	k_mutex_unlock(&lock);
}

static void writer(void *a, void *b, void *c)
{
	ARG_UNUSED(a); ARG_UNUSED(b); ARG_UNUSED(c);
	for (;;) {
		bool idle = k_sem_take(&blocks_ready, K_MSEC(IDLE_MS)) != 0;

		if (erase_asked) {
			erase_all();
			continue;
		}
		while (file_open && tail != head) {
			if (first + count >= BLOCKS ||
			    (first + count + 1 == BLOCKS && ring[tail][0] != REC_END)) {
				/* the last block kept for the end alone */
				fault = FAULT_CARD_FULL;
				dropped++;
			} else {
				int rc;

				k_mutex_lock(&lock, K_FOREVER);
				rc = wake();
				if (rc == 0) {
					rc = flash_write(flash, (off_t)(first + count) * REC_BLOCK_BYTES,
							 ring[tail], REC_BLOCK_BYTES);
				}
				k_mutex_unlock(&lock);
				if (rc) {
					fault = FAULT_WRITE_FAILED;
					LOG_ERR("a write failed (%d)", rc);
				} else {
					count++;
				}
			}
			tail = (tail + 1) % RING_BLOCKS;
		}
		if (closing && file_open && tail == head) {
			k_mutex_lock(&lock, K_FOREVER);
			if (count > 0) {
				flashrec_add(&index, open_session, open_belka, first, count);
			}
			file_open = false;
			closing = false;
			k_mutex_unlock(&lock);
			LOG_INF("the session closed, %u blocks, %u dropped", count, dropped);
		}
		if (idle && !file_open && !erasing && readers == 0) {
			k_mutex_lock(&lock, K_FOREVER);
			if (readers == 0 && !file_open) {
				sleep_flash();
			}
			k_mutex_unlock(&lock);
		}
	}
}

K_THREAD_DEFINE(flash_writer, 2048, writer, NULL, NULL, NULL, WRITER_PRIO, 0, 0);
