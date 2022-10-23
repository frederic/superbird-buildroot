#######################################
#  This makefile is written for a GNU make or equivalent.
#  For example:
#   make clean
#   make
# Copyright (c) 2010, Amlogic Corp., All Rights Reserved.
#######################################
TAR=libdspc.so
obj=PortAudioMicrophoneWrapper.cpp DSPC_interface.cpp

$(TAR): $(obj)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $^  


all:$(TAR)

clean:
	rm -f $(TAR)


