MAIN := main.c

ASSIGNMENT := assembler.c

CFLAGS := 

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor gcc is available. Exiting.)
endif

SOURCES := $(MAIN) $(ASSIGNMENT)
TARGET := $(basename $(ASSIGNMENT))

all: debug

debug: $(TARGET)

release: CFLAGS += -O2
release: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET)

.PHONY: all debug release clean