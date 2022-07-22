CC=gcc
CFLAGS=-Wall
LDFLAGS=-lpthread

SRC=server.o server-helper.o 
TARGET=server

all: $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(CFLAGS) $(LDFLAGS) -lm

clean:
	rm -f $(TARGET) *.o
