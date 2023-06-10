CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -pthread -std=c11
LIBS=-lproactor

all: chat

chat: chat.o libproactor.a
	$(CC) $(CFLAGS) -o chat chat.o -L. $(LIBS)

chat.o: chat.c chat.h
	$(CC) $(CFLAGS) -c chat.c

libproactor.a: proactor.o
	ar rcs libproactor.a proactor.o

proactor.o: proactor.c proactor.h
	$(CC) $(CFLAGS) -c proactor.c

clean:
	rm -f chat chat.o proactor.o libproactor.a