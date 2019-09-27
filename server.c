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
#include <errno.h>
#include <sys/wait.h>

#define PORT "58044"
#define BUFFER_SIZE 1024
#define SMALL_BUFFER_SIZE 64

int n, udp, tcp;
fd_set rfds;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

void closeServer()
{
    freeaddrinfo(res);
    FD_CLR(udp, &rfds);
    close(udp);
    FD_CLR(tcp, &rfds);
    close(tcp);
    puts("Closing server");
    exit(EXIT_SUCCESS);
}

void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void waitChild() {
    pid_t pid;

    if ((pid = waitpid(-1, NULL, WNOHANG)) == -1)
        error("Error waiting for child");
}

/*Reads from the tcp socket*/
void readFromTCP(int src, char *buffer) {
    int nbytes, nleft, nread;
    char *ptr = buffer;

    nbytes = BUFFER_SIZE;
    nleft = nbytes;

    /*Makes sure the entire message is read*/
    while (nleft > 0) {
        nread = read(src, ptr, nleft);
        if (nread == -1)
            error("Error reading from tcp socket");
        else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
}

/*Writes to tcp socket*/
void replyToTCP(char *msg, int dst)
{
    int nbytes, nleft, nwritten;

    nbytes = strlen(msg);
    nleft = nbytes;

    /*Makes sure the entire message is written*/
    while (nleft > 0) {
        nwritten = write(dst, msg, nleft);
        if (nwritten <= 0)
            error("Error writing to tcp socket");
        nleft -= nwritten;
    }
}


int openTCP()
{
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

    //TODO change listen number
    if (listen(fd, 5) == -1)
        error("Error on listen");

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

    return fd;
}

/* Check if str has newLine at the end and if so, deletes it */
int deleteNewLine(char *str)
{
    char *c;
    if ((c = strchr(str, '\n')) != NULL)
        *c = '\0';

    return c != NULL;
}

/*Performs register command and saves the reply in status*/
void registerCommand(char *command, char *status)
{
    char *type = strtok(NULL, " ");

    if (strlen(type) == 6 && deleteNewLine(type))
    {
        strcpy(status, "RGR OK\n");
    }
    else
        strcpy(status, "RGR NOK\n");
}

void handleCommand(char *request, char *status)
{
    char *type;

    /* Gets command name */
    type = strtok(request, " ");

    /* Checks for command type */
    // TODO: implement commented funtions
    if (!strcmp(type, "REG"))
    {
        registerCommand(request, status);
    }
    else if (!strcmp(type, "LTP"))
    {
        //topicListCommand();
    }
    else if (!strcmp(type, "PTP"))
    {
        // topicProposeCommand(command);
    }
    else if (!strcmp(type, "LQU"))
    {
        // questionListCommand();
    }
    else if (!strcmp(type, "GQU"))
    {
        // questionGetCommand(command);
    }
    else if (!strcmp(type, "QUS"))
    {
    }
    else if (!strcmp(type, "ANS"))
    {
    }
    else
    {
        strcpy(status, "ERR\n");
    }
}

int main()
{
    struct sigaction pipe, intr, child;
    char status[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int client;
    pid_t pid;

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

    //Sets up signal handler for waiting children
    memset(&child, 0, sizeof child);
    child.sa_handler = waitChild;
    if (sigaction(SIGCHLD, &child, NULL) == -1)
        error("Error on sigaction");

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(udp, &rfds);
        FD_SET(tcp, &rfds);

        n = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (n <= 0 && errno != EINTR) {
            error("Error on select");
        }

        else if (FD_ISSET(udp, &rfds))
        {
            //Got message from udp server
            addrlen = sizeof(addr);
            n = recvfrom(udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (n == -1)
                error("Error receiving from udp socket");

            // Update client
            client = udp;

            write(1, "udp received: ", 14);
            write(1, buffer, n);

            handleCommand(buffer, status);
            n = sendto(client, status, strlen(status), 0, (struct sockaddr *)&addr, addrlen);
            if (n == -1)
                error("Error writing to udp socket");
        }

        else if (FD_ISSET(tcp, &rfds))
        {
            //Got message from tcp server
            int ret;

            if ((client = accept(tcp, (struct sockaddr *)&addr, &addrlen)) == -1)
                error("Error accepting tcp connection");

            //Creates child to handle tcp connection
            if ((pid = fork()) == -1)
                error("Error on fork");
            else if (pid == 0) {
                close(tcp);
                readFromTCP(client, buffer);
                handleCommand(buffer, status);
                replyToTCP(status, client);
                close(client);
                exit(EXIT_SUCCESS);
            }

            do {
                ret = close(client);
            } while (ret == -1 && errno == EINTR);
            if (ret == -1)
                error("Error closing client");
        }
    }

    puts("closing server, byeeee\n");
    freeaddrinfo(res);
    FD_CLR(udp, &rfds);
    close(udp);
    FD_CLR(tcp, &rfds);
    close(tcp);
    return 0;
}
