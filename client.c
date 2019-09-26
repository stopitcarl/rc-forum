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

#define BUFFER_SIZE 256

/*************************
 * VARIAVEIS GLOBAIS
 * ***********************/

char FShost[50];
char FSport[10];

socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char userID[10];
char selectedTopic[10];
char selectedQuestion[10];

/*************************
 * DECLARACOES
 * ***********************/

int openUDP();
int openTCP();
void parseArgs(int argc, char *argv[]);
void registerCommand(char *command);
void topicListCommand();
void topicSelectCommand(char *command);
void topicProposeCommand(char *command);
void questionListCommand();
void questionGetCommand(char *command);
void questionSubmitCommand(char *command);
void answerSubmitCommand(char *command);
void sendMessageUDP(char *message);
void sendMessageTCP(char *message);


/*************************
 * CODIGO
 * **********************/

int main(int argc, char *argv[]) {

    char hostName[100], command[BUFFER_SIZE];
    char *buffer;
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

    /* Main cycle */
    fgets(command, BUFFER_SIZE, stdin);
    while (strcmp(command, "exit\n")) {

        /* Gets command name */
        buffer = strtok(command, " ");

        /* Checks for command type */
        if (!strcmp(buffer, "register") || !strcmp(buffer, "reg")) {
            registerCommand(command);
        }
        else if (!strcmp(buffer, "topic_list\n") || !strcmp(buffer, "tl\n")) {
            topicListCommand();
        }
        else if (!strcmp(buffer, "topic_select") || !strcmp(buffer, "ts")) {
            topicSelectCommand(command);
        }
        else if (!strcmp(buffer, "topic_propose") || !strcmp(buffer, "tp")) {
            topicProposeCommand(command);
        }
        else if (!strcmp(buffer, "question_list\n") || !strcmp(buffer, "ql\n")) {
            questionListCommand();
        }
        else if (!strcmp(buffer, "question_get") || !strcmp(buffer, "qg")) {
            questionGetCommand(command);
        }
        else if (!strcmp(buffer, "question_submit") || !strcmp(buffer, "qs")) {

        }
        else if (!strcmp(buffer, "answer_submit") || !strcmp(buffer, "as")) {

        }
        else {
            printf("unkown command\n");
        }

        /* Waits for user input */
        fgets(command, BUFFER_SIZE, stdin);
    }

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

void registerCommand(char *command) {

    char message[256];
    char *id;
    char *c;

    id = strtok(NULL, " ");
    strcpy(message, "REG ");
    strcat(message, id);

    /* removes \n from end of string */
    c = strchr(id, '\n');
    *c = '\0';
    strcpy(userID, id);

    printf("%s", message);
    sendMessageUDP(message);

}

void topicListCommand() {

    char message[256];

    strcpy(message, "LTP\n");

    printf("%s", message);
    sendMessageUDP(message);

}

void topicSelectCommand(char *command) {

    char *topic;
    char *c;

    topic = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(topic, '\n');
    *c = '\0';
    strcpy(selectedTopic, topic);

    printf("%s\n", topic);

}

void topicProposeCommand(char *command) {

    char message[256];
    char *topic;

    topic = strtok(NULL, " ");
    strcpy(message, "PTP ");
    strcat(message, userID);
    strcat(message, " ");
    strcat(message, topic);

    printf("%s", message);
    sendMessageUDP(message);

}

void questionListCommand() {

    char message[256];

    strcpy(message, "LQU ");
    strcat(message, selectedTopic);
    strcat(message, "\n");

    printf("%s", message);
    sendMessageUDP(message);

}

void questionGetCommand(char *command) {

    char message[256];
    char *question;
    char *c;

    question = strtok(NULL, " ");
    strcpy(message, "GQU ");
    strcat(message, selectedTopic);
    strcat(message, " ");
    strcat(message, question);

    /* removes \n from end of string */
    c = strchr(question, '\n');
    *c = '\0';
    strcpy(selectedQuestion, question);

    printf("%s", message);
    sendMessageTCP(message);

}

void sendMessageUDP(char *message) {
    /* DOES NOTHING ATM */
}

void sendMessageTCP(char *message) {
    /* DOES NOTHING ATM */
}
