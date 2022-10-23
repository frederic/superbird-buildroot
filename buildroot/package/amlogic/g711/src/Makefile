LIBS:=libg711.a

OBJS:=g711.o
FULL_OBJ=$(addprefix src/,$(OBJS))
APIS+=$(wildcard include/*.h)

CFLAGS+=-Iinclude

all: $(LIBS)

libg711.a: $(FULL_OBJ)
	$(AR) -cr $@ $^

clean:
	rm -rf g711.o libg711.a

install:
	mkdir -p $(STAGING_DIR)/usr/include/g711
	-install -m 644 $(APIS) $(STAGING_DIR)/usr/include/g711
	-install -m 644 $(LIBS) $(STAGING_DIR)/usr/lib
