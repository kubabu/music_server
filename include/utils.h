#ifndef _utils_h
  #define _utils_h
#include <netinet/in.h>

#define SERV_DEF_PORT           1234
#define TIME_BUFLEN             26
#define IP_BUFLEN               26

#define COMMAND_MAX_LEN         512
#define IO_BUF_SIZE             512
#define CLIENT_TIMEOUT_SEC      2
#define CLIENT_CMD_TIMEOUT_SEC      7
#define CLIENT_CON_TIMEOUT_SEC      2

#define MAX_CLIENT_COUNT        5

/* Serious business */
#define PASS_LENGTH             4
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

    char verbose;
    char dos;
    char exit;

    char tmr_buf[TIME_BUFLEN];
    char ip_buffer[26];
    int port;
    int sfd;
    client_t *last_client;
    pthread_t mid;
} status_t;

status_t st;

/* Circular buffer */
typedef enum io_buf_status {BUFFER_OK, BUFFER_EMPTY, BUFFER_FULL} io_buf_status;

typedef struct io_buffer_t {
    char data[IO_BUF_SIZE];
    int ni;
    int oi;
    io_buf_status st;
} io_buffer_t;

/* avoid reading uninitialized values */
io_buffer_t *io_buf_init(io_buffer_t *buf);

io_buf_status io_buf_write(io_buffer_t *buf, char c);

io_buf_status io_buf_read(io_buffer_t *buf, char *c);

io_buf_status io_buf_peek(io_buffer_t *buf, char *c);

char ends_cmd(char c);

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

void thread_sig_capture(int sig);

void json_list_mp3s(int cfd, const char *path);

#endif
