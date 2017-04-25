RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# Clean and compile .so
all: build/libmllibs.so

create-dirs:
	mkdir -p build/sockets
	mkdir -p build/log
	mkdir -p build/umc
	mkdir -p build/nucleoCpu
	mkdir -p build/stack
	mkdir -p build/string

build/libmllibs.so: create-dirs $(OBJS)
	$(CC) -shared -o "build/libmllibs.so" $(OBJS)

build/sockets/%.o: sockets/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
build/log/%.o: log/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/umc/%.o: umc/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
build/nucleoCpu/%.o: nucleoCpu/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	
build/stack/%.o: stack/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/string/%.o: string/%.c
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install: all
	mkdir -p /usr/include/mllibs
	cp -u build/libmllibs.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include/mllibs

uninstall:
	rm -f /usr/lib/libmllibs.so
	rm -rf /usr/include/mllibs

.PHONY: all create-dirs clean install uninstall
