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


#undef SYSTEM_MPG123_INSTALL
#define LIBPATH         "../lib/bin/"

#ifdef SYSTEM_MPG123_INSTALL
  #define PLAYER_PATH     "/usr/bin/mpg123"
#else
  #define PLAYER_PATH     LIBPATH"mpg123"
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
    char timer_buffer[TIME_BUFLEN];
    char ip_buffer[26];
    int port;
    struct cln *c;
} status_t;


status_t status = {
    .mp3_pid=0,
    .port=SERV_DEF_PORT
};


struct cln {
    int cfd;
    struct sockaddr_in caddr;
};


int close_mp3()
{
    int i = -1;
    if(status.mp3_pid != 0) {
        i = kill(status.mp3_pid, SIGKILL);
        status.mp3_pid = 0;
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

void safe_exit(int sig)
{
    switch(sig) {
    case SIGQUIT:
    case SIGINT:
    default:
        close_mp3();
        if(status.c){
            free(status.c);
        }
        printf("\r[%s] Closing now \n", time_printable(status.timer_buffer));
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
    printf("[Accepted] Request from IP %s port %d, %s\n",
        inet_ntoa(c->caddr.sin_addr), c->caddr.sin_port,
        time_printable(status.timer_buffer)
    );
    write(c->cfd, "Kuba Buda 119507", 16);

    close(c->cfd);
    free(c);

    return 0;
}

int play_path(char *path)
{
    int i;
    close_mp3();

    if((status.mp3_pid = fork()) == 0) {
        // launch mpg123 in separate process binded by two pipes
        dup2(status.to_music_player[0], STDIN_FILENO);
        dup2(status.from_music_player[1], STDERR_FILENO);

        printf("[CALLING]%s %s %s\n", PLAYER_PATH, COMMAND, path);
        i = execlp(PLAYER_PATH, PLAYER_PATH, COMMAND, path, NULL);
        if(i) {
            perror("Invalid calling of mp3 player");
        }
        return i;
    } else {
        if(status.verbose){
            printf("[%s] Now playing:%s\n", time_printable(status.timer_buffer),
                   path
            );
        }
        return 0;
    }
}

int main(int argc, char *argv[])
{
    signal(SIGQUIT, &safe_exit);
    signal(SIGINT, &safe_exit);

    pipe(status.to_music_player);
    pipe(status.from_music_player);

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
    myaddr.sin_port = htons(status.port);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sfd, (struct sockaddr*)&myaddr, sizeof(myaddr)) != 0) {
        perror("Problem with binding socket");
        return 1;
    }

    listen(sfd, 5);

    // startup info maybe
    printf("[%s] now on IP %s Port %d \nPID=%d %s\n", (argv[0] + 2),
           get_eth0_ip(status.ip_buffer), status.port, getpid(),
           time_printable(status.timer_buffer)
    );

    status.verbose = 1;
    play_path(MUSIC_PATH);
    while(1) {

        struct cln *c = malloc(sizeof(struct cln));
        if(!c){
            perror("Problems with memory");
            exit(EXIT_FAILURE);
        }
        status.c = c;
        l = sizeof(c->caddr);
        if((c->cfd = accept(sfd, (struct sockaddr*)&clientaddr, &l)) < 0) {
            perror("Problem with accepting client");
        }
        pthread_create(&tid, NULL, cthread, c);
        pthread_detach(tid);
    }
    close(sfd);

    write(status.to_music_player[1], "q", 1);
    if(kill(status.mp3_pid, SIGKILL)) {
        perror("Closing mp3 player");
    }
    return 0;
}
