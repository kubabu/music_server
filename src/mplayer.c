#include <signal.h>
#include <stdio.h>
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



int init_mp3(void)
{
    static int i;

    if(!initd) {
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
    pos = 0;
    play = 0;
    mpg123_exit();
    ao_shutdown();

    return 0;
}

/* This function is inspired by article of Johnny Huang
 * http://hzqtc.github.io/2012/05/play-mp3-with-libmpg123-and-libao.html
 */
int play_local(char *path)
{
    ao_device *dev;
    ao_sample_format format;
    mpg123_handle *mh;
    unsigned char *buffer;
    int driver;
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
    mpg123_close(mh);
    mpg123_delete(mh);
/*    mpg123_exit();
    ao_shutdown(); */

    return err;
}


int play_locally(char *path)
{
    if (access(path, F_OK) != -1) {
        printf("[%s] Now playing: %s\n", timestamp(stamp), path);
        play_local(path);
    } else {
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



