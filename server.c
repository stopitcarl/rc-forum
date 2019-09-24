// Server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>

#define PORT "58000"
#define BUFFER_SIZE 128
#define MAXNAME 80

int fd, n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[BUFFER_SIZE];

int openTCP()
{
    int fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    n = getaddrinfo(NULL, PORT, &hints, &res);
    if (n != 0) /*error*/
        exit(1);

    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) /*error*/
        exit(2);

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(3);

    if (listen(fd, 5) == -1) /*error*/
        exit(4);

    puts("openTCP");
    return fd;
}

int openUDP()
{
    // Setup socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    // Get hostname
    n = getaddrinfo(NULL, PORT, &hints, &res);
    if (n != 0) /*error*/
        exit(1);

    // Open socket to filee-descriptor fd
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1) /*error*/
        exit(1);

    // Open socket to file-descriptor fd
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);

    // ############# <Communications> #############
    // addrlen = sizeof(addr);
    // n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
    // if (n == -1) /*error*/
    //     exit(1);

    // write(1, "received: ", 10);
    // write(1, buffer, n);
    // n = sendto(fd, buffer, n, 0, (struct sockaddr *)&addr, addrlen);
    // if (n == -1) /*error*/
    //     exit(1);
    // ############# </Communications> #############
    puts("openUDP");
    return fd;
}

int main()
{
    //int tcp = openTCP();
    int udp = openUDP();
    int tcp = openTCP();
    int newfd;
    fd_set rfds;
    struct sigaction act;

    memset(&act, 0, sizeof act);
    act.sa_handler = SIG_IGN;

    if (sigaction(SIGPIPE, &act, NULL) == -1)/*error*/
        exit(1);

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(udp, &rfds);
        FD_SET(tcp, &rfds);

        n = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (n <= 0) /*error*/
            exit(1);

        if (FD_ISSET(udp, &rfds)) {
            addrlen = sizeof(addr);
            n = recvfrom(udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (n == -1) /*error*/
                exit(1);

            write(1, "received: ", 10);
            write(1, buffer, n);
            n = sendto(udp, buffer, n, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1) /*error*/
                exit(1);
            break;
        }

        if (FD_ISSET(tcp, &rfds)) {
            if ((newfd = accept(tcp, (struct sockaddr *)&addr, &addrlen)) == -1) /*error*/
                exit(1);

            write(1, "received", 8);
            n = write(newfd, "hey", 3);
            if (n == -1)/*error*/
                exit(1);
            break;
        }
    }

    freeaddrinfo(res);
    close(newfd);
    close(udp);
    FD_CLR(udp, &rfds);
    close(tcp);
    FD_CLR(tcp, &rfds);
    return 0;
}
