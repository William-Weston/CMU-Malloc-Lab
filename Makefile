# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11 -O2 -fsanitize=undefined

# Target executables
IMPLICIT = implicit-list
EXPLICIT = explicit-list
SEG = seg-list

# Source files for each executable
IMPLICIT_SRCS = memlib.c std_wrappers.c mm.c mmtester.c
EXPLICIT_SRCS = memlib.c std_wrappers.c explicit_list.c explicit_tests.c
SEG_SRCS = memlib.c std_wrappers.c seg_list.c seg_tests.c

# Object files for each executable
IMPLICIT_OBJS = $(IMPLICIT_SRCS:.c=.o)
EXPLICIT_OBJS = $(EXPLICIT_SRCS:.c=.o)
SEG_OBJS = $(SEG_SRCS:.c=.o)

# Default target
all: $(IMPLICIT) $(EXPLICIT) $(SEG)

# Linking for implicit-list
$(IMPLICIT): $(IMPLICIT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Linking for explicit-list
$(EXPLICIT): $(EXPLICIT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Linking for seg-list
$(SEG): $(SEG_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation rule for object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(IMPLICIT_OBJS) $(EXPLICIT_OBJS) $(SEG_OBJS) $(IMPLICIT) $(EXPLICIT) $(SEG)

.PHONY: all clean