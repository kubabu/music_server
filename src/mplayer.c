#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include  "ao/ao.h"
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

struct mpeg3_handle{
};

struct mpeg3_handle mph;

char stamp[TIME_BUFLEN];

int init_mp3(void)
{

    return 0;
}

int close_mp3(void)
{

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
    char initialized;
    unsigned char *buffer;
    int driver;
    int err;
    int channels, encoding;
    long rate;
    size_t buffer_size;
    size_t done;


    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = malloc(buffer_size * sizeof(unsigned char));
    initialized = 1;

    mpg123_open(mh, path);
    mpg123_getformat(mh, &rate, &channels, &encoding);

    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);

    while(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK) {
        ao_play(dev, (char*)buffer, done);
    }
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();

    return err;
}


int _play_path(char *path, status_t *st)
{
    int i;
    close_mp3();

    ao_initialize();
    mpg123_init();

    if((st->mp3_pid = fork()) == 0) {
        // launch mpg123 in separate process binded by two pipes
        dup2(st->to_music_player[0], STDIN_FILENO);
        dup2(st->from_music_player[1], STDERR_FILENO);

        printf("[CALLING]%s %s %s\n", PLAYER_PATH, COMMAND, path);
        i = execlp(PLAYER_PATH, PLAYER_PATH, COMMAND, path, NULL);
        if(i) {
            perror("Invalid calling of mp3 player");
        }
        return i;
    } else {
        if(st->verbose){
            printf("[%s] Now playing:%s\n", timestamp(st->tmr_buf),
                   path
            );
        }
        return 0;
    }
}

int play_locally(char *path)
{
    if (access(path, F_OK) != -1) {
        printf("[%s] Now playing: %s\n", timestamp(stamp), path);
        play_local(path);
    } else {
        //file doesn't exist, something went wrong
        return -1;
    }
    return 0;
}


/* command slave mp3 player */
void mplayer_pause(void)
{

}

void mplayer_louder(void)
{

}



