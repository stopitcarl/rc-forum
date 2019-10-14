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
#include <dirent.h>
#include <sys/stat.h>
#include "auxiliary.h"

#define PORT "58044"
#define BUFFER_SIZE 1024
#define TOPIC_LEN 16
#define QUESTION_LEN 19
#define SMALL_BUFFER_SIZE 64

int transferBytes, udp, tcp;
int nTopics;
fd_set rfds, afds;
socklen_t addrlen;
struct addrinfo hints, *res;
struct sockaddr_in addr;

void closeServer()
{
    freeaddrinfo(res);
    FD_CLR(udp, &rfds);
    FD_CLR(udp, &afds);
    close(udp);
    FD_CLR(tcp, &rfds);
    FD_CLR(tcp, &afds);
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
int readFromTCP(int src, char *buffer) {
    int nbytes, nleft, nread;

    nbytes = BUFFER_SIZE;
    nleft = nbytes;

    /*Makes sure the entire message is read*/
    while ((nread = read(src, buffer + nbytes - nleft, nleft)) > 0) {
        nleft -= nread;
        if (nleft == 0) {
            nleft = BUFFER_SIZE;
            nbytes += BUFFER_SIZE;
            if ((buffer = (char *) realloc(buffer, nbytes)) == NULL)
                error("Error on realloc");
        }

        if (*(buffer + nbytes - nleft - 1) == '\n') break;
    }

    if (nread == -1)
        error("Error reading from tcp socket");

    *(buffer + nbytes - nleft) = '\0';

    return nbytes - nleft;
}

/*Writes to tcp socket*/
int replyToTCP(char *msg, int dst)
{
    int nbytes, n, nwritten = 0;

    nbytes = transferBytes;

    /*Makes sure the entire message is written*/
    while (nwritten < nbytes) {
        n = write(dst, msg + nwritten, nbytes - nwritten);
        if (n == -1)
            error("Error writing to tcp socket");
        nwritten += n;
    }

    return nwritten;
}


int openTCP()
{
    //Sets up tcp socket
    int fd, n;

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
    int n;

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

// ######## START OF AUX FUNCTIONS ###########

int countQuestions(char *dirName) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Counts number questions in certain topic*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') == NULL) {
            count++;
        }
    }

    closedir(dir);

    return count;
}

int countAnswers(char *dirName, char *question) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Counts number of answers to a certain question*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') != NULL &&
                !strcmp(strtok(entry->d_name, "_"), strtok(question, "-"))) {
            count++;
        }
    }

    closedir(dir);

    return count;
}

int countTopics() {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (entry->d_name[0] == '.')
                continue;
            count++;
        }
    }

    closedir(dir);

    return count;
}

int findImage(char *dirName, char *question, char *imageName) {
    DIR *dir;
    struct dirent *entry;
    char testFile[TOPIC_LEN + 4];

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Cycles through directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            strcpy(testFile, entry->d_name);
            if (!strcmp(strtok(testFile, "-"), question)) {
                if (strstr(entry->d_name, ".txt") == NULL) {
                    strcpy(imageName, entry->d_name);
                    return 1;
                }
            }
        }
    }

    return 0;
}
// ######## END OF AUX FUNCTIONS ###########


// ######## START OF UDP COMMANDS ###########

/*Performs register command and saves the reply in status*/
void registerCommand(char *id, char *status)
{
    char *arg = getNextArg(id, ' ', -1);

    if (arg != NULL) {
        strcpy(status, "RGR NOK\n");
        return;
    }

    if (isValidID(id))
    {
        strcpy(status, "RGR OK\n");
    }
    else
        strcpy(status, "RGR NOK\n");
}

/*Lists available topics to send back to user*/
void topicListCommand(char *response) {
    DIR *dir;
    struct dirent *entry;
    char topics[99 * (TOPIC_LEN + 1)];
    char *fp;

    topics[0] = '\0';

    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (entry->d_name[0] == '.')
                continue;
            strcat(topics, " ");
            fp = strchr(entry->d_name, '-');
            if (fp != NULL) *fp = ':';
            strcat(topics, entry->d_name);
        }
    }

    closedir(dir);

    if (nTopics == 0)
        strcpy(response, "LTR 0\n");
    else
        sprintf(response, "LTR %d%s\n", nTopics, topics);
}

/*Returns the directory name of a certain topic*/
void findTopic(char *topic, char *dirName) {
    DIR *dir;
    struct dirent *entry;
    char *name, aux[TOPIC_LEN];


    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            if (entry->d_name[0] == '.')
                continue;

            strcpy(aux, entry->d_name);
            name = strtok(aux, "-");
            if (!strcmp(topic, name)) {
                strcpy(dirName, entry->d_name);
                return;
            }
        }
    }

    closedir(dir);

    dirName[0] = '\0';
}

/*Searches a certain topic for a question*/
int findQuestion(char *topicDir, char *question, char *fileName) {
    DIR *dir;
    struct dirent *entry;
    char testFile[TOPIC_LEN + 4];

    /*Opens directory*/
    if (!(dir = opendir(topicDir)))
        error("Error opening directory");

    /*Cycles through the topic's directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') == NULL) {
            strcpy(testFile, entry->d_name);
            if (!strcmp(strtok(testFile, "-"), question) &&
                    strstr(entry->d_name, ".txt") != NULL) {
                if (fileName != NULL) strcpy(fileName, entry->d_name);
                return 1;
            }
        }
    }

    return 0;
}

/*Searches a certain topic for an answer*/
int findAnswer(char *topicDir, char *answer) {
    DIR *dir;
    struct dirent *entry;

    /*Opens directory*/
    if (!(dir = opendir(topicDir)))
        error("Error opening directory");

    /*Cycles through the topic's directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') != NULL) {
            if (!strcmp(strtok(entry->d_name, "-"), answer))
                return 1;
        }
    }

    return 0;
}

/*Proposes topic to the forum*/
void topicProposeCommand(char *id, char *response) {
    char *topic, *lastArg;
    char dirName[TOPIC_LEN + 8];
    char testTopic[TOPIC_LEN];
    int n;

    topic = getNextArg(id, ' ', -1);

    if (topic == NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    lastArg = getNextArg(topic, ' ', -1);

    if (lastArg != NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    findTopic(topic, testTopic);

    /*Checks if id and topic are correct*/
    if (!isValidID(id) || !isValidTopic(topic)) {
        strcpy(response, "PTR NOK\n");
        return;
    } else if (nTopics == 99) {
        strcpy(response, "PTR FUL\n");
        return;
    } else if (testTopic[0] != '\0') {
        strcpy(response, "PTR DUP\n");
        return;
    }

    sprintf(dirName, "topics/%s-%s", topic, id);

    /*Creates topic in directory*/
    if ((n = mkdir(dirName, 0777)) == -1) {
        error("Error creating dir");
    } else {
        strcpy(response, "PTR OK\n");
        nTopics++;
    }
}

/*Lists questions in a certain topic*/
void questionListCommand(char *topic, char *response) {
    DIR *dir;
    struct dirent *entry;
    char topicDir[TOPIC_LEN];
    char dirName[TOPIC_LEN + 8];
    char questions[99 * (QUESTION_LEN + 1)];
    char *fp;
    int nQuestions;
    char *lastArg = getNextArg(topic, ' ', -1);

    if (lastArg != NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    questions[0] = '\0';

    findTopic(topic, topicDir);

    if (topicDir[0] == '\0') {
        strcpy(response, "ERR\n");
        return;
    }

    sprintf(dirName, "topics/%s", topicDir);

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    nQuestions = 0;

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            if (strchr(entry->d_name, '_') == NULL &&
                    strstr(entry->d_name, ".txt") != NULL) {
                int nAnswers = countAnswers(dirName, entry->d_name);
                char answerString[2];
                sprintf(answerString, "%d", nAnswers);
                strcat(questions, " ");
                fp = strchr(entry->d_name, '-');
                if (fp != NULL) *fp = ':';
                strcat(questions, strtok(entry->d_name, "."));
                strcat(questions, ":");
                strcat(questions, answerString);
                nQuestions++;
            }
        }
    }

    closedir(dir);

    if (nQuestions == 0) {
        strcpy(response, "LQR 0\n");
    } else {
        sprintf(response, "LQR %d%s\n", nQuestions, questions);
    }
}

// ######## END OF UDP COMMANDS ###########

// ######## START OF TCP COMMANDS ###########

void questionGetCommand(char *topic, char *response){
    char *question, *lastArg;
    char dirName[TOPIC_LEN + 8];
    char testTopic[TOPIC_LEN];
    char testQuestion[TOPIC_LEN + 4];
    char questionFile[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char answer[TOPIC_LEN + 2];
    char answerFile[TOPIC_LEN + 9 + TOPIC_LEN + 2 + 4];
    char imageName[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char *id;
    char *questionData;
    char *answerData;
    long size, newSize;
    int IMG, nAnswers;

    puts("quesiton get");

    transferBytes = BUFFER_SIZE;

    question = getNextArg(topic, ' ', -1);
    if (question == NULL) {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    }

    lastArg = getNextArg(question, ' ', -1);
    if (lastArg != NULL) {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    }

    findTopic(topic, testTopic);
    sprintf(dirName, "topics/%s", testTopic);

    /*Checks if question and topic are correct*/
    if (!isValidTopic(topic) || *testTopic == '\0' || !isValidQuestion(question)) {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    } else if (!findQuestion(dirName, question, testQuestion)) {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    }

    sprintf(questionFile, "%s/%s", dirName, testQuestion);

    /*Gets id from question file*/
    id = testQuestion;
    while (*id != '-' && *id != '\0')
        ++id;

    if (*id == '\0') {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    }

    id++;
    id[5] = '\0';


    IMG = findImage(dirName, question, imageName);

    printf("%s\n", questionFile);
    fflush(stdout);
    printf("%d\n", IMG);
    fflush(stdout);
    printf("%ln", &size);
    fflush(stdout);

    if (!IMG) {
        puts("!IMG");
        questionData = createDataBlock(questionFile, IMG, NULL, &size);
    } else {
        puts("IMG");
        questionData = createDataBlock(questionFile, IMG, imageName, &size);
    }

    puts("Out of data block");

    if (questionData == NULL) {
        strcpy(response, "ERR\n");
        transferBytes = strlen(response);
        return;
    }

    while (size > transferBytes + 10) {
        transferBytes += BUFFER_SIZE;
        response = (char *) realloc(response, transferBytes + 10);
    }

    nAnswers = countAnswers(dirName, questionFile);

    printf("id: %s\n", id);
    fflush(stdout);
    printf("questionData: %s\n", questionData);
    fflush(stdout);
    printf("nAnswers: %d\n", nAnswers);
    fflush(stdout);


    sprintf(response, "QGR %s %s %d ", id, questionData, nAnswers);
    free(questionData);

    /*Gets all the answers to question*/
    for (int i = nAnswers; i > nAnswers - 10 && i > 0; i--) {
        sprintf(answer, "%s_%02d", question, i);
        sprintf(answerFile, "%s/%s-%s", dirName, answer, id);

        IMG = findImage(dirName, answer, imageName);

        if (!IMG)
            answerData = createDataBlock(answerFile, IMG, NULL, &newSize);
        else
            answerData = createDataBlock(answerFile, IMG, imageName, &newSize);

        if (answerData == NULL) {
            strcpy(response, "ERR\n");
            transferBytes = strlen(response);
            return;
        }

        while (newSize > size) {
            size += BUFFER_SIZE;
            response = (char *) realloc(response, size);
        }

        strcat(response, " ");
        strcat(response, answerData);
        free(answerData);
    }

    strcat(response, "\n");
}

/*Submits question to a certain topic*/
void questionSubmitCommand(char *id, char *response) {
    char *topic, *question, *data;
    char *ret;
    char dirName[TOPIC_LEN + 8];
    char questionName[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char testTopic[TOPIC_LEN];
    int nQuestions;

    /*Get arguments*/
    topic = getNextArg(id, ' ', -1);
    if (topic == NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    question = getNextArg(topic, ' ', -1);
    if (question == NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    data = getNextArg(question, ' ', -1);
    if (data == NULL) {
        strcpy(response, "ERR\n");
        return;
    }
    findTopic(topic, testTopic);

    sprintf(dirName, "topics/%s", testTopic);

    /*Checks if id and topic are correct*/
    if (!isValidID(id) || !isValidTopic(topic) || *testTopic == '\0' ||
            !isValidQuestion(question)) {
        strcpy(response, "QUR NOK\n");
        return;
    } else if (findQuestion(dirName, question, NULL)) {
        strcpy(response, "QUR DUP\n");
        return;
    } else if ((nQuestions = countQuestions(dirName)) == 99) {
        strcpy(response, "QUR FUL\n");
        return;
    }

    sprintf(questionName, "%s/%s-%s", dirName, question, id);

    strcat(data, " ");

    //lSize = atol(size);
    ret = parseDataBlock(data, transferBytes, questionName);
    if (ret == NULL) {
        strcpy(response, "QUR NOK\n");
        return;
    }

    strcpy(response, "QUR OK\n");
}

/*Submits answer to a certain question*/
void answerSubmitCommand(char *id, char *response) {
    char *topic, *question, *data;
    char *ret;
    char dirName[TOPIC_LEN + 8];
    char answerName[TOPIC_LEN + 9 + TOPIC_LEN + 4 + 3];
    char questionName[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char testTopic[TOPIC_LEN];
    int nAnswers;

    /*Get arguments*/
    topic = getNextArg(id, ' ', -1);
    if (topic == NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    question = getNextArg(topic, ' ', -1);
    if (question == NULL) {
        strcpy(response, "ERR\n");
        return;
    }

    data = getNextArg(question, ' ', -1);
    if (data == NULL) {
        strcpy(response, "ERR\n");
        return;
    }
    findTopic(topic, testTopic);

    sprintf(dirName, "topics/%s", testTopic);
    sprintf(questionName, "%s/%s-%s", dirName, question, id);

    /*Checks if id and topic are correct*/
    if (!isValidID(id) || !isValidTopic(topic) || *testTopic == '\0' ||
            !isValidQuestion(question)) {
        strcpy(response, "ANR NOK\n");
        return;
    } else if (!findQuestion(dirName, question, NULL)) {
        strcpy(response, "ANR NOK\n");
        return;
    } else if ((nAnswers = countAnswers(dirName, questionName)) == 99) {
        strcpy(response, "ANR FUL\n");
        return;
    }

    strcat(data, " ");
    sprintf(answerName, "%s/%s_%02d-%s",dirName, question, ++nAnswers, id);

    ret = parseDataBlock(data, transferBytes, answerName);
    if (ret == NULL) {
        strcpy(response, "QUR NOK\n");
        return;
    }

    strcpy(response, "ANR OK\n");
}

// ######## END OF TCP COMMANDS ###########


void handleCommand(char *request, char *response)
{
    char *arg;

    if (deleteNewLine(request)) {
        strcpy(response, "ERR\n");
        return;
    }


    /* Gets command name */
    arg = getNextArg(request, ' ', -1);

    /* Checks for command type */
    if (!strcmp(request, "REG"))
    {
        registerCommand(arg, response);
    } else if (!strcmp(request, "LTP"))
    {
        topicListCommand(response);
    }
    else if (!strcmp(request, "PTP"))
    {
        topicProposeCommand(arg, response);
    }
    else if (!strcmp(request, "LQU"))
    {
        questionListCommand(arg, response);
    }
    else if (!strcmp(request, "GQU"))
    {
        questionGetCommand(arg, response);
    }
    else if (!strcmp(request, "QUS"))
    {
        questionSubmitCommand(arg, response);
    }
    else if (!strcmp(request, "ANS"))
    {
        answerSubmitCommand(arg, response);
    }
    else
    {
        strcpy(response, "ERR\n");
    }
}

int main()
{
    struct sigaction pipe, intr, child;
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    char *tcpBuffer, *tcpResponse;
    int n, client;
    pid_t pid;

    udp = openUDP();
    tcp = openTCP();

    nTopics = countTopics();

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

    FD_ZERO(&afds);
    FD_SET(udp, &afds);
    FD_SET(tcp, &afds);

    while (1)
    {
        rfds = afds;

        do {
            n = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        } while (n == -1 && errno == EINTR);
        if (n == -1)
            error("Error on select");

        else if (FD_ISSET(udp, &rfds))
        {
            //Got message from udp server
            memset(buffer, 0, sizeof buffer);
            addrlen = sizeof(addr);
            transferBytes = recvfrom(udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (transferBytes == -1)
                error("Error receiving from udp socket");

            // Update client
            client = udp;

            write(1, "udp received: ", 14);
            write(1, buffer, transferBytes);

            *(buffer + transferBytes) = '\0';
            handleCommand(buffer, response);
            transferBytes = sendto(client, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
            if (transferBytes == -1)
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
                tcpBuffer = (char *) malloc(BUFFER_SIZE);
                tcpResponse = (char *) malloc(BUFFER_SIZE);
                if (tcpBuffer == NULL)
                    error("Error on malloc");

                transferBytes = readFromTCP(client, tcpBuffer);
                write(1, "tcp received: ", 14);
                write(1, tcpBuffer, transferBytes);
                handleCommand(tcpBuffer, tcpResponse);
                printf("%s\n", tcpResponse);
                replyToTCP(tcpResponse, client);
                close(client);
                freeaddrinfo(res);
                free(tcpBuffer);
                free(tcpResponse);
                exit(EXIT_SUCCESS);
            }

            do {
                ret = close(client);
            } while (ret == -1 && errno == EINTR);
            if (ret == -1)
                error("Error closing client");
        }
    }

    puts("Closing server\n");
    freeaddrinfo(res);
    FD_CLR(udp, &rfds);
    close(udp);
    FD_CLR(tcp, &rfds);
    close(tcp);
    return 0;
}
