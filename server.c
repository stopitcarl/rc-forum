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
#define TOPIC_LEN 16
#define QUESTION_LEN 19

int udp, tcp;
int nTopics;
fd_set rfds, afds;
struct addrinfo hints, *res;

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

void waitChild()
{
    pid_t pid;

    if ((pid = waitpid(-1, NULL, WNOHANG)) == -1)
        error("Error waiting for child");
}

/*Reads from the tcp socket*/
int readFromTCP(int src, char *buffer, int nbytes)
{
    fd_set fds;
    struct timeval tv = {0, 10};
    int n, nread, nleft = nbytes;

    FD_ZERO(&fds);
    FD_SET(src, &fds);

    if (nbytes > BUFFER_SIZE)
    {
        error("readFromTCP too big for buffer");
        return 0;
    }

    // Uses select to implement timeout
    do {
        n = select(FD_SETSIZE, &fds, NULL, NULL, &tv);
    } while (n == -1 && errno == EINTR);
    if (n == -1)
        error("Error on select");

    nread = 0;

    if (FD_ISSET(src, &fds)) {
        if (nbytes < 0) {
            while (1) {
                nread += read(src, buffer + nread, 1);

                if (nread == -1)
                    error("Error reading byte-by-byte from tcp socket");
                else if (nread == BUFFER_SIZE)
                    return -1;

                if (buffer[nread - 1] == ' ' || buffer[nread - 1] == '\n') {
                    nbytes = nread;
                    break;
                }
            }
        } else
            /* Makes sure the entire message is read */
            while (nleft > 0) {
                // Read nleft bytes
                nread = read(src, buffer + nbytes - nleft, nleft);
                nleft -= nread;
                if (nread == 0)
                    error("Broken pipe");

                if (nread == -1)
                    error("Error reading from tcp socket");
            }
    } else {
        // Timed out
        return 0;
    }


    write(1, buffer, nbytes);

    return nbytes;
}

/*Writes to tcp socket*/
int replyToTCP(char *msg, int dst)
{
    long n, nbytes, nwritten = 0;

    nbytes = strlen(msg);

    /*Makes sure the entire message is written*/
    while (nwritten < nbytes)
    {
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

int countQuestions(char *dirName)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Counts number questions in certain topic*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') == NULL &&
            strstr(entry->d_name, ".txt") != NULL)
        {
            count++;
        }
    }

    closedir(dir);

    return count;
}

/*
args: 'question' format question-id
returns: number
*/
int countAnswers(char *dirName, char *question)
{
    DIR *dir;
    struct dirent *entry;
    char holder[TOPIC_LEN];
    int count = 0;

    strcpy(holder, question);

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Counts number of answers to a certain question*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') != NULL)
        {
            if (strstr(entry->d_name, ".txt") != NULL &&
                !strcmp(strtok(entry->d_name, "_"), strtok(holder, "-")))
            {
                count++;
            }
        }
    }

    closedir(dir);

    return count;
}

int countTopics()
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (entry->d_name[0] == '.')
                continue;
            count++;
        }
    }

    closedir(dir);

    return count;
}

int findImage(char *dirName, char *question, char *imageName)
{
    DIR *dir;
    struct dirent *entry;
    char testFile[TOPIC_LEN + 4];

    /*Opens directory*/
    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    /*Cycles through directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            strcpy(testFile, entry->d_name);
            if (!strcmp(strtok(testFile, "-"), question))
            {
                if (strstr(entry->d_name, ".txt") == NULL)
                {
                    strcpy(imageName, entry->d_name);
                    closedir(dir);
                    return 1;
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

// args: filename(char*), client(int), data (char[] buffer), filesize(long)
int receiveAndWriteFile(char *filename, int client, char *buffer, long filesize)
{

    int bytesReaded = 0, ret;

    FILE *f = fopen(filename, "w");
    if (f == NULL)
    {
        error("ERROR: fopen");
        exit(EXIT_FAILURE);
    }

    do
    {
        // Read buffer
        bytesReaded = readFromTCP(client, buffer, filesize > BUFFER_SIZE ? BUFFER_SIZE : filesize);
        filesize -= bytesReaded;
        // Write to file
        if ((ret = fwrite(buffer, sizeof(char), bytesReaded, f)) < bytesReaded)
        {
            fclose(f);
            return -1;
        }
        // Repeat
    } while (filesize > 0);
    fclose(f);

    return 0;
}

// ######## END OF AUX FUNCTIONS ###########

// ######## START OF UDP COMMANDS ###########

/*Performs register command and saves the reply in status*/
void registerCommand(char *id, char *status)
{
    char *arg = getNextArg(id, ' ', -1);

    if (arg != NULL)
    {
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
void topicListCommand(char *response)
{
    DIR *dir;
    struct dirent *entry;
    char topics[99 * (TOPIC_LEN + 1)];
    char *fp;

    topics[0] = '\0';

    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (entry->d_name[0] == '.')
                continue;
            strcat(topics, " ");
            fp = strchr(entry->d_name, '-');
            if (fp != NULL)
                *fp = ':';
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
void findTopic(char *topic, char *dirName)
{
    DIR *dir;
    struct dirent *entry;
    char *name, aux[TOPIC_LEN];

    /*Opens topics directory*/
    if (!(dir = opendir("topics")))
        error("Error opening directory");

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            if (entry->d_name[0] == '.')
                continue;

            strcpy(aux, entry->d_name);
            name = strtok(aux, "-");
            if (!strcmp(topic, name))
            {
                strcpy(dirName, entry->d_name);
                closedir(dir);
                return;
            }
        }
    }

    closedir(dir);

    dirName[0] = '\0';
}

/*Searches a certain topic for a question*/
int findQuestion(char *topicDir, char *question, char *fileName)
{
    DIR *dir;
    struct dirent *entry;
    char testFile[TOPIC_LEN + 4];

    /*Opens directory*/
    if (!(dir = opendir(topicDir)))
        error("Error opening directory");

    /*Cycles through the topic's directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') == NULL)
        {
            strcpy(testFile, entry->d_name);
            if (!strcmp(strtok(testFile, "-"), question) &&
                strstr(entry->d_name, ".txt") != NULL)
            {
                if (fileName != NULL)
                    strcpy(fileName, entry->d_name);
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);

    return 0;
}

/*Searches a certain topic for an answer
returns: 0 if answer is found, 1 otherwise
*/
int findAnswerId(char *topicDir, char *answer, char *answerId)
{
    DIR *dir;
    struct dirent *entry;

    /*Opens directory*/
    if (!(dir = opendir(topicDir)))
        error("Error opening directory");

    /*Cycles through the topic's directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG && strchr(entry->d_name, '_') != NULL)
        {
            if (!strcmp(strtok(entry->d_name, "-"), answer))
            {
                strcpy(answerId, strtok(NULL, "."));
                closedir(dir);
                return 0;
            }
        }
    }

    closedir(dir);

    return 1;
}

/*Proposes topic to the forum*/
void topicProposeCommand(char *id, char *response)
{
    char *topic, *lastArg;
    char dirName[TOPIC_LEN + 8];
    char testTopic[TOPIC_LEN];
    int n;

    topic = getNextArg(id, ' ', -1);

    if (topic == NULL)
    {
        strcpy(response, "ERR\n");
        return;
    }

    lastArg = getNextArg(topic, ' ', -1);

    if (lastArg != NULL)
    {
        strcpy(response, "ERR\n");
        return;
    }

    findTopic(topic, testTopic);

    /*Checks if id and topic are correct*/
    if (!isValidID(id) || !isValidTopic(topic))
    {
        strcpy(response, "PTR NOK\n");
        return;
    }
    else if (nTopics == 99)
    {
        strcpy(response, "PTR FUL\n");
        return;
    }
    else if (testTopic[0] != '\0')
    {
        strcpy(response, "PTR DUP\n");
        return;
    }

    sprintf(dirName, "topics/%s-%s", topic, id);

    /*Creates topic in directory*/
    if ((n = mkdir(dirName, 0777)) == -1)
    {
        error("Error creating dir");
    }
    else
    {
        strcpy(response, "PTR OK\n");
        nTopics++;
    }
}

/*Lists questions in a certain topic*/
void questionListCommand(char *topic, char *response)
{
    DIR *dir;
    struct dirent *entry;
    char topicDir[TOPIC_LEN + 1];
    char dirName[TOPIC_LEN + 8];
    char questions[99 * (QUESTION_LEN + 1)];
    char *fp;
    int nQuestions;
    char *lastArg = getNextArg(topic, ' ', -1);

    if (lastArg != NULL)
    {
        strcpy(response, "ERR\n");
        return;
    }

    questions[0] = '\0';

    findTopic(topic, topicDir);

    if (topicDir[0] == '\0')
    {
        strcpy(response, "ERR\n");
        return;
    }

    sprintf(dirName, "topics/%s", topicDir);

    /*Opens directory*/

    if (!(dir = opendir(dirName)))
        error("Error opening directory");

    nQuestions = 0;

    /*Cycles through all topics in directory*/
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            if (strchr(entry->d_name, '_') == NULL &&
                strstr(entry->d_name, ".txt") != NULL)
            {
                int nAnswers = countAnswers(dirName, entry->d_name);
                char answerString[2];
                sprintf(answerString, "%d", nAnswers);
                strcat(questions, " ");
                fp = strchr(entry->d_name, '-');
                if (fp != NULL)
                    *fp = ':';
                strcat(questions, strtok(entry->d_name, "."));
                strcat(questions, ":");
                strcat(questions, answerString);
                nQuestions++;
            }
        }
    }

    closedir(dir);

    if (nQuestions == 0)
    {
        strcpy(response, "LQR 0\n");
    }
    else
    {
        sprintf(response, "LQR %d%s\n", nQuestions, questions);
    }
}

// ######## END OF UDP COMMANDS ###########

// ######## START OF TCP COMMANDS ###########
// Request:
// GQU topic question
//     |
// Response
// QGR qUserID  qsize  qdata  qIMG [qiext  qisize  qidata] N (AN aUserID asize adata aIMG [aiext aisize aidata])*
void questionGetCommand(int client)
{

    char dirName[TOPIC_LEN + 8];
    char topic[TOPIC_SIZE + 1];
    char testTopic[TOPIC_LEN + 1];
    char question[QUESTION_SIZE + 1];
    char testQuestion[TOPIC_LEN + 4];
    char questionFile[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char answer[TOPIC_LEN + 4];
    char answerID[ID_SIZE + 1];
    char answerFile[BUFFER_SIZE];
    char imageName[TOPIC_LEN + 4];
    char imageFile[TOPIC_LEN + 9 + TOPIC_LEN + 4];
    char numOfAnswers[5];
    char buffer[BUFFER_SIZE];
    char *id;
    char *AN;
    int IMG, nAnswers, N, bytesReaded;

    // Gets arguments from message

    // Read topic
    bytesReaded = readFromTCP(client, topic, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || topic[bytesReaded - 1] != ' ')
    {
        // Reads rest of messgae before replying
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QGR ERR\n", client);
        return;
    }
    topic[bytesReaded - 1] = '\0';

    // Read question
    bytesReaded = readFromTCP(client, question, -1);
    if (bytesReaded > QUESTION_SIZE + 1 || question[bytesReaded - 1] != '\n')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QGR ERR\n", client);
        return;
    }
    question[bytesReaded - 1] = '\0';

    testTopic[0] = '\0';
    testQuestion[0] = '\0';
    findTopic(topic, testTopic);
    sprintf(dirName, "topics/%s", testTopic);

    // Checks if question and topic are correct
    if (!isValidTopic(topic) || !isValidQuestion(question))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QGR ERR\n", client);
        return;
    }
    else if (testTopic[0] == '\0' || !findQuestion(dirName, question, testQuestion))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QGR EOF\n", client);
        return;
    }

    sprintf(questionFile, "%s/%s", dirName, testQuestion);

    // Gets id from question file
    id = testQuestion;
    while (*id != '-' && *id != '\0')
        id++;

    if (*id == '\0')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QGR EOF\n", client);
        return;
    }
    id++;
    id[5] = '\0';

    replyToTCP("QGR ", client);
    replyToTCP(id, client);
    replyToTCP(" ", client);

    // Gets corresponding image if it exists
    imageName[0] = '\0';
    IMG = findImage(dirName, question, imageName);
    sprintf(imageFile, "%s/%s", dirName, imageName);

    sendDataBlock(client, questionFile, IMG, imageFile);

    // Adds data to the response
    nAnswers = countAnswers(dirName, testQuestion);
    if (nAnswers >= 10)
        N = 10;
    else
        N = nAnswers;

    sprintf(numOfAnswers, " %d", N);
    replyToTCP(numOfAnswers, client);

    // Gets all the answers to question
    for (int i = nAnswers; i > nAnswers - MAX_ANSWERS && i > 0; i--)
    {

        sprintf(answer, "%s_%02d", question, i);

        findAnswerId(dirName, answer, answerID);

        sprintf(answerFile, "%s/%s-%s.txt", dirName, answer, answerID);

        imageName[0] = '\0';
        IMG = findImage(dirName, answer, imageName);
        sprintf(imageFile, "%s/%s", dirName, imageName);

        // Gets answer number from answer file
        AN = answer;
        while (*AN != '_' && *AN != '\0')
            AN++;

        if (*AN == '\0')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("QGR EOF\n", client);
            return;
        }

        AN++;
        AN[2] = '\0';

        // TODO: create buffer
        sprintf(buffer, " %s %s ", AN, answerID);
        replyToTCP(buffer, client);

        sendDataBlock(client, answerFile, IMG, imageFile);
    }

    replyToTCP("\n", client);
}

/*
Submits question to a certain topic
Format:
QUS  qUserID  topic  question  qsize  qdata  qIMG  [iext isize idata]\n
    |
./topics/'topic'-id/'question'-'id'.txt
*/
void questionSubmitCommand(int client)
{
    char id[ID_SIZE + 1];
    char topic[TOPIC_SIZE + 1];
    char question[QUESTION_SIZE + 1];
    char qsize[QUESTION_SIZE + 1];
    long filesize;
    char data[BUFFER_SIZE];
    char dirName[TOPIC_LEN + 8];
    char questionName[TOPIC_LEN + 8 + TOPIC_LEN + 5];
    char testTopic[TOPIC_LEN + 1];
    int n, nQuestions, bytesReaded, qIMG;

    /*Get arguments*/
    bytesReaded = readFromTCP(client, id, -1);
    if (bytesReaded > ID_SIZE + 1 || id[bytesReaded - 1] != ' ')
    {
        // Reads rest of message before replying
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }
    id[bytesReaded - 1] = '\0';

    // Reading topic
    bytesReaded = readFromTCP(client, topic, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || topic[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }
    topic[bytesReaded - 1] = '\0';

    //Reading question
    bytesReaded = readFromTCP(client, question, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || question[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }
    question[bytesReaded - 1] = '\0';

    // Reading qsize
    bytesReaded = readFromTCP(client, qsize, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || qsize[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }
    qsize[bytesReaded - 1] = '\0';
    if ((filesize = toPositiveNum(qsize)) == -1 || filesize > MAX_FILE_SIZE)
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }

    // sets testTopic to 'topic'-id
    findTopic(topic, testTopic);

    sprintf(dirName, "topics/%s", testTopic);
    // dirname = "./topics/$(topic)-id/"

    // Checks if id and topic are correct
    if (!isValidID(id) || !isValidTopic(topic) || *testTopic == '\0' ||
            !isValidQuestion(question))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }
    // Checks if question $(question) exists
    else if (findQuestion(dirName, question, NULL))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR DUP\n", client);
        return;
        // Checks if max number of questions has been reached
    }
    else if ((nQuestions = countQuestions(dirName)) == MAX_TOPICS)
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR FUL\n", client);
        return;
    }

    // Create question name = dir/name.txt
    sprintf(questionName, "%s/%s-%s.txt", dirName, question, id);

    /* Stores data in file */
    // args: filename(char*), client(int), data (char[] buffer), filesize(long)
    if (receiveAndWriteFile(questionName, client, data, filesize) != 0)
    {
        error("Error writing file to disk");
    }

    // Read qIMG flag
    bytesReaded = readFromTCP(client, data, 3);
    int shouldNewLine = (data[1] == '0');
    // Check if qIMG is surrounded by spaces
    if (data[0] != ' ' ||
        !(
            (data[bytesReaded - 1] == ' ' && !shouldNewLine) ||
            (data[bytesReaded - 1] == '\n' && shouldNewLine)))
    {
        replyToTCP("QUR NOK\n", client);
        return;
    }

    // Check if qIMG is 0 or 1
    data[0] = data[1]; // Shift...
    data[1] = '\0';    //  ... chars to left
    // Check if qIMG is 0 or 1
    if (!((qIMG = (int)toPositiveNum(data)) == 0 || qIMG == 1))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("QUR NOK\n", client);
        return;
    }

    // If qIMG is 1
    if (qIMG)
    {
        //format: iext isize idata
        char ext[EXTENSION_SIZE + 1];
        char isize[TOPIC_SIZE + 1];
        long picsize;

        // Read iext

        bytesReaded = readFromTCP(client, ext, EXTENSION_SIZE + 1);
        if (bytesReaded != (EXTENSION_SIZE + 1) || ext[bytesReaded - 1] != ' ')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("QUR NOK\n", client);
            return;
        }
        ext[bytesReaded - 1] = '\0';

        // Read isize
        bytesReaded = readFromTCP(client, isize, -1);
        if (bytesReaded > (TOPIC_SIZE + 1) || isize[bytesReaded - 1] != ' ')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("QUR NOK\n", client);
            return;
        }
        isize[bytesReaded - 1] = '\0';

        if ((picsize = toPositiveNum(isize)) == -1 || picsize > MAX_FILE_SIZE)
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("QUR NOK\n", client);
            return;
        }

        // Create question image name = dir/name.ext
        sprintf(questionName, "%s/%s-%s.%s", dirName, question, id, ext);
        if (receiveAndWriteFile(questionName, client, data, picsize) != 0)
        {
            error("Error writing file to disk");
        }
    }
    else
    {

        // bytesReaded = readFromTCP(client, data, 1);
        if (data[2] != '\n')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("QUR NOK\n", client);
            return;
        }
    }

    // Answer client OK
    replyToTCP("QUR OK\n", client);
}

/*Submits answer to a certain question*/
/*ANS aUserID topic  question  asize  adata  aIMG  [iext isize idata]*/
void answerSubmitCommand(int client)
{
    char id[ID_SIZE + 1];
    char topic[TOPIC_SIZE + 1];
    char question[QUESTION_SIZE + 1];
    char asize[QUESTION_SIZE + 1];
    long filesize;
    char data[BUFFER_SIZE];
    char dirName[TOPIC_LEN + 8];
    char answerName[TOPIC_LEN + 8 + TOPIC_LEN + 5 + 3];
    char testTopic[TOPIC_LEN + 1];
    char testQuestion[TOPIC_LEN + 1];
    int nAnswers, bytesReaded, aIMG;

    /*Get arguments*/
    bytesReaded = readFromTCP(client, id, -1);
    if (bytesReaded > ID_SIZE + 1 || id[bytesReaded - 1] != ' ')
    {
        // Reads rest of message before replying
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    id[bytesReaded - 1] = '\0';

    // Reading topic
    bytesReaded = readFromTCP(client, topic, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || topic[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    topic[bytesReaded - 1] = '\0';

    //Reading question
    bytesReaded = readFromTCP(client, question, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || question[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    question[bytesReaded - 1] = '\0';

    // Reading asize
    bytesReaded = readFromTCP(client, asize, -1);
    if (bytesReaded > TOPIC_SIZE + 1 || asize[bytesReaded - 1] != ' ')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    asize[bytesReaded - 1] = '\0';
    if ((filesize = toPositiveNum(asize)) == -1 || filesize > MAX_FILE_SIZE)
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }

    // sets testTopic to 'topic'-id
    findTopic(topic, testTopic);

    sprintf(dirName, "topics/%s", testTopic);
    // dirname = "./topics/$(topic)-id/"

    findQuestion(dirName, question, testQuestion);

    // Checks if id and topic are correct
    if (!isValidID(id) || !isValidTopic(topic) || *testTopic == '\0' ||
        !isValidQuestion(question) || *testQuestion == '\0')
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    // Checks if max number of answers has been reached
    else if ((nAnswers = countAnswers(dirName, testQuestion)) == MAX_TOPICS)
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR FUL\n", client);
        return;
    }

    // Create question name = dir/name.txt
    sprintf(answerName, "%s/%s_%02d-%s.txt", dirName, question, ++nAnswers, id);

    /* Stores data in file */
    if (receiveAndWriteFile(answerName, client, data, filesize) != 0)
    {
        error("Error writing file to disk");
    }

    // Read aIMG flag
    bytesReaded = readFromTCP(client, data, 3);
    int shouldNewLine = (data[1] == '0');
    // Check if aIMG is surrounded by spaces
    if (data[0] != ' ' ||
        !(
            (data[bytesReaded - 1] == ' ' && !shouldNewLine) ||
            (data[bytesReaded - 1] == '\n' && shouldNewLine)))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }
    // Check if aIMG is 0 or 1
    data[0] = data[1]; // Shift...
    data[1] = '\0';    //  ... chars to left
    // Check if aIMG is 0 or 1
    if (!((aIMG = (int)toPositiveNum(data)) == 0 || aIMG == 1))
    {
        do n = readFromTCP(client, question, -1); while (n != 0);
        replyToTCP("ANR NOK\n", client);
        return;
    }

    // If aIMG is 1
    if (aIMG)
    {
        //format: iext isize idata
        char ext[EXTENSION_SIZE + 1];
        char isize[TOPIC_SIZE + 1];
        long picsize;

        // Read iext
        bytesReaded = readFromTCP(client, ext, EXTENSION_SIZE + 1);
        if (bytesReaded != (EXTENSION_SIZE + 1) || ext[bytesReaded - 1] != ' ')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("ANR NOK\n", client);
            return;
        }
        ext[bytesReaded - 1] = '\0';

        // Read isize
        bytesReaded = readFromTCP(client, isize, -1);
        if (bytesReaded > (TOPIC_SIZE + 1) || isize[bytesReaded - 1] != ' ')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("ANR NOK\n", client);
            return;
        }
        isize[bytesReaded - 1] = '\0';
        if ((picsize = toPositiveNum(isize)) == -1 || picsize > MAX_FILE_SIZE)
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("ANR NOK\n", client);
            return;
        }


        // Create question image name = dir/name.ext
        sprintf(answerName, "%s/%s_%02d-%s.%s", dirName, question, nAnswers, id, ext);
        if (receiveAndWriteFile(answerName, client, data, picsize) != 0)
        {
            error("Error writing file to disk");
        }
    }
    else
    {
        if (data[2] != '\n')
        {
            do n = readFromTCP(client, question, -1); while (n != 0);
            replyToTCP("ANR NOK\n", client);
            return;
        }
    }

    // Answer client OK
    replyToTCP("ANR OK\n", client);
}

// ######## END OF TCP COMMANDS ###########

void handleTCPCommand(int client, char *command)
{
    //   if (!strcmp(readBuffer, "GQU")) {
    //      questionGetCommand(readBuffer, client);
    //   }
    /*   else*/ if (!strcmp(command, "QUS"))
    {
        questionSubmitCommand(client);
    }
    else if (!strcmp(command, "ANS"))
    {
        answerSubmitCommand(client);
    }
    else if (!strcmp(command, "GQU"))
    {
        questionGetCommand(client);
    }
    else
    {
        replyToTCP("ERR\n", client);
    }
}

void handleUDPCommand(char *request, char *response)
{
    char *arg;

    if (deleteNewLine(request))
    {
        strcpy(response, "ERR\n");
        return;
    }

    /* Gets command name */
    arg = getNextArg(request, ' ', -1);

    /* Checks for command type */
    /* <udp> */
    if (!strcmp(request, "REG"))
    {
        registerCommand(arg, response);
    }
    else if (!strcmp(request, "LTP"))
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
    else
    {
        strcpy(response, "ERR\n");
    }
}

int main()
{
    struct sigaction pipe, intr, child;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char udpResponse[BUFFER_SIZE_L];
    char udpBuffer[BUFFER_SIZE_L];
    char tcpBuffer[BUFFER_SIZE];
    int n, client;
    long transferBytes;
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

        do
        {
            n = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        } while (n == -1 && errno == EINTR);
        if (n == -1)
            error("Error on select");

        else if (FD_ISSET(udp, &rfds))
        {
            //Got message from udp server
            memset(udpBuffer, 0, BUFFER_SIZE);
            addrlen = sizeof(addr);
            transferBytes = recvfrom(udp, udpBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&addr, &addrlen);
            if (transferBytes == -1)
                error("Error receiving from udp socket");

            // Update client
            client = udp;

            printf("UDP command from %s:%u\n", inet_ntoa(addr.sin_addr), addr.sin_port);
            write(1, udpBuffer, transferBytes);

            *(udpBuffer + transferBytes) = '\0';
            handleUDPCommand(udpBuffer, udpResponse);
            transferBytes = sendto(client, udpResponse, strlen(udpResponse), 0, (struct sockaddr *)&addr, addrlen);

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
            else if (pid == 0)
            {
                close(tcp);

                printf("TCP command from %s:%u\n", inet_ntoa(addr.sin_addr), addr.sin_port);

                if ((transferBytes = readFromTCP(client, tcpBuffer, -1)) != 4)
                    error("Couldn\'t read initial 3 bytes");
                tcpBuffer[3] = '\0';
                handleTCPCommand(client, tcpBuffer);
                close(client);
                freeaddrinfo(res);
                exit(EXIT_SUCCESS);
            }

            do
            {
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
