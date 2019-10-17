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

long digits(long n) {

    int d = 1;

    if (n < 0) {
        return -1;
    }
    while (n > 9) {
        n /= 10;
        d++;
    }
    return d;

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

    if (s == NULL || size == 1 || !endsWithNewLine(s, size)) {
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
    char *c;

    for (i = 0; i < size; i++) {
        if ((c = strchr(l[i], ' ')) == NULL) {
            return 0;
        }
        if (strncmp(s, l[i], c-l[i]) == 0) {
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

int parseDataBlock(int fd, char *fn) {

    char fileName[TOPIC_SIZE + QUESTION_SIZE + 6], buffer[BUFFER_SIZE];
    int IMG;

    /* Handles "fSize fData" */
    sprintf(fileName, "%s.txt", fn);
    if (handleFile(fd, fileName)) {
        return 1;
    }

    /* Gets " IMG" */
    if (readBytes(fd, buffer, 3) == 0) {
        return 1;
    }
    if (buffer[0] != ' ') {
        return 1;
    }
    if ((IMG = (int) toPositiveNum(buffer + 1)) == -1) {
        return 1;
    }

    /* Checks if IMG is a binary flag */
    if (IMG < 0 || IMG > 1) {
        return 1;
    }

    /* There is an image */ 
    if (IMG) {

        /* Gets a space */
        if (readBytes(fd, buffer, 2) == 0 && buffer[0] != ' ') {
            return 1;
        }

        /* Handles "iExt iSize iData" */
        if (handleImage(fd, fn)) {
            return 1;
        }
    }

    return 0;

}

int handleImage(int fd, char *fn) {

    char buffer[BUFFER_SIZE + 1], imageName[strlen(fn) + EXTENSION_SIZE + 2];

    /* Gets image extension by reading one byte at a time */
    if (readUntillSpace(fd, buffer, EXTENSION_SIZE + 1)) {
        return 1;
    }
    
    /* Checks if extension is valid */
    if (!strcmp(buffer, "txt") || strlen(buffer) != EXTENSION_SIZE) {
        return 1;
    }

    sprintf(imageName, "%s.%s", fn, buffer);

    /* Handles "size data" */
    if (handleFile(fd, imageName)) {
        return 1;
    }

    return 0;

}

int handleFile(int fd, char *fn) {

    char buffer[BUFFER_SIZE + 1];
    int read = 0, blocks, rest;
    long size;

    /* Gets size of data */
    if (readUntillSpace(fd, buffer, MAX_FILE_SIZE_LENGTH + 1)) {
        return 1;
    }
    if ((size = toPositiveNum(buffer)) == -1 || size > MAX_FILE_SIZE) {
        return 1;
    }

    /* Opens file */
    FILE *f = fopen(fn, "w");
    if (f == NULL) {
        perror("ERROR: fopen\n");
        exit(EXIT_FAILURE);
    }

    /* Determines how many blocks we need */
    blocks = size / BUFFER_SIZE;
    for (; blocks > 0; blocks--) {
        read = readBytes(fd, buffer, BUFFER_SIZE + 1);
        if (read < BUFFER_SIZE) {
            return 1;
        }
        fwrite(buffer, sizeof(char), BUFFER_SIZE, f);
    }

    /* Stores the rest of the data */
    rest = size % BUFFER_SIZE;
    read = readBytes(fd, buffer, rest + 1);
    if (read < rest) {
        return 1;
    }
    fwrite(buffer, sizeof(char), rest, f);
    fclose(f);

    return 0;

}

int sendDataBlock(int fd, char *fn, int IMG, char *in) {

    char *iExt, buffer[BUFFER_SIZE];

    /* Sends size and data from file */
    if (sendFile(fd, fn)) {
        return 1;
    }

    if (IMG) {

        if (in == NULL) {
            printf("ERROR: image name not defined\n");
            return 1;
        }

        /* Writes IMG */
        sprintf(buffer, " %d ", IMG);
        writeBytes(fd, buffer, strlen(buffer));

        /* Gets image ext */
        if ((iExt = strrchr(in, '.')) == NULL) {
            printf("Image name must contain extension\n");
            return 1;
        }
        iExt++;
        if (!strcmp(iExt, "txt")) {
            printf("Image extension must be different from txt\n");
            return 1;
        }
        if (strlen(iExt) != EXTENSION_SIZE) {
            printf("Extension must be %d bytes long\n", EXTENSION_SIZE);
            return 1;
        }

        /* Writes extension */
        writeBytes(fd, iExt, strlen(iExt));

        /* Writes a space */
        sprintf(buffer, " ");
        writeBytes(fd, buffer, strlen(buffer));

        /* Sends size and data from image */
        if (sendFile(fd, in)) {
            return 1;
        }

    }
    else {     

        /* Writes IMG */
        sprintf(buffer, " %d", IMG);
        writeBytes(fd, buffer, strlen(buffer));

    }

    return 0;

}

int sendFile(int fd, char *fn) {

    char buffer[BUFFER_SIZE];
    long size, read;

    /* Opens file */
    FILE *f = fopen(fn, "r");
    if (f == NULL) {
        printf("No such file\n");
        return 1;
    }

    /* Gets size of file */
    if (fseek(f, 0, SEEK_END) == -1) {
        perror("ERROR: fseek\n");
        exit(EXIT_FAILURE);
    }
    if ((size = ftell(f)) == -1) {
        perror("ERROR: ftell\n");
        exit(EXIT_FAILURE);
    }
    if (size > MAX_FILE_SIZE) {
        printf("File size is greater than max allowed size\n");
        return 1;
    }

    /* Sends file size trough socket */
    sprintf(buffer, "%ld ", size);
    writeBytes(fd, buffer, strlen(buffer));

    if (fseek(f, 0, SEEK_SET) == -1) {
        perror("ERROR: fseek\n");
        exit(EXIT_FAILURE);
    }

    /* Sends data trough socket */
    while ((read = fread(buffer, sizeof(char), BUFFER_SIZE, f)) > 0) {
        writeBytes(fd, buffer, read);
    }
    fclose(f);

    return 0;

}

void writeBytes(int fd, char *bytes, long size) {

    long written = 0, n;

    printf("Sending %ld bytes and content: \"", size);
    fflush(stdout);
    write(1, bytes, size);
    printf("\"\n");
    fflush(stdout);

    /* Sends bytes */
    while (written < size) {
        n = write(fd, bytes + written, size - written);
        if (n == -1) {
            perror("ERROR: writing bytes\n");
            exit(EXIT_FAILURE);
        }
        written += n;
    }

}

int readBytes(int fd, char *buffer, long size) {

    long n, left = size;

    while ((n = read(fd, buffer + size - left, left - 1)) > 0) {
        left -=n;
    }
    if (n == -1) {
        perror("ERROR: reading bytes\n");
        exit(EXIT_FAILURE);
    }

    /* Null terminates buffer */
    *(buffer + size - left) = '\0';


    printf("Read %ld bytes and content: \"", size - left);
    fflush(stdout);
    write(1, buffer, size - left);
    printf("\"\n");
    fflush(stdout);

    return size - left;

}

int readUntillSpace(int fd, char *buffer, long size) {

    char c[2];
    long read = 0;

    do {
        if (read >= size) {
            return 1;
        }
        if (readBytes(fd, c, 2) == 0) {
            return 1;
        }
        buffer[read] = c[0];
        read++;
    } while (c[0] != ' ');

    buffer[read - 1] = '\0';

    printf("Read %ld bytes and content: \"%s\"\n", strlen(buffer), buffer);
    fflush(stdout);

    return 0;

}
