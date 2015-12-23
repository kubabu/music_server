#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include  "ao/ao.h"
#include  "curl/curl.h"
#include  "mpg123.h"

#include  "controls.h"
#include  "mplayer.h"
#include  "utils.h"


#define LIBPATH         "../lib/bin/"
#define PLAYER_PATH     LIBPATH"mpg123p"
#define MUSIC_ROOT      "../mp3s/"
#define MUSIC_SAMPLE    "Tracy.mp3"
#define MUSIC_PATH      MUSIC_ROOT MUSIC_SAMPLE
#define COMMAND         "-C"


#define BITS                8
#define MP_COMMAND_MAX_LEN  512

static char play;
static char mplayer_on;
static off_t pos;

static pthread_t mplayer_tid;
static pthread_mutex_t mplayer_buf_mutex;

static char mplayer_cmdbuf[MP_COMMAND_MAX_LEN];

char stamp[TIME_BUFLEN];
static mpg123_handle *mh;

char *music_root = MUSIC_ROOT;


int play_locally(char *path);

void mplayer_parse_cmd(void)
{
    pthread_mutex_lock(&mplayer_buf_mutex);
    switch(mplayer_cmdbuf[MPLAYER_CMD_TYPE]) {
        case '\0':
            if(!play){
                usleep(100);
            } else {
                /* continue playing */
            }
/*                write(c->cfd, "\0", 1);  * pingback */
            break;
        case MPLAYER_PLAY_LOCAL:
            if(st.verbose) {
                printf("play %s\n", mplayer_cmdbuf + 1);
            }
            play_locally(mplayer_cmdbuf + 1);
            break;
        case MPLAYER_SET_PAUSE:
            if(st.verbose) {
                printf("pause\n");
            }
            break;
        case MPLAYER_SET_STOP:
            if(st.verbose) {
                printf("stop\n");
            }
            break;
        case MPLAYER_SET_VOL_UP:
            if(st.verbose) {
                printf("vol up\n");
            }
            break;

        default:
            if(st.verbose) {
                printf("Unsupported or invalid command\n");
            }
            break;
    }
    memset(mplayer_cmdbuf, '\0', MP_COMMAND_MAX_LEN);
    pthread_mutex_unlock(&mplayer_buf_mutex);
}

void *mplayer_thread(void *arg)
{
    static char mp_thr_on = 0;
    if(arg != NULL && arg != &mplayer_cmdbuf) {
        puts("Mplayer initiated without module builtin command buffer.\
                \n\rThis option is not supported for now");
    }
    /* launch new thread with music player */
    if(mp_thr_on++) {
        pthread_exit(NULL); /* there is to be only one */
    }

    signal(SIGQUIT, &thread_sig_capture);
    signal(SIGINT, &thread_sig_capture);

    /* parse commands and execute them */
    while(mplayer_on  && !st.exit) {
        /* usleep(100); */
        mplayer_parse_cmd();
    }
    return NULL;
}

void mplayer_init(void)
{
    static char initiated;
    if(initiated++) {
        return; /* Already initiated */
    }
    mplayer_on = 1;
    pthread_mutex_init(&mplayer_buf_mutex, NULL);
    play = 0;
    pthread_create(&mplayer_tid, NULL, mplayer_thread, NULL);
}

void mplayer_end(void)
{
    mplayer_on = 0;

    pthread_join(mplayer_tid, NULL);

}

void mplayer_load_command(char *c, size_t n)
{
    /* */
    int count;
    count = n > MP_COMMAND_MAX_LEN ? MP_COMMAND_MAX_LEN : n;
    pthread_mutex_lock(&mplayer_buf_mutex);
    memcpy(mplayer_cmdbuf, c, count);
    pthread_mutex_unlock(&mplayer_buf_mutex);
}

int init_mp3(void)
{
    static char initiated;
    static int i = 0;

    if(!initiated) {
        pthread_mutex_init(&mplayer_buf_mutex, NULL);
        ao_initialize();
        i = ao_default_driver_id();
        mpg123_init();
    }
    initiated = 1;
    play = 1;
    pos = 1;

    return i;
}

int close_mp3(void)
{
/*    pthread_mutex_lock(&mplayer_buf_mutex); */
    pos = 0;
    play = 0;
    if(mh != NULL) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mh = NULL;
    }

    mpg123_exit();
    ao_shutdown();
/*    pthread_mutex_unlock(&mplayer_mutex); */

    return 0;
}

/* This function is inspired by article of Johnny Huang
 * http://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html
 */
int play_local(char *path)
{
    int driver;
    ao_device *dev;
    ao_sample_format format;
    unsigned char *buffer;
    int err;
    int channels, encoding;
    long rate;
    size_t buffer_size;
    size_t done;

    driver = init_mp3();

    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = malloc(buffer_size * sizeof(unsigned char));

    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    while(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK && play) {
      ao_play(dev, (char*)buffer, done);
    }
    free(buffer);
    ao_close(dev);
    mpg123_exit();
    ao_shutdown();

    return err;
}


int play_locally(char *path)
{
    char fullpath[COMMAND_MAX_LEN] = {'\0'};

    memcpy(fullpath, music_root, strlen(music_root));
    memcpy(fullpath + strlen(MUSIC_ROOT), path, strlen(path));

    if (access(fullpath, F_OK) != -1) {
        printf("[%s] Now playing: %s\n", timestamp(stamp), fullpath);
        play_local(fullpath);
    } else {
        printf("%s: file not found\n", path);
        /* file doesn't exist, something went wrong */
        return -1;
    }
    return 0;
}


void mplayer_pause(void)
{

}

void mplayer_louder(void)
{

}



