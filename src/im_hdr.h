/*
 * im_hdr.h
 *
 *  Created on: Dec 4, 2013
 *      Author: reboot
 */

#ifndef IM_HDR_H_
#define IM_HDR_H_

#include <fp_types.h>

#define F_GH_NOMEM                      (a64 << 1)
#define F_GH_ISDIRLOG                   (a64 << 2)
#define F_GH_EXEC                       (a64 << 3)
#define F_GH_ISNUKELOG                  (a64 << 4)
#define F_GH_FFBUFFER                   (a64 << 5)
#define F_GH_WAPPEND                    (a64 << 6)
#define F_GH_DFWASWIPED                 (a64 << 7)
#define F_GH_DFNOWIPE                   (a64 << 8)
#define F_GH_ISDUPEFILE                 (a64 << 9)
#define F_GH_ISLASTONLOG                (a64 << 10)
#define F_GH_ISONELINERS                (a64 << 11)
#define F_GH_ONSHM                      (a64 << 12)
#define F_GH_ISONLINE                   (a64 << 13)
#define F_GH_ISIMDB                     (a64 << 14)
#define F_GH_ISGAME                     (a64 << 15)
#define F_GH_ISFSX                      (a64 << 16)
#define F_GH_ISTVRAGE                   (a64 << 17)
#define F_GH_ISGENERIC1                 (a64 << 18)
#define F_GH_SHM                        (a64 << 19)
#define F_GH_SHMRB                      (a64 << 20)
#define F_GH_SHMDESTROY                 (a64 << 21)
#define F_GH_SHMDESTONEXIT              (a64 << 22)
#define F_GH_FROMSTDIN                  (a64 << 23)
#define F_GH_HASLOM                     (a64 << 24)
#define F_GH_HASMATCHES                 (a64 << 25)
#define F_GH_HASEXC                     (a64 << 26)
#define F_GH_APFILT                     (a64 << 27)
#define F_GH_HASMAXRES                  (a64 << 28)
#define F_GH_HASMAXHIT                  (a64 << 29)
#define F_GH_IFRES                      (a64 << 30)
#define F_GH_IFHIT                      (a64 << 31)
#define F_GH_ISGENERIC2                 (a64 << 32)
#define F_GH_HASSTRM                    (a64 << 33)
#define F_GH_ISGENERIC3                 (a64 << 34)
#define F_GH_ISGENERIC4                 (a64 << 35)
#define F_GH_TOSTDOUT                   (a64 << 36)

/* these bits determine log type */
#define F_GH_ISTYPE                     (F_GH_ISGENERIC3|F_GH_ISGENERIC2|F_GH_ISGENERIC1|F_GH_ISNUKELOG|F_GH_ISDIRLOG|F_GH_ISDUPEFILE|F_GH_ISLASTONLOG|F_GH_ISONELINERS|F_GH_ISONLINE|F_GH_ISIMDB|F_GH_ISGAME|F_GH_ISFSX|F_GH_ISTVRAGE)

#define F_GH_ISSHM                      (F_GH_SHM|F_GH_ONSHM)
#define F_GH_ISMP                       (F_GH_HASMATCHES|F_GH_HASMAXRES|F_GH_HASMAXHIT)


typedef int
(*__d_exec)(void *buffer, void *callback, char *ex_str, void *hdl);

typedef struct ___execv
{
  int argc;
  char **argv, **argv_c;
  char exec_v_path[PATH_MAX];
  mda ac_ref;
  mda mech;
  __d_exec exc;
} _execv, *__execv;

typedef struct g_handle
{
  FILE *fh;
  off_t offset, bw, br, total_sz;
  off_t rw;
  uint32_t block_sz;
  uint64_t flags;
  mda buffer, w_buffer;
  mda _match_rr;
  off_t max_results, max_hits;
  __g_ipcbm ifrh_l0, ifrh_l1;
  _execv exec_args;
  mda print_mech;
  void *data;
  char s_buffer[PATH_MAX];
  char mv1_b[MAX_VAR_LEN];
  char file[PATH_MAX], mode[32];
  mode_t st_mode;
  key_t ipc_key;
  int shmid;
  struct shmid_ds ipcbuf;
  int
  (*g_proc0)(void *, char *, char *);
  __g_proc_v g_proc1_ps;
  __d_ref_to_pv g_proc2;
  __g_proc_v g_proc1_lookup;
  _d_proc3 g_proc3, g_proc3_batch, g_proc3_export;
  _d_omfp g_proc4;
  size_t j_offset, jm_offset;
  int d_memb;
  void *_x_ref;
} _g_handle, *__g_handle;

#endif /* IM_HDR_H_ */