CC = clang
CFLAGS = -D NDEBUG -Ofast -Wall -Werror -target wasm32-wasm
LD = wasm-ld
LDFLAGS = --allow-undefined --export=compress --export=decompress --no-entry --strip-debug
TARGET_BINARY = rangecoder.wasm
TARGET_SOURCES = compress.c decompress.c rangecoder.c
TARGET_OBJECTS = $(TARGET_SOURCES:%.c=%.o)

all: $(TARGET_BINARY)

clean:
	$(RM) alice29.txt.rc decompressed.txt $(TARGET_BINARY) $(TARGET_OBJECTS)

disasm: $(TARGET_BINARY)
	@llvm-objdump --disassemble-all $(TARGET_BINARY) | less

test: $(TARGET_BINARY)
	@npm run example
	@cmp decompressed.txt alice29.txt > /dev/null || { echo "\033[31mFailure\033[m" && exit 1; }
	@echo "\033[32mSuccess\033[m"

.PHONY: clean disasm test

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGET_BINARY): $(TARGET_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $(TARGET_OBJECTS)
