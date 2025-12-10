CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c11 -D_GNU_SOURCE
LDFLAGS = -pthread
TARGET = psx
SOURCES = psx.c process_table.c message_queue.c memory_allocator.c \
          proc_reader.c stats.c logger.c scheduler.c supervisor.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = common.h process_table.h message_queue.h memory_allocator.h \
          proc_reader.h stats.h logger.h scheduler.h supervisor.h

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f psx_log.txt psx_stats.log

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)

uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)

