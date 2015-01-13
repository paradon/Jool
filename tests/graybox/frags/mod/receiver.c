#include "receiver.h"

#include <linux/netfilter.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <linux/ip.h>
#include <linux/spinlock.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/list.h>

#include "types.h"
#include "skb_ops.h"

static struct skb_user_db skb_ipv4_db;
static struct skb_user_db skb_ipv6_db;

static __u8 success_comparison;

struct skb_user_db {
	/* The root of the DB. */
	struct list_head list;
	/* Counter to know how many skb we have in the DB.*/
	__u8 counter;
	/* Counter to know when an incoming and an stored skb are the same.*/
	__u8 success_comparison;
	/* Counter to know when an incoming and an stored skb are not the same, either the incoming
	 * packet or the stored packet is wrong.*/
	__u8 fail_comparison;
	/* A lock to prevent concurrent access.*/
	spinlock_t lock;
};

struct skb_entry {
	/* skb created from user app.*/
	struct sk_buff *skb;
	/* A pointer in the database.*/
	struct list_head list;
};

static void delete_skb_entry(struct skb_entry *entry)
{
	skb_free(entry->skb);
	list_del(&entry->list);
	kfree(entry);
	return;
}

/**
 * Store a packet from the user app.
 */
int handle_skb_from_user(struct sk_buff *skb)
{
	struct skb_entry *entry;
	struct skb_user_db *skb_db;

	entry = kmalloc(sizeof(struct skb_entry), GFP_ATOMIC);
	if (!entry)
		return -ENOMEM;

	entry->skb = skb;

	if (skb->protocol == htons(ETH_P_IP)) {
		skb_db = &skb_ipv4_db;
	}
	else if (skb->protocol == htons(ETH_P_IPV6)) {
		skb_db = &skb_ipv6_db;
	}
	else {
		log_err("skb from user space have no protocol assigned.");
		kfree(entry);
		return -EINVAL;
	}

	spin_lock_bh(&skb_db->lock);
	list_add(&entry->list, &skb_db->list);
	skb_db->counter++;
	spin_unlock_bh(&skb_db->lock);

	return 0;
}

/**
 * Compare an incoming packet from the network against the packets that we store in the database.
 * First checks if the source and destination address are the same, if so, compare the entire
 * packet, else, compare to the next packet in the database until we found same addresses or
 * until all iterations ends.
 */
static unsigned int compare_incoming_skb(struct sk_buff *skb, struct skb_user_db *skb_db)
{
	struct list_head *current_hook, *next_hook;
	struct skb_entry *tmp_skb;
	int errors = 0;

	bool is_expected = false;

	if (list_empty(&skb_db->list)) { /* nothing to do. */
		return NF_ACCEPT;
	}

	spin_lock_bh(&skb_db->lock);
	list_for_each_safe(current_hook, next_hook, &skb_db->list) {
		tmp_skb = list_entry(current_hook, struct skb_entry, list);

		if (!skb_has_same_address(tmp_skb->skb, skb)) {
			continue;
		}

		is_expected = skb_compare(tmp_skb->skb, skb, &errors);
		if (is_expected) {/* we found a perfect match.*/
			delete_skb_entry(tmp_skb);
			skb_db->counter--;
			skb_db->success_comparison++;
			spin_unlock_bh(&skb_db->lock);
			log_info("success comparison :D");
			goto nf_drop;
		}
		/* else continue iterating through the list. */
	}

	if (errors) {
		/* Apparently the incoming skb has a twin from usrspace, or jool is translating wrong or a
		 * skb_from_user_space was incorrectly created, anyway delete the incoming packet and
		 * return NF_DROP;
		 */
		skb_db->fail_comparison++;
		spin_unlock_bh(&skb_db->lock);
		log_info("Apparently the incoming skb has a twin from usrspace, or jool is translating "
				"wrong or a skb_from_user_space was incorrectly created");
		goto nf_drop;
	}

	/* If we fall through here that means the incoming packet wasn't for the receiver module, so
	 * let it pass. */
	spin_unlock_bh(&skb_db->lock);
	return NF_ACCEPT;

nf_drop:
	/* TODO: If I return NF_DROP, it's not necessary to kfree the "incoming skb", right?
	 * skb_free(skb);
	 */
	return NF_DROP;
}

unsigned int receiver_incoming_skb4(struct sk_buff *skb)
{
	return compare_incoming_skb(skb, &skb_ipv4_db);
}

unsigned int receiver_incoming_skb6(struct sk_buff *skb)
{
	return compare_incoming_skb(skb, &skb_ipv6_db);
}

int receiver_init(void)
{
	skb_ipv4_db.counter = 0;
	INIT_LIST_HEAD(&skb_ipv4_db.list);
	spin_lock_init(&skb_ipv4_db.lock);

	skb_ipv6_db.counter = 0;
	INIT_LIST_HEAD(&skb_ipv6_db.list);
	spin_lock_init(&skb_ipv6_db.lock);

	success_comparison = 0;

	return 0;
}

static void destroy_aux(struct skb_user_db *skb_db)
{
	struct list_head *current_hook, *next_hook;
	struct skb_entry *skb_usr;

	list_for_each_safe(current_hook, next_hook, &skb_db->list) {
		skb_usr = list_entry(current_hook, struct skb_entry, list);
		delete_skb_entry(skb_usr);
	}
}

void receiver_destroy(void)
{
	destroy_aux(&skb_ipv4_db);
	destroy_aux(&skb_ipv6_db);

	log_info("***** IPv4 Stats *****");
	log_info("Successful comparisons: %u.", skb_ipv4_db.success_comparison);
	log_info("Failed comparisons: %u.", skb_ipv4_db.fail_comparison);
	log_info("skb remaining in db.: %u.", skb_ipv4_db.counter);
	log_info("***** IPv6 Stats *****");
	log_info("Successful comparisons: %u.", skb_ipv6_db.success_comparison);
	log_info("Failed comparisons: %u.", skb_ipv6_db.fail_comparison);
	log_info("skb remaining in db.: %u.", skb_ipv6_db.counter);
}


