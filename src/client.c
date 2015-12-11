#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controls.h"
/* This client is intended only for testing server functionalities
 */

int main(int argc, char *argv[])
{
    int fd, con, n, port;
    char buf[128];
    struct sockaddr_in serv_addr;
    struct hostent *host_addr;

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
    n = write(fd, PASS, PASS_LENGTH);

    n = read(fd, buf, 128);
    if(n < 0) {
        puts("Problem with reading from server");
        return 1;
    }

    buf[n] = '\0';
    write(1, buf, n);
    if(buf[n - 1] != '\n') {
        printf("\n");
    }

    close(fd);


    return 0;
}

