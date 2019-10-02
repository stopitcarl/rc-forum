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

#define PORT "58044"
#define BUFFER_SIZE 1024
#define TOPIC_LEN 16
#define QUESTION_LEN 19
#define SMALL_BUFFER_SIZE 64

int n, udp, tcp;
int nTopics;
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

    /*Coutns number of answers to a certain question*/
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') != NULL &&
                !strcmp(strtok(entry->d_name, "_"), strtok(question, "."))) {
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

/*Performs register command and saves the reply in status*/
void registerCommand(char *status)
{
    char *type = strtok(NULL, " ");

    if (strlen(type) == 6 && deleteNewLine(type))
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
            name = strtok(aux, ":");
            if (!strcmp(topic, name)) {
                strcpy(dirName, entry->d_name);
                return;
            }
        }
    }

    closedir(dir);

    dirName[0] = '\0';
}

/*Proposes topic to the forum*/
void topicProposeCommand(char *response) {
    char *userID = strtok(NULL, " ");
    char *topic = strtok(NULL, " ");
    char dirName[TOPIC_LEN + 8];
    char testTopic[TOPIC_LEN];
    int n;

    deleteNewLine(topic);
    findTopic(topic, testTopic);

    /*Checks if id and topic are correct*/
    if (strlen(userID) != 5 || strlen(topic) > 10) {
        strcpy(response, "PTR NOK\n");
        return;
    } else if (nTopics == 99) {
        strcpy(response, "PTR FUL\n");
        return;
    } else if (testTopic[0] != '\0') {
        strcpy(response, "PTR DUP\n");
        return;
    }

    sprintf(dirName, "topics/%s:%s", topic, userID);

    /*Creates topic in directory*/
    if ((n = mkdir(dirName, 0777)) == -1) {
        error("Error creating dir");
    } else {
        strcpy(response, "PTR OK\n");
        nTopics++;
    }
}

/*Lists questions in a certain topic*/
void questionListCommand(char *response) {
    DIR *dir;
    struct dirent *entry;
    char *topic = strtok(NULL, " ");
    char topicDir[TOPIC_LEN];
    char dirName[TOPIC_LEN + 8];
    char questions[99 * (QUESTION_LEN + 1)];
    int nQuestions;

    questions[0] = '\0';

    deleteNewLine(topic);
    findTopic(topic, topicDir);

    if (topicDir[0] == '\0') {
        strcpy(response, "LQR 0\n");
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
            if (strchr(entry->d_name, '_') == NULL) {
                int nAnswers = countAnswers(dirName, entry->d_name);
                printf("answers: %d\n", nAnswers);
                char answerString[2];
                sprintf(answerString, "%d", nAnswers);
                strcat(questions, " ");
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

void handleCommand(char *request, char *response)
{
    char *type;

    /* Gets command name */
    type = strtok(request, " ");

    /* Checks for command type */
    // TODO: implement commented funtions
    if (!strcmp(type, "REG"))
    {
        registerCommand(response);
    }
    else if (!strcmp(request, "LTP\n"))
    {
        topicListCommand(response);
    }
    else if (!strcmp(type, "PTP"))
    {
        topicProposeCommand(response);
    }
    else if (!strcmp(type, "LQU"))
    {
        questionListCommand(response);
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
        strcpy(response, "ERR\n");
    }
}

int main()
{
    struct sigaction pipe, intr, child;
    char response[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int client;
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
            memset(buffer, 0, sizeof buffer);
            addrlen = sizeof(addr);
            n = recvfrom(udp, buffer, 128, 0, (struct sockaddr *)&addr, &addrlen);
            if (n == -1)
                error("Error receiving from udp socket");

            // Update client
            client = udp;

            write(1, "udp received: ", 14);
            write(1, buffer, n);

            handleCommand(buffer, response);
            n = sendto(client, response, strlen(response), 0, (struct sockaddr *)&addr, addrlen);
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
                handleCommand(buffer, response);
                replyToTCP(response, client);
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
