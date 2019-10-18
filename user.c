#include "auxiliary.h"

/*******************************************************************************
 * GLOBAL VARIABLES
 * ****************************************************************************/

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
long qlSize = 0;

/*******************************************************************************
 * DECLARATIONS
 * ****************************************************************************/

int openUDP();
int openTCP();
void parseArgs(int argc, char *argv[]);
void registerCommand(char *command);
void topicListCommand();
void topicSelectCommand(char *command, int flag);
void topicProposeCommand(char *command);
void questionListCommand();
void questionGetCommand(char *command, int flag);
void questionSubmitCommand(char *command);
void answerSubmitCommand(char *command);
int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize);

/*******************************************************************************
 * CODE
 * ****************************************************************************/

int main(int argc, char *argv[]) {

    struct sigaction pipe;
    char command[BUFFER_SIZE], *nextArg;

    /* Handles SIGPIPE */
    memset(&pipe, 0, sizeof pipe);
    pipe.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &pipe, NULL) == -1) {
        perror("ERROR: sigaction\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_NUMERICSERV;

    /* Initializes default variables */
    sprintf(FSport, "%d", DEFAULT_PORT);
    if(gethostname(FShost, HOST_NAME_MAX)) {
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
        else if (!strcmp(command, "question_get")) {
            if (tIsSet && qlSize > 0) {
                questionGetCommand(nextArg, 0);
            }
            else {
                if (!tIsSet) {
                    printf("No topic selected\n");
                }
                if (!(qlSize > 0)) {
                    printf("No question list available\n");
                }
            }
        }
        else if (!strcmp(command, "qg")) {
            if (tIsSet && qlSize > 0) {
                questionGetCommand(nextArg, 1);
            }
            else {
                if (!tIsSet) {
                    printf("No topic selected\n");
                }
                if (!(qlSize > 0)) {
                    printf("No question list available\n");
                }
            }
        }
        else if (!strcmp(command, "question_submit") || !strcmp(command, "qs")) {
            if (tIsSet && uIsSet) {
                questionSubmitCommand(nextArg);
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
                answerSubmitCommand(nextArg);
            }
            else {
                if (!tIsSet) {
                    printf("No topic selected\n");
                }
                if (!uIsSet) {
                    printf("No user registered\n");
                }
                if (!qIsSet) {
                    printf("No question selected\n");
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
        freeaddrinfo(res);
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
        freeaddrinfo(res);
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

/*******************************************************************************
 * COMMANDS
 * ****************************************************************************/

void registerCommand(char *command) {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long id, responseSize = 0;

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

    /* Shows response to user */
    if (!strcmp(response, "RGR OK\n")) {

        /* Saves user in case of acceptance */
        sprintf(userID, "%ld", id);
        uIsSet = 1;
        printf("User %s registered successfully\n", userID);
    }
    else if (!strcmp(response, "RGR NOK\n")){
        printf("Unable to register user %ld\n", id);
    }
    else {
        printf("ERR\n");
    }

}

void topicListCommand() {

    char message[BUFFER_SIZE], response[BUFFER_SIZE_L];
    long responseSize = 0;

    /* Builds message */
    strcpy(message, "LTP\n");

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE_L);

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
    long ID, IDLength, topicLength;

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
        tlSize = 0;
    }
    topicList = initList(N);
    arg = nextArg;

    /* Processes every topic */
    i = N;
    if (i > 0) {

        for(; i > 0; i--) {

            /* Gets topic */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR\n");
                return;
            }
            if (!isValidTopic(arg)) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR\n");
                return;
            }
            topic = arg;
            topicLength = nextArg - arg - 1;
            arg = nextArg;

            /* Gets userID */
            if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR\n");
                return;
            }
            if ((ID = getUserID(arg)) == -1) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR\n");
                return;
            }
            IDLength = nextArg - arg - 1;
            arg = nextArg;

            /* Allocates and saves topic in topic list */
            topicList[N - i] = (char*) malloc(sizeof(char) * (topicLength + IDLength + 2));
            sprintf(topicList[N - i], "%s %ld", topic, ID);
            tlSize++;

        }

        /* Checks if we parsed trough all arguments */
        if (nextArg != response + responseSize - 1) {
            freeList(topicList, tlSize);
            tlSize = 0;
            printf("ERR\n");
            return;
        }

        /* Prints list of topics */
        printTopicList(topicList, tlSize);

    }
    else {
        printf("No topics available\n");

        /* Checks if we parsed trough all arguments */
        if (nextArg != response + responseSize - 1) {
            freeList(topicList, tlSize);
            tlSize = 0;
            printf("ERR\n");
            return;
        }
    }

}

void topicSelectCommand(char *command, int flag) {

    int number;

    /* removes \n from end of string */
    if (deleteNewLine(command)) {
        printf("ERR\n");
        return;
    }

    /* short form ts */
    if (flag) {

        /* Gtes topic number */
        if ((number = (int) toPositiveNum(command)) == -1) {
            printf("Invalid topic number\n");
            return;
        }

        /* Checks if its valid */
        if (number == 0 || number > tlSize) {
            printf("Invalid topic number\n");
            return;
        }
        cpyUntilSpace(selectedTopic, topicList[number - 1]);
    }

    /* long form topic_select */
    else {

        /* Checks if its a valid topic */
        if (!isValidTopic(command)) {
            printf("Invalid topic name\n");
            return;
        }

        /* Checks if topic is in list */
        if (!isInList(command, topicList, tlSize)) {
            printf("Topic not in topic list\n");
            return;
        }
        strcpy(selectedTopic, command);
    }

    tIsSet = 1;

    printf("Topic %s successfully selected\n", selectedTopic);

}

void topicProposeCommand(char *command) {

    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    long responseSize = 0;

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

    /* Deletes new line character */
    if (deleteNewLine(response)) {
        printf("ERR\n");
        return;
    }

    /* Shows response to user */
    if (!strcmp(response, "PTR OK")) {

        /* Upsates selected topic in case of success */
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

    char message[BUFFER_SIZE], response[BUFFER_SIZE_L];
    long responseSize = 0;

    /* Builds message */
    sprintf(message, "LQU %s\n", selectedTopic);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE_L);

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
    long ID, NA, i, N, questionLength, IDLength, NALength;

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
        qlSize = 0;
    }
    questionList = initList(N);
    arg = nextArg;

    /* Processes every question */
    i = N;
    if (i > 0) {

        for(; i > 0; i--) {

            /* Gets question */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            if (!isValidQuestion(arg)) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            question = arg;
            questionLength = nextArg - arg - 1;
            arg = nextArg;

            /* Gets userID */
            if ((nextArg = getNextArg(arg, ':', -1)) == NULL) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            if ((ID = getUserID(arg)) == -1) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            IDLength = nextArg - arg - 1;
            arg = nextArg;

            /* Gets number of answers */
            if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            if ((NA = toPositiveNum(arg)) == -1) {
                freeList(questionList, qlSize);
                qlSize = 0;
                printf("ERR\n");
                return;
            }
            NALength = nextArg - arg - 1;
            arg = nextArg;

            /* Saves question in question list */
            questionList[N - i] = (char*) malloc(sizeof(char) * (questionLength + IDLength + NALength + 3));
            sprintf(questionList[N - i], "%s %ld %ld", question, ID, NA);
            qlSize++;

        }

        /* Checks if we parsed trough all arguments */
        if (nextArg != response + responseSize - 1) {
            freeList(topicList, tlSize);
            tlSize = 0;
            printf("ERR\n");
            return;
        }

        /* Prints list of topics */
        printQuestionList(questionList, qlSize);

    }
    else {

        /* Checks if we parsed trough all arguments */
        if (nextArg != response + responseSize - 1) {
            freeList(topicList, tlSize);
            tlSize = 0;
            printf("ERR\n");
            return;
        }

        printf("No questions available\n");
    }

}

void questionGetCommand(char *command, int flag) {

    char message[BUFFER_SIZE], question[QUESTION_SIZE + 1];
    long number;

    /* Deletes new line character */
    if (deleteNewLine(command)) {
        printf("ERR\n");
        fflush(stdout);
        return;
    }

    /* short form qg */
    if (flag) {

        /* Gets question number */
        if ((number = toPositiveNum(command)) == -1) {
            printf("Invalid question number\n");
            return;
        }

        /* Checks if its valid */
        if (number == 0 || number > qlSize) {
            printf("Invalid question number\n");
            return;
        }
        cpyUntilSpace(question, questionList[number - 1]);
        sprintf(message, "GQU %s %s\n", selectedTopic, question);
    }

    /* long form question_get */
    else {

        /* Checks if its a valid question */
        if (!isValidQuestion(command)) {
            printf("Invalid question name\n");
            return;
        }

        /* Checks if question is in list */
        if (!isInList(command, questionList, qlSize)) {
            printf("Question not in question list\n");
            return;
        }
        strcpy(question, command);
        sprintf(message, "GQU %s %s\n", selectedTopic, question);
    }

    TCPfd = openTCP();

    /* Sends message to server */
    if (writeBytes(TCPfd, message, strlen(message))) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Shows response to user */
    char *arg, *nextArg, qFileName[QUESTION_SIZE + TOPIC_SIZE + 2], aFileName[QUESTION_SIZE + TOPIC_SIZE + 5],  buffer[BUFFER_SIZE + 1], *c;
    int N, AN;

    /* Parses "QGR ID " */
    if (readBytes(TCPfd, buffer, 6 + ID_SIZE) == 0) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Checks for special responses */
    if (!strcmp(buffer, "QGR EOF\n")) {
        printf("Request can not be answered\n");
        close(TCPfd);
        return;
    }
    if (!strcmp(buffer, "QGR ERR\n")) {
        printf("Somehow question_get request was not correctly formulated\n");
        close(TCPfd);
        return;
    }

    /* Gets QGR */
    if ((nextArg = getNextArg(buffer, ' ', -1)) == NULL) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }
    if (strcmp(buffer, "QGR")) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }
    arg = nextArg;

    /* Gets ID */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }
    if (getUserID(arg) == -1) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }
    arg = nextArg;

    /* Creates dir to store files in */
    if (mkdir(selectedTopic, 0777) == -1) {
        if (errno != EEXIST) {
            perror("ERROR: mkdir\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Parses a Data Block */
    sprintf(qFileName, "%s/%s", selectedTopic, question);
    if (parseDataBlock(TCPfd, qFileName)) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Prints confirmation */
    c = strchr(qFileName, '/');
    printf("Question %s download\n", c + 1);

    /* Parses Number of answers */
    if (readBytes(TCPfd, buffer, 4) == 0) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }
    if (buffer[1] == '0') {
        if (buffer[2] != '\n' && readBytes(TCPfd, buffer, 2) != 0) {
            printf("ERR\n");
            close(TCPfd);
        }
        printf("Recieved question and available answers successfully\n");
        return;
    }
    else {
        if (buffer[2] == ' ') {
            buffer[2] = '\0';
            if ((N = toPositiveNum(buffer + 1)) == - 1) {
                close(TCPfd);
                printf("ERR\n");
                return;
            }
        }
        else {
            if ((N = toPositiveNum(buffer + 1)) == - 1) {
                close(TCPfd);
                printf("ERR\n");
                return;
            }

            /* Gets the space */
            if (readBytes(TCPfd, buffer, 2) == 0 || buffer[0] != ' ') {
                close(TCPfd);
                printf("ERR\n");
                return;
            }
        }
    }

    /* Only a maximum of 10 answers can be sent */
    if (N > 10) {
        close(TCPfd);
        printf("ERR\n");
        return;
    }

    /* Starts parsing answers */
    for (; N > 0; N--) {

        /* Gets answer number and ID */
        if (readBytes(TCPfd, buffer, 2 + 1 + ID_SIZE + 1 + 1) == 0) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }

        /* Reads answer number */
        if ((nextArg = getNextArg(buffer, ' ', 2)) == NULL) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }
        if ((AN = toPositiveNum(buffer)) == -1) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }
        if (AN > 10) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }
        arg = nextArg;

        /* Gets ID */
        if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }
        if (getUserID(arg) == -1) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }

        /* Processes Data Block */
        sprintf(aFileName, "%s/%s_%s", selectedTopic, question, buffer);
        if (parseDataBlock(TCPfd, aFileName)) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }

        /* Prints confirmation */
        printf("Answer number %d download\n", AN);

        /* Gets space or '\n' */
        if (readBytes(TCPfd, buffer, 2) == 0) {
            close(TCPfd);
            printf("ERR\n");
            return;
        }
        if (buffer[0] != ' ') {
            if (buffer[0] == '\n' && N != 1) {
                close(TCPfd);
                printf("ERR\n");
                return;
            }
        }

    }

    /* Checks if there are more bytes in the socket */
    if (read(TCPfd, buffer, BUFFER_SIZE) != 0) {
        close(TCPfd);
        printf("ERR\n");
        return;
    }

    close(TCPfd);

    /* Saves question as selected question */
    strcpy(selectedQuestion, question);
    qIsSet = 1;

    printf("Recieved question and all available answers successfully\n");

    return;

}

void questionSubmitCommand(char *command) {

    char *arg, *nextArg, *question, *fn, *in, buffer[BUFFER_SIZE + 1];
    long n;
    int IMG;

    /* Replace new line character */
    if (replaceNewLine(command, ' ')) {
        printf("ERR\n");
        return;
    }

    /* Gets question */
    if ((nextArg = getNextArg(command, ' ', -1)) == NULL) {
        printf("ERR\n");
        return;
    }
    if (!isValidQuestion(command)) {
        printf("Invalid question\n");
        return;
    }
    question = command;
    if (isInList(question, questionList, qlSize)) {
        printf("Question already exists\n");
        return;
    }
    arg = nextArg;

    /* Checks for file */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        printf("Please provide a file\n");
        return;
    }
    if ((fn = (char*) malloc(sizeof(char) * (nextArg - arg - 1 + 5))) == NULL) {
        perror("ERROR: malloc\n");
        exit(EXIT_FAILURE);
    }
    sprintf(fn, "%s%s", arg, ".txt");
    arg = nextArg;

    /* Checks for image */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        in = NULL;
        IMG = 0;
    }
    else {
        in = arg;
        IMG = 1;
    }

    /* Sends first bytes */
    sprintf(buffer, "QUS %s %s %s ", userID, selectedTopic, question);
    TCPfd = openTCP();
    if (writeBytes(TCPfd, buffer, strlen(buffer))) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Send Data Block */
    if (sendDataBlock(TCPfd, fn, IMG, in)) {
        close(TCPfd);
        return;
    }
    free(fn);
    printf("Question %s uploaded\n", question);

    /* Sends '\n' */
    sprintf(buffer, "\n");
    if (writeBytes(TCPfd, buffer, strlen(buffer))) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Recieves response */
    if ((n = readBytes(TCPfd, buffer, BUFFER_SIZE + 1)) == 0) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Checks if there are more bytes after '\0' */
    char *c;
    c = strchr(buffer, '\0');
    if (c != buffer + n) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Displays response */
    if (!strcmp(buffer, "QUR OK\n")) {

        /* Saves question in case of success */
        strcpy(selectedQuestion, question);
        qIsSet = 1;
        printf("Question submited successfully\n");
    }
    else if (!strcmp(buffer, "QUR DUP\n")) {
        printf("Question already exists\n");
    }
    else if (!strcmp(buffer, "QUR FUL\n")) {
        printf("Question list is full\n");
    }
    else if (!strcmp(buffer, "QUR NOK\n")) {
        printf("Something went wrong on server side\n");
    }
    else {
        printf("ERR\n");
    }

    close(TCPfd);

}

void answerSubmitCommand(char *command) {

    char *arg, *nextArg, *fn, *in, buffer[BUFFER_SIZE + 1];
    long n;
    int IMG;

    /* Replace new line character */
    if (replaceNewLine(command, ' ')) {
        printf("ERR\n");
        return;
    }

    /* Checks for file */
    if ((nextArg = getNextArg(command, ' ', -1)) == NULL) {
        printf("Please provide a file\n");
        return;
    }
    if ((fn = (char*) malloc(sizeof(char) * (nextArg - command - 1 + 5))) == NULL) {
        perror("ERROR: malloc\n");
        exit(EXIT_FAILURE);
    }
    sprintf(fn, "%s%s", command, ".txt");
    arg = nextArg;

    /* Checks for image */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        in = NULL;
        IMG = 0;
    }
    else {
        in = arg;
        IMG = 1;
    }

    /* Sends first bytes */
    sprintf(buffer, "ANS %s %s %s ", userID, selectedTopic, selectedQuestion);
    TCPfd = openTCP();
    if (writeBytes(TCPfd, buffer, strlen(buffer))) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Sends Data Block */
    if (sendDataBlock(TCPfd, fn, IMG, in)) {
        close(TCPfd);
        return;
    }
    free(fn);

    printf("Answer uploaded\n");

    /* Sends '\n' */
    sprintf(buffer, "\n");
    if (writeBytes(TCPfd, buffer, strlen(buffer))) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Recieves response */
    if ((n = readBytes(TCPfd, buffer, BUFFER_SIZE + 1)) == 0) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Checks if there are more bytes after '\0' */
    char *c;
    c = strchr(buffer, '\0');
    if (c != buffer + n) {
        printf("ERR\n");
        close(TCPfd);
        return;
    }

    /* Displays response */
    if (!strcmp(buffer, "ANR OK\n")) {
        printf("Answer submited successfully\n");
    }
    else if (!strcmp(buffer, "ANR FUL\n")) {
        printf("Answer list is full\n");
    }
    else if (!strcmp(buffer, "ANR NOK\n")) {
        printf("Something went wrong on server side\n");
    }
    else {
        printf("ERR\n");
    }

}

/*******************************************************************************
 * COMMUNICATION
 * ****************************************************************************/

/*  returns size of data read
    null terminates response */
int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize) {

    int n;
    fd_set active_fd_set;
    struct timeval timeout;

    /*
    printf("Sending %d bytes and message: \"", mBufferSize);
    fflush(stdout);
    write(1, message, mBufferSize);
    printf("\"\n");
    fflush(stdout);
    */

    /* Sends message */
    if ((n =sendto(UDPfd, message, mBufferSize, 0, res->ai_addr, res->ai_addrlen)) == -1) {
        perror("ERROR: sendto\n");
        exit(EXIT_FAILURE);
    }

    /* Initializes set of active fds */
    FD_ZERO(&active_fd_set);
    FD_SET(UDPfd, &active_fd_set);

    /* Sets timeout for server response */
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    if (select(FD_SETSIZE, &active_fd_set, NULL, NULL, &timeout) < 0) {
        perror("ERROR: select\n");
        exit(EXIT_FAILURE);
    }
    if (FD_ISSET(UDPfd, &active_fd_set)) {
        if ((n = recvfrom(UDPfd, response, rBufferSize, 0, (struct sockaddr*) &addr, &addrlen)) == -1) {
            perror("ERROR: recvfrom");
            exit(EXIT_FAILURE);
        }
    }
    else {
        printf("Server took to long to respond\n");
        return 0;
    }

    /* Null terminates response */
    if (n < rBufferSize) {
        response[n] = '\0';
        n++;
    }
    else {
        response[n - 1] = '\0';
    }

    /*
    printf("Recieved %d bytes and response: \"%s\"\n", n - 1, response);
    */

    return n;

}
