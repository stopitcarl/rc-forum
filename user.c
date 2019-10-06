/*  Perguntar ao prof:
    1. LTR o ultimo (topic:userID )* nao leva espaço
    2. same whit LQR

    TODO:
    1. CALCULATE SIZE NEEDED FOR RESPONSE AND FOR MESSAGE
    2. VERIFICAR SE QUANDO ACABEI DE DAR PARSE NOS ARGUMENTOS AINDA HA MAIS LIXO
*/

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
char* parseDataBlock(char *content, long cSize, char *fn);
char* handleFile(char *content, int cSize, char *fn);
char* handleImage(char *content, int cSize, char *fn);
void questionSubmitCommand(char *command);
void answerSubmitCommand(char *command);
char* submitAux(char *type, char *userID, char *topic, char *question, long *bufferSize);
int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize);
char* sendMessageTCP(char *message, long mBufferSize, long *responseSize);

/*******************************************************************************
 * CODE
 * ****************************************************************************/

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

/*******************************************************************************
 * COMMANDS
 * ****************************************************************************/

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

    /* Deletes new line character */
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
        printf("ERR1\n");
        return;
    }

    /* replaces end of line char with space */
    if (replaceNewLine(response, ' ')) {
        printf("ERR2\n");
        return;
    }

    /* Shows response to user */
    char *arg, *nextArg, *topic;
    int N, i;
    long ID, IDLength, topicLength;

    /* Checks for LTR */
    if ((nextArg = getNextArg(response, ' ', -1)) == NULL) {
        printf("ERR3\n");
        return;
    }
    if (strcmp(response, "LTR")) {
        printf("ERR4\n");
        return;
    }
    arg = nextArg;

    /* Gets N */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        printf("ERR5\n");
        return;
    }

    /* Turns it into a number */
    if ((N = (int) toPositiveNum(arg)) == -1) {
        printf("ERR6\n");
        return;
    }

    /* Checks if number is valid */
    if (N > MAX_TOPICS) {
        printf("ERR7\n");
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
                printf("ERR8\n");
                return;
            }
            if (!isValidTopic(arg)) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR9\n");
                return;
            }
            topic = arg;
            topicLength = nextArg - arg;
            arg = nextArg;

            /* Gets userID */
            if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
                freeList(topicList, tlSize); 
                tlSize = 0;
                printf("ERR10\n");
                return;
            }
            if ((ID = getUserID(arg)) == -1) {
                freeList(topicList, tlSize);
                tlSize = 0;
                printf("ERR11\n");
                return;
            }
            IDLength = nextArg - arg;
            arg = nextArg;

            /* Allocates and saves topic in topic list */
            topicList[N - i] = (char*) malloc(sizeof(char) * (topicLength + IDLength + 2));
            sprintf(topicList[N - i], "%s %ld", topic, ID);
            tlSize++;

        }

        /* Prints list of topics */
        printTopicList(topicList, tlSize);

    }
    else {
        printf("No topics available\n");
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
            questionLength = nextArg - arg;
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
            IDLength = nextArg - arg;
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
            NALength = nextArg - arg;
            arg = nextArg;

            /* Saves question in question list */
            questionList[N - i] = (char*) malloc(sizeof(char) * (questionLength + IDLength + NALength + 3));
            sprintf(questionList[N - i], "%s %ld %ld", question, ID, NA);
            qlSize++;
        
        }

        /* Prints list of topics */
        printQuestionList(questionList, qlSize);

    }
    else {
        printf("No questions available\n");
    }

}

void questionGetCommand(char *command, int flag) {
    
    char message[BUFFER_SIZE], *response, question[QUESTION_SIZE + 1];
    long responseSize, number;

    /* Deletes new line character */
    if (deleteNewLine(command)) {
        printf("ERR\n");
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

    response = sendMessageTCP(message, strlen(message), &responseSize);

    /* Checks if its a valid response */
    if (!isValidResponse(response, responseSize)) {
        free(response);
        printf("ERR1\n");
        return;
    }

    /* Checks for special responses */
    if (!strcmp(response, "QGR EOF\n")) {
        free(response);
        printf("Request can not be answered\n");
        return;
    }
    if (!strcmp(response, "QGU ERR\n")) {
        free(response);
        printf("Somehow QGU request was not correctly formulated\n");
        return;
    }

    /* Replaces new line character with space (cant use replaceNewLine because files may contain '\0') */
    response[responseSize - 2] = ' ';

    /* Shows response to user */
    char *arg, *nextArg, qFileName[QUESTION_SIZE + TOPIC_SIZE + 2], aFileName[QUESTION_SIZE + TOPIC_SIZE + 5];
    int N, AN;

    /* Creates dir to store files in */
    if (mkdir(selectedTopic, 0777) == -1) {
        if (errno != EEXIST) {
            perror("ERROR: mkdir\n");
            exit(EXIT_FAILURE);
        }
    }

    /* Gets QGR */
    if ((nextArg = getNextArg(response, ' ', -1)) == NULL) {
        free(response);
        printf("ERR3\n");
        return;
    }
    if (strcmp(response, "QGR")) {
        free(response);
        printf("ERR4\n");
        return;
    }
    arg = nextArg;

    /* Gets ID */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        free(response);
        printf("ERR5\n");
        return;
    }
    if (getUserID(arg) == -1) {
        free(response);
        printf("ERR6\n");
        return;
    }
    arg = nextArg;

    /* parses a Data Block */
    sprintf(qFileName, "%s/%s", selectedTopic, question);
    if ((arg = parseDataBlock(arg, responseSize - (arg - response), qFileName)) == NULL) {
        free(response);
        printf("ERR7\n");
        return;
    }

    /* Gets number of answers */
    if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
        free(response);
        printf("ERR8\n");
        return;
    }
    if ((N = toPositiveNum(arg)) == - 1) {
        free(response);
        printf("ERR9\n");
        return;
    }

    /* Only a maximum of 10 answers can be sent */
    if (N > 10) {
        free(response);
        printf("ERR10\n");
        return;
    }
    arg = nextArg;
    
    /* Starts processing each answer */
    for (; N > 0; N--) {

        /* Gets answer number */
        if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
            free(response);
            printf("ERR11\n");
            return;
        }
        if ((AN = toPositiveNum(arg)) == -1) {
            free(response);
            printf("ERR12\n");
            return;
        }
        if (AN > 10) {
            free(response);
            printf("ERR13\n");
            return;
        }
        arg = nextArg;

        /* Gets ID */
        if ((nextArg = getNextArg(arg, ' ', -1)) == NULL) {
            free(response);
            printf("ERR14\n");
            return;
        }
        if (getUserID(arg) == -1) {
            free(response);
            printf("ERR15\n");
            return;
        }
        arg = nextArg;

        /* Processes Data Block */
        sprintf(aFileName, "%s/%s_%d", selectedTopic, question, AN);
        if ((arg = parseDataBlock(arg, responseSize - (arg - response), aFileName)) == NULL) {
            free(response);
            printf("ERR16\n");
            return;
        }

    }

    /* Selects question as localy active question */
    strcpy(selectedQuestion, question);

    free(response);

}

/*  Parses a Data Block consisting of "fSize fData IMG [iExt iSize iData]"
    content must be null terminated and have a space before null terminator
    fn is the name of the file where data is going to be stored in
    function handles extensions (file assumed to be .txt)
    returns a pointer to end of string or null in case of insuccess */
char* parseDataBlock(char *content, long cSize, char *fn) {

    char *arg, *nextArg, fileName[TOPIC_SIZE + QUESTION_SIZE + 6];
    int IMG;

    /* Handles "fSize fData" */
    sprintf(fileName, "%s.txt", fn);
    if ((arg = handleFile(content, cSize, fileName)) == NULL) {
        printf("1");
        return NULL;
    }

    /* Checks if there is an image */
    if ((nextArg = getNextArg(arg, ' ', 1)) == NULL) {
        printf("2");
        return NULL;
    } 
    if ((IMG = (int) toPositiveNum(arg)) == -1) {
        printf("3");
        return NULL;
    }

    /* Checks if IMG is a binary flag */
    if (IMG < 0 || IMG > 1) {
        printf("4");
        return NULL;
    }
    arg = nextArg;

    /* There is an image */
    if (IMG) {

        /* Handles "iExt iSize iData" */
        if ((nextArg = handleImage(arg, cSize - (nextArg - content), fn)) == NULL) {
            printf("5");
            return NULL;
        }
    }
        
    return nextArg;

}

/*  Handles the image portion of the Data Block "ext size data"
    return a pointer to end of portion or null in case of insuccess */
char* handleImage(char *content, int cSize, char *fn) {

    char *nextArg, imageName[strlen(fn) + EXTENSION_SIZE + 2];

    /* Gets image extension */
    if ((nextArg = getNextArg(content, ' ', EXTENSION_SIZE)) == NULL) {
        return NULL;
    }
    sprintf(imageName, "%s.%s", fn, content);

    /* Handles "size data" */
    if ((nextArg = handleFile(nextArg, cSize - (nextArg - content), imageName)) == NULL) {
        return NULL;
    }

    return nextArg;

}

/*  Handles the file portion of the Data Block "size data"
    returns a pointer to end of portion or null in case of insucess */
char* handleFile(char *content, int cSize, char *fn) {

    char *nextArg;
    long size;

    /* Gets size of data */
    if ((nextArg = getNextArg(content, ' ', -1)) == NULL) {
        return NULL;
    }
    if ((size = toPositiveNum(content)) == -1) {
        return NULL;
    }

    /* Checks if size isnt greater than space left on the buffer */
    if (nextArg + size > content + cSize - 1) {
        return NULL;
    }

    /* Stores data in file */
    FILE *f = fopen(fn, "w");
    if (f == NULL) {
        perror("ERROR: fopen\n");
        exit(EXIT_FAILURE);
    }
    fwrite(nextArg, sizeof(char), size, f);
    fclose(f);

    return nextArg + size + 1;

}

/*******************************************************************************
 * COMMUNICATION
 * ****************************************************************************/

/*  returns size of data read
    null terminates response */
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
    /* Null terminates response */
    if (n < rBufferSize) {
        response[n] = '\0';
        n++;
    }
    else {
        response[n - 1] = '\0';
    }
    printf("Recieved %d bytes and response: \"%s\"\n", n - 1, response);

    return n;

}

/*  returns null terminated response
    stores amount of data read in responseSize */
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

    write(1, response, *responseSize - left);

    /* Null terminates reponse */
    if (left == 0) {
        printf("aaaaaaa\n");
        left = BUFFER_SIZE;
        *responseSize += BUFFER_SIZE;
        if((response = (char *) realloc(response, *responseSize)) == NULL) {
            perror("ERROR: realloc\n");
            exit(EXIT_FAILURE);
        }
    }
    *(response + *responseSize - left) = '\0';
    left--;
    *responseSize -= left;

    close(TCPfd);

    printf("Recieved %ld bytes and response: \"", *responseSize - 1);
    fflush(stdout);
    write(1, response, *responseSize);
    printf("\"\n");
    fflush(stdout);

    return response;

}
