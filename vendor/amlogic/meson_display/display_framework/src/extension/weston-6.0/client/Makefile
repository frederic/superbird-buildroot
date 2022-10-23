default: main
.PHONY: clean

clean:
	-@rm ./test
main: drm-help-client.c drm-help-client.h main.c
	$(CC) drm-help-client.c drm-help-client.h -I../../../ipc main.c -g -o test -lpthread $$(pkg-config json-ipc --libs --cflags) $$(pkg-config json-c --libs --cflags) -DDEBUG
run: main ./test
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$$HOME/local/lib ./test
