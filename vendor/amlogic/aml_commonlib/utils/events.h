//
// Input events.
//
#ifndef __EVENTS_H__
#define __EVENTS_H__

struct input_event;

// TODO: move these over to std::function.
typedef int (*ev_callback)(int fd, uint32_t epevents, void* data);
typedef int (*ev_set_key_callback)(int code, int value, void* data);

int ev_init(ev_callback input_cb, void* data);
void ev_exit();
int ev_add_fd(int fd, ev_callback cb, void* data);
int ev_sync_key_state(ev_set_key_callback set_key_cb, void* data);

// 'timeout' has the same semantics as poll(2).
//    0 : don't block
//  < 0 : block forever
//  > 0 : block for 'timeout' milliseconds
int ev_wait(int timeout);

int ev_get_input(int fd, uint32_t epevents, input_event* ev);
void ev_dispatch();
int ev_get_epollfd();

#endif
