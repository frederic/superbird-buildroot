default: main
.PHONY: clean

clean:
	-@rm ./test
main: ipc.c ipc.h main.c
	g++ ipc.c main.c -g -o test  -lpthread  $$(pkg-config json-c --libs --cflags) -DDEBUG
run: main ./test
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$$HOME/local/lib ./test
