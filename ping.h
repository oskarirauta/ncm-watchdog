#ifndef __ping_h__
#define __ping_h__ 1

#define PING_PERM_ERROR		-99
#define PING_DNS_FAILURE	-1
#define PING_ERROR		-2
#define PING_FAILED		0
#define PING_SUCCESS		1

#define STR_PING_PERM_ERROR	"Ping failed. Permission error. (Not running as root?)"
#define STR_PING_DNS_FAILURE	"Ping failed. DNS error. Cannot map hostname to IP address."
#define STR_PING_ERROR		"Ping error. Cannot ping host. Connection blocked or ping sent only partially."
#define STR_PING_FAILED		"Ping failed. Host not reached."
#define STR_PING_SUCCESS	"Ping success. Host reached."
#define STR_PING_UNKNOWN	"Unknown result for ping function."

#define DATALEN			56
#define IPADDRLEN		60
#define ICMPLEN			76
  
int ping(const char *host, int ping_timeout);
char *pingResult(int result);

#endif
