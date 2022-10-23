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

#ifndef AML_IPC_OBJECT_H

#define AML_IPC_OBJECT_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AML_OBJ_TYPEID_TYPE_(type) struct type##_type_
#define AML_OBJ_TYPEID_NAME_(type) type##_type_instance_

/**
 * @brief declear a new typeid and it's inheritance, this is usually used in a header file
 *
 * @param otype, object type, name of the C struct
 * @param ptype, parent object type
 * @param ctype, class object type
 */
#define AML_OBJ_DECLARE_TYPEID(otype, ptype, ctype)                                                \
    AML_OBJ_TYPEID_TYPE_(otype) {                                                                  \
        const char *name;                                                                          \
        AML_OBJ_TYPEID_TYPE_(ptype) * parent_type;                                                 \
        union {                                                                                    \
            struct ctype *class_obj;                                                               \
            struct otype *instance;                                                                \
        };                                                                                         \
        void (*class_init)(struct ctype *);                                                        \
        void (*inst_init)(struct otype *);                                                         \
        void (*inst_free)(struct otype *);                                                         \
        int instance_size;                                                                         \
        int flags;                                                                                 \
        struct AmlTypeClass *next;                                                                 \
    };                                                                                             \
    extern AML_OBJ_TYPEID_TYPE_(otype) AML_OBJ_TYPEID_NAME_(otype)

/**
 * @brief define the typeid of the new type
 * @param otype, object type, name of the C struct
 * @param ptype, parent object type
 * @param ctype, class object type
 * @param c_init, class object initialize
 * @param i_init, object instance initialize
 * @param i_free, object instance destruction
 */
#define AML_OBJ_DEFINE_TYPEID(otype, ptype, ctype, c_init, i_init, i_free)                         \
    static struct ctype otype##_class_;                                                            \
    AML_OBJ_TYPEID_TYPE_(otype)                                                                    \
    AML_OBJ_TYPEID_NAME_(otype) = {#otype,                                                         \
                                   &AML_OBJ_TYPEID_NAME_(ptype),                                   \
                                   {&otype##_class_},                                              \
                                   c_init,                                                         \
                                   i_init,                                                         \
                                   i_free,                                                         \
                                   sizeof(struct otype),                                           \
                                   0};                                                             \
    __attribute__((constructor)) static void init_type_##otype##_(void) {                          \
        aml_obj_typelib_init(AML_OBJ_TYPEID_INSTANCE(otype));                                      \
    }

#define AML_OBJ_EXTENDS(otype, ptype, ctype)                                                       \
    union {                                                                                        \
        struct ptype parent_obj;                                                                   \
        AML_OBJ_TYPEID_TYPE_(otype) * type_obj;                                                    \
    }

// get typeid from a type
#define AML_OBJ_TYPEID_INSTANCE(otype) ((struct AmlTypeClass *)&AML_OBJ_TYPEID_NAME_(otype))
#define AML_OBJ_TYPEID_CLASS(otype) (AML_OBJ_TYPEID_NAME_(otype).class_obj)

// get typeid from an object
#define AML_OBJ_TYPEID(obj) ((struct AmlTypeClass *)(obj)->type_obj)
// get class object
#define AML_OBJ_CLASS(obj) ((obj)->type_obj->class_obj)
// get parent class object
#define AML_OBJ_PARENT_CLASS(obj) ((obj)->type_obj->parent_type->class_obj)
// get parent object
#define AML_OBJ_PARENT(obj) (&(obj)->parent_obj)

// create an object otype
#define AML_OBJ_NEW(otype) (struct otype *)aml_obj_new_from_type(AML_OBJ_TYPEID_INSTANCE(otype))
// init an object, mainly used to init an object allocated on stack or as a member of other object
#define AML_OBJ_INIT(obj, otype) aml_obj_init_with_type(AML_OBJ_TYPEID_INSTANCE(otype), obj)

// check if object is an derived instance of otype
#define AML_OBJ_INSTANCEOF(obj, otype)                                                             \
    aml_obj_type_instance_of(AML_OBJ_TYPEID(obj), AML_OBJ_TYPEID_INSTANCE(otype))

// check if object is an instance of the type
#define AML_OBJ_ISTYPE(obj, otype) (AML_OBJ_TYPEID(obj) == AML_OBJ_TYPEID_INSTANCE(otype))

// cls->mem = val, mainly used to assign function member
// TODO: store the function name somewhere in the class, and improve the log output
#define AML_OBJ_SET_CLASS_MEMBER(cls, mem, val) (cls)->mem = (typeof((cls)->mem))(val)

#define AML_TYPE_FLAG_INITIALIZED (1 << 0)
struct AmlObject;
// typeid define
struct AmlTypeClass {
    const char *name;
    struct AmlTypeClass *parent_type;
    void *class_obj;
    void (*class_init)(void *);
    void (*inst_init)(void *);
    void (*inst_free)(void *);
    int instance_size;
    int flags;
    struct AmlTypeClass *next;
};

// base class of all object
struct AmlType {
    struct AmlTypeClass *type_obj;
};

struct AmlPropertyInfo;

struct AmlPropertyInfoClass {
    // set property from variadic arguments(depends on property type), write the value to dst
    AmlStatus (*set)(struct AmlPropertyInfo *, void *dst, va_list *ap);
    // set property from string representation, write the value to dst
    AmlStatus (*set_str)(struct AmlPropertyInfo *, void *dst, const char *str);
    // get property pointed by src and write value to variadic arguments(depends on property type)
    AmlStatus (*get)(struct AmlPropertyInfo *, void *src, va_list *ap);
    // write property default value to dst
    AmlStatus (*def)(struct AmlPropertyInfo *, void *dst);
};
struct AmlPropertyInfo {
    AML_OBJ_EXTENDS(AmlPropertyInfo, AmlType, AmlPropertyInfoClass);
    const char *name;
    const char *description;
    int offset;
    int val_size;
};

struct AmlValueEnum {
    const char *name;
    int val;
};

#define _M_EXPR_(...) __VA_ARGS__
#define AML_DECLARE_PRIMITIVE_TYPE(t, mem)                                                         \
    struct AmlPropertyInfo##t {                                                                    \
        AML_OBJ_EXTENDS(AmlPropertyInfo##t, AmlPropertyInfo, AmlPropertyInfoClass);                \
        _M_EXPR_ mem                                                                               \
    };                                                                                             \
    AML_OBJ_DECLARE_TYPEID(AmlPropertyInfo##t, AmlPropertyInfo, AmlPropertyInfoClass);

#define AML_DEFINE_PRIMITIVE_TYPE(t, set_, setstr_, get_, def_)                                    \
    static inline AmlStatus property_##t##_set_(struct AmlPropertyInfo##t *info, void *dst,        \
                                                va_list *ap) {                                     \
        _M_EXPR_ set_                                                                              \
    }                                                                                              \
    static inline AmlStatus property_##t##_set_str_(struct AmlPropertyInfo##t *info, void *dst,    \
                                                    const char *str) {                             \
        _M_EXPR_ setstr_                                                                           \
    }                                                                                              \
    static inline AmlStatus property_##t##_get_(struct AmlPropertyInfo##t *info, void *src,        \
                                                va_list *ap) {                                     \
        _M_EXPR_ get_                                                                              \
    }                                                                                              \
    static inline AmlStatus property_##t##_def_(struct AmlPropertyInfo##t *info, void *dst) {      \
        _M_EXPR_ def_                                                                              \
    }                                                                                              \
    static inline void pi_##t##_class_init(struct AmlPropertyInfoClass *cls) {                     \
        cls->set = (AmlStatus(*)(struct AmlPropertyInfo *, void *, va_list *))property_##t##_set_; \
        cls->set_str =                                                                             \
            (AmlStatus(*)(struct AmlPropertyInfo *, void *, const char *))property_##t##_set_str_; \
        cls->get = (AmlStatus(*)(struct AmlPropertyInfo *, void *, va_list *))property_##t##_get_; \
        cls->def = (AmlStatus(*)(struct AmlPropertyInfo *, void *))property_##t##_def_;            \
    }                                                                                              \
    AML_OBJ_DEFINE_TYPEID(AmlPropertyInfo##t, AmlPropertyInfo, AmlPropertyInfoClass,               \
                          pi_##t##_class_init, NULL, NULL);

AML_DECLARE_PRIMITIVE_TYPE(Int, (int min; int max; int def;));
AML_DECLARE_PRIMITIVE_TYPE(Int64, (int64_t min; int64_t max; int64_t def;));
AML_DECLARE_PRIMITIVE_TYPE(Float, (float min; float max; float def;));
AML_DECLARE_PRIMITIVE_TYPE(Str, (const char *def;));
AML_DECLARE_PRIMITIVE_TYPE(Object, (struct AmlTypeClass * type;));
AML_DECLARE_PRIMITIVE_TYPE(Enum, (int def; struct AmlValueEnum * val;));
AML_DECLARE_PRIMITIVE_TYPE(Struct, ());

#define AML_OBJ_PROP_TYPE(prop, otype, name, desc, vlen, ...)                                      \
    static struct otype prop##__ = {{{{{AML_OBJ_TYPEID_INSTANCE(otype)}}, name, desc, 0, vlen}},   \
                                    ##__VA_ARGS__};                                                \
    static struct AmlPropertyInfo *prop = (struct AmlPropertyInfo *)&prop##__
#define AML_OBJ_PROP_INT(prop, name, desc, min, max, def)                                          \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoInt, name, desc, sizeof(int), min, max, def)
#define AML_OBJ_PROP_INT64(prop, name, desc, min, max, def)                                        \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoInt64, name, desc, sizeof(int64_t), min, max, def)
#define AML_OBJ_PROP_FLOAT(prop, name, desc, min, max, def)                                        \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoFloat, name, desc, sizeof(float), min, max, def)
#define AML_OBJ_PROP_STR(prop, name, desc, def)                                                    \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoStr, name, desc, sizeof(char *), def)
#define AML_OBJ_PROP_ENUM(prop, name, desc, val, def)                                              \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoEnum, name, desc, sizeof(int), def, val)
#define AML_OBJ_PROP_OBJECT(prop, name, desc, otype)                                               \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoObject, name, desc, sizeof(void *),                     \
                      AML_OBJ_TYPEID_INSTANCE(otype))
#define AML_OBJ_PROP_STRUCT(prop, name, desc, struct_name)                                         \
    AML_OBJ_PROP_TYPE(prop, AmlPropertyInfoStruct, name, desc, sizeof(struct_name))

struct AmlPropertyNode {
    STAILQ_ENTRY(AmlPropertyNode) next;
    struct AmlPropertyInfo *info;
};

/* clang-format off */
/**
 * @brief AmlObject "virtual" functions
 * get_property: get one property from the object, implementation shall write the value to the
 *   memory pointed by "val".
 * set_property: set one property of the object, implementation shall get the value from the 
 *   memory pointed by "val".
 *
 * What struct "val" points to depends on the property type,  "val" type is same as the variadic
 * arguments of aml_obj_get_properties, here's list of SDK build-in properties:
 *
 *  property type | aml_obj_set_properties | aml_obj_get_properties, get_property/set_property
 *  -----------------------------------------------------------------------------------------------
 *  Int           | int                    | int *
 *  Int64         | int64_t                | int64_t *
 *  Float         | float                  | float *
 *  Enum          | int                    | int *
 *  Struct name   | struct name *          | struct name *
 *  Str           | char *                 | char **
 *  Object        | AmlObject *            | AmlObject **
 *
 *  NOTE: an AmlObject's property is part of the object, i.e. for Str and Object type, 
 *  aml_obj_get_properties will output a pointer, the pointer shall remain valid before AmlObject
 *  is destroyed, this means the caller of aml_obj_get_properties gets the pointer
 *  it does not need to free/release the pointer
 *
 */
/* clang-format on */
#define AML_OBJECT_FLAG_PROPERTY_NO_DEF (1 << 0)
struct AmlObjectClass {
    AmlStatus (*get_property)(struct AmlObject *, struct AmlPropertyInfo *, void *val);
    AmlStatus (*set_property)(struct AmlObject *, struct AmlPropertyInfo *, void *val);
    STAILQ_HEAD(AmlObjectPropertyList, AmlPropertyNode) properties;
    int num_create;
    int flags;
};

struct AmlObject {
    AML_OBJ_EXTENDS(AmlObject, AmlType, AmlObjectClass);
    int refcount;
    char *name;
};

// cast to AmlObject *
#define AML_OBJECT(obj) ((struct AmlObject *)(obj))
#define AML_OBJECT_NAME(obj) (AML_OBJECT(obj)->name)

#define AmlType_type_ AmlTypeClass
extern AML_OBJ_TYPEID_TYPE_(AmlType) AML_OBJ_TYPEID_NAME_(AmlType);
AML_OBJ_DECLARE_TYPEID(AmlObject, AmlType, AmlObjectClass);
AML_OBJ_DECLARE_TYPEID(AmlPropertyInfo, AmlType, AmlPropertyInfoClass);

struct AmlType *aml_obj_new_from_type(struct AmlTypeClass *type);
void aml_obj_init_with_type(struct AmlTypeClass *type, void *obj);
struct AmlType *aml_obj_new_from_name(const char *name);
int aml_obj_type_instance_of(struct AmlTypeClass *ot, struct AmlTypeClass *t);

void aml_obj_addref(struct AmlObject *obj);
void aml_obj_release(struct AmlObject *obj);
static inline int aml_obj_refcnt(struct AmlObject *obj) { return obj->refcount; }

/**
 * @brief add properties to an object, a property can optionally bind to an object's member
 * variable, in such case default AmlObjectClass.get_property and AmlObjectClass.set_property
 * will have enough information to handle the the properties, call of aml_obj_set_properties will
 * update the member variable and aml_obj_get_properties will return the member variable
 *
 * @param ocls, object class
 * @param prop, property
 * @param offset, property value pointer(offset in instance)
 * @param ... list of property and offset, NULL to terminate the list
 *
 * @return AML_STATUS_OK/AML_STATUS_FAIL
 */
AmlStatus aml_obj_add_properties(struct AmlObjectClass *ocls, struct AmlPropertyInfo *prop,
                                 int offset, ...);
// get/set multiple properties, the arguments is list of name,property pairs, the list is terminated
// by a NULL name
// i.e. aml_obj_get_properties(obj,"width",&w,"height",&h,NULL);
// aml_obj_set_properties(obj,"width",w,"height",h,NULL);
// aml_obj_set_properties_str(obj,"width","1920","height","1080",NULL);
AmlStatus aml_obj_get_properties(struct AmlObject *obj, void *name, ...);
AmlStatus aml_obj_set_properties(struct AmlObject *obj, void *name, ...);
AmlStatus aml_obj_set_properties_str(struct AmlObject *obj, void *name, ...);

// init typelibs, no need for gcc compiler, __attribute__((constructor)) will do this automatically
void aml_obj_typelib_init(struct AmlTypeClass *type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: AML_IPC_OBJECT_H */
