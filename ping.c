#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <string.h>
#include "ping.h"
 
static int in_cksum(unsigned short *buf, int sz) {
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;
   
	while ( nleft > 1 ) {
		sum += *w++;
		nleft -= 2;
	}
   
	if ( nleft == 1 ) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}
   
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return (ans);
}
 
int ping(const char *host, int ping_timeout) {
	struct hostent *h;
	struct sockaddr_in pingaddr;
	struct icmp *packet;
	int pingsocket, c;
	char pkt[DATALEN + IPADDRLEN + ICMPLEN];

	if (( pingsocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0 )
		return(PING_PERM_ERROR); // Error, cannot create socket (not running as root?)

	setuid(getuid());

	memset(&pingaddr, 0, sizeof(struct sockaddr_in));

	pingaddr.sin_family = AF_INET;
	if (!( h = gethostbyname(host)))
		return(PING_DNS_FAILURE); // Unknown host, dns failed?

	memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));
	
	packet = (struct icmp *)pkt;
	memset(packet, 0, sizeof(packet));
	packet -> icmp_type = ICMP_ECHO;
	packet -> icmp_cksum = in_cksum((unsigned short *)packet, sizeof(pkt));

	c = sendto(pingsocket, pkt, sizeof(pkt), 0, (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (c < 0 || c != sizeof(pkt))
		return(PING_ERROR); // Partially failed, not able to send complete packet

	time_t timeout = time(NULL) + ping_timeout;

	struct timeval tv; // set timeout 100ms
	tv.tv_sec = 0;
//	tv.tv_usec = 100000;
	tv.tv_usec = 10000 * ping_timeout;
	if (setsockopt(pingsocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
		perror("Socket error");

	while ( time(NULL) < timeout ) {

		struct sockaddr_in from;
		int fromlen = sizeof(from);

		if (( c = recvfrom(pingsocket, pkt, sizeof(pkt), 0, (struct sockaddr *) &from, &fromlen)) < 0 )
			continue;

		if ( c >= ICMPLEN ) {
			struct iphdr *iphdr = (struct iphdr *) pkt;

			packet = (struct icmp *) (pkt + (iphdr->ihl << 2));
			if (packet->icmp_type == ICMP_ECHOREPLY)
				return(PING_SUCCESS); // Got reply succesfully
		}
	}

	return(PING_FAILED); // Ping failed
}

char *pingResult(int result) {
	switch(result) {
		case PING_PERM_ERROR:
			return STR_PING_PERM_ERROR;
			break;
		case PING_DNS_FAILURE:
			return STR_PING_DNS_FAILURE;
			break;
		case PING_ERROR:
			return STR_PING_ERROR;
			break;
		case PING_SUCCESS:
			return STR_PING_SUCCESS;
			break;
		case PING_FAILED:
			return STR_PING_FAILED;
			break;
		default:
			break;
	}
	return STR_PING_UNKNOWN;
}
