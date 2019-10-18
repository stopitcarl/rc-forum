#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define BUFFER_SIZE_L 2048
#define MAX_PORT_SIZE 10
#define ID_SIZE 5
#define TOPIC_SIZE 10
#define QUESTION_SIZE 10
#define ANSWER_SIZE 10
#define EXTENSION_SIZE 3
#define DEFAULT_PORT 58044
#define MAX_TOPICS 99
#define MAX_ANSWERS 10
#define MAX_FILE_SIZE 9999999999
#define MAX_FILE_SIZE_LENGTH 10
#define TIMEOUT 10

/*  Turns a string into a number
    returns number or -1 in case of error */
long toPositiveNum(char *s);

/*  Returns number of digits of a positive number or -1 if negative */
long digits(long n);

/*  Turns a string into an ID if possible
    returns ID or -1 in case of error */
long getUserID(char *s);

/*  Null terminates the curent arg and returns a pointer to the next one
    l defines length of the argument
    Does basic error checks */
char *getNextArg(char *s, char c, long l);

/*  Replaces last occurence of new line character of string s with '\0'
    returns 0 in case of success */
int deleteNewLine(char *s);

/*  Replaces last occurence of new line character with c
    return 0 in case of success */
int replaceNewLine(char *s, char c);

/*  Checks if server response respects protocol
    return 1 in case of success */
int isValidResponse(char *s, long size);

/*  Checks if s is a valid ID
    return 1 in case of success */
int isValidID(char *s);

/*  Checks if s is a valid answer 
    return 1 in case of success */
int isValidAnswer(char *s);

/*  Checks if s is a valid question 
    return 1 in case of success */
int isValidQuestion(char *s);

/*  Checks if s is a valid topic name 
    return 1 in case of success */
int isValidTopic(char *s);

/*  Checks if a string is only composed of alphanumeric characters
    return 1 in case of success */
int isAlphaNumeric(char *s);

/*  Checks if string s ends with a new line character
    returns 1 in case of success */
int endsWithNewLine(char *s, long size);

/*  Copy a string untill a space is read */
void cpyUntilSpace(char *dest, char *src);

/*  Prints topics in list format */
void printTopicList(char **l, long size);

/*  Prints questions in list format */
void printQuestionList(char **l, long size);

/*  Checks if s is in list l
    return 1 in case of success */
int isInList(char *s, char **l, long size);

/* Allocate list l with n char* */
char **initList(int n);

/* Frees list */
void freeList(char **l, long size);

/*  Parses a Data Block consisting of "fSize fData IMG [iExt iSize iData]"
    fn is the name of the file where data is going to be stored in
    fn must not have extension
    returns a pointer to end of string or null in case of failure */
int parseDataBlock(int fd, char *fn);

/*  Handles the image portion of the Data Block "ext size data"
    return a pointer to end of portion or null in case of failure */
int handleFile(int fd, char *fn);

/*  Handles the file portion of the Data Block "size data"
    returns a pointer to end of portion or null in case of failure */
int handleImage(int fd, char *fn);

/*  Composes a Data Block consisting of "fSize fData IMG [iExt iSize iData]"
    fn must be .txt and in must include extension
    returns 0 in case of success */
int sendDataBlock(int fd, char *fn, int IMG, char *in);

/*  Sends data in file fn
    returns 0 in case of success */
int sendFile(int fd, char *fn);

/* Writes size amount of bytes in file pointed by fd */
int writeBytes(int fd, char *bytes, long size);

/*  Reads size - 1 amount of bytes from file pointed by fd
    Null terminates buffer
    Returns number of bytes read */
int readBytes(int fd, char *buffer, long size);

/*  Reads untill a space or size bytes are read
    replaces space with null terminator
    returns 0 in case of success */ 
int readUntillSpace(int fd, char *buffer, long size);
