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


void *client_thread(void *arg);

int client_close(client_t *c)
{
    if(c != NULL) {
        close(c->cfd);
        clbuf[c->cid] = NULL;
        free(c);
    }

    return 0;
}

void ct_close(client_t *c)
{
    printf("[%s] Client %d {%d} disconnectng \n", timestamp(st.tmr_buf),
           c->cid, (int)pthread_self());
    client_close(c);
    if(st.exit) {
        shutdown(st.sfd, SHUT_RDWR);
    }
    pthread_exit(NULL);
}


void s_safe_exit(int sig)
{
    int i;

    if(sig) {
        printf("\rGot signal %d\n", sig);
    }
    st.exit = 1;
    shutdown(st.sfd, SHUT_RDWR);
    for(i = 0; i < MAX_CLIENT_COUNT; ++i){
        if(clbuf[i] != NULL) {
            pthread_cancel(clbuf[i]->tid);
            client_close(clbuf[i]);
        }
    }
    if(st.c != NULL) {
        free(st.c);
    }
    close_mp3();
    printf("\r[%s] Closing now \n", timestamp(st.tmr_buf));
    exit(EXIT_SUCCESS);
}


void *client_thread(void *cln)
{
    client_t *c = cln;

    char pass_buf[PASS_LENGTH];
    char cmd_buf[COMMAND_MAX_LEN];
    char connect;
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
     do {
        /* listen for client commands */
        j = 0;
        memset(cmd_buf, '\0', COMMAND_MAX_LEN);
        for(i = 0; i < COMMAND_MAX_LEN; i += j) {
            j = read(c->cfd, &cmd_buf[i], 1);

            switch(i){
            case 0:
                break;
            case 1:
                printf("[%d] %c=%d\n", 0, cmd_buf[0], cmd_buf[0]);
            default:
                printf("[%d] %c=%d\n", i, cmd_buf[i], cmd_buf[i]);
                break;
            }
            switch(cmd_buf[i]) {
                case EOF:
                    connect = 0;
                case '\0':
                case '\r':
                case '\n':
                    goto cmd_parse;
                }
        }
    cmd_parse:
        /* parse command */
        switch(cmd_buf[MPLAYER_CMD_MODE]) {
            case 'e':
                if(memcmp(cmd_buf, "exit", 4) == 0) {
                    /* write(c->cfd, "SERVER EXIT\n", 13); */
                    st.exit = 1;
                    ct_close(c);
                }

            default:
                if(j) {
                    printf("[%s] Client %d {%d}: invalid command\n", timestamp(st.tmr_buf),
                            c->cid, (int)pthread_self());
                }
        }
    } while(connect && !st.exit);

    ct_close(c);

     return 0;
}


int main(int argc, char *argv[])
{
    int ffi, sfd_on;
    socklen_t l;
    pthread_t *ptid;
    client_t *c;
    struct sockaddr_in myaddr;

    signal(SIGQUIT, &s_safe_exit);
    signal(SIGINT, &s_safe_exit);

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
    printf("[%s] now on IP %s Port %d \n[%s] Ready, PID=%d\n", (argv[0] + 2),
           get_ip(st.ip_buffer), st.port, timestamp(st.tmr_buf), getpid()
    );
    st.verbose = 1;

    while(!st.exit) {
        while((ffi = getcid(clbuf, MAX_CLIENT_COUNT)) < 0) {
            /* too much clients */
            sleep(1); /* veery fine but works */
            if(!st.dos) {
                printf("[%s] DOS - to much clients, server over capacity\n", timestamp(st.tmr_buf));
                st.dos = 1;
            }
        }
        if(st.dos) {
            printf("[%s] End of DOS - clients accepted back again\n", timestamp(st.tmr_buf));
        }
        st.dos = 0;

        c = malloc(sizeof(client_t));
        st.c = c;
        /*c = clbuf[ffi]; */
        if(c == NULL){
            perror("Problems with memory");
            s_safe_exit(EXIT_FAILURE);
        }
        c->cid = ffi;
        ptid = &c->tid;
        l = sizeof(c->caddr);

        if((c->cfd = accept(st.sfd, (struct sockaddr*)&c->caddr, &l)) < 0) {
            if(!st.exit) {
                perror("Problem with accepting client");
            } else {
                s_safe_exit(0);
            }
        } else {
            clbuf[ffi] = c;
            pthread_create(ptid, NULL, client_thread, clbuf[ffi]);
            pthread_detach(*ptid);
        }
    }
    s_safe_exit(0);

    return 0;
}
