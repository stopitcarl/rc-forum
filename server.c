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

int n, udp, tcp, newfd;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;
char buffer[BUFFER_SIZE];

void closeServer() {
    freeaddrinfo(res);
    close(newfd);
    FD_CLR(udp, &rfds);
    close(udp);
    FD_CLR(tcp, &rfds);
    close(tcp);
    puts("Closing server");
    exit(EXIT_SUCCESS);
}

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int openTCP() {
    //Sets up tcp socket
    int fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    //Sets up host information
    n = getaddrinfo(NULL, PORT, &hints, &res);
    if (n != 0)
        error("Error setting up tcp host");

    //Open socket
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
        error("Error opening tcp socket");

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        error("Error binding tcp socket");

    if (listen(fd, 5) == -1)
        error("Error on listen");

    return fd;
}

int openUDP() {
    // Setup socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    // Get hostname
    n = getaddrinfo(NULL, PORT, &hints, &res);
    if (n != 0)
        error("Error setting up udp host");

    // Open socket to filee-descriptor fd
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd == -1)
        error("Error opening udp socket");

    // Open socket to file-descriptor fd
    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
        error("Error binding udp socket");

    // ############# <Communications> #############
    // addrlen = sizeof(addr);
    // n = recvfrom(fd, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
    // if (n == -1) /*error*/
    //     exit(BAD);

    // write(1, "received: ", 10);
    // write(1, buffer, n);
    // n = sendto(fd, buffer, n, 0, (struct sockaddr *)&addr, addrlen);
    // if (n == -1) /*error*/
    //     exit(BAD);
    // ############# </Communications> #############
    return fd;
}

int main() {
    struct sigaction pipe, intr;

    udp = openUDP();
    tcp = openTCP();

    //Protects against SIGPIPE
    memset(&pipe, 0, sizeof pipe);
    pipe.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &pipe, NULL) == -1)
        error("Error on sigaction");

    //Sets up exit through SIGINT
    memset(&intr, 0, sizeof intr);
    intr.sa_handler = closeServer;
    if (sigaction(SIGINT, &intr, NULL) == -1)
        error("Error on sigacton");

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(udp, &rfds);
        FD_SET(tcp, &rfds);

        n = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (n <= 0)
            error("Error on select");

        if (FD_ISSET(udp, &rfds)) {
            //Got message from udp server
            addrlen = sizeof(addr);
            n = recvfrom(udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (n == -1)
                error("Error receiving from udp socket");

            write(1, "udp received: ", 14);
            write(1, buffer, n);
            n = sendto(udp, buffer, n, 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1)
                error("Error sending to udp socket");
        }

        if (FD_ISSET(tcp, &rfds)) {
            //Got message from tcp server
            if ((newfd = accept(tcp, (struct sockaddr *)&addr, &addrlen)) == -1)
                error("Error accepting tcp connection");

            //Read message
            n = read(newfd, buffer, BUFFER_SIZE);
            if (n == -1)
                error("Error reading from tcp socket");


            write(1, "tcp received", 14);
            write(1, buffer, n);
            n = write(newfd, buffer, n);
            if (n == -1)
                error("Error writing to tcp socket");
        }
    }

    puts("closing server, byeeee\n");
    freeaddrinfo(res);
    close(newfd);
    FD_CLR(udp, &rfds);
    close(udp);
    FD_CLR(tcp, &rfds);
    close(tcp);
    return 0;
}
