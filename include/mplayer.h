#include  "utils.h"
#include  "utils.h"

pthread_t mplayer_tid;
pthread_mutex_t mplayer_mutex;

char mplayer_cmdbuf[COMMAND_MAX_LEN];

void *mplayer_thread(void *arg);

int close_mp3(void);


