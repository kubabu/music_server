#ifndef _utils_h
  #define _utils_h

#define SERV_DEF_PORT    1234
#define TIME_BUFLEN 26
#define IP_BUFLEN 26

#define COMMAND_MAX_LEN 512

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

char *time_printable(char timer_buffer[TIME_BUFLEN]);

#endif
