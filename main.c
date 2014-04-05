#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <time.h>

#include "defines.h"
#include "hostlist.h"
#include "util.h"
#include "ping.h"

int interval = DEF_INTERVAL;
int firstcheck = FIRST_CHECK;
int use_dns = 0;
int dns_pri = 0;
int treatErrors = 0;
int treatDNS = 0;
int verboseMode = 0;
int failsAllowed = RESTART_AMOUNT;
int pingTimeout = PING_TIMEOUT;
int needDNSRefresh = 0;
char *ifd;
char *json_param;

int failedPings = 0;
int ifdPrevious = 0;
int ifdState = 0;

static struct ubus_context *ctx;
static struct blob_buf b;

static void cleanUp(void);

void addDNS(void) {
	FILE *fd;
	char s[200];

	clearList(dnsList);

	if (( fd = fopen(RESOLVCONF_LOCATION, "r")) != NULL ) {
		while ( fgets(s, 200, fd)) {
			if ( s[0] == '#' )
				continue;

			int i = 0;

			if ( strncmp(s, "nameserver ", 11) == 0 ) i = 11;
			else if ( strncmp(s, "nameservers ", 12) == 0 ) i = 12;

			if ( i != 0 ) {
				char *dnsname = strip_copy(s+i);
				addHost(dnsList, dnsname);
				free(dnsname);
			}
		}
		fclose(fd);
	} else if ( use_dns )
		fprintf(stderr, "Warning: Cannot open %s - DNS servers not added.\r\n", RESOLVCONF_LOCATION);
}

static void receive_nothing(struct ubus_request *req, int type, struct blob_attr *msg) {
	return;
}

static void receive_ifd_status(struct ubus_request *req, int type, struct blob_attr *msg) {

	struct blob_attr *pos;
	int rem = blobmsg_data_len(msg);

	if (!msg)
		return;

	__blob_for_each_attr(pos, blobmsg_data(msg), rem) {
		if (!blobmsg_check_attr(pos, false))
			continue;

		if (strcmp(blobmsg_name(pos), "up") == 0 ) {
			void *data = blobmsg_data(pos);
			ifdState = *(uint8_t *)data ? 1 : 0;
			free(data);
		}
	}
}

static void get_ifd_status(void) {
	int ret;
	uint32_t id;

	blob_buf_init(&b, 0);
	if ( !blobmsg_add_json_from_string(&b, json_param)) {
		fprintf(stderr, "Failed to parse ubus message data\n");
		cleanUp();
		exit(-1);
	}

	if ( ubus_lookup_id(ctx, "network.device", &id) != 0 ) {
		ifdState = 0;
		return;
	}

	if ( ubus_invoke(ctx, id, "status", b.head, receive_ifd_status,
			NULL, UBUS_TIMEOUT * 1000) != 0 )
		ifdState = 0;
}

static void call_ifd(int state) {
	int ret;
	uint32_t id;

	blob_buf_init(&b, 0);
	
	if ( ubus_lookup_id(ctx, "network.interface.wan", &id) != 0 )
		return;

	ubus_invoke(ctx, id, state ? "up" : "down", b.head, receive_nothing,
			NULL, UBUS_TIMEOUT * 1000);
}

static int pingTest(void) {
	int fAttempts = 0;
	int sAttempts = 0;
	int dnsErrors = 0;
	int comErrors = 0;
	int ret;
	hostItem *h;

	h = dnsList -> next;
	if (( use_dns ) && ( dns_pri ) && ( h != NULL ))
		while ( h != NULL ) {
			if ( verboseMode )
				printf("Trying to ping host %s\r\n", h -> host);
			ret = ping( h -> host, pingTimeout );
			switch ( ret ) {
				case PING_PERM_ERROR:
							fprintf(stderr, "You must be root to run this program.\n");
							cleanUp();
							exit(-1);
							break;
				case PING_DNS_FAILURE:
							dnsErrors++;
							if ( verboseMode )
								printf("DNS failure when trying to ping %s\r\n", h -> host);
							break;
				case PING_ERROR:
							comErrors++;
							if ( verboseMode )
								printf("Incomplete ping for host %s\r\n", h -> host);
							break;
				case PING_FAILED:
							fAttempts++;
							if ( verboseMode )
								printf("Ping failed for host %s\r\n", h -> host);
							break;
				case PING_SUCCESS:
							sAttempts++;
							if ( verboseMode )
								printf("Ping reached host %s\r\n", h -> host);
							return 1;
							break;
				default:
							break;
			}
			h = h -> next;
		}

	h = hostList -> next;
	while ( h != NULL ) {
		if ( verboseMode )
			printf("Trying to ping host %s\r\n", h -> host);
		ret = ping( h -> host, pingTimeout );
		switch ( ret ) {
			case PING_PERM_ERROR:
						fprintf(stderr, "You must be root to run this program.\n");
						cleanUp();
						exit(-1);
						break;
			case PING_DNS_FAILURE:
						dnsErrors++;
						if ( verboseMode )
							printf("DNS failure when trying to ping %s\r\n", h -> host);
						break;
			case PING_ERROR:
						comErrors++;
						if ( verboseMode )
							printf("Incomplete ping for host %s\r\n", h -> host);
						break;
			case PING_FAILED:
						fAttempts++;
						if ( verboseMode )
							printf("Ping failed for host %s\r\n", h -> host);
						break;
			case PING_SUCCESS:
						sAttempts++;
						if ( verboseMode )
							printf("Ping reached host %s\r\n", h -> host);
						return 1;
						break;
			default:
						break;
		}
		h = h -> next;
	}

	h = dnsList -> next;
	if (( use_dns ) && ( !dns_pri ) && ( h != NULL ))
		while ( h != NULL ) {
			if ( verboseMode )
				printf("Trying to ping host %s\r\n", h -> host);
			ret = ping( h -> host, pingTimeout );
			switch ( ret ) {
				case PING_PERM_ERROR:
							fprintf(stderr, "You must be root to run this program.\n");
							cleanUp();
							exit(-1);
							break;
				case PING_DNS_FAILURE:
							dnsErrors++;
							if ( verboseMode )
								printf("DNS failure when trying to ping %s\r\n", h -> host);
							break;
				case PING_ERROR:
							comErrors++;
							if ( verboseMode )
								printf("Incomplete ping for host %s\r\n", h -> host);
							break;
				case PING_FAILED:
							fAttempts++;
							if ( verboseMode )
								printf("Ping failed for host %s\r\n", h -> host);
							break;
				case PING_SUCCESS:
							sAttempts++;
							if ( verboseMode )
								printf("Ping reached host %s\r\n", h -> host);
							return 1;
							break;
				default:
							break;
                }
                h = h -> next;
        }

	if (( !fAttempts ) && ( !sAttempts ) && ( !dnsErrors ) && ( !comErrors )) {
		if ( verboseMode ) {
			printf("No pings made. Host lists were empty.\r\n");
			if ( use_dns )
				printf("Refreshing nameservers on next time timer occurs.\r\n");
		}
		if ( use_dns )
			needDNSRefresh = 1;
		return 1;
	}

	if ( verboseMode ) {
		printf("Ping(s) results:\r\n");
		printf("Failed pings:    %d\r\n", fAttempts);
		printf("Succeeded pings: %d\r\n", sAttempts);
		printf("DNS errors:      %d\r\n", dnsErrors);
		printf("Ping errors:     %d\r\n", comErrors);
	}

	if (( !fAttempts ) && ( !sAttempts ) && ( !treatErrors ) && ( treatDNS ) && ( comErrors > 0 ))
		return 1;

	if (( !fAttempts ) && ( !sAttempts ) && ( treatErrors ) && ( !treatDNS ) && ( dnsErrors > 0 ))
		return 1;

	return 0;
}

static void cleanUp(void) {
	removeHosts();
        if ( ifd )
		free( ifd );
	if ( json_param )
		free( json_param );
	if ( ctx )
		ubus_free(ctx);
}

static void printVersion(void) {
	printf("%s v%s", APPNAME, VERSION);
	if (strlen(SUBVERSION) > 0 ) printf(" (%s)", SUBVERSION);
	printf("\r\nWritten by Oskari Rauta.\r\n\n");
}

void printStatistics(void) {
	int a;
	hostItem *h;

	printVersion();

	printf("Network interface: '%s'\r\n", !ifd ? "(null)" : ifd);
	printf("Network interface state: %s\r\n", ifdState ? "up" : "down");
	printf("Use dns: %s\r\n", use_dns ? "yes" : "no");
	printf("DNS is primary: %s\r\n", dns_pri ? "yes": "no");
	printf("Treat ping errors as failures to connect: %s\r\n", treatErrors ? "yes" : "no");
	printf("Treat DNS errors as failures to connect: %s\r\n", treatDNS ? "yes" : "no");
	printf("Verbose mode: %s\r\n", verboseMode ? "yes" : "no");
	printf("Primary timer interval for checking: %d seconds\r\n", interval);
	printf("Secondary timer interval: %d seconds\r\n", firstcheck);
	printf("Allowed number of failed ping cycles before connection restarts: %d\r\n", failsAllowed);
	printf("Ping timeout: %d seconds\r\n", pingTimeout);
	printf("Refreshing DNS on first cycle: %s\r\n", needDNSRefresh ? "yes" : "no");
	printf("\r\n");
	if (( use_dns ) && ( dnsList -> next == NULL ))
		printf("Nameserver addresses are used, but there are none in the list.\r\n\n");
	if (( use_dns ) && ( dnsList -> next != NULL )) {
		printf("List of nameserver addresses to check ( %s ):\r\n", dns_pri ? "primary" : "non-primary");
		a = 1;
		h = dnsList -> next;
		while ( h != NULL ) {
			printf("%d: %s\r\n", a++, h -> host);
			h = h -> next;
		}
		printf("\r\n");
	}
	if ( hostList -> next == NULL ) {
		printf("No hosts to check in the list.\r\n\n");
		return;
	}
	printf("List of checked hosts:\r\n");
	a = 1;
	h = hostList -> next;
	while ( h != NULL ) {
		printf("%d: %s\r\n", a++, h -> host);
		h = h -> next;
	}
	printf("\r\n");
}

void usage(char *progname) {
	printVersion();
	printf("Usage: %s options\r\n", progname);
	printf("Options:\r\n");
	printf(" -d interface   set network interface\r\n");
	printf(" -n             Add DNS servers from resolv.conf.auto\r\n");
	printf(" -p             Used with -d. Adds DNS servers as first to the\r\n");
	printf("                list of checked hosts\r\n");
	printf(" -t ip/address  add host ip/address to check\r\n");
	printf(" -i seconds     primary timer interval for checking ( default: %d seconds )\r\n", DEF_INTERVAL);
	printf(" -f seconds	secondary timer interval ( default: %d seconds )\r\n", FIRST_CHECK);
	printf(" -e             Treat ping errors as failures\r\n");
	printf(" -m             Treat DNS errors as failures\r\n");
	printf(" -c             Count of failured ping attempts before restarting\r\n");
	printf("                connection ( default: %d )\r\n", RESTART_AMOUNT);
	printf(" -w seconds     set ping timeout ( default: %d )\r\n", PING_TIMEOUT);
	printf(" -s <socket>    Set the ubus's unix domain socket to connect to\r\n");
	printf(" -v		Verbose mode\r\n");
	printf("\r\nAtleast network interface and one host is required, either added with -d or -t.\r\n");
	printf("If you add nameservers, but connection is down, they will be refreshed\r\nonce connection is up.\r\n");
}

int main(int argc, char **argv) {
	char *progname = argv[0];
	int ch;
	const char *ubus_socket = NULL;
	time_t nextEvent;

	initHosts();

	while (( ch = getopt(argc, argv, "d:npt:i:f:emc:w:vs:")) != -1 ) {
		switch (ch) {
			case 'd':
				ifd = (char*)malloc(strlen(optarg)+1);
				strcpy(ifd, optarg);
				json_param = (char*)malloc(16 + strlen(optarg));
				sprintf(json_param, "{ \"name\": \"%s\" }", optarg);
				break;
			case 'n':
				use_dns = 1;
				break;
			case 'p':
				use_dns = 1;
				dns_pri = 1;
				break;
			case 't':
				addHost(hostList, optarg);
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			case 'f':
				firstcheck = atoi(optarg);
				break;
			case 'e':
				treatErrors = 1;
				break;
			case 'm':
				treatDNS = 1;
				break;
			case 'c':
				failsAllowed = atoi(optarg);
				break;
			case 'w':
				pingTimeout = atoi(optarg);
				break;
			case 'v':
				verboseMode = 1;
				break;
			case 's':
				ubus_socket = optarg;
				break;
			default:
				usage(progname);
				removeHosts();
				return 0;
		}
	}

	if ( use_dns ) {
		addDNS();
		if ( dnsList -> next == NULL )
			needDNSRefresh = 1;
	}

	if ( ! ( ctx = ubus_connect(ubus_socket))) {
		fprintf(stderr, "Failed to connect to ubus\n");
		if ( verboseMode )
			printStatistics();
		cleanUp();
		return -1;
	}

	if ( ifd ) {
		get_ifd_status();
		ifdPrevious = ifdState;
	}

	if (( argc < 2 ) || ( !ifd )) {
		usage(progname);
		if ( verboseMode )
			printStatistics();
		cleanUp();
		return -1;
	}

	if (( hostList -> next == NULL ) && ( !use_dns )) {
		fprintf(stderr, "No addresses to check.\n");
		printf("If you started this program with only nameservers, did you start it\r\n");
		printf("before nameservers were retrieved by activating connection?\r\n\n");
		if ( verboseMode )
			printStatistics();
		cleanUp();
		return -1;
	}

	if ( verboseMode )
		printStatistics();

	nextEvent = time(NULL) + firstcheck;

	int ret;

	// Main loop
	while (1) {

		if ( time(NULL) > nextEvent ) {

			get_ifd_status();
			
			if (( ifdState == 1 ) && ( ifdPrevious == 1 )) { // Refresh DNS when needed and start a ping test

				if ( verboseMode )
					printf("State: up. Starting ping test.\r\n");

				if ( needDNSRefresh ) {
					addDNS();
					needDNSRefresh = 0;
				}

				ret = pingTest();
				if ( ret )
					failedPings = 0;
				else {
					failedPings++;
					if ( failedPings >= failsAllowed ) {
						printf("Connection lost. Restarting interface.\r\n");
						failedPings = 0;
						needDNSRefresh = 1;
						ifdPrevious = 0;
						if ( verboseMode )
							printf("Calling network.interface.wan down\r\n");
						call_ifd(0);
						sleep(3);
						if ( verboseMode )
							printf("Calling network.interface.wan up\r\n");
						call_ifd(1);
					}
				}

				nextEvent = time(NULL) + interval;
			} else if (( ifdState == 1 ) && ( ifdPrevious == 0 )) { // Communication was just brought up
				// Order a DNS refresh for next cycle
				if ( verboseMode )
					printf("State: down -> up. Commensing a DNS refresh for next cycle.\r\n");
				needDNSRefresh = 1;
				nextEvent = time(NULL) + firstcheck;
				ifdPrevious = ifdState;
			} else if (( ifdState == 0 ) && ( ifdPrevious == 0 )) { // We have lost the connection with purpose
				if ( verboseMode )
					printf("State: down. Waiting for connection.\r\n");
				nextEvent = time(NULL) + firstcheck;
			} else {
				if ( verboseMode )
					printf("State: up -> down. Connection was terminated by user/system.\r\n");
				nextEvent = time(NULL) + firstcheck; // We just lost the connection, check later if it's back up
				ifdPrevious = ifdState;
			}

		if ( verboseMode )
			printf("\r\n");
		}

	}

	cleanUp();
	return 0;
}
