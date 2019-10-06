#include "auxiliary.h"

long toPositiveNum(char *s) {

    long n;
    char *e;

    /* for some reason strtol ignores white spaces in the beginning of the string */
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

int replaceNewLine(char *s, char c) {

    char *p;

    p = strrchr(s, '\n');
    if (p == NULL) {
        return 1;
    }
    else {
        *p = c;
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
    else {
        return 0;
    }
    
}

int isValidAnswer(char *s) {

    if (strlen(s) <= ANSWER_SIZE && strlen(s) > 0 && isAlphaNumeric(s)) {
        return 1;
    }
    else {
        return 0;
    }
    
}

int isValidQuestion(char *s) {

    if (strlen(s) <= QUESTION_SIZE && strlen(s) > 0 && isAlphaNumeric(s)) {
        return 1;
    }
    else {
        return 0;
    }
    
}

int isValidTopic(char *s) {

    if (strlen(s) <= TOPIC_SIZE && strlen(s) > 0 && isAlphaNumeric(s)) {
        return 1;
    }
    else {
        return 0;
    }

}

int isAlphaNumeric(char *s) {

    int i = 0;

    while (s[i] != '\0') {
        if (!isalnum(s[i])) {
            return 0;
        }
        i++;
    }
    
    return 1;

}

int endsWithNewLine(char *s, long size) {

    if (s[size - 2] == '\n') {
        return 1;
    }
    else {
        return 0;
    }

}

void printTopicList(char **l, long size) {

    int i;

    printf("Topic List (Number Name UserID):\n");
    for (i = 0; i < size; i++) {
        printf("Topic %d. %s\n", i + 1, l[i]);
    }

}

void printQuestionList(char **l, long size) {

    int i;

    printf("Question List (Number Name UserID):\n");
    for (i = 0; i < size; i++) {
        printf("Question %d. %s\n", i + 1, l[i]);
    }
}

int isInList(char *s, char **l, long size) {

    int i;

    for (i = 0; i < size; i++) {
        if (strncmp(s, l[i], strlen(s)) == 0) {
            return 1;
        }
    }

    return 0;

}

void cpyUntilSpace(char *dest, char *src) {

    while (*src != ' ' && *src != '\0') {
        *dest++ = *src++;
    }
    *dest = '\0';

}

char **initList(int n) {

    char **l;

    l = (char**) malloc(sizeof(char*) * n);
    if (l == NULL) {
        perror("ERROR: malloc\n");
        exit(EXIT_FAILURE);
    }

    return l;

}

void freeList(char **l, long size) {

    int i;

    for (i = 0; i < size; i++) {
        free(l[i]);
    }

    free(l);

}
