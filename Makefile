CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = paragraph_threads

all: $(TARGET)

$(TARGET): paragraph_threads.c
	$(CC) $(CFLAGS) -o $(TARGET) paragraph_threads.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)