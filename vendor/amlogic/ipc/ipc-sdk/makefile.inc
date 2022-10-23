CFLAGS += -Wall -Werror -Werror=jump-misses-init -fPIC -g -O2

OBJDIR = objs

OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

SUFFIXES += .d
DEPFILES := $(patsubst %c,$(OBJDIR)/%d,$(SRCS))
NODEPS := clean distclean

all: $(TARGET)

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif

$(TARGET): $(OBJS)
	@echo " [LINK] $@"
	@$(CC) -o $@ $(LDFLAGS) $+ $(LDLIBS)

$(OBJDIR)/%.o: %.c $(OBJDIR)/%.d
	@echo " [CC] $<"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.d: %.c
	@echo " [DEP] $<"
	@mkdir -p $(OBJDIR)/$(dir $<)
	@$(CC) $(CPPFLAGS) $(CFLAGS) -M -MT "$(patsubst %.c,$(OBJDIR)/%.o,$<)" $< -MF $@

.PHONY:clean

clean:
	@echo " [CLEAN] ..."
	@rm -rf $(OBJDIR) $(TARGET)

