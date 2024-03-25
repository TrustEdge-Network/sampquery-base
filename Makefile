HEADERS=$(wildcard ./src/*.h)
C_SOURCE=$(wildcard ./src/*.c)
OBJECTS:=$(subst .c,.o,$(subst src,bin,$(C_SOURCE)))
CFLAGS:=-c -Wall -pthread
LDFLAGS:=-pthread
LD:=gcc
CC:=gcc
TARGET:=out.exe

all: clean build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ -o bin/$@

./bin/%.o: ./src/%.c
	$(CC) $< $(CFLAGS) -o $@

.PHONY:clean
clean:
	rm -rf bin/*.o