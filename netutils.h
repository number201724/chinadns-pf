#ifndef CHINADNS_NG_NETUTILS_H
#define CHINADNS_NG_NETUTILS_H

#define _GNU_SOURCE
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "radix.h"
#undef _GNU_SOURCE

/* ipv4/ipv6 address length (binary) */
#define IPV4_BINADDR_LEN 4  /* 4byte, 32bit */
#define IPV6_BINADDR_LEN 16 /* 16byte, 128bit */

/* struct sockaddr_* typedef */
typedef struct sockaddr_in  skaddr4_t;
typedef struct sockaddr_in6 skaddr6_t;

#pragma pack(push,1)
#ifndef _SOCKADDR_UNION_DEFINED
#define	_SOCKADDR_UNION_DEFINED

union sockaddr_union {
	struct sockaddr		sa;
	struct sockaddr_in	sin;
	struct sockaddr_in6	sin6;
};

#endif /* _SOCKADDR_UNION_DEFINED */

struct pfr_kentry {
	struct radix_node	 pfrke_node[2];
	union sockaddr_union	 pfrke_sa;
	u_int8_t		 pfrke_af;
	u_int8_t		 pfrke_net;
};


#pragma pack(pop)
/* socket port number typedef */
typedef uint16_t portno_t;

/* setsockopt(SO_REUSEPORT) */
void set_reuse_port(int sockfd);

/* create a udp socket (v4/v6) */
int new_udp_socket(int family);

/* AF_INET or AF_INET6 or -1(invalid) */
int get_ipstr_family(const char *ipstr);

/* build ipv4/ipv6 address structure */
void build_socket_addr(int family, void *skaddr, const char *ipstr, portno_t portno);

/* parse ipv4/ipv6 address structure */
void parse_socket_addr(const void *skaddr, char *ipstr, portno_t *portno);

/* init netlink socket for ipset query */
void chnroute_init(void);

/* check given ipaddr is exists in ipset */
bool ipset_addr_is_exists(const void *addr_ptr, bool is_ipv4);

#endif
