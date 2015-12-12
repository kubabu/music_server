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



status_t st = {
    .mp3_pid=0,
    .port=SERV_DEF_PORT
};

void *cthread(void *arg);

struct cln {
    int cfd;
    struct sockaddr_in caddr;
};

void mplayer_exit(int sig)
{
    if(sig == SIGCHLD) {
        printf("[%s] Track end\n", time_printable(st.tmr_buf));
        st.mp3_pid = 0;
        /* if playlist : play next tune */
    }
}

void safe_exit(int sig)
{
    switch(sig) {
    case SIGQUIT:
    case SIGINT:
        printf("\rGot signal %d\n", sig);
    default:
        close_mp3(&st.mp3_pid);
        if(st.c){
            free(st.c);
        }
//        pthread_join(cthread, NULL);
        printf("\r[%s] Closing now \n", time_printable(st.tmr_buf));
        exit(EXIT_SUCCESS);
    }
}

void *cthread(void *arg)
{
    struct cln *c = (struct cln *)arg;
//    printf("[Accepted] Request from IP %s port %d,%s\n", inet_ntoa(c->caddr.sin_addr), c->caddr.sin_port, time_printable(st.tmr_buf) );
    char pass_buf[PASS_LENGTH], valid_pass[PASS_LENGTH];
    char cmd_buf[COMMAND_MAX_LEN];
    int i, j, connect;

    memset(pass_buf, '\0', PASS_LENGTH);
    memset(valid_pass, '\0', PASS_LENGTH);
    strncpy(valid_pass, "PASS\0", PASS_LENGTH);

    i = read(c->cfd, &pass_buf, PASS_LENGTH);

    if((memcmp(valid_pass, pass_buf, PASS_LENGTH - 1))/* || (i != PASS_LENGTH) */) {
        printf("Client authentication failed: got %s [%d] expected %s [%d]\n",
                pass_buf, i, valid_pass, PASS_LENGTH);
        for(i=0; i < PASS_LENGTH; ++i) {
            printf("[%d] %c{%d} -  %c{%d}\n", i, pass_buf[i], pass_buf[i], valid_pass[i], valid_pass[i]);
        }
        goto con_end;
    }

    time_printable(st.tmr_buf);
    printf("[%s] Accepted client\n", st.tmr_buf);
    connect = 1;
    write(c->cfd, "Client accepted\n", 17);
    // send JSON with mp3 root
//    write(c->cfd, "Kuba Buda 119507", 16);
     do {
        // listen for client commands
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
        //parse command
        switch(cmd_buf[MPLAYER_CMD_MODE]) {
            case 'e':
                if(memcmp(cmd_buf, "exit", 4) == 0) {
                    write(c->cfd, "SERVER EXIT\n", 13);
                    safe_exit(0);
                }

            default:
                if(j) {
                    printf("[%s] Got invalid command\n", time_printable(st.tmr_buf));           }
        }
    } while(connect);

con_end:
    printf("[%s] Client disconnected \n", time_printable(st.tmr_buf) );
    close(c->cfd);
    free(c);

    return 0;
}


int main(int argc, char *argv[])
{

    signal(SIGQUIT, &safe_exit);
    signal(SIGINT, &safe_exit);

    pipe(st.to_music_player);
    pipe(st.from_music_player);

    int sfd;
    pthread_t tid;
    socklen_t l;
    struct sockaddr_in myaddr, clientaddr;

    if((sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        return 1;
    }

    int sfd_on = 1;
    // set SO_REUSEADDR to enable quick rebooting server on the same port
    if(( setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sfd_on, sizeof(sfd_on))) < 0) {
        perror("Failed to set socket options");
        return 1;
    }

    myaddr.sin_family = PF_INET;
    myaddr.sin_port = htons(st.port);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) != 0) {
        perror("Problem with binding socket");
        return 1;
    }

    listen(sfd, 5);

    // startup info maybe
    printf("[%s] now on IP %s Port %d \nPID=%d %s\n", (argv[0] + 2),
           get_eth0_ip(st.ip_buffer), st.port, getpid(),
           time_printable(st.tmr_buf)
    );

    signal(SIGCHLD, &mplayer_exit);
    st.verbose = 1;
//    play_path(MUSIC_PATH);

    while(1) {

        struct cln *c = malloc(sizeof(struct cln));
        if(!c){
            perror("Problems with memory");
            exit(EXIT_FAILURE);
        }
        st.c = c;
        l = sizeof(c->caddr);
        if((c->cfd = accept(sfd, (struct sockaddr*)&clientaddr, &l)) < 0) {
            perror("Problem with accepting client");
        }
        pthread_create(&tid, NULL, cthread, c);
        pthread_detach(tid);

    }

    return 0;
}
