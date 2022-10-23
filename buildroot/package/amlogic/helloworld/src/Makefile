#
## sample Makefile for helloworld
#
#
OBJ = helloworld.o
CFLAGS = -Wall -Wextra
TARGET=helloworld

# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	cp helloworld $(DESTDIR)/bin/helloworld
