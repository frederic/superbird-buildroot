BS DRM
===

BS DRM is a library to factor out most of the boilerplate found in programs
written against DRM.

Dependencies
---
- Clang or GCC with `-std=gnu99`
- `libgbm`
- `libdrm`

Features
---
- Opens DRM devices
- Creates display pipelines
- Allocates buffer objects with framebuffers
- Dumb maps buffer objects

Goals
---
(in order of priority)

- Useful
- Consistent and guessable API
- Easy to read
- Code is documentation
- 0 warnings from Clang or GCC
- Make errors easy to debug
- Fun

Code Philosophy
---
- This library is meant to be consumed by applications, not other libraries
- Applications usually can't recover from errors
- Corollary: returning error codes to applications is a waste of programmer time
- Badly behaving applications should fail fast
- Asserts fail fast
- Use asserts to make bad applications fail fast
- Make code style consistent with clang-format

Naming Rules
---
- File names: <module name>.c
- Functions: start with bs\_<name of file with extension>\_
- Constructor functions: suffix with "new"
- Destructor functions: suffix with "destroy"
- Conversion functions: suffix with "create"
- Functions that overload: put overload name at the very end

Build Example
---
Define `CC` `CFLAGS` and `LDFLAGS` for your device. Then run the following from the top level directory:
```
${CC} ${CFLAGS} -c src/drm_fb.c -o drm_fb.o || exit 1
${CC} ${CFLAGS} -c src/drm_open.c -o drm_open.o || exit 1
${CC} ${CFLAGS} -c src/drm_pipe.c -o drm_pipe.o || exit 1
${CC} ${CFLAGS} -c src/pipe.c -o pipe.o || exit 1
${CC} ${CFLAGS} -c example/stripe.c -o stripe.o || exit 1
${CC} ${LDFLAGS} \
    pipe.o \
    drm_pipe.o \
    drm_fb.o \
    drm_open.o \
    stripe.o -o bstest || exit 1
```

Clang Format Example
---
If you want to format `gamma_test.c`, first go to the drm-tests source directory and then run:

```
clang-format -style=file -i gamma_test.c
```

The `-style=file` argument will cause clang-format to search recursively upwards for the `.clang-format` style file, which is in drm-tests.

Upcoming Features
---
- Vsync
- Atomic Commit
- Multi-card and multi-monitor
- Keyboard/Mouse input for debugging
- Dump buffer output for debugging

What does BS stand for?
---
BS does't stand for anything.
