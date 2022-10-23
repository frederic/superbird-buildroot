###################################################################################
#Application MakeFile
#
#Author:zhenhua
#
#Description:use the makefile to make exe
#
###################################################################################
#source file
#find all source file and define to .o
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))

#target you can change test to what you want
TARGET  := bt.cgi
#compile and lib parameter
CXX     := g++
CC      := gcc
LIBS    := -lbsa -lrt -lpthread -lcjson
LDFLAGS :=
#-L/mnt/nfsroot/zhenhua.he/code2/output/mesonaxg_s420_32_debug/target/usr/lib
DEFINES :=
INCLUDE :=
#-I./include
CFLAGS  := -g -Wall -O3 $(DEFINES) $(INCLUDE)
CXXFLAGS:= $(CFLAGS) -DHAVE_CONFIG_H


.PHONY : everything objs clean veryclean rebuild

everything : $(TARGET)

all : $(TARGET)

objs : $(OBJS)

rebuild: veryclean everything

clean :
	rm -fr *.so
	rm -fr *.o
veryclean : clean
	rm -fr $(TARGET)
install :
	install -m 0755 btlist.html $(TARGET_DIR)/var/www
	install -m 0755 $(TARGET) $(TARGET_DIR)/var/www/cgi-bin
	install -m 0755 bt_con.cgi $(TARGET_DIR)/var/www/cgi-bin
	install -m 0755 S52bt_setup $(TARGET_DIR)/etc/init.d
$(TARGET) : $(OBJS)
	$(CC) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)
