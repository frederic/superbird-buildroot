#ifndef WAKE_LOCK_H
#define WAKE_LOCK_H
#include "data_types.h"

int acquire_wake_lock(void);
int release_wake_lock(void);
void wake_lock_exit(void);
static int is_suspend = FALSE;
#endif

