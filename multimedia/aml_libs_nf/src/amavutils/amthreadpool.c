/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */


#include <amthreadpool.h>
#include <itemlist.h>

#define LOG_TAG "amthreadpool"
#include <cutils/log.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static struct itemlist threadpool_list;
static struct itemlist threadpool_threadlist;
#define MAX_THREAD_DEPTH 8

#define T_ASSERT_TRUE(x)\
    do {\
    if (!(x))\
        ALOGE("amthreadpool error at %d\n",__LINE__);\
    } while(0)

#define T_ASSERT_NO_NULL(p) T_ASSERT_TRUE((p)!=NULL)

typedef struct threadpool {
    pthread_t pid;
    struct itemlist threadlist;
} threadpool_t;

typedef struct threadpool_thread_data {
    pthread_t pid;
    void * (*start_routine)(void *);
    void * arg;
    pthread_t ppid[MAX_THREAD_DEPTH];
    threadpool_t *pool;
    pthread_mutex_t pthread_mutex;
    pthread_cond_t pthread_cond;
    int on_requred_exit;
    int thread_inited;
} threadpool_thread_data_t;
#define POOL_OF_ITEM(item) ((threadpool_t *)(item)->extdata[0])
#define THREAD_OF_ITEM(item) ((threadpool_thread_data_t *)(item)->extdata[0])

static int amthreadpool_release(pthread_t  pid);
static threadpool_t * amthreadpool_create_pool(pthread_t  pid);

/*
static threadpool_t * amthreadpool_findthead_pool(pthread_t  pid)
{
    struct item *item;
    item = itemlist_find_match_item(&threadpool_list, pid);
    if (item) {
        return (threadpool_t *)item->extdata[0];
    }
    return NULL;
}
*/

static threadpool_thread_data_t * amthreadpool_findthead_thread_data(pthread_t pid)
{
    struct item *item;
    item = itemlist_find_match_item(&threadpool_threadlist, pid);
    if (item) {
        return (threadpool_thread_data_t *)item->extdata[0];
    }
    return NULL;
}

/*creat thread pool  for main thread*/
static threadpool_t * amthreadpool_create_pool(pthread_t pid)
{
    //struct item *poolitem;
    threadpool_t *pool;
    //int ret = -1;
    unsigned long exdata[2];
    pool = malloc(sizeof(threadpool_t));
    if (!pool) {
        ALOGE("malloc pool data failed\n");
        return NULL;
    }
    memset(pool, 0, sizeof(threadpool_t));
    pool->pid = pid;
    if (pid == 0) {
        pool->pid = pthread_self();
    }
    pool->threadlist.max_items = 0;
    pool->threadlist.item_ext_buf_size = 0;
    pool->threadlist.muti_threads_access = 1;
    pool->threadlist.reject_same_item_data = 1;
    itemlist_init(&pool->threadlist);
    exdata[0] = (unsigned long)pool;
    itemlist_add_tail_data_ext(&threadpool_list, pool->pid, 1, exdata);

    return pool;
}

static int amthreadpool_pool_add_thread(threadpool_t *pool, unsigned long pid, threadpool_thread_data_t* thread)
{
    int ret = 0;
    unsigned long exdata[2];
    exdata[0] = (unsigned long)thread;
    if (pool) {
        ret = itemlist_add_tail_data_ext(&pool->threadlist, thread->pid, 1, exdata);
    } else {
        pool = amthreadpool_create_pool(pid);
        thread->pool = pool;
    }
    ret |= itemlist_add_tail_data_ext(&threadpool_threadlist, thread->pid, 1, exdata);
    return ret;
}
static int amthreadpool_pool_del_thread(pthread_t pid)
{
    threadpool_t *pool;
    threadpool_thread_data_t* t1, *t2;
    int ret = 0;
    struct item *item;
    item = itemlist_get_match_item(&threadpool_threadlist, pid);
    if (!item) {
        return -2;    /*freed before*/
    }
    t1 = THREAD_OF_ITEM(item);
    item_free(item);
    T_ASSERT_NO_NULL(t1);
    pool = t1->pool;
    T_ASSERT_NO_NULL(pool);
    item = itemlist_get_match_item(&pool->threadlist, pid);
    if (item) {
        T_ASSERT_NO_NULL(item);
        t2 = THREAD_OF_ITEM(item);
        T_ASSERT_NO_NULL(t2);
        if (t1 != t2) {
            ALOGE("%d thread data not mached, %p!=%p\n", (int)pid, t1, t2);
        }
        item_free(item);
    }
    pthread_cond_destroy(&t1->pthread_cond);
    pthread_mutex_destroy(&t1->pthread_mutex);
    free(t1);
    if (pool->pid == pid) {
        amthreadpool_release(pid);
    }
    return ret;
}


static int amthreadpool_thread_wake_t(threadpool_thread_data_t*t, int trycancel)
{
    int ret;
    pthread_mutex_lock(&t->pthread_mutex);
    t->on_requred_exit = trycancel;
    ret = pthread_cond_signal(&t->pthread_cond);
    pthread_mutex_unlock(&t->pthread_mutex);
    return ret;
}
static int64_t amthreadpool_gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int amthreadpool_thread_usleep_in_monotonic(int us)
{
/* 64bit compiler do not have pthread_cond_timedwait_monotonic_np */
#ifndef __aarch64__
    pthread_t pid = pthread_self();
    struct timespec pthread_ts;
    int64_t us64 = us;
    threadpool_thread_data_t *t = amthreadpool_findthead_thread_data(pid);
    int ret = 0;
    if (!t) {
        ///ALOGE("%lu thread sleep data not found!!!\n", pid);
        usleep(us);//for not deadlock.
        return 0;
    }
    if (t->on_requred_exit > 1) {
        if (us64 < 100 * 1000) {
            us64 = 100 * 1000;
        }
        t->on_requred_exit--; /*if on_requred_exit,do less sleep till 1.*/
    }
#if defined(__LP32__) && __ANDROID_API__ < 21
    struct timespec tnow;
    clock_gettime(CLOCK_MONOTONIC, &tnow);
    pthread_ts.tv_sec = tnow.tv_sec + (us64 + tnow.tv_nsec / 1000) / 1000000;
    pthread_ts.tv_nsec = (us64 * 1000 + tnow.tv_nsec) % 1000000000;
    pthread_mutex_lock(&t->pthread_mutex);
    ret = pthread_cond_timedwait_monotonic_np(&t->pthread_cond, &t->pthread_mutex, &pthread_ts);
#else
    struct timeval tnow;
    ret = gettimeofday(&tnow, NULL);
    pthread_ts.tv_sec = tnow.tv_sec + (us64 + tnow.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((us64 + tnow.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&t->pthread_mutex);
    ret = pthread_cond_timedwait(&t->pthread_cond, &t->pthread_mutex, &pthread_ts);
#endif
    pthread_mutex_unlock(&t->pthread_mutex);
    return ret;
#else
    usleep(us);//for not deadlock.
    return 0;
#endif

}


int amthreadpool_thread_usleep_in(int us)
{
    pthread_t pid = pthread_self();
    struct timespec pthread_ts;
    struct timeval now;
    int64_t us64 = us;
    threadpool_thread_data_t *t = amthreadpool_findthead_thread_data(pid);
    int ret = 0;

    if (!t) {
        ///ALOGE("%lu thread sleep data not found!!!\n", pid);
        usleep(us);//for not deadlock.
        return 0;
    }
    if (t->on_requred_exit > 1) {
        if (us64 < 100 * 1000) {
            us64 = 100 * 1000;
        }
        t->on_requred_exit--; /*if on_requred_exit,do less sleep till 1.*/
    }
    ret = gettimeofday(&now, NULL);
    pthread_ts.tv_sec = now.tv_sec + (us64 + now.tv_usec) / 1000000;
    pthread_ts.tv_nsec = ((us64 + now.tv_usec) * 1000) % 1000000000;
    pthread_mutex_lock(&t->pthread_mutex);
    ret = pthread_cond_timedwait(&t->pthread_cond, &t->pthread_mutex, &pthread_ts);
    pthread_mutex_unlock(&t->pthread_mutex);
    return ret;
}
int amthreadpool_thread_usleep_debug(int us, const char *func, int line)
{

    int64_t starttime = amthreadpool_gettime();
    int64_t endtime;
    int ret;
    (void *) func;
     (void *) line;

#ifdef AMTHREADPOOL_SLEEP_US_MONOTONIC
    ret = amthreadpool_thread_usleep_in_monotonic(us);
#else
    ret = amthreadpool_thread_usleep_in(us);
#endif
    endtime = amthreadpool_gettime();
    if ((endtime - starttime - us) > 100 * 1000) {
        //ALOGE("***amthreadpool_thread_usleep wast more time wait %d us, real %lld us\n", us, (int64_t)(endtime - starttime));
    }
    return ret;
}

int amthreadpool_thread_wake(pthread_t pid)
{
    threadpool_thread_data_t *t = amthreadpool_findthead_thread_data(pid);
    if (!t) {
        ALOGE("%lu wake thread data not found!!!\n", pid);
        return -1;
    }
    return amthreadpool_thread_wake_t(t, t->on_requred_exit);
}
int amthreadpool_on_requare_exit(pthread_t pid)
{
    unsigned long rpid = pid != 0 ? pid : pthread_self();
    threadpool_thread_data_t *t = amthreadpool_findthead_thread_data(rpid);
    if (!t) {
        return 0;
    }
    if (t->on_requred_exit) {
        ///ALOGI("%lu name  on try exit.\n", pid);
    }
    return !!t->on_requred_exit;
}

static int amthreadpool_pool_thread_cancel_l1(pthread_t pid, int cancel, int allthreads)
{
    struct itemlist *itemlist;
    threadpool_thread_data_t *t, *t1;
    struct item *item = NULL;
    threadpool_t *pool;
    t = amthreadpool_findthead_thread_data(pid);
    if (!t) {
        ALOGE("%lu pool data not found!!!\n", pid);
        return 0;
    }
    pool = t->pool;
    if (allthreads && pool && pool->pid == pid) {
        itemlist = &pool->threadlist;
        FOR_EACH_ITEM_IN_ITEMLIST(itemlist, item)
        t1 = THREAD_OF_ITEM(item);
        amthreadpool_thread_wake_t(t1, cancel);
        FOR_ITEM_END(itemlist);
    }
    amthreadpool_thread_wake_t(t, cancel);
    ///amthreadpool_system_dump_info();
    return 0;
}

int amthreadpool_pool_thread_cancel(pthread_t pid)
{
    return amthreadpool_pool_thread_cancel_l1(pid, 3, 1);
}

int amthreadpool_pool_thread_uncancel(pthread_t pid)
{
    return amthreadpool_pool_thread_cancel_l1(pid, 0, 1);
}
int amthreadpool_thread_cancel(pthread_t pid)
{
    return amthreadpool_pool_thread_cancel_l1(pid, 3, 0);
}
int amthreadpool_thread_uncancel(pthread_t pid)
{
    return amthreadpool_pool_thread_cancel_l1(pid, 0, 0);
}

static int amthreadpool_release(pthread_t pid)
{
    struct item *poolitem;
    threadpool_t *pool;
    poolitem = itemlist_get_match_item(&threadpool_list, pid);
    if (poolitem) {
        pool = POOL_OF_ITEM(poolitem);
        itemlist_deinit(&pool->threadlist);
        free((void *)pool);
        item_free(poolitem);
    }
    return 0;
}


void * amthreadpool_start_thread(void *arg)
{
    void *ret;
    threadpool_thread_data_t *t = (threadpool_thread_data_t *)arg;
    {
        threadpool_thread_data_t *thread_p;
        threadpool_t *pool = NULL;
        int i;
        t->pid = pthread_self();
        thread_p = amthreadpool_findthead_thread_data(t->ppid[0]);
        if (thread_p) {
            pool = thread_p->pool;
            for (i = 0; i < MAX_THREAD_DEPTH - 1; i++) {
                if (!thread_p->ppid[i]) {
                    break;
                }
                t->ppid[i + 1] = thread_p->ppid[i];
            }
            t->pool = pool;

        }
        amthreadpool_pool_add_thread(pool, t->pid, t);
    }
    pthread_mutex_lock(&t->pthread_mutex);
    t->thread_inited = 1;
    pthread_cond_signal(&t->pthread_cond);
    pthread_mutex_unlock(&t->pthread_mutex);
    ret = t->start_routine(t->arg);
    return ret;
}

int amthreadpool_pthread_create_name(pthread_t * newthread,
                                     __const pthread_attr_t * attr,
                                     void * (*start_routine)(void *),
                                     void * arg, const char *name)
{
    pthread_t pid = pthread_self();
    pthread_t subpid;
    int ret;

    threadpool_thread_data_t *t = malloc(sizeof(threadpool_thread_data_t));
    if (!t) {
        ALOGE("malloc threadpool_thread_data_t data failed\n");
        return -100;
    }
    memset(t, 0, sizeof(threadpool_thread_data_t));
    t->start_routine = start_routine;
    t->arg = arg;
    t->ppid[0] = pid;
    t->thread_inited = 0;
    pthread_mutex_init(&t->pthread_mutex, NULL);
    pthread_cond_init(&t->pthread_cond, NULL);
    ret = pthread_create(&subpid, attr, amthreadpool_start_thread, (void *)t);
    if (ret == 0) {
        *newthread = subpid;
        if (name) {
            pthread_setname_np(pid, name);
        }
        pthread_mutex_lock(&t->pthread_mutex);
        while (t->thread_inited == 0)
            pthread_cond_wait(&t->pthread_cond, &t->pthread_mutex);
        pthread_mutex_unlock(&t->pthread_mutex);
    }
    return ret;
}



int amthreadpool_pthread_create(pthread_t * newthread,
                                __const pthread_attr_t * attr,
                                void * (*start_routine)(void *),
                                void * arg)
{
    return amthreadpool_pthread_create_name(newthread, attr, start_routine, arg, NULL);
}

int amthreadpool_pthread_join(pthread_t thid, void ** ret_val)
{
    int ret;
    ret = pthread_join(thid, ret_val);
    amthreadpool_pool_del_thread(thid);
    return ret;
}





/*creat thread pool  system init*/
int amthreadpool_system_init(void)
{
    static int inited = 0;
    if (inited) {
        return 0;
    }
    inited ++;
    threadpool_list.max_items = 0;
    threadpool_list.item_ext_buf_size = 0;
    threadpool_list.muti_threads_access = 1;
    threadpool_list.reject_same_item_data = 1;
    itemlist_init(&threadpool_list);

    threadpool_threadlist.max_items = 0;
    threadpool_threadlist.item_ext_buf_size = 0;
    threadpool_threadlist.muti_threads_access = 1;
    threadpool_threadlist.reject_same_item_data = 1;
    itemlist_init(&threadpool_threadlist);
    return 0;
}


int amthreadpool_system_dump_info(void)
{
    threadpool_thread_data_t *t;
    threadpool_t *pool;
    struct item *item = NULL;
    struct item *item1 = NULL;
    ALOGI("------------amthreadpool_system_dump_info----------START\n");
    ALOGI("pool & threads:\n");
    FOR_EACH_ITEM_IN_ITEMLIST(&threadpool_list, item) {
        pool = POOL_OF_ITEM(item);
        ALOGI("pool:%p\n", pool);
        ALOGI("--tpid:%lu\n", pool->pid);
        FOR_EACH_ITEM_IN_ITEMLIST(&pool->threadlist, item1) {
            t = THREAD_OF_ITEM(item1);
            ALOGI("--tpid:%lu\n", t->pid);
            //ALOGI("----name=%p\n",amthreadpool_thread_name(t->pid));
            ALOGI("----ppid=%lu,%lu,%lu,%lu,%lu", t->ppid[0], t->ppid[1], t->ppid[2], t->ppid[3], t->ppid[4]);
            ALOGI("----pool:%p\n", t->pool);
            ALOGI("----on_requred_exit:%d\n", t->on_requred_exit);
        }
        FOR_ITEM_END(&pool->threadlist);
    }
    FOR_ITEM_END(&threadpool_list);
    ALOGI("all threads:\n");
    FOR_EACH_ITEM_IN_ITEMLIST(&threadpool_threadlist, item) {
        t = THREAD_OF_ITEM(item);
        ALOGI("--tpid:%lu\n", t->pid);
        //ALOGI("----name=%p\n",amthreadpool_thread_name(t->pid));
        ALOGI("----ppid=%lu,%lu,%lu,%lu,%lu", t->ppid[0], t->ppid[1], t->ppid[2], t->ppid[3], t->ppid[4]);
        ALOGI("----pool:%p\n", t->pool);
        ALOGI("----on_requred_exit:%d\n", t->on_requred_exit);
    }
    FOR_ITEM_END(&threadpool_threadlist);
    ALOGI("------------amthreadpool_system_dump_info----------END\n");
    return 0;
}
