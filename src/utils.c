#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"


char *time_printable(char timer_buffer[TIME_BUFLEN])
{
    time_t timer;
    struct tm *timer_info;

    time(&timer);
    timer_info = localtime(&timer);
    strftime(timer_buffer, 26, "%Y:%m:%d %H:%M:%S", timer_info);

    return timer_buffer;
}

char *get_eth0_ip(char *buff)
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
        close(p[0]);
        buff[n] = '\0';
    }
    return buff;
}

void dump_incoming_buffer(int cfd, int ofd){
    int printed = 0;
    int n = 0;
    char buf[1];
    while((n = read(cfd, buf, 1))) {
        if(!printed) {
            write(ofd, "[remote] ", 10);
            printed = 1;
        }
        write(ofd, buf, 1);
        // n = ?
    }
}


