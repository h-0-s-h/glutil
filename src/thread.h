/*
 * thread.h
 *
 *  Created on: Sep 14, 2012
 *      Author: reboot
 */

#ifdef _G_SSYS_THREAD

#ifndef THREAD_H_
#define THREAD_H_ 1

#include <memory_t.h>

#include <stdio.h>

#include <pthread.h>

#define F_THRD_TERM                     (a32 << 1)

#define THREAD_MAX	                50
#define	MAX_OBJ_GEN_WORKER_ARG	        256

#define THREAD_DEFAULT_BUFFER0_SIZE     8192

typedef struct gen_worker_arg
{
  int count;
} gwa, *p_gwa;

typedef struct object_gen_worker
{
  void *call;
  int a_count;
  void *arg;
} o_gw, *p_ogw;

typedef struct __thread_timers
{
  time_t t0, t1, t2;
} _thread_tm;

typedef struct object_thrd
{
  pthread_t pt;
  int id;
  int sig;
  uint32_t flags;
  mda in_objects, proc_objects;
  uint16_t role, oper_mode;
  void *buffer0;
  size_t buffer0_size;
  _thread_tm timers;
  pthread_mutex_t mutex;
} o_thrd, *po_thrd;

int
thread_create(void *call, int id, pmda _thrd_r, uint16_t role,
    uint16_t oper_mode);
int
mutex_init(pthread_mutex_t *mutex, int flags, int robust);

int
thread_destroy(p_md_obj ptr);

mda _net_thrd_r;

p_md_obj
search_thrd_id(pmda thread_r, pthread_t *pt);
int
thread_broadcast_kill(pmda thread_r);
off_t
thread_register_count(pmda thread_r);

int
spawn_threads(int num, void *call, int id, pmda thread_register, uint16_t role,
    uint16_t oper_mode);

void
mutex_lock(pthread_mutex_t *mutex);

typedef float
dt_score_pt(pmda in, pmda out, void *arg1, void *arg2);
typedef float
(*dt_score_ptp)(pmda in, pmda out, void *arg1, void *arg2);

int
push_object_to_thread(void *object, pmda threadr, dt_score_ptp scalc);

#endif /* THREAD_H_ */

#endif