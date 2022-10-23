CFLAGS +=-O2 -fPIC
CFLAGS += -g
LDFLAGS += -lm -lrt -shared
LDFLAGS += -lge2d

SUFFIXES += .d
NODEPS := clean distclean

C_SRCS := $(wildcard *.c) $(wildcard **/*.c)
C_OBJS := $(C_SRCS:%.c=%.o)

CXX_SRCS := $(wildcard *.cpp) $(wildcard **/*.cpp)
CXX_OBJS := $(CXX_SRCS:%.cpp=%.o)

DEPFILES := $(patsubst %c,%d,$(C_SRCS))
DEPFILES += $(patsubst %cpp,%d,$(CXX_SRCS))


INC = -I./include -I.
TARGET = libvphevcodec.so

all: $(TARGET)

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif


$(TARGET): $(C_OBJS) $(CXX_OBJS)
	@echo " [LINK] $@"
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo " [CXX] $<"
	@$(CC) -c $(CFLAGS) $(INC) $< -o $@

%.o: %.cpp
	@echo " [CXX] $<"
	@$(CXX) -c $(CFLAGS) $(INC) $< -o $@

%.d: %.cpp
	@echo " [DEP] $<"
	@$(CXX) $(CFLAGS) -MM -MT "$(patsubst %.cpp,%.o,$<)" $< -MF $@

%.d: %.c
	@echo " [DEP] $<"
	@$(CC) $(CFLAGS) -MM -MT "$(patsubst %.c,%.o,$<)" $< -MF $@

clean:
	@rm -f $(DEPFILES)
	@rm -f $(C_OBJS) $(CXX_OBJS)
	@rm -f $(TARGET)
