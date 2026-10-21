#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include "strelka_mgmt_zephyr.h"

static const struct strelka_ops *ops;
static void *ops_ctx;

__weak struct smp_lock *strelka_mgmt_lock(struct net_buf *nb)
{
	ARG_UNUSED(nb);
	return NULL;
}

/* MCUmgr has opened the answer's map and will close it; the body's pairs and the code are
 * common/strelka_mgmt.c's */
static int handle(struct smp_streamer *c, uint8_t op, uint8_t id)
{
	if (ops == NULL) {
		return MGMT_ERR_ENOTSUP;
	}
	return strelka_mgmt_handle(op, id, c->reader->zs, c->writer->zs, ops,
				   strelka_mgmt_lock(c->reader->nb), ops_ctx);
}

#define READER(name, id) \
	static int name##_read(struct smp_streamer *c) { return handle(c, SMP_OP_READ, id); }
#define WRITER(name, id) \
	static int name##_write(struct smp_streamer *c) { return handle(c, SMP_OP_WRITE, id); }

READER(info, CMD_INFO)
WRITER(session, CMD_SESSION)
READER(sessions, CMD_SESSIONS)
READER(cal, CMD_CAL)
WRITER(cal, CMD_CAL)
WRITER(blink, CMD_BLINK)
WRITER(card, CMD_CARD)
WRITER(off, CMD_OFF)
WRITER(forget, CMD_FORGET)
WRITER(key, CMD_KEY)
WRITER(join, CMD_JOIN)
WRITER(record, CMD_RECORD)
WRITER(live, CMD_LIVE)
READER(search, CMD_SEARCH)
WRITER(search, CMD_SEARCH)
WRITER(take, CMD_TAKE)
READER(set, CMD_SET)
WRITER(drop, CMD_DROP)
READER(auth, CMD_AUTH)
WRITER(auth, CMD_AUTH)
WRITER(password, CMD_PASSWORD)
WRITER(table, CMD_TABLE)

static const struct mgmt_handler handlers[] = {
	[CMD_INFO] = { .mh_read = info_read },
	[CMD_SESSION] = { .mh_write = session_write },
	[CMD_SESSIONS] = { .mh_read = sessions_read },
	[CMD_CAL] = { .mh_read = cal_read, .mh_write = cal_write },
	[CMD_BLINK] = { .mh_write = blink_write },
	[CMD_CARD] = { .mh_write = card_write },
	[CMD_OFF] = { .mh_write = off_write },
	[CMD_FORGET] = { .mh_write = forget_write },
	[CMD_KEY] = { .mh_write = key_write },
	[CMD_JOIN] = { .mh_write = join_write },
	[CMD_RECORD] = { .mh_write = record_write },
	[CMD_LIVE] = { .mh_write = live_write },
	[CMD_SEARCH] = { .mh_read = search_read, .mh_write = search_write },
	[CMD_TAKE] = { .mh_write = take_write },
	[CMD_SET] = { .mh_read = set_read },
	[CMD_DROP] = { .mh_write = drop_write },
	[CMD_AUTH] = { .mh_read = auth_read, .mh_write = auth_write },
	[CMD_PASSWORD] = { .mh_write = password_write },
	[CMD_TABLE] = { .mh_write = table_write },
};

static struct mgmt_group group = {
	.mg_handlers = handlers,
	.mg_handlers_count = ARRAY_SIZE(handlers),
	.mg_group_id = SMP_GROUP_STRELKA,
};

static void register_group(void)
{
	mgmt_register_group(&group);
}

MCUMGR_HANDLER_DEFINE(strelka_mgmt, register_group);

void strelka_mgmt_zephyr_init(const struct strelka_ops *o, void *ctx)
{
	ops = o;
	ops_ctx = ctx;
}
