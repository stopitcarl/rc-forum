// Client

/* TODO: 
        1. IMAGES
        2. SAFETY
        3. TRIGGER HAPPY
        4. ETC...
*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 1024

/*************************
 * VARIAVEIS GLOBAIS
 * ***********************/

char FShost[50];
char FSport[10];

int UDPfd;
int TCPfd;

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
void sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize);
char* sendMessageTCP(char *message, int mBufferSize, int *responseSize);


/*************************
 * CODIGO
 * **********************/

int main(int argc, char *argv[]) {

    char hostName[100], command[BUFFER_SIZE];
    char *buffer;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICSERV;

    /* Initializes default variables */
    strcpy(FSport, "58044");
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
            questionSubmitCommand(command);
        }
        else if (!strcmp(buffer, "answer_submit") || !strcmp(buffer, "as")) {
            answerSubmitCommand(command);
        }
        else {
            printf("unkown command\n");
        }

        /* Waits for user input */
        fgets(command, BUFFER_SIZE, stdin);
    }

    freeaddrinfo(res);
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
                break;
            case 'p':
                strcpy(FSport, optarg);
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

    if (connect(fd, res->ai_addr, res->ai_addrlen) == -1) {
        perror("ERROR: connect\n");
        exit(EXIT_FAILURE);
    }

    return fd;
}

void registerCommand(char *command) {

    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *id;
    char *c;

    id = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(id, '\n');
    *c = '\0';

    sprintf(message, "REG %s\n", id);

    sendMessageUDP(message, BUFFER_SIZE, response, BUFFER_SIZE);

    /* Shows response to user */
    if (!strcmp(response, " REG 0K\n")) {
        printf("User OK\n");
        strcpy(userID, id);
    }
    else if (!strcmp(response, "REG NOK\n")){
        printf("User NOK\n");
    }
    else {
        printf("ERR\n");
    }

}

void topicListCommand() {

    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    strcpy(message, "LTP\n");

    sendMessageUDP(message, BUFFER_SIZE, response, BUFFER_SIZE);

    /* Shows response to user */
    char *buffer;
    int n;
    char *c;

    /* Checks for \n */
    c = strrchr(response, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    else {
        *c = '\0';
    }

    strtok(response, " ");
    buffer = strtok(NULL, " ");
    n = atoi(buffer);
    printf("Topic list (Number Topic UserID):");
    for(; n > 0; n--) {
        buffer = strtok(NULL, " ");
        c = strchr(buffer, ':');
        *c = ' ';
        printf("%d %s\n", n - n + 1, buffer);
    }

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

    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char *topic;

    topic = strtok(NULL, " ");
    sprintf(message, "PTP %s %s", userID, topic);

    sendMessageUDP(message, BUFFER_SIZE, response, BUFFER_SIZE);

    /* Shows response to user */
    if (!strcmp(response, "PTR 0K\n")) {
        printf("Topic OK\n");
    }
    else if (strcmp(response, "PTR DUP\n")){
        printf("Topic DUP\n");
    }
    else if (strcmp(response, "PTR FUL\n")){
        printf("Topic list is full\n");
    }
    else {
        printf("ERR\n");
    }

}

void questionListCommand() {

    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sprintf(message, "LQU %s\n", selectedTopic);

    sendMessageUDP(message, BUFFER_SIZE, response, BUFFER_SIZE);

    /* Shows response to user */
    char *buffer;
    int n;
    char *c;

    /* Checks for \n */
    c = strrchr(response, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    else {
        *c = '\0';
    }

    strtok(response, " ");
    buffer = strtok(NULL, " ");
    n = atoi(buffer);
    printf("Question list (Number UserID AvailableAnswers):");
    for(; n > 0; n--) {
        buffer = strtok(NULL, " ");
        while ((c = strchr(buffer, ':')) != NULL) {
            *c = ' ';
        }
        printf("%d %s\n", n - n + 1, buffer);
    }

}

void questionGetCommand(char *command) {

    char message[BUFFER_SIZE];
    char *response;
    char *question;
    char *c;
    int responseSize;

    question = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(question, '\n');
    *c = '\0';
    strcpy(selectedQuestion, question);

    sprintf(message, "GQU %s %s\n", selectedTopic, selectedQuestion);

    response = sendMessageTCP(message, BUFFER_SIZE, &responseSize);

    /* Shows response to user */
    char *buffer, questionName[15], answerName[18];
    int qSize, aSize, N, AN;

    /* Checks for \n */
    c = strrchr(response, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    else {
        *c = '\0';
    }

    /* Creates question file */

    if (mkdir(selectedTopic, 0777) == -1) {
        perror("ERROR: mkdir\n");
        exit(EXIT_FAILURE);
    }
    strtok(response, " "); /* USERID */

    char *qData;

    /* Gets metadata */
    buffer = strtok(NULL, " ");
    qSize = atoi(buffer);
    qData = strtok(NULL, " ");
    sprintf(questionName, "%s/%s.txt", selectedTopic, selectedQuestion);

    /* Stores question data in file */
    FILE *q = fopen(questionName, "wb");
    fwrite(qData, sizeof(char), qSize, q);
    fclose(q);

    /* Creates answer files */
    char *aData;

    buffer = strtok(NULL, " ");
    N = atoi(buffer);
    for (; N > 0; N--) {

        /* Gets metadata */
        buffer = strtok(NULL, " ");
        AN = atoi(buffer);
        strtok(NULL, " "); /* USERID */
        buffer = strtok(NULL, " ");
        aSize = atoi(buffer);
        aData = strtok(NULL, " ");
        sprintf(answerName, "%s/%s_%d.txt", selectedTopic, selectedQuestion, AN);

        /* Stores answer data in file */
        FILE *a = fopen(answerName, "wb");
        fwrite(aData, sizeof(char), aSize, a);
        fclose(a);  
    }

    free(response);

}

void questionSubmitCommand(char *command) {

    char *question;
    char *fileName;
    int qsize;
    char *qdata;
    char *c;
    char *response;
    int responseSize;

    question = strtok(NULL, " ");
    fileName = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(fileName, '\n');
    *c = '\0';

    /* Gets file data */
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        printf("File not found\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    qsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    qdata = (char*) malloc(qsize);
    fread(qdata, sizeof(char), qsize, f);

    /* Creates message */
    char message[BUFFER_SIZE + qsize];
    sprintf(message, "QUS %s %s %s %d %s\n", userID, selectedTopic, question, qsize, qdata);

    response = sendMessageTCP(message, BUFFER_SIZE + qsize, &responseSize);

    /* Shows response to user */
    if (!strcmp(response, "QUR 0K\n")) {
        printf("question OK\n");
    }
    else if (strcmp(response, "QUR DUP\n")){
        printf("question DUP\n");
    }
    else if (strcmp(response, "QUR FUL\n")){
        printf("question list is full\n");
    }
    else {
        printf("NOK\n");
    }

    free(response);

}

void answerSubmitCommand(char *command) {

    char *fileName;
    int asize;
    char *adata;
    char *c;
    char *response;
    int responseSize;

    fileName = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(fileName, '\n');
    *c = '\0';

    /* Gets file data */
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        printf("File not found\n");
        return;
    }
    fseek(f, 0, SEEK_END);
    asize = ftell(f);
    fseek(f, 0, SEEK_SET);

    adata = (char*) malloc(asize);
    fread(adata, sizeof(char), asize, f);

    /* Creates message */
    char message[BUFFER_SIZE + asize];
    sprintf(message, "QUS %s %s %s %d %s\n", userID, selectedTopic, selectedQuestion, asize, adata);

    response = sendMessageTCP(message, BUFFER_SIZE + asize, &responseSize);

    /* Shows response to user */
    if (!strcmp(response, "NAS 0K\n")) {
        printf("answer OK\n");
    }
    else if (strcmp(response, "ANS FUL\n")){
        printf("answer list is full\n");
    }
    else {
        printf("NOK\n");
    }

    free(response);

}

void sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize) {
    
    int n;
    
    printf("Sending message: %s", message);
    /* Sends message */
    if ( (n =sendto(UDPfd, message, mBufferSize, 0, res->ai_addr, res->ai_addrlen)) == -1) {
        perror("ERROR: sendto\n");
        exit(EXIT_FAILURE);
    }
    printf("Sent: %d bytes\n", n);

    /* Waits for response */
    if ( (n = recvfrom(UDPfd, response, rBufferSize, 0, (struct sockaddr*) &addr, &addrlen)) == -1) {
        perror("ERROR: recvfrom");
        exit(EXIT_FAILURE);
    }
    printf("Recieved: %d bytes\n", n);
    printf("Recieved message: %s", response);
    
}

char* sendMessageTCP(char *message, int mBufferSize, int *responseSize) {

    char *response;
    int n, written = 0, left = BUFFER_SIZE;

    *responseSize = 0;

    TCPfd = openTCP();

    printf("Sending message: %s", message);
    /* Sends message */
    while (written < mBufferSize) {
        n = write(TCPfd, message + written, mBufferSize - written);
        if (n == -1) {
            perror("ERROR: write\n");
            exit(EXIT_FAILURE);
        }
        written += n;
    }
    
    /* Waits for response */
    *responseSize = BUFFER_SIZE;
    response = (char*) malloc(BUFFER_SIZE);
    while ((n = read(TCPfd, response + *responseSize - left, left)) > 0) {
        left -= n;
        if (left == 0) {
            left = BUFFER_SIZE;
            *responseSize += BUFFER_SIZE;
            if((response = (char *) realloc(response, *responseSize)) == NULL) {
                perror("ERROR: realloc\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    if (n == -1) {
        perror("ERROR: read\n");
        exit(EXIT_FAILURE);
    }
    printf("Recieved message: %s", response);

    close(TCPfd);

    return response;

}
