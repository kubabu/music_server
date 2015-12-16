#ifndef _utils_h
  #define _utils_h
#include <netinet/in.h>

#define SERV_DEF_PORT           1234
#define TIME_BUFLEN             26
#define IP_BUFLEN               26

#define COMMAND_MAX_LEN         512
#define IO_BUF_SIZE             512
#define CLIENT_TIMEOUT_SEC      2

#define MAX_CLIENT_COUNT        5

/* Serious business */
#define PASS_LENGTH             5
#define PASS 'root'

/* aggregate parameters for client thread */
typedef struct client_t {
    int cfd;
    int cid;
    struct sockaddr_in caddr;
    pthread_t tid;
} client_t;

/* Global structure with configuration */
typedef struct status_t {
    int from_music_player[2];
    char *l_s_root;

    char verbose;
    char dos;
    char exit;

    char tmr_buf[TIME_BUFLEN];
    char ip_buffer[26];
    int port;
    int sfd;
    client_t *c;
} status_t;

/* Circular buffer */
typedef struct io_buffer {
    char data[IO_BUF_SIZE];
    int ni;
    int oi;
} io_buffer;

enum BufferStatus {BUFFER_OK, BUFFER_EMPTY, BUFFER_FULL};


/* low resolution [1 s] timeout detection */
char timeout(time_t *t, int lim);

void timeout_update(time_t *t);

/* printable timestamp */
char *timestamp(char timer_buffer[TIME_BUFLEN]);

/* printable IP */
char *get_ip(char *buff);

/* check if client authenticated properly */
char conf_pswd(char *pass, int c);

/* get client ID - first free index of pointers array */
int getcid(client_t **buf, int max);

int printcl(client_t *cl, int i);

#endif
