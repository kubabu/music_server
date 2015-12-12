#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "controls.h"


#undef SYSTEM_MPG123_INSTALL
#define LIBPATH         "../lib/bin/"

#ifdef SYSTEM_MPG123_INSTALL
  #define PLAYER_PATH     "/usr/bin/mpg123p"
#else
  #define PLAYER_PATH     LIBPATH"mpg123p"
#endif
#define MUSIC_ROOT      "../mp3s/"
#define MUSIC_SAMPLE    "Tracy.mp3"
#define MUSIC_PATH      MUSIC_ROOT MUSIC_SAMPLE
#define COMMAND         "-C"


#define TIME_BUFLEN 26
#define IP_BUFLEN 26




#define SERV_DEF_PORT    1234

typedef struct status_t {
    int mp3_pid;
    int to_music_player[2];
    int from_music_player[2];
    int verbose;
    char tmr_buf[TIME_BUFLEN];
    char ip_buffer[26];
    int port;
    struct cln *c;
} status_t;


status_t st = {
    .mp3_pid=0,
    .port=SERV_DEF_PORT
};

void *cthread(void *arg);

struct cln {
    int cfd;
    struct sockaddr_in caddr;
};


int close_mp3()
{
    int i = -1;
    if(st.mp3_pid != 0) {
        i = kill(st.mp3_pid, SIGKILL);
        st.mp3_pid = 0;
    }
    return i;
}

char *time_printable(char timer_buffer[TIME_BUFLEN])
{
    time_t timer;
    struct tm *timer_info;

    time(&timer);
    timer_info = localtime(&timer);
    strftime(timer_buffer, 26, "%Y:%m:%d %H:%M:%S", timer_info);

    return timer_buffer;
}

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
    default:
        close_mp3();
        if(st.c){
            free(st.c);
        }
//        pthread_join(cthread, NULL);
        printf("\r[%s] Closing now \n", time_printable(st.tmr_buf));
        exit(EXIT_SUCCESS);
    }
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

void *cthread(void *arg)
{
    struct cln *c = (struct cln *)arg;
//    printf("[Accepted] Request from IP %s port %d,%s\n", inet_ntoa(c->caddr.sin_addr), c->caddr.sin_port, time_printable(st.tmr_buf) );
    char pass_buf[PASS_LENGTH], valid_pass[PASS_LENGTH];
    char cmd_buf[COMMAND_MAX_LEN];
    int i, j, connect;

    strncpy(valid_pass, "PASS", PASS_LENGTH);
    i = read(c->cfd, &pass_buf, PASS_LENGTH);

    if((i != PASS_LENGTH) || (strcmp(valid_pass, pass_buf))) {
        printf("Client authentication failed: got %s expected %s\n",
                pass_buf, valid_pass);
        goto con_end;
    }

    printf("[%s] Accepted client\n", time_printable(st.tmr_buf) );
    connect = 1;
//    write(c->cfd, "Kuba Buda 119507", 16);
     do {
        // listen for client commands
        j = 0;
        memset(cmd_buf, '\0', COMMAND_MAX_LEN);
        for(i = 0; i < COMMAND_MAX_LEN; i += j) {
            j = read(c->cfd, &cmd_buf[i], 1);
            printf("[%d] %c=%d\n", i, cmd_buf[i], cmd_buf[i]);
            switch(cmd_buf[i]) {
                case EOF:
                    connect = 0;
                case '\0':
                case '\n':
                    goto cmd_parse;
                }
        }
    cmd_parse:
        j = 0;
        //parse command
    } while(connect);
con_end:
    printf("[%s] Client disconnected \n", time_printable(st.tmr_buf) );
    close(c->cfd);
    free(c);

    return 0;
}

int play_path(char *path)
{
    int i;
    close_mp3();

    if((st.mp3_pid = fork()) == 0) {
        // launch mpg123 in separate process binded by two pipes
        dup2(st.to_music_player[0], STDIN_FILENO);
        dup2(st.from_music_player[1], STDERR_FILENO);

        printf("[CALLING]%s %s %s\n", PLAYER_PATH, COMMAND, path);
        i = execlp(PLAYER_PATH, PLAYER_PATH, COMMAND, path, NULL);
        if(i) {
            perror("Invalid calling of mp3 player");
        }
        return i;
    } else {
        if(st.verbose){
            printf("[%s] Now playing:%s\n", time_printable(st.tmr_buf),
                   path
            );
        }
        return 0;
    }
}

int play_locally(char *path)
{
    char full_path[COMMAND_MAX_LEN];
    if (access(full_path, F_OK) != -1) {
        play_path(full_path);
    } else {
        //file doesn't exist, something went wrong
    }
    return 0;
}

/* write string to slave music player through pipe */
int swrite_mplayer(char *s, int n)
{
    int i = 0;
    i = write(st.to_music_player[1], s, n);

    return i;
}

/* write string to slave music player through pipe */
int cwrite_mplayer(char c)
{
    int i = 0;
    char cmd[2] = { '\0' };
    cmd[0] = c;

    i = write(st.to_music_player[1], cmd, 1);

    return i;
}

/* command slave mp3 player */
void mplayer_pause(void)
{
    cwrite_mplayer(MPG123_PAUSE_KEY);
}

void mplayer_louder(void)
{
    cwrite_mplayer(MPG123_VOL_UP_KEY);
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
