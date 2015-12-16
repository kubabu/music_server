#ifndef _utils_h
  #define _utils_h
#include <netinet/in.h>

#define SERV_DEF_PORT    1234
#define TIME_BUFLEN 26
#define IP_BUFLEN 26

#define COMMAND_MAX_LEN 512

#define MAX_CLIENTS 512

/* Global structure with configuration */
typedef struct status_t {
    int from_music_player[2];
    char *l_s_root;

    char verbose;
    char exit;

    char tmr_buf[TIME_BUFLEN];
    char ip_buffer[26];
    int port;
    struct client_t *c;
} status_t;


struct client_t {
    int cfd;
    struct sockaddr_in caddr;
    pthread_t tid;
};

char *timestamp(char timer_buffer[TIME_BUFLEN]);

char *get_ip(char *buff);

int dump_incoming_buffer(int cfd, int ofd, int count);

#endif
