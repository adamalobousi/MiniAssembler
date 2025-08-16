MAIN := src/main.c
ASSIGNMENT := src/assembler.c
INCLUDE := include

ASSIGNMENT := src/assembler.c

CFLAGS := -I$(INCLUDE)

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor gcc is available. Exiting.)
endif

SOURCES := $(MAIN) $(ASSIGNMENT)
TARGET := bin/assembler

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET)
	
.PHONY: all debug release clean