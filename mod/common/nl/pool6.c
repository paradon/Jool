#include "nat64/mod/common/nl/pool6.h"

#include "nat64/mod/common/types.h"
#include "nat64/mod/common/nl/nl_common.h"
#include "nat64/mod/common/nl/nl_core2.h"
#include "nat64/mod/common/pool6.h"
#include "nat64/mod/stateful/session/db.h"

static const enum config_mode COMMAND = MODE_POOL6;

static int pool6_entry_to_userspace(struct ipv6_prefix *prefix, void *arg)
{
	return nlbuffer_write(arg, prefix, sizeof(*prefix));
}

static int handle_pool6_display(struct pool6 *pool, struct genl_info *info,
		union request_pool6 *request)
{
	struct nl_core_buffer *buffer;
	struct ipv6_prefix *prefix;
	int error;

	log_debug("Sending pool6 to userspace.");

	error = nlbuffer_new(&buffer, nlbuffer_data_max_size());
	if (error)
		return nlcore_respond_error(info, COMMAND, error);

	prefix = request->display.prefix_set ? &request->display.prefix : NULL;
	error = pool6_foreach(pool, pool6_entry_to_userspace, buffer, prefix);
	buffer->pending_data = error > 0;
	error = (error >= 0)
			? nlbuffer_send(info, COMMAND, buffer)
			: nlcore_respond_error(info, COMMAND, error);

	nlbuffer_free(buffer);
	return error;
}

static int handle_pool6_count(struct pool6 *pool, struct genl_info *info)
{
	__u64 count;
	int error;

	log_debug("Returning pool6's prefix count.");
	error = pool6_count(pool, &count);
	if (error)
		return nlcore_respond_error(info, COMMAND, error);

	return nlcore_respond_struct(info, COMMAND, &count, sizeof(count));
}

static int handle_pool6_add(struct pool6 *pool, union request_pool6 *request)
{
	if (verify_superpriv())
		return -EPERM;

	log_debug("Adding a prefix to pool6.");
	return pool6_add(pool, &request->add.prefix);
}

static int handle_pool6_rm(struct xlator *jool, union request_pool6 *request)
{
	int error;

	if (verify_superpriv())
		return -EPERM;

	log_debug("Removing a prefix from pool6.");
	error = pool6_rm(jool->pool6, &request->rm.prefix);

	if (xlat_is_nat64() && !request->flush.quick) {
		sessiondb_delete_taddr6s(jool->nat64.session,
				&request->rm.prefix);
	}

	return error;
}

static int handle_pool6_flush(struct xlator *jool, union request_pool6 *request)
{
	int error;

	if (verify_superpriv())
		return -EPERM;

	log_debug("Flushing pool6.");
	error = pool6_flush(jool->pool6);

	if (xlat_is_nat64() && !request->flush.quick)
		sessiondb_flush(jool->nat64.session);

	return error;
}

int handle_pool6_config(struct xlator *jool, struct genl_info *info)
{
	struct request_hdr *jool_hdr = get_jool_hdr(info);
	union request_pool6 *request = (union request_pool6 *)(jool_hdr + 1);
	int error;

	error = validate_request_size(jool_hdr, sizeof(*request));
	if (error)
		return nlcore_respond_error(info, COMMAND, error);

	switch (jool_hdr->operation) {
	case OP_DISPLAY:
		return handle_pool6_display(jool->pool6, info, request);
	case OP_COUNT:
		return handle_pool6_count(jool->pool6, info);
	case OP_ADD:
	case OP_UPDATE:
		error = handle_pool6_add(jool->pool6, request);
		break;
	case OP_REMOVE:
		error = handle_pool6_rm(jool, request);
		break;
	case OP_FLUSH:
		error = handle_pool6_flush(jool, request);
		break;
	default:
		log_err("Unknown operation: %d", jool_hdr->operation);
		error = -EINVAL;
	}

	return nlcore_respond(info, COMMAND, error);
}