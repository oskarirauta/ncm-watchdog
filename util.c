#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

void strip(char *s) {
    char *p = s;
    int n;
    while (*s) {
        n = strcspn(s, STRIPCHARS);
        strncpy(p, s, n);
        p += n;
        s += n + strspn(s+n, STRIPCHARS);
    }
    *p = 0;
}

char *strip_copy(const char *s) {
    char *buf = malloc(1 + strlen(s));
    if (buf) {
        char *p = buf;
        char const *q;
        int n;
        for (q = s; *q; q += n + strspn(q+n, STRIPCHARS)) {
            n = strcspn(q, STRIPCHARS);
            strncpy(p, q, n);
            p += n;
        }
        *p++ = '\0';
        buf = realloc(buf, p - buf);
    }
    return buf;
}
