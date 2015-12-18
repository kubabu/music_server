#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "controls.h"
#include "mplayer.h"
#include "utils.h"



status_t st;
client_t *clbuf[MAX_CLIENT_COUNT];
pthread_mutex_t mx;


void *client_thread(void *arg);

int client_close(client_t *c)
{
    if(c != NULL) {
        printf("[%s] Client %d {%d} disconnectng \n", timestamp(st.tmr_buf),
           c->cid, (int)pthread_self());
        close(c->cfd);
        clbuf[c->cid] = NULL;
        free(c);
    }

    return 0;
}

void ct_close(client_t *c)
{
    int n = c->cid;
    if(st.exit) {
        shutdown(st.sfd, SHUT_RDWR);
    }
    client_close(c);
    printf("clbuf[%d] = %p\n", n, (void *)clbuf[n]);
    pthread_exit(NULL);
}


void s_safe_exit(int sig)
{
    int i, me;
    static int re;

    pthread_mutex_lock(&mx);
    st.exit = 1;
    if(re) {
        return;
    }
    re = 1;
    me = -1;
    if(sig) {
        printf("\rGot signal %d\n", sig);
    }
    shutdown(st.sfd, SHUT_RDWR);
    for(i = 0; i < MAX_CLIENT_COUNT; ++i){
        if(clbuf[i] != NULL) {
            if(clbuf[i]->tid == pthread_self()) {
                me = i;
            } else {
                pthread_join(clbuf[i]->tid, NULL);
                client_close(clbuf[i]);
            }
        }
    }
    if(st.c != NULL) {
        free(st.c);
    }
    close_mp3();
    printf("\r[%s] Closing now \n", timestamp(st.tmr_buf));
    if(me != -1) {
        pthread_exit(NULL);
    }

    exit(EXIT_SUCCESS);
}

char ends_cmd(char c) {
    switch(c) {
        case '%':
        case EOF:
            return -1;
        case '\0':
        case '\r':
        case '\n':
            return 1;
    }
    return 0;
}

void *client_thread(void *cln)
{
    client_t *c = cln;

    char pass_buf[PASS_LENGTH];
    char cmd_buf[COMMAND_MAX_LEN];
    char cb, connect;
    int i, j;

    /* set buffers */
    memset(pass_buf, '\0', PASS_LENGTH);

    /* Authenticate */
    i = read(c->cfd, &pass_buf, PASS_LENGTH);
    if(!conf_pswd(pass_buf, i)) {
        printf("[%s] Request from IP %s port %d [client %d {%d}] failed to authenticate\n",
               timestamp(st.tmr_buf), inet_ntoa(c->caddr.sin_addr),
               c->caddr.sin_port, c->cid, (int)pthread_self());
        ct_close(c);
    }

    /* Log info */
    printf("[%s] Accepted request from IP %s port %d as client %d {%d}\n", timestamp(st.tmr_buf),
            inet_ntoa(c->caddr.sin_addr), c->caddr.sin_port, c->cid, (int)pthread_self());
    st.exit = 0;
    connect = 1;

    write(c->cfd, "Client accepted", 17);
    /*
    send JSON with mp3 root
    */
     while(connect && !st.exit) {
        /* listen for client commands */
        i = 0;
        j = 0;
        cb = ' ';
        memset(cmd_buf, '\0', COMMAND_MAX_LEN);

        while(!ends_cmd(cb) && i < COMMAND_MAX_LEN && connect) {
            j = read(c->cfd, &cb, 1);
            if(j == 0 || (ends_cmd(cb) < 0)) {
                connect = 0;
                break;
            }
            cmd_buf[i] = cb;
            printf("[%d] %c=%d\n", 0, cmd_buf[i], cmd_buf[i]);
            i += j;
        }
        cmd_buf[i] = '\0';
        /* parse command */
        switch(cmd_buf[MPLAYER_CMD_MODE]) {
            case 'e':
                if(memcmp(cmd_buf, "exit", 4) == 0) {
                    /* write(c->cfd, "SERVER EXIT\n", 13); */
                    connect = 0;
                    st.exit = 1;
                    ct_close(c);
                }

            default:
                if(j && st.verbose) {
                    printf("[%s] Client %d {%d}: invalid command\n",
                        timestamp(st.tmr_buf), c->cid, (int)pthread_self());
                }
        }
    }

    ct_close(c);

     return 0;
}

int getffi(status_t s, client_t **cbuf) {
    int n = getcid(cbuf, MAX_CLIENT_COUNT);

    while(n < 0) {
        n = getcid(cbuf, MAX_CLIENT_COUNT);
        /* too much clients */
        sleep(1); /* veery fine but works */
        if(!s.dos) {
            printf("[%s] DOS - to much clients, server over capacity\n", timestamp(st.tmr_buf));
            s.dos = 1;
        }
    }
    if(s.dos) {
        printf("[%s] End of DOS - clients accepted back again\n", timestamp(st.tmr_buf));
    }
    st.dos = 0;

    printf("getffi return %d\n", n);
    return n;
}

int main(int argc, char *argv[])
{
    int ffi, sfd_on;
    socklen_t l;
    pthread_t *ptid;
    struct sockaddr_in myaddr;

    signal(SIGQUIT, &s_safe_exit);
    signal(SIGINT, &s_safe_exit);
    pthread_mutex_init(&mx, NULL);

    st.port = SERV_DEF_PORT;
    if(argc > 1) {
        st.port = atoi(argv[1]);
    }

    if((st.sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        return 1;
    }

    sfd_on = 1;
    /* SO_REUSEADDR to enable quick rebooting server on the same port */
    if(( setsockopt(st.sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sfd_on, sizeof(sfd_on))) < 0) {
        perror("Failed to set socket options");
        return 1;
    }

    myaddr.sin_family = PF_INET;
    myaddr.sin_port = htons(st.port);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(st.sfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) != 0) {
        perror("Problem with binding socket");
        return 1;
    }

    listen(st.sfd, 5);

    /* display info on startup */
    printf("[%s] now on IP %s Port %d \n[%s] Ready, PID=%d, [%ld]\n", (argv[0] + 2),
           get_ip(st.ip_buffer), st.port, timestamp(st.tmr_buf), getpid(), sizeof(client_t)
    );
    st.verbose = 1;

    while(!st.exit) {
        ffi = getffi(st, clbuf);
        st.c = malloc(sizeof(client_t));
        /*c = clbuf[ffi]; */
        if(st.c == NULL){
            perror("Problems with memory");
            s_safe_exit(EXIT_FAILURE);
        }
        st.c->cid = ffi;
        ptid = &st.c->tid;
        l = sizeof(st.c->caddr);
        puts("Accepting");
        if((st.c->cfd = accept(st.sfd, (struct sockaddr*)&st.c->caddr, &l)) < 0) {
            if(!st.exit) {
                perror("Problem with accepting client");
            } else {
                s_safe_exit(0);
            }
        } else {
            puts("Accepted");
            pthread_mutex_lock(&mx);
            clbuf[ffi] = st.c;
            pthread_create(ptid, NULL, client_thread, st.c);
            pthread_detach(*ptid);
            pthread_mutex_unlock(&mx);
        }
        st.c = NULL;
    }
    s_safe_exit(0);

    return 0;
}
