#ifndef _JOOL_USR_ARGP_OPTIONS_H
#define _JOOL_USR_ARGP_OPTIONS_H

#include <argp.h>

#define PREFIX6_FORMAT "ADDR6/NUM"
#define PREFIX4_FORMAT "ADDR4/NUM"

/**
 * The flags the user can write as program parameters.
 */
enum argp_flags {
	/* Modes */
	ARGP_POOL6 = '6',
	ARGP_POOL4 = '4',
	ARGP_BIB = 'b',
	ARGP_SESSION = 's',
	ARGP_EAMT = 'e',
	ARGP_BLACKLIST = 7000,
	ARGP_RFC6791 = 6791,
	ARGP_LOGTIME = 'l',
	ARGP_GLOBAL = 'g',
	ARGP_PARSE_FILE = 'p',

	/* Operations */
	ARGP_DISPLAY = 'd',
	ARGP_COUNT = 'c',
	ARGP_TEST = 5001,
	ARGP_ADD = 'a',
	ARGP_UPDATE = 5000,
	ARGP_REMOVE = 'r',
	ARGP_FLUSH = 'f',

	/* Pools */
	ARGP_PREFIX = 1000,
	ARGP_ADDRESS = 1001,
	ARGP_QUICK = 'q',
	ARGP_MARK = 'm',
	ARGP_FORCE = 1002,

	/* BIB, session */
	ARGP_TCP = 't',
	ARGP_UDP = 'u',
	ARGP_ICMP = 'i',
	ARGP_NUMERIC_HOSTNAME = 'n',
	ARGP_CSV = 2022,
	ARGP_BIB_IPV6 = 2020,
	ARGP_BIB_IPV4 = 2021,

	/* General */
	ARGP_DROP_ADDR = 3000,
	ARGP_DROP_INFO = 3001,
	ARGP_DROP_TCP = 3002,
	ARGP_UDP_TO = 3010,
	ARGP_ICMP_TO = 3011,
	ARGP_TCP_TO = 3012,
	ARGP_TCP_TRANS_TO = 3013,
	ARGP_STORED_PKTS = 3014,
	ARGP_SRC_ICMP6ERRS_BETTER = 3015,
	ARGP_BIB_LOGGING,
	ARGP_SESSION_LOGGING,
	ARGP_RESET_TCLASS = 4002,
	ARGP_RESET_TOS = 4003,
	ARGP_NEW_TOS = 4004,
	ARGP_DF = 4005,
	ARGP_BUILD_FH = 4006,
	ARGP_BUILD_ID = 4007,
	ARGP_LOWER_MTU_FAIL = 4008,
	ARGP_PLATEAUS = 4010,
	ARGP_FRAG_TO = 4012,
	ARGP_ENABLE_TRANSLATION = 4013,
	ARGP_DISABLE_TRANSLATION = 4014,
	ARGP_COMPUTE_CSUM_ZERO = 4015,
	ARGP_EAM_HAIRPIN_MODE = 4018,
	ARGP_RANDOMIZE_RFC6791 = 4017,
	ARGP_ATOMIC_FRAGMENTS = 4016,
};

struct argp_option *build_options(void);

#endif /* _JOOL_USR_ARGP_OPTIONS_H */
