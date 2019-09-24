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

char FSIP[10];
char FSport[10];

int fd;
ssize_t n;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

/*************************
 * DECLARACOES
 * ***********************/

int creatUDPclient();
int creatUTCclient();
void parseArgs(int argc, char *argv[]);

/*************************
 * CODIGO
 * **********************/

int main(int argc, char *argv[]) {   

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    /* Parses Args */
    strcmp(FSport, "58002");
    strcmp()
    
    parseArgs(argc, argv);

    return 0;
}

void parseArgs(int argc, char *argv[]) {

    int opt;

    while ((opt = getopt(argc, argv, "a:b:")) != -1) {
        switch (opt) {
            case 'n':
                if (!strcmp(optarg, "")) {
                    printf("missing argument");
                    exit(1);
                }
                strcpy(FSIP, optarg);
                break;
            case 'p':
                if (!strcmp(optarg, "")) {
                    printf("missing argument");
                    exit(1);
                }
                strcpy(FSport, optarg);
                break;
            default:
                printf("unkown option\n");
                exit(1);
        }
    }
}
