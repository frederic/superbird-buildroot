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
#include <assert.h>
#include <math.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ipc_jpegenc.h"

#include "recognition_db.h"

#define EPS 0.000001

static float gs_threshold = 0.6;
static float gs_scale = 0.0;
static int gs_threshold_uint = 0;

static int calcdiff(const FACE_ID_t *ldat, const FACE_ID_t *rdat, size_t size) {
  int i;
  int diff, sum = 0;
  for (i = 0; i < size; i++) {
    diff = ldat[i] - rdat[i];
    sum += diff * diff;
  }
  return sum;
}

static void calculateDiff(sqlite3_context *ctx, int argc,
                          sqlite3_value **argv) {
  assert(2 == argc);
  const void *ldat = sqlite3_value_blob(argv[0]);
  const void *rdat = sqlite3_value_blob(argv[1]);
  size_t size_ldat = sqlite3_value_bytes(argv[0]) / sizeof(FACE_ID_t);
  size_t size_rdat = sqlite3_value_bytes(argv[1]) / sizeof(FACE_ID_t);
  int diff = gs_threshold_uint + 1;
  if (size_ldat == size_rdat) {
    diff =
        calcdiff((const FACE_ID_t *)ldat, (const FACE_ID_t *)rdat, size_ldat);
  }
  sqlite3_result_int(ctx, diff);
}

static void prepare_tables(sqlite3 *db) {
  sqlite3_stmt *stmt = NULL;

  sqlite3_prepare_v2(db,
                     "CREATE TABLE IF NOT EXISTS `faceinfo` ("
                     "`index` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                     "`uid`   INTEGER NOT NULL,"
                     "`faceid`    BLOB NOT NULL,"
                     "`faceimg`   BLOB NOT NULL"
                     ");",
                     -1, &stmt, NULL);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  stmt = NULL;

  sqlite3_prepare_v2(db,
                     "CREATE TABLE IF NOT EXISTS `userinfo` ("
                     "`uid`   INTEGER NOT NULL PRIMARY KEY UNIQUE,"
                     "`name`  TEXT NOT NULL"
                     ");",
                     -1, &stmt, NULL);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  return;
}

static void insert_record(sqlite3 *db, FACE_ID_t *faceid, size_t faceid_size,
                          unsigned char *faceimg, int width, int height) {
  sqlite3_stmt *stmt = NULL;
  unsigned char *jpg = NULL;

  int rc = sqlite3_prepare_v2(
      db, "SELECT count(`uid`) FROM faceinfo WHERE `faceid`=? collate BINARY",
      -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_prepare_v2. %s\n", sqlite3_errmsg(db));
    return;
  }
  rc = sqlite3_bind_blob(stmt, 1, (void *)faceid,
                         sizeof(FACE_ID_t) * faceid_size, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_blob. %s\n", sqlite3_errmsg(db));
    goto insert_record_end;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    if (count > 0) {
      // face already exists
      goto insert_record_end;
    }
  }

  size_t szjpg = aml_ipc_jpegencsw_encode(faceimg, width, height, 80, &jpg);

  if (szjpg == 0) {
    return;
  }

  sqlite3_prepare_v2(
      db, "insert into faceinfo (`uid`, `faceid`, `faceimg`) values (0, ?, ?)",
      -1, &stmt, NULL);
  sqlite3_bind_blob(stmt, 1, (void *)faceid, sizeof(FACE_ID_t) * faceid_size,
                    NULL);
  sqlite3_bind_blob(stmt, 2, (void *)jpg, szjpg, NULL);
  sqlite3_step(stmt);

insert_record_end:
  sqlite3_finalize(stmt);
  stmt = NULL;
  if (jpg != NULL)
    free(jpg);
}

void *recognition_db_init(const char *dbfile) {
  sqlite3 *db;
  int rc = sqlite3_open(dbfile, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_open %s. %s\n", dbfile, sqlite3_errmsg(db));
    return NULL;
  }
  rc = sqlite3_create_function_v2(db, "CalcDiff", 2, SQLITE_UTF8, NULL,
                                  calculateDiff, NULL, NULL, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_create_function_v2 -> calcdiff. %s\n",
            sqlite3_errmsg(db));
    goto on_error;
  }

  prepare_tables(db);

  return (void *)db;
on_error:
  if (db) {
    sqlite3_close(db);
    db = NULL;
  }
  return (void *)db;
}

void recognition_db_deinit(void *db) {
  if (db) {
    sqlite3_close((sqlite3 *)db);
  }
}

static void update_threshold_uint() {
  gs_threshold_uint =
      (int)((gs_threshold * gs_threshold) / (gs_scale * gs_scale));
}

void recognition_db_set_threshold(float t) {
  gs_threshold = t;
  if (gs_scale > EPS) {
    update_threshold_uint();
  }
}

void recognition_db_set_scale(float s) {
  gs_scale = s;
  update_threshold_uint();
}

int recognition_db_search_result(void *db, FACE_ID_t *faceid,
                                 size_t faceid_size, unsigned char *faceimg,
                                 int width, int height, const char *format,
                                 char *buf, size_t bufsize) {
  if (db == NULL)
    return -1;

  int rc;
  int uid = -1;

  sqlite3 *sdb = (sqlite3 *)db;
  sqlite3_stmt *stmt = NULL;

  if (buf)
    buf[0] = '\0';

  // find the matched user id
  rc = sqlite3_prepare_v2(sdb,
                          "SELECT `index`, `uid`, CalcDiff(`faceid`, ?) FROM "
                          "faceinfo WHERE `uid` > 0",
                          -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_prepare_v2. %s\n", sqlite3_errmsg(sdb));
    goto on_search_end;
  }
  rc = sqlite3_bind_blob(stmt, 1, (void *)faceid,
                         sizeof(FACE_ID_t) * faceid_size, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_blob. %s\n", sqlite3_errmsg(sdb));
    goto on_search_end;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int face_diff = 0;
    int id = sqlite3_column_int(stmt, 1);
    int diff = sqlite3_column_int(stmt, 2);
    if (diff > gs_threshold_uint)
      continue;
    if (uid == -1) {
      uid = id;
      face_diff = diff;
    } else {
      if (diff < face_diff) {
        uid = id;
        face_diff = diff;
      }
    }
  }

  sqlite3_finalize(stmt);
  stmt = NULL;

  if (uid == -1) {
    goto on_search_end;
  }

  // get the detail info
  char sql[256] = {0};
  snprintf(sql, sizeof(sql), "SELECT %s FROM userinfo WHERE uid=?", format);
  rc = sqlite3_prepare_v2(sdb, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_prepare_v2. %s\n", sqlite3_errmsg(sdb));
    goto on_search_end;
  }
  rc = sqlite3_bind_int(stmt, 1, uid);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "sqlite3_bind_int. %s\n", sqlite3_errmsg(sdb));
    goto on_search_end;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    int count = sqlite3_column_count(stmt);
    int i;
    for (i = 0; i < count; i++) {
      int type = sqlite3_column_type(stmt, i);
      char vbuf[256] = {0};
      switch (type) {
      case SQLITE_TEXT: {
        const unsigned char *t = sqlite3_column_text(stmt, i);
        snprintf(vbuf, sizeof(vbuf), "%s", t);
      } break;
      case SQLITE_INTEGER: {
        int v = sqlite3_column_int(stmt, i);
        snprintf(vbuf, sizeof(vbuf), "%d", v);
      } break;
      case SQLITE_FLOAT: {
        double v = sqlite3_column_double(stmt, i);
        snprintf(vbuf, sizeof(vbuf), "%f", v);
      } break;
      default:
        break;
      }
      if (bufsize > 0) {
        size_t vbuf_len = strlen(vbuf);
        strncat(buf, vbuf, bufsize > vbuf_len ? vbuf_len : bufsize);
        bufsize -= vbuf_len;
      } else {
        break;
      }
    }
  } else {
    goto on_search_end;
  }

on_search_end:
  if (stmt) {
    sqlite3_finalize(stmt);
  }
  if (faceimg != NULL) {
    insert_record(sdb, faceid, faceid_size, faceimg, width, height);
  }
  return uid;
}
