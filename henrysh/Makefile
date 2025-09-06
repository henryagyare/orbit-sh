CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
TARGET = henrysh
SRC = src/main.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean