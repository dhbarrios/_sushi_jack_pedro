CC=gcc
TARGET_PATH="$(shell pwd)/../"
TARGET="$(TARGET_PATH)Consola"
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

#Defino el entorno necesario para que corra la consola
env:
	ln -sf $(TARGET) /usr/bin
	echo 'export CONF_CONSOLA="$(TARGET_PATH)consola.conf"' > /etc/profile.d/consola.sh

install: all env

clean:
	rm -rf build
	rm -f $(TARGET)

.PHONY: default all clean env install
