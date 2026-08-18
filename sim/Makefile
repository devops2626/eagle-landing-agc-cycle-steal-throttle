# Makefile – AGC Cycle-Steal Throttle skeleton
# Strict C11, no dynamic allocation required by the core.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude
LDFLAGS =

SRCS    = src/agc.c src/main.c
OBJS    = $(SRCS:.c=.o)
TARGET  = agc_sim

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c include/agc.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
