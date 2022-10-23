#
##Makefile for Aml_Mic_Test
#
#
OBJ = AmlMic_check.o main.o
CFLAGS = -Wall -Wextra
TARGET=Aml_Mic_Test

LDLIBS += -lm
# rules
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $^ -o $@ $(LDLIBS)

.PHONY: clean

clean:
	rm -f $(OBJ)

install:
	cp Aml_Mic_Test  $(DESTDIR)/bin/Aml_Mic_Test
