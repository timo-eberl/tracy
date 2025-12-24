CC = clang

CLI_TARGET = tracy

CFLAGS = -std=c11 -Wall -Wextra -Iinclude -Idependencies/pcg-c/include -O3 -march=native -flto

.PHONY: all clean run

all: $(CLI_TARGET)

# Build the CLI application
$(CLI_TARGET): src/tracy.c examples/c_render/main.c $(wildcard dependencies/pcg-c/src/*.c)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Build and run
run: $(CLI_TARGET)
	./$(CLI_TARGET)

clean:
	rm -f $(CLI_TARGET)
