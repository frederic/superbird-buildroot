# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

CFLAGS += -O2 -std=c99
CFLAGS+= -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I. -I../vpuapi/include -I../vpuapi


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs
TARGET = libvpcodec.so

LDLIBS += -lm -lrt 
# pthread 
LDLIBS += -lpthread 
LIB= -L../vpuapi -lamvenc_api


LDFLAGS += -shared

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = libvpmulti_codec.c  \
		  AML_MultiEncoder.c \

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))


all: CREATE_DIR $(OBJECTPATHS)
	$(CC) -o $(TARGET) $(LDFLAGS) $(LIB) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group

-include $(OBJECTPATHS:.o=.dep)

clean: 
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)
