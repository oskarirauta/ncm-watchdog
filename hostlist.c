#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hostlist.h"

void initHosts(void) {
	if ( hostList == NULL ) {
		hostList = (hostItem *)malloc(sizeof(hostItem));
		hostList -> next = NULL;
	}

	if ( dnsList == NULL ) {
		dnsList = (hostItem *)malloc(sizeof(hostItem));
		dnsList -> next = NULL;
	}
}

void addHost(hostItem *list, char *newHost) {
	hostItem *cur, *new;
	cur = list;
	while ( cur -> next != NULL )
		cur = cur -> next;
	new = (hostItem *)malloc(sizeof(hostItem));
	new -> host = malloc(strlen(newHost) + 1);
	strcpy(new -> host, newHost);
	new -> next = NULL;
	cur -> next = new;
}

void clearList(hostItem *list) {
	hostItem *cur;

	while ( list -> next != NULL ) {
		cur = list -> next;
		list -> next = cur -> next;
		cur -> next = NULL;
		free( cur -> host );
		free( cur );
	}
}

void removeHosts(void) {
	hostItem *cur;
	while ( hostList -> next != NULL ) {
		cur = hostList -> next;
		free( cur -> host );
		hostList -> next = cur -> next;
		cur -> next = NULL;
		free( cur );
	}
	free( hostList );

	while ( dnsList -> next != NULL ) {
		cur = dnsList -> next;
		free( cur -> host );
		dnsList -> next = cur -> next;
		cur -> next = NULL;
		free( cur );
	}
	free( dnsList );
}
