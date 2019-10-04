#include "aux.h"

long toPositiveNum(char *s) {

    long n;
    char *e;

    /* for some reason strtol ignores white spaces beggining of the string */
    if (isspace(s[0])) {
        return -1;
    }
    n = strtol(s, &e, 10);
    if (strlen(s) == 0 || *e != '\0' || n < 0) {
        return -1;
    }
    else {
        return n;
    }

}

long getUserID(char *s) {

    if (strlen(s) == ID_SIZE) {
        return toPositiveNum(s);
    }
    else {
        return -1;
    }

}

char *getNextArg(char *s, char c, long l) {

    char *p;

    p = strchr(s, c);

    /* tests for end of string */ 
    if (p == NULL) {
        return NULL;
    }
    *p = '\0';


    /* tests for empty string and if l is defined tests for length */
    if (strlen(s) == 0 || (l > 0 && p - s != l)) {
        return NULL;
    }
        
    return p + 1;

}

int deleteNewLine(char *s) {

    char *p;

    p = strrchr(s, '\n');
    if (p == NULL) {
        return 1;
    }
    else {
        *p = '\0';
        return 0;
    }

}

int isValidResponse(char *s, long size) {

    if (size == 0 || !endsWithNewLine(s, size)) {
        return 0;
    }
    else {
        return 1;
    } 

}

int isValidID(char *s) {

    if (strlen(s) == ID_SIZE && toPositiveNum(s) > 0) {
        return 1;
    }
    else
    {
        return 0;
    }
    

}

int isValidAnswer(char *s) {

    if (strlen(s) <= ANSWER_SIZE && strlen(s) > 0 && strchr(s, ' ') == NULL) {
        return 1;
    }
    else
    {
        return 0;
    }
    
}

int isValidQuestion(char *s) {

    if (strlen(s) <= QUESTION_SIZE && strlen(s) > 0 && strchr(s, ' ') == NULL) {
        return 1;
    }
    else
    {
        return 0;
    }
    
}

int isValidTopic(char *s) {

    if (strlen(s) <= TOPIC_SIZE && strlen(s) > 0 && strchr(s, ' ') == NULL && strchr(s, '\n') == NULL) {
        return 1;
    }
    else
    {
        return 0;
    }
    
}

int endsWithNewLine(char *s, long size) {

    if (s[size - 1] == '\n') {
        return 1;
    }
    else {
        return 0;
    }

}