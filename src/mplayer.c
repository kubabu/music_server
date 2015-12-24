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

static char mp_cmd_mode;
static char mp_cmd;
static char mplayer_cmdbuf[MP_COMMAND_MAX_LEN];

static char stamp[TIME_BUFLEN];
static mpg123_handle *mh;

static char *music_root = MUSIC_ROOT;
/* audio output buffer */
static unsigned char *mpg_buffer;
static size_t buffer_size = 0;
static int err;
/* for libao */
static ao_device *dev;
static ao_sample_format format;
static int driver;
static size_t done;


int play_locally(char *path);

void mplayer_parse_cmd(void)
{
    if((mp_cmd_mode == '\0') && !play){
        usleep(100);
    }

    pthread_mutex_lock(&mplayer_buf_mutex);
    switch(mp_cmd_mode) {
        case '\0':
            break;
        case MPLAYER_PLAY_LOCAL:
            if(st.verbose) {
                printf("play %s\n", mplayer_cmdbuf);
            }
            play_locally(mplayer_cmdbuf);
            if(st.verbose) {
                printf("playing %s ended \n", mplayer_cmdbuf);
            }
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
    mp_cmd_mode = '\0';
    mp_cmd = '\0';
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
    /* mplayer is turned on, but not playing anything yet */
    mplayer_on = 1;
    play = 0;
    pthread_mutex_init(&mplayer_buf_mutex, NULL);
    /* initiate mpg123 */
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    /* prepare audio output buffer */
    buffer_size = mpg123_outblock(mh);
    mpg_buffer = malloc(buffer_size * sizeof(unsigned char));
    /* init libao */
    ao_initialize();

    pthread_create(&mplayer_tid, NULL, mplayer_thread, NULL);
}

void mplayer_end(void)
{
    mplayer_on = 0;
    play = 0;
    pthread_join(mplayer_tid, NULL);
    free(mpg_buffer);
    if(mh != NULL) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mh = NULL;
    }
    ao_shutdown();
}

void mplayer_load_command(char mode, char cmd, char *c, size_t n)
{
    /* */
    int count;
    pthread_mutex_lock(&mplayer_buf_mutex);
    mp_cmd_mode = mode;
    mp_cmd = cmd;
    count = n > MP_COMMAND_MAX_LEN ? MP_COMMAND_MAX_LEN : n;
    memcpy(mplayer_cmdbuf, c, count);
    pthread_mutex_unlock(&mplayer_buf_mutex);
}

int init_mp3(void)
{
    static char initiated;
    static int i = 0;

    if(!initiated) {
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

/* get path of file and play it till the end
 * This function is inspired by article of Johnny Huang
 * http://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html
 */
int _play_local(char *path)
{
    ao_sample_format format;
    int channels, encoding;
    long rate;

    play = 1;
    /* so mpg123_encsize don't use uninitialised value */
    encoding = 0;
    driver = ao_default_driver_id();

    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    while(mpg123_read(mh, mpg_buffer, buffer_size, &done) == MPG123_OK && play) {
      ao_play(dev, (char*)mpg_buffer, done);
    }
    ao_close(dev);
    play = 0;

    return err;
}


void start_playing_local(char *path)
{
    int channels, encoding;
    long rate;

    play = 1;
    /* so mpg123_encsize don't use uninitialised value */
    encoding = 0;
    driver = ao_default_driver_id();

    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);
}

int continue_play_local(void)
{
    int stat = mpg123_read(mh, mpg_buffer, buffer_size, &done);
    if((stat == MPG123_OK) && play) {
      ao_play(dev, (char*)mpg_buffer, done);
    }
    return stat == MPG123_OK ? 1 : 0;
}

void stop_play_local(void)
{
    ao_close(dev);
    play = 0;
}

void play_local(char *path)
{
    start_playing_local(path);
    while(continue_play_local());
    stop_play_local();
}

int play_locally(char *path)
{
    char fullpath[COMMAND_MAX_LEN] = {'\0'};

    memcpy(fullpath, music_root, strlen(music_root));
    memcpy(fullpath + strlen(MUSIC_ROOT), path, strlen(path));

    if(path[0] == '\0') {
        /* empty path */
        if(st.verbose) {
            printf("play_locally called with empty path");
        }
        return -1;
    }

    if (access(fullpath, F_OK) != -1) {
        printf("[%s] Now playing: %s\n", timestamp(stamp), fullpath);
        play_local(fullpath);
    } else {
        printf("%s: file not found {%s}\n", path, fullpath);
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



