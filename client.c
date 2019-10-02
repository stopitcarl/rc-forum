// Client

/* TODO: 
        1. RUN TESTS TO SEE IF STUFF WORKS
        2. TS NUMBER/QS NUMBER
        3. ETC...
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

char userID[6];
int uIsSet = 0;
char selectedTopic[11];
int tIsSet = 0;
char selectedQuestion[11];
int qIsSet = 0;

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
char* handleDataBlock(char *content, int cSize, char *fn);
char* handleFile(char *content, int cSize, char *fn);
char* handleImage(char *content, int cSize, char *fn);
void questionSubmitCommand(char *command);
void answerSubmitCommand(char *command);
char* submitAux(char *type, char *userID, char *topic, char *question, long *bufferSize);
int sendMessageUDP(char *message, int mBufferSize, char *response, int rBufferSize);
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
            if (uIsSet == 0) {
                printf("ERR\n");
            }
            else {
                topicProposeCommand(command);
            }
        }
        else if (!strcmp(buffer, "question_list\n") || !strcmp(buffer, "ql\n")) {
            if (tIsSet == 0) {
                printf("ERR\n");
            }
            else {
                questionListCommand();
            }
        }
        else if (!strcmp(buffer, "question_get") || !strcmp(buffer, "qg")) {
            if (tIsSet == 0) {
                printf("ERR\n");
            }
            else {
                questionGetCommand(command);
            }
        }
        else if (!strcmp(buffer, "question_submit") || !strcmp(buffer, "qs")) {
            if (tIsSet == 0 || uIsSet == 0) {
                printf("ERR\n");
            }
            else {
                questionSubmitCommand(command);
            }
        }
        else if (!strcmp(buffer, "answer_submit") || !strcmp(buffer, "as")) {
            if (tIsSet == 0 || qIsSet == 0 || uIsSet == 0) {
                printf("ERR\n");
            }
            else {
                answerSubmitCommand(command);
            }
        }
        else {
            printf("ERR\n");
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

    char message[BUFFER_SIZE], response[BUFFER_SIZE], *c, *buffer, *i;
    int id, responseSize;

    /* Gets id */
    buffer = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(buffer, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    *c = '\0';
    if (strlen(buffer) != 5) {
        printf("ERR\n");
        return;
    }

    id = (int) strtol(buffer, &i, 10);
    if (*i != '\0') {
        printf("ERR\n");
        return;
    }
    sprintf(message, "REG %d\n", id);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);
    if (responseSize == -1) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    if (!strcmp(response, "RGR OK")) {
        sprintf(userID, "%d", id);
        uIsSet = 1;
        printf("User %s registered successfully\n", userID);
    }
    else if (!strcmp(response, "RGR NOK")){
        printf("Unable to register user %d\n", id);
    }
    else {
        printf("ERR\n");
    }

}

void topicListCommand() {

    char message[BUFFER_SIZE], response[BUFFER_SIZE], *i;
    int responseSize;

    strcpy(message, "LTP\n");

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);
    if (responseSize == -1) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    char *buffer, *c;
    int n, N;

    buffer = strtok(response, " ");
    if (buffer == NULL || strcmp(buffer, "LTR")) {
        printf("ERR\n");
        return;
    }
    buffer = strtok(NULL, " ");
    if (buffer == NULL) {
        printf("ERR\n");
        return;
    }

    N = (int) strtol(buffer, &i, 10);
    if (*i != '\0') {
        printf("ERR\n");
        return;
    }
    n = N;
    if (n > 0) {
        printf("Topic list (Number Topic UserID):\n");
        for(; n > 0; n--) {
            buffer = strtok(NULL, " ");
            if (buffer == NULL) {
                printf("ERR\n");
                return;
            }
            c = strchr(buffer, ':');
            if (c == NULL) {
                printf("ERR\n");
                return;
            }
            *c = ' ';
            printf("Topic %d. %s\n", N - n + 1, buffer);
        }
    }
    else {
        printf("No topics available\n");
    }

}

void topicSelectCommand(char *command) {

    char *topic;
    char *c;

    topic = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(topic, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    *c = '\0';

    if (topic == NULL || strlen(topic) == 0) {
        printf("ERR\n");
        return;
    }

    strcpy(selectedTopic, topic);
    tIsSet = 1;

    printf("Topic %s successfully selected\n", selectedTopic);

}

void topicProposeCommand(char *command) {

    char message[BUFFER_SIZE], response[BUFFER_SIZE], *topic, *c;
    int responseSize;

    topic = strtok(NULL, " ");

    /* removes \n from end of string */
    c = strchr(topic, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    *c = '\0';

    if (topic == NULL || strlen(topic) == 0) {
        printf("ERR\n");
        return;
    }

    sprintf(message, "PTP %s %s\n", userID, topic);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);
    if (responseSize == -1) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    if (!strcmp(response, "PTR OK")) {
        strcpy(selectedTopic, topic);
        tIsSet = 1;
        printf("New topic %s successufly submited\n", selectedTopic);
    }
    else if (!strcmp(response, "PTR DUP")){
        printf("Topic %s already exists\n", topic);
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
    int responseSize;

    sprintf(message, "LQU %s\n", selectedTopic);

    responseSize = sendMessageUDP(message, strlen(message), response, BUFFER_SIZE);
    if (responseSize == -1) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    char *buffer, *c, *i;
    int n, N, j;

    buffer = strtok(response, " ");
    if (strcmp(buffer, "LQR")) {
        printf("ERR\n");
        return;
    }
    buffer = strtok(NULL, " ");
    if (buffer == NULL) {
        printf("ERR\n");
        return;
    }
    N = (int) strtol(buffer, &i, 10);
    if (*i != '\0') {
        printf("ERR\n");
        return;
    }
    n = N;
    if (n > 0) {
        printf("Question list (Number UserID AvailableAnswers):\n");
        for(; n > 0; n--) {
            buffer = strtok(NULL, " ");
            if (buffer == NULL) {
                printf("ERR\n");
                return;
            }
            for (j = 0; j < 2; j++) {
                c = strchr(buffer, ':');
                if (c == NULL) {
                    printf("ERR\n");
                    return;
                }
                *c = ' ';
            }
            printf("Question %d. %s\n", N - n + 1, buffer);
        }
    }
    else {
        printf("No questions available for selected topic\n");
    }

}

void questionGetCommand(char *command) {

    char message[BUFFER_SIZE], *response, *question, *c, *i;
    int responseSize;

    question = strtok(NULL, " ");
        
    /* removes \n from end of string */
    c = strchr(question, '\n');
    if (c == NULL) {
        printf("ERR\n");
        return;
    }
    *c = '\0';

    if (question == NULL || strlen(question) == 0) {
        printf("ERR\n");
        return;
    }

    sprintf(message, "GQU %s %s\n", selectedTopic, question);

    response = sendMessageTCP(message, strlen(message), &responseSize);
    if (response == NULL) {
        return;
    }
    if (responseSize == 0) {
        printf("QGR EOF\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("QGR ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /***************************************************
     *  Response Handling.
     *  We can't use strtok here because files might
     *  have spaces in them
     ***************************************************/

    char *ps, *ns, qFileName[21], aFileName[24];
    int N, AN;

    /* Creates question folder */
    if (mkdir(selectedTopic, 0777) == -1) {
        perror("ERROR: mkdir\n");
        return;
    }

    /*  Checks first argument  */
    ns = strchr(response, ' ');
    if (ns == NULL) {
        printf("QGR ERR\n");
        return;
    }
    *ns = '\0';
    if (strcmp(response, "QGR")) {
        printf("QGR ERR\n");
        return;
    }
    ps = ns;

    /* Processes a data block */
    sprintf(qFileName, "%s/%s", selectedTopic, question);
    if ((ps = handleDataBlock(ps + 1, response + responseSize - ps - 1, qFileName)) == NULL) {
        printf("QGR ERR\n");
        return;
    }

    /* Gets number of answers */
    ns = strchr(ps + 1, ' ');
    if (ns == NULL) {
        if ((N = (int) strtol(ps + 1, &i, 10)) != 0) {
            printf("QGR ERR\n");
            return;
        }
        return;
    }
    else {
        *ns = '\0';
        if (strlen(ps + 1) == 0) {
            printf("QGR ERR\n");
        }
        N = (int) strtol(ps + 1, &i, 10);
        if (*i != '\0') {
            printf("QGR ERR\n");
            return;
        }
        ps = ns;
    }

    /* Processes each answer */ 
    for (; N > 0; N--) {

        /* Gets answer number */
        ns = strchr(ps + 1, ' ');
        if (ns == NULL) {
            printf("QGR ERR\n");
            return;
        }
        *ns = '\0';
        if (strlen(ps + 1) == 0) {
            printf("QGR ERR\n");
            return;
        }
        AN = (int) strtol(ps + 1, &i, 10);
        if (*i != '\0') {
            printf("QGR ERR\n");
            return;
        }
        ps = ns;
        /* Processes a data block */
        sprintf(aFileName, "%s/%s_%d", selectedTopic, question, AN);
        if ((ps = handleDataBlock(ps + 1, response + responseSize - ps - 1, aFileName)) == NULL) {
            printf("QGR ERR\n");
            return;            
        }
    }

    strcpy(selectedQuestion, question);
    qIsSet = 1;

    free(response);

}

/*  "UserID Size Data IMG [Ext Size Data]"; 
    fn is the name of the file without extension
    RETURNS: pointer to end of string */
char* handleDataBlock(char *content, int cSize, char *fn) {

    char *ps, *ns, *i, fileName[strlen(fn) + 4];
    int qIMG;

    /* Ignores UserID */
    ns = strchr(content, ' ');
    if (ns == NULL) {
        return NULL;
    }

    /* Saves file */
    sprintf(fileName, "%s.txt", fn);
    if ((ns = handleFile(ns + 1, content + cSize - ns - 1, fileName)) == NULL) {
        return NULL;
    }
    ps = ns;

    /* Checks for image */
    if (*(ps + 1) != '\0') {
        if (*(ps + 2) == ' ') {
            ns = ps + 2;
            *ns = '\0';
            qIMG = strtol(ps + 1, &i, 10);
            if (*i != '\0') {
                return NULL;
            }
        }
        else if (*(ps + 2) == '\0') {
            if ((qIMG = (int) strtol(ps + 1, &i, 10)) != 0) {
                return NULL;
            }
        }
        else {
            return NULL;
        }
    }
    else {
        return NULL;
    }
    if (qIMG) {
        if ((ns = handleImage(ns + 1, content + cSize - ns + 1, fn)) == NULL) {
            return NULL;
        }
    }

    return ns;

}

/*  Recieves a string whit format "Ext Size Data" 
    and saves it to file whit name fn.Ext
    RETURNS: pointer to end of string 
*/
char* handleImage(char *content, int cSize, char *fn) {

    char *p, *ext, file[strlen(fn) + 4];

    /* Gets image extension */
    p = strchr(content, ' ');
    if (p == NULL) {
        return NULL;
    } 
    if (p - content != 3) {
        return NULL;
    }
    *p = '\0';
    ext = content;

    sprintf(file, "%s.%s", fn, ext);

    return handleFile(p + 1, content + cSize - p - 1, file);

}

/*  Recieves a string whit format "Size Data" 
    and saves it to file whit name fn
    RETURNS: pointer to end of string 
*/
char* handleFile(char *content, int cSize, char *fn) {
    
    char *p, *i;
    int size;

    /* Gets file size */  
    p = strchr(content, ' ');
    if (p == NULL) {
        return NULL;
    } 
    *p = '\0';
    size = strtol(content, &i, 10);
    if (*i != '\0') {
            return NULL;
    }
    if (p + 1 + size >= content + cSize) { /* segfault prevention */
        return NULL;
    }

    /* Saves data to file */
    FILE *f = fopen(fn, "w");
    fwrite(p + 1, sizeof(char), size, f);
    fclose(f);
    p += size + 1;
    
    return p;

}

void questionSubmitCommand(char *command) {

    char *question, *response, *message;
    int responseSize;
    long bufferSize;

    question = strtok(NULL, " ");
    if (question == NULL || strlen(question) == 0) {
        printf("ERR\n");
        return;
    }

    if ((message = submitAux("QUS", userID, selectedTopic, question, &bufferSize)) == NULL) {
        return;
    }

    response = sendMessageTCP(message, bufferSize, &responseSize);
    free(message);
    if (response == NULL) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR1\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR2\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    if (!strcmp(response, "QUR OK")) {
        strcpy(selectedQuestion, question);
        qIsSet = 1;
        printf("Question successfully added\n");
    }
    else if (!strcmp(response, "QUR DUP")){
        printf("Question already exists\n");
    }
    else if (!strcmp(response, "QUR FUL")){
        printf("Question list is full\n");
    }
    else {
        printf("Unable to submit question\n");
    }

    free(response);

}

void answerSubmitCommand(char *command) {

    char *message, *response;
    int responseSize;
    long bufferSize;

    if ((message = submitAux("ANS", userID, selectedTopic, selectedQuestion, &bufferSize)) == NULL) {
        return;
    }
    
    response = sendMessageTCP(message, bufferSize, &responseSize);
    free(message);
    if (response == NULL) {
        return;
    }
    if (responseSize == 0) {
        printf("ERR\n");
        return;
    }

    /* checks for \n */
    if (response[responseSize - 1] != '\n') {
        printf("ERR\n");
        return;
    }
    else {
        response[responseSize - 1] = '\0';
    }

    /* Shows response to user */
    if (!strcmp(response, "ANR OK")) {
        printf("Answered successfully\n");
    }
    else if (!strcmp(response, "ANS FUL")){
        printf("Answer list is full\n");
    }
    else {
        printf("Unable to submit answer\n");
    }

    free(response);

}

/*  Recieves id topic question and "fileName\0[imageFile]\0"
    RETURNS: "type id topic question fSize fData IMG [iExt iSize iData]"
*/
char* submitAux(char *type, char *id, char *topic, char *question, long *bufferSize) {

    char *fileName, *fData, *imageFile, *iExt, *iData, *buffer, *c;
    int fSize = 0, iSize = 0, IMG = 0;

    fileName = strtok(NULL, " ");
    if (fileName == NULL || strlen(fileName) == 0) {
        printf("ERR\n");
        return NULL;
    }

    /* removes \n from end of string */
    c = strchr(fileName, '\n');
    if (c != NULL) {
        *c = '\0';
    }

    /* Gets file data */
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        printf("ERROR: fopen\n");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    fSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    fData = (char*) malloc(fSize * sizeof(char) + 1);
    if (fData == NULL) {
        printf("ERROR: malloc\n");
        return NULL;
    }
    fread(fData, sizeof(char), fSize, f);
    fData[fSize] = '\0';
    fclose(f);

    /* Checks for image */
    if ((imageFile = strtok(NULL, " ")) != NULL) {
        IMG = 1;

        /* removes \n from end of string */
        c = strchr(imageFile, '\n');
        if (c == NULL) {
            printf("ERR\n");
            return NULL;
        }
        *c = '\0';
        
        c = strchr(imageFile, '.');
        if (c == NULL) {
            printf("ERR\n");
            return NULL;
        }
        iExt = c + 1;
        if (strlen(iExt) != 3) {
            printf("ERR\n");
            return NULL;
        }

        FILE *i = fopen(imageFile, "rb");
        if (i == NULL) {
            printf("ERROR: fopen\n");
            return NULL;
        }
        fseek(i, 0, SEEK_END);
        iSize = ftell(i);
        fseek(i, 0, SEEK_SET);

        iData = (char*) malloc(iSize * sizeof(char));
        if (iData == NULL) {
            printf("ERROR: malloc\n");
            return NULL;
        }
        fread(iData, sizeof(char), iSize, i);
        fclose(i);
    }
    else {
        IMG = 0;
    }

    /* Creates return string */
    if (IMG) {
        buffer = (char*) malloc(sizeof(char) * (BUFFER_SIZE + fSize + iSize));
        if (buffer == NULL) {
            printf("ERROR: malloc\n");
            return NULL;
        }
        sprintf(buffer, "%s %s %s %s %d %s %d %s %d ", type, id, topic, question, fSize, fData, IMG, iExt, iSize);
        *bufferSize = strlen(buffer);
        c = strchr(buffer, '\0');
        if (c == NULL) {
            return NULL;
        }
        memcpy(c, iData, iSize);
        *(c + iSize) = '\n';
        *bufferSize = *bufferSize + iSize + 1;
    }
    else {
        buffer = (char*) malloc(sizeof(char)*(BUFFER_SIZE + fSize));
        sprintf(buffer, "%s %s %s %s %d %s %d\n", type, id, topic, question, fSize, fData, IMG);
        *bufferSize = strlen(buffer);
    }

    free(fData);
    if (IMG) {
        free(iData);
    }

    return buffer;

}

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
        return -1;
    }

    /* Waits for response */
    if ( (n = recvfrom(UDPfd, response, rBufferSize, 0, (struct sockaddr*) &addr, &addrlen)) == -1) {
        perror("ERROR: recvfrom");
        return -1;
    }
    printf("Recieved %d bytes and response: \"", n);
    fflush(stdout);
    write(1, response, n);
    printf("\"\n");
    fflush(stdout);

    return n;

}

char* sendMessageTCP(char *message, int mBufferSize, int *responseSize) {

    char *response;
    int n, written = 0, left = BUFFER_SIZE;

    *responseSize = 0;

    TCPfd = openTCP();
    printf("Sending %d bytes and message: \"", mBufferSize);
    fflush(stdout);
    write(1, message, mBufferSize);
    printf("\"\n");
    fflush(stdout);
    /* Sends message */
    while (written < mBufferSize) {
        n = write(TCPfd, message + written, mBufferSize - written);
        if (n == -1) {
            perror("ERROR: write\n");
            close(TCPfd);
            return NULL;
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
        close(TCPfd);
        return NULL;
    }
    *responseSize -= left;
    printf("Recieved %d bytes and response: \"", *responseSize);
    fflush(stdout);
    write(1, response, *responseSize);
    printf("\"\n");
    fflush(stdout);

    close(TCPfd);

    return response;

}
