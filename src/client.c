#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controls.h"
#include "dbg.h"
#include "utils.h"

/* This client is intended only for server testing */

int conn_fd;


//extern void dump_incoming_buffer(int cfd, int ofd);

void print_cmd_buf(char *buf, size_t n)
{
    assert(n < COMMAND_MAX_LEN);
    /* make sure string is properly terminated */
    if(n < COMMAND_MAX_LEN) {
        buf[n] = '\0';
    } else {
        buf[COMMAND_MAX_LEN - 1] = '\0';
    }
    write(1, buf, n);
    /* be sure to add new line */
    if((n > 0) && (buf[n - 1] != '\n')) {
        printf("\n");
    }
}

void conn_close(int conn_fd)
{
    char cmd[1];
    cmd[0] = EOF;
    write(conn_fd, cmd, 1);
}

void safe_exit(int sig)
{
    conn_close(conn_fd);
    close(conn_fd);
    printf("\rClosing test client...\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int fd, con, n, port;
    char cmd_buf[COMMAND_MAX_LEN];
    struct sockaddr_in serv_addr;
    struct hostent *host_addr;

    signal(SIGINT, safe_exit);
    signal(SIGQUIT, safe_exit);
    memset(cmd_buf, '\0', COMMAND_MAX_LEN);

    if(argc < 3) {
        printf("USE %s [IP_ADDR] [PORT]\n", argv[0]);
        return 1;
    }
    port = atoi(argv[2]);
    if(port < 0) {
        printf("Invalid port: %d\n", port);
        return 1;
    }
    host_addr = gethostbyname(argv[1]);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        puts("Failed to open socket");
        return 1;
    }

    serv_addr.sin_family = PF_INET;
    serv_addr.sin_port = htons(port);

    memcpy(&serv_addr.sin_addr.s_addr, host_addr->h_addr, host_addr->h_length);


    con = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(con != 0) {
        perror("Problem with connecting to server");
        return 1;
    }
    n = write(fd, "PASS", PASS_LENGTH);
    if(n != PASS_LENGTH) {
        perror("Problem with authentication to server");
    }
/*
    n = read(fd, cmd_buf, COMMAND_MAX_LEN);
    if(n < 0) {
        puts("Problem with reading from server");
        return 1;
    }
    print_cmd_buf(cmd_buf, n);

    memcpy(cmd_buf, "PASS", PASS_LENGTH);
    for(n=0; n < PASS_LENGTH; n++) {
        write(fd, cmd_buf+n, 1);
        printf("[%d] %c=%d\n", n, cmd_buf[n], cmd_buf[n]);
    }
*/
    conn_fd = fd;
//    dup2(fd, STDOUT_FILENO);
//    dup2(fd, STDOUT_FILENO);
    dump_incoming_buffer(fd, STDOUT_FILENO);

    printf("REPL mode\n> ");
    while(read(STDIN_FILENO, cmd_buf, 1)) {
        write(fd, cmd_buf, 1);
        dump_incoming_buffer(fd, STDOUT_FILENO);
    }
    conn_close(fd);
    close(fd);

    return 0;
}

