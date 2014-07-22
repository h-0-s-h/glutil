/*
 * lref_altlog.c
 *
 *  Created on: Jul 22, 2014
 *      Author: reboot
 */



#include <glutil.h>
#include "config.h"
#include <xref.h>

#include <lref.h>

#include <str.h>
#include <lref_gen.h>
#include "lref_altlog.h"
#include <misc.h>
#include <omfp.h>

#include <errno.h>
#include <time.h>


void
dt_set_altlog(__g_handle hdl)
{
  hdl->flags |= F_GH_IS_ALTLOG;
  hdl->block_sz = AL_SZ;
  hdl->d_memb = 7;
  hdl->g_proc0 = gcb_altlog;
  hdl->g_proc1_lookup = ref_to_val_lk_altlog;
  hdl->g_proc2 = ref_to_val_ptr_altlog;
  hdl->g_proc3 = altlog_format_block;
  hdl->g_proc3_batch = altlog_format_block_batch;
  hdl->g_proc3_export = altlog_format_block_exp;
  hdl->g_proc4 = g_omfp_norm;
  hdl->ipc_key = IPC_KEY_ALTLOG;
  hdl->jm_offset = (size_t) &((struct altlog*) NULL)->dirname;
}

char *
dt_rval_altlog_user(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct altlog *) arg)->uploader);
  return output;
}

char *
dt_rval_altlog_group(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct altlog *) arg)->group);
  return output;
}

char *
dt_rval_altlog_files(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct altlog *) arg)->files);
  return output;
}

char *
dt_rval_altlog_size(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (ulint64_t) ((struct altlog *) arg)->bytes);
  return output;
}

char *
dt_rval_altlog_status(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      ((struct altlog *) arg)->status);
  return output;
}

char *
dt_rval_altlog_time(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  snprintf(output, max_size, ((__d_drt_h ) mppd)->direc,
      (int32_t) ((struct altlog *) arg)->uptime);
  return output;
}

char *
dt_rval_altlog_mode_e(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  g_l_fmode(((struct altlog *) arg)->dirname, max_size, output);
  return output;
}

char *
dt_rval_altlog_dir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return ((struct altlog *) arg)->dirname;
}

char *
dt_rval_altlog_basedir(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  return g_basename(((struct altlog *) arg)->dirname);
}

char *
dt_rval_xg_altlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  snprintf(xref_t.name, sizeof(xref_t.name), "%s%s", GLROOT,
      ((struct altlog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[3], output, max_size, mppd);
  return output;
}

char *
dt_rval_x_altlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  _d_xref xref_t =
    {
      { 0 } };
  strcp_s(xref_t.name, sizeof(xref_t.name), ((struct altlog *) arg)->dirname);
  ref_to_val_x((void*) &xref_t, &match[2], output, max_size, mppd);
  return output;
}

void *
ref_to_val_lk_altlog(void *arg, char *match, char *output, size_t max_size,
    void *mppd)
{
  void *ptr;
  if ((ptr = ref_to_val_lk_generic(NULL, match, output, max_size, mppd)))
    {
      return ptr;
    }

  if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_user, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_GLOB_BASEDIR, 7))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_basedir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_DIR, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_dir, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_group, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_ALTLOG_FILES, 5))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_files, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_size, (__d_drt_h ) mppd,
          "%llu");
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_status, (__d_drt_h ) mppd,
          "%hu");
    }
  else if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_time, (__d_drt_h ) mppd,
          "%d");
    }
  else if (!strncmp(match, _MC_GLOB_MODE, 4))
    {
      return as_ref_to_val_lk(match, dt_rval_altlog_mode_e, (__d_drt_h ) mppd,
          "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XREF, 2))
    {
      return as_ref_to_val_lk(match, dt_rval_x_altlog, (__d_drt_h ) mppd, "%s");
    }
  else if (!strncmp(match, _MC_GLOB_XGREF, 3))
    {
      return as_ref_to_val_lk(match, dt_rval_xg_altlog, (__d_drt_h ) mppd, "%s");
    }

  return NULL;
}


int
altlog_format_block(void *iarg, char *output)
{
  char buffer2[255];

  struct altlog *data = (struct altlog *) iarg;

  char *base = NULL;

  if (gfl & F_OPT_VERBOSE)
    {
      base = data->dirname;
    }
  else
    {
      base = g_basename(data->dirname);
    }

  time_t t_t = (time_t) data->uptime;

  strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

  return print_str(
      "ALTLOG: %s - %llu Mbytes in %hu files - created %s by %hu.%hu [%hu]\n",
      base, (ulint64_t) (data->bytes / 1048576), data->files, buffer2,
      data->uploader, data->group, data->status);
}

int
altlog_format_block_batch(void *iarg, char *output)
{
  struct altlog *data = (struct altlog *) iarg;
  return printf("ALTLOG\x9%s\x9%llu\x9%hu\x9%d\x9%hu\x9%hu\x9%hu\n",
      data->dirname, (ulint64_t) data->bytes, data->files,
      (int32_t) data->uptime, data->uploader, data->group, data->status);

}

int
altlog_format_block_exp(void *iarg, char *output)
{
  struct altlog *data = (struct altlog *) iarg;
  return printf("dir %s\n"
      "size %llu\n"
      "files %hu\n"
      "time %d\n"
      "user %hu\n"
      "group %hu\n"
      "status %hu\n\n", data->dirname, (ulint64_t) data->bytes, data->files,
      (int32_t) data->uptime, data->uploader, data->group, data->status);
}


int
gcb_altlog(void *buffer, char *key, char *val)
{
  size_t k_l = strlen(key), v_l;
  errno = 0;

  struct altlog * ptr = (struct altlog *) buffer;
  if (k_l == 3 && !strncmp(key, _MC_GLOB_DIR, 3))
    {
      if (!(v_l = strlen(val)))
        {
          return 0;
        }
      memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_USER, 4))
    {
      uint16_t v_i = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->uploader = v_i;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_GLOB_GROUP, 5))
    {
      uint16_t v_i = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->group = v_i;
      return 1;
    }
  else if (k_l == 5 && !strncmp(key, _MC_ALTLOG_FILES, 5))
    {
      uint16_t k_ui = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->files = k_ui;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_SIZE, 4))
    {
      ulint64_t k_uli = (ulint64_t) strtoll(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->bytes = k_uli;
      return 1;
    }
  else if (k_l == 4 && !strncmp(key, _MC_GLOB_TIME, 4))
    {
      int32_t k_ui = (int32_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->uptime = k_ui;
      return 1;
    }
  else if (k_l == 6 && !strncmp(key, _MC_GLOB_STATUS, 6))
    {
      uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
      if ( errno == ERANGE)
        {
          return 0;
        }
      ptr->status = k_us;
      return 1;
    }
  return 0;
}


void *
ref_to_val_ptr_altlog(void *arg, char *match, int *output)
{
  struct altlog *data = (struct altlog *) arg;

  if (!strncmp(match, _MC_GLOB_TIME, 4))
    {
      *output = ~((int) sizeof(data->uptime));
      return &data->uptime;
    }
  else if (!strncmp(match, _MC_GLOB_USER, 4))
    {
      *output = sizeof(data->uploader);
      return &data->uploader;
    }
  else if (!strncmp(match, _MC_GLOB_GROUP, 5))
    {
      *output = sizeof(data->group);
      return &data->group;
    }
  else if (!strncmp(match, _MC_ALTLOG_FILES, 5))
    {
      *output = sizeof(data->files);
      return &data->files;
    }
  else if (!strncmp(match, _MC_GLOB_SIZE, 4))
    {
      *output = sizeof(data->bytes);
      return &data->bytes;
    }
  else if (!strncmp(match, _MC_GLOB_STATUS, 6))
    {
      *output = sizeof(data->status);
      return &data->status;
    }
  return NULL;
}
