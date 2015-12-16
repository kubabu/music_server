#include <ctype.h>
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

/* client_t clbuff[MAX_CLIENTS]; */

void *cthread(void *arg);

void s_safe_exit(int sig)
{
    if(sig) {
        printf("\rGot signal %d\n", sig);
    }
/*    if(pthread_join(st.c->tid, NULL)) {
        puts("Joining client thread went wrong");

    } */
    if(st.c != NULL) {
        free(st.c);
    }
    close_mp3();
    printf("\r[%s] Closing now \n", timestamp(st.tmr_buf));
    exit(EXIT_SUCCESS);
}


int ct_close(struct client_t *c)
{
    close(c->cfd);
    if(c != NULL) {
        free(c);
    }
    if(st.exit) {
        write(STDOUT_FILENO, "SERVER EXIT\n", 13);
        shutdown(st.sfd, SHUT_RDWR);
    }
    printf("[%s] Client %d disconnected \n", timestamp(st.tmr_buf), (int)pthread_self());
    pthread_exit(NULL);
}

void *cthread(void *cln)
{
    struct client_t *c = cln;

    char pass_buf[PASS_LENGTH], valid_pass[PASS_LENGTH];
    char cmd_buf[COMMAND_MAX_LEN];
    char connect;
    int i, j;

    /* set buffers */
    memset(pass_buf, '\0', PASS_LENGTH);
    memset(valid_pass, '\0', PASS_LENGTH);
    strncpy(valid_pass, "PASS\0", PASS_LENGTH);

    /* Authenticate */
    i = read(c->cfd, &pass_buf, PASS_LENGTH);

    if((memcmp(valid_pass, pass_buf, PASS_LENGTH - 1))/* || (i != PASS_LENGTH) */) {
        printf("Client authentication failed: got %s [%d] expected %s [%d]\n",
                pass_buf, i, valid_pass, PASS_LENGTH);
        for(i=0; i < PASS_LENGTH; ++i) {
            printf("[%d] %c{%d} -  %c{%d}\n", i, pass_buf[i], pass_buf[i], valid_pass[i], valid_pass[i]);
        }
        ct_close(c);
    }

    /* Log info */
    printf("[%s] Accepted request from IP %s port %d as client %d\n", timestamp(st.tmr_buf),
            inet_ntoa(c->caddr.sin_addr), c->caddr.sin_port, (int)pthread_self());
    st.exit = 0;
    connect = 1;
/*
    st.exit = 1;
    ct_close(c);

    write(c->cfd, "Client accepted", 17);
    send JSON with mp3 root
    write(c->cfd, "Kuba Buda 119507", 16);
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
                    printf("[%s] Got invalid command\n", timestamp(st.tmr_buf));           }
        }
    } while(connect && !st.exit);

    ct_close(c);
/*    kill(getpid(), SIGINT); */
    return 0;
}


int main(int argc, char *argv[])
{
    int sfd_on;
    socklen_t l;
    pthread_t *ptid;
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

    /* startup info maybe */
    printf("[%s] now on IP %s Port %d \n[%s] Ready, PID=%d\n", (argv[0] + 2),
           get_ip(st.ip_buffer), st.port, timestamp(st.tmr_buf), getpid()
    );
    st.verbose = 1;

    while(!st.exit) {
        struct client_t *c;
        c = malloc(sizeof(struct client_t));
        if(!c){
            perror("Problems with memory");
            exit(EXIT_FAILURE);
        }
        st.c = c;
        ptid = &c->tid;
        l = sizeof(c->caddr);

        if((c->cfd = accept(st.sfd, (struct sockaddr*)&c->caddr, &l)) < 0) {
            if(!st.exit) {
                perror("Problem with accepting client");
            } else {
                s_safe_exit(0);
            }
        }
        pthread_create(ptid, NULL, cthread, c);
        pthread_detach(*ptid);
    }
    s_safe_exit(0);

    return 0;
}
