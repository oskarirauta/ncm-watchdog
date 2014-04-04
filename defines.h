#ifndef __defines_h__
#define __defines_h__ 1

#define APPNAME "ncm-watchdog"
#define VERSION "1.0"
#define SUBVERSION "openwrt"

#define DEF_INTERVAL 180
#define FIRST_CHECK 60
#define PING_TIMEOUT 5
#define RESTART_AMOUNT 2

#define UBUS_TIMEOUT 30

#ifndef RESOLVCONF_LOCATION
#define RESOLVCONF_LOCATION "/etc/resolv.conf"
#endif

#endif
