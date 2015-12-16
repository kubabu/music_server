#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"



/* low resolution timeout counter */
char timeout(time_t *t, int lim)
{
    if(*t + lim > time(NULL)) {
        return 0;
    }
    return 1;
}

void timeout_update(time_t *t)
{
    time(t);
}


/* printable timestamp */
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


char conf_pswd(char *pass, int count)
{
    char valid_pass[PASS_LENGTH];

    memset(valid_pass, '\0', PASS_LENGTH);
    strncpy(valid_pass, "PASS\0", PASS_LENGTH);

    if((memcmp(valid_pass, pass, PASS_LENGTH - 1)) || (count != PASS_LENGTH) ) {
#ifdef DEBUG
        int i;
        for(i=0; i < PASS_LENGTH; ++i) {
            printf("[%d] %c{%d} -  %c{%d}\n", i, pass[i], pass[i],
                                        valid_pass[i], valid_pass[i]);
        }
#endif
        printf("Client authentication failed: got %s [%d] expected %s [%d]\n",
                pass, count, valid_pass, PASS_LENGTH);
        return 0;
    }
    return 1;
}

/* get first free index of pointers array */
int getcid(client_t **buf, int max)
{
    int i;
    i = 0;
    for(i = 0; i < max; ++i) {
        if(buf[i] == NULL) {
            return i;
        }
    }
    return -1;
}

int printcl(client_t *cl, int i)
{
    if(cl == NULL) {
        printf("Cl %d empty \n", i);
        return -1;
    }
    printf("___\n|cfd=%d\n|cid=%d\n|tid=%d\n", cl->cfd, cl->cid, (int)cl->tid);
    return cl->cid;
}

