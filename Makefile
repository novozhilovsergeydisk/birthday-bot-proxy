CC = gcc
# Default flags for Linux (Debian)
CFLAGS = -Wall -O2 `pkg-config --cflags libpq libcurl 2>/dev/null || echo "-I/usr/include/postgresql"`
LDFLAGS = `pkg-config --libs libpq libcurl 2>/dev/null || echo "-lpq -lcurl"`

TARGET = birthday_bot
SRC = birthday_bot.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
