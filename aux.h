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

#define BUFFER_SIZE 1024
#define MAX_PORT_SIZE 10
#define ID_SIZE 5
#define TOPIC_SIZE 10
#define QUESTION_SIZE 10
#define ANSWER_SIZE 10
#define DEFAULT_PORT 58044

/*  Turns a string into a number
    returns number or -1 in case of error */
long toPositiveNum(char *s);

/*  Turns a string into an ID if possible
    returns ID or -1 in case of error */
long getUserID(char *s);

/*  Null terminates the curent arg and returns a pointer to the next one;
    l defines length of the argument;
    Does basic error checks; */
char *getNextArg(char *s, char c, long l);

/*  Replaces last occurence of new line character of string s with '\0'
    returns 0 in case of success */
int deleteNewLine(char *s);

/*  Checks if server response respects protocol
    return 1 in case of success */
int isValidResponse(char *s, long size);

/*  Checks if s is a valid ID
    return 1 in case of success */
int isValidID(char *s);

/*  Checks if s is a valid topic name 
    return 1 in case of success */
int isValidTopic(char *s);

/*  Checks if string s ends with a new line character
    returns 1 in case of success */
int endsWithNewLine(char *s, long size);