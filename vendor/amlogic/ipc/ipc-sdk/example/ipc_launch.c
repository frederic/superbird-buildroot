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
#define _GNU_SOURCE
#include "aml_ipc_internal.h"
#include <dlfcn.h>
#include <elf.h>
#include <fcntl.h>
#include <getopt.h>
#include <regex.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static volatile int bexit = 0;

void signal_handle(int sig) { bexit = 1; }

static inline void file_close(int *fd) { if (*fd >= 0) close(*fd); }

static int get_symbol_name(void *addr, char *buf, int len) {
    Dl_info dl;
    char *offset = addr;
    if (addr == NULL || dladdr(addr, &dl) == 0) {
        return snprintf(buf, len, "%p (unknown)", offset);
    }
    if (dl.dli_sname) {
        // in .dynsym section
        return snprintf(buf, len, "%p %s", offset, dl.dli_sname);
    }
    // try to lookup in .symtab section
    if (dl.dli_fname == NULL) {
        return snprintf(buf, len, "%p nolib", offset);
    }
    offset -= (uintptr_t)dl.dli_fbase;
    int retlen = snprintf(buf, len, "%p %s+%p", addr, dl.dli_fname, offset);

    AML_AUTO_VAR(file_close) int fd = open(dl.dli_fname, O_RDONLY);
    struct stat sb;
    if (fd < 0 || (fstat(fd, &sb) < 0)) {
        return retlen;
    }
    const char *baseaddr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (baseaddr[EI_MAG0] != ELFMAG0 || baseaddr[EI_MAG1] != ELFMAG1 ||
        baseaddr[EI_MAG2] != ELFMAG2 || baseaddr[EI_MAG3] != ELFMAG3) {
        return retlen;
    }
    if (baseaddr[EI_CLASS] == ELFCLASS64) {
        Elf64_Ehdr *hdr = (Elf64_Ehdr *)baseaddr;
        Elf64_Shdr *shdr = (Elf64_Shdr *)(baseaddr + hdr->e_shoff);
        for (int i = 0; i < hdr->e_shnum; ++i) {
            if ((shdr[i].sh_type == SHT_SYMTAB) || (shdr[i].sh_type == SHT_DYNSYM)) {
                Elf64_Sym *symboler = (Elf64_Sym *)(baseaddr + shdr[i].sh_offset);
                for (int j = (shdr[i].sh_size / shdr[i].sh_entsize); j--; ++symboler) {
                    if (symboler->st_value == (uintptr_t)offset) {
                        retlen = snprintf(buf, len, "%p %s", addr,
                                          baseaddr + shdr[shdr[i].sh_link].sh_offset +
                                              symboler->st_name);
                        goto found;
                    }
                }
            }
        }
    } else if (baseaddr[EI_CLASS] == ELFCLASS32) {
        Elf32_Ehdr *hdr = (Elf32_Ehdr *)baseaddr;
        Elf32_Shdr *shdr = (Elf32_Shdr *)(baseaddr + hdr->e_shoff);
        for (int i = 0; i < hdr->e_shnum; ++i) {
            if ((shdr[i].sh_type == SHT_SYMTAB) || (shdr[i].sh_type == SHT_DYNSYM)) {
                Elf32_Sym *symboler = (Elf32_Sym *)(baseaddr + shdr[i].sh_offset);
                for (int j = (shdr[i].sh_size / shdr[i].sh_entsize); j--; ++symboler) {
                    if (symboler->st_value == (uintptr_t)offset) {
                        retlen = snprintf(buf, len, "%p %s", addr,
                                          baseaddr + shdr[shdr[i].sh_link].sh_offset +
                                              symboler->st_name);
                        goto found;
                    }
                }
            }
        }
    }
found:
    munmap((void*)baseaddr, sb.st_size);

    return retlen;
}

static void show_type(struct AmlTypeClass *cls, struct AmlTypeClass *cur, struct AmlObject *inst) {
    if (cur == AML_OBJ_TYPEID_INSTANCE(AmlType)) {
        printf("%s:\n\n", cur->name);
    } else {
        printf("%s:\n", cur->name);
    }
    if (cur == AML_OBJ_TYPEID_INSTANCE(AmlIPCComponent)) {
        struct AmlIPCComponentClass *ccls = (struct AmlIPCComponentClass *)cls->class_obj;
        char symname[256];
        get_symbol_name(ccls->get_frame, symname, sizeof(symname));
        printf("get_frame %s\n", symname);
        get_symbol_name(ccls->handle_frame, symname, sizeof(symname));
        printf("handle_frame %s\n", symname);
        struct AmlIPCComponent *comp;
        if (inst == NULL) {
            comp = (struct AmlIPCComponent *)aml_obj_new_from_type(cls);
        } else {
            comp = (struct AmlIPCComponent *)inst;
        }
        int num_chan = aml_ipc_num_channel(comp, AML_CHANNEL_BOTH);
        for (int i=0; i<num_chan; ++i) {
            int id = i;
            enum AmlIPCChannelDir dir = AML_CHANNEL_BOTH;
            const char *name;
            if (aml_ipc_get_channel_by_index(comp, &id, &dir, &name) == AML_STATUS_OK) {
                printf("channel %d ID:%d dir:%s name:%s\n", i, id,
                       dir == AML_CHANNEL_INPUT ? "in" : "out", name);
            }
        }
        if (inst == NULL)
            aml_obj_release(AML_OBJECT(comp));
    } else if (cur == AML_OBJ_TYPEID_INSTANCE(AmlObject)) {
        struct AmlObjectClass *ocls = (struct AmlObjectClass *)cls->class_obj;
        struct AmlPropertyNode *node;
        STAILQ_FOREACH(node, &ocls->properties, next) {
            struct AmlPropertyInfo *info = node->info;
            printf("%s @%d %s ", info->name, info->offset, info->description);
            if (AML_OBJ_ISTYPE(info, AmlPropertyInfoInt)) {
                struct AmlPropertyInfoInt *p = (struct AmlPropertyInfoInt *)info;
                printf("(int)%d [%d-%d]\n", p->def, p->min, p->max);
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoInt64)) {
                struct AmlPropertyInfoInt64 *p = (struct AmlPropertyInfoInt64 *)info;
                printf("(int64)%" PRId64 " [%" PRId64 "-%" PRId64 "]\n", p->def, p->min, p->max);
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoFloat)) {
                struct AmlPropertyInfoFloat *p = (struct AmlPropertyInfoFloat *)info;
                printf("(float)%f [%f-%f]\n", p->def, p->min, p->max);
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoStr)) {
                struct AmlPropertyInfoStr *p = (struct AmlPropertyInfoStr *)info;
                printf("(string)%s\n", p->def);
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoEnum)) {
                struct AmlPropertyInfoEnum *p = (struct AmlPropertyInfoEnum *)info;
                printf("(enum)%d\n", p->def);
                for (struct AmlValueEnum *ev = p->val; ev->name; ++ev) {
                    printf("    %d - %s\n", ev->val, ev->name);
                }
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoObject)) {
                struct AmlPropertyInfoObject *p = (struct AmlPropertyInfoObject *)info;
                printf("(object)%s\n", p->type->name);
            } else if (AML_OBJ_ISTYPE(info, AmlPropertyInfoStruct)) {
                printf("(struct) %d bytes\n", info->val_size);
            } else {
                printf("(%s) %d bytes\n", info->type_obj->name, info->val_size);
            }
        }
    }
    if (cur->parent_type) {
        show_type(cls, cur->parent_type, inst);
    }
}

static void list_obj(const char *name) {
    regex_t pattern;
    if (name) {
        if (regcomp(&pattern, name, REG_NOSUB | REG_ICASE) != 0) {
            printf("invalid pattern '%s'\n", name);
            exit(1);
        }
    }
    struct AmlTypeClass *cls = aml_obj_get_registerd_type();
    for (; cls; cls = cls->next) {
        if (name == NULL) {
            show_type(cls, cls, NULL);
        } else if (regexec(&pattern, cls->name, 0, NULL, 0) == 0) {
            show_type(cls, cls, NULL);
        }
    }
    if (name)
        regfree(&pattern);
}

#if 0
static void dump_json(int level, struct JSONValue *val, const char *sep) {
    if (val->val_type == JSON_TYPE_ARRAY) {
        printf("%*s [\n", level * 2, " ");
        for (struct JSONValue *p = val->val_list; p; p = p->next) {
            dump_json(level + 1, p, ",\n");
        }
        printf("%*s ]\n", level * 2, " ");
    } else if (val->val_type == JSON_TYPE_OBJECT) {
        printf("%*s {\n", level * 2, " ");
        for (struct JSONValue *p = val->val_list; p; p = p->next) {
            dump_json(level + 1, p, ":\n");
            p = p->next;
            if (p)
                dump_json(level + 1, p, ",\n");
        }
        printf("%*s }\n", level * 2, " ");
    } else {
        static char buffer[256];
        aml_ipc_get_json_value_string(val, buffer, sizeof(buffer));
        printf("%*s %s %s", level * 2, " ", buffer, sep);
    }
}
#endif

int main(int argc, const char *argv[]) {
    aml_ipc_sdk_init();
    const char *debuglevel = getenv("AML_DEBUG");
    if (!debuglevel) debuglevel = ".*:3";
    aml_ipc_log_set_from_string(debuglevel);
    const char *tracelevel = getenv("AML_TRACE");
    if (tracelevel) {
        aml_ipc_trace_set_from_string(tracelevel);
        const char *tracefile = getenv("AML_TRACE_FILE");
        if (tracefile) {
            FILE *ftrace = NULL;
            if (strcmp(tracefile, "stderr") == 0)
                ftrace = stderr;
            else if (strcmp(tracefile, "stdout") == 0)
                ftrace = stdout;
            else
                ftrace = fopen(tracefile, "wb");
            aml_ipc_trace_set_json_output(ftrace);
        }
    }

    int list = 0;
    int opt;
    const char *name = NULL;
    const char *json_file = NULL;
    while ((opt = getopt(argc, (char **)argv, "hl::f:")) != -1) {
        switch (opt) {
        case 'l':
            if (optarg) {
                name = optarg;
            } else if (optind < argc && argv[optind][0] != '-') {
                name = argv[optind];
                ++optind;
            }
            list = 1;
            break;
        case 'f':
            json_file = optarg;
            break;
        default:
            printf("unrecognized opt -%c\n", opt);
        case 'h':
            printf("USAGE:%s [opt] [desc]\n"
                   "  -h: show this help\n"
                   "  -l [obj]: list object informations\n"
                   "  -f jsonfile: read pipeline descriptions form json file\n"
                   " desc: pipeline descriptions\n",
                   argv[0]);
            return 0;
        }
    }
    struct AmlIPCPipeline *pipeline = NULL;
    if (json_file)  {
        AML_AUTO_VAR(file_close) int fd = open(json_file, O_RDONLY);
        struct stat sb;
        if (fd < 0 || (fstat(fd, &sb) < 0)) {
            printf("fail to open json file %s\n", json_file);
            return 1;
        }
        char *buffer = malloc(sb.st_size + 1);
        ssize_t len = read(fd, buffer, sb.st_size);
        if (len >= 0) {
            buffer[len] = '\0';
        }
        pipeline = aml_ipc_parse_launch(buffer);
        free(buffer);
    } else if (optind < argc) {
        const char *jsontext = argv[optind];
        pipeline = aml_ipc_parse_launch(jsontext);
    }
    if (list) {
        if (pipeline) {
            struct AmlIPCComponent *comp;
            struct AmlIPCPipelineIterator iter = {NULL};
            while (aml_ipc_pipeline_iterate(pipeline, &iter, &comp) == AML_STATUS_OK) {
                show_type(AML_OBJ_TYPEID(comp), AML_OBJ_TYPEID(comp), AML_OBJECT(comp));
            }
        } else {
            list_obj(name);
        }
        return 0;
    }
    if (pipeline) {
        signal(SIGINT, signal_handle);
        aml_ipc_start(AML_IPC_COMPONENT(pipeline));
        while (!bexit) {
            usleep(1000 * 100);
        }
        aml_ipc_stop(AML_IPC_COMPONENT(pipeline));
        aml_obj_release(AML_OBJECT(pipeline));
    }
    return 0;
}
