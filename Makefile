CC = gcc

CFLAGS = -Wall -Wextra

all: processflow

processflow: processflow.c
	$(CC) $(CFLAGS) -o processflow processflow.c

clean:
	rm -f processflow