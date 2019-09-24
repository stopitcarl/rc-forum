// Client

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*************************
 * VARIAVEIS GLOBAIS
 * ***********************/

char FShost[50];
char FSport[10];

socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

/*************************
 * DECLARACOES
 * ***********************/

int openUDP();
int openTCP();
void parseArgs(int argc, char *argv[]);

/*************************
 * CODIGO
 * **********************/

int main(int argc, char *argv[]) {   

    char hostName[100];
    int UDPfd;
    int TCPfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_flags = AI_NUMERICSERV;    

    /* Initializes default variables */
    strcpy(FSport, "58002");
    if(gethostname(hostName, 100)) {
        perror("ERROR: gethostname\n");
        exit(EXIT_FAILURE);
    }
    strcpy(FShost, hostName);

    /* Parses Args */
    parseArgs(argc, argv);

    printf("%s\n", FShost);
    printf("%s\n", FSport);

    UDPfd = openUDP();
    TCPfd = openTCP();

    freeaddrinfo(res);
    close(TCPfd);
    close(UDPfd);

    return 0;
}

void parseArgs(int argc, char *argv[]) {

    int opt;

    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
            case 'n':
                strcpy(FShost, optarg);
                hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
                printf("Current IP:%s\n", optarg);
                break;
            case 'p':
                strcpy(FSport, optarg);
                printf("Current port:%s\n", optarg);
                break;
        }
    }
}

int openUDP() {

    int fd;

    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(FShost, FSport, &hints, &res)) {
        perror("ERROR: getaddrinfo UDP\n");
        exit(EXIT_FAILURE);
    }

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("ERROR: socket\n");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int openTCP() {

    int fd;

    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(FShost, FSport, &hints, &res)) {
        perror("ERROR: getaddrinfo TCP\n");
        exit(EXIT_FAILURE);
    }

    if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("ERROR: socket\n");
        exit(EXIT_FAILURE);
    }
    
    /*
    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("ERROR: connect\n");
        exit(EXIT_FAILURE);
    }*/

    return fd;
}
