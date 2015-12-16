#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"


char *timestamp(char timer_buffer[TIME_BUFLEN])
{
    time_t timer;
    struct tm *timer_info;

    time(&timer);
    timer_info = localtime(&timer);
    strftime(timer_buffer, 26, "%Y:%m:%d %H:%M:%S", timer_info);

    return timer_buffer;
}

char *get_ip(char *buff)
{
    char c;
    int n, p[2];
    pipe(p);

    if(!fork()){
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
         /* soo portable */
        system("/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'");
        close(p[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(p[1]);
        n = 0;
        while(read(p[0], &c, 1) & (n + 1 < IP_BUFLEN)) {
            if(isalnum(c) | ispunct(c)) {
                buff[n++] = c;
            }
        }
        if(n == 0) {
            n = strlen("localhost");
            strncpy(buff, "localhost", n);
        }
        close(p[0]);
        buff[n] = '\0';
    }
    return buff;
}


/* it expects unix type string - '\0' terminated  */
int dump_incoming_buffer(int cfd, int ofd, int count)
{
    int c, n;
    int printed = 0;
    char buf;

    c = 0;
    n = 0;
    buf = '\0';

    while((n = read(cfd, &buf, 1)) && buf != '\0') {
        c += n;
        if(c > count) {
            break;
        }
        if(n && !printed) {
            write(ofd, "[remote] ", 10);
            printed = n;
        }
        write(ofd, &buf, n);
    }
    buf != '\n' ? write(ofd, "\n", n) : buf;
    return c;
}


