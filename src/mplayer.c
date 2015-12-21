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


#define BITS            8


static char initd;
static char play;
static off_t pos;
char stamp[TIME_BUFLEN];
mpg123_handle *mh;

char *music_root = MUSIC_ROOT;


void *mplayer_thread(void *arg)
{
    char *cmd_buf = (char *)arg;

    signal(SIGQUIT, &thread_sig_capture);
    signal(SIGINT, &thread_sig_capture);

    pthread_mutex_init(&mplayer_mutex, NULL);
    while((memcmp(cmd_buf, "exit", 4) != 0  && !st.exit)) {
        /*  */
        usleep(100);
    }
    puts("Mplayer thread leaving now...");
    return NULL;
}

int init_mp3(void)
{
    static int i = 0;

    if(!initd) {
        pthread_mutex_init(&mplayer_mutex, NULL);
        ao_initialize();
        i = ao_default_driver_id();
        mpg123_init();
    }
    initd = 1;
    play = 1;
    pos = 1;

    return i;
}

int close_mp3(void)
{
    pthread_mutex_lock(&mplayer_mutex);
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



