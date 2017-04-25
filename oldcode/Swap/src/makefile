CC=gcc
TARGET_PATH="../"
TARGET="$(TARGET_PATH)Swap"
HEADERS=$(wildcard *.h)
OBJECTS=$(patsubst %.c, build/%.o, $(wildcard *.c))
LIBS=-lmllibs -lcommons -lparser-ansisop

all: $(TARGET)
default: all

create-dirs:
	mkdir -p build

build/%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@

$(TARGET): create-dirs $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	rm -rf build
	rm -f $(TARGET)

.PHONY: default all clean
