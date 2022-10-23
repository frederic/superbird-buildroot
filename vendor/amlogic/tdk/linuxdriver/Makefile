ccflags-y+=-Werror
ccflags-y+=-I$(M)/include/linux
ccflags-y+=-I$(M)/include
ccflags-y+=-I$(M)

obj-m += optee.o
obj-y += optee/

optee-objs := tee_core.o \
	      tee_shm.o \
	      tee_shm_pool.o
