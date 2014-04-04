#ifndef __hostlist_h__
#define __hostlist_h__ 1

struct _hostItem {
	char *host;
	struct _hostItem *next;
};

typedef struct _hostItem hostItem;

hostItem *hostList;
hostItem *dnsList;

void initHosts(void);
void addHost(hostItem *list, char *newHost);
void clearList(hostItem *list);
void removeHosts(void);

#endif
