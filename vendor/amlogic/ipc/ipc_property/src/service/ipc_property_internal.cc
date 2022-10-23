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

#include "ipc_property_internal.h"
#include <json.hpp>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <regex>
#include <libgen.h>

using namespace std;
using namespace nlohmann;

static json g_json_properties;


#define MAX_LEVEL 256
int entry_item[MAX_LEVEL] = {0};

static void
iterate_js(json js, int level, ostringstream &sout) {
  string prefix = "";
  for(int i = 0; i < level; i++) {
    if(entry_item[i] > 0) {
      prefix += "│   ";
    } else {
      prefix += "    ";
    }
  }
  if(js.is_object() || js.is_array()) {
    entry_item[level] = js.size();
    for(json::iterator it = js.begin(); it != js.end(); ++it) {
      string entry = prefix;
      entry_item[level] --;
      json j = *it;
      if(entry_item[level] == 0) {
        entry += "└─";
      } else {
        entry += "├─";
      }
      if(js.is_object()) {
        sout << entry << "─ " << it.key() << endl;
        iterate_js(j, level + 1, sout);
      } else {
        iterate_js(j, level, sout);
      }
    }
  } else {
    if(entry_item[level] > 0) {
      prefix += "├── ";
    } else {
      prefix += "└── ";
    }
    sout << prefix << js << endl;
  }
}

static void
remove_trailing_slash(char *key) {
  if (key) {
    size_t len = strlen (key);
    while (len-- > 0) {
      if (key[len] == '/') {
        key[len] = '\0';
      } else {
        break;
      }
    }
  }
}

size_t __property_list(const char *key, char *outbuf, size_t bufsize) {
  if(outbuf == nullptr || bufsize == 0) return 0;
  ostringstream s;
  try {
    char *d = key == nullptr ? nullptr : strdup(key);
    remove_trailing_slash (d);
    if (d && strlen(d) > 0) {
      s << "[" << d << "]" << endl;
      json j = g_json_properties.at(json::json_pointer(d));
      if (not j.empty()) {
        iterate_js (j, 0, s);
      }
      free (d);
    } else {
      s << "[root]" << endl;
      iterate_js (g_json_properties, 0, s);
    }
    strncpy(outbuf, s.str().c_str(), bufsize);
    outbuf[bufsize - 1] = '\0';
    return strlen(outbuf);
  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return 0;
  } catch (const char *str) {
    cerr << str << endl;
    return 0;
  }
}

bool __property_set(const char *key, const char *value) {
  if (key == nullptr || value == nullptr) return false;

  try {
    json &j = g_json_properties[json::json_pointer(key)];
    if (j.empty()) {
      j = value;
    } else {
      if (j.is_primitive()) {
        string oldval;
        if (j.is_string()) {
          oldval = j.get<string>();
        } else {
          oldval = j.dump();
        }
        if (oldval != value) {
          if (j.is_string()) {
            j = value;
          } else {
            istringstream is(value);
            if (j.is_boolean()){
              bool b;
              is >> boolalpha >> b; j = b;
            } else if (j.is_number_unsigned()) {
              unsigned long ul;
              is >> ul; j = ul;
            } else if (j.is_number_integer()) {
              long l;
              is >> l; j = l;
            } else if (j.is_number_float()) {
              float f;
              is >> f; j = f;
            } else {
              // unsupported
              return false;
            }
          }
        } else {
          return false; // no change
        }
      } else {
        return false;
      }
    }
    return true;
  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return false;
  } catch (const char *str) {
    cerr << str << endl;
    return false;
  }

}

/*
 * key consists of keys and slash the same as the file path
 * /key/to/the/location for example.
 * as of the array, visit the value with index,
 * /key/to/the/array/0/item for example.
 */
size_t __property_get(const char *key, char *value, size_t bufsize) {
  if (key == nullptr || value == nullptr || bufsize == 0) return 0;

  try {
    json j = g_json_properties.at(json::json_pointer(key));
    if (j.empty()) return 0;

    string jval;
    if (j.is_primitive()) {
      if(j.is_string()) {
        jval = j.get<string>();
      } else {
        jval = j.dump();
      }
      strncpy(value, jval.c_str(), bufsize);
      value[bufsize - 1] = '\0';
      return strlen(value);
    }
    return 0;
  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return 0;
  } catch (const char *str) {
    cerr << str << endl;
    return 0;
  }

}

bool __property_remove(const char *key) {
  if (key == nullptr) return 0;
  try {
    char *keydup = strdup(key);
    remove_trailing_slash (keydup);
    json::json_pointer ptr(keydup);
    if (not g_json_properties.at(ptr).empty()) {
      char * k = strdup (keydup);
      string kdir = dirname (k); free (k);
      k = strdup (keydup);
      string kbase = basename (k); free (k);
      json &j = g_json_properties[json::json_pointer(kdir)];
      if (j.is_array()) {
        j.erase(stoi(kbase));
      } else {
        j.erase(kbase);
      }
    }
    free (keydup);
    return true;
  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return false;
  } catch (const char *str) {
    cerr << str << endl;
    return false;
  }
}

int __property_get_array_size(const char *key) {
  if (key == nullptr) return 0;

  try {
    json j = g_json_properties.at(json::json_pointer(key));
    if (j.empty()) return 0;

    if (j.is_array()) {
      return j.size();
    }
    return 0;
  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return 0;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return 0;
  } catch (const char *str) {
    cerr << str << endl;
    return 0;
  }

}

bool __save_properties(const char *fn) {
  try {
    ofstream stream_json(fn);
    if (!stream_json.is_open()){
      throw("failed to open config file");
    }
    stream_json << setw(4) << g_json_properties << endl;
  } catch (const char *str) {
    cerr << str << endl;
    return false;
  }
  return true;
}

bool __load_properties(const char *fn) {
  try {
    ifstream stream_json(fn);
    if (!stream_json.is_open()) {
      throw("failed to open config file");
    }
    g_json_properties.clear();
    stream_json >> g_json_properties;

  } catch (detail::parse_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::type_error &e) {
    cerr << e.what() << endl;
    return false;
  } catch (detail::out_of_range &e) {
    cerr << "error happend: " << e.what() << endl;
    return false;
  } catch (const char *str) {
    cerr << str << endl;
    return false;
  }

  return true;
}

