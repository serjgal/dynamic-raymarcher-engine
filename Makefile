CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include
LDFLAGS = 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	# macOS: Point to the Frameworks folder for headers
	CFLAGS += -F/Library/Frameworks
	# macOS: Link the frameworks AND set the runtime path so dyld finds SDL3
	LDFLAGS += -F/Library/Frameworks -framework SDL3 -framework OpenGL -Wl,-rpath,/Library/Frameworks
else
	# Linux/Windows: Link standard Unix libraries
	LDFLAGS += -lSDL3 -lGL -lm
endif

SRC = src/main.c
OBJ = $(SRC:.c=.o)
EXEC = raymarcher

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)