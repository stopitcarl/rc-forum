/*  Perguntar ao prof:
    1. LTR o ultimo (topic:userID )* nao leva espaço
    2. same whit LQR

    TODO:
    1. THINK OF A SMARTER WAY OF LEADING WITH TOPIC AND QUESTION LISTS
*/

#include "aux.h"

/************************************************
 * VARIAVEIS GLOBAIS
 * *********************************************/

char FShost[HOST_NAME_MAX];
char FSport[MAX_PORT_SIZE + 1];

int UDPfd;
int TCPfd;

socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

char userID[ID_SIZE + 1];
int uIsSet = 0;
char selectedTopic[TOPIC_SIZE + 1];
int tIsSet = 0;
char selectedQuestion[QUESTION_SIZE + 1];
int qIsSet = 0;
char **topicList;
int tlSize = 0;
char **questionList;
int qlSize = 0;

/************************************************
 * DECLARACOES
 * *********************************************/

int openUDP();
int openTCP();
void parseArgs(int argc, char *argv[]);
void registerCommand(char *command);
void topicListCommand();
void topicSelectCommand(char *command, int flag);
void topicProposeCommand(char *command);
void questionListCommand();
void questionGetCommand(char *command, int flag);
char* handleDataBlock(char *content, int cSize, char *fn);
char* handleFile(char *content, int cSize, char *fn);
char* handleImage(char *content, int cSize, char *fn);
void questionSubmitCommand(char *command);
void answerSubmitCommand(char *command);
char* submitAux(char *type, char *userID, char *topic, char *question, long *bufferSize);
int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize);
char* sendMessageTCP(char *message, long mBufferSize, long *responseSize);

/*************************************************************
 * CODIGO
 *************************************************************/

int main(int argc, char *argv[]) {

    char command[BUFFER_SIZE], *nextArg;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICSERV;

    /* Initializes default variables */
    sprintf(FSport, "%d", DEFAULT_PORT);
    if(gethostname(FShost, 100)) {
        perror("ERROR: gethostname\n");
        exit(EXIT_FAILURE);
    }

    /* Parses Args */
    parseArgs(argc, argv);

    printf("Communicating with: %s\n", FShost);
    printf("On port: %s\n", FSport);

    UDPfd = openUDP();

    /* Main cycle */
    fgets(command, BUFFER_SIZE, stdin);
    while (strcmp(command, "exit\n")) {

        /* Gets command name */
        nextArg = getNextArg(command, ' ', -1);

        /* Checks for command type */
        if (!strcmp(command, "register") || !strcmp(command, "reg")) {
            if (uIsSet) {
                printf("User %s already registered\n", userID);
            }
            else {
                registerCommand(nextArg);
            }
        }
        else if (!strcmp(command, "topic_list\n") || !strcmp(command, "tl\n")) {
            topicListCommand();
        }
        else if (!strcmp(command, "topic_select")) {
            if (tlSize > 0) {
                topicSelectCommand(nextArg, 0);
            }
            else {
                printf("No topic list unavailable \n");
            }
        }
        else if (!strcmp(command, "ts")) {
            if (tlSize > 0) {
                topicSelectCommand(nextArg, 1);
            }
            else {
                printf("No topic list unavailable \n");
            }
        }
        else if (!strcmp(command, "topic_propose") || !strcmp(command, "tp")) {
            if (uIsSet) {
                topicProposeCommand(nextArg);   
            }
            else {
                printf("No user registered\n");
            }
        }
        else if (!strcmp(command, "question_list\n") || !strcmp(command, "ql\n")) {
            if (tIsSet) {
                questionListCommand();
            }
            else {
                printf("No topic selected\n");
            }
        }
        else if (!strcmp(command, "question_get") || !strcmp(command, "qg")) {
            if (tIsSet) {
                //questionGetCommand(nextArg);
            }
            else {
                printf("No topic selected\n");
            }
        }
        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs")) {
            if (tIsSet && uIsSet) {
                //questionSubmitCommand(nextArg);
            }
            else {
                if (!tIsSet) {
                    printf("No topic selected\n");
                }
                if (!uIsSet) {
                    printf("No user registered\n");
                }
            }
        }
        else if (!strcmp(command, "answer_submit") || !strcmp(command, "as")) {
            if (tIsSet && qIsSet && uIsSet) {
                //answerSubmitCommand(nextArg);
            }
            else {
                if (!tIsSet) {
                    printf("No topic selected\n");
                }
                if (!uIsSet) {
                    printf("No user registered\n");
                }
            }
        }
        else {
            printf("ERR\n");
        }

        /* Waits for user input */
        fgets(command, BUFFER_SIZE, stdin);
    }

    if (tlSize > 0) {
        freeList(topicList, tlSize);
    }
    if (qlSize > 0) {
    freeList(questionList, qlSize);
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
                if (strlen(optarg) == 0 || strlen(optarg) > MAX_PORT_SIZE || toPositiveNum(optarg) == -1) {
                    printf("Port must be a positive number with %d or less digits\n", MAX_PORT_SIZE);
                    exit(EXIT_FAILURE);
                }
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

/*************************************************************
 * COMMANDS
 *************************************************************/

void registerCommand(char *command) {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long id, responseSize;

    /* removes \n from end of string */
    if (deleteNewLine(command)) {
        printf("ERR\n");
        return;
    }

    /* Checks if its a valid ID */
    if ((id = getUserID(command)) == -1) {
        printf("Invalid ID\n");
        return;
    }

    /* Builds message */
    sprintf(message, "REG %ld\n", id);

    /* Sends it trough UDP */
    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);

    /* Checks if response is valid */
    if (!isValidResponse(response, responseSize)) {
        printf("ERR\n");
        return;
    }

    /* Deletes '\n' */
    if (deleteNewLine(response)) {
        printf("ERR\n");
        return;
    }

    /* Shows response to user */
    if (!strcmp(response, "RGR OK")) {

        /* Saves user in case of acceptance */
        sprintf(userID, "%ld", id);
        uIsSet = 1;
        printf("User %s registered successfully\n", userID);
    }
    else if (!strcmp(response, "RGR NOK")){
        printf("Unable to register user %ld\n", id);
    }
    else {
        printf("ERR\n");
    }

}

void topicListCommand() {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long responseSize;

    /* Builds message */
    strcpy(message, "LTP\n");

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);

    /* Checks if response is valid */
    if (!isValidResponse(response, responseSize)) {
        printf("ERR\n");
        return;
    }

    /* replaces end of line char with space */
    if (replaceNewLine(response, ' ')) {
        printf("ERR\n");
        return;
    }

    /* Shows response to user */
    char *arg, *nextArg, *topic;
    int N, i;
    long ID;

    /* Checks for LTR */
    if ((nextArg = getNextArg(response, ' ', -1)) == NULL) {
        printf("ERR\n");
        return;
    }
    if (strcmp(response, "LTR")) {
        printf("ERR\n");
        return;
    }
    arg = nextArg;

    /* Gets N */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        printf("ERR\n");
        return;
    }

    /* Turns it into a number */
    if ((N = (int) toPositiveNum(arg)) == -1) {
        printf("ERR\n");
        return;
    }

    /* Checks if number is valid */
    if (N > MAX_TOPICS) {
        printf("ERR\n");
        return;
    }

    /* Allocates topic list */
    if (tlSize > 0) {
        freeList(topicList, tlSize);
    }
    topicList = initList(N, TOPIC_SIZE + ID_SIZE + 2);
    arg = nextArg;

    /* Processes every topic */
    i = N;
    if (i > 0) {

        for(; i > 0; i--) {

            /* Gets topic */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(topicList, tlSize);
                printf("ERR\n");
                return;
            }
            if (!isValidTopic(arg)) {
                freeList(topicList, tlSize);
                printf("ERR\n");
                return;
            }
            topic = arg;
            arg = nextArg;

            /* Gets userID */
            if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
                freeList(topicList, tlSize); 
                printf("ERR\n");
                return;
            }
            if ((ID = getUserID(arg)) == -1) {
                freeList(topicList, tlSize);
                printf("ERR\n");
                return;
            }
            arg = nextArg;

            /* Saves topic in topic list */
            sprintf(topicList[N - i], "%s %ld", topic, ID);

        }

        tlSize = N;

        /* Prints list of topics */
        printTopicList(topicList, tlSize);

    }
    else {
        printf("No topics available\n");
    } 

}

void topicSelectCommand(char *command, int flag) {

    char *topic;
    int number;

    /* removes \n from end of string */
    if (deleteNewLine(command)) {
        printf("ERR\n");
        return;
    }

    /* short form ts */
    if (flag) {
        if ((number = toPositiveNum(command)) == -1) {
            printf("Invalid topic number\n");
            return;
        }
        if (number == 0 || number > tlSize) {
            printf("Invalid topic number\n");
            return;
        }
        cpyUntilSpace(selectedTopic, topicList[number - 1]);
    }

    /* long form topic_select */
    else {
        topic = command;

        /* Checks if its a valid topic */
        if (!isValidTopic(topic)) {
            printf("Invalid topic name\n");
            return;
        }

        /* Checks if topic is in list */
        if (!isInList(topic, topicList, tlSize)) {
            printf("Topic not on topic list\n");
            return;
        }
        strcpy(selectedTopic, topic);
    }

    tIsSet = 1;

    printf("Topic %s successfully selected\n", selectedTopic);

}

void topicProposeCommand(char *command) {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long responseSize;

    /* Deletes new line character */
    if (deleteNewLine(command)) {
        printf("ERR\n");
        return;
    }

    /* Checks if its a valid topic */
    if (!isValidTopic(command)) {
        printf("Invalid topic name\n");
        return;
    }

    /* Builds message */
    sprintf(message, "PTP %s %s\n", userID, command);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);

    /* Checks if response is valid */
    if (!isValidResponse(response, responseSize)) {
        printf("ERR\n");
        return;
    }

    /* Deletes '\n' */
    if (deleteNewLine(response)) {
        printf("ERR\n");
        return;
    }

    /* Shows response to user */
    if (!strcmp(response, "PTR OK")) {
        strcpy(selectedTopic, command);
        tIsSet = 1;
        printf("New topic %s successufly submited\n", selectedTopic);
    }
    else if (!strcmp(response, "PTR DUP")){
        printf("Topic %s already exists\n", command);
    }
    else if (!strcmp(response, "PTR FUL")){
        printf("Max number of topics reached\n");
    }
    else {
        printf("ERR\n");
    }

}

void questionListCommand() {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long responseSize;

    /* Builds message */
    sprintf(message, "LQU %s\n", selectedTopic);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);

    /* Checks if response is valid */
    if (!isValidResponse(response, responseSize)) {
        printf("ERR\n");
        return;
    }

    /* replaces end of line char with space */
    if (replaceNewLine(response, ' ')) {
        printf("ERR\n");
        return;
    }

    /* Shows response to user */
    char *arg, *nextArg, *question;
    long ID, NA, i, N;

    /* Checks for LQR */
    if ((nextArg = getNextArg(response, ' ', -1)) == NULL) {
        printf("ERR\n");
        return;
    }
    if (strcmp(response, "LQR")) {
        printf("ERR\n");
        return;
    }
    arg = nextArg;

    /* Gets N */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        printf("ERR\n");
        return;
    }

    /* Turns it into a number */
    if ((N = toPositiveNum(arg)) == -1) {
        printf("ERR\n");
        return;
    }
    /* Allocates topic list */
    if (qlSize > 0) {
        freeList(questionList, qlSize);
    }
    questionList = initList(N, TOPIC_SIZE + ID_SIZE + 10 + 3); /* hoping NA is no more than 10 characters long*/
    arg = nextArg;

    /* Processes every question */
    i = N;
    if (i > 0) {

        for(; i > 0; i--) {

            /* Gets question */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(questionList, qlSize);
                printf("ERR\n");
                return;
            }
            if (!isValidQuestion(arg)) {
                freeList(questionList, qlSize);
                printf("ERR\n");
                return;
            }
            question = arg;
            arg = nextArg;

            /* Gets userID */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(questionList, qlSize);
                printf("ERR\n");
                return;
            }
            if ((ID = getUserID(arg)) == -1) {
                freeList(questionList, qlSize);
                printf("ERR\n");
                return;
            }
            arg = nextArg;

            /* Gets number of answers */
            if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
                freeList(questionList, qlSize);
                printf("ERR\n");
                return;
            }
            if ((NA = toPositiveNum(arg)) == -1) {
                freeList(questionList, qlSize);
                return;
            }
            arg = nextArg;

            /* Saves question in question list */
            sprintf(questionList[N - i], "%s %ld %ld", question, ID, NA);
        
        }

        qlSize = N;

        /* Prints list of topics */
        printQuestionList(questionList, qlSize);

    }
    else {
        printf("No questions available\n");
    }

}

/*************************************************************
 * COMMUNICATION
 *************************************************************/

int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize) {
    
    int n;
    
    printf("Sending %d bytes and message: \"", mBufferSize);
    fflush(stdout);
    write(1, message, mBufferSize);
    printf("\"\n");
    fflush(stdout);
    /* Sends message */
    if ( (n =sendto(UDPfd, message, mBufferSize, 0, res->ai_addr, res->ai_addrlen)) == -1) {
        perror("ERROR: sendto\n");
        exit(EXIT_FAILURE);    
    }

    /* Waits for response */
    if ( (n = recvfrom(UDPfd, response, rBufferSize, 0, (struct sockaddr*) &addr, &addrlen)) == -1) {
        perror("ERROR: recvfrom");
        exit(EXIT_FAILURE);
    }
    printf("Recieved %d bytes and response: \"", n);
    fflush(stdout);
    write(1, response, n);
    printf("\"\n");
    fflush(stdout);

    return n;

}

char* sendMessageTCP(char *message, long mBufferSize, long *responseSize) {

    char *response;
    long written = 0;
    int n, left = BUFFER_SIZE;

    *responseSize = 0;

    TCPfd = openTCP();
    printf("Sending %ld bytes and message: \"", mBufferSize);
    fflush(stdout);
    write(1, message, mBufferSize);
    printf("\"\n");
    fflush(stdout);
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
    if (response == NULL) {
        perror("ERROR: malloc\n");
        exit(EXIT_FAILURE);
    }
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
    *responseSize -= left;
    printf("Recieved %ld bytes and response: \"", *responseSize);
    fflush(stdout);
    write(1, response, *responseSize);
    printf("\"\n");
    fflush(stdout);

    close(TCPfd);

    return response;

}
