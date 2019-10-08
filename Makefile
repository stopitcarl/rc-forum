SOURCES = server.c user.c auxiliary.c
OBJS = $(SOURCES:%.c=%.o)
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lm
TARGETS = server user

all: $(TARGETS)

server: server.o auxiliary.o
user: user.o auxiliary.o

$(TARGETS):
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	
server.o: server.c auxiliary.h
user.o: user.c auxiliary.h
auxiliary.o: auxiliary.c

$(OBJS):
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo Cleaning...
	rm -f $(OBJS) $(TARGETS)
