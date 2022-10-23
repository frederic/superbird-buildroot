#
## makefile for libgdc and gdc_test
#
LIBGDC_OBJ = gdc.o
CFLAGS += -I ./include/gdc/
LIBGDC = libgdc.so

LIBDIR:= .
GDC_TEST_OBJ = gdc_test.o
GDC_CHIP_CHECK_OBJ = gdc_chip_check.o
GDC_TEST = gdc_test
GDC_CHIP_CHECK = gdc_chip_check
OBJS = $(GDC_TEST_OBJ) $(GDC_CHIP_CHECK_OBJ) $(LIBGDC_OBJ)

# rules
all: $(LIBGDC) $(GDC_TEST) $(GDC_CHIP_CHECK)

$(GDC_TEST): $(LIBGDC)
$(GDC_CHIP_CHECK): $(LIBGDC)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@

$(LIBGDC): $(LIBGDC_OBJ)
	$(CC) -shared -fPIC -lion $(CFLAGS) $^ -o $(LIBGDC)

$(GDC_TEST): $(GDC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc -lpthread $(CFLAGS) -o $@

$(GDC_CHIP_CHECK): $(GDC_CHIP_CHECK_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc -lion -lpthread $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(GDC_TEST) $(GDC_CHIP_CHECK) $(LIBGDC)
