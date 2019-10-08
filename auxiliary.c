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

char* handleFile(char *content, int cSize, char *fn) {

    char *nextArg;
    long size;

    /* Gets size of data */
    if ((nextArg = getNextArg(content, ' ', -1)) == NULL) {
        return NULL;
    }
    if ((size = toPositiveNum(content)) == -1 || size > MAX_FILE_SIZE) {
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

char* createDataBlock(char *fn, int IMG, char *in, long *size) {

    char *fData, *iData, *iExt, *dataBlock, *p;
    long fSize, iSize;
    int fsDigits, isDigits;

    /* Reads from text file */
    if ((fData = readFromFile(fn, &fSize)) == NULL) {
        return NULL;
    }
    fsDigits = floor(log10(abs(fSize))) + 1;

    if (IMG) {

        if (in == NULL) {
            printf("CALL ERROR\n");
            return NULL;
        }

        /* Gets image ext */
        if ((iExt = strrchr(in, '.')) == NULL) {
            printf("Image name must contain extension\n");
            return NULL;
        }
        iExt++;
        if (strlen(iExt) != EXTENSION_SIZE) {
            printf("Extension must be %d bytes long\n", EXTENSION_SIZE);
            return NULL;
        }

        /* Reads from image */
        if ((iData = readFromFile(in, &iSize)) == NULL) {
            return NULL;
        }

        /* Allocates and builds Data Block */
        isDigits = floor(log10(abs(iSize))) + 1;
        *size = fsDigits + fSize + 1 + EXTENSION_SIZE + isDigits + iSize + 5;
        dataBlock = (char*) malloc(sizeof(char) * (*size));
        if (dataBlock == NULL) {
            perror("ERROR: malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(dataBlock, "%ld ", fSize);
        p = strchr(dataBlock, ' ');
        p++;
        memcpy(p, fData, fSize);
        p += fSize;
        sprintf(p, " %d %s %ld ", IMG, iExt, iSize);
        p = strrchr(p, ' ');
        p++;
        memcpy(p, iData, iSize);

        free(iData);

    }
    else {

        if (in != NULL) {
            printf("CALL ERROR\n");
            return NULL;
        }        

        /* Allocates and builds Data Block */
        *size = fsDigits + fSize + 1 + 2;
        dataBlock = (char*) malloc(sizeof(char) * (*size));
        if (dataBlock == NULL) {
            perror("ERROR: malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(dataBlock, "%ld ", fSize);
        p = strchr(dataBlock, ' ');
        p++;
        memcpy(p, fData, fSize);
        p += fSize;
        memcpy(p, " 0", 2);

    }

    free(fData);

    return dataBlock;

}

char* readFromFile(char *fn, long *size) {

    char *data;

    /* Opens file */
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        printf("No such file\n");
        return NULL;
    }

    /* Gets size of file */
    fseek(f, 0, SEEK_END);
    if ((*size = ftell(f)) > MAX_FILE_SIZE) {
        printf("File size is greater than %ld", MAX_FILE_SIZE);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    /* Allocs memory for file data and stores it */
    data = (char*) malloc(*size * sizeof(char));
    if (data == NULL) {
        perror("ERROR: malloc\n");
        exit(EXIT_FAILURE);
    }
    fread(data, sizeof(char), *size, f);
    fclose(f);

    return data;

}
