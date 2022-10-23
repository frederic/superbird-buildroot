CFLAGS += -Wall -Werror -fPIC -g -O2
LDLIBS += -lpthread -laml_ipc_sdk -lgtest -lgtest_main

CXXFLAGS += -std=c++11

OBJDIR = objs

OBJS = $(patsubst %.cc,$(OBJDIR)/%.o,$(SRCS))

SUFFIXES += .d
DEPFILES := $(patsubst %cc,$(OBJDIR)/%d,$(SRCS))
NODEPS := clean distclean

all: $(TARGET)

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif

$(TARGET): $(OBJS)
	@echo " [LINK] $@"
	@$(CXX) -o $@ $(LDFLAGS) $+ $(LDLIBS)

$(OBJDIR)/%.o: %.cc $(OBJDIR)/%.d
	@echo " [CXX] $<"
	@$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d: %.cc
	@echo " [DEP] $<"
	@mkdir -p $(OBJDIR)
	@$(CXX) $(CPPFLAGS) $(CFLAGS) -M -MT "$(patsubst %.cc,$(OBJDIR)/%.o,$<)" $< -MF $@

.PHONY:clean

clean:
	@echo " [CLEAN] ..."
	@rm -rf $(OBJS) $(TARGET)

