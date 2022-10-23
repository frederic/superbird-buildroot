/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipc_property.h>

#ifdef TEST_CALLBACK
static void
on_property_changed(const char* key, const char* value) {
  printf("%s changed to %s\n", key, value);
}
#endif

int main(int argc, char* argv[]) {
  if (argc < 2) {
    ipc_property_list(NULL);
    return 0;
  }

  if (argc < 3) {
    printf("usage: %s <get/set/del/list/arrsz> <key> <value>\n", argv[0]);
    return -1;
  }
  char* act = argv[1];
  char* key = argv[2];
  char retval[256];

#ifdef TEST_CALLBACK
  ipc_property_register_callback(on_property_changed);
#endif

  if (strcmp(act, "set") == 0) {
    if (argc < 4) {
      printf("usage: %s set <key> <value>\n", argv[0]);
      return -1;
    }
    char* val = argv[3];
    ipc_property_set (key, val);
    int r = ipc_property_get (key, retval, sizeof(retval));
    retval[r] = '\0';
    printf("%s: %s\n", key, retval);
  } else if(strcmp(act, "get") == 0){
    int r = ipc_property_get (key, retval, sizeof(retval));
    retval[r] = '\0';
    printf("%s: %s\n", key, retval);
  } else if(strcmp(act, "list") == 0){
    ipc_property_list (key);
  } else if(strcmp(act, "del") == 0){
    ipc_property_remove (key);
  } else if(strcmp(act, "arrsz") == 0){
    int sz = ipc_property_get_array_size (key);
    printf("%s: %d\n", key, sz);
  } else {
    printf("usage: %s <get/set/del/list> <key> <value>\n", argv[0]);
    return -1;
  }

  return 0;

}
