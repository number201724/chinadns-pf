#include "netutils.h"
#include "logutils.h"
#include "chinadns.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "radix.h"
#include <err.h>
#include <netdb.h>

/* since linux 3.9 */
#ifndef SO_REUSEPORT
  #define SO_REUSEPORT 15
#endif

#define s6_addr8  __u6_addr.__u6_addr8
#define s6_addr16 __u6_addr.__u6_addr16
#define s6_addr32 __u6_addr.__u6_addr32
#define s6_addr   __u6_addr.__u6_addr8

#define	FILLIN_SIN(sin, addr)			\
	do {					\
		(sin).sin_len = sizeof(sin);	\
		(sin).sin_family = AF_INET;	\
		(sin).sin_addr = (addr);	\
	} while (0)

#define	FILLIN_SIN6(sin6, addr)			\
	do {					\
		(sin6).sin6_len = sizeof(sin6);	\
		(sin6).sin6_family = AF_INET6;	\
		(sin6).sin6_addr = (addr);	\
	} while (0)

/* setsockopt(IPV6_V6ONLY) */
static inline void set_ipv6_only(int sockfd) {
    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int))) {
        LOGERR("[set_ipv6_only] setsockopt(%d, IPV6_V6ONLY): (%d) %s", sockfd, errno, strerror(errno));
        exit(errno);
    }
}

/* setsockopt(SO_REUSEADDR) */
static inline void set_reuse_addr(int sockfd) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        LOGERR("[set_reuse_addr] setsockopt(%d, SO_REUSEADDR): (%d) %s", sockfd, errno, strerror(errno));
        exit(errno);
    }
}

/* setsockopt(SO_REUSEPORT) */
void set_reuse_port(int sockfd) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))) {
        LOGERR("[set_reuse_port] setsockopt(%d, SO_REUSEPORT): (%d) %s", sockfd, errno, strerror(errno));
        exit(errno);
    }
}

/* create a udp socket (v4/v6) */
int new_udp_socket(int family) {
    int sockfd = socket(family, SOCK_DGRAM | SOCK_NONBLOCK, 0); /* since Linux 2.6.27 */
    if (sockfd < 0) {
        LOGERR("[new_udp_socket] failed to create udp%c socket: (%d) %s", family == AF_INET ? '4' : '6', errno, strerror(errno));
        exit(errno);
    }
    if (family == AF_INET6) set_ipv6_only(sockfd);
    set_reuse_addr(sockfd);
    return sockfd;
}

/* AF_INET or AF_INET6 or -1(invalid) */
int get_ipstr_family(const char *ipstr) {
    if (!ipstr) return -1;
    char buffer[IPV6_BINADDR_LEN]; /* v4 or v6 */
    if (inet_pton(AF_INET, ipstr, buffer) == 1) {
        return AF_INET;
    } else if (inet_pton(AF_INET6, ipstr, buffer) == 1) {
        return AF_INET6;
    } else {
        return -1;
    }
}

/* build ipv4 address structure */
static inline void build_socket_addr4(skaddr4_t *skaddr, const char *ipstr, portno_t port) {
    skaddr->sin_family = AF_INET;
    inet_pton(AF_INET, ipstr, &skaddr->sin_addr);
    skaddr->sin_port = htons(port);
}

/* build ipv6 address structure */
static inline void build_socket_addr6(skaddr6_t *skaddr, const char *ipstr, portno_t port) {
    skaddr->sin6_family = AF_INET6;
    inet_pton(AF_INET6, ipstr, &skaddr->sin6_addr);
    skaddr->sin6_port = htons(port);
}

/* build v4/v6 address structure */
void build_socket_addr(int family, void *skaddr, const char *ipstr, portno_t portno) {
    if (family == AF_INET) {
        build_socket_addr4(skaddr, ipstr, portno);
    } else {
        build_socket_addr6(skaddr, ipstr, portno);
    }
}

/* parse ipv4 address structure */
static inline void parse_socket_addr4(const skaddr4_t *skaddr, char *ipstr, portno_t *port) {
    inet_ntop(AF_INET, &skaddr->sin_addr, ipstr, INET_ADDRSTRLEN);
    *port = ntohs(skaddr->sin_port);
}

/* parse ipv6 address structure */
static inline void parse_socket_addr6(const skaddr6_t *skaddr, char *ipstr, portno_t *port) {
    inet_ntop(AF_INET6, &skaddr->sin6_addr, ipstr, INET6_ADDRSTRLEN);
    *port = ntohs(skaddr->sin6_port);
}

/* parse v4/v6 address structure */
void parse_socket_addr(const void *skaddr, char *ipstr, portno_t *portno) {
    if (((const skaddr4_t *)skaddr)->sin_family == AF_INET) {
        parse_socket_addr4(skaddr, ipstr, portno);
    } else {
        parse_socket_addr6(skaddr, ipstr, portno);
    }
}

struct radix_node_head	*pfrkt_ip4;
struct radix_node_head	*pfrkt_ip6;

//
// Purpose: 
//
void trim( char *str )
{
	char *start = str;
	char *end = start + strlen( str );

	while( --end >= start ) {   /* trim right */
		if( !isspace( *end ) )
			break;
	}

	*(++end) = '\0';

	while( isspace( *start ) )    /* trim left */
		start++;

	if( start != str )          /* there is a string */
		memmove( str, start, end - start + 1 );
}


static void
pfr_prepare_network(union sockaddr_union *sa, int af, int net)
{
	int	i;

	bzero(sa, sizeof(*sa));
	if (af == AF_INET) {
		sa->sin.sin_len = sizeof(sa->sin);
		sa->sin.sin_family = AF_INET;
		sa->sin.sin_addr.s_addr = net ? htonl(-1 << (32-net)) : 0;
	} else if (af == AF_INET6) {
		sa->sin6.sin6_len = sizeof(sa->sin6);
		sa->sin6.sin6_family = AF_INET6;
		for (i = 0; i < 4; i++) {
			if (net <= 32) {
				sa->sin6.sin6_addr.s6_addr32[i] =
				    net ? htonl(-1 << (32-net)) : 0;
				break;
			}
			sa->sin6.sin6_addr.s6_addr32[i] = 0xFFFFFFFF;
			net -= 32;
		}
	}
}

static void load_chnroute4(void)
{
    char filename[256];
    char ipstr[128];
    char ip[64];
    char maskstr[32];
    FILE *fh;
    union sockaddr_union	 sa, mask;
    struct in_addr addr;
    struct pfr_kentry *ke;
    struct radix_node	*rn;

    sprintf(filename, "%s.txt", g_ipset_setname4);
    
    fh = fopen(filename, "r");
    if (fh == NULL) {
        err(EXIT_FAILURE, "fopen failed");
    }

    while(fgets(ipstr, sizeof(ipstr)-1, fh) != NULL)
    {
        trim(ipstr);

        char *s = strstr(ipstr, "/");
        if(!s) {
            continue;
        }

        memset(ip, 0, sizeof(ip));
        memcpy(ip, ipstr, s-ipstr);

        s++;

        strcpy(maskstr, s);

        addr.s_addr = inet_addr(ip);
        FILLIN_SIN(sa.sin, addr);

        pfr_prepare_network(&mask, AF_INET, atoi(maskstr));


        ke = malloc(sizeof(struct pfr_kentry));
        bzero(ke, sizeof(struct pfr_kentry));

        
        FILLIN_SIN(ke->pfrke_sa.sin, addr);
        ke->pfrke_af = AF_INET;
        ke->pfrke_net = atoi(maskstr);
        
        rn = rn_addroute(&ke->pfrke_sa, &mask, &pfrkt_ip4->rh, ke->pfrke_node);


    }

    fclose(fh);
}


static void load_chnroute6(void)
{
    char filename[256];
    char ipstr[128];
    char ip[64];
    char maskstr[32];
    FILE *fh;
    union sockaddr_union	 sa, mask;
    struct in6_addr addr;
    struct pfr_kentry *ke;
    struct radix_node	*rn;
    struct addrinfo		 hints, *res;

    sprintf(filename, "%s.txt", g_ipset_setname6);
    
    fh = fopen(filename, "r");
    if (fh == NULL) {
        err(EXIT_FAILURE, "fopen failed");
    }

    while(fgets(ipstr, sizeof(ipstr)-1, fh) != NULL)
    {
        trim(ipstr);

        char *s = strrchr(ipstr, '/');
        if(!s) {
            continue;
        }

        memset(ip, 0, sizeof(ip));
        memcpy(ip, ipstr, s-ipstr);

        s++;

        strcpy(maskstr, s);

        res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM; /*dummy*/
        hints.ai_flags = AI_NUMERICHOST;
	    if (getaddrinfo(ip, "0", &hints, &res) != 0) {
            err(EXIT_FAILURE, "bad ipv6 addr:%s\n", ip);
	    }

		memcpy(&addr,
		    &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr,
		    sizeof(addr));
            
		freeaddrinfo(res);
        
        FILLIN_SIN6(sa.sin6, addr);

        pfr_prepare_network(&mask, AF_INET6, atoi(maskstr));

        ke = malloc(sizeof(struct pfr_kentry));
        bzero(ke, sizeof(struct pfr_kentry));

        
        FILLIN_SIN6(ke->pfrke_sa.sin6, addr);
        ke->pfrke_af = AF_INET6;
        ke->pfrke_net = atoi(maskstr);

        rn = rn_addroute(&ke->pfrke_sa, &mask, &pfrkt_ip6->rh, ke->pfrke_node);

    }

    fclose(fh);
}


void chnroute_init(void) {

    if (!rn_inithead((void **)&pfrkt_ip4, offsetof(struct sockaddr_in, sin_addr) * 8) ||
	    !rn_inithead((void **)&pfrkt_ip6, offsetof(struct sockaddr_in6, sin6_addr) * 8)) 
    {
        err(EXIT_FAILURE, "rn_inithead failed");
	}

    load_chnroute4();
    load_chnroute6();
}



/* check given ipaddr is exists in ipset */
bool ipset_addr_is_exists(const void *addr_ptr, bool is_ipv4) {
    union sockaddr_union sa;
    struct in_addr addr;
    struct in6_addr addr6;
    void *rn;

    if(is_ipv4) {
        memcpy(&addr, addr_ptr, sizeof(struct in_addr));
        FILLIN_SIN(sa.sin, addr);
        rn = rn_match(&sa, &pfrkt_ip4->rh);

        return rn != NULL;
    }

    memcpy(&addr6, addr_ptr, sizeof(struct in6_addr));
    FILLIN_SIN6(sa.sin6, addr6);
    rn = rn_match(&sa, &pfrkt_ip6->rh);

    return rn != NULL;
}
