/*
 * ============================================================================
 * Name        : glutil
 * Authors     : nymfo, siska
 * Version     : 1.9-15
 * Description : glFTPd binary logs utility
 * ============================================================================
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#define _LARGEFILE64_SOURCE 1
#define _LARGEFILE_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <inttypes.h>
#include <time.h>

/* requires 'native' glconf.h (/glroot/bin/sources) */
#include "glconf.h"

/* gl root  */
#ifndef glroot
#define glroot "/glftpd"
#endif

/* site root, relative to gl root */
#ifndef siteroot
#define siteroot "/site"
#endif

/* ftp-data, relative to gl root */
#ifndef ftp_data
#define ftp_data "/ftp-data"
#endif

/* glftpd's user segment ipc key */
#ifndef shm_ipc
#define shm_ipc 0x0000DEAD
#endif

/* glutil's ipc keys */
#define IPC_KEY_DIRLOG		0xDEAD1000
#define IPC_KEY_NUKELOG		0xDEAD1100
#define IPC_KEY_DUPEFILE	0xDEAD1200
#define IPC_KEY_LASTONLOG	0xDEAD1300
#define IPC_KEY_ONELINERS	0xDEAD1400
#define IPC_KEY_IMDBLOG 	0xDEAD1500
#define IPC_KEY_GAMELOG 	0xDEAD1600
#define IPC_KEY_TVRAGELOG 	0xDEAD1700
#define IPC_KEY_GEN1LOG 	0xDEAD1800

/*
 * log file path
 * setting this variable enables logging (default is off)
 */
#ifndef log_file
#define log_file ""
#endif

/* dirlog file path */
#ifndef dir_log
#define dir_log "/glftpd/ftp-data/logs/dirlog"
#endif

/* nukelog file path */
#ifndef nuke_log
#define nuke_log "/glftpd/ftp-data/logs/nukelog"
#endif

/* dupe file path */
#ifndef dupe_file
#define dupe_file "/glftpd/ftp-data/logs/dupefile"
#endif

/* last-on log file path */
#ifndef last_on_log
#define last_on_log "/glftpd/ftp-data/logs/laston.log"
#endif

/* oneliner file path */
#ifndef oneliner_file
#define oneliner_file "/glftpd/ftp-data/logs/oneliners.log"
#endif

/* imdb log file path */
#ifndef imdb_file
#define imdb_file "/glftpd/ftp-data/logs/imdb.log"
#endif

/* game log file path */
#ifndef game_log
#define game_log "/glftpd/ftp-data/logs/game.log"
#endif

/* tv log file path */
#ifndef tv_log
#define tv_log "/glftpd/ftp-data/logs/tv.log"
#endif

/* generic 1 log file path */
#ifndef ge1_log
#define ge1_log "/glftpd/ftp-data/logs/gen1.log"
#endif

/* see README file about this */
#ifndef du_fld
#define du_fld "/glftpd/bin/glutil.folders"
#endif

/* file extensions to skip generating crc32 (SFV mode)*/
#ifndef PREG_SFV_SKIP_EXT
#define PREG_SFV_SKIP_EXT "\\.(nfo|sfv(\\.tmp|))$"
#endif

/* -------------------------- */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <sys/wait.h>

#ifndef WEXITSTATUS
#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#endif

#define VER_MAJOR 1
#define VER_MINOR 9
#define VER_REVISION 15
#define VER_STR ""

#ifndef _STDINT_H
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
#ifndef __uint32_t_defined
typedef unsigned int uint32_t;
# define __uint32_t_defined
#endif
#if __WORDSIZE == 64
typedef unsigned long int uint64_t;
#else
__extension__
typedef unsigned long long int uint64_t;
#endif
#endif

typedef unsigned long long int ulint64_t;

#if __x86_64__ || __ppc64__
#define uintaa_t uint64_t
#define ENV_64
#define __STR_ARCH	"x86_64"
#define __AA_SPFH 	"%.16X"
#else
#define uintaa_t uint32_t
#define ENV_32
#define __STR_ARCH	"i686"
#define __AA_SPFH 	"%.8X"
#endif

#define MAX_uint64_t 		((uint64_t) -1)
#define MAX_uint32_t 		((uint32_t) -1)

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define a64					((uint64_t) 1)
#define a32					((uint32_t) 1)

/* ------------------------------------------- */

#pragma pack(push, 4)

typedef struct ___d_imdb {
	char dirname[255];
	int32_t timestamp;
	char imdb_id[64]; /* IMDB ID */
	float rating; /* IMDB Rating */
	uint32_t votes; /* IMDB Votes */
	char genres[255]; /* List of genres (comma delimited) */
	char year[6];
	char title[128];
	int32_t released;
	int32_t runtime;
	char rated[8];
	char actors[128];
	char director[64];
	/* ------------- */
	char _d_unused[230]; /* Reserved for future use */

} _d_imdb, *__d_imdb;

typedef struct ___d_game {
	char dirname[255];
	uint32_t timestamp;
	float rating;
	/* ------------- */
	char _d_unused[512]; /* Reserved for future use */

} _d_game, *__d_game;

typedef struct ___d_tvrage {
	char dirname[255];
	uint32_t timestamp;
	uint32_t showid;
	char name[128];
	char link[128];
	char country[24];
	uint32_t started;
	uint32_t ended;
	uint16_t seasons;
	char status[64];
	uint32_t runtime;
	char airtime[6];
	char airday[32];
	char class[64];
	char genres[256];
	char _d_unused[256]; /* Reserved for future use */
} _d_tvrage, *__d_tvrage;

typedef struct ___d_generic_s2044 {
	uint32_t i32;
	char s_1[255];
	char s_2[255];
	char s_3[255];
	char s_4[255];
	char s_5[255];
	char s_6[255];
	char s_7[255];
	char s_8[255];
} _d_generic_s2044, *__d_generic_s2044;

#pragma pack(pop)

/* ------------------------------------------- */

typedef struct e_arg {
	int depth;
	uint32_t flags;
	char buffer[PATH_MAX];
	char buffer2[PATH_MAX + 10];
	struct dirlog *dirlog;
	time_t t_stor;
} ear;

typedef struct option_reference_array {
	char *option;
	void *function, *arg_cnt;
}*p_ora;

struct d_stats {
	uint64_t bw, br, rw;
};

typedef struct mda_object {
	void *ptr;
	void *next;
	void *prev;
//	unsigned char flags;
}*p_md_obj, md_obj;

#define F_MDA_REFPTR		(a32 << 1)
#define F_MDA_FREE			(a32 << 2)
#define F_MDA_REUSE			(a32 << 3)
#define F_MDA_WAS_REUSED	(a32 << 4)
#define F_MDA_EOF			(a32 << 5)
#define F_MDA_FIRST_REUSED  (a32 << 6)

typedef struct mda_header {
	p_md_obj objects; /* holds references */
	p_md_obj pos, r_pos, c_pos, first, last;
	off_t offset, r_offset, count, hitcnt, rescnt;
	uint32_t flags;
	void *lref_ptr;
} mda, *pmda;

typedef struct config_header {
	char *key, *value;
	mda data;
} cfg_h, *p_cfg_h;

typedef int (*__d_exec)(void *buffer, void *callback, char *ex_str, void *hdl);

typedef struct ___execv {
	int argc;
	char **argv, **argv_c;
	char exec_v_path[PATH_MAX];
	mda ac_ref;
	__d_exec exc;
} _execv, *__execv;

typedef struct g_handle {
	FILE *fh;
	off_t offset, bw, br, total_sz;
	off_t rw;
	uint32_t block_sz, flags;
	mda buffer, w_buffer;
	mda _match_rr;
	off_t max_results, max_hits;
	_execv exec_args;
	void *data;
	char s_buffer[PATH_MAX];
	char file[PATH_MAX], mode[32];
	mode_t st_mode;
	key_t ipc_key;
	int shmid;
	struct shmid_ds ipcbuf;
	int (*g_proc0)(void *, char *, char *);
	int (*g_proc1)(void *, char *, char *, size_t);
	void *(*g_proc2)(void *, char *, size_t*);
	int (*g_proc3)(void *, char *);
	size_t j_offset;
	int d_memb;
} _g_handle, *__g_handle;

typedef struct g_cfg_ref {
	mda cfg;
	char file[PATH_MAX];
} cfg_r, *p_cfg_r;

typedef struct ___si_argv0 {
	int ret;
	uint32_t flags;
	char p_buf_1[4096];
	char p_buf_2[PATH_MAX];
	char s_ret[262144];
} _si_argv0, *__si_argv0;

typedef struct sig_jmp_buf {
	sigjmp_buf env, p_env;
	uint32_t flags, pflags;
	int id, pid;
	unsigned char ci, pci;
	char type[32];
	void *callback, *arg;
} sigjmp, *p_sigjmp;

typedef float (*g_tf_p)(void *base, size_t offset);
typedef uint64_t (*g_t_p)(void *base, size_t offset);
typedef int (*g_op)(int s, int d);

typedef struct ___g_match_h {
	uint32_t flags;
	char *match, *field;
	int reg_i_m, match_i_m, regex_flags;
	regex_t preg;
	mda lom;
	g_op g_oper_ptr;
	char data[5120];
	char b_data[5120];
} _g_match, *__g_match;

typedef struct ___g_lom {
	int result;
	/* --- */
	uint32_t flags;
	g_t_p g_t_ptr_left;
	g_t_p g_t_ptr_right;
	g_tf_p g_tf_ptr_left;
	g_tf_p g_tf_ptr_right;
	int (*g_icomp_ptr)(uint64_t s, uint64_t d);
	int (*g_fcomp_ptr)(float s, float d);
	g_op g_oper_ptr;
	uint64_t t_left, t_right;
	float tf_left, tf_right;
	/* --- */
	size_t t_l_off, t_r_off;
} _g_lom, *__g_lom;

typedef struct ___g_eds {
	uint32_t flags;
	uint32_t r_minor, r_major;
	struct stat st;
} _g_eds, *__g_eds;

/*
 * CRC-32 polynomial 0x04C11DB7 (0xEDB88320)
 * see http://en.wikipedia.org/wiki/Cyclic_redundancy_check#Commonly_used_and_standardized_CRCs
 */

static uint32_t crc_32_tab[] = { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
		0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB,
		0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E,
		0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75,
		0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808,
		0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
		0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162,
		0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49,
		0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC,
		0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3,
		0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
		0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D,
		0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
		0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767,
		0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A,
		0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
		0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14,
		0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B,
		0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE,
		0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
		0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
		0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };

#define UPDC32(octet,crc) (crc_32_tab[((crc) ^ ((uint8_t)octet)) & 0xff] ^ ((crc) >> 8))

uint32_t crc32(uint32_t crc32, uint8_t *buf, size_t len) {
	uint32_t oldcrc32;

	if (crc32) {
		oldcrc32 = crc32;
	} else {
		oldcrc32 = MAX_uint32_t;
	}

	for (; len; len--, buf++) {
		oldcrc32 = UPDC32(*buf, oldcrc32);
	}

	return ~oldcrc32;
}

#define PTRSZ 				(sizeof(void*))

#define REG_MATCHES_MAX	 	4

#define UPD_MODE_RECURSIVE 		0x1
#define UPD_MODE_SINGLE			0x2
#define UPD_MODE_CHECK			0x3
#define UPD_MODE_DUMP			0x4
#define UPD_MODE_DUMP_NUKE		0x5
#define UPD_MODE_DUPE_CHK		0x6
#define UPD_MODE_REBUILD		0x7
#define UPD_MODE_DUMP_DUPEF 	0x8
#define UPD_MODE_DUMP_LON		0x9
#define UPD_MODE_DUMP_ONEL		0xA
#define UPD_MODE_DUMP_ONL		0xB
#define UPD_MODE_NOOP			0xC
#define UPD_MODE_MACRO			0xD
#define UPD_MODE_FORK			0xE
#define UPD_MODE_BACKUP			0xF
#define UPD_MODE_DUMP_USERS		0x10
#define UPD_MODE_DUMP_GRPS		0x11
#define UPD_MODE_DUMP_GEN		0x12
#define UPD_MODE_WRITE			0x13
#define UPD_MODE_DUMP_IMDB		0x14
#define UPD_MODE_DUMP_GAME		0x15
#define UPD_MODE_DUMP_TV		0x16
#define UPD_MODE_DUMP_GENERIC	0x17

#define PRIO_UPD_MODE_MACRO 	0x1001
#define PRIO_UPD_MODE_INFO 		0x1002

/* -- flags -- */

#define F_OPT_FORCE 			(a64 << 1)
#define F_OPT_VERBOSE 			(a64 << 2)
#define F_OPT_VERBOSE2 			(a64 << 3)
#define F_OPT_VERBOSE3 			(a64 << 4)
#define F_OPT_SFV	 			(a64 << 5)
#define F_OPT_NOWRITE			(a64 << 6)
#define F_OPT_NOBUFFER			(a64 << 7)
#define F_OPT_UPDATE			(a64 << 8)
#define F_OPT_FIX				(a64 << 9)
#define F_OPT_FOLLOW_LINKS		(a64 << 10)
#define F_OPT_FORMAT_BATCH		(a64 << 11)
#define F_OPT_KILL_GLOBAL		(a64 << 12)
#define F_OPT_MODE_RAWDUMP  	(a64 << 13)
#define F_OPT_HAS_G_REGEX		(a64 << 14)
#define F_OPT_VERBOSE4 			(a64 << 15)
#define F_OPT_WBUFFER			(a64 << 16)
#define F_OPT_FORCEWSFV			(a64 << 17)
#define F_OPT_FORMAT_COMP   	(a64 << 18)
#define F_OPT_DAEMONIZE			(a64 << 19)
#define F_OPT_LOOP				(a64 << 20)
#define F_OPT_LOOPEXEC			(a64 << 21)
#define F_OPT_PS_SILENT			(a64 << 22)
#define F_OPT_PS_TIME			(a64 << 23)
#define F_OPT_PS_LOGGING		(a64 << 24)
#define F_OPT_TERM_ENUM			(a64 << 25)
#define F_OPT_HAS_G_MATCH		(a64 << 26)
#define F_OPT_HAS_M_ARG1		(a64 << 27)
#define F_OPT_HAS_M_ARG2		(a64 << 28)
#define F_OPT_HAS_M_ARG3		(a64 << 29)
#define F_OPT_PREEXEC			(a64 << 30)
#define F_OPT_POSTEXEC			(a64 << 31)
#define F_OPT_NOBACKUP			(a64 << 32)
#define F_OPT_C_GHOSTONLY		(a64 << 33)
#define F_OPT_XDEV				(a64 << 34)
#define F_OPT_XBLK				(a64 << 35)
#define F_OPT_MATCHQ			(a64 << 36)
#define F_OPT_IMATCHQ			(a64 << 37)
#define F_OPT_CDIRONLY			(a64 << 38)
#define F_OPT_SORT 				(a64 << 39)
#define F_OPT_HASLOM			(a64 << 40)
#define F_OPT_HAS_G_LOM			(a64 << 41)
#define F_OPT_FORCE2 			(a64 << 42)
#define F_OPT_VERBOSE5 			(a64 << 43)
#define F_OPT_SHAREDMEM			(a64 << 44)
#define F_OPT_SHMRELOAD			(a64 << 45)
#define F_OPT_LOADQ				(a64 << 46)
#define F_OPT_SHMDESTROY		(a64 << 47)
#define F_OPT_SHMDESTONEXIT		(a64 << 48)
#define F_OPT_MODE_BINARY		(a64 << 49)
#define F_OPT_IFIRSTRES			(a64 << 50)
#define F_OPT_IFIRSTHIT			(a64 << 51)
#define F_OPT_HASMAXHIT			(a64 << 52)
#define F_OPT_HASMAXRES			(a64 << 53)
#define F_OPT_PROCREV			(a64 << 54)
#define F_OPT_NOFQ				(a64 << 55)

#define F_OPT_HASMATCH			(F_OPT_HAS_G_REGEX|F_OPT_HAS_G_MATCH|F_OPT_HAS_G_LOM|F_OPT_HASMAXHIT|F_OPT_HASMAXRES)

#define F_DL_FOPEN_BUFFER		(a32 << 1)
#define F_DL_FOPEN_FILE			(a32 << 2)
#define F_DL_FOPEN_REWIND		(a32 << 3)
#define F_DL_FOPEN_SHM			(a32 << 4)

#define F_EARG_SFV 				(a32 << 1)
#define F_EAR_NOVERB			(a32 << 2)

#define F_FC_MSET_SRC			(a32 << 1)
#define F_FC_MSET_DEST			(a32 << 2)

#define F_GH_NOMEM  			(a32 << 1)
#define F_GH_ISDIRLOG			(a32 << 2)
#define F_GH_EXEC				(a32 << 3)
#define F_GH_ISNUKELOG			(a32 << 4)
#define F_GH_FFBUFFER			(a32 << 5)
#define F_GH_WAPPEND			(a32 << 6)
#define F_GH_DFWASWIPED			(a32 << 7)
#define F_GH_DFNOWIPE			(a32 << 8)
#define F_GH_ISDUPEFILE			(a32 << 9)
#define F_GH_ISLASTONLOG		(a32 << 10)
#define F_GH_ISONELINERS		(a32 << 11)
#define F_GH_ONSHM				(a32 << 12)
#define F_GH_ISONLINE			(a32 << 13)
#define F_GH_ISIMDB				(a32 << 14)
#define F_GH_ISGAME				(a32 << 15)
#define F_GH_ISFSX				(a32 << 16)
#define F_GH_ISTVRAGE			(a32 << 17)
#define F_GH_ISGENERIC1			(a32 << 18)
#define F_GH_SHM				(a32 << 19)
#define F_GH_SHMRB				(a32 << 20)
#define F_GH_SHMDESTROY			(a32 << 21)
#define F_GH_SHMDESTONEXIT		(a32 << 22)
#define F_GH_FROMSTDIN			(a32 << 23)
#define F_GH_HASLOM				(a32 << 24)
#define F_GH_HASMATCHES			(a32 << 25)
#define F_GH_HASEXC				(a32 << 26)
#define F_GH_APFILT 			(a32 << 27)
#define F_GH_HASMAXRES			(a32 << 28)
#define F_GH_HASMAXHIT			(a32 << 29)

/* these bits determine file type */
#define F_GH_ISTYPE				(F_GH_ISGENERIC1|F_GH_ISNUKELOG|F_GH_ISDIRLOG|F_GH_ISDUPEFILE|F_GH_ISLASTONLOG|F_GH_ISONELINERS|F_GH_ISONLINE|F_GH_ISIMDB|F_GH_ISGAME|F_GH_ISFSX|F_GH_ISTVRAGE)
#define F_GH_ISSHM				(F_GH_SHM|F_GH_ONSHM)
#define F_GH_ISMP				(F_GH_HASMATCHES|F_GH_HASMAXRES|F_GH_HASMAXHIT)

#define F_OVRR_IPC				(a32 << 1)
#define F_OVRR_GLROOT			(a32 << 2)
#define F_OVRR_SITEROOT			(a32 << 3)
#define F_OVRR_DUPEFILE			(a32 << 4)
#define F_OVRR_LASTONLOG		(a32 << 5)
#define F_OVRR_ONELINERS		(a32 << 6)
#define F_OVRR_DIRLOG			(a32 << 7)
#define F_OVRR_NUKELOG			(a32 << 8)
#define F_OVRR_NUKESTR			(a32 << 9)
#define F_OVRR_IMDBLOG			(a32 << 10)
#define F_OVRR_TVLOG			(a32 << 11)
#define F_OVRR_GAMELOG			(a32 << 12)
#define F_OVRR_GE1LOG			(a32 << 13)
#define F_OVRR_LOGFILE			(a32 << 14)

#define F_PD_RECURSIVE 			(a32 << 1)
#define F_PD_MATCHDIR			(a32 << 2)
#define F_PD_MATCHREG			(a32 << 3)

#define F_PD_MATCHTYPES			(F_PD_MATCHDIR|F_PD_MATCHREG)

#define F_GSORT_DESC			(a32 << 1)
#define F_GSORT_ASC				(a32 << 2)
#define F_GSORT_RESETPOS		(a32 << 3)
#define F_GSORT_NUMERIC	    	(a32 << 4)

#define F_GSORT_ORDER			(F_GSORT_DESC|F_GSORT_ASC)

#define F_ENUMD_ENDFIRSTOK		(a32 << 1)
#define F_ENUMD_BREAKONBAD		(a32 << 2)
#define F_ENUMD_NOXDEV			(a32 << 3)
#define F_ENUMD_NOXBLK			(a32 << 4)

#define F_GM_ISREGEX			(a32 << 1)
#define F_GM_ISMATCH			(a32 << 2)
#define F_GM_ISLOM				(a32 << 3)
#define F_GM_IMATCH				(a32 << 4)
#define F_GM_NAND				(a32 << 5)
#define F_GM_NOR				(a32 << 6)

#define F_GM_TYPES				(F_GM_ISREGEX|F_GM_ISMATCH|F_GM_ISLOM)

#define F_LOM_LVAR_KNOWN		(a32 << 1)
#define F_LOM_RVAR_KNOWN		(a32 << 2)
#define F_LOM_FLOAT				(a32 << 3)
#define F_LOM_INT				(a32 << 4)
#define F_LOM_HASOPER			(a32 << 5)

#define F_LOM_TYPES				(F_LOM_FLOAT|F_LOM_INT)
#define F_LOM_VAR_KNOWN			(F_LOM_LVAR_KNOWN|F_LOM_RVAR_KNOWN)

#define F_EDS_ROOTMINSET		(a32 << 1)
#define F_EDS_KILL				(a32 << 2)

/* -- end flags -- */

#define V_MB					0x100000

#define DL_SZ 					sizeof(struct dirlog)
#define NL_SZ 					sizeof(struct nukelog)
#define DF_SZ 					sizeof(struct dupefile)
#define LO_SZ 					sizeof(struct lastonlog)
#define OL_SZ 					sizeof(struct oneliner)
#define ON_SZ 					sizeof(struct ONLINE)
#define ID_SZ 					sizeof(_d_imdb)
#define GM_SZ 					sizeof(_d_game)
#define TV_SZ 					sizeof(_d_tvrage)
#define G1_SZ 					sizeof(_d_generic_s2044)

#define CRC_FILE_READ_BUFFER_SIZE 26214400
#define	DB_MAX_SIZE 			((ulint64_t)536870912)   /* max file size allowed to load into memory */
#define MAX_EXEC_STR 			262144

#define	PIPE_READ_MAX			0x2000
#define MAX_VAR_LEN				4096

#define MSG_GEN_NODFILE 		"ERROR: %s: could not open data file: %s\n"
#define MSG_GEN_DFWRITE 		"ERROR: %s: [%d] [%llu] writing record to dirlog failed! (mode: %s)\n"
#define MSG_GEN_DFCORRU 		"ERROR: %s: corrupt data file detected! (data file size [%llu] is not a multiple of block size [%d])\n"
#define MSG_GEN_DFRFAIL 		"ERROR: %s: building data file failed!\n"
#define MSG_BAD_DATATYPE 		"ERROR: %s: could not determine data type\n"
#define MSG_GEN_WROTE			"STATS: %s: wrote %llu bytes in %llu records\n"

#define DEFPATH_LOGS 			"/logs"
#define DEFPATH_USERS 			"/users"
#define DEFPATH_GROUPS 			"/groups"

#define DEFF_DIRLOG 			"dirlog"
#define DEFF_NUKELOG 			"nukelog"
#define DEFF_LASTONLOG  		"laston.log"
#define DEFF_DUPEFILE 			"dupefile"
#define DEFF_ONELINERS 			"oneliners.log"
#define DEFF_DULOG	 			"glutil.log"
#define DEFF_IMDB	 			"imdb.log"
#define DEFF_GAMELOG 			"game.log"
#define DEFF_TV 				"tv.log"
#define DEFF_GEN1 				"gen1.log"

#define F_SIGERR_CONTINUE 		0x1  /* continue after exception */

#define ID_SIGERR_UNSPEC 		0x0
#define ID_SIGERR_MEMCPY 		0x1
#define ID_SIGERR_STRCPY 		0x2
#define ID_SIGERR_FREE 			0x3
#define ID_SIGERR_FREAD 		0x4
#define ID_SIGERR_FWRITE 		0x5
#define ID_SIGERR_FOPEN 		0x6
#define ID_SIGERR_FCLOSE 		0x7
#define ID_SIGERR_MEMMOVE 		0x8

sigjmp g_sigjmp = { { { { 0 } } } };
uint64_t gfl = F_OPT_WBUFFER;
uint32_t ofl = 0;
FILE *fd_log = NULL;
char LOGFILE[PATH_MAX] = { log_file };

void e_pop(p_sigjmp psm) {
	memcpy(psm->p_env, psm->env, sizeof(sigjmp_buf));
	psm->pid = psm->id;
	psm->pflags = psm->flags;
	psm->pci = psm->ci;
}

void e_push(p_sigjmp psm) {
	memcpy(psm->env, psm->p_env, sizeof(sigjmp_buf));
	psm->id = psm->pid;
	psm->flags = psm->pflags;
	psm->ci = psm->pci;
}

void g_setjmp(uint32_t flags, char *type, void *callback, void *arg) {
	if (flags) {
		g_sigjmp.flags = flags;
	}
	g_sigjmp.id = ID_SIGERR_UNSPEC;

	bzero(g_sigjmp.type, 32);
	memcpy(g_sigjmp.type, type, strlen(type));

	return;
}

void *g_memcpy(void *dest, const void *src, size_t n) {
	void *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_MEMCPY;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = memcpy(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

void *g_memmove(void *dest, const void *src, size_t n) {
	void *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_MEMMOVE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = memmove(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

char *g_strncpy(char *dest, const char *src, size_t n) {
	char *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags = 0;
	g_sigjmp.id = ID_SIGERR_STRCPY;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = strncpy(dest, src, n);
	}
	e_push(&g_sigjmp);
	return ret;
}

void g_free(void *ptr) {
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FREE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		free(ptr);
	}
	e_push(&g_sigjmp);
}

size_t g_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FREAD;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fread(ptr, size, nmemb, stream);
	}
	e_push(&g_sigjmp);
	return ret;
}

size_t g_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FWRITE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fwrite(ptr, size, nmemb, stream);
	}
	e_push(&g_sigjmp);
	return ret;
}

FILE *gg_fopen(const char *path, const char *mode) {
	FILE *ret = NULL;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FOPEN;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fopen(path, mode);
	}
	e_push(&g_sigjmp);
	return ret;
}

int g_fclose(FILE *fp) {
	int ret = 0;
	e_pop(&g_sigjmp);
	g_sigjmp.flags |= F_SIGERR_CONTINUE;
	g_sigjmp.id = ID_SIGERR_FCLOSE;
	if (!sigsetjmp(g_sigjmp.env, 1)) {
		ret = fclose(fp);
	}
	e_push(&g_sigjmp);
	return ret;
}

#define MSG_DEF_UNKN1 	"(unknown)"

void sighdl_error(int sig, siginfo_t* siginfo, void* context) {

	char *s_ptr1 = MSG_DEF_UNKN1, *s_ptr2 = MSG_DEF_UNKN1, *s_ptr3 = "";
	char buffer1[4096] = { 0 };

	switch (sig) {
	case SIGSEGV:
		s_ptr1 = "SEGMENTATION FAULT";
		break;
	case SIGFPE:
		s_ptr1 = "FLOATING POINT EXCEPTION";
		break;
	case SIGILL:
		s_ptr1 = "ILLEGAL INSTRUCTION";
		break;
	case SIGBUS:
		s_ptr1 = "BUS ERROR";
		break;
	case SIGTRAP:
		s_ptr1 = "TRACE TRAP";
		break;
	default:
		s_ptr1 = "UNKNOWN EXCEPTION";
	}

	snprintf(buffer1, 4096, ", fault address: 0x%.16llX",
			(ulint64_t) (uintaa_t) siginfo->si_addr);

	switch (g_sigjmp.id) {
	case ID_SIGERR_MEMCPY:
		s_ptr2 = "memcpy";
		break;
	case ID_SIGERR_STRCPY:
		s_ptr2 = "strncpy";
		break;
	case ID_SIGERR_FREE:
		s_ptr2 = "free";
		break;
	case ID_SIGERR_FREAD:
		s_ptr2 = "fread";
		break;
	case ID_SIGERR_FWRITE:
		s_ptr2 = "fwrite";
		break;
	case ID_SIGERR_FCLOSE:
		s_ptr2 = "fclose";
		break;
	case ID_SIGERR_MEMMOVE:
		s_ptr2 = "memove";
		break;
	}

	if (g_sigjmp.flags & F_SIGERR_CONTINUE) {
		s_ptr3 = ", resuming execution..";
	}

	printf("EXCEPTION: %s: [%s] [%s] [%d]%s%s\n", s_ptr1, g_sigjmp.type, s_ptr2,
			siginfo->si_errno, buffer1, s_ptr3);

	usleep(450000);

	g_sigjmp.ci++;

	if (g_sigjmp.flags & F_SIGERR_CONTINUE) {
		siglongjmp(g_sigjmp.env, 0);
	}

	g_sigjmp.ci = 0;
	g_sigjmp.flags = 0;

	exit(siginfo->si_errno);
}

struct tm *get_localtime(void) {
	time_t t = time(NULL);
	return localtime(&t);
}

#define F_MSG_TYPE_ANY		 	MAX_uint32_t
#define F_MSG_TYPE_EXCEPTION 	(a32 << 1)
#define F_MSG_TYPE_ERROR 		(a32 << 2)
#define F_MSG_TYPE_WARNING 		(a32 << 3)
#define F_MSG_TYPE_NOTICE		(a32 << 4)
#define F_MSG_TYPE_STATS		(a32 << 5)
#define F_MSG_TYPE_NORMAL		(a32 << 6)

#define F_MSG_TYPE_EEW 			(F_MSG_TYPE_EXCEPTION|F_MSG_TYPE_ERROR|F_MSG_TYPE_WARNING)

uint32_t LOGLVL = F_MSG_TYPE_EEW;

uint32_t get_msg_type(char *msg) {
	if (!strncmp(msg, "INIT:", 5)) {
		return F_MSG_TYPE_ANY;
	}
	if (!strncmp(msg, "EXCEPTION:", 10)) {
		return F_MSG_TYPE_EXCEPTION;
	}
	if (!strncmp(msg, "ERROR:", 6)) {
		return F_MSG_TYPE_ERROR;
	}
	if (!strncmp(msg, "WARNING:", 8)) {
		return F_MSG_TYPE_WARNING;
	}
	if (!strncmp(msg, "NOTICE:", 7)) {
		return F_MSG_TYPE_NOTICE;
	}
	if (!strncmp(msg, "MACRO:", 6)) {
		return F_MSG_TYPE_NOTICE;
	}
	if (!strncmp(msg, "STATS:", 6)) {
		return F_MSG_TYPE_STATS;
	}

	return F_MSG_TYPE_NORMAL;
}

int w_log(char *w, char *ow) {

	if (ow && !(get_msg_type(ow) & LOGLVL)) {
		return 1;
	}

	size_t wc, wll;

	wll = strlen(w);

	if ((wc = fwrite(w, 1, wll, fd_log)) != wll) {
		printf("ERROR: %s: writing log failed [%d/%d]\n", LOGFILE, (int) wc,
				(int) wll);
	}

	fflush(fd_log);

	return 0;
}

#define PSTR_MAX	(V_MB/4)

int print_str(const char * volatile buf, ...) {
	g_setjmp(0, "print_str", NULL, NULL);
	;
	char d_buffer_2[PSTR_MAX + 1];
	va_list al;
	va_start(al, buf);

	if ((gfl & F_OPT_PS_LOGGING) || (gfl & F_OPT_PS_TIME)) {
		struct tm tm = *get_localtime();
		snprintf(d_buffer_2, PSTR_MAX, "[%.2u-%.2u-%.2u %.2u:%.2u:%.2u] %s",
				(tm.tm_year + 1900) % 100, tm.tm_mon + 1, tm.tm_mday,
				tm.tm_hour, tm.tm_min, tm.tm_sec, buf);
	}

	if ((gfl & F_OPT_PS_LOGGING) && fd_log) {
		char wl_buffer[PSTR_MAX + 1];
		vsnprintf(wl_buffer, PSTR_MAX, d_buffer_2, al);
		w_log(wl_buffer, (char*) buf);
	}

	if (gfl & F_OPT_PS_SILENT) {
		return 0;
	}

	va_end(al);
	va_start(al, buf);

	if (gfl & F_OPT_PS_TIME) {
		vprintf(d_buffer_2, al);
	} else {
		vprintf(buf, al);

	}

	va_end(al);

	fflush(stdout);

	return 0;
}

/* ---------------------------------------------------------------------------------- */

struct d_stats dl_stats = { 0 };
struct d_stats nl_stats = { 0 };

_g_handle g_act_1 = { 0 };
_g_handle g_act_2 = { 0 };

mda dirlog_buffer = { 0 };
mda nukelog_buffer = { 0 };

int updmode = 0;
char *argv_off = NULL;
char GLROOT[255] = { glroot };
char SITEROOT_N[255] = { siteroot };
char SITEROOT[255] = { 0 };
char DIRLOG[PATH_MAX] = { dir_log };
char NUKELOG[PATH_MAX] = { nuke_log };
char DU_FLD[PATH_MAX] = { du_fld };
char DUPEFILE[PATH_MAX] = { dupe_file };
char LASTONLOG[PATH_MAX] = { last_on_log };
char ONELINERS[PATH_MAX] = { oneliner_file };
char FTPDATA[PATH_MAX] = { ftp_data };
char IMDBLOG[PATH_MAX] = { imdb_file };
char GAMELOG[PATH_MAX] = { game_log };
char TVLOG[PATH_MAX] = { tv_log };
char GE1LOG[PATH_MAX] = { ge1_log };
char *LOOPEXEC = NULL;
long long int db_max_size = DB_MAX_SIZE;
key_t SHM_IPC = (key_t) shm_ipc;

int EXITVAL = 0;

int loop_interval = 0;
uint64_t loop_max = 0;
char *NUKESTR = NULL;

char *exec_str = NULL;
char **exec_v = NULL;
int exec_vc = 0;
//char exec_v_path[PATH_MAX];
__d_exec exc = NULL;

mda glconf = { 0 };

char b_spec1[PATH_MAX];

void *_p_macro_argv = NULL;
int _p_macro_argc = 0;

uint32_t g_sleep = 0;
uint32_t g_usleep = 0;

void *p_argv_off = NULL;
void *prio_argv_off = NULL;

mda cfg_rf = { 0 };

uint32_t flags_udcfg = 0;

uint32_t g_sort_flags = 0;

off_t max_hits = 0, max_results = 0;

char *hpd_up =
		"glFTPd binary logs utility, version %d.%d-%d%s-%s\n"
				"\n"
				"Main options:\n"
				"\n Output:\n"
				"  -d, [--raw] [--batch] Parse directory log and print to stdout in text/binary format\n"
				"                          (-vv prints dir nuke status from nukelog)\n"
				"  -n, -||-              Parse nuke log to stdout\n"
				"  -i, -||-              Parse dupe file to stdout\n"
				"  -l, -||-              Parse last-on log to stdout\n"
				"  -o, -||-              Parse oneliners to stdout\n"
				"  -a, -||-              Parse iMDB log to stdout\n"
				"  -k, -||-              Parse game log to stdout\n"
				"  -h, -||-              Parse tvrage log to stdout\n"
				"  -w  -||- [--comp]     Parse online users data from shared memory to stdout\n"
				"  -q <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1> [--raw] [--batch]\n"
				"                        Parse specified log to stdout\n"
				"  -t                    Parse all user files inside /ftp-data/users\n"
				"  -g                    Parse all group files inside /ftp-data/groups\n"
				"  -x <root dir> [--recursive] ([--dir]|[--file]|[--cdir])\n"
				"                        Parses filesystem and processes each item found with internal filters/hooks\n"
				"                          --dir scans directories only\n"
				"                          --file scans files only (default is both dirs and files)\n"
				"                          --cdir processes only the root directory itself\n"
				"\n Input:\n"
				"  -e <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1>\n"
				"                        Rebuilds existing data file, based on filtering rules (see --exec,\n"
				"                          --[i]regex[i] and --[i]match\n"
				"  -z <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1> [--infile=/path/file] [--binary]\n"
				"                        Creates a binary record from ASCII data, inserting it into the specified log\n"
				"                          Captures input from stdin, unless --infile is set\n"
				"                        --binary expects a normal binary log as input and merges it (skips exact\n"
				"                         duplicates)\n"
				"\n Directory log:\n"
				"  -s <folders>          Import specific directories. Use quotation marks with multiple arguments\n"
				"                           <folders> are passed relative to SITEROOT, separated by space\n"
				"                           Use -f to overwrite existing entries\n"
				"  -r [-u]               Rebuild dirlog based on filesystem data\n"
				"                           .folders file (see README) defines a list of dirs in SITEROOT to scan\n"
				"                           -u only imports new records and does not truncate existing dirlog\n"
				"                           -f ignores .folders file and does a full recursive scan\n"
				"  -c, --check [--fix] [--ghost]\n"
				"                        Compare dirlog and filesystem records and warn on differences\n"
				"                          --fix attempts to correct dirlog/filesystem\n"
				"                          --ghost only looks for dirlog records with missing directories on filesystem\n"
				"                          Folder creation dates are ignored unless -f is given\n"
				"  -p, --dupechk         Look for duplicate records within dirlog and print to stdout\n"
				"  -b, --backup <dirlog|nukelog|dupefile|lastonlog|imdb|game|tvrage|ge1>\n"
				"                        Perform backup on specified log\n"
				"\n Other:\n"
				"  -m <macro> [--arg[1-3] <string>]\n"
				"                         Searches subdirs for script that has the given macro defined, and executes\n"
				"                         --arg[1-3] sets values that fill {m:arg[1-3]} variables inside a macro\n"
				"\n Hooks:\n"
				"  --exec <command [{field}..{field}..]>\n"
				"                        While parsing data structure/filesystem, execute shell command for each record\n"
				"                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
				"                          Operators {..} are overwritten with dirlog values\n"
				"                          If return value is non-zero, the processed record gets filtered\n"
				"                          Uses system() call (man system)\n"
				"  --execv <command [{field}..{field}..]>\n"
				"                        Same as --exec, only instead of calling system(), this uses execv() (man exec)\n"
				"                          It's generally much faster compared to --exec, since it doesn't fork /bin/sh\n"
				"                          for each individual processed record\n"
				"  --preexec <command [{field}..{field}..]>\n"
				"                        Execute shell <command> before starting main procedure\n"
				"  --postexec <command [{field}..{field}..]>\n"
				"                        Execute shell <command> after main procedure finishes\n"
				"  --loopexec <command [{field}..{field}..]>\n"
				"\n Matching:\n"
				"  --regex [<field>,]<match>\n"
				"                        Regex filter string, used during various operations\n"
				"                          If <field> is set, matching is performed against a specific data log field\n"
				"                            (field names are the same as --exec variable names for logs)\n"
				"                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
				"  --regexi [<var>,]<match>\n"
				"                        Case insensitive variant of --regex\n"
				"  --iregex [<var>,]<match> \n"
				"                        Same as --regex with negated match\n"
				"  --iregexi [<var>,]<match>\n"
				"                        Same as --regexi with negated match\n"
				"  --match [<field>,]<match>\n"
				"                        Regular filter string (exact matches)\n"
				"                          Used with -r, -e, -p, -d, -i, -l, -o, -w, -t, -g, -x, -a, -k, -h, -n, -q\n"
				"  --imatch [<field>,]<match>\n"
				"                        Inverted --match\n"
				"  --lom <<field> > 5.0 && <field> != 0 || <field> ..>\n"
				"                        Compare values by logical and comparison/relational operators\n"
				"                        Applies to any integer/floating point fields from data sources\n"
				"                          Use quotation marks to avoid collisions with bash operators\n"
				"                        Valid logical operators: && (and), || (or)\n"
				"                        Valid comparison/relational operators: =, !=, >, <, <=, >=\n"
				"  --ilom <expression>   Same as --lom with negated match\n"
				"  --maxhit <limit>      Maximum number of positive filter matches (rest are forced negative)\n"
				"  --maxres <limit>      Maximum number of negative filter matches (rest are forced positive)\n"
				"  --ifhit               Ignore first match\n"
				"  --ifres               Ignore first result\n"
				"  --matchq              Exit on first match\n"
				"  --imatchq             Exit on first result\n"
				"\n"
				"  In between match arguments, logical or|and operators apply:\n"
				"  \".. --<argument1> <or|and> --<margument2> ..\"\n"
				"\n Misc:\n"
				"  --sort <mode>,<order>,<field>\n"
				"                        Sort data log entries\n"
				"                          <mode> can only be 'num' (numeric)\n"
				"                          <order> can be 'asc' (ascending) or 'desc' (descending)\n"
				"                          Sorts by the specified data log <field>\n"
				"                          Used with -e, -d, -i, -l, -o, -w, -a, -k, -h, -n, -q\n"
				"\n"
				"Options:\n"
				"  -f                    Force operation where it applies (use -ff for greater effect)\n"
				"  -v                    Increase verbosity level (use -vv or more for greater effect)\n"
				"  --nowrite             Perform a dry run, executing normally except no writing is done\n"
				"  -b, --nobuffer        Disable data file memory buffering\n"
				"  -y, --followlinks     Follow symbolic links (default is skip)\n"
				"  --nowbuffer           Disable write pre-caching (faster but less safe), applies to -r\n"
				"  --memlimit=<bytes>    Maximum file size that can be pre-buffered into memory\n"
				"  --shmem [--shmdestroy] [--shmdestonexit] [--shmreload]\n"
				"                        Instead of internal memory, use the shared memory segment to buffer log data\n"
				"                           This is usefull as an inter-process caching mechanism, allowing other glutil\n"
				"                            instances (or other processes) rapid access to the log data, without having to\n"
				"                            allocate pages and load from filesystem each time\n"
				"                           If log file is present and it's size doesn't match segment size, glutil\n"
				"                            destroys the segment and re-creates it with new size\n"
				"                           Although this applies globally, it should only be used with dump operations (-d,-n,-i,..)\n"
				"                           DO NOT use, unless you know exactly what you're doing\n"
				"                        --shmdestroy forces the old segment (if any) be destroyed and re-loaded\n"
				"                        --shmdestonexit destroys used segments before exiting process\n"
				"                        --shmreload forces existing records (if any) be reloaded, but segment is not destroyed\n"
				"                         and it's size remains the same (when data log size doesn't match segment size,\n"
				"                         there will be junk/missing records, depending on input size being higher/lower)\n"
				"  --loadq               Quit just after loading data into memory\n"
				"                           Applies to dump operations only\n"
				"  --nofq                Abort data (re)build operation unconditionally, if nothing was filtered\n"
				"  --sfv                 Generate new SFV files inside target folders, works with -r [-u] and -s\n"
				"                           Used by itself, triggers -r (fs rebuild) dry run (does not modify dirlog)\n"
				"                           Avoid using this if doing a full recursive rebuild\n"
				"  --xdev                Ignores files/dirs on other filesystems\n"
				"                           Applies to -r, -t, -g, -x (can apply to other modes)\n"
				"  --xblk                Ignores files/dirs on non-block devices\n"
				"                           Applies to -r, -t, -g, -x (can apply to other modes)\n"
				"  --rev                 Reverses the order in which records are processed\n"
				"  --batch               Prints with simple formatting\n"
				"  --ipc <key>           Override gl's shared memory segment key setting\n"
				"  --daemon              Fork process into background\n"
				"  --loop <interval>     Loops the given operation\n"
				"                           Use caution, some operations might fail when looped\n"
				"                           This is usefull when running yourown scripts (--exec)\n"
				"                         Execute command each loop\n"
				"  --loglevel <0-6>      Log verbosity level (1: exception only..6: everything)\n"
				"                           Level 0 turns logging off\n"
				"  --silent              Silent mode\n"
				"  --ftime               Prepend formatted timestamps to output\n"
				"  --log                 Force logging enabled\n"
				"  --fork <command>      Fork process into background and execute <command>\n"
				"                           Used only with when running macros (-m)\n"
				"  --[u]sleep <timeout>  Wait for <timeout> before running\n"
				"  --version             Print version and exit\n"
				"\n"
				"Directory and file:\n"
				"  --glroot=PATH         Override default glFTPd root path\n"
				"  --siteroot=PATH       Override default site root path (relative to glFTPd root)\n"
				"  --dirlog=FILE         Override default path to directory log\n"
				"  --nukelog=FILE        Override default path to nuke log\n"
				"  --dupefile=FILE       Override default path to dupe file\n"
				"  --lastonlog=FILE      Override default path to last-on log\n"
				"  --oneliners=FILE      Override default path to oneliners file\n"
				"  --imdblog=FILE        Override default path to iMDB log\n"
				"  --gamelog=FILE        Override default path to game log\n"
				"  --tvlog=FILE          Override default path to TVRAGE log\n"
				"  --folders=FILE        Override default path to folders file (contains sections and depths,\n"
				"                           used on recursive imports)\n"
				"  --logfile=FILE        Override default log file path\n"
				"\n";

int md_init(pmda md, int nm);
p_md_obj md_first(pmda md);
int split_string(char *, char, pmda);
off_t s_string(char *input, char *m, off_t offset);
int is_ascii_text(uint8_t in);
int is_ascii_lowercase_text(uint8_t in);
int is_ascii_uppercase_text(uint8_t in);
int is_ascii_alphanumeric(uint8_t in_c);
void *md_alloc(pmda md, int b);
__g_match g_global_register_match(void);
int g_oper_and(int s, int d);
int g_oper_or(int s, int d);
char **build_argv(char *args, size_t max, int *c);
int find_absolute_path(char *exec, char *output);
int g_do_exec(void *, void *, char*, void *hdl);
int g_do_exec_v(void *buffer, void *p_hdl, char *ex_str, void *hdl);

int g_cpg(void *arg, void *out, int m, size_t sz) {
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}
	if (!buffer) {
		return 1;
	}

	size_t a_l = strlen(buffer);

	if (!a_l) {
		return 2;
	}

	a_l > sz ? a_l = sz : sz;
	char *ptr = (char*) out;
	g_strncpy(ptr, buffer, a_l);
	ptr[a_l] = 0x0;

	return 0;
}
void *g_pg(void *arg, int m) {
	if (m == 2) {
		return (char *) arg;
	}
	return ((char **) arg)[0];
}

char *g_pd(void *arg, int m, size_t l) {
	char *buffer = (char*) g_pg(arg, m);
	char *ptr = NULL;
	size_t a_l = strlen(buffer);

	if (!a_l) {
		return NULL;
	}

	a_l > l ? a_l = l : l;

	if (a_l) {
		ptr = (char*) calloc(a_l + 10, 1);
		g_strncpy(ptr, buffer, a_l);
	}
	return ptr;
}

int prio_opt_g_macro(void *arg, int m) {
	prio_argv_off = g_pg(arg, m);
	updmode = PRIO_UPD_MODE_MACRO;
	return 0;
}

int prio_opt_g_pinfo(void *arg, int m) {
	updmode = PRIO_UPD_MODE_INFO;
	return 0;
}

int opt_g_loglvl(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	int lvl = atoi(buffer), i;
	uint32_t t_LOGLVL = 0;

	for (i = 0; i < lvl; i++) {
		t_LOGLVL <<= 1;
		t_LOGLVL |= 0x1;
	}

	LOGLVL = t_LOGLVL;
	gfl |= F_OPT_PS_LOGGING;

	return 0;
}

int opt_g_verbose(void *arg, int m) {
	gfl |= F_OPT_VERBOSE;
	return 0;
}

int opt_g_verbose2(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2;
	return 0;
}

int opt_g_verbose3(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3;
	return 0;
}

int opt_g_verbose4(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4;
	return 0;
}

int opt_g_verbose5(void *arg, int m) {
	gfl |= F_OPT_VERBOSE | F_OPT_VERBOSE2 | F_OPT_VERBOSE3 | F_OPT_VERBOSE4
			| F_OPT_VERBOSE5;
	return 0;
}

int opt_g_force(void *arg, int m) {
	gfl |= F_OPT_FORCE;
	return 0;
}

int opt_g_force2(void *arg, int m) {
	gfl |= (F_OPT_FORCE2 | F_OPT_FORCE);
	return 0;
}

int opt_g_update(void *arg, int m) {
	gfl |= F_OPT_UPDATE;
	return 0;
}

int opt_g_loop(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	loop_interval = (int) strtol(buffer, NULL, 10);
	gfl |= F_OPT_LOOP;
	return 0;
}

int opt_loop_max(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	errno = 0;
	loop_max = (uint64_t) strtol(buffer, NULL, 10);
	if ((errno == ERANGE && (loop_max == LONG_MAX || loop_max == LONG_MIN))
			|| (errno != 0 && loop_max == 0)) {
		return (a32 << 17);
	}

	return 0;
}

int opt_g_udc(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_DUMP_GEN;
	return 0;
}

int opt_g_dg(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_DUMP_GENERIC;
	return 0;
}

int opt_g_recursive(void *arg, int m) {
	flags_udcfg |= F_PD_RECURSIVE;
	return 0;
}

int opt_g_udc_dir(void *arg, int m) {
	flags_udcfg |= F_PD_MATCHDIR;
	return 0;
}

int opt_g_udc_f(void *arg, int m) {
	flags_udcfg |= F_PD_MATCHREG;
	return 0;
}

int opt_g_loopexec(void *arg, int m) {
	LOOPEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (LOOPEXEC) {
		gfl |= F_OPT_LOOPEXEC;
	}
	return 0;
}

int opt_g_maxresults(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	max_results = (off_t) strtoll(buffer, NULL, 10);
	gfl |= F_OPT_HASMAXRES;
	return 0;
}

int opt_g_maxhits(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	max_hits = (off_t) strtoll(buffer, NULL, 10);
	gfl |= F_OPT_HASMAXHIT;
	return 0;
}

int opt_g_daemonize(void *arg, int m) {
	gfl |= F_OPT_DAEMONIZE;
	return 0;
}

int opt_g_ifres(void *arg, int m) {
	gfl |= F_OPT_IFIRSTRES;
	return 0;
}

int opt_g_ifhit(void *arg, int m) {
	gfl |= F_OPT_IFIRSTHIT;
	return 0;
}

int opt_g_nofq(void *arg, int m) {
	gfl |= F_OPT_NOFQ;
	return 0;
}

int opt_g_fix(void *arg, int m) {
	gfl |= F_OPT_FIX;
	return 0;
}

int opt_g_sfv(void *arg, int m) {
	gfl |= F_OPT_SFV;
	return 0;
}

int opt_batch_output_formatting(void *arg, int m) {
	gfl |= F_OPT_FORMAT_BATCH;
	return 0;
}

int opt_compact_output_formatting(void *arg, int m) {
	gfl |= F_OPT_FORMAT_COMP;
	return 0;
}

int opt_g_shmem(void *arg, int m) {
	gfl |= F_OPT_SHAREDMEM;
	return 0;
}

int opt_g_loadq(void *arg, int m) {
	gfl |= F_OPT_LOADQ;
	return 0;
}

int opt_g_shmdestroy(void *arg, int m) {
	gfl |= F_OPT_SHMDESTROY;
	return 0;
}

int opt_g_shmdestroyonexit(void *arg, int m) {
	gfl |= F_OPT_SHMDESTONEXIT;
	return 0;
}

int opt_g_shmreload(void *arg, int m) {
	gfl |= F_OPT_SHMRELOAD;
	return 0;
}

int opt_g_nowrite(void *arg, int m) {
	gfl |= F_OPT_NOWRITE;
	return 0;
}

int opt_g_nobuffering(void *arg, int m) {
	gfl |= F_OPT_NOBUFFER;
	return 0;
}

int opt_g_buffering(void *arg, int m) {
	gfl ^= F_OPT_WBUFFER;
	return 0;
}

int opt_g_followlinks(void *arg, int m) {
	gfl |= F_OPT_FOLLOW_LINKS;
	return 0;
}

int opt_g_ftime(void *arg, int m) {
	gfl |= F_OPT_PS_TIME;
	return 0;
}

int opt_g_matchq(void *arg, int m) {
	gfl |= F_OPT_MATCHQ;
	return 0;
}

int opt_g_imatchq(void *arg, int m) {
	gfl |= F_OPT_IMATCHQ;
	return 0;
}

int opt_update_single_record(void *arg, int m) {
	argv_off = g_pg(arg, m);
	updmode = UPD_MODE_SINGLE;
	return 0;
}

int opt_recursive_update_records(void *arg, int m) {
	updmode = UPD_MODE_RECURSIVE;
	return 0;
}

int opt_raw_dump(void *arg, int m) {
	gfl |= F_OPT_MODE_RAWDUMP;
	gfl |= F_OPT_PS_SILENT;
	return 0;
}

int opt_binary(void *arg, int m) {
	gfl |= F_OPT_MODE_BINARY;
	return 0;
}

int opt_g_reverse(void *arg, int m) {
	gfl |= F_OPT_PROCREV;
	return 0;
}

int opt_silent(void *arg, int m) {
	gfl |= F_OPT_PS_SILENT;
	return 0;
}

int opt_logging(void *arg, int m) {
	gfl |= F_OPT_PS_LOGGING;
	return 0;
}

int opt_nobackup(void *arg, int m) {
	gfl |= F_OPT_NOBACKUP;
	return 0;
}

int opt_backup(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_BACKUP;
	return 0;
}

int opt_exec(void *arg, int m) {
	exec_str = g_pd(arg, m, MAX_EXEC_STR);
	exc = g_do_exec;
	return 0;
}

long amax = 0;

int opt_execv(void *arg, int m) {
	int c = 0;

#ifdef _SC_ARG_MAX
	amax = sysconf(_SC_ARG_MAX);
#else
#ifdef ARG_MAX
	val = ARG_MAX;
#endif
#endif

	if (!amax) {
		amax = LONG_MAX;
	}

	long count = amax / sizeof(char*);

	exec_str = g_pd(arg, m, MAX_EXEC_STR);

	if (!exec_str) {
		return 9008;
	}

	if (!strlen(exec_str)) {
		return 9009;
	}

	char **ptr = build_argv(exec_str, count, &c);

	if (!c) {
		return 9001;
	}

	if (c > count / 2) {
		return 9002;
	}

	exec_vc = c;

	exec_v = ptr;
	exc = g_do_exec_v;

	return 0;
}

FILE *pf_infile = NULL;
char infile_p[PATH_MAX];

int opt_g_infile(void *arg, int m) {
	g_cpg(arg, infile_p, m, PATH_MAX);

	pf_infile = gg_fopen(infile_p, "rb");
	return 0;
}

int opt_shmipc(void *arg, int m) {
	char *buffer = g_pg(arg, m);

	if (!strlen(buffer)) {
		return (a32 << 16);
	}

	SHM_IPC = (key_t) strtoul(buffer, NULL, 16);

	if (!SHM_IPC) {
		return 2;
	}

	ofl |= F_OVRR_IPC;

	return 0;
}

int opt_log_file(void *arg, int m) {
	g_cpg(arg, LOGFILE, m, PATH_MAX);
	gfl |= F_OPT_PS_LOGGING;
	ofl |= F_OVRR_LOGFILE;
	return 0;
}

char MACRO_ARG1[4096] = { 0 };
char MACRO_ARG2[4096] = { 0 };
char MACRO_ARG3[4096] = { 0 };

int opt_g_arg1(void *arg, int m) {
	g_cpg(arg, MACRO_ARG1, m, 4095);
	gfl |= F_OPT_HAS_M_ARG1;
	return 0;
}

int opt_g_arg2(void *arg, int m) {
	g_cpg(arg, MACRO_ARG2, m, 4095);
	gfl |= F_OPT_HAS_M_ARG2;
	return 0;
}

int opt_g_arg3(void *arg, int m) {
	g_cpg(arg, MACRO_ARG3, m, 4095);
	gfl |= F_OPT_HAS_M_ARG3;
	return 0;
}

char *GLOBAL_PREEXEC = NULL;
char *GLOBAL_POSTEXEC = NULL;

int opt_g_preexec(void *arg, int m) {
	GLOBAL_PREEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (GLOBAL_PREEXEC) {
		gfl |= F_OPT_PREEXEC;
	}
	return 0;
}

int opt_g_postexec(void *arg, int m) {
	GLOBAL_POSTEXEC = g_pd(arg, m, MAX_EXEC_STR);
	if (GLOBAL_POSTEXEC) {
		gfl |= F_OPT_POSTEXEC;
	}
	return 0;
}

int opt_glroot(void *arg, int m) {
	if (!(ofl & F_OVRR_GLROOT)) {
		g_cpg(arg, GLROOT, m, 255);
		ofl |= F_OVRR_GLROOT;
	}
	return 0;
}

int opt_siteroot(void *arg, int m) {
	if (!(ofl & F_OVRR_SITEROOT)) {
		g_cpg(arg, SITEROOT_N, m, 255);
		ofl |= F_OVRR_SITEROOT;
	}
	return 0;
}

int opt_dupefile(void *arg, int m) {
	if (!(ofl & F_OVRR_DUPEFILE)) {
		g_cpg(arg, DUPEFILE, m, PATH_MAX);
		ofl |= F_OVRR_DUPEFILE;
	}
	return 0;
}

int opt_lastonlog(void *arg, int m) {
	if (!(ofl & F_OVRR_LASTONLOG)) {
		g_cpg(arg, LASTONLOG, m, PATH_MAX);
		ofl |= F_OVRR_LASTONLOG;
	}
	return 0;
}

int opt_oneliner(void *arg, int m) {
	if (!(ofl & F_OVRR_ONELINERS)) {
		g_cpg(arg, ONELINERS, m, PATH_MAX);
		ofl |= F_OVRR_ONELINERS;
	}
	return 0;
}

int opt_imdblog(void *arg, int m) {
	if (!(ofl & F_OVRR_IMDBLOG)) {
		g_cpg(arg, IMDBLOG, m, PATH_MAX);
		ofl |= F_OVRR_IMDBLOG;
	}
	return 0;
}

int opt_tvlog(void *arg, int m) {
	if (!(ofl & F_OVRR_TVLOG)) {
		g_cpg(arg, TVLOG, m, PATH_MAX);
		ofl |= F_OVRR_TVLOG;
	}
	return 0;
}

int opt_gamelog(void *arg, int m) {
	if (!(ofl & F_OVRR_GAMELOG)) {
		g_cpg(arg, GAMELOG, m, PATH_MAX);
		ofl |= F_OVRR_GAMELOG;
	}
	return 0;
}

int opt_GE1LOG(void *arg, int m) {
	if (!(ofl & F_OVRR_GE1LOG)) {
		g_cpg(arg, GE1LOG, m, PATH_MAX);
		ofl |= F_OVRR_GE1LOG;
	}
	return 0;
}

int opt_rebuild(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_REBUILD;
	return 0;
}

int opt_dirlog_file(void *arg, int m) {
	if (!(ofl & F_OVRR_DIRLOG)) {
		g_cpg(arg, DIRLOG, m, PATH_MAX);
		ofl |= F_OVRR_DIRLOG;
	}
	return 0;
}

int opt_g_sleep(void *arg, int m) {
	g_sleep = atoi(g_pg(arg, m));
	return 0;
}

int opt_g_usleep(void *arg, int m) {
	g_usleep = atoi(g_pg(arg, m));
	return 0;
}

mda _md_gsort = { 0 };
char *g_sort_field = NULL;

int opt_g_sort(void *arg, int m) {
	char *buffer = g_pg(arg, m);

	if (gfl & F_OPT_SORT) {
		return 0;
	}

	if (_md_gsort.offset >= 64) {
		return (a32 << 7);
	}

	md_init(&_md_gsort, 3);

	int r = split_string(buffer, 0x2C, &_md_gsort);

	if (r != 3) {
		return (a32 << 10);
	}

	p_md_obj ptr = md_first(&_md_gsort);

	if (!ptr) {
		return (a32 << 11);
	}

	char *s_ptr = (char*) ptr->ptr;

	if (!strncmp(s_ptr, "num", 3)) {
		g_sort_flags |= F_GSORT_NUMERIC;
	} else {
		return (a32 << 12);
	}

	ptr = ptr->next;
	s_ptr = (char*) ptr->ptr;

	if (!strncmp(s_ptr, "desc", 4)) {
		g_sort_flags |= F_GSORT_DESC;
	} else if (!strncmp(s_ptr, "asc", 3)) {
		g_sort_flags |= F_GSORT_ASC;
	} else {
		return (a32 << 13);
	}

	ptr = ptr->next;
	g_sort_field = (char*) ptr->ptr;

	if (!strlen(g_sort_field)) {
		return (a32 << 14);
	}

	g_sort_flags |= F_GSORT_RESETPOS;

	gfl |= F_OPT_SORT;

	return 0;
}

mda _match_rr = { 0 };
mda _lom_strings = { 0 };

typedef struct ___lom_strings_header {
	uint32_t flags;
	g_op g_oper_ptr;
	__g_match m_ref;
	char string[8192];
} _lom_s_h, *__lom_s_h;

#define G_MATCH		((int)0)
#define G_NOMATCH	((int)1)

typedef struct ___last_match {
	uint32_t flags;
	void *ptr;
} _l_match, *__l_match;

#define F_LM_CPRG		(a32 << 1)
#define F_LM_LOM		(a32 << 2)

#define F_LM_TYPES		(F_LM_CPRG|F_LM_LOM)

_l_match _match_rr_l = { 0 };

int g_cprg(void *arg, int m, int match_i_m, int reg_i_m, int regex_flags,
		uint32_t flags) {
	char *buffer = g_pg(arg, m);

	size_t a_i = strlen(buffer);

	if (!a_i) {
		return 0;
	}

	a_i > 4096 ? a_i = 4096 : a_i;

	__g_match pgm = g_global_register_match();

	if (!pgm) {
		return 11000;
	}

	char *ptr = (char*) pgm->data;

	g_strncpy(ptr, buffer, a_i);
	g_strncpy((char*) pgm->b_data, buffer, a_i);

	off_t i = 0;

	while ((ptr[i] != 0x2C || (ptr[i] == 0x2C && ptr[i - 1] == 0x5C))
			&& i < (off_t) a_i) {
		i++;
	}

	if (ptr[i] == 0x2C && i != (off_t) a_i) {
		ptr[i] = 0x0;
		pgm->field = ptr;
		ptr = &ptr[i + 1];
	}

	pgm->match = ptr;
	pgm->match_i_m = match_i_m;
	pgm->reg_i_m = reg_i_m;
	pgm->regex_flags = regex_flags;
	pgm->flags = flags;
	if (regcomp(&pgm->preg, pgm->match,
			(regex_flags | REG_EXTENDED | REG_NOSUB))) {
		return 11001;
	}

	if (pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1) {
		pgm->g_oper_ptr = g_oper_or;
	} else {
		pgm->g_oper_ptr = g_oper_and;
	}

	bzero(&_match_rr_l, sizeof(_match_rr_l));
	_match_rr_l.ptr = (void *) pgm;
	_match_rr_l.flags = F_LM_CPRG;

	switch (flags & F_GM_TYPES) {
	case F_GM_ISREGEX:
		if (!(gfl & F_OPT_HAS_G_REGEX)) {
			gfl |= F_OPT_HAS_G_REGEX;
		}
		break;
	case F_GM_ISMATCH:
		if (!(gfl & F_OPT_HAS_G_MATCH)) {
			gfl |= F_OPT_HAS_G_MATCH;
		}
		break;
	}

	return 0;
}

int opt_g_lom(void *arg, int m, uint32_t flags) {
	char *buffer = g_pg(arg, m);

	if (_lom_strings.offset > 8100) {
		return (a32 << 28);
	}

	//md_init(&_lom_strings, 32);

	size_t a_i = strlen(buffer);

	if (!a_i) {
		return 0;
	}

	a_i > 8190 ? a_i = 8190 : a_i;

	/*__lom_s_h lsh = md_alloc(&_lom_strings, sizeof(_lom_s_h));

	 if (!lsh) {
	 return (a32 << 29);
	 }*/

	__g_match pgm = g_global_register_match();

	if (!pgm) {
		return (a32 << 25);
	}

	/*lsh->m_ref = pgm;
	 lsh->flags |= flags;*/
	pgm->flags |= flags | F_GM_ISLOM;
	if (pgm->flags & F_GM_IMATCH) {
		pgm->g_oper_ptr = g_oper_or;
	} else {
		pgm->g_oper_ptr = g_oper_and;
	}

	gfl |= F_OPT_HAS_G_LOM;

	bzero(&_match_rr_l, sizeof(_match_rr_l));
	_match_rr_l.ptr = (void *) pgm;
	_match_rr_l.flags = F_LM_LOM;

	g_cpg(arg, pgm->data, m, a_i);

	return 0;
}

int opt_g_operator_or(void *arg, int m) {
	__g_match pgm = (__g_match) _match_rr_l.ptr;
	if (!pgm) {
		return (a32 << 26);
	}
	switch (_match_rr_l.flags & F_LM_TYPES) {

		case F_LM_CPRG:
		;
		if ( pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1) {
			pgm->g_oper_ptr = g_oper_and;
		}
		else {
			pgm->g_oper_ptr = g_oper_or;
		}
		break;
		case F_LM_LOM:;
		if (pgm->flags & F_GM_IMATCH) {
			pgm->g_oper_ptr = g_oper_and;
		}
		else {
			pgm->g_oper_ptr = g_oper_or;
		}
		break;
		default:
		return (a32 << 27);
		break;
	}
	return 0;
}

int opt_g_operator_and(void *arg, int m) {
	__g_match pgm = (__g_match) _match_rr_l.ptr;
	if (!pgm) {
		return (a32 << 26);
	}
	switch (_match_rr_l.flags & F_LM_TYPES) {

		case F_LM_CPRG:
		;
		if ( pgm->reg_i_m == REG_NOMATCH || pgm->match_i_m == 1) {
			pgm->g_oper_ptr = g_oper_or;
		}
		else {
			pgm->g_oper_ptr = g_oper_and;
		}
		break;
		case F_LM_LOM:;
		if (pgm->flags & F_GM_IMATCH) {
			pgm->g_oper_ptr = g_oper_or;
		}
		else {
			pgm->g_oper_ptr = g_oper_and;
		}
		break;
		default:
		return (a32 << 27);
		break;
	}
	return 0;
}

int opt_g_lom_match(void *arg, int m) {
	return opt_g_lom(arg, m, 0);
}

int opt_g_lom_imatch(void *arg, int m) {
	return opt_g_lom(arg, m, F_GM_IMATCH);
}

int opt_g_regexi(void *arg, int m) {
	return g_cprg(arg, m, 0, 0, REG_ICASE, F_GM_ISREGEX);
}

int opt_g_match(void *arg, int m) {
	return g_cprg(arg, m, 0, 0, 0, F_GM_ISMATCH);
}

int opt_g_imatch(void *arg, int m) {
	return g_cprg(arg, m, 1, 0, 0, F_GM_ISMATCH);
}

int opt_g_regex(void *arg, int m) {
	return g_cprg(arg, m, 0, 0, 0, F_GM_ISREGEX);
}

int opt_g_iregexi(void *arg, int m) {
	return g_cprg(arg, m, 0, REG_NOMATCH, REG_ICASE, F_GM_ISREGEX);
}

int opt_g_iregex(void *arg, int m) {
	return g_cprg(arg, m, 0, REG_NOMATCH, 0, F_GM_ISREGEX);
}

int opt_nukelog_file(void *arg, int m) {
	g_cpg(arg, NUKELOG, m, PATH_MAX - 1);
	ofl |= F_OVRR_NUKELOG;
	return 0;
}

int opt_dirlog_sections_file(void *arg, int m) {
	g_cpg(arg, DU_FLD, m, PATH_MAX - 1);
	return 0;
}

int print_version(void *arg, int m) {
	print_str("glutil-%d.%d-%d%s-%s\n", VER_MAJOR, VER_MINOR,
	VER_REVISION, VER_STR, __STR_ARCH);
	updmode = UPD_MODE_NOOP;
	return 0;
}

int g_opt_mode_noop(void *arg, int m) {
	updmode = UPD_MODE_NOOP;
	return 0;
}

int print_version_long(void *arg, int m) {
	print_str("* glutil-%d.%d-%d%s-%s - glFTPd binary logs tool *\n", VER_MAJOR,
	VER_MINOR,
	VER_REVISION, VER_STR, __STR_ARCH);
	return 0;
}

int opt_dirlog_check(void *arg, int m) {
	updmode = UPD_MODE_CHECK;
	return 0;
}

int opt_check_ghost(void *arg, int m) {
	gfl |= F_OPT_C_GHOSTONLY;
	return 0;
}

int opt_g_xdev(void *arg, int m) {
	gfl |= F_OPT_XDEV;
	return 0;
}

int opt_g_xblk(void *arg, int m) {
	gfl |= F_OPT_XBLK;
	return 0;
}

int opt_g_cdironly(void *arg, int m) {
	gfl |= F_OPT_CDIRONLY;
	return 0;
}

int opt_dirlog_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP;
	return 0;
}

int opt_dupefile_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_DUPEF;
	return 0;
}

int opt_online_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_ONL;
	return 0;
}

int opt_lastonlog_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_LON;
	return 0;
}

int opt_dump_users(void *arg, int m) {
	updmode = UPD_MODE_DUMP_USERS;
	return 0;
}

int opt_dump_grps(void *arg, int m) {
	updmode = UPD_MODE_DUMP_GRPS;
	return 0;
}

int opt_dirlog_dump_nukelog(void *arg, int m) {
	updmode = UPD_MODE_DUMP_NUKE;
	return 0;
}

int opt_g_write(void *arg, int m) {
	updmode = UPD_MODE_WRITE;
	p_argv_off = g_pg(arg, m);
	return 0;
}

int opt_g_dump_imdb(void *arg, int m) {
	updmode = UPD_MODE_DUMP_IMDB;
	return 0;
}

int opt_g_dump_game(void *arg, int m) {
	updmode = UPD_MODE_DUMP_GAME;
	return 0;
}

int opt_g_dump_tv(void *arg, int m) {
	updmode = UPD_MODE_DUMP_TV;
	return 0;
}

int opt_oneliner_dump(void *arg, int m) {
	updmode = UPD_MODE_DUMP_ONEL;
	return 0;
}

int print_help(void *arg, int m) {
	print_str(hpd_up, VER_MAJOR, VER_MINOR, VER_REVISION,
	VER_STR, __STR_ARCH);
	if (m != -1) {
		updmode = UPD_MODE_NOOP;
	}
	return 0;
}

int opt_g_ex_fork(void *arg, int m) {
	p_argv_off = g_pg(arg, m);
	updmode = UPD_MODE_FORK;
	gfl |= F_OPT_DAEMONIZE;
	return 0;
}

int opt_dirlog_chk_dupe(void *arg, int m) {
	updmode = UPD_MODE_DUPE_CHK;
	return 0;
}

int opt_membuffer_limit(void *arg, int m) {
	char *buffer = g_pg(arg, m);
	if (!buffer) {
		return 1;
	}
	long long int l_buffer = atoll(buffer);
	if (l_buffer > 1024) {
		db_max_size = l_buffer;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: max memory buffer limit set to %lld bytes\n",
					l_buffer);
		}
	} else {
		print_str(
				"NOTICE: invalid memory buffer limit, using default (%lld bytes)\n",
				db_max_size);
	}
	return 0;
}

/* generic types */
typedef int _d_ag_handle_i(__g_handle);
typedef int _d_achar_i(char *);
typedef int _d_avoid_i(void);

/* specific types */
typedef int __d_enum_cb(char *, unsigned char, void *, __g_eds);
typedef int __d_ref_to_val(void *, char *, char *, size_t);
typedef void *__d_ref_to_pval(void *arg, char *match, size_t *output);
typedef int __d_format_block(void *, char *);
typedef uint64_t __d_dlfind(char *, int, uint32_t, void *);
typedef pmda __d_cfg(pmda md, char * file);
typedef int __d_mlref(void *buffer, char *key, char *val);
typedef uint64_t __g_t_ptr(void *base, size_t offset);

__g_t_ptr g_t8_ptr, g_t16_ptr, g_t32_ptr, g_t64_ptr;
_d_ag_handle_i g_cleanup, gh_rewind, determine_datatype, g_close, g_shm_cleanup;
_d_avoid_i dirlog_check_dupe, rebuild_dirlog, dirlog_check_records,
		g_print_info;

_d_achar_i self_get_path, file_exists, get_file_type, dir_exists,
		dirlog_update_record, g_dump_ug, g_dump_gen, d_write, d_gen_dump;

__d_enum_cb proc_section, proc_release, ssd_4macro, g_process_directory;

__d_ref_to_val ref_to_val_dirlog, ref_to_val_nukelog, ref_to_val_dupefile,
		ref_to_val_lastonlog, ref_to_val_oneliners, ref_to_val_online,
		ref_to_val_generic, ref_to_val_macro, ref_to_val_imdb, ref_to_val_game,
		ref_to_val_tv, ref_to_val_x, ref_to_val_gen1;

__d_ref_to_pval ref_to_val_ptr_dirlog, ref_to_val_ptr_nukelog,
		ref_to_val_ptr_oneliners, ref_to_val_ptr_online, ref_to_val_ptr_imdb,
		ref_to_val_ptr_game, ref_to_val_ptr_lastonlog, ref_to_val_ptr_dupefile,
		ref_to_val_ptr_tv, ref_to_val_ptr_dummy, ref_to_val_ptr_gen1;

__d_format_block lastonlog_format_block, dupefile_format_block,
		oneliner_format_block, online_format_block, nukelog_format_block,
		dirlog_format_block, imdb_format_block, game_format_block,
		tv_format_block, gen1_format_block;
__d_dlfind dirlog_find, dirlog_find_old, dirlog_find_simple;
__d_cfg search_cfg_rf, register_cfg_rf;
__d_mlref gcb_dirlog, gcb_nukelog, gcb_imdbh, gcb_oneliner, gcb_dupefile,
		gcb_lastonlog, gcb_game, gcb_tv, gcb_gen1;

uint64_t file_crc32(char *, uint32_t *);

int data_backup_records(char*);
ssize_t file_copy(char *, char *, char *, uint32_t);

int release_generate_block(char *, ear *);
off_t get_file_size(char *);
char **process_macro(void *, char **);
char *generate_chars(size_t, char, char*);
time_t get_file_creation_time(struct stat *);
int dirlog_write_record(struct dirlog *, off_t, int);

char *string_replace(char *, char *, char *, char *, size_t);
int enum_dir(char *, void *, void *, int, __g_eds);
int update_records(char *, int);
off_t read_file(char *, void *, size_t, off_t, FILE *);
int option_crc32(void *, int);

int load_cfg(pmda md, char * file, uint32_t flags, pmda *res);
int free_cfg_rf(pmda md);

int reg_match(char *, char *, int);

int delete_file(char *, unsigned char, void *);

int write_file_text(char *, char *);

size_t str_match(char *, char *);
size_t exec_and_wait_for_output(char*, char*);

p_md_obj get_cfg_opt(char *, pmda, pmda*);

uint64_t nukelog_find(char *, int, struct nukelog *);
int parse_args(int argc, char **argv, void*fref_t[]);

int process_opt(char *, void *, void *, int);

int g_fopen(char *, char *, uint32_t, __g_handle);
void *g_read(void *buffer, __g_handle, size_t);
int process_exec_string(char *, char *, size_t, void *, void*);
int process_exec_args(void *data, __g_handle hdl);

int is_char_uppercase(char);

void sig_handler(int);
void child_sig_handler(int, siginfo_t*, void*);
int flush_data_md(__g_handle, char *);
int rebuild(void *);
int rebuild_data_file(char *, __g_handle);

int g_bmatch(void *, __g_handle, pmda md);
int do_match(__g_handle hdl, void *d_ptr, __g_match _gm, void *callback);

size_t g_load_data_md(void *, size_t, char *, __g_handle hdl);
int g_load_record(__g_handle, const void *);
int remove_repeating_chars(char *string, char c);

int g_buffer_into_memory(char *, __g_handle);
int g_print_stats(char *, uint32_t, size_t);

int load_data_md(pmda md, char *file, __g_handle hdl);
int g_shmap_data(__g_handle, key_t);
int g_map_shm(__g_handle, key_t);
int gen_md_data_ref(__g_handle hdl, pmda md, off_t count);
int gen_md_data_ref_cnull(__g_handle hdl, pmda md, off_t count);
int is_memregion_null(void *addr, size_t size);

char *build_data_path(char *, char *, char *);
void *ref_to_val_get_cfgval(char *, char *, char *, int, char *, size_t);

int get_relative_path(char *, char *, char *);

void free_cfg(pmda);

int g_init(int argc, char **argv);

char *g_dgetf(char *str);

int m_load_input(__g_handle hdl, char *input);

off_t s_string_r(char *input, char *m);

int g_bin_compare(const void *p1, const void *p2, off_t size);

typedef int __d_icomp(uint64_t s, uint64_t d);
typedef int __d_fcomp(float s, float d);

int rtv_q(void *query, char *output, size_t max_size);

__d_icomp g_is_higher, g_is_lower, g_is_higherorequal, g_is_equal,
		g_is_not_equal, g_is_lowerorequal, g_is_not, g_is, g_is_lower_2,
		g_is_higher_2;
__d_fcomp g_is_lower_f, g_is_higher_f, g_is_higherorequal_f, g_is_equal_f,
		g_is_lower_f_2, g_is_higher_f_2, g_is_not_equal_f, g_is_lowerorequal_f,
		g_is_f, g_is_not_f;

int g_sort(__g_handle hdl, char *field, uint32_t flags);
int g_sortf_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2);
int g_sorti_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2);
int g_rtval_ex(char *arg, char *match, size_t max_size, char *output,
		uint32_t flags);
int do_sort(__g_handle hdl, char *field, uint32_t flags);

int g_filter(__g_handle hdl, pmda md);

char *g_bmatch_get_def_mstr(void *d_ptr, __g_handle hdl);

int g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
		size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
		uint32_t flags);

int g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags);

int g_load_lom(__g_handle hdl);

int g_proc_mr(__g_handle hdl);
int md_copy(pmda source, pmda dest, size_t block_sz);
int g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
		uint32_t flags);

#define R_SHMAP_ALREADY_EXISTS	(a32 << 1)
#define R_SHMAP_FAILED_ATTACH	(a32 << 2)
#define R_SHMAP_FAILED_SHMAT	(a32 << 3)

void *shmap(key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
		int *shmid);

void *prio_f_ref[] = { "noop", g_opt_mode_noop, (void*) 0, "--raw",
		opt_raw_dump, (void*) 0, "--silent", opt_silent, (void*) 0, "-arg1",
		opt_g_arg1, (void*) 1, "--arg1", opt_g_arg1, (void*) 1, "-arg2",
		opt_g_arg2, (void*) 1, "--arg2", opt_g_arg2, (void*) 1, "-arg3",
		opt_g_arg3, (void*) 1, "--arg3", opt_g_arg3, (void*) 1, "-vvvvv",
		opt_g_verbose5, (void*) 0, "-vvvv", opt_g_verbose4, (void*) 0, "-vvv",
		opt_g_verbose3, (void*) 0, "-vv", opt_g_verbose2, (void*) 0, "-v",
		opt_g_verbose, (void*) 0, "-m", prio_opt_g_macro, (void*) 1, "--info",
		prio_opt_g_pinfo, (void*) 0, "--loglevel", opt_g_loglvl, (void*) 1,
		"--logfile", opt_log_file, (void*) 1, "--log", opt_logging, (void*) 0,
		"--dirlog", opt_dirlog_file, (void*) 1, "--ge1log", opt_GE1LOG,
		(void*) 1, "--gamelog", opt_gamelog, (void*) 1, "--tvlog", opt_tvlog,
		(void*) 1, "--imdblog", opt_imdblog, (void*) 1, "--oneliners",
		opt_oneliner, (void*) 1, "--lastonlog", opt_lastonlog, (void*) 1,
		"--nukelog", opt_nukelog_file, (void*) 1, "--siteroot", opt_siteroot,
		(void*) 1, "--glroot", opt_glroot, (void*) 1,
		NULL, NULL, NULL };

void *f_ref[] = { "noop", g_opt_mode_noop, (void*) 0, "and", opt_g_operator_and,
		(void*) 0, "or", opt_g_operator_or, (void*) 0, "--rev", opt_g_reverse,
		(void*) 0, "lom", opt_g_lom_match, (void*) 1, "--lom", opt_g_lom_match,
		(void*) 1, "ilom", opt_g_lom_imatch, (void*) 1, "--ilom",
		opt_g_lom_imatch, (void*) 1, "--info", prio_opt_g_pinfo, (void*) 0,
		"sort", opt_g_sort, (void*) 1, "--sort", opt_g_sort, (void*) 1, "-h",
		opt_g_dump_tv, (void*) 0, "-k", opt_g_dump_game, (void*) 0, "--cdir",
		opt_g_cdironly, (void*) 0, "--imatchq", opt_g_imatchq, (void*) 0,
		"--matchq", opt_g_matchq, (void*) 0, "-a", opt_g_dump_imdb, (void*) 0,
		"-z", opt_g_write, (void*) 1, "--infile", opt_g_infile, (void*) 1,
		"-xdev", opt_g_xdev, (void*) 0, "--xdev", opt_g_xdev, (void*) 0,
		"-xblk", opt_g_xblk, (void*) 0, "--xblk", opt_g_xblk, (void*) 0,
		"-file", opt_g_udc_f, (void*) 0, "--file", opt_g_udc_f, (void*) 0,
		"-dir", opt_g_udc_dir, (void*) 0, "--dir", opt_g_udc_dir, (void*) 0,
		"--loopmax", opt_loop_max, (void*) 1, "--ghost", opt_check_ghost,
		(void*) 0, "-q", opt_g_dg, (void*) 1, "-x", opt_g_udc, (void*) 1,
		"-recursive", opt_g_recursive, (void*) 0, "--recursive",
		opt_g_recursive, (void*) 0, "-g", opt_dump_grps, (void*) 0, "-t",
		opt_dump_users, (void*) 0, "--backup", opt_backup, (void*) 1, "-b",
		opt_backup, (void*) 1, "--postexec", opt_g_postexec, (void*) 1,
		"--preexec", opt_g_preexec, (void*) 1, "--usleep", opt_g_usleep,
		(void*) 1, "--sleep", opt_g_sleep, (void*) 1, "-arg1", NULL, (void*) 1,
		"--arg1", NULL, (void*) 1, "-arg2",
		NULL, (void*) 1, "--arg2", NULL, (void*) 1, "-arg3", NULL, (void*) 1,
		"--arg3", NULL, (void*) 1, "-m", NULL, (void*) 1, "--imatch",
		opt_g_imatch, (void*) 1, "imatch", opt_g_imatch, (void*) 1, "match",
		opt_g_match, (void*) 1, "--match", opt_g_match, (void*) 1, "--fork",
		opt_g_ex_fork, (void*) 1, "-vvvvv", opt_g_verbose5, (void*) 0, "-vvvv",
		opt_g_verbose4, (void*) 0, "-vvv", opt_g_verbose3, (void*) 0, "-vv",
		opt_g_verbose2, (void*) 0, "-v", opt_g_verbose, (void*) 0, "--loglevel",
		opt_g_loglvl, (void*) 1, "--ftime", opt_g_ftime, (void*) 0, "--logfile",
		opt_log_file, (void*) 0, "--log", opt_logging, (void*) 0, "--silent",
		opt_silent, (void*) 0, "--loopexec", opt_g_loopexec, (void*) 1,
		"--loop", opt_g_loop, (void*) 1, "--daemon", opt_g_daemonize, (void*) 0,
		"-w", opt_online_dump, (void*) 0, "--ipc", opt_shmipc, (void*) 1, "-l",
		opt_lastonlog_dump, (void*) 0, "--ge1log", opt_GE1LOG, (void*) 1,
		"--gamelog", opt_gamelog, (void*) 1, "--tvlog", opt_tvlog, (void*) 1,
		"--imdblog", opt_imdblog, (void*) 1, "--oneliners", opt_oneliner,
		(void*) 1, "-o", opt_oneliner_dump, (void*) 0, "--lastonlog",
		opt_lastonlog, (void*) 1, "-i", opt_dupefile_dump, (void*) 0,
		"--dupefile", opt_dupefile, (void*) 1, "--nowbuffer", opt_g_buffering,
		(void*) 0, "--raw", opt_raw_dump, (void*) 0, "--binary", opt_binary,
		(void*) 0, "iregexi", opt_g_iregexi, (void*) 1, "--iregexi",
		opt_g_iregexi, (void*) 1, "iregex", opt_g_iregex, (void*) 1, "--iregex",
		opt_g_iregex, (void*) 1, "regexi", opt_g_regexi, (void*) 1, "--regexi",
		opt_g_regexi, (void*) 1, "regex", opt_g_regex, (void*) 1, "--regex",
		opt_g_regex, (void*) 1, "-e", opt_rebuild, (void*) 1, "--comp",
		opt_compact_output_formatting, (void*) 0, "--batch",
		opt_batch_output_formatting, (void*) 0, "-y", opt_g_followlinks,
		(void*) 0, "--allowsymbolic", opt_g_followlinks, (void*) 0,
		"--followlinks", opt_g_followlinks, (void*) 0, "--allowlinks",
		opt_g_followlinks, (void*) 0, "--execv", opt_execv, (void*) 1, "-execv",
		opt_execv, (void*) 1, "-exec", opt_exec, (void*) 1, "--exec", opt_exec,
		(void*) 1, "--fix", opt_g_fix, (void*) 0, "-u", opt_g_update, (void*) 0,
		"--memlimit", opt_membuffer_limit, (void*) 1, "-p", opt_dirlog_chk_dupe,
		(void*) 0, "--dupechk", opt_dirlog_chk_dupe, (void*) 0, "--nobuffer",
		opt_g_nobuffering, (void*) 0, "-n", opt_dirlog_dump_nukelog, (void*) 0,
		"--help", print_help, (void*) 0, "--version", print_version, (void*) 0,
		"--folders", opt_dirlog_sections_file, (void*) 1, "--dirlog",
		opt_dirlog_file, (void*) 1, "--nukelog", opt_nukelog_file, (void*) 1,
		"--siteroot", opt_siteroot, (void*) 1, "--glroot", opt_glroot,
		(void*) 1, "--nowrite", opt_g_nowrite, (void*) 0, "--sfv", opt_g_sfv,
		(void*) 0, "--crc32", option_crc32, (void*) 1, "--nobackup",
		opt_nobackup, (void*) 0, "-c", opt_dirlog_check, (void*) 0, "--check",
		opt_dirlog_check, (void*) 0, "--dump", opt_dirlog_dump, (void*) 0, "-d",
		opt_dirlog_dump, (void*) 0, "-f", opt_g_force, (void*) 0, "-ff",
		opt_g_force2, (void*) 0, "-s", opt_update_single_record, (void*) 1,
		"-r", opt_recursive_update_records, (void*) 0, "--shmem", opt_g_shmem,
		(void*) 0, "--shmreload", opt_g_shmreload, (void*) 0, "--loadq",
		opt_g_loadq, (void*) 0, "--shmdestroy", opt_g_shmdestroy, (void*) 0,
		"--shmdestonexit", opt_g_shmdestroyonexit, (void*) 0, "--maxres",
		opt_g_maxresults, (void*) 1, "--maxhit", opt_g_maxhits, (void*) 1,
		"--ifres", opt_g_ifres, (void*) 0, "--ifhit", opt_g_ifhit, (void*) 0,
		"--nofq", opt_g_nofq, (void*) 0, NULL, NULL, NULL };

int md_init(pmda md, int nm) {
	if (!md || md->objects) {
		return 1;
	}
	bzero(md, sizeof(mda));
	md->objects = calloc(nm, sizeof(md_obj));
	md->count = nm;
	md->pos = md->objects;
	md->r_pos = md->objects;
	md->first = md->objects;
	return 0;
}

int md_g_free(pmda md) {
	if (!md || !md->objects)
		return 1;

	if (!(md->flags & F_MDA_REFPTR)) {
		p_md_obj ptr = md_first(md), ptr_s;
		while (ptr) {
			ptr_s = ptr->next;
			if (ptr->ptr) {
				g_free(ptr->ptr);
				ptr->ptr = NULL;
			}
			ptr = ptr_s;
		}
	}

	g_free(md->objects);
	bzero(md, sizeof(mda));

	return 0;
}

uintaa_t md_relink(pmda md) {
	off_t off, l = 1;

	p_md_obj last = NULL, cur = md->objects;

	for (off = 0; off < md->count; off++) {
		if (cur->ptr) {
			if (last) {
				last->next = cur;
				cur->prev = last;
				l++;
			} else {
				md->first = cur;
			}
			last = cur;
		}
		cur++;
	}
	return l;
}

p_md_obj md_first(pmda md) {

	if (md->first && md->first != md->objects) {
		if (md->first->ptr) {
			return md->first;
		}
	}

	off_t off = 0;
	p_md_obj ptr = md->objects;

	for (off = 0; off < md->count; off++, ptr++) {
		if (ptr->ptr) {
			return ptr;
		}
	}

	return NULL;
}

p_md_obj md_last(pmda md) {
	p_md_obj ptr = md_first(md);

	if (!ptr) {
		return ptr;
	}

	while (ptr->next) {
		ptr = ptr->next;
	}

	return ptr;
}

#define MDA_MDALLOC_RE	0x1

void *md_alloc(pmda md, int b) {
	int flags = 0;

	if (md->offset >= md->count) {
		if (gfl & F_OPT_VERBOSE5) {
			print_str(
					"NOTICE: re-allocating memory segment to increase size; current address: 0x%.16llX, current size: %llu\n",
					(ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count);
		}
		md->objects = realloc(md->objects, (md->count * sizeof(md_obj)) * 2);
		md->pos = md->objects;
		md->pos += md->count;
		bzero(md->pos, md->count * sizeof(md_obj));

		md->count *= 2;
		uintaa_t rlc = md_relink(md);
		flags |= MDA_MDALLOC_RE;
		if (gfl & F_OPT_VERBOSE5) {
			print_str(
					"NOTICE: re-allocation done; new address: 0x%.16llX, new size: %llu, re-linked %llu records\n",
					(ulint64_t) (uintaa_t) md->objects, (ulint64_t) md->count,
					(ulint64_t) rlc);
		}
	}

	p_md_obj prev = md->pos;
	uintaa_t pcntr = 0;
	while (md->pos->ptr
			&& (pcntr = ((md->pos - md->objects) / sizeof(md_obj))) < md->count) {
		md->pos++;
	}

	if (pcntr >= md->count) {
		return NULL;
	}

	if (md->pos > md->objects && !(md->pos - 1)->ptr) {
		flags |= MDA_MDALLOC_RE;
	}

	if (md->pos->ptr)
		return NULL;

	if (md->flags & F_MDA_REFPTR) {
		md->pos->ptr = md->lref_ptr;
	} else {
		md->pos->ptr = calloc(1, b);
	}

	if (prev != md->pos) {
		prev->next = md->pos;
		md->pos->prev = prev;
	}

	md->offset++;

	if (flags & MDA_MDALLOC_RE) {
		md_relink(md);
	}

	return md->pos->ptr;
}

void *md_unlink(pmda md, p_md_obj md_o) {
	if (!md_o) {
		return NULL;
	}

	p_md_obj c_ptr = NULL;

	if (md_o->prev) {
		((p_md_obj) md_o->prev)->next = (p_md_obj) md_o->next;
		c_ptr = md_o->prev;
	}

	if (md_o->next) {
		((p_md_obj) md_o->next)->prev = (p_md_obj) md_o->prev;
		c_ptr = md_o->next;
	}

	if (md->first == md_o) {
		if (md_o->prev) {
			md->first = md_o->prev;
		} else if (md_o->next) {
			md->first = md_o->next;
		} else {
			md->first = md->objects;
		}
	}

	md->offset--;
	if (md->pos == md_o && c_ptr) {
		md->pos = c_ptr;
	}
	if (!(md->flags & F_MDA_REFPTR) && md_o->ptr) {
		g_free(md_o->ptr);
	}
	md_o->ptr = NULL;

	return (void*) c_ptr;
}

void *md_swap(pmda md, p_md_obj md_o1, p_md_obj md_o2) {
	if (!md_o1 || !md_o2) {
		return NULL;
	}

	void *ptr2_s;

	ptr2_s = md_o1->prev;
	md_o1->next = md_o2->next;
	md_o1->prev = md_o2;
	md_o2->next = md_o1;
	md_o2->prev = ptr2_s;

	if (md_o2->prev) {
		((p_md_obj) md_o2->prev)->next = md_o2;
	}

	if (md_o1->next) {
		((p_md_obj) md_o1->next)->prev = md_o1;
	}

	if (md->first == md_o1) {
		md->first = md_o2;
	}

	/*if (md->pos == md_o1) {
	 md->pos = md_o2;
	 }

	 if (md->pos == md_o2) {
	 md->pos = md_o1;
	 }*/

	return md_o2->next;
}

int setup_sighandlers(void) {
	struct sigaction sa = { { 0 } }, sa_c = { { 0 } }, sa_e = { { 0 } };
	int r = 0;

	sa.sa_handler = &sig_handler;
	sa.sa_flags = SA_RESTART;

	sa_c.sa_sigaction = &child_sig_handler;
	sa_c.sa_flags = SA_RESTART | SA_SIGINFO;

	sa_e.sa_sigaction = sighdl_error;
	sa_e.sa_flags = SA_RESTART | SA_SIGINFO;

	sigfillset(&sa.sa_mask);
	sigfillset(&sa_c.sa_mask);
	sigemptyset(&sa_e.sa_mask);

	r += sigaction(SIGINT, &sa, NULL);
	r += sigaction(SIGQUIT, &sa, NULL);
	r += sigaction(SIGABRT, &sa, NULL);
	r += sigaction(SIGTERM, &sa, NULL);
	r += sigaction(SIGCHLD, &sa_c, NULL);
	r += sigaction(SIGSEGV, &sa_e, NULL);
	r += sigaction(SIGILL, &sa_e, NULL);
	r += sigaction(SIGFPE, &sa_e, NULL);
	r += sigaction(SIGBUS, &sa_e, NULL);
	r += sigaction(SIGTRAP, &sa_e, NULL);

	signal(SIGKILL, sig_handler);

	return r;
}

int g_shutdown(void *arg) {
	g_setjmp(0, "g_shutdown", NULL, NULL);
	g_cleanup(&g_act_1);
	g_cleanup(&g_act_2);
	free_cfg_rf(&cfg_rf);
	free_cfg(&glconf);
	g_setjmp(0, "g_shutdown(2)", NULL, NULL);
	if (NUKESTR) {
		g_free(NUKESTR);
	}

	if ((gfl & F_OPT_PS_LOGGING) && fd_log) {
		g_fclose(fd_log);
	}

	if (_p_macro_argv) {
		g_free(_p_macro_argv);
	}

	if (GLOBAL_PREEXEC) {
		g_free(GLOBAL_PREEXEC);
	}

	if (GLOBAL_POSTEXEC) {
		g_free(GLOBAL_POSTEXEC);
	}

	if (LOOPEXEC) {
		g_free(LOOPEXEC);
	}

	if (exec_str) {
		g_free(exec_str);
	}

	if (pf_infile) {
		g_fclose(pf_infile);
	}

	if (exec_v) {
		int i;

		for (i = 0; i < exec_vc && exec_v[i]; i++) {
			g_free(exec_v[i]);
		}
		g_free(exec_v);

	}

	md_g_free(&_match_rr);
	md_g_free(&_md_gsort);
	md_g_free(&_lom_strings);

	_p_macro_argc = 0;

	exit(EXITVAL);
}

int g_shm_cleanup(__g_handle hdl) {
	int r = 0;

	if (shmdt(hdl->data) == -1) {
		r++;
	}

	if ((hdl->flags & F_GH_SHM) && (hdl->flags & F_GH_SHMDESTONEXIT)) {
		if (shmctl(hdl->shmid, IPC_RMID, NULL) == -1) {
			r++;
		}
	}

	hdl->data = NULL;
	hdl->shmid = 0;

	return r;
}

int g_cleanup(__g_handle hdl) {
	int r = 0;

	r += md_g_free(&hdl->buffer);
	r += md_g_free(&hdl->w_buffer);

	p_md_obj ptr;

	if (hdl->_match_rr.objects) {
		ptr = md_first(&hdl->_match_rr);

		while (ptr) {
			__g_match g_ptr = (__g_match) ptr->ptr;
			if ( g_ptr->flags & F_GM_ISLOM) {
				md_g_free(&g_ptr->lom);
			}
			ptr = ptr->next;
		}

		r += md_g_free(&hdl->_match_rr);
	}

	if (hdl->exec_args.ac_ref.objects) {
		ptr = md_first(&hdl->exec_args.ac_ref);
		while (ptr) {
			int *t = (int*) ptr->ptr;
			g_free(hdl->exec_args.argv_c[*t]);
			ptr = ptr->next;
		}
		if (hdl->exec_args.argv_c) {
			g_free(hdl->exec_args.argv_c);
		}
		r += md_g_free(&hdl->exec_args.ac_ref);
	}

	if (!(hdl->flags & F_GH_ISSHM) && hdl->data) {
		g_free(hdl->data);
	} else if ((hdl->flags & F_GH_ISSHM) && hdl->data) {
		g_shm_cleanup(hdl);
	}
	bzero(hdl, sizeof(_g_handle));
	return r;
}

char *build_data_path(char *file, char *path, char *sd) {
	remove_repeating_chars(path, 0x2F);

	if (strlen(path) && !file_exists(path)) {
		return path;
	}

	char buffer[PATH_MAX] = { 0 };

	snprintf(buffer, PATH_MAX, "%s/%s/%s/%s", GLROOT, FTPDATA, sd, file);

	remove_repeating_chars(buffer, 0x2F);

	size_t blen = strlen(buffer), plen = strlen(path);

	if (blen > 255) {
		return path;
	}

	if (plen == blen && !strncmp(buffer, path, plen)) {
		return path;
	}

	if (gfl & F_OPT_VERBOSE3) {
		print_str("NOTICE: %s: was not found, setting default data path: %s\n",
				path, buffer);
	}

	bzero(path, 255);
	g_memcpy(path, buffer, strlen(buffer));
	return path;
}

void enable_logging(void) {
	if ((gfl & F_OPT_PS_LOGGING) && !fd_log) {
		if (!(ofl & F_OVRR_LOGFILE)) {
			build_data_path(DEFF_DULOG, LOGFILE, DEFPATH_LOGS);
		}
		if (!(fd_log = gg_fopen(LOGFILE, "a"))) {
			gfl ^= F_OPT_PS_LOGGING;
			print_str(
					"ERROR: %s: [%d]: could not open file for writing, logging disabled\n",
					LOGFILE, errno);
		}
	}
	return;
}

#define F_LCONF_NORF 	0x1
#define MSG_INIT_PATH_OVERR 	"NOTICE: %s path set to '%s'\n"

int g_init(int argc, char **argv) {
	g_setjmp(0, "g_init", NULL, NULL);
	int r;

	if (strlen(LOGFILE)) {
		gfl |= F_OPT_PS_LOGGING;
	}
	r = parse_args(argc, argv, f_ref);

	if (r == -2 || r == -1) {
		print_help(NULL, -1);
		return 4;
	}

	if (r > 0) {
		print_str("ERROR: [%d] processing command line arguments failed\n", r);
		EXITVAL = 2;
		return EXITVAL;
	}

	enable_logging();

	if (updmode && updmode != UPD_MODE_NOOP && !(gfl & F_OPT_FORMAT_BATCH)
			&& !(gfl & F_OPT_FORMAT_COMP) && (gfl & F_OPT_VERBOSE)) {
		print_str("INIT: glutil %d.%d-%d%s-%s starting [PID: %d]\n",
		VER_MAJOR,
		VER_MINOR,
		VER_REVISION, VER_STR, __STR_ARCH, getpid());
	}

#ifdef GLCONF

	if ((r = load_cfg(&glconf, GLCONF, F_LCONF_NORF, NULL))
			&& (gfl & F_OPT_VERBOSE)) {
		print_str("WARNING: %s: could not load GLCONF file [%d]\n", GLCONF, r);
	}

	if ((gfl & F_OPT_VERBOSE4) && glconf.offset) {
		print_str("NOTICE: %s: loaded %d config lines into memory\n",
		GLCONF, (int) glconf.offset);
	}

	p_md_obj ptr = get_cfg_opt("ipc_key", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_IPC)) {
		SHM_IPC = (key_t) strtol(ptr->ptr, NULL, 16);
	}

	ptr = get_cfg_opt("rootpath", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_GLROOT)) {
		bzero(GLROOT, 255);
		g_memcpy(GLROOT, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE4)) {
			print_str("NOTICE: GLCONF: using 'rootpath': %s\n", GLROOT);
		}
	}

	ptr = get_cfg_opt("min_homedir", &glconf, NULL);

	if (ptr && !(ofl & F_OVRR_SITEROOT)) {
		bzero(SITEROOT_N, 255);
		g_memcpy(SITEROOT_N, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE4)) {
			print_str("NOTICE: GLCONF: using 'min_homedir': %s\n", SITEROOT_N);
		}
	}

	ptr = get_cfg_opt("ftp-data", &glconf, NULL);

	if (ptr) {
		bzero(FTPDATA, 255);
		g_memcpy(FTPDATA, ptr->ptr, strlen((char*) ptr->ptr));
		if ((gfl & F_OPT_VERBOSE4)) {
			print_str("NOTICE: GLCONF: using 'ftp-data': %s\n", FTPDATA);
		}
	}

	ptr = get_cfg_opt("nukedir_style", &glconf, NULL);

	if (ptr) {
		NUKESTR = calloc(255, 1);
		NUKESTR = string_replace(ptr->ptr, "%N", "%s", NUKESTR, 255);
		if ((gfl & F_OPT_VERBOSE4)) {
			print_str("NOTICE: GLCONF: using 'nukedir_style': %s\n", NUKESTR);
		}
		ofl |= F_OVRR_NUKESTR;

	}

	remove_repeating_chars(FTPDATA, 0x2F);
#else
	print_str("WARNING: GLCONF not defined in glconf.h");
#endif

	remove_repeating_chars(GLROOT, 0x2F);
	remove_repeating_chars(SITEROOT_N, 0x2F);

	if (!strlen(GLROOT)) {
		print_str("ERROR: glftpd root directory not specified!\n");
		return 2;
	}

	if (!strlen(SITEROOT_N)) {
		print_str("ERROR: glftpd site root directory not specified!\n");
		return 2;
	}

	if (updmode != UPD_MODE_WRITE && updmode != UPD_MODE_RECURSIVE) {
		build_data_path(DEFF_DIRLOG, DIRLOG, DEFPATH_LOGS);
		build_data_path(DEFF_NUKELOG, NUKELOG, DEFPATH_LOGS);
		build_data_path(DEFF_LASTONLOG, LASTONLOG, DEFPATH_LOGS);
		build_data_path(DEFF_DUPEFILE, DUPEFILE, DEFPATH_LOGS);
		build_data_path(DEFF_ONELINERS, ONELINERS, DEFPATH_LOGS);
		build_data_path(DEFF_IMDB, IMDBLOG, DEFPATH_LOGS);
		build_data_path(DEFF_GAMELOG, GAMELOG, DEFPATH_LOGS);
		build_data_path(DEFF_TV, TVLOG, DEFPATH_LOGS);
		build_data_path(DEFF_GEN1, GE1LOG, DEFPATH_LOGS);
	}

	bzero(SITEROOT, 255);
	snprintf(SITEROOT, 254, "%s%s", GLROOT, SITEROOT_N);
	remove_repeating_chars(SITEROOT, 0x2F);

	if (dir_exists(SITEROOT) && !dir_exists(SITEROOT_N)) {
		snprintf(SITEROOT, 254, "%s", SITEROOT_N);
	}

	if ((gfl & F_OPT_VERBOSE) && dir_exists(SITEROOT)) {
		print_str("WARNING: no valid siteroot!\n");
	}

	if (!updmode && (gfl & F_OPT_SFV)) {
		updmode = UPD_MODE_RECURSIVE;
		if (!(gfl & F_OPT_NOWRITE)) {
			gfl |= F_OPT_FORCEWSFV | F_OPT_NOWRITE;
		}

		if ((gfl & F_OPT_VERBOSE)) {
			print_str(
					"NOTICE: switching to non-destructive filesystem rebuild mode\n");
		}
	}

	if ((gfl & F_OPT_VERBOSE)) {
		if (gfl & F_OPT_NOBUFFER) {
			print_str("NOTICE: disabling memory buffering\n");
			if (gfl & F_OPT_SHAREDMEM) {
				print_str(
						"WARNING: --shmem: shared memory segment buffering option is invalid when --nobuffer specified\n");
			}
		}
		if (SHM_IPC && SHM_IPC != shm_ipc) {
			print_str("NOTICE: IPC key set to '0x%.8X'\n", SHM_IPC);
		}
		if (ofl & F_OVRR_GLROOT) {
			print_str(MSG_INIT_PATH_OVERR, "GLROOT", GLROOT);
		}
		if (ofl & F_OVRR_SITEROOT) {
			print_str(MSG_INIT_PATH_OVERR, "SITEROOT", SITEROOT);
		}
		if (ofl & F_OVRR_DIRLOG) {
			print_str(MSG_INIT_PATH_OVERR, "DIRLOG", DIRLOG);
		}
		if (ofl & F_OVRR_NUKELOG) {
			print_str(MSG_INIT_PATH_OVERR, "NUKELOG", NUKELOG);
		}
		if (ofl & F_OVRR_DUPEFILE) {
			print_str(MSG_INIT_PATH_OVERR, "DUPEFILE", DUPEFILE);
		}
		if (ofl & F_OVRR_LASTONLOG) {
			print_str(MSG_INIT_PATH_OVERR, "LASTONLOG", LASTONLOG);
		}
		if (ofl & F_OVRR_ONELINERS) {
			print_str(MSG_INIT_PATH_OVERR, "ONELINERS", ONELINERS);
		}
		if (ofl & F_OVRR_IMDBLOG) {
			print_str(MSG_INIT_PATH_OVERR, "IMDBLOG", IMDBLOG);
		}
		if (ofl & F_OVRR_TVLOG) {
			print_str(MSG_INIT_PATH_OVERR, "TVLOG", TVLOG);
		}
		if (ofl & F_OVRR_GAMELOG) {
			print_str(MSG_INIT_PATH_OVERR, "GAMELOG", GAMELOG);
		}
		if (ofl & F_OVRR_GE1LOG) {
			print_str(MSG_INIT_PATH_OVERR, "GE1LOG", GE1LOG);
		}
		if ((gfl & F_OPT_VERBOSE2) && (gfl & F_OPT_PS_LOGGING)) {
			print_str("NOTICE: Logging enabled: %s\n", LOGFILE);
		}
	}

	if ((gfl & F_OPT_VERBOSE) && (gfl & F_OPT_NOWRITE)) {
		print_str("WARNING: performing dry run, no writing will be done\n");
	}

	if (gfl & F_OPT_DAEMONIZE) {
		print_str("NOTICE: forking into background.. [PID: %d]\n", getpid());
		if (daemon(1, 0) == -1) {
			print_str(
					"ERROR: [%d] could not fork into background, terminating..\n",
					errno);
			return errno;
		}
	}

	if (updmode && (gfl & F_OPT_PREEXEC)) {
		if (gfl & F_OPT_VERBOSE) {
			print_str("PREEXEC: running: '%s'\n", GLOBAL_PREEXEC);
		}
		int r_e = 0;
		if ((r_e = g_do_exec(NULL, ref_to_val_generic, GLOBAL_PREEXEC, NULL))
				== -1 || WEXITSTATUS(r_e)) {
			if (gfl & F_OPT_VERBOSE) {
				print_str("WARNING: [%d]: PREEXEC returned non-zero: '%s'\n",
				WEXITSTATUS(r_e), GLOBAL_PREEXEC);
			}
			return 1;
		}
	}

	if (g_usleep) {
		usleep(g_usleep);
	} else if (g_sleep) {
		sleep(g_sleep);
	}

	uint64_t mloop_c = 0;
	char m_b1[128];
	int m_f = 0x1;

	g_setjmp(0, "main(start)", NULL, NULL);

	enter:

	if ((m_f & 0x1)) {
		snprintf(m_b1, 127, "main(loop) [c:%llu]",
				(long long unsigned int) mloop_c);
		g_setjmp(0, m_b1, NULL, NULL);
		m_f ^= 0x1;
	}

	switch (updmode) {
	case UPD_MODE_RECURSIVE:
		EXITVAL = rebuild_dirlog();
		break;
	case UPD_MODE_SINGLE:
		EXITVAL = dirlog_update_record(argv_off);
		break;
	case UPD_MODE_CHECK:
		EXITVAL = dirlog_check_records();
		break;
	case UPD_MODE_DUMP:
		EXITVAL = g_print_stats(DIRLOG, 0, 0);
		break;
	case UPD_MODE_DUMP_NUKE:
		EXITVAL = g_print_stats(NUKELOG, 0, 0);
		break;
	case UPD_MODE_DUMP_DUPEF:
		EXITVAL = g_print_stats(DUPEFILE, 0, 0);
		break;
	case UPD_MODE_DUMP_LON:
		EXITVAL = g_print_stats(LASTONLOG, 0, 0);
		break;
	case UPD_MODE_DUMP_ONEL:
		EXITVAL = g_print_stats(ONELINERS, 0, 0);
		break;
	case UPD_MODE_DUMP_IMDB:
		EXITVAL = g_print_stats(IMDBLOG, 0, 0);
		break;
	case UPD_MODE_DUMP_GAME:
		EXITVAL = g_print_stats(GAMELOG, 0, 0);
		break;
	case UPD_MODE_DUMP_TV:
		EXITVAL = g_print_stats(TVLOG, 0, 0);
		break;
	case UPD_MODE_DUMP_GENERIC:
		EXITVAL = d_gen_dump(p_argv_off);
		break;
	case UPD_MODE_DUPE_CHK:
		EXITVAL = dirlog_check_dupe();
		break;
	case UPD_MODE_REBUILD:
		EXITVAL = rebuild(p_argv_off);
		break;
	case UPD_MODE_DUMP_ONL:
		EXITVAL = g_print_stats("ONLINE USERS", F_DL_FOPEN_SHM, ON_SZ);
		break;
	case UPD_MODE_FORK:
		if (p_argv_off) {
			if ((EXITVAL = WEXITSTATUS(system(p_argv_off)))) {
				if (gfl & F_OPT_VERBOSE) {
					print_str("WARNING: '%s': command failed, code %d\n",
							p_argv_off, EXITVAL);
				}
			}
		}
		break;
	case UPD_MODE_BACKUP:
		EXITVAL = data_backup_records(g_dgetf(p_argv_off));
		break;
	case UPD_MODE_DUMP_USERS:
		EXITVAL = g_dump_ug(DEFPATH_USERS);
		break;
	case UPD_MODE_DUMP_GRPS:
		EXITVAL = g_dump_ug(DEFPATH_GROUPS);
		break;
	case UPD_MODE_DUMP_GEN:
		EXITVAL = g_dump_gen(p_argv_off);
		break;
	case UPD_MODE_WRITE:
		EXITVAL = d_write((char*) p_argv_off);
		break;
	case PRIO_UPD_MODE_INFO:
		g_print_info();
		break;
	case UPD_MODE_NOOP:
		break;
	default:
		print_str("ERROR: no mode specified\n");
		print_help(NULL, -1);
		break;
	}

	if ((gfl & F_OPT_LOOP) && !(gfl & F_OPT_KILL_GLOBAL)
			&& (!loop_max || mloop_c < loop_max - 1)) {
		g_cleanup(&g_act_1);
		g_cleanup(&g_act_2);
		free_cfg_rf(&cfg_rf);
		sleep(loop_interval);
		if (gfl & F_OPT_LOOPEXEC) {
			g_do_exec(NULL, ref_to_val_generic, LOOPEXEC, NULL);
		}
		mloop_c++;
		goto enter;
	}

	if (updmode && (gfl & F_OPT_POSTEXEC)) {
		if (gfl & F_OPT_VERBOSE) {
			print_str("POSTEXEC: running: '%s'\n", GLOBAL_POSTEXEC);
		}
		if (g_do_exec(NULL, ref_to_val_generic, GLOBAL_POSTEXEC, NULL) == -1) {
			if (gfl & F_OPT_VERBOSE) {
				print_str("WARNING: POSTEXEC failed: '%s'\n", GLOBAL_POSTEXEC);
			}
		}
	}

	return EXITVAL;
}

int main(int argc, char *argv[]) {
	g_setjmp(0, "main", NULL, NULL);
	char **p_argv = (char**) argv;
	int r;

	if ((r = setup_sighandlers())) {
		print_str(
				"WARNING: UNABLE TO SETUP SIGNAL HANDLERS! (this is weird, please report it!) [%d]\n",
				r);
		sleep(5);
	}

	_p_macro_argc = argc;

	parse_args(argc, argv, prio_f_ref);

	enable_logging();

	switch (updmode) {
	case PRIO_UPD_MODE_MACRO:
		;
		char **ptr;
		ptr = process_macro(prio_argv_off, NULL);
		if (ptr) {
			_p_macro_argv = p_argv = ptr;
			gfl = F_OPT_WBUFFER
					| (gfl & F_OPT_PS_LOGGING ? F_OPT_PS_LOGGING : 0);
		} else {
			g_shutdown(NULL);
		}
		break;
	case PRIO_UPD_MODE_INFO:
		g_print_info();
		g_shutdown(NULL);
		break;
	}

	updmode = 0;

	g_init(_p_macro_argc, p_argv);

	g_shutdown(NULL);

	return EXITVAL;
}

#define MSG_NL 	"\n"

int g_print_info(void) {
	char buffer[4096];
	print_version_long(NULL, 0);
	print_str(MSG_NL);
	snprintf(buffer, 4095, "EP: @0x%s\n", __AA_SPFH);
	print_str(buffer, (uintaa_t) main);
	print_str(MSG_NL);
	print_str(" DATA SRC   BLOCK SIZE(B)   \n"
			"--------------------------\n");
	print_str(" DIRLOG          %d        \n", DL_SZ);
	print_str(" NUKELOG         %d        \n", NL_SZ);
	print_str(" DUPEFILE        %d        \n", DF_SZ);
	print_str(" LASTONLOG       %d        \n", LO_SZ);
	print_str(" ONELINERS       %d        \n", LO_SZ);
	print_str(" IMDBLOG         %d        \n", ID_SZ);
	print_str(" GAMELOG         %d        \n", GM_SZ);
	print_str(" TVLOG           %d        \n", TV_SZ);
	print_str(" GE1             %d        \n", G1_SZ);
	print_str(" ONLINE(SHR)     %d        \n", OL_SZ);
	print_str(MSG_NL);
	if (gfl & F_OPT_VERBOSE) {
		print_str("  TYPE         SIZE(B)   \n"
				"-------------------------\n");
		print_str(" off_t            %d      \n", (sizeof(off_t)));
		print_str(" uintaa_t         %d      \n", (sizeof(uintaa_t)));
		print_str(" uint8_t          %d      \n", (sizeof(uint8_t)));
		print_str(" uint16_t         %d      \n", (sizeof(uint16_t)));
		print_str(" uint32_t         %d      \n", (sizeof(uint32_t)));
		print_str(" uint64_t         %d      \n", (sizeof(uint64_t)));
		print_str(" size_t           %d      \n", (sizeof(size_t)));
		print_str(MSG_NL);
		print_str(" void *           %d      \n", PTRSZ);
		print_str(MSG_NL);
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str(" FILE TYPE     DECIMAL   \n"
				"-------------------------\n");
		print_str(" DT_UNKNOWN       %d     \n", DT_UNKNOWN);
		print_str(" DT_FIFO          %d     \n", DT_FIFO);
		print_str(" DT_CHR           %d     \n", DT_CHR);
		print_str(" DT_DIR           %d     \n", DT_DIR);
		print_str(" DT_BLK           %d     \n", DT_BLK);
		print_str(" DT_REG           %d     \n", DT_REG);
		print_str(" DT_LNK           %d     \n", DT_LNK);
		print_str(" DT_SOCK          %d     \n", DT_SOCK);
		print_str(" DT_WHT           %d     \n", DT_WHT);
		print_str(MSG_NL);
	}

	return 0;
}

char **process_macro(void * arg, char **out) {
	g_setjmp(0, "process_macro", NULL, NULL);
	if (!arg) {
		print_str("ERROR: missing data type argument (-m <macro name>)\n");
		return NULL;
	}

	char *a_ptr = (char*) arg;

	char buffer[PATH_MAX] = { 0 };

	if (self_get_path(buffer)) {
		print_str("ERROR: could not get own path\n");
		return NULL;
	}

	char *dirn = dirname(buffer);

	_si_argv0 av = { 0 };

	av.ret = -1;

	g_strncpy(av.p_buf_1, a_ptr, strlen(a_ptr));

	if (gfl & F_OPT_VERBOSE2) {
		print_str("MACRO: '%s': searching for macro inside '%s/' (recursive)\n",
				av.p_buf_1, dirn);
	}

	_g_eds eds = { 0 };

	if (enum_dir(dirn, ssd_4macro, &av, F_ENUMD_NOXBLK, &eds) < 0) {
		print_str("ERROR: %s: recursion failed (macro not found)\n",
				av.p_buf_1);
		return NULL;
	}

	if (av.ret == -1) {
		print_str("ERROR: %s: could not find macro\n", av.p_buf_1);
		return NULL;
	}

	g_strncpy(b_spec1, av.p_buf_2, strlen(av.p_buf_2));

	if (gfl & F_OPT_VERBOSE2) {
		print_str("MACRO: '%s': found macro in '%s'\n", av.p_buf_1, av.p_buf_2);
	}

	char *s_buffer = (char*) calloc(262144, 1), **s_ptr = NULL;
	int r;

	if ((r = process_exec_string(av.s_ret, s_buffer, MAX_EXEC_STR,
			ref_to_val_macro, NULL))) {

		print_str("ERROR: [%d]: could not process exec string: '%s'\n", r,
				av.s_ret);
		goto end;
	}

	int c = 0;
	s_ptr = build_argv(s_buffer, 4096, &c);

	if (!c) {
		print_str("ERROR: %s: macro was declared, but no arguments found\n",
				av.p_buf_1);
	}

	_p_macro_argc = c;

	if (gfl & F_OPT_VERBOSE2) {
		print_str("MACRO: '%s': built argument string array with %d elements\n",
				av.p_buf_1, c);
	}

	if (gfl & F_OPT_VERBOSE) {
		print_str("MACRO: '%s': EXECUTING: '%s'\n", av.p_buf_1, s_buffer);
	}

	end:

	g_free(s_buffer);

	return s_ptr;
}

char **build_argv(char *args, size_t max, int *c) {
	char **ptr = (char **) calloc(max, sizeof(char **));

	size_t args_l = strlen(args);
	int i_0, l_p = 0, b_c = 0;
	char sp_1 = 0x20, sp_2 = 0x22, sp_3 = 0x60;

	*c = 0;

	for (i_0 = 0; i_0 <= args_l && b_c < max; i_0++) {
		if (i_0 == 0) {
			while (args[i_0] == sp_1) {
				i_0++;
			}
			if (args[i_0] == sp_2 || args[i_0] == sp_3) {
				while (args[i_0] == sp_2 || args[i_0] == sp_3) {
					i_0++;
				}
				sp_1 = sp_2;
				l_p = i_0;
			}
		}

		if ((((args[i_0] == sp_1 || (args[i_0] == sp_2 || args[i_0] == sp_3))
				&& args[i_0 - 1] != 0x5C && args[i_0] != 0x5C) || !args[i_0])
				&& i_0 > l_p) {

			if (i_0 == args_l - 1) {
				if (!(args[i_0] == sp_1 || args[i_0] == sp_2
						|| args[i_0] == sp_3)) {
					i_0++;
				}

			}

			size_t ptr_b_l = i_0 - l_p;

			ptr[b_c] = (char*) calloc(ptr_b_l + 1, 1);
			g_strncpy((char*) ptr[b_c], &args[l_p], ptr_b_l);

			b_c++;
			*c += 1;

			int ii_l = 1;
			while (args[i_0] == sp_1 || args[i_0] == sp_2 || args[i_0] == sp_3) {
				if (sp_1 != sp_2 && sp_1 != sp_3) {
					if (args[i_0] == sp_2 || args[i_0] == sp_3) {
						i_0++;
						break;
					}
				}
				i_0++;
			}
			l_p = i_0;
			if (sp_1 == sp_2 || sp_1 == sp_3) {
				sp_1 = 0x20;
				sp_2 = 0x22;
				sp_3 = 0x60;
				while (args[i_0] == sp_1 || args[i_0] == sp_2
						|| args[i_0] == sp_3) {
					i_0++;
				}
				l_p = i_0;
			} else {
				if (args[i_0 - ii_l] == 0x22 || args[i_0 - ii_l] == 0x60) {
					sp_1 = args[i_0 - ii_l];
					sp_2 = args[i_0 - ii_l];
					sp_3 = args[i_0 - ii_l];
					while (args[i_0] == sp_1 || args[i_0] == sp_2
							|| args[i_0] == sp_3) {
						i_0++;
					}
					l_p = i_0;
				}

			}
		}

	}

	return ptr;
}

char *g_dgetf(char *str) {
	if (!str) {
		return NULL;
	}
	if (!strncmp(str, "dirlog", 6)) {
		return DIRLOG;
	} else if (!strncmp(str, "nukelog", 7)) {
		return NUKELOG;
	} else if (!strncmp(str, "dupefile", 8)) {
		return DUPEFILE;
	} else if (!strncmp(str, "lastonlog", 9)) {
		return LASTONLOG;
	} else if (!strncmp(str, "oneliners", 9)) {
		return ONELINERS;
	} else if (!strncmp(str, "imdb", 4)) {
		return IMDBLOG;
	} else if (!strncmp(str, "game", 4)) {
		return GAMELOG;
	} else if (!strncmp(str, "tvrage", 6)) {
		return TVLOG;
	} else if (!strncmp(str, "ge1", 3)) {
		return GE1LOG;
	}
	return NULL;
}

#define MSG_UNRECOGNIZED_DATA_TYPE 	"ERROR: [%s] unrecognized data type\n"

int rebuild(void *arg) {
	g_setjmp(0, "rebuild", NULL, NULL);
	if (!arg) {
		print_str("ERROR: missing data type argument (-e <dirlog|nukelog>)\n");
		return 1;
	}

	char *a_ptr = (char*) arg;
	char *datafile = g_dgetf(a_ptr);

	if (!datafile) {
		print_str(MSG_UNRECOGNIZED_DATA_TYPE, a_ptr);
		return 2;
	}

	if (g_fopen(datafile, "r", F_DL_FOPEN_BUFFER, &g_act_1)) {
		return 3;
	}

	if (!g_act_1.buffer.count) {
		print_str(
				"ERROR: data log rebuilding requires buffering, increase mem limit (or dump with --raw --nobuffer for huge files)\n");
		return 4;
	}

	int r;

	if ((r = rebuild_data_file(datafile, &g_act_1))) {
		print_str(MSG_GEN_DFRFAIL, datafile);
		return 5;
	}

	if (g_act_1.bw || (gfl & F_OPT_VERBOSE4)) {
		print_str(MSG_GEN_WROTE, datafile, (ulint64_t) g_act_1.bw,
				(ulint64_t) g_act_1.rw);
	}

	if ((gfl & F_OPT_NOFQ) && !(g_act_1.flags & F_GH_APFILT)) {
		return 6;
	}

	return 0;
}

int d_gen_dump(char *arg) {
	char *datafile = g_dgetf(arg);

	if (!datafile) {
		print_str(MSG_UNRECOGNIZED_DATA_TYPE, arg);
		return 2;
	}

	return g_print_stats(datafile, 0, 0);
}

#define MAX_DATAIN_F		(V_MB*512)

int d_write(char *arg) {
	g_setjmp(0, "d_write", NULL, NULL);

	int ret = 0;

	if (!arg) {
		print_str("ERROR: missing data type argument\n");
		return 1;
	}

	char *a_ptr = (char*) arg;
	char *datafile = g_dgetf(a_ptr);

	if (!datafile) {
		print_str(MSG_UNRECOGNIZED_DATA_TYPE, a_ptr);
		return 2;
	}

	off_t f_sz = get_file_size(datafile);

	g_act_1.flags |= F_GH_FFBUFFER;

	if (f_sz > 0) {
		g_act_1.flags |= F_GH_WAPPEND | F_GH_DFNOWIPE;
	}

	g_strncpy(g_act_1.file, datafile, strlen(datafile));

	if (determine_datatype(&g_act_1)) {
		print_str(MSG_BAD_DATATYPE, datafile);
		return 3;
	}

	FILE *in;

	if (pf_infile) {
		in = pf_infile;
	} else {
		in = stdin;
		g_act_1.flags |= F_GH_FROMSTDIN;
	}

	off_t fsz = 0;

	int r;

	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: loading data..\n", datafile);
	}

	if (!(gfl & F_OPT_MODE_BINARY)) {
		char *buffer = malloc(MAX_DATAIN_F);
		if (!(fsz = read_file(NULL, buffer, MAX_DATAIN_F, 0, in))) {
			print_str("ERROR: %s: could not read input data\n", datafile);
			ret = 4;
			g_free(buffer);
			goto end;
		}

		if ((r = m_load_input(&g_act_1, buffer))) {
			print_str("ERROR: %s: [%d]: could not parse input data\n", datafile,
					r);
			ret = 5;
			g_free(buffer);
			goto end;
		}
		g_free(buffer);
	} else {
		if (!(g_act_1.flags & F_GH_FROMSTDIN)) {
			g_act_1.total_sz = get_file_size(infile_p);
		} else {
			g_act_1.total_sz = DB_MAX_SIZE;
		}
		if ((r = load_data_md(&g_act_1.w_buffer, infile_p, &g_act_1))) {
			print_str(
					"ERROR: %s: [%d]: could not load input data (binary source)\n",
					datafile, r);
			ret = 12;
			goto end;
		}
	}

	if (g_act_1.flags & F_GH_FROMSTDIN) {
		g_act_1.flags ^= F_GH_FROMSTDIN;
	}

	if (!g_act_1.w_buffer.offset) {
		print_str("ERROR: %s: no records were loaded, aborting..\n", datafile);
		ret = 15;
		goto end;
	}

	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: '%s': parsed and loaded %llu records\n", a_ptr,
				(unsigned long long int) g_act_1.w_buffer.offset);
	}

	if (!(gfl & F_OPT_FORCE2) && !file_exists(datafile)
			&& !g_fopen(datafile, "rb", F_DL_FOPEN_BUFFER, &g_act_1)
			&& g_act_1.buffer.count) {
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: '%s': pruning exact data duplicates..\n", a_ptr);
		}
		p_md_obj ptr_w = md_first(&g_act_1.w_buffer), ptr_r;
		int m = 1;
		while (ptr_w) {
			ptr_r = md_first(&g_act_1.buffer);
			m = 1;
			while (ptr_r) {
				if (!(m = g_bin_compare(ptr_r->ptr, ptr_w->ptr,
						(off_t) g_act_1.block_sz))) {
					if (gfl & F_OPT_VERBOSE5) {
						print_str(
								"NOTICE: record @0x%.16X already exists, not importing\n",
								(unsigned long long int) (uintaa_t) ptr_w);
					}
					if (!md_unlink(&g_act_1.w_buffer, ptr_w)) {
						print_str("%s: %s: [%llu]: %s, aborting build..\n",
								g_act_1.w_buffer.offset ? "ERROR" : "WARNING",
								datafile,
								(unsigned long long int) g_act_1.w_buffer.offset,
								g_act_1.w_buffer.offset ?
										"could not unlink existing record" :
										"all records already exist (nothing to do)");
						ret = 11;
						goto end;
					}
					break;
				}

				ptr_r = ptr_r->next;
			}

			ptr_w = ptr_w->next;
		}
	}

	if (rebuild_data_file(datafile, &g_act_1)) {
		print_str(MSG_GEN_DFRFAIL, datafile);
		ret = 7;
		goto end;
	} else {
		if (g_act_1.bw || (gfl & F_OPT_VERBOSE4)) {
			print_str(MSG_GEN_WROTE, datafile, g_act_1.bw, g_act_1.rw);
		}
	}

	end:

	return ret;
}

int g_bin_compare(const void *p1, const void *p2, off_t size) {
	unsigned char *b_ptr1 = (unsigned char *) p1,
			*b_ptr2 = (unsigned char *) p2, *ptr1, *ptr2;

	ptr1 = b_ptr1 + (size - 1);
	ptr2 = b_ptr2 + (size - 1);

	while (ptr1[0] == ptr2[0] && ptr1 >= b_ptr1) {
		ptr1--;
		ptr2--;
	}
	if (ptr1 < b_ptr1) {
		return 0;
	}
	return 1;
}

typedef struct ___std_rh_0 {
	uint8_t rt_m;
	uint32_t flags;
	uint64_t st_1, st_2;
	_g_handle hdl;
} _std_rh, *__std_rh;

int g_dump_ug(char *ug) {
	g_setjmp(0, "g_dump_ug", NULL, NULL);
	_std_rh ret = { 0 };
	_g_eds eds = { 0 };
	char buffer[PATH_MAX] = { 0 };

	ret.flags = flags_udcfg | F_PD_MATCHREG;
	ret.hdl.flags |= F_GH_ISFSX;
	ret.hdl.g_proc1 = ref_to_val_x;
	ret.hdl.g_proc2 = ref_to_val_ptr_dummy;
	if (g_proc_mr(&ret.hdl)) {
		return 1;
	}

	snprintf(buffer, PATH_MAX, "%s/%s/%s", GLROOT, FTPDATA, ug);

	remove_repeating_chars(buffer, 0x2F);

	return enum_dir(buffer, g_process_directory, &ret, 0, &eds);
}

int g_dump_gen(char *root) {
	g_setjmp(0, "g_dump_gen", NULL, NULL);
	if (!root) {
		return 1;
	}

	_std_rh ret = { 0 };
	_g_eds eds = { 0 };

	ret.flags = flags_udcfg;
	ret.hdl.flags |= F_GH_ISFSX;
	ret.hdl.g_proc1 = ref_to_val_x;
	ret.hdl.g_proc2 = ref_to_val_ptr_dummy;

	if (g_proc_mr(&ret.hdl)) {
		return 1;
	}

	if (!(ret.flags & F_PD_MATCHTYPES)) {
		ret.flags |= F_PD_MATCHTYPES;
	}

	if (!file_exists(root)) {
		ret.flags ^= F_PD_MATCHTYPES;
		ret.flags |= F_PD_MATCHREG;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s is a file\n", root);
		}
		g_process_directory(root, DT_REG, &ret, &eds);
		return ret.rt_m;
	} else if ((gfl & F_OPT_CDIRONLY) && !dir_exists(root)) {
		ret.flags ^= F_PD_MATCHTYPES;
		ret.flags |= F_PD_MATCHDIR;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s is a directory\n", root);
		}
		g_process_directory(root, DT_DIR, &ret, NULL);
		return ret.rt_m;
	}

	char buffer[PATH_MAX] = { 0 };
	snprintf(buffer, PATH_MAX, "%s", root);
	remove_repeating_chars(buffer, 0x2F);

	ret.rt_m = 1;

	enum_dir(buffer, g_process_directory, &ret, 0, &eds);

	print_str("STATS: %s: OK: %llu/%llu\n", root,
			(unsigned long long int) ret.st_1,
			(unsigned long long int) ret.st_1 + ret.st_2);

	return ret.rt_m;
}

int g_process_directory(char *name, unsigned char type, void *arg, __g_eds eds) {
	__std_rh aa_rh = (__std_rh) arg;

	switch (type) {
		case DT_REG:
		if (aa_rh->flags & F_PD_MATCHREG) {
			if ((g_bmatch(name, &aa_rh->hdl, &aa_rh->hdl.buffer))) {
				aa_rh->st_2++;
				break;
			}
			if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_FORMAT_BATCH)) {
				print_str("FILE: %s\n", name);
			} else if (gfl & F_OPT_FORMAT_BATCH) {
				printf("%s\n", name);
			}
			aa_rh->rt_m = 0;
			aa_rh->st_1++;
		}
		break;
		case DT_DIR:
		if (aa_rh->flags & F_PD_RECURSIVE) {
			enum_dir(name, g_process_directory, arg, 0, eds);
		}
		if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM)) {
			break;
		}
		if (aa_rh->flags & F_PD_MATCHDIR) {
			if ((g_bmatch(name, &aa_rh->hdl, &aa_rh->hdl.buffer))) {
				aa_rh->st_2++;
				break;
			}
			if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_FORMAT_BATCH)) {
				print_str("DIR: %s\n", name);
			} else if (gfl & F_OPT_FORMAT_BATCH) {
				printf("%s\n", name);
			}
			aa_rh->rt_m = 0;
			aa_rh->st_1++;
		}
		break;
		case DT_LNK:
		if (gfl & F_OPT_FOLLOW_LINKS) {
			if ((g_bmatch(name, &aa_rh->hdl, &aa_rh->hdl.buffer))) {
				aa_rh->st_2++;
				break;
			}
			if ((gfl & F_OPT_VERBOSE) && !(gfl & F_OPT_FORMAT_BATCH)) {
				print_str("SYMLINK: %s\n", name);
			} else if (gfl & F_OPT_FORMAT_BATCH) {
				printf("%s\n", name);
			}
			aa_rh->rt_m = 0;
			aa_rh->st_1++;
		}
		break;
	}

	return 0;
}

void g_progress_stats(time_t s_t, time_t e_t, off_t total, off_t done) {
	register float diff = (float) (e_t - s_t);
	register float rate = ((float) done / diff);

	fprintf(stderr,
			"PROCESSING: %llu/%llu [ %.2f%s ] | %.2f r/s | ETA: %.1f s\r",
			(long long unsigned int) done, (long long unsigned int) total,
			((float) done / ((float) total / 100.0)), "%", rate,
			(float) (total - done) / rate);

}

int dirlog_check_dupe(void) {
	g_setjmp(0, "dirlog_check_dupe", NULL, NULL);
	struct dirlog buffer, buffer2;
	struct dirlog *d_ptr = NULL, *dd_ptr = NULL;
	char *s_pb, *ss_pb;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER, &g_act_1)) {
		return 2;
	}
	off_t st1, st2 = 0, st3 = 0;
	p_md_obj pmd_st1 = NULL, pmd_st2 = NULL;
	g_setjmp(0, "dirlog_check_dupe(loop)", NULL, NULL);
	time_t s_t = time(NULL), e_t = time(NULL), d_t = time(NULL);

	off_t nrec = g_act_1.total_sz / g_act_1.block_sz;

	if (g_act_1.buffer.count) {
		nrec = g_act_1.buffer.count;
	}

	if (gfl & F_OPT_VERBOSE) {
		g_progress_stats(s_t, e_t, nrec, st3);
	}
	off_t rtt;
	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1,
			g_act_1.block_sz))) {
		st3++;
		if (gfl & F_OPT_KILL_GLOBAL) {
			break;
		}

		if (g_bmatch(d_ptr, &g_act_1, &g_act_1.buffer)) {
			continue;
		}

		rtt = s_string_r(d_ptr->dirname, "/");
		s_pb = &d_ptr->dirname[rtt + 1];
		size_t s_pb_l = strlen(s_pb);

		if (s_pb_l < 4) {
			continue;
		}

		if (gfl & F_OPT_VERBOSE) {
			e_t = time(NULL);

			if (e_t - d_t) {
				d_t = time(NULL);
				g_progress_stats(s_t, e_t, nrec, st3);
			}
		}

		st1 = g_act_1.offset;

		if (!g_act_1.buffer.count) {
			st2 = (off_t) ftello(g_act_1.fh);
		} else {

			pmd_st1 = g_act_1.buffer.r_pos;
			pmd_st2 = g_act_1.buffer.pos;
		}

		int ch = 0;

		while ((dd_ptr = (struct dirlog *) g_read(&buffer2, &g_act_1,
				g_act_1.block_sz))) {
			rtt = s_string_r(dd_ptr->dirname, "/");
			ss_pb = &dd_ptr->dirname[rtt + 1];
			size_t ss_pb_l = strlen(ss_pb);

			if (ss_pb_l == s_pb_l && !strncmp(s_pb, ss_pb, s_pb_l)) {
				if (!ch) {
					print_str("\rDUPE %s               \n", d_ptr->dirname);
					ch++;
				}
				print_str("\rDUPE %s               \n", dd_ptr->dirname);
				if (gfl & F_OPT_VERBOSE) {
					e_t = time(NULL);
					g_progress_stats(s_t, e_t, nrec, st3);
				}
			}
		}

		g_act_1.offset = st1;
		if (!g_act_1.buffer.count) {
			fseeko(g_act_1.fh, (off_t) st2, SEEK_SET);
		} else {
			g_act_1.buffer.r_pos = pmd_st1;
			g_act_1.buffer.pos = pmd_st2;
		}

	}
	if (gfl & F_OPT_VERBOSE) {
		d_t = time(NULL);
		g_progress_stats(s_t, e_t, nrec, st3);
		print_str("\nSTATS: processed %llu/%llu records\n", st3, nrec);
	}
	return 0;
}

int gh_rewind(__g_handle hdl) {
	g_setjmp(0, "gh_rewind", NULL, NULL);
	if (hdl->buffer.count) {
		hdl->buffer.r_pos = hdl->buffer.objects;
		hdl->buffer.pos = hdl->buffer.r_pos;
		hdl->buffer.offset = 0;
		hdl->offset = 0;
	} else {
		if (hdl->fh) {
			rewind(hdl->fh);
			hdl->offset = 0;
		}
	}
	return 0;
}

int dirlog_update_record(char *argv) {
	g_setjmp(0, "dirlog_update_record", NULL, NULL);

	int r, seek = SEEK_END, ret = 0, dr;
	off_t offset = 0;
	uint64_t rl = MAX_uint64_t;
	struct dirlog dl = { 0 };
	ear arg = { 0 };
	arg.dirlog = &dl;

	if (gfl & F_OPT_SFV) {
		gfl |= F_OPT_NOWRITE | F_OPT_FORCE | F_OPT_FORCEWSFV;
	}

	mda dirchain = { 0 };
	p_md_obj ptr;

	md_init(&dirchain, 1024);

	if ((r = split_string(argv, 0x20, &dirchain)) < 1) {
		print_str("ERROR: [dirlog_update_record]: missing arguments\n");
		ret = 1;
		goto r_end;
	}

	data_backup_records(DIRLOG);

	char s_buffer[PATH_MAX];
	ptr = dirchain.objects;
	while (ptr) {
		snprintf(s_buffer, PATH_MAX, "%s/%s", SITEROOT, (char*) ptr->ptr);
		remove_repeating_chars(s_buffer, 0x2F);
		size_t s_buf_len = strlen(s_buffer);
		if (s_buffer[s_buf_len - 1] == 0x2F) {
			s_buffer[s_buf_len - 1] = 0x0;
		}

		rl = dirlog_find(s_buffer, 0, 0, NULL);

		char *mode = "a";

		if (!(gfl & F_OPT_FORCE) && rl < MAX_uint64_t) {
			print_str(
					"WARNING: %s: [%llu] already exists in dirlog (use -f to overwrite)\n",
					(char*) ptr->ptr, rl);
			ret = 4;
			goto end;
		} else if (rl < MAX_uint64_t) {
			if (gfl & F_OPT_VERBOSE) {
				print_str(
						"WARNING: %s: [%llu] overwriting existing dirlog record\n",
						(char*) ptr->ptr, rl);
			}
			offset = rl;
			seek = SEEK_SET;
			mode = "r+";
		}

		if (g_fopen(DIRLOG, mode, 0, &g_act_1)) {
			goto r_end;
		}

		if ((r = release_generate_block(s_buffer, &arg))) {
			if (r < 5) {
				print_str(
						"ERROR: %s: [%d] generating dirlog data chunk failed\n",
						(char*) ptr->ptr, r);
			}
			ret = 3;
			goto end;
		}

		if ((dr = dirlog_write_record(arg.dirlog, offset, seek))) {
			print_str(
			MSG_GEN_DFWRITE, (char*) ptr->ptr, dr, (ulint64_t) offset, mode);
			ret = 6;
			goto end;
		}

		char buffer[2048] = { 0 };

		if (dirlog_format_block(arg.dirlog, buffer) > 0)
			print_str(buffer);

		end:

		g_close(&g_act_1);
		ptr = ptr->next;
	}
	r_end: md_g_free(&dirchain);

	if (dl_stats.bw || (gfl & F_OPT_VERBOSE4)) {
		print_str(MSG_GEN_WROTE, DIRLOG, dl_stats.bw, dl_stats.rw);
	}

	return ret;
}

int option_crc32(void *arg, int m) {
	g_setjmp(0, "option_crc32", NULL, NULL);
	char *buffer;
	if (m == 2) {
		buffer = (char *) arg;
	} else {
		buffer = ((char **) arg)[0];
	}

	updmode = UPD_MODE_NOOP;

	if (!buffer)
		return 1;

	uint32_t crc32;

	uint64_t read = file_crc32(buffer, &crc32);

	if (read)
		print_str("%.8X\n", (uint32_t) crc32);
	else {
		print_str("ERROR: %s: [%d] could not get CRC32\n", buffer,
		errno);
		EXITVAL = 1;
	}

	return 0;
}

int data_backup_records(char *file) {
	g_setjmp(0, "data_backup_records", NULL, NULL);
	int r;
	off_t r_sz;

	if (!file) {
		print_str("ERROR: null argument passed (this is likely a bug)\n");
		return -1;
	}

	if ((gfl & F_OPT_NOWRITE) || (gfl & F_OPT_NOBACKUP)) {
		return 0;
	}

	if (file_exists(file)) {
		if (gfl & F_OPT_VERBOSE3) {
			print_str("WARNING: BACKUP: %s: data file doesn't exist\n", file);
		}
		return 0;
	}

	if (!(r_sz = get_file_size(file))) {
		if ((gfl & F_OPT_VERBOSE)) {
			print_str("WARNING: %s: refusing to backup 0-byte data file\n",
					file);
		}
		return 0;
	}

	char buffer[PATH_MAX];

	snprintf(buffer, PATH_MAX, "%s.bk", file);

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: creating data backup: %s ..\n", file, buffer);
	}

	if ((r = (int) file_copy(file, buffer, "wb", F_FC_MSET_SRC)) < 1) {
		print_str("ERROR: %s: [%d] failed to create backup %s\n", file, r,
				buffer);
		return r;
	}
	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: created data backup: %s\n", file, buffer);
	}
	return 0;
}

int dirlog_check_records(void) {
	g_setjmp(0, "dirlog_check_records", NULL, NULL);
	struct dirlog buffer, buffer4;
	ear buffer3 = { 0 };
	char s_buffer[PATH_MAX], s_buffer2[PATH_MAX], s_buffer3[PATH_MAX] = { 0 };
	buffer3.dirlog = &buffer4;
	int r = 0, r2;
	char *mode = "r";
	uint32_t flags = 0;
	off_t dsz;

	if ((dsz = get_file_size(DIRLOG)) % DL_SZ) {
		print_str(MSG_GEN_DFCORRU, DIRLOG, (ulint64_t) dsz, (int) DL_SZ);
		print_str("NOTICE: use -r to rebuild (see --help)\n");
		return -1;
	}

	if (g_fopen(DIRLOG, mode, F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return 2;
	}

	if (!g_act_1.buffer.count && (gfl & F_OPT_FIX)) {
		print_str(
				"ERROR: internal buffering must be enabled when fixing, increase limit with --memlimit (see --help)\n");
	}

	struct dirlog *d_ptr = NULL;
	int ir;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "dirlog_check_records(loop)",
			NULL,
			NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}
			snprintf(s_buffer, PATH_MAX, "%s/%s", GLROOT, d_ptr->dirname);
			remove_repeating_chars(s_buffer, 0x2F);

			if (d_ptr->status == 1) {
				char *c_nb, *base, *c_nd, *dir;
				c_nb = strdup(d_ptr->dirname);
				base = basename(c_nb);
				c_nd = strdup(d_ptr->dirname);
				dir = dirname(c_nd);

				snprintf(s_buffer2, PATH_MAX, NUKESTR, base);
				snprintf(s_buffer3, PATH_MAX, "%s/%s/%s", GLROOT, dir,
						s_buffer2);
				remove_repeating_chars(s_buffer3, 0x2F);
				g_free(c_nb);
				g_free(c_nd);
			}

			if ((d_ptr->status != 1 && dir_exists(s_buffer))
					|| (d_ptr->status == 1 && dir_exists(s_buffer3))) {
				print_str(
						"WARNING: %s: listed in dirlog but does not exist on filesystem\n",
						s_buffer);
				if (gfl & F_OPT_FIX) {
					if (!md_unlink(&g_act_1.buffer, g_act_1.buffer.pos)) {
						print_str("ERROR: %s: unlinking ghost record failed\n",
								s_buffer);
					}
					r++;
				}
				continue;
			}

			if (gfl & F_OPT_C_GHOSTONLY) {
				continue;
			}

			struct nukelog n_buffer;
			ir = r;
			if (d_ptr->status == 1 || d_ptr->status == 2) {
				if (nukelog_find(d_ptr->dirname, 2, &n_buffer) == MAX_uint64_t) {
					print_str(
							"WARNING: %s: was marked as '%sNUKED' in dirlog but not found in nukelog\n",
							s_buffer, d_ptr->status == 2 ? "UN" : "");
				} else {
					if ((d_ptr->status == 1 && n_buffer.status != 0)
							|| (d_ptr->status == 2 && n_buffer.status != 1)
							|| (d_ptr->status == 0)) {
						print_str(
								"WARNING: %s: MISMATCH: was marked as '%sNUKED' in dirlog, but nukelog reads '%sNUKED'\n",
								s_buffer, d_ptr->status == 2 ? "UN" : "",
								n_buffer.status == 1 ? "UN" : "");
					}
				}
				continue;
			}
			buffer3.flags |= F_EAR_NOVERB;

			if ((r2 = release_generate_block(s_buffer, &buffer3))) {
				if (r2 == 5) {
					if ((gfl & F_OPT_FIX) && (gfl & F_OPT_FORCE)) {
						if (remove(s_buffer)) {
							print_str(
									"WARNING: %s: failed removing empty directory\n",
									s_buffer);

						} else {
							if (gfl & F_OPT_VERBOSE) {
								print_str("FIX: %s: removed empty directory\n",
										s_buffer);
							}
						}
					}
				} else {
					print_str(
							"WARNING: [%s] - could not get directory information from the filesystem\n",
							s_buffer);
				}
				r++;
				continue;
			}
			if (d_ptr->files != buffer4.files) {
				print_str(
						"WARNING: [%s] file counts in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->files, buffer4.files);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->files = buffer4.files;
				}
			}

			if (d_ptr->bytes != buffer4.bytes) {
				print_str(
						"WARNING: [%s] directory sizes in dirlog and on disk do not match ( dirlog: %llu , filesystem: %llu )\n",
						d_ptr->dirname, (ulint64_t) d_ptr->bytes,
						(ulint64_t) buffer4.bytes);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->bytes = buffer4.bytes;
				}
			}

			if (d_ptr->group != buffer4.group) {
				print_str(
						"WARNING: [%s] group ids in dirlog and on disk do not match (dirlog:%hu filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->group, buffer4.group);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->group = buffer4.group;
				}
			}

			if (d_ptr->uploader != buffer4.uploader) {
				print_str(
						"WARNING: [%s] user ids in dirlog and on disk do not match (dirlog:%hu, filesystem:%hu)\n",
						d_ptr->dirname, d_ptr->uploader, buffer4.uploader);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uploader = buffer4.uploader;
				}
			}

			if ((gfl & F_OPT_FORCE) && d_ptr->uptime != buffer4.uptime) {
				print_str(
						"WARNING: [%s] folder creation dates in dirlog and on disk do not match (dirlog:%u, filesystem:%u)\n",
						d_ptr->dirname, d_ptr->uptime, buffer4.uptime);
				r++;
				if (gfl & F_OPT_FIX) {
					d_ptr->uptime = buffer4.uptime;
				}
			}
			if (r == ir) {
				if (gfl & F_OPT_VERBOSE2) {
					print_str("OK: %s\n", d_ptr->dirname);
				}
			} else {
				if (gfl & F_OPT_VERBOSE2) {
					print_str("BAD: %s\n", d_ptr->dirname);
				}

			}
		}

	}

	if (!(gfl & F_OPT_KILL_GLOBAL) && (gfl & F_OPT_FIX) && r) {
		if (rebuild_data_file(DIRLOG, &g_act_1)) {
			print_str(MSG_GEN_DFRFAIL, DIRLOG);
		} else {
			if (g_act_1.bw || (gfl & F_OPT_VERBOSE4)) {
				print_str(MSG_GEN_WROTE, DIRLOG, (ulint64_t) g_act_1.bw,
						(ulint64_t) g_act_1.rw);
			}
		}
	}

	g_close(&g_act_1);

	return r;
}

int do_match(__g_handle hdl, void *d_ptr, __g_match _gm, void *callback) {
	char buffer[MAX_VAR_LEN], *mstr = NULL;

	buffer[0] = 0x0;

	if (_gm->field) {
		if (!hdl->g_proc1(d_ptr, _gm->field, buffer, MAX_VAR_LEN)) {
			mstr = buffer;
		} else {
			if (_gm->match != (char*) _gm->b_data) {
				_gm->match = (char*) _gm->b_data;
				if (regcomp(&_gm->preg, _gm->match, _gm->regex_flags)) {
					print_str(
							"ERROR: could not re-compile regex expression '%s', critical..\n",
							_gm->match);
					EXITVAL = 0;
					gfl |= F_OPT_KILL_GLOBAL;
					return 0;
				}
			}
			mstr = g_bmatch_get_def_mstr(d_ptr, hdl);
		}
	} else {
		mstr = g_bmatch_get_def_mstr(d_ptr, hdl);
	}

	if (!mstr) {
		return 0;
	}

	int r = 0;
	if ((_gm->flags & F_GM_ISMATCH)) {
		size_t mstr_l = strlen(mstr);

		int irl = strlen(_gm->match) != mstr_l, ir = strncmp(mstr, _gm->match,
				mstr_l);

		if ((_gm->match_i_m && (ir || irl))
				|| (!_gm->match_i_m && (!ir && !irl))) {
			if ((gfl & F_OPT_VERBOSE4)) {
				print_str("WARNING: %s: match positive\n", mstr);
			}
			r = 1;
		}
		goto end;
	}
	if ((_gm->flags & F_GM_ISREGEX)
			&& regexec(&_gm->preg, mstr, 0, NULL, 0) == _gm->reg_i_m) {
		if ((gfl & F_OPT_VERBOSE5)) {
			print_str("WARNING: %s: REGEX match positive\n", mstr);
		}
		r = 1;
	}

	end:

	return r;
}

char *g_bmatch_get_def_mstr(void *d_ptr, __g_handle hdl) {
	char *ptr = NULL;
	switch (hdl->flags & F_GH_ISTYPE) {
	case F_GH_ISDIRLOG:
		return (char*) ((struct dirlog*) d_ptr)->dirname;

	case F_GH_ISNUKELOG:
		return (char*) ((struct nukelog*) d_ptr)->dirname;

	case F_GH_ISDUPEFILE:
		return (char*) ((struct dupefile*) d_ptr)->filename;

	case F_GH_ISLASTONLOG:
		return (char*) ((struct lastonlog*) d_ptr)->uname;

	case F_GH_ISONELINERS:
		return (char*) ((struct oneliner*) d_ptr)->uname;

	case F_GH_ISONLINE:
		ptr = (char*) ((struct ONLINE*) d_ptr)->username;
		if (!strlen(ptr)) {
			return NULL;
		}
		break;
	case F_GH_ISIMDB:
		return (char*) ((__d_imdb) d_ptr)->dirname;

		case F_GH_ISGAME:
		return (char*) ((__d_game) d_ptr)->dirname;
		case F_GH_ISTVRAGE:
		return (char*) ((__d_tvrage) d_ptr)->dirname;

		case F_GH_ISFSX:
		return (char*) d_ptr;

	}
	return ptr;
}

int g_lom_var(void *d_ptr, __g_lom lom) {

	uint64_t l_val, r_val;
	float l_val_f, r_val_f;
	switch (lom->flags & F_LOM_TYPES) {
	case F_LOM_INT:
		if (lom->flags & F_LOM_LVAR_KNOWN) {
			l_val = lom->t_left;
		} else {
			if (!lom->g_t_ptr_left) {
				l_val = lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
			} else {
				l_val = lom->g_t_ptr_left(d_ptr, lom->t_l_off);
			}

		}
		if (lom->flags & F_LOM_RVAR_KNOWN) {
			r_val = lom->t_right;
		} else {
			if (!lom->g_t_ptr_right) {
				r_val = lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
			} else {
				r_val = lom->g_t_ptr_right(d_ptr, lom->t_r_off);
			}

		}

		lom->result = lom->g_icomp_ptr(l_val, r_val);

		break;
	case F_LOM_FLOAT:

		if (lom->flags & F_LOM_LVAR_KNOWN) {
			l_val_f = lom->tf_left;
		} else {
			if (!lom->g_tf_ptr_left) {
				l_val_f = lom->g_t_ptr_left(d_ptr, lom->t_l_off);
			} else {
				l_val_f = lom->g_tf_ptr_left(d_ptr, lom->t_l_off);
			}
		}
		if (lom->flags & F_LOM_RVAR_KNOWN) {
			r_val_f = lom->tf_right;
		} else {
			if (!lom->g_tf_ptr_right) {
				r_val_f = lom->g_t_ptr_right(d_ptr, lom->t_r_off);
			} else {
				r_val_f = lom->g_tf_ptr_right(d_ptr, lom->t_r_off);
			}
		}

		lom->result = lom->g_fcomp_ptr(l_val_f, r_val_f);

		break;
	default:
		return 1;
		break;
	}
	return 0;
}

int g_lom_match(__g_handle hdl, void *d_ptr, __g_match _gm) {
	p_md_obj ptr = md_first(&_gm->lom);
	__g_lom lom, p_lom = NULL;

	int r_p = 1, i = 0;

	while (ptr) {
		lom = (__g_lom) ptr->ptr;
		lom->result = 0;
		i++;
		g_lom_var(d_ptr, lom);

		if (!p_lom && lom->result && !ptr->next) {
			return !lom->result;
		}

		if ( p_lom && p_lom->g_oper_ptr ) {
			if (!(r_p = p_lom->g_oper_ptr(r_p, lom->result))) {
				if ( !ptr->next) {
					return 1;
				}
			}
			else {
				if (!ptr->next) {
					return !r_p;
				}
			}
		}
		else {
			r_p = lom->result;
		}

		p_lom = lom;
		ptr = ptr->next;
	}

	return 1;
}

int eval_cmatch(__g_match _gm, __g_match _p_gm, int r) {
	if (_p_gm && _p_gm->g_oper_ptr) {

		return 1;
	}
	return 0;
}

int g_bmatch(void *d_ptr, __g_handle hdl, pmda md) {
	if (md) {
		if (hdl->max_results && md->rescnt >= hdl->max_results) {
			return 1;
		}
		if (hdl->max_hits && md->hitcnt >= hdl->max_hits) {
			return 0;
		}
	}

	p_md_obj ptr = md_first(&hdl->_match_rr);
	int r, r_p = 0;
	__g_match _gm, _p_gm = NULL;

	while (ptr) {
		r = 0;
		_gm = (__g_match) ptr->ptr;

		if ((_gm->flags & F_GM_ISLOM)) {
			if ((g_lom_match(hdl, d_ptr, _gm)) == _gm->match_i_m) {
				r = 1;
				if ((gfl & F_OPT_VERBOSE5)) {
					print_str("WARNING: %s: LOM match positive\n", hdl->file);
				}
				goto l_end;
			}
		}

		if ((r = do_match(hdl, d_ptr, _gm, (void*) hdl->g_proc1))) {
			goto l_end;
		}

		l_end:

		if (_p_gm && _p_gm->g_oper_ptr) {
			r_p = _p_gm->g_oper_ptr(r_p, r);
		} else {
			r_p = r;
		}

		_p_gm = _gm;
		ptr = ptr->next;
	}

	if (!r_p) {
		int r_e;

		if (hdl->exec_args.exc
				&& WEXITSTATUS(r_e = hdl->exec_args.exc(d_ptr, (void*) hdl->g_proc1, NULL, (void*)hdl))) {
			if ((gfl & F_OPT_VERBOSE3)) {
				print_str("WARNING: external call returned non-zero: [%d]\n",
				WEXITSTATUS(r_e));
			}
			r_p = 1;
		}
	}

	if (md) {
		if (!r_p) {
			if ((gfl & F_OPT_IFIRSTRES) && !md->rescnt) {
				r_p = 1;
			}
			md->rescnt++;
		} else {
			if ((gfl & F_OPT_IFIRSTHIT) && !md->hitcnt) {
				r_p = 0;
			}
			md->hitcnt++;
		}
	}

	if (((gfl & F_OPT_MATCHQ) && r_p) || ((gfl & F_OPT_IMATCHQ) && !r_p)) {
		EXITVAL = 1;
		gfl |= F_OPT_KILL_GLOBAL;
	}

	return r_p;
}

int do_sort(__g_handle hdl, char *field, uint32_t flags) {
	if (!(gfl & F_OPT_SORT)) {
		return 0;
	}

	if (!field) {
		print_str("ERROR: %s: sorting requested but no field was set\n",
				hdl->file);
		return 1;
	}

	if ((gfl & F_OPT_VERBOSE)) {
		print_str("NOTICE: %s: sorting %llu records..\n", hdl->file,
				(uint64_t) hdl->buffer.offset);
	}

	int r = g_sort(hdl, field, flags);

	if (r) {
		print_str("ERROR: %s: [%d]: could not sort data\n", hdl->file, r);
	} else {
		if (gfl & F_OPT_VERBOSE4) {
			print_str("NOTICE: %s: sorting done\n", hdl->file);
		}
	}

	return r;
}

int g_filter(__g_handle hdl, pmda md) {
	g_setjmp(0, "g_filter", NULL, NULL);
	if (!((hdl->exec_args.exc || (hdl->flags & F_GH_ISMP))) || !md->count) {
		return 0;
	}

	if (g_proc_mr(hdl)) {
		return -1;
	}

	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: passing %llu records through filters..\n",
				hdl->file, (uint64_t) hdl->buffer.offset);
	}

	off_t s_offset = md->offset;

	p_md_obj ptr = NULL, o_ptr;

	if (hdl->j_offset == 2) {
		ptr = md_last(md);
	} else {
		ptr = md_first(md);
		hdl->j_offset = 1;
	}

	int r = 0;

	while (ptr) {
		if (gfl & F_OPT_KILL_GLOBAL) {
			break;
		}
		if (g_bmatch(ptr->ptr, hdl, md)) {
			o_ptr = ptr;
			ptr = *((void**) ptr + hdl->j_offset);
			if (!(md_unlink(md, o_ptr))) {
				r = 2;
				break;
			}
			continue;
		}
		ptr = *((void**) ptr + hdl->j_offset);
	}

	if (gfl & F_OPT_VERBOSE3) {
		print_str("NOTICE: %s: filtered %llu records..\n", hdl->file,
				(uint64_t) (s_offset - md->offset));
	}

	if (s_offset != md->offset) {
		hdl->flags |= F_GH_APFILT;
	}

	if (!md->offset) {
		return 1;
	}

	return r;
}

#define MAX_G_PRINT_STATS_BUFFER	8192

#define F_GPS_NONUKELOG				0x1

int g_print_stats(char *file, uint32_t flags, size_t block_sz) {
	g_setjmp(0, "g_print_stats", NULL, NULL);

	if (block_sz) {
		g_act_1.block_sz = block_sz;
	}

	if (g_fopen(file, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return 0;
	}

	if (gfl & F_OPT_LOADQ) {
		goto rc_end;
	}

	void *buffer = calloc(1, g_act_1.block_sz);

	int r;
	uint32_t i_flags = 0;

	if (gfl & F_OPT_SORT) {
		if ((gfl & F_OPT_NOBUFFER)) {
			print_str("ERROR: %s: unable to sort with buffering disabled\n",
					g_act_1.file);
			goto r_end;
		}

		void *s_exec = (void*) g_act_1.exec_args.exc;

		g_act_1.exec_args.exc = NULL;

		r = g_filter(&g_act_1, &g_act_1.buffer);

		g_act_1.exec_args.exc = (__d_exec) s_exec;

		if (gfl & F_OPT_KILL_GLOBAL) {
			goto r_end;
		}

		if (r == 1) {
			print_str("WARNING: %s: all records were filtered\n", g_act_1.file);
			goto r_end;
		} else if (r) {
			print_str("ERROR: %s: [%d]: filtering failed\n", g_act_1.file, r);
			goto r_end;
		}

		if (do_sort(&g_act_1, g_sort_field, g_sort_flags)) {
			goto r_end;
		}

		if (gfl & F_OPT_KILL_GLOBAL) {
			goto r_end;
		}

		if (g_act_1.j_offset == 2) {
			g_act_1.buffer.r_pos = md_last(&g_act_1.buffer);
		} else {
			g_act_1.buffer.r_pos = md_first(&g_act_1.buffer);
		}

		p_md_obj lm_ptr = md_first(&g_act_1._match_rr), lm_s_ptr;

		while (lm_ptr) {
			__g_match gm_ptr = (__g_match ) lm_ptr->ptr;
			if (gm_ptr->flags & F_GM_TYPES) {
				lm_s_ptr = lm_ptr;
				lm_ptr = lm_ptr->next;
				md_unlink(&g_act_1._match_rr, lm_s_ptr);
			} else {
				lm_ptr = lm_ptr->next;
			}
		}

	}

	void *ptr;

	char sbuffer[MAX_G_PRINT_STATS_BUFFER + 8], *ns_ptr;
	off_t c = 0;
	int re_c;

	g_setjmp(0, "g_print_stats(loop)", NULL, NULL);

	g_act_1.buffer.offset = 0;

	while ((ptr = g_read(buffer, &g_act_1, g_act_1.block_sz))) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "g_print_stats(loop)", NULL,
			NULL);

			if (gfl & F_OPT_KILL_GLOBAL) {
				break;
			}

			if ((r = g_bmatch(ptr, &g_act_1, &g_act_1.buffer))) {
				if (r == -1) {
					print_str("ERROR: %s: [%d] matching record failed\n",
							g_act_1.file, r);
					break;
				}
				continue;
			}

			c++;
			if (gfl & F_OPT_MODE_RAWDUMP) {
				g_fwrite((void*) ptr, g_act_1.block_sz, 1, stdout);
			} else {

				if (g_act_1.flags & F_GH_ISONLINE) {

					if (!(re_c = g_act_1.g_proc3(ptr, sbuffer))) {
						goto end;
					}
					if (c == 1 && (gfl & F_OPT_FORMAT_COMP)) {
						print_str(
								"+-------------------------------------------------------------------------------------------------------------------------------------------\n"
										"|                     USER/HOST/PID                       |    TIME ONLINE     |    TRANSFER RATE      |        STATUS       \n"
										"|---------------------------------------------------------|--------------------|-----------------------|------------------------------------\n");

					}
				} else {
					re_c = g_act_1.g_proc3(ptr, sbuffer);
				}

				if (re_c) {
					print_str("%s", sbuffer);
				} else {
					print_str("ERROR: %s: zero-length formatted result\n",
							g_act_1.file);
					continue;
				}
				if (!(i_flags & F_GPS_NONUKELOG)
						&& (g_act_1.flags & F_GH_ISDIRLOG)) {
					if ((gfl & F_OPT_VERBOSE2)) {
						ns_ptr = ((struct dirlog*) ptr)->dirname;
						struct nukelog n_buffer = { 0 };
						if (nukelog_find(ns_ptr, 2, &n_buffer) < MAX_uint64_t) {
							if (nukelog_format_block(&n_buffer, sbuffer) > 0) {
								print_str(sbuffer);
							}
							goto end_loop;
						} else {
							i_flags |= F_GPS_NONUKELOG;
						}
					}
				}

			}
		}
		end_loop: ;
	}

	end:

	if (gfl & F_OPT_MODE_RAWDUMP) {
		fflush(stdout);
	}

	g_setjmp(0, "dirlog_print_stats(2)", NULL, NULL);

	if (!(gfl & F_OPT_FORMAT_BATCH) && !(gfl & F_OPT_FORMAT_COMP)
			&& !(g_act_1.flags & F_GH_ISONLINE)) {
		print_str("STATS: %s: read %llu/%llu records\n", file,
				(unsigned long long int) c,
				!g_act_1.buffer.count ? c : g_act_1.buffer.count);
	}

	r_end:

	g_free(buffer);

	rc_end:

	g_close(&g_act_1);

	return EXITVAL;
}

#define 	ACT_WRITE_BUFFER_MEMBERS	10000

int rebuild_dirlog(void) {
	g_setjmp(0, "rebuild_dirlog", NULL, NULL);
	char mode[255] = { 0 };
	uint32_t flags = 0;
	int rt = 0;

	if (!(ofl & F_OVRR_NUKESTR)) {
		print_str(
				"WARNING: failed extracting nuke string from glftpd.conf, nuked dirs might not get detected properly\n");
	}

	if (gfl & F_OPT_NOWRITE) {
		g_strncpy(mode, "r", 1);
		flags |= F_DL_FOPEN_BUFFER;
	} else if (gfl & F_OPT_UPDATE) {
		g_strncpy(mode, "a+", 2);
		flags |= F_DL_FOPEN_BUFFER | F_DL_FOPEN_FILE;
	} else {
		g_strncpy(mode, "w+", 2);
	}

	if (gfl & F_OPT_WBUFFER) {
		g_strncpy(g_act_1.mode, "r", 1);
		if (gfl & F_OPT_VERBOSE3) {
			print_str(
					"NOTICE: %s: allocating %u bytes for references (overhead)\n",
					DIRLOG,
					(uint32_t) (ACT_WRITE_BUFFER_MEMBERS * sizeof(md_obj)));
		}
		md_init(&g_act_1.w_buffer, ACT_WRITE_BUFFER_MEMBERS);
		g_act_1.block_sz = DL_SZ;
		g_act_1.flags |= F_GH_FFBUFFER | F_GH_WAPPEND
				| ((gfl & F_OPT_UPDATE) ? F_GH_DFNOWIPE : 0);
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s: explicit write pre-caching enabled\n",
					DIRLOG);
		}
	} else {
		g_strncpy(g_act_1.mode, mode, strlen(mode));
		data_backup_records(DIRLOG);
	}
	g_act_1.block_sz = DL_SZ;
	g_act_1.flags |= F_GH_ISDIRLOG;

	int dfex = file_exists(DIRLOG);

	if ((gfl & F_OPT_UPDATE) && dfex) {
		print_str(
				"WARNING: %s: requested update, but no dirlog exists - removing update flag..\n",
				DIRLOG);
		gfl ^= F_OPT_UPDATE;
		flags ^= F_DL_FOPEN_BUFFER;
	}

	if (!strncmp(g_act_1.mode, "r", 1) && dfex) {
		if (gfl & F_OPT_VERBOSE) {
			print_str(
					"WARNING: %s: requested read mode access but file not there\n",
					DIRLOG);
		}
	} else if (g_fopen(DIRLOG, g_act_1.mode, flags, &g_act_1)) {
		print_str("ERROR: could not open dirlog, mode '%s', flags %u\n",
				g_act_1.mode, flags);
		return errno;
	}

	g_proc_mr(&g_act_1);

	if (gfl & F_OPT_FORCE) {
		print_str("SCANNING: '%s'\n", SITEROOT);
		update_records(SITEROOT, 0);
		goto rw_end;
	}

	char buffer[V_MB + 1] = { 0 };
	mda dirchain = { 0 }, buffer2 = { 0 };

	md_init(&dirchain, 1024);

	if (read_file(DU_FLD, buffer, V_MB, 0, NULL) < 1) {
		print_str(
				"WARNING: unable to read folders file, doing full siteroot recursion in '%s'..\n",
				SITEROOT);
		gfl |= F_OPT_FORCE;
		update_records(SITEROOT, 0);
		goto rw_end;
	}

	int r, r2;

	if ((r = split_string(buffer, 0x13, &dirchain)) < 1) {
		print_str("ERROR: [%d] could not parse input from %s\n", r, DU_FLD);
		rt = 5;
		goto r_end;
	}

	int i = 0, ib;
	char s_buffer[PATH_MAX] = { 0 };
	p_md_obj ptr = dirchain.objects;

	while (ptr) {
		if (!sigsetjmp(g_sigjmp.env, 1)) {
			g_setjmp(F_SIGERR_CONTINUE, "rebuild_dirlog(loop)", NULL,
			NULL);
			if (gfl & F_OPT_KILL_GLOBAL) {
				rt = EXITVAL;
				break;
			}

			md_init(&buffer2, 6);
			i++;
			if ((r2 = split_string((char*) ptr->ptr, 0x20, &buffer2)) != 2) {
				print_str("ERROR: [%d] could not parse line %d from %s\n", r2,
						i, DU_FLD);
				goto lend;
			}
			bzero(s_buffer, PATH_MAX);
			snprintf(s_buffer, PATH_MAX, "%s/%s", SITEROOT,
					(char*) buffer2.objects->ptr);
			remove_repeating_chars(s_buffer, 0x2F);

			size_t s_buf_len = strlen(s_buffer);
			if (s_buffer[s_buf_len - 1] == 0x2F) {
				s_buffer[s_buf_len - 1] = 0x0;
			}

			ib = strtol((char*) ((p_md_obj) buffer2.objects->next)->ptr,
			NULL, 10);

			if (errno == ERANGE) {
				print_str("ERROR: could not get depth from line %d\n", i);
				goto lend;
			}
			if (dir_exists(s_buffer)) {
				print_str("ERROR: %s: directory doesn't exist (line %d)\n",
						s_buffer, i);
				goto lend;
			}
			char *ndup = strdup(s_buffer);
			char *nbase = basename(ndup);

			print_str("SCANNING: '%s', depth: %d\n", nbase, ib);
			if (update_records(s_buffer, ib) < 1) {
				print_str("WARNING: %s: nothing was processed\n", nbase);
			}

			g_free(ndup);
			lend:

			md_g_free(&buffer2);
		}
		ptr = ptr->next;
	}

	rw_end:

	if (g_act_1.flags & F_GH_FFBUFFER) {
		if (rebuild_data_file(DIRLOG, &g_act_1)) {
			print_str(MSG_GEN_DFRFAIL, DIRLOG);
		}
	}

	r_end:

	md_g_free(&dirchain);

	g_close(&g_act_1);

	if (dl_stats.bw || (gfl & F_OPT_VERBOSE4)) {
		print_str(MSG_GEN_WROTE, DIRLOG, dl_stats.bw, dl_stats.rw);
	}

	return rt;
}

int parse_args(int argc, char **argv, void*fref_t[]) {
	g_setjmp(0, "parse_args", NULL, NULL);
	int i, oi, vi, ret, r, c = 0;

	char *c_arg;
	mda cmd_lt = { 0 };

	p_ora ora = (p_ora) fref_t;

	for (i = 1, ret = 0; i < argc; i++, r = 0) {
		c_arg = argv[i];
		bzero(&cmd_lt, sizeof(mda));
		md_init(&cmd_lt, 256);

		if ((r = split_string(c_arg, 0x3D, &cmd_lt)) == 2) {
			c_arg = (char*) cmd_lt.objects->ptr;
		}

		if ((vi = process_opt(c_arg, NULL, fref_t, 1)) < 0) {
			if (fref_t != prio_f_ref) {
				print_str("CMDLINE: [%d] invalid argument '%s'\n", vi, c_arg);
				ret = -2;
				goto end;
			}

		}

		if (r == 2) {
			ret |= process_opt(c_arg, ((p_md_obj) cmd_lt.objects->next)->ptr,
					fref_t, 2);

			c++;
		} else {
			oi = i;
			uintaa_t vp;
			void *buffer = NULL;

			if ((vp = (uintaa_t) ora[vi].arg_cnt)) {
				if (i + vp > argc - 1) {
					if (fref_t != prio_f_ref) {
						print_str(
								"CMDLINE: '%s' missing argument parameters [%llu]\n",
								argv[i], (ulint64_t) ((i + vp) - (argc - 1)));
						c = 0;
						goto end;
					}

				}
				buffer = &argv[i + 1];
				i += vp;
			}
			ret |= process_opt(argv[oi], buffer, fref_t, 0);

			c++;
		}
		md_g_free(&cmd_lt);

	}
	end:

	if (!c) {
		return -1;
	}

	return ret;
}

int process_opt(char *opt, void *arg, void *reference_array, int m) {
	int (*proc_opt_generic)(void *arg, int m);
	int i = 0;
	p_ora ora = (p_ora) reference_array;

	while (ora->option) {
		if (strlen(ora->option) == strlen(opt)
				&& !strncmp(ora->option, opt, strlen(ora->option))) {
			if (m == 1)
				return i;
			else {
				if (ora->function) {
					proc_opt_generic = ora->function;
					if (proc_opt_generic) {
						return proc_opt_generic(arg, m);
					}
				} else {
					return -4;
				}
			}
		}

		ora++;
		i++;
	}
	return -2;
}

int update_records(char *dirname, int depth) {
	g_setjmp(0, "update_records", NULL, NULL);
	struct dirlog buffer = { 0 };
	ear arg = { 0 };

	if (dir_exists(dirname))
		return 2;

	arg.depth = depth;
	arg.dirlog = &buffer;

	_g_eds eds = { 0 };

	return enum_dir(dirname, proc_section, &arg, 0, &eds);

}

int proc_release(char *name, unsigned char type, void *arg, __g_eds eds) {
	ear *iarg = (ear*) arg;
	uint32_t crc32 = 0;
	char buffer[PATH_MAX] = { 0 };
	char *fn, *fn2, *fn3, *base;

	if (!reg_match("\\/[.]{1,2}$", name, 0))
		return 1;

	if (!reg_match("\\/[.].*$", name, REG_NEWLINE))
		return 1;

	switch (type) {
	case DT_REG:
		fn2 = strdup(name);
		fn3 = strdup(name);
		base = basename(fn3);
		if ((gfl & F_OPT_SFV)
				&& (updmode == UPD_MODE_RECURSIVE || updmode == UPD_MODE_SINGLE)
				&& reg_match(PREG_SFV_SKIP_EXT, fn2,
				REG_ICASE | REG_NEWLINE) && file_crc32(name, &crc32)) {
			fn = strdup(name);
			char *dn = basename(dirname(fn));
			g_free(fn2);
			fn2 = strdup(name);
			snprintf(iarg->buffer, PATH_MAX, "%s/%s.sfv.tmp", dirname(fn2), dn);
			snprintf(iarg->buffer2, PATH_MAX, "%s/%s.sfv", dirname(fn2), dn);
			char buffer2[PATH_MAX + 10];
			snprintf(buffer2, 1024, "%s %.8X\n", base, (uint32_t) crc32);
			if (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV)) {
				if (!write_file_text(buffer2, iarg->buffer)) {
					print_str("ERROR: %s: failed writing to SFV file: '%s'\n",
							name, iarg->buffer);
				}
			}
			iarg->flags |= F_EARG_SFV;
			snprintf(buffer, PATH_MAX, "  %.8X", (uint32_t) crc32);
			g_free(fn);
		}
		off_t fs = get_file_size(name);
		iarg->dirlog->bytes += fs;
		iarg->dirlog->files++;
		if (gfl & F_OPT_VERBOSE4) {
			print_str("     %s  %.2fMB%s\n", base,
					(double) fs / 1024.0 / 1024.0, buffer);
		}
		g_free(fn3);
		g_free(fn2);
		break;
	case DT_DIR:
		if ((gfl & F_OPT_SFV)
				&& (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV))) {
			enum_dir(name, delete_file, (void*) "\\.sfv(\\.tmp|)$", 0, NULL);
		}

		enum_dir(name, proc_release, iarg, 0, eds);
		break;
	}

	return 0;
}

#define MSG_PS_UWSFV	"WARNING: %s: unable wiping temp. SFV file (remove it manually)\n"

int proc_section(char *name, unsigned char type, void *arg, __g_eds eds) {
	ear *iarg = (ear*) arg;
	int r;
	uint64_t rl;

	if (!reg_match("\\/[.]{1,2}", name, 0)) {
		return 1;
	}

	if (!reg_match("\\/[.].*$", name, REG_NEWLINE)) {
		return 1;
	}

	switch (type) {
	case DT_DIR:
		iarg->depth--;
		if (!iarg->depth || (gfl & F_OPT_FORCE)) {
			if (gfl & F_OPT_UPDATE) {
				if (((rl = dirlog_find(name, 1, F_DL_FOPEN_REWIND, NULL))
						< MAX_uint64_t)) {
					if (gfl & F_OPT_VERBOSE2) {
						print_str(
								"WARNING: %s: [%llu] record already exists, not importing\n",
								name, rl);
					}
					goto end;
				}
			}
			bzero(iarg->buffer, PATH_MAX);
			iarg->flags = 0;
			if ((r = release_generate_block(name, iarg))) {
				if (r < 5)
					print_str(
							"ERROR: %s: [%d] generating dirlog data chunk failed\n",
							name, r);
				goto end;
			}

			if (g_bmatch(iarg->dirlog, &g_act_1, NULL)) {
				if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV)) {
					if (remove(iarg->buffer)) {
						print_str( MSG_PS_UWSFV, iarg->buffer);
					}
				}
				goto end;
			}

			if (gfl & F_OPT_KILL_GLOBAL) {
				if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV)) {
					if (remove(iarg->buffer)) {
						print_str( MSG_PS_UWSFV, iarg->buffer);
					}
				}
				goto end;
			}

			if ((gfl & F_OPT_SFV) && (iarg->flags & F_EARG_SFV)) {
				iarg->dirlog->bytes += (uint64_t) get_file_size(iarg->buffer);
				iarg->dirlog->files++;
				if (!rename(iarg->buffer, iarg->buffer2)) {
					if (gfl & F_OPT_VERBOSE) {
						print_str(
								"NOTICE: '%s': succesfully generated SFV file\n",
								name);
					}
				} else {
					print_str("ERROR: '%s': failed renaming '%s' to '%s'\n",
					basename(iarg->buffer2), basename(iarg->buffer));
				}
			}

			if (g_act_1.flags & F_GH_FFBUFFER) {
				if ((r = g_load_record(&g_act_1, (const void*) iarg->dirlog))) {
					print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
							(ulint64_t) g_act_1.w_buffer.offset, "wbuffer");
				}
			} else {
				if ((r = dirlog_write_record(iarg->dirlog, 0, SEEK_END))) {
					print_str(MSG_GEN_DFWRITE, iarg->dirlog->dirname, r,
							(ulint64_t) g_act_1.offset - 1, "w");
					goto end;
				}
			}

			char buffer[2048] = { 0 };

			if ((gfl & F_OPT_VERBOSE)
					&& dirlog_format_block(iarg->dirlog, buffer) > 0) {
				print_str(buffer);
			}

			if (gfl & F_OPT_FORCE) {
				enum_dir(name, proc_section, iarg, 0, eds);
			}
		} else {
			enum_dir(name, proc_section, iarg, 0, eds);
		}
		end: iarg->depth++;
		break;
	}
	return 0;
}

int release_generate_block(char *name, ear *iarg) {
	bzero(iarg->dirlog, sizeof(struct dirlog));

	int r, ret = 0;
	struct stat st = { 0 }, st2 = { 0 };

	if (gfl & F_OPT_FOLLOW_LINKS) {
		if (stat(name, &st)) {
			return 1;
		}
	} else {
		if (lstat(name, &st)) {
			return 1;
		}

		if ((st.st_mode & S_IFMT) == S_IFLNK) {
			if (gfl & F_OPT_VERBOSE2) {
				print_str("WARNING: %s - is symbolic link, skipping..\n", name);
			}
			return 6;
		}
	}

	time_t orig_ctime = get_file_creation_time(&st);

	if ((gfl & F_OPT_SFV)
			&& (!(gfl & F_OPT_NOWRITE) || (gfl & F_OPT_FORCEWSFV))) {
		enum_dir(name, delete_file, (void*) "\\.sfv(\\.tmp|)$", 0, NULL);
	}

	if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
		print_str("ENTERING: %s\n", name);

	_g_eds eds = { 0 };

	if ((r = enum_dir(name, proc_release, iarg, 0, &eds)) < 1
			|| !iarg->dirlog->files) {
		if (gfl & F_OPT_VERBOSE) {
			print_str("WARNING: %s: [%d] - empty directory\n", name, r);
		}
		ret = 5;
	}
	g_setjmp(0, "release_generate_block(2)", NULL, NULL);

	if ((gfl & F_OPT_VERBOSE2) && !(iarg->flags & F_EAR_NOVERB))
		print_str("EXITING: %s\n", name);

	if ((gfl & F_OPT_SFV) && !(gfl & F_OPT_NOWRITE)) {
		if (gfl & F_OPT_FOLLOW_LINKS) {
			if (stat(name, &st2)) {
				ret = 1;
			}
		} else {
			if (lstat(name, &st2)) {
				ret = 1;
			}
		}
		time_t c_ctime = get_file_creation_time(&st2);
		if (c_ctime != orig_ctime) {
			if (gfl & F_OPT_VERBOSE4) {
				print_str(
						"NOTICE: %s: restoring original folder modification date\n",
						name);
			}
			struct utimbuf utb;
			utb.actime = 0;
			utb.modtime = orig_ctime;
			if (utime(name, &utb)) {
				print_str(
						"WARNING: %s: SFVGEN failed to restore original modification date\n",
						name);
			}
		}

	}

	if (ret) {
		goto end;
	}

	char *namedup = strdup(name);
	char *bn = basename(namedup);

	iarg->dirlog->uptime = orig_ctime;
	iarg->dirlog->uploader = (uint16_t) st.st_uid;
	iarg->dirlog->group = (uint16_t) st.st_gid;
	char buffer[PATH_MAX] = { 0 };

	if ((r = get_relative_path(name, GLROOT, buffer))) {
		print_str("ERROR: [%s] could not get relative to root directory name\n",
				bn);
		ret = 2;
		goto r_end;
	}

	struct nukelog n_buffer = { 0 };
	if (nukelog_find(buffer, 2, &n_buffer) < MAX_uint64_t) {
		iarg->dirlog->status = n_buffer.status + 1;
		g_strncpy(iarg->dirlog->dirname, n_buffer.dirname,
				strlen(n_buffer.dirname));
	} else {
		g_strncpy(iarg->dirlog->dirname, buffer, strlen(buffer));
	}

	r_end:

	g_free(namedup);

	end:

	return ret;
}

int get_relative_path(char *subject, char *root, char *output) {
	char *root_dir = root;

	if (!root_dir)
		return 11;

	int i, root_dir_len = strlen(root_dir);

	for (i = 0; i < root_dir_len; i++) {
		if (subject[i] != root_dir[i])
			break;
	}

	while (subject[i] != 0x2F && i > 0)
		i--;

	snprintf(output, PATH_MAX, "%s", &subject[i]);
	return 0;
}

#define STD_FMT_TIME_STR  	"%d %b %Y %T"

int dirlog_format_block(void *iarg, char *output) {
	char buffer2[255];

	struct dirlog *data = (struct dirlog *) iarg;

	char *ndup = strdup(data->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE) {
		base = data->dirname;
	} else {
		base = basename(ndup);
	}

	time_t t_t = (time_t) data->uptime;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;

	if (gfl & F_OPT_FORMAT_BATCH) {

		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"DIRLOG\x9%s\x9%llu\x9%hu\x9%u\x9%hu\x9%hu\x9%hu\n",
				data->dirname, (ulint64_t) data->bytes, data->files,
				(uint32_t) data->uptime, data->uploader, data->group,
				data->status);
	} else {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"DIRLOG: %s - %llu Mbytes in %hu files - created %s by %hu.%hu [%hu]\n",
						base, (ulint64_t) (data->bytes / 1024 / 1024),
						data->files, buffer2, data->uploader, data->group,
						data->status);
	}

	g_free(ndup);

	return c;
}

int nukelog_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	struct nukelog *data = (struct nukelog *) iarg;

	char *ndup = strdup(data->dirname), *base = NULL;

	if (gfl & F_OPT_VERBOSE)
		base = data->dirname;
	else
		base = basename(ndup);

	time_t t_t = (time_t) data->nuketime;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"NUKELOG\x9%s\x9%s\x9%hu\x9%.2f\x9%s\x9%s\x9%u\x9%u\n",
				data->dirname, data->reason, data->mult, data->bytes,
				!data->status ? data->nuker : data->unnuker, data->nukee,
				(uint32_t) data->nuketime, data->status);
	} else {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"NUKELOG: %s - %s, reason: '%s' [%.2f MB] - factor: %hu, %s: %s, nukee: %s - %s\n",
						base, !data->status ? "NUKED" : "UNNUKED", data->reason,
						data->bytes, data->mult,
						!data->status ? "nuker" : "unnuker",
						!data->status ? data->nuker : data->unnuker,
						data->nukee, buffer2);
	}
	g_free(ndup);

	return c;
}

int dupefile_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	struct dupefile *data = (struct dupefile *) iarg;

	time_t t_t = (time_t) data->timeup;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"DUPEFILE\x9%s\x9%s\x9%u\n", data->filename, data->uploader,
				(uint32_t) data->timeup);
	} else {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"DUPEFILE: %s - uploader: %s, time: %s\n", data->filename,
				data->uploader, buffer2);
	}

	return c;
}

int lastonlog_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 }, buffer3[255] = { 0 }, buffer4[12] = { 0 };

	struct lastonlog *data = (struct lastonlog *) iarg;

	time_t t_t_ln = (time_t) data->logon;
	time_t t_t_lf = (time_t) data->logoff;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t_ln));
	strftime(buffer3, 255, STD_FMT_TIME_STR, localtime(&t_t_lf));

	g_memcpy(buffer4, data->stats, sizeof(data->stats));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"LASTONLOG\x9%s\x9%s\x9%s\x9%u\x9%u\x9%u\x9%u\x9%s\n",
				data->uname, data->gname, data->tagline, (uint32_t) data->logon,
				(uint32_t) data->logoff, (uint32_t) data->upload,
				(uint32_t) data->download, buffer4);
	} else {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"LASTONLOG: user: %s/%s [%s] - logon: %s, logoff: %s - up/down: %u/%u B, changes: %s\n",
						data->uname, data->gname, data->tagline, buffer2,
						buffer3, (uint32_t) data->upload,
						(uint32_t) data->download, buffer4);
	}

	return c;
}

int oneliner_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	struct oneliner *data = (struct oneliner *) iarg;

	time_t t_t = (time_t) data->timestamp;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"ONELINER\x9%s\x9%s\x9%s\x9%u\x9%s\n", data->uname, data->gname,
				data->tagline, (uint32_t) data->timestamp, data->message);
	} else {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"ONELINER: user: %s/%s [%s] - time: %s, message: %s\n",
				data->uname, data->gname, data->tagline, buffer2,
				data->message);
	}

	return c;
}

#define FMT_SP_OFF 	30

int online_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	struct ONLINE *data = (struct ONLINE *) iarg;

	time_t t_t = (time_t) data->login_time;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	if (!strlen(data->username)) {
		return 0;
	}

	int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
	float kb = (data->bytes_xfer / 1024), kbps = 0.0;

	if (tdiff > 0 && kb > 0) {
		kbps = kb / (float) tdiff;
	}

	time_t ltime = time(NULL) - (time_t) data->login_time;

	if (ltime < 0) {
		ltime = 0;
	}

	int c = 0;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"ONLINE\x9%s\x9%s\x9%u\x9%u\x9%s\x9%u\x9%u\x9%llu\x9%llu\x9%llu\x9%s\x9%s\n",
						data->username, data->host, (uint32_t) data->groupid,
						(uint32_t) data->login_time, data->tagline,
						(uint32_t) data->ssl_flag, (uint32_t) data->procid,
						(ulint64_t) data->bytes_xfer,
						(ulint64_t) data->bytes_txfer, (ulint64_t) kbps,
						data->status, data->currentdir);
	} else {
		if (gfl & F_OPT_FORMAT_COMP) {
			char sp_buffer[255], sp_buffer2[255], sp_buffer3[255];
			char d_buffer[255] = { 0 };

			snprintf(d_buffer, 254, "%u", (uint32_t) ltime);
			size_t d_len1 = strlen(d_buffer);
			snprintf(d_buffer, 254, "%.2f", kbps);
			size_t d_len2 = strlen(d_buffer);
			snprintf(d_buffer, 254, "%u", data->procid);
			size_t d_len3 = strlen(d_buffer);
			generate_chars(
					54
							- (strlen(data->username) + strlen(data->host)
									+ d_len3 + 1), 0x20, sp_buffer);
			generate_chars(10 - d_len1, 0x20, sp_buffer2);
			generate_chars(13 - d_len2, 0x20, sp_buffer3);
			c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
					"| %s!%s/%u%s |        %us%s |     %.2fKB/s%s |  %s\n",
					data->username, data->host, data->procid, sp_buffer,
					(uint32_t) ltime, sp_buffer2, kbps, sp_buffer3,
					data->status);
		} else {
			c = snprintf(output, MAX_G_PRINT_STATS_BUFFER, "[ONLINE]\n"
					"    User:            %s\n"
					"    Host:            %s\n"
					"    GID:             %u\n"
					"    Login:           %s (%us)\n"
					"    Tag:             %s\n"
					"    SSL:             %s\n"
					"    PID:             %u\n"
					"    XFER:            %lld Bytes\n"
					"    Rate:            %.2f KB/s\n"
					"    Status:          %s\n"
					"    CWD:             %s\n\n", data->username, data->host,
					(uint32_t) data->groupid, buffer2, (uint32_t) ltime,
					data->tagline,
					(!data->ssl_flag ?
							"NO" :
							(data->ssl_flag == 1 ?
									"YES" :
									(data->ssl_flag == 2 ?
											"YES (DATA)" : "UNKNOWN"))),
					(uint32_t) data->procid, (ulint64_t) data->bytes_xfer, kbps,
					data->status, data->currentdir);
		}
	}

	return c;
}

#define STD_FMT_DATE_STR  	"%d %b %Y"

int imdb_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 }, buffer3[255] = { 0 };

	__d_imdb data = (__d_imdb) iarg;

	time_t t_t = (time_t) data->timestamp, t_t2 = (time_t) data->released;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));
	strftime(buffer3, 255, STD_FMT_DATE_STR, localtime(&t_t2));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"IMDB\x9%s\x9%s\x9%u\x9%s\x9%.1f\x9%u\x9%s\x9%s\x9%u\x9%u\x9%s\x9%s\x9%s\n",
						data->dirname, data->title, (uint32_t) data->timestamp,
						data->imdb_id, data->rating, data->votes, data->genres,
						data->year, data->released, data->runtime, data->rated,
						data->actors, data->director);
	} else {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"IMDB: %s: %s (%s): created: %s - iMDB ID: %s - rating: %.1f - votes: %u - genres: %s - released: %s - runtime: %u min - rated: %s - actors: %s - director: %s\n",
						data->dirname, data->title, data->year, buffer2,
						data->imdb_id, data->rating, data->votes, data->genres,
						buffer3, data->runtime, data->rated, data->actors,
						data->director);
	}

	return c;
}

int game_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	__d_game data = (__d_game) iarg;

	time_t t_t = (time_t) data->timestamp;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"GAMELOG\x9%s\x9%u\x9%.1f\n", data->dirname, data->timestamp,
				data->rating);
	} else {
		c = snprintf(output, MAX_G_PRINT_STATS_BUFFER,
				"GAMELOG: %s: created: %s - rating: %.1f\n", data->dirname,
				buffer2, data->rating);
	}

	return c;
}

int tv_format_block(void *iarg, char *output) {
	char buffer2[255] = { 0 };

	__d_tvrage data = (__d_tvrage) iarg;

	time_t t_t = (time_t) data->timestamp;

	strftime(buffer2, 255, STD_FMT_TIME_STR, localtime(&t_t));

	int c;
	if (gfl & F_OPT_FORMAT_BATCH) {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"TVRAGE\x9%s\x9%u\x9%s\x9%s\x9%u\x9%s\x9%s\x9%s\x9%s\x9%u\x9%u\x9%u\n",
						data->dirname, data->timestamp, data->name, data->class,
						data->showid, data->link, data->status, data->airday,
						data->airtime, data->runtime, data->started,
						data->ended);
	} else {
		c =
				snprintf(output, MAX_G_PRINT_STATS_BUFFER,
						"TVRAGE: %s: created: %s - %s/%s [%u] - runtime: %u min - status: %s - country: %s - genres: %s\n",
						data->dirname, buffer2, data->name, data->class,
						data->showid, data->runtime, data->status,
						data->country, data->genres);
	}

	return c;
}

int gen1_format_block(void *iarg, char *output) {
	__d_generic_s2044 data = (__d_generic_s2044) iarg;

	return snprintf(output, MAX_G_PRINT_STATS_BUFFER,
			"GENERIC1\x9%u\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\x9%s\n",
			data->i32, data->s_1, data->s_2, data->s_3, data->s_4,
			data->s_5, data->s_6, data->s_7, data->s_8);

}

char *generate_chars(size_t num, char chr, char*buffer) {
	g_setjmp(0, "generate_chars", NULL, NULL);
	bzero(buffer, 255);
	if (num < 1 || num > 254) {
		return buffer;
	}
	memset(buffer, (int) chr, num);

	return buffer;
}

off_t get_file_size(char *file) {
	struct stat st;

	if (stat(file, &st) == -1)
		return 0;

	return st.st_size;
}

#ifdef HAVE_ST_BIRTHTIME
#define birthtime(x) x->st_birthtime
#else
#define birthtime(x) x->st_ctime
#endif

time_t get_file_creation_time(struct stat *st) {
	return (time_t) birthtime(st);
}

uint64_t dirlog_find(char *dirn, int mode, uint32_t flags, void *callback) {
	if (!(ofl & F_OVRR_NUKESTR)) {
		return dirlog_find_old(dirn, mode, flags, callback);
	}

	struct dirlog buffer;
	int (*callback_f)(struct dirlog *) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
		return MAX_uint64_t;

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[512] = { 0 }, buffer_s2[512], buffer_s3[512];
	char *dup, *dup2, *base, *dir;

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	size_t d_l = strlen(buffer_s);

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		if (!strncmp(buffer_s, d_ptr->dirname, d_l)) {
			goto match;
		}
		dup = strdup(d_ptr->dirname);
		base = basename(dup);
		dup2 = strdup(d_ptr->dirname);
		dir = dirname(dup2);
		snprintf(buffer_s2, 512, NUKESTR, base);
		snprintf(buffer_s3, 512, "%s/%s", dir, buffer_s2);
		remove_repeating_chars(buffer_s3, 0x2F);
		g_free(dup);
		g_free(dup2);
		if (!strncmp(buffer_s3, buffer_s, d_l)) {
			match: ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					break;
				}

			} else {
				break;
			}
		}
	}

	if (mode != 1) {
		g_close(&g_act_1);
	}

	return ur;
}

uint64_t dirlog_find_old(char *dirn, int mode, uint32_t flags, void *callback) {
	struct dirlog buffer;
	int (*callback_f)(struct dirlog *data) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1))
		return MAX_uint64_t;

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[255] = { 0 };
	char *dup, *dup2, *base, *dir;
	int gi1, gi2;

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	gi2 = strlen(buffer_s);

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		dup = strdup(d_ptr->dirname);
		base = basename(dup);
		gi1 = strlen(base);
		dup2 = strdup(d_ptr->dirname);
		dir = dirname(dup2);
		if (!strncmp(&buffer_s[gi2 - gi1], base, gi1)
				&& !strncmp(buffer_s, d_ptr->dirname, strlen(dir))) {

			ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					g_free(dup);
					g_free(dup2);
					break;
				}

			} else {
				g_free(dup);
				g_free(dup2);
				break;
			}
		}
		g_free(dup);
		g_free(dup2);
	}

	if (mode != 1) {
		g_close(&g_act_1);
	}

	return ur;
}

size_t str_match(char *input, char *match) {
	size_t i_l = strlen(input), m_l = strlen(match);

	size_t i;

	for (i = 0; i < i_l - m_l + 1; i++) {
		if (!strncmp(&input[i], match, m_l)) {
			return i;
		}
	}

	return -1;
}

char *string_replace(char *input, char *match, char *with, char *output,
		size_t max_out) {
	size_t i_l = strlen(input), w_l = strlen(with), m_l = strlen(match);

	size_t m_off = str_match(input, match);

	if ((int) m_off < 0) {
		return output;
	}

	bzero(output, max_out);

	g_strncpy(output, input, m_off);
	g_strncpy(&output[m_off], with, w_l);
	g_strncpy(&output[m_off + w_l], &input[m_off + m_l], i_l - m_off - m_l);

	return output;
}

uint64_t dirlog_find_simple(char *dirn, int mode, uint32_t flags,
		void *callback) {
	struct dirlog buffer;
	int (*callback_f)(struct dirlog *data) = callback;

	if (g_fopen(DIRLOG, "r", F_DL_FOPEN_BUFFER | flags, &g_act_1)) {
		return MAX_uint64_t;
	}

	int r;
	uint64_t ur = MAX_uint64_t;

	char buffer_s[255] = { 0 };

	if ((r = get_relative_path(dirn, GLROOT, buffer_s)))
		g_strncpy(buffer_s, dirn, strlen(dirn));

	size_t s_blen = strlen(buffer_s), d_ptr_blen;

	struct dirlog *d_ptr = NULL;

	while ((d_ptr = (struct dirlog *) g_read(&buffer, &g_act_1, DL_SZ))) {
		d_ptr_blen = strlen(d_ptr->dirname);
		if (d_ptr_blen == s_blen
				&& !strncmp(buffer_s, d_ptr->dirname, d_ptr_blen)) {
			ur = g_act_1.offset - 1;
			if (mode == 2 && callback) {
				if (callback_f(&buffer)) {
					break;
				}
			} else {

				break;
			}
		}
	}

	if (mode != 1) {
		g_close(&g_act_1);
	}

	return ur;
}

uint64_t nukelog_find(char *dirn, int mode, struct nukelog *output) {
	struct nukelog buffer = { 0 };

	uint64_t r = MAX_uint64_t;
	char *dup, *dup2, *base, *dir;

	if (g_fopen(NUKELOG, "r", F_DL_FOPEN_BUFFER, &g_act_2)) {
		goto r_end;
	}

	int gi1, gi2;
	gi2 = strlen(dirn);

	struct nukelog *n_ptr = NULL;

	while ((n_ptr = (struct nukelog *) g_read(&buffer, &g_act_2, NL_SZ))) {
		dup = strdup(n_ptr->dirname);
		base = basename(dup);
		gi1 = strlen(base);
		dup2 = strdup(n_ptr->dirname);
		dir = dirname(dup2);

		if (gi1 >= gi2 || gi1 < 2) {
			goto l_end;
		}

		if (!strncmp(&dirn[gi2 - gi1], base, gi1)
				&& !strncmp(dirn, n_ptr->dirname, strlen(dir))) {
			if (output) {
				g_memcpy(output, n_ptr, NL_SZ);
			}
			r = g_act_2.offset - 1;
			if (mode != 2) {
				g_free(dup);
				g_free(dup2);
				break;
			}
		}
		l_end: g_free(dup2);
		g_free(dup);
	}

	if (mode != 1) {
		g_close(&g_act_2);
	}

	r_end:

	return r;
}

#define MSG_REDF_ABORT "WARNING: %s: aborting rebuild (will not be writing what was done up to here)\n"

int rebuild_data_file(char *file, __g_handle hdl) {
	g_setjmp(0, "rebuild_data_file", NULL, NULL);
	int ret = 0, r;
	off_t sz_r;
	struct stat st;
	char buffer[PATH_MAX] = { 0 };

	if (strlen(file) + 4 > PATH_MAX) {
		return 1;
	}

	bzero(hdl->s_buffer, PATH_MAX - 1);
	snprintf(hdl->s_buffer, PATH_MAX - 1, "%s.%d.dtm", file, getpid());
	snprintf(buffer, PATH_MAX - 1, "%s.bk", file);

	pmda p_ptr;

	if (hdl->flags & F_GH_FFBUFFER) {
		p_ptr = &hdl->w_buffer;

	} else {
		p_ptr = &hdl->buffer;
	}

	if (updmode != UPD_MODE_RECURSIVE) {
		g_setjmp(0, "rebuild_data_file(2)", NULL, NULL);
		if ((r = g_filter(hdl, p_ptr))) {
			if (r == 1) {
				if (!(gfl & F_OPT_FORCE)) {
					print_str(
							"WARNING: %s: everything got filtered, refusing to write 0-byte data file\n",
							file);
					return 11;
				}
			} else if (r == 2) {
				print_str(
						"ERROR: %s: failed unlinking record entry after match!\n",
						file);
				return 12;
			}
		}
		if ((gfl & F_OPT_NOFQ) && (hdl->exec_args.exc || (gfl & F_OPT_HASMATCH))
				&& !(hdl->flags & F_GH_APFILT)) {
			return 0;
		}
	}

	if (do_sort(&g_act_1, g_sort_field, g_sort_flags)) {
		ret = 11;
		goto end;
	}

	g_setjmp(0, "rebuild_data_file(3)", NULL, NULL);

	if (gfl & F_OPT_KILL_GLOBAL) {
		print_str( MSG_REDF_ABORT, file);
		return 0;
	}

	if (gfl & F_OPT_VERBOSE) {
		print_str("NOTICE: %s: flushing data to disk..\n", hdl->s_buffer);
	}

	hdl->st_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

	if (!file_exists(file)) {
		if (stat(file, &st)) {
			print_str(
					"WARNING: %s: [%d]: could not get stats from data file!\n",
					errno, file);
		} else {
			hdl->st_mode = st.st_mode;
		}
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: using mode %o\n", hdl->s_buffer, hdl->st_mode);
	}

	off_t rw_o = hdl->rw, bw_o = hdl->bw;

	if ((r = flush_data_md(hdl, hdl->s_buffer))) {
		if (r == 1) {
			if (gfl & F_OPT_VERBOSE) {
				print_str("WARNING: %s: empty buffer (nothing to flush)\n",
						hdl->s_buffer);
			}
			ret = 0;
		} else {
			print_str("ERROR: %s: [%d] flushing data failed!\n", hdl->s_buffer,
					r);
			ret = 2;
		}

		goto end;
	}

	g_setjmp(0, "rebuild_data_file(4)", NULL, NULL);

	if (gfl & F_OPT_KILL_GLOBAL) {
		print_str( MSG_REDF_ABORT, file);
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		return 0;
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: flushed %llu records, %llu bytes\n",
				hdl->s_buffer, (ulint64_t) (hdl->rw - rw_o),
				(ulint64_t) (hdl->bw - bw_o));
	}

	g_setjmp(0, "rebuild_data_file(5)", NULL, NULL);

	if (!(gfl & F_OPT_FORCE) && !(gfl & F_OPT_NOWRITE)
			&& (sz_r = get_file_size(hdl->s_buffer)) < hdl->block_sz) {
		print_str(
				"ERROR: %s: [%u/%u] generated data file is smaller than a single record!\n",
				hdl->s_buffer, (uint32_t) sz_r, (uint32_t) hdl->block_sz);
		ret = 7;
		if (!(gfl & F_OPT_NOWRITE)) {
			remove(hdl->s_buffer);
		}
		goto cleanup;
	}

	g_setjmp(0, "rebuild_data_file(6)", NULL, NULL);

	if (!(gfl & F_OPT_NOWRITE)
			&& !((hdl->flags & F_GH_WAPPEND) && (hdl->flags & F_GH_DFWASWIPED))) {
		if (data_backup_records(file)) {
			ret = 3;
			if (!(gfl & F_OPT_NOWRITE)) {
				remove(hdl->s_buffer);
			}
			goto cleanup;
		}
	}

	g_setjmp(0, "rebuild_data_file(7)", NULL, NULL);

	if (!(gfl & F_OPT_NOWRITE)) {

		if (hdl->fh) {
			g_fclose(hdl->fh);
			hdl->fh = NULL;
		}

		if (!file_exists(file) && !(hdl->flags & F_GH_DFNOWIPE)
				&& (hdl->flags & F_GH_WAPPEND)
				&& !(hdl->flags & F_GH_DFWASWIPED)) {
			if (remove(file)) {
				print_str("ERROR: %s: could not clean old data file\n", file);
				ret = 9;
				goto cleanup;
			}
			hdl->flags |= F_GH_DFWASWIPED;
		}

		g_setjmp(0, "rebuild_data_file(8)", NULL, NULL);

		if (!strncmp(hdl->mode, "a", 1) || (hdl->flags & F_GH_WAPPEND)) {
			if ((r = (int) file_copy(hdl->s_buffer, file, "ab",
			F_FC_MSET_SRC)) < 1) {
				print_str("ERROR: %s: [%d] merging temp file failed!\n",
						hdl->s_buffer, r);
				ret = 4;
			}

		} else {
			if ((r = rename(hdl->s_buffer, file))) {
				print_str("ERROR: %s: [%d] renaming temporary file failed!\n",
						hdl->s_buffer,
						errno);
				ret = 4;
			}
			goto end;
		}

		cleanup:

		if ((r = remove(hdl->s_buffer))) {
			print_str(
					"WARNING: %s: [%d] deleting temporary file failed (remove manually)\n",
					hdl->s_buffer,
					errno);
			ret = 5;
		}

	}
	end:

	return ret;
}

#define MAX_WBUFFER_HOLD	100000

int g_load_record(__g_handle hdl, const void *data) {
	g_setjmp(0, "g_load_record", NULL, NULL);
	void *buffer = NULL;

	if (hdl->w_buffer.offset == MAX_WBUFFER_HOLD) {
		hdl->w_buffer.flags |= F_MDA_FREE;
		rebuild_data_file(hdl->file, hdl);
		p_md_obj ptr = hdl->w_buffer.objects, ptr_s;
		if (gfl & F_OPT_VERBOSE3) {
			print_str("NOTICE: scrubbing write cache..\n");
		}
		while (ptr) {
			ptr_s = ptr->next;
			g_free(ptr->ptr);
			bzero(ptr, sizeof(md_obj));
			ptr = ptr_s;
		}
		hdl->w_buffer.pos = hdl->w_buffer.objects;
		hdl->w_buffer.offset = 0;
	}

	buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);

	if (!buffer) {
		return 2;
	}

	memcpy(buffer, data, hdl->block_sz);

	return 0;
}

int flush_data_md(__g_handle hdl, char *outfile) {
	g_setjmp(0, "flush_data_md", NULL, NULL);

	if (gfl & F_OPT_NOWRITE) {
		return 0;
	}

	FILE *fh = NULL;
	size_t bw = 0;
	unsigned char *buffer = NULL;
	char *mode = "w";

	int ret = 0;

	if (!(gfl & F_OPT_FORCE)) {
		if (hdl->flags & F_GH_FFBUFFER) {
			if (!hdl->w_buffer.offset) {
				return 1;
			}
		} else {
			if (!hdl->buffer.count) {
				return 1;
			}
		}
	}

	if ((fh = gg_fopen(outfile, mode)) == NULL) {
		return 2;
	}

	size_t v = (V_MB * 8) / hdl->block_sz;

	buffer = calloc(v, hdl->block_sz);

	p_md_obj ptr;

	if (hdl->flags & F_GH_FFBUFFER) {
		ptr = md_first(&hdl->w_buffer);
	} else {
		ptr = md_first(&hdl->buffer);
	}

	g_setjmp(0, "flush_data_md(loop)", NULL, NULL);

	while (ptr) {
		if ((bw = g_fwrite(ptr->ptr, hdl->block_sz, 1, fh)) != 1) {
			ret = 3;
			break;
		}
		hdl->bw += hdl->block_sz;
		hdl->rw++;
		ptr = ptr->next;
	}

	if (!hdl->bw && !(gfl & F_OPT_FORCE)) {
		ret = 5;
	}

	g_setjmp(0, "flush_data_md(2)", NULL, NULL);

	g_free(buffer);
	g_fclose(fh);

	if (hdl->st_mode) {
		chmod(outfile, hdl->st_mode);
	}

	return ret;
}

size_t g_load_data_md(void *output, size_t max, char *file, __g_handle hdl) {
	g_setjmp(0, "g_load_data_md", NULL, NULL);
	size_t fr, c_fr = 0;
	FILE *fh;

	if (!(hdl->flags & F_GH_FROMSTDIN)) {
		if (!(fh = gg_fopen(file, "rb"))) {

			return 0;
		}
	} else {
		fh = stdin;
	}

	unsigned char *b_output = (unsigned char*) output;
	while ((fr = g_fread(&b_output[c_fr], 1, max - c_fr, fh))) {
		c_fr += fr;
	}

	if (!(hdl->flags & F_GH_FROMSTDIN)) {
		g_fclose(fh);
	}

	return c_fr;
}

int gen_md_data_ref(__g_handle hdl, pmda md, off_t count) {

	unsigned char *w_ptr = (unsigned char*) hdl->data;

	off_t i;

	for (i = 0; i < count; i++) {
		md->lref_ptr = (void*) w_ptr;
		w_ptr += hdl->block_sz;
		if (!md_alloc(md, hdl->block_sz)) {
			md_g_free(md);
			return -5;
		}

		//hdl->buffer_count++;
	}

	return 0;
}

int is_memregion_null(void *addr, size_t size) {
	size_t i = size - 1;
	unsigned char *ptr = (unsigned char*) addr;
	while (!ptr[i] && i) {
		i--;
	}
	return i;
}

int gen_md_data_ref_cnull(__g_handle hdl, pmda md, off_t count) {

	unsigned char *w_ptr = (unsigned char*) hdl->data;

	off_t i;

	for (i = 0; i < count; i++) {
		if (is_memregion_null((void*) w_ptr, hdl->block_sz)) {
			md->lref_ptr = (void*) w_ptr;
			if (!md_alloc(md, hdl->block_sz)) {
				md_g_free(md);
				return -5;
			}

			//hdl->buffer_count++;
		}
		w_ptr += hdl->block_sz;
	}

	return 0;
}

typedef int (*__g_mdref)(__g_handle hdl, pmda md, off_t count);

int load_data_md(pmda md, char *file, __g_handle hdl) {
	g_setjmp(0, "load_data_md", NULL, NULL);
	errno = 0;
	int r = 0;
	off_t count = 0;

	if (!hdl->block_sz) {
		return -2;
	}

	uint32_t sh_ret = 0;

	if (hdl->flags & F_GH_ONSHM) {
		if ((r = g_shmap_data(hdl, SHM_IPC))) {
			md_g_free(md);
			return r;
		}
		count = hdl->total_sz / hdl->block_sz;
	} else if (hdl->flags & F_GH_SHM) {
		if (hdl->shmid != -1) {
			sh_ret |= R_SHMAP_ALREADY_EXISTS;
		}
		hdl->data = shmap(hdl->ipc_key, &hdl->ipcbuf, (size_t) hdl->total_sz,
				&sh_ret, &hdl->shmid);

		if (sh_ret & R_SHMAP_FAILED_ATTACH) {
			return -22;
		}
		if (sh_ret & R_SHMAP_FAILED_SHMAT) {
			return -23;
		}
		if (!hdl->data) {
			return -12;
		}

		if (sh_ret & R_SHMAP_ALREADY_EXISTS) {
			errno = 0;
			if (hdl->flags & F_GH_SHMRB) {
				bzero(hdl->data, hdl->ipcbuf.shm_segsz);
			}

			hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;
		}
		count = hdl->total_sz / hdl->block_sz;
	} else {
		if (!hdl->total_sz) {
			return -3;
		}
		count = hdl->total_sz / hdl->block_sz;
		hdl->data = malloc(count * hdl->block_sz);
	}

	size_t b_read = 0;

	//hdl->buffer_count = 0;

	__g_mdref cb = NULL;

	if ((hdl->flags & F_GH_ONSHM)) {
		cb = (__g_mdref ) gen_md_data_ref_cnull;
	} else {
		if (!((hdl->flags & F_GH_SHM) && (sh_ret & R_SHMAP_ALREADY_EXISTS))
				|| ((hdl->flags & F_GH_SHM) && (hdl->flags & F_GH_SHMRB))) {
			if ((b_read = g_load_data_md(hdl->data, hdl->total_sz, file, hdl))
					% hdl->block_sz || !b_read) {

				md_g_free(md);
				return -9;
			}
			if (b_read != hdl->total_sz) {
				hdl->total_sz = b_read;
				count = hdl->total_sz / hdl->block_sz;
			}
		}
		cb = (__g_mdref ) gen_md_data_ref;
	}

	if (md_init(md, count)) {
		return -4;
	}

	md->flags |= F_MDA_REFPTR;
	cb(hdl, md, count);

	g_setjmp(0, "load_data_md", NULL, NULL);

	if (!md->count) {
		return -5;
	}

	return 0;
}

#define MSG_DEF_SHM "SHARED MEMORY"

int g_map_shm(__g_handle hdl, key_t ipc) {
	hdl->flags |= F_GH_ONSHM;

	if (hdl->buffer.count) {
		return 0;
	}

	if (!SHM_IPC) {
		print_str(
				"ERROR: %s: could not get IPC key, set manually (--ipc <key>)\n",
				MSG_DEF_SHM);
		return 1;
	}

	int r;

	if ((r = load_data_md(&hdl->buffer, NULL, hdl))) {
		if (((gfl & F_OPT_VERBOSE) && r != 1002) || (gfl & F_OPT_VERBOSE4)) {
			print_str(
					"ERROR: %s: [%u/%u] [%u] [%u] could not map shared memory segment! [%d] [%d]\n",
					MSG_DEF_SHM, (uint32_t) hdl->buffer.count,
					(uint32_t) (hdl->total_sz / hdl->block_sz),
					(uint32_t) hdl->total_sz, hdl->block_sz, r, errno);
		}
		return 9;
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: mapped %u records\n",
		MSG_DEF_SHM, (uint32_t) hdl->buffer.count);
	}

	hdl->flags |= F_GH_ISONLINE;
	hdl->d_memb = 3;

	hdl->g_proc1 = ref_to_val_online;
	hdl->g_proc2 = ref_to_val_ptr_online;
	hdl->g_proc3 = online_format_block;

	return 0;
}

#define F_GBM_SHM_NO_DATAFILE		(a32 << 1)

int g_buffer_into_memory(char *file, __g_handle hdl) {
	g_setjmp(0, "g_buffer_into_memory", NULL, NULL);

	uint32_t flags = 0;

	if (hdl->buffer.count) {
		return 0;
	}

	struct stat st;

	if (stat(file, &st) == -1) {
		if (!(gfl & F_OPT_SHAREDMEM)) {
			print_str(
					"ERROR: %s: [%d] unable to get information from data file\n",
					file,
					errno);
			return 2;
		} else {
			flags |= F_GBM_SHM_NO_DATAFILE;
		}
	}

	if (!(flags & F_GBM_SHM_NO_DATAFILE)) {
		if (!st.st_size) {
			print_str("WARNING: %s: 0-byte data file\n", file);
			return 3;
		}

		if (st.st_size > db_max_size) {
			print_str(
					"WARNING: %s: disabling memory buffering, file too big (%lld MB max)\n",
					file, db_max_size / 1024 / 1024);
			hdl->flags |= F_GH_NOMEM;
			return 5;
		}

		hdl->total_sz = st.st_size;
	}

	bzero(hdl->file, PATH_MAX);
	g_strncpy(hdl->file, file, strlen(file));

	if (determine_datatype(hdl)) {
		print_str(MSG_BAD_DATATYPE, file);
		return 6;
	}

	if (gfl & F_OPT_SHMRELOAD) {
		hdl->flags |= F_GH_SHMRB;
	}

	if (gfl & F_OPT_SHMDESTROY) {
		hdl->flags |= F_GH_SHMDESTROY;
	}

	if (gfl & F_OPT_SHMDESTONEXIT) {
		hdl->flags |= F_GH_SHMDESTONEXIT;
	}

	off_t tot_sz = 0;

	if (!(flags & F_GBM_SHM_NO_DATAFILE)) {
		if (st.st_size % hdl->block_sz) {
			print_str(MSG_GEN_DFCORRU, file, (ulint64_t) st.st_size,
					hdl->block_sz);
			return 12;
		}

		tot_sz = hdl->total_sz;

	} else {
		bzero(hdl->file, PATH_MAX);
		g_strncpy(hdl->file, "SHM", 3);
	}

	if (gfl & F_OPT_SHAREDMEM) {
		hdl->shmid = -1;
		hdl->flags |= F_GH_SHM;
		if ((hdl->shmid = shmget(hdl->ipc_key, 0, 0)) != -1) {
			if (gfl & F_OPT_VERBOSE2) {
				print_str(
						"NOTICE: %s: [IPC: 0x%.8X]: [%d]: attached to existing shared memory segment\n",
						hdl->file, (uint32_t) hdl->ipc_key, hdl->shmid);
			}
			if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) != -1) {
				if (flags & F_GBM_SHM_NO_DATAFILE) {
					hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;
				}
				if ((off_t) hdl->ipcbuf.shm_segsz != hdl->total_sz
						|| ((hdl->flags & F_GH_SHMDESTROY)
								&& !(flags & F_GBM_SHM_NO_DATAFILE))) {
					if (gfl & F_OPT_VERBOSE2) {
						print_str(
								"NOTICE: %s: [IPC: 0x%.8X]: destroying existing shared memory segment [%zd]%s\n",
								hdl->file, hdl->ipc_key, hdl->ipcbuf.shm_segsz,
								((off_t) hdl->ipcbuf.shm_segsz != hdl->total_sz) ?
										": segment and data file sizes differ" :
										"");
					}

					if (shmctl(hdl->shmid, IPC_RMID, NULL) == -1) {
						print_str(
								"WARNING: %s: [IPC: 0x%.8X] [%d] unable to destroy shared memory segment\n",
								hdl->file, hdl->ipc_key, errno);
					} else {
						if (gfl & F_OPT_VERBOSE2) {
							print_str(
									"NOTICE: %s: [IPC: 0x%.8X]: [%d]: marked segment to be destroyed\n",
									hdl->file, hdl->ipc_key, hdl->shmid);
						}
						hdl->shmid = -1;
					}
				}
			} else {
				print_str(
						"ERROR: %s: [IPC: 0x%.8X]: [%d]: could not get shared memory segment information from kernel\n",
						hdl->file, hdl->ipc_key, errno);
				return 21;
			}
			if ((gfl & F_OPT_VERBOSE2) && hdl->shmid != -1
					&& (hdl->flags & F_GH_SHMRB)) {
				print_str(
						"NOTICE: %s: [IPC: 0x%.8X]: [%d]: segment data will be reloaded from file\n",
						hdl->file, hdl->ipc_key, hdl->shmid);
			}

		} else if ((flags & F_GBM_SHM_NO_DATAFILE)) {
			print_str(
					"ERROR: %s: [IPC: 0x%.8X]: failed loading shared memory segment: [%d]: no shared memory segment or data file available to load\n",
					hdl->file, hdl->ipc_key, errno);
			return 22;
		}
	}

	if (gfl & F_OPT_VERBOSE2) {
		print_str("NOTICE: %s: %s [%llu records] [%llu bytes]\n", file,
				(hdl->flags & F_GH_SHM) ?
						hdl->shmid == -1 && !(hdl->flags & F_GH_SHMRB) ?
								"loading shared memory segment" :
						(hdl->flags & F_GH_SHMRB) ?
								"re-loading shared memory segment" :
								"mapping shared memory segment"
						:
						"loading data file into memory",
				(hdl->flags & F_GH_SHM) && hdl->shmid != -1 ?
						(uint64_t) (hdl->ipcbuf.shm_segsz / hdl->block_sz) :
						(uint64_t) (hdl->total_sz / hdl->block_sz),
				(hdl->flags & F_GH_SHM) && hdl->shmid != -1 ?
						(ulint64_t) hdl->ipcbuf.shm_segsz :
						(ulint64_t) hdl->total_sz);
	}

	errno = 0;
	int r;
	if ((r = load_data_md(&hdl->buffer, file, hdl))) {
		print_str(
				"ERROR: %s: [%llu/%llu] [%llu] [%u] could not load data!%s [%d] [%d]\n",
				file, (ulint64_t) hdl->buffer.count,
				(ulint64_t) (hdl->total_sz / hdl->block_sz), hdl->total_sz,
				hdl->block_sz,
				(hdl->flags & F_GH_SHM) ? " [shared memory segment]" : "", r,
				errno);
		return 4;
	} else {
		if (!(flags & F_GBM_SHM_NO_DATAFILE)) {
			if (tot_sz != hdl->total_sz) {
				print_str(
						"WARNING: %s: [%llu/%llu] actual data loaded was not the same size as source data file\n",
						file, (uint64_t) hdl->total_sz, (uint64_t) tot_sz);
			}
		}
		if (gfl & F_OPT_VERBOSE2) {
			print_str("NOTICE: %s: loaded %llu records\n", file,
					(uint64_t) hdl->buffer.count);
		}
	}
	return 0;
}

int determine_datatype(__g_handle hdl) {
	if (!strncmp(hdl->file, DIRLOG, strlen(DIRLOG))) {
		hdl->flags |= F_GH_ISDIRLOG;
		hdl->block_sz = DL_SZ;
		hdl->d_memb = 7;
		hdl->g_proc0 = gcb_dirlog;
		hdl->g_proc1 = ref_to_val_dirlog;
		hdl->g_proc2 = ref_to_val_ptr_dirlog;
		hdl->g_proc3 = dirlog_format_block;
		hdl->ipc_key = IPC_KEY_DIRLOG;
	} else if (!strncmp(hdl->file, NUKELOG, strlen(NUKELOG))) {
		hdl->flags |= F_GH_ISNUKELOG;
		hdl->block_sz = NL_SZ;
		hdl->d_memb = 9;
		hdl->g_proc0 = gcb_nukelog;
		hdl->g_proc1 = ref_to_val_nukelog;
		hdl->g_proc2 = ref_to_val_ptr_nukelog;
		hdl->g_proc3 = nukelog_format_block;
		hdl->ipc_key = IPC_KEY_NUKELOG;
	} else if (!strncmp(hdl->file, DUPEFILE, strlen(DUPEFILE))) {
		hdl->flags |= F_GH_ISDUPEFILE;
		hdl->block_sz = DF_SZ;
		hdl->d_memb = 3;
		hdl->g_proc0 = gcb_dupefile;
		hdl->g_proc1 = ref_to_val_dupefile;
		hdl->g_proc2 = ref_to_val_ptr_dupefile;
		hdl->g_proc3 = dupefile_format_block;
		hdl->ipc_key = IPC_KEY_DUPEFILE;
	} else if (!strncmp(hdl->file, LASTONLOG, strlen(LASTONLOG))) {
		hdl->flags |= F_GH_ISLASTONLOG;
		hdl->block_sz = LO_SZ;
		hdl->d_memb = 8;
		hdl->g_proc0 = gcb_lastonlog;
		hdl->g_proc1 = ref_to_val_lastonlog;
		hdl->g_proc2 = ref_to_val_ptr_lastonlog;
		hdl->g_proc3 = lastonlog_format_block;
		hdl->ipc_key = IPC_KEY_LASTONLOG;
	} else if (!strncmp(hdl->file, ONELINERS, strlen(ONELINERS))) {
		hdl->flags |= F_GH_ISONELINERS;
		hdl->block_sz = OL_SZ;
		hdl->d_memb = 5;
		hdl->g_proc0 = gcb_oneliner;
		hdl->g_proc1 = ref_to_val_oneliners;
		hdl->g_proc2 = ref_to_val_ptr_oneliners;
		hdl->g_proc3 = oneliner_format_block;
		hdl->ipc_key = IPC_KEY_ONELINERS;
	} else if (!strncmp(hdl->file, IMDBLOG, strlen(IMDBLOG))) {
		hdl->flags |= F_GH_ISIMDB;
		hdl->block_sz = ID_SZ;
		hdl->d_memb = 13;
		hdl->g_proc0 = gcb_imdbh;
		hdl->g_proc1 = ref_to_val_imdb;
		hdl->g_proc2 = ref_to_val_ptr_imdb;
		hdl->g_proc3 = imdb_format_block;
		hdl->ipc_key = IPC_KEY_IMDBLOG;
	} else if (!strncmp(hdl->file, GAMELOG, strlen(GAMELOG))) {
		hdl->flags |= F_GH_ISGAME;
		hdl->block_sz = GM_SZ;
		hdl->d_memb = 3;
		hdl->g_proc0 = gcb_game;
		hdl->g_proc1 = ref_to_val_game;
		hdl->g_proc2 = ref_to_val_ptr_game;
		hdl->g_proc3 = game_format_block;
		hdl->ipc_key = IPC_KEY_GAMELOG;
	} else if (!strncmp(hdl->file, TVLOG, strlen(TVLOG))) {
		hdl->flags |= F_GH_ISTVRAGE;
		hdl->block_sz = TV_SZ;
		hdl->d_memb = 15;
		hdl->g_proc0 = gcb_tv;
		hdl->g_proc1 = ref_to_val_tv;
		hdl->g_proc2 = ref_to_val_ptr_tv;
		hdl->g_proc3 = tv_format_block;
		hdl->ipc_key = IPC_KEY_TVRAGELOG;
	} else if (!strncmp(hdl->file, GE1LOG, strlen(GE1LOG))) {
		hdl->flags |= F_GH_ISGENERIC1;
		hdl->block_sz = G1_SZ;
		hdl->d_memb = 9;
		hdl->g_proc0 = gcb_gen1;
		hdl->g_proc1 = ref_to_val_gen1;
		hdl->g_proc2 = ref_to_val_ptr_gen1;
		hdl->g_proc3 = gen1_format_block;
		hdl->ipc_key = IPC_KEY_GEN1LOG;
	} else {
		return 1;
	}

	return 0;
}

int g_fopen(char *file, char *mode, uint32_t flags, __g_handle hdl) {
	g_setjmp(0, "g_fopen", NULL, NULL);

	if (flags & F_DL_FOPEN_SHM) {
		if (g_map_shm(hdl, SHM_IPC)) {
			return 12;
		}
		if (g_proc_mr(hdl)) {
			return 14;
		}
		return 0;
	}

	if (flags & F_DL_FOPEN_REWIND) {
		gh_rewind(hdl);
	}

	if (!(gfl & F_OPT_NOBUFFER) && (flags & F_DL_FOPEN_BUFFER)
			&& !(hdl->flags & F_GH_NOMEM)) {
		if (!g_buffer_into_memory(file, hdl)) {
			if (g_proc_mr(hdl)) {
				return 24;
			}

			if (!(flags & F_DL_FOPEN_FILE)) {
				return 0;
			}
		} else {
			if (!(hdl->flags & F_GH_NOMEM)) {
				return 11;
			}
		}
	}

	if (hdl->fh) {
		return 0;
	}

	if (strlen(file) > PATH_MAX) {
		print_str(MSG_GEN_NODFILE, file, "file path too large");
		return 2;
	}

	hdl->total_sz = get_file_size(file);

	if (!hdl->total_sz) {
		print_str(MSG_GEN_NODFILE, file, "zero-byte data file");
	}

	bzero(hdl->file, PATH_MAX);
	g_strncpy(hdl->file, file, strlen(file));

	if (determine_datatype(hdl)) {
		print_str(MSG_GEN_NODFILE, file, "could not determine data-type");
		return 3;
	}

	if (hdl->total_sz % hdl->block_sz) {
		print_str(MSG_GEN_DFCORRU, file, (ulint64_t) hdl->total_sz,
				hdl->block_sz);
		return 4;
	}

	FILE *fd;

	if (!(fd = gg_fopen(file, mode))) {
		print_str(MSG_GEN_NODFILE, file, "not available");
		return 1;
	}

	g_proc_mr(hdl);

	hdl->fh = fd;

	return 0;
}

int g_close(__g_handle hdl) {
	g_setjmp(0, "g_close", NULL, NULL);
	bzero(&dl_stats, sizeof(struct d_stats));
	dl_stats.br += hdl->br;
	dl_stats.bw += hdl->bw;
	dl_stats.rw += hdl->rw;

	if (hdl->fh) {
		g_fclose(hdl->fh);
		hdl->fh = NULL;
	}

	if (hdl->buffer.count) {
		hdl->buffer.r_pos = hdl->buffer.objects;
		hdl->buffer.pos = hdl->buffer.r_pos;
		hdl->buffer.offset = 0;
		hdl->offset = 0;
		if ((hdl->flags & F_GH_ONSHM)) {
			md_g_free(&hdl->buffer);
			md_g_free(&hdl->w_buffer);
		}
	}

	hdl->br = 0;
	hdl->bw = 0;
	hdl->rw = 0;

	return 0;
}

static int prep_for_exec(void) {
	const char inputfile[] = "/dev/null";

	if (close(0) < 0) {
		fprintf(stdout, "ERROR: could not close stdin\n");
		return 1;
	} else {
		if (open(inputfile, O_RDONLY
#if defined O_LARGEFILE
				| O_LARGEFILE
#endif
				) < 0) {
			fprintf(stdout, "ERROR: could not open %s\n", inputfile);
		}
	}
	return 0;
}

int l_execv(char *exec, char **argv) {
	pid_t c_pid;

	fflush(stdout);
	fflush(stderr);

	c_pid = fork();

	if (c_pid == (pid_t) -1) {
		fprintf(stderr, "ERROR: %s: fork failed\n", exec);
		return 1;
	}
	if (!c_pid) {
		if (prep_for_exec()) {
			_exit(1);
		} else {
			execv(exec, argv);
			fprintf(stderr, "ERROR: %s: execv failed to execute [%d]\n", exec,
			errno);
			_exit(1);
		}
	}
	int status;
	while (waitpid(c_pid, &status, 0) == (pid_t) -1) {
		if (errno != EINTR) {
			fprintf(stderr,
					"ERROR: %s:failed waiting for child process to finish [%d]\n",
					exec, errno);
			return 2;
		}
	}

	return status;
}

int g_do_exec_v(void *buffer, void *callback, char *ex_str, void * p_hdl) {
	g_setjmp(0, "g_do_exec", NULL, NULL);

	__g_handle hdl = (__g_handle) p_hdl;

	process_exec_args(buffer, hdl);

	return l_execv(hdl->exec_args.exec_v_path, hdl->exec_args.argv_c);
}

int g_do_exec(void *buffer, void *callback, char *ex_str, void *hdl) {
	g_setjmp(0, "g_do_exec", NULL, NULL);

	if (callback) {
		char b_glob[MAX_EXEC_STR];
		char *e_str;
		if (ex_str) {
			e_str = ex_str;
		} else {
			if (!exec_str) {
				return -1;
			}
			e_str = exec_str;
		}
		bzero(b_glob, MAX_EXEC_STR);

		if (process_exec_string(e_str, b_glob, MAX_EXEC_STR, callback,
				buffer)) {
			return 2;
		}

		return system(b_glob);
	} else if (ex_str) {
		if (strlen(ex_str) > MAX_EXEC_STR) {
			return -1;
		}
		return system(ex_str);
	} else {
		return -1;
	}
}

void *g_read(void *buffer, __g_handle hdl, size_t size) {
	if (hdl->buffer.count) {
		hdl->buffer.pos = hdl->buffer.r_pos;
		if (!hdl->buffer.pos) {
			return NULL;
		}
		hdl->buffer.r_pos = *((void**) hdl->buffer.r_pos + hdl->j_offset);
		hdl->buffer.offset++;
		hdl->offset++;
		hdl->br += hdl->block_sz;
		return (void *) hdl->buffer.pos->ptr;
	}

	if (!buffer) {
		print_str("IO ERROR: %s: no buffer to write to\n", hdl->file);
		return NULL;
	}

	if (!hdl->fh) {
		print_str("IO ERROR: %s: data file handle not open\n", hdl->file);
		return NULL;
	}

	if (feof(hdl->fh)) {
		return NULL;
	}

	size_t fr;

	if ((fr = g_fread(buffer, 1, size, hdl->fh)) != size) {
		if (fr == 0) {
			return NULL;
		}
		return NULL;
	}

	hdl->br += fr;
	hdl->offset++;

	return buffer;
}

size_t read_from_pipe(char *buffer, FILE *pipe) {
	size_t read = 0, r;

	while (!feof(pipe)) {
		if ((r = g_fread(&buffer[read], 1, PIPE_READ_MAX - read, pipe)) <= 0) {
			break;
		}
		read += r;
	}

	return read;
}

size_t exec_and_wait_for_output(char *command, char *output) {
	char buf[PIPE_READ_MAX] = { 0 };
	size_t r = 0;
	FILE *pipe = NULL;

	if (!(pipe = popen(command, "r"))) {
		return 0;
	}

	r = read_from_pipe(buf, pipe);

	pclose(pipe);

	if (output && r) {

		g_strncpy(output, buf, strlen(buf));
	}

	return r;
}

int dirlog_write_record(struct dirlog *buffer, off_t offset, int whence) {
	g_setjmp(0, "dirlog_write_record", NULL, NULL);
	if (gfl & F_OPT_NOWRITE) {
		return 0;
	}

	if (!buffer) {
		return 2;
	}

	if (!g_act_1.fh) {
		print_str("ERROR: dirlog handle is not open\n");
		return 1;
	}

	if (whence == SEEK_SET
			&& fseeko(g_act_1.fh, offset * DL_SZ, SEEK_SET) < 0) {
		print_str("ERROR: seeking dirlog failed!\n");
		return 1;
	}

	int fw;

	if ((fw = g_fwrite(buffer, 1, DL_SZ, g_act_1.fh)) < DL_SZ) {
		print_str("ERROR: could not write dirlog record! %d/%d\n", fw,
				(int) DL_SZ);
		return 1;
	}

	g_act_1.bw += (off_t) fw;
	g_act_1.rw++;

	if (whence == SEEK_SET)
		g_act_1.offset = offset;
	else
		g_act_1.offset++;

	return 0;
}

typedef int (*__d_edscb)(char *, unsigned char, void *, __g_eds);

int enum_dir(char *dir, void *cb, void *arg, int f, __g_eds eds) {
	__d_edscb callback_f = (__d_edscb ) cb;
	struct dirent *dirp, _dirp = { 0 };
	int r = 0, ir;

	DIR *dp = opendir(dir);

	if (!dp) {
		return -2;
	}

	char buf[PATH_MAX];
	struct stat st;

	if (stat(dir, &st)) {
		return -3;
	}

	if (eds) {
		if (!(eds->flags & F_EDS_ROOTMINSET)) {
			eds->r_minor = minor(st.st_dev);
			eds->flags |= F_EDS_ROOTMINSET;
			g_memcpy(&eds->st, &st, sizeof(struct stat));
		}

		if (!(f & F_ENUMD_NOXBLK) && (gfl & F_OPT_XBLK)
				&& major(eds->st.st_dev) != 8) {
			r = -6;
			goto end;
		}

		if ((gfl & F_OPT_XDEV) && minor(st.st_dev) != eds->r_minor) {
			r = -5;
			goto end;
		}
	}

	while ((dirp = readdir(dp))) {
		if ((gfl & F_OPT_KILL_GLOBAL) || (gfl & F_OPT_TERM_ENUM)) {
			break;
		}

		size_t d_name_l = strlen(dirp->d_name);

		if ((d_name_l == 1 && !strncmp(dirp->d_name, ".", 1))
				|| (d_name_l == 2 && !strncmp(dirp->d_name, "..", 2))) {
			continue;
		}

		snprintf(buf, PATH_MAX, "%s/%s", dir, dirp->d_name);
		remove_repeating_chars(buf, 0x2F);

		if (dirp->d_type == DT_UNKNOWN) {
			_dirp.d_type = get_file_type(buf);
		} else {
			_dirp.d_type = dirp->d_type;
		}

		if (!(ir = callback_f(buf, _dirp.d_type, arg, eds))) {
			if (f & F_ENUMD_ENDFIRSTOK) {
				r = ir;
				break;
			} else {
				r++;
			}
		} else {
			if (f & F_ENUMD_BREAKONBAD) {
				r = ir;
				break;
			}
		}
	}

	end:

	closedir(dp);
	return r;
}

int dir_exists(char *dir) {
	int r;

	errno = 0;
	DIR *dd = opendir(dir);

	r = errno;

	if (dd) {
		closedir(dd);
	} else {
		if (!r) {
			r++;
		}
	}

	return r;
}

int reg_match(char *expression, char *match, int flags) {
	regex_t preg;
	size_t r;
	regmatch_t pmatch[REG_MATCHES_MAX];

	if ((r = regcomp(&preg, expression, (flags | REG_EXTENDED))))
		return r;

	r = regexec(&preg, match, REG_MATCHES_MAX, pmatch, 0);

	regfree(&preg);

	return r;
}

int split_string(char *line, char dl, pmda output_t) {
	int i, p, c, llen = strlen(line);

	for (i = 0, p = 0, c = 0; i <= llen; i++) {

		while (line[i] == dl && line[i])
			i++;
		p = i;

		while (line[i] != dl && line[i] != 0xA && line[i])
			i++;

		if (i > p) {
			char *buffer = md_alloc(output_t, (i - p) + 10);
			if (!buffer)
				return -1;
			g_memcpy(buffer, &line[p], i - p);
			c++;
		}
	}
	return c;
}

int split_string_sp_tab(char *line, pmda output_t) {
	int i, p, c, llen = strlen(line);

	for (i = 0, p = 0, c = 0; i <= llen; i++) {

		while ((line[i] == 0x20 && line[i] != 0x9) && line[i])
			i++;
		p = i;

		while ((line[i] != 0x20 && line[i] != 0x9) && line[i] != 0xA && line[i])
			i++;

		if (i > p) {
			char *buffer = md_alloc(output_t, (i - p) + 10);
			if (!buffer)
				return -1;
			g_memcpy(buffer, &line[p], i - p);
			c++;
		}
	}
	return c;
}

int remove_repeating_chars(char *string, char c) {
	g_setjmp(0, "remove_repeating_chars", NULL, NULL);
	size_t s_len = strlen(string);
	int i, i_c = -1;

	for (i = 0; i <= s_len; i++, i_c = -1) {
		while (string[i + i_c] == c) {
			i_c++;
		}
		if (i_c > 0) {
			int ct_l = (s_len - i) - i_c;
			if (!g_memmove(&string[i], &string[i + i_c], ct_l)) {
				return -1;
			}
			string[i + ct_l] = 0;
			i += i_c;
		} else {
			i += i_c + 1;
		}
	}

	return 0;
}

int write_file_text(char *data, char *file) {
	g_setjmp(0, "write_file_text", NULL, NULL);
	int r;
	FILE *fp;

	if ((fp = gg_fopen(file, "a")) == NULL)
		return 0;

	r = g_fwrite(data, 1, strlen(data), fp);

	g_fclose(fp);

	return r;
}

int delete_file(char *name, unsigned char type, void *arg) {
	g_setjmp(0, "delete_file", NULL, NULL);
	char *match = (char*) arg;

	if (type != DT_REG) {
		return 1;
	}

	if (!reg_match(match, name, 0)) {
		return remove(name);
	}

	return 2;
}

off_t read_file(char *file, void *buffer, size_t read_max, off_t offset,
		FILE *_fp) {
	g_setjmp(0, "read_file", NULL, NULL);
	size_t read;
	int r;
	FILE *fp = _fp;

	if (!_fp) {
		off_t a_fsz = get_file_size(file);

		if (!a_fsz)
			return 0;

		if (read_max > a_fsz) {
			read_max = a_fsz;
		}
		if ((fp = gg_fopen(file, "rb")) == NULL)
			return 0;
	}

	if (offset)
		fseeko(fp, (off_t) offset, SEEK_SET);

	for (read = 0; !feof(fp) && read < read_max;) {
		if ((r = g_fread(&((unsigned char*) buffer)[read], 1, read_max - read,
				fp)) < 1)
			break;
		read += r;
	}

	if (!_fp) {
		g_fclose(fp);
	}

	return read;
}

int file_exists(char *file) {
	g_setjmp(0, "file_exists", NULL, NULL);

	int r = get_file_type(file);

	if (r == DT_REG) {
		return 0;
	}

	return 1;
}

ssize_t file_copy(char *source, char *dest, char *mode, uint32_t flags) {
	g_setjmp(0, "file_copy", NULL, NULL);

	if (gfl & F_OPT_NOWRITE) {
		return 1;
	}

	struct stat st_s, st_d;
	mode_t st_mode = 0;

	if (stat(source, &st_s)) {
		return -9;
	}

	off_t ssize = st_s.st_size;

	if (ssize < 1) {
		return -1;
	}

	FILE *fh_s = gg_fopen(source, "rb");

	if (!fh_s) {
		return -2;
	}

	if (!strncmp(mode, "a", 1) && (flags & F_FC_MSET_DEST)) {
		if (file_exists(dest)) {
			st_mode = st_s.st_mode;
		} else {
			if (stat(source, &st_d)) {
				return -10;
			}
			st_mode = st_d.st_mode;
		}
	} else if (flags & F_FC_MSET_SRC) {
		st_mode = st_s.st_mode;
	}

	FILE *fh_d = gg_fopen(dest, mode);

	if (!fh_d) {
		return -3;
	}

	size_t r = 0, t = 0, w;
	char *buffer = calloc(V_MB, 1);

	while ((r = g_fread(buffer, 1, V_MB, fh_s)) > 0) {
		if ((w = g_fwrite(buffer, 1, r, fh_d))) {
			t += w;
		} else {
			return -4;
		}
	}

	g_free(buffer);
	g_fclose(fh_d);
	g_fclose(fh_s);

	if (st_mode) {
		chmod(dest, st_mode);
	}

	return t;
}

uint64_t file_crc32(char *file, uint32_t *crc_out) {
	g_setjmp(0, "file_crc32", NULL, NULL);
	FILE *fp;
	int read;
	size_t r;
	unsigned char *buffer = calloc(CRC_FILE_READ_BUFFER_SIZE, 1);

	*crc_out = 0x0;

	if ((fp = gg_fopen((char*) &file[0], "rb")) == NULL) {
		g_free(buffer);
		return 0;
	}

	uint32_t crc = MAX_uint32_t;

	for (read = 0, r = 0; !feof(fp);) {
		if ((r = g_fread(buffer, 1, CRC_FILE_READ_BUFFER_SIZE, fp)) < 1)
			break;
		crc = crc32(crc, buffer, r);
		bzero(buffer, CRC_FILE_READ_BUFFER_SIZE);
		read += r;
	}

	if (read)
		*crc_out = crc;

	g_free(buffer);
	g_fclose(fp);

	return read;
}

#define F_CFGV_BUILD_FULL_STRING	(a32 << 1)
#define F_CFGV_RETURN_MDA_OBJECT	(a32 << 2)
#define F_CFGV_RETURN_TOKEN_EX		(a32 << 3)

#define F_CFGV_BUILD_DATA_PATH		(a32 << 10)

#define F_CFGV_MODES				(F_CFGV_BUILD_FULL_STRING|F_CFGV_RETURN_MDA_OBJECT|F_CFGV_RETURN_TOKEN_EX)

#define MAX_CFGV_RES_LENGTH			50000

void *ref_to_val_get_cfgval(char *cfg, char *key, char *defpath, int flags,
		char *out, size_t max) {

	char buffer[PATH_MAX];

	if (flags & F_CFGV_BUILD_DATA_PATH) {
		snprintf(buffer, PATH_MAX, "%s/%s/%s/%s", GLROOT, FTPDATA, defpath,
				cfg);
	} else {
		snprintf(buffer, PATH_MAX, "%s", cfg);
	}

	pmda ret;

	if (load_cfg(&cfg_rf, buffer, 0, &ret)) {
		return NULL;
	}

	mda s_tk = { 0 };
	int r;
	size_t c_token = -1;
	char *s_key = key;

	md_init(&s_tk, 4);

	if ((r = split_string(key, 0x40, &s_tk)) == 2) {
		p_md_obj s_tk_ptr = s_tk.objects->next;
		flags ^= F_CFGV_BUILD_FULL_STRING;
		flags |= F_CFGV_RETURN_TOKEN_EX;
		c_token = atoi(s_tk_ptr->ptr);
		s_key = s_tk.objects->ptr;

	}

	p_md_obj ptr;
	pmda s_ret = NULL;
	void *p_ret = NULL;

	if ((ptr = get_cfg_opt(s_key, ret, &s_ret))) {
		switch (flags & F_CFGV_MODES) {
		case F_CFGV_RETURN_MDA_OBJECT:
			p_ret = (void*) ptr;
			break;
		case F_CFGV_BUILD_FULL_STRING:
			;
			size_t o_w = 0, w;
			while (ptr) {
				w = strlen((char*) ptr->ptr);
				if (o_w + w + 1 < max) {
					g_memcpy(&out[o_w], ptr->ptr, w);
					o_w += w;
					if (ptr->next) {
						memset(&out[o_w], 0x20, 1);
						o_w++;
					}
				} else {
					break;
				}
				ptr = ptr->next;
			}
			p_ret = (void*) out;
			break;
		case F_CFGV_RETURN_TOKEN_EX:

			if (c_token < 0 || c_token >= s_ret->count) {
				return NULL;
			}

			p_md_obj p_ret_tx = &s_ret->objects[c_token];
			if (p_ret_tx) {
				p_ret = (void*) p_ret_tx->ptr;
			}
			break;
		default:
			p_ret = ptr->ptr;
			break;
		}
	}
	md_g_free(&s_tk);
	return p_ret;
}

int is_char_uppercase(char c) {
	if (c >= 0x41 && c <= 0x5A) {
		return 0;
	}
	return 1;
}

int g_l_fmode(char *path, size_t max_size, char *output) {
	struct stat st;
	char buffer[PATH_MAX + 1];
	snprintf(buffer, PATH_MAX, "%s/%s", GLROOT, path);
	remove_repeating_chars(buffer, 0x2F);
	if (lstat(buffer, &st)) {
		return 0;
	}
	snprintf(output, max_size, "%d", IFTODT(st.st_mode));
	return 0;
}

int g_l_fmode_n(char *path, size_t max_size, char *output) {
	struct stat st;
	if (lstat(path, &st)) {
		return 1;
	}
	snprintf(output, max_size, "%d", IFTODT(st.st_mode));
	return 0;
}

int ref_to_val_macro(void *arg, char *match, char *output, size_t max_size) {
	if (!strcmp(match, "m:exe")) {
		if (self_get_path(output)) {
			snprintf(output, max_size, "%s", "UNKNOWN");
		}
	} else if (!strncmp(match, "m:glroot", 8)) {
		snprintf(output, max_size, "%s", GLROOT);
	} else if (!strncmp(match, "m:siteroot", 10)) {
		snprintf(output, max_size, "%s", SITEROOT);
	} else if (!strncmp(match, "m:ftpdata", 9)) {
		snprintf(output, max_size, "%s", FTPDATA);
	} else if ((gfl & F_OPT_PS_LOGGING) && !strncmp(match, "m:logfile", 9)) {
		snprintf(output, max_size, "%s", LOGFILE);
	} else if (!strncmp(match, "m:PID", 5)) {
		snprintf(output, max_size, "%d", getpid());
	} else if (!strncmp(match, "m:IPC", 5)) {
		snprintf(output, max_size, "%.8X", (uint32_t) SHM_IPC);
	} else if (!strncmp(match, "m:spec1", 7)) {
		snprintf(output, max_size, "%s", b_spec1);
	} else if (!strncmp(match, "m:arg1", 5)) {
		snprintf(output, max_size, "%s", MACRO_ARG1);
	} else if (!strncmp(match, "m:arg2", 5)) {
		snprintf(output, max_size, "%s", MACRO_ARG2);
	} else if (!strncmp(match, "m:arg3", 5)) {
		snprintf(output, max_size, "%s", MACRO_ARG3);
	} else if (!strncmp(match, "m:q:", 4)) {
		return rtv_q(&match[4], output, max_size);
	} else {
		return 1;
	}
	return 0;
}

int rtv_q(void *query, char *output, size_t max_size) {
	mda md_s = { 0 };

	md_init(&md_s, 2);
	p_md_obj ptr;

	if (split_string(query, 0x40, &md_s) != 2) {
		return 0;
	}

	ptr = md_s.objects;

	char *rtv_l = g_dgetf((char*) ptr->ptr);

	if (!rtv_l) {
		return 0;
	}

	ptr = ptr->next;

	int r = 0;
	char *rtv_q = (char*) ptr->ptr;

	if (!strncmp(rtv_q, "size", 4)) {
		snprintf(output, max_size, "%llu", (ulint64_t) get_file_size(rtv_l));
	} else if (!strncmp(rtv_q, "count", 5)) {
		_g_handle hdl = { 0 };
		size_t rtv_ll = strlen(rtv_l);

		g_strncpy(hdl.file, rtv_l,
				rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
		if (determine_datatype(&hdl)) {
			goto end;
		}

		snprintf(output, max_size, "%llu",
				(ulint64_t) get_file_size(rtv_l) / (ulint64_t) hdl.block_sz);
	} else if (!strncmp(rtv_q, "corrupt", 7)) {
		_g_handle hdl = { 0 };
		size_t rtv_ll = strlen(rtv_l);

		g_strncpy(hdl.file, rtv_l,
				rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
		if (determine_datatype(&hdl)) {
			goto end;
		}

		snprintf(output, max_size, "%llu",
				(ulint64_t) get_file_size(rtv_l) % (ulint64_t) hdl.block_sz);
	} else if (!strncmp(rtv_q, "bsize", 5)) {
		_g_handle hdl = { 0 };
		size_t rtv_ll = strlen(rtv_l);

		g_strncpy(hdl.file, rtv_l,
				rtv_ll > max_size - 1 ? max_size - 1 : rtv_ll);
		if (determine_datatype(&hdl)) {
			goto end;
		}

		snprintf(output, max_size, "%u", (uint32_t) hdl.block_sz);
	} else if (!strncmp(rtv_q, "mode", 4)) {
		return g_l_fmode_n(rtv_l, max_size, output);
	} else if (!strncmp(rtv_q, "file", 4)) {
		snprintf(output, max_size, "%s", rtv_l);
	} else if (!strncmp(rtv_q, "read", 4)) {
		snprintf(output, max_size, "%d",
				!access(rtv_l, R_OK) ? 1 : errno == EACCES ? 0 : -1);
	} else if (!strncmp(rtv_q, "write", 5)) {
		snprintf(output, max_size, "%d",
				!access(rtv_l, W_OK) ? 1 : errno == EACCES ? 0 : -1);
	}

	end:

	md_g_free(&md_s);

	return r;
}

int ref_to_val_generic(void *arg, char *match, char *output, size_t max_size) {
	if (!strcmp(match, "exe")) {
		if (self_get_path(output)) {
			snprintf(output, max_size, "%s", "UNKNOWN");
		}
	} else if (!strcmp(match, "glroot")) {
		snprintf(output, max_size, "%s", GLROOT);
	} else if (!strcmp(match, "siteroot")) {
		snprintf(output, max_size, "%s", SITEROOT);
	} else if (!strcmp(match, "siterootn")) {
		snprintf(output, max_size, "%s", SITEROOT_N);
	} else if (!strcmp(match, "ftpdata")) {
		snprintf(output, max_size, "%s", FTPDATA);
	} else if (!strcmp(match, "logfile")) {
		snprintf(output, max_size, "%s", LOGFILE);
	} else if (!strcmp(match, "imdbfile")) {
		snprintf(output, max_size, "%s", IMDBLOG);
	} else if (!strcmp(match, "gamefile")) {
		snprintf(output, max_size, "%s", GAMELOG);
	} else if (!strcmp(match, "tvragefile")) {
		snprintf(output, max_size, "%s", TVLOG);
	} else if (!strcmp(match, "nukestr")) {
		if (NUKESTR) {
			snprintf(output, max_size, NUKESTR, "");
		}
	} else if (!strncmp(match, "procid", 6)) {
		snprintf(output, max_size, "%d", getpid());
	} else if (!strncmp(match, "ipc", 3)) {
		snprintf(output, max_size, "%.8X", (uint32_t) SHM_IPC);
	} else if (!strncmp(match, "spec1", 5)) {
		snprintf(output, max_size, "%s", b_spec1);
	} else if (!strncmp(match, "usroot", 6)) {
		snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
		DEFPATH_USERS);
		remove_repeating_chars(output, 0x2F);
	} else if (!strncmp(match, "logroot", 7)) {
		snprintf(output, max_size, "%s/%s/%s", GLROOT, FTPDATA,
		DEFPATH_LOGS);
		remove_repeating_chars(output, 0x2F);
	} else if (!strncmp(match, "memlimit", 8)) {
		snprintf(output, max_size, "%llu", db_max_size);
	} else if (!strncmp(match, "glconf", 6)) {
		snprintf(output, max_size, "%s", GLCONF);
	} else if (!strncmp(match, "q:", 2)) {
		return rtv_q(&match[2], output, max_size);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_x(void *arg, char *match, char *output, size_t max_size) {
	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	if (arg && !strcmp(match, "size")) {
		snprintf(output, max_size, "%llu", (ulint64_t) get_file_size(arg));
	} else if (arg && !strcmp(match, "mode")) {
		return g_l_fmode_n(arg, max_size, output);
	} else if (arg && !strcmp(match, "arg")) {
		snprintf(output, max_size, "%s", (char*) arg);
	} else if (arg && !strcmp(match, "path")) {
		snprintf(output, max_size, "%s", (char*) arg);
	} else if (arg && !strncmp(match, "c:", 2)) {
		return g_rtval_ex(arg, &match[2], max_size, output,
		F_CFGV_BUILD_FULL_STRING);
	} else {
		return 1;
	}

	return 0;
}

int g_rtval_ex(char *arg, char *match, size_t max_size, char *output,
		uint32_t flags) {
	int r = 1;

	char *buffer = calloc(max_size + 1, 1);
	void *ptr = ref_to_val_get_cfgval(arg, match,
	NULL, flags, buffer, max_size);
	if (ptr && strlen(ptr) < max_size) {
		snprintf(output, max_size, "%s", (char*) ptr);
		r = 0;
	}
	g_free(buffer);
	return r;
}

int g_is_higher(uint64_t s, uint64_t d) {
	if (s > d) {
		return 0;
	}
	return 1;
}

int g_is_lower(uint64_t s, uint64_t d) {
	if (s < d) {
		return 0;
	}
	return 1;
}

int g_is_higher_2(uint64_t s, uint64_t d) {
	return (s > d);
}

int g_is_lower_2(uint64_t s, uint64_t d) {
	return (s < d);
}

int g_is_equal(uint64_t s, uint64_t d) {
	return (s == d);
}

int g_is_not_equal(uint64_t s, uint64_t d) {
	return (s != d);
}

int g_is_higherorequal(uint64_t s, uint64_t d) {
	return (s >= d);
}

int g_is_lowerorequal(uint64_t s, uint64_t d) {
	return (s <= d);
}

int g_is(uint64_t s, uint64_t d) {
	return s != 0;
}

int g_is_not(uint64_t s, uint64_t d) {
	return s == 0;
}

int g_is_higher_f(float s, float d) {
	if (s > d) {
		return 0;
	}
	return 1;
}

int g_is_lower_f(float s, float d) {
	if (s < d) {
		return 0;
	}
	return 1;
}

int g_is_higher_f_2(float s, float d) {
	return (s > d);
}

int g_is_lower_f_2(float s, float d) {
	return (s < d);
}

int g_is_equal_f(float s, float d) {
	return (s == d);
}

int g_is_not_equal_f(float s, float d) {
	return (s != d);
}

int g_is_higherorequal_f(float s, float d) {
	return (s >= d);
}

int g_is_lowerorequal_f(float s, float d) {
	return (s <= d);
}

int g_is_f(float s, float d) {
	return s != 0;
}

int g_is_not_f(float s, float d) {
	return s == 0;
}

int g_oper_and(int s, int d) {
	return (s && d);
}

int g_oper_or(int s, int d) {
	return (s || d);
}

uint64_t g_t8_ptr(void *base, size_t offset) {
	return (uint64_t) *((uint8_t*) (base + offset));
}

uint64_t g_t16_ptr(void *base, size_t offset) {
	return (uint64_t) *((uint16_t*) (base + offset));
}

uint64_t g_t32_ptr(void *base, size_t offset) {
	return (uint64_t) *((uint32_t*) (base + offset));
}

uint64_t g_t64_ptr(void *base, size_t offset) {
	return *((uint64_t*) (base + offset));
}

float g_tf_ptr(void *base, size_t offset) {
	return *((float*) (base + offset));
}

#define MAX_SORT_LOOPS				MAX_uint64_t

#define F_INT_GSORT_LOOP_DID_SORT	(a32 << 1)
#define F_INT_GSORT_DID_SORT		(a32 << 2)

int g_sorti_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2) {
	int (*m_op)(uint64_t s, uint64_t d) = cb1;
	uint64_t (*g_t_ptr_c)(void *base, size_t offset) = cb2;

	p_md_obj ptr, ptr_n;
	int r = 0;
	uint64_t t_b, t_b_n;
	uint32_t ml_f = 0;
	uint64_t ml_i;

	for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++) {
		ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
		ptr = m_ptr->first;
		while (ptr && ptr->next) {
			ptr_n = (p_md_obj) ptr->next;

			t_b = g_t_ptr_c(ptr->ptr, off);
			t_b_n = g_t_ptr_c(ptr_n->ptr, off);

			if (!m_op(t_b, t_b_n)) {
				ptr = md_swap(m_ptr, ptr, ptr_n);
				if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT)) {
					ml_f |= F_INT_GSORT_LOOP_DID_SORT;
				}
			} else {
				ptr = ptr->next;
			}
		}

		if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT)) {
			break;
		}

		if (!(ml_f & F_INT_GSORT_DID_SORT)) {
			ml_f |= F_INT_GSORT_DID_SORT;
		}
	}

	if (!(ml_f & F_INT_GSORT_DID_SORT)) {
		return -1;
	}

	if (!r && (flags & F_GSORT_RESETPOS)) {
		m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
	}

	return r;
}

int g_sortf_exec(pmda m_ptr, size_t off, uint32_t flags, void *cb1, void *cb2) {
	int (*m_op)(float s, float d) = cb1;
	float (*g_t_ptr_c)(void *base, size_t offset) = cb2;

	p_md_obj ptr, ptr_n;
	int r = 0;
	uint64_t ml_i;
	uint32_t ml_f = 0;
	float t_b, t_b_n;

	for (ml_i = 0; ml_i < MAX_SORT_LOOPS; ml_i++) {
		ml_f ^= F_INT_GSORT_LOOP_DID_SORT;
		ptr = m_ptr->first;
		while (ptr && ptr->next) {
			ptr_n = (p_md_obj) ptr->next;

			t_b = g_t_ptr_c(ptr->ptr, off);
			t_b_n = g_t_ptr_c(ptr_n->ptr, off);

			if (!m_op(t_b, t_b_n)) {
				ptr = md_swap(m_ptr, ptr, ptr_n);
				if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT)) {
					ml_f |= F_INT_GSORT_LOOP_DID_SORT;
				}
			} else {
				ptr = ptr->next;
			}
		}

		if (!(ml_f & F_INT_GSORT_LOOP_DID_SORT)) {
			break;
		}

		if (!(ml_f & F_INT_GSORT_DID_SORT)) {
			ml_f |= F_INT_GSORT_DID_SORT;
		}
	}

	if (!(ml_f & F_INT_GSORT_DID_SORT)) {
		return -1;
	}

	if (!r && (flags & F_GSORT_RESETPOS)) {
		m_ptr->pos = m_ptr->r_pos = md_first(m_ptr);
	}

	return r;
}

int g_sort(__g_handle hdl, char *field, uint32_t flags) {
	g_setjmp(0, "g_sort", NULL, NULL);
	void *m_op = NULL, *g_t_ptr_c = NULL;

	int (*g_s_ex)(pmda, size_t, uint32_t, void *, void *) = NULL;
	pmda m_ptr;

	if (!hdl) {
		return 1;
	}

	if (!(hdl->flags & F_GH_FFBUFFER)) {
		m_ptr = &hdl->buffer;
	} else {
		m_ptr = &hdl->w_buffer;
	}

	if (!hdl->g_proc2) {
		return 2;
	}

	switch (flags & F_GSORT_ORDER) {
	case F_GSORT_DESC:
		m_op = g_is_lower;
		break;
	case F_GSORT_ASC:
		m_op = g_is_higher;
		break;
	}

	if (!m_op) {
		return 3;
	}

	/*p_md_obj ptr = md_first(m_ptr);

	 if (!ptr) {
	 return 11;
	 }*/

	size_t vb = 0;

	size_t off = (size_t) hdl->g_proc2(NULL, field, &vb);

	if (!vb) {
		return 13;
	}

	switch (vb) {
	case -1:
		switch (flags & F_GSORT_ORDER) {
		case F_GSORT_DESC:
			m_op = g_is_lower_f;
			break;
		case F_GSORT_ASC:
			m_op = g_is_higher_f;
			break;
		}
		g_t_ptr_c = g_tf_ptr;
		g_s_ex = g_sortf_exec;
		break;
	case 1:
		g_t_ptr_c = g_t8_ptr;
		g_s_ex = g_sorti_exec;
		break;
	case 2:
		g_t_ptr_c = g_t16_ptr;
		g_s_ex = g_sorti_exec;
		break;
	case 4:
		g_t_ptr_c = g_t32_ptr;
		g_s_ex = g_sorti_exec;
		break;
	case 8:
		g_t_ptr_c = g_t64_ptr;
		g_s_ex = g_sorti_exec;
		break;
	default:
		return 14;
		break;
	}

	return g_s_ex(m_ptr, off, flags, m_op, g_t_ptr_c);

}

#define F_GLT_LEFT		(a32 << 1)
#define F_GLT_RIGHT		(a32 << 2)

#define F_GLT_DIRECT	(F_GLT_LEFT|F_GLT_RIGHT)

int g_get_lom_g_t_ptr(__g_handle hdl, char *field, __g_lom lom, uint32_t flags) {
	g_setjmp(0, "g_get_lom_g_t_ptr", NULL, NULL);

	size_t vb = 0;

	size_t off = (size_t) hdl->g_proc2(NULL, field, &vb);

	if (!vb) {
		errno = 0;
		uint32_t t_f = 0;

		if (!(lom->flags & F_LOM_TYPES)) {
			if (s_string(field, ".", 0)) {
				lom->flags |= F_LOM_FLOAT;
			} else {
				lom->flags |= F_LOM_INT;
			}
		}

		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			switch (lom->flags & F_LOM_TYPES) {
			case F_LOM_INT:
				lom->t_left = (uint64_t) strtol(field, NULL, 10);
				break;
			case F_LOM_FLOAT:
				lom->tf_left = (float) strtof(field, NULL);
				break;
			}
			t_f |= F_LOM_LVAR_KNOWN;
			break;
		case F_GLT_RIGHT:
			switch (lom->flags & F_LOM_TYPES) {
			case F_LOM_INT:
				lom->t_right = (uint64_t) strtol(field, NULL, 10);
				break;
			case F_LOM_FLOAT:
				lom->tf_right = (float) strtof(field, NULL);
				break;
			}
			t_f |= F_LOM_RVAR_KNOWN;
			break;
		}

		if (errno == ERANGE) {
			return 4;
		}

		lom->flags |= t_f;

		return 0;
	}

	if (off > hdl->block_sz) {
		return 601;
	}

	switch (vb) {
	case -1:
		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			lom->g_tf_ptr_left = g_tf_ptr;
			break;
		case F_GLT_RIGHT:
			lom->g_tf_ptr_right = g_tf_ptr;
			break;
		}
		lom->flags |= F_LOM_FLOAT;
		break;
	case 1:
		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			lom->g_t_ptr_left = g_t8_ptr;
			break;
		case F_GLT_RIGHT:
			lom->g_t_ptr_right = g_t8_ptr;
			break;
		}
		lom->flags |= F_LOM_INT;
		break;
	case 2:
		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			lom->g_t_ptr_left = g_t16_ptr;
			break;
		case F_GLT_RIGHT:
			lom->g_t_ptr_right = g_t16_ptr;
			break;
		}
		lom->flags |= F_LOM_INT;
		break;
	case 4:
		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			lom->g_t_ptr_left = g_t32_ptr;
			break;
		case F_GLT_RIGHT:
			lom->g_t_ptr_right = g_t32_ptr;
			break;
		}
		lom->flags |= F_LOM_INT;
		break;
	case 8:
		switch (flags & F_GLT_DIRECT) {
		case F_GLT_LEFT:
			lom->g_t_ptr_left = g_t64_ptr;
			break;
		case F_GLT_RIGHT:
			lom->g_t_ptr_right = g_t64_ptr;
			break;
		}
		lom->flags |= F_LOM_INT;
		break;
	default:
		return 608;
		break;
	}

	switch (flags & F_GLT_DIRECT) {
	case F_GLT_LEFT:
		lom->t_l_off = off;
		break;
	case F_GLT_RIGHT:
		lom->t_r_off = off;
		break;
	}

	return 0;

}

int g_build_lom_packet(__g_handle hdl, char *left, char *right, char *comp,
		size_t comp_l, char *oper, size_t oper_l, __g_match match, __g_lom *ret,
		uint32_t flags) {
	g_setjmp(0, "g_build_lom_packet", NULL, NULL);
	int rt = 0;
	md_init(&match->lom, 16);

	__g_lom lom = (__g_lom ) md_alloc(&match->lom, sizeof(_g_lom));

	if (!lom) {
		rt = 1;
		goto end;
	}

	int r = 0;

	if ((r = g_get_lom_g_t_ptr(hdl, left, lom, F_GLT_LEFT))) {
		rt = r;
		goto end;
	}

	char *r_ptr = right;

	if (!r_ptr) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			r_ptr = "1.0";
			break;
		case F_LOM_INT:
			r_ptr = "1";
			break;
		}
	}

	if ((r = g_get_lom_g_t_ptr(hdl, r_ptr, lom, F_GLT_RIGHT))) {
		rt = 4;
		goto end;
	}

	if ((lom->flags & F_LOM_FLOAT) && (lom->flags & F_LOM_INT)) {
		lom->flags ^= F_LOM_INT;
	}

	if (!(lom->flags & F_LOM_TYPES)) {
		rt = 6;
		goto end;
	}

	if (!comp) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is;
			break;
		}

	} else if (comp_l == 2 && !strncmp(comp, "==", 1)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_equal_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_equal;
			break;
		}

	} else if (comp_l == 1 && !strncmp(comp, "=", 1)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_equal_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_equal;
			break;
		}
	} else if (comp_l == 1 && !strncmp(comp, "<", 1)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_lower_f_2;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_lower_2;
			break;
		}
	} else if (comp_l == 1 && !strncmp(comp, ">", 1)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_higher_f_2;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_higher_2;
			break;
		}
	} else if (comp_l == 2 && !strncmp(comp, "!=", 2)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_higherorequal_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_not_equal;
			break;
		}
	} else if (comp_l == 2 && !strncmp(comp, ">=", 2)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_higherorequal_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_higherorequal;
			break;
		}
	} else if (comp_l == 2 && !strncmp(comp, "<=", 2)) {
		switch (lom->flags & F_LOM_TYPES) {
		case F_LOM_FLOAT:
			lom->g_fcomp_ptr = g_is_lowerorequal_f;
			break;
		case F_LOM_INT:
			lom->g_icomp_ptr = g_is_lowerorequal;
			break;
		}

	} else {
		rt = 11;
		goto end;
	}

	if (oper) {
		if (oper_l == 2 && !strncmp(oper, "&&", 2)) {
			lom->g_oper_ptr = g_oper_and;
		} else if (oper_l == 2 && !strncmp(oper, "||", 2)) {
			lom->g_oper_ptr = g_oper_or;
		} else {
			lom->g_oper_ptr = g_oper_and;
		}
	}

	if (ret) {
		*ret = lom;
	}

	end:

	if (rt) {
		md_unlink(&match->lom, match->lom.pos);
	} else {
		match->flags |= F_GM_ISLOM;
		match->flags |= flags;
		if (match->flags & F_GM_IMATCH) {
			match->match_i_m = G_NOMATCH;
		} else {
			match->match_i_m = G_MATCH;
		}
	}

	return rt;
}

__g_match g_global_register_match(void) {
	md_init(&_match_rr, 32);

	if (_match_rr.offset >= 8192) {
		return NULL;
	}

	return (__g_match ) md_alloc(&_match_rr, sizeof(_g_match));

}

int is_opr(char in) {
	if (in == 0x21 || in == 0x26 || in == 0x3D || in == 0x3C || in == 0x3E
			|| in == 0x7C) {
		return 0;
	}
	return 1;
}

int get_opr(char *in) {
	if (!strncmp(in, "&&", 2) || !strncmp(in, "||", 2)) {
		return 1;
	} else {
		return 0;
	}
}

int md_copy(pmda source, pmda dest, size_t block_sz) {
	if (!source || !dest) {
		return 1;
	}

	if (dest->count) {
		return 2;
	}
	int ret = 0;
	p_md_obj ptr = md_first(source);
	void *d_ptr;

	md_init(dest, source->count);

	while (ptr) {
		d_ptr = md_alloc(dest, block_sz);
		if (!d_ptr) {
			ret = 10;
			break;
		}
		g_memcpy(d_ptr, ptr->ptr, block_sz);
		ptr = ptr->next;
	}

	if (ret) {
		md_g_free(dest);
	}

	if (source->offset != dest->offset) {
		return 3;
	}

	return 0;
}

int g_build_argv_c(__g_handle hdl) {
	md_init(&hdl->exec_args.ac_ref, hdl->exec_args.argc);

	hdl->exec_args.argv_c = calloc(amax / sizeof(char*), sizeof(char**));
	int i, *t;
	char *ptr;
	for (i = 0; i < hdl->exec_args.argc && hdl->exec_args.argv[i]; i++) {
		ptr = strchr(hdl->exec_args.argv[i], 0x7B);
		if (ptr) {
			hdl->exec_args.argv_c[i] = (char*) calloc(8192, 1);
			size_t t_l = strlen(hdl->exec_args.argv[i]);
			g_strncpy(hdl->exec_args.argv_c[i], hdl->exec_args.argv[i],
					t_l > 8191 ? 8191 : t_l);
			t = md_alloc(&hdl->exec_args.ac_ref, sizeof(int));
			*t = i;
		} else {
			hdl->exec_args.argv_c[i] = hdl->exec_args.argv[i];
		}
	}

	if (!i) {
		return 1;
	}

	return 0;
}

int g_proc_mr(__g_handle hdl) {
	g_setjmp(0, "g_proc_mr", NULL, NULL);
	int r;

	if (!(gfl & F_OPT_PROCREV)) {
		hdl->j_offset = 1;
		if (hdl->buffer.count) {
			hdl->buffer.r_pos = md_first(&hdl->buffer);
		}
	} else {
		if (hdl->buffer.count) {
			hdl->buffer.r_pos = md_last(&hdl->buffer);
		}
		hdl->j_offset = 2;
	}

	if (!(hdl->flags & F_GH_HASMATCHES)) {
		if ((r = md_copy(&_match_rr, &hdl->_match_rr, sizeof(_g_match)))) {
			print_str("ERROR: %s: could not copy matches to handle\n",
					hdl->file);
			return 2000;
		}
		if (hdl->_match_rr.offset) {
			if ((gfl & F_OPT_VERBOSE4)) {
				print_str("NOTICE: %s: commit %llu matches to handle\n",
						hdl->file, (ulint64_t) hdl->_match_rr.offset);
			}
			hdl->flags |= F_GH_HASMATCHES;
		}

	}

	if (gfl & F_OPT_HASMAXHIT) {
		hdl->max_hits = max_hits;
		hdl->flags |= F_GH_HASMAXHIT;
	}

	if (gfl & F_OPT_HASMAXRES) {
		hdl->max_results = max_results;
		hdl->flags |= F_GH_HASMAXRES;
	}

	if ((gfl & F_OPT_HAS_G_LOM)) {
		if ((r = g_load_lom(hdl))) {
			return r;
		}
	}

	if ((exec_v || exec_str)) {
		if (!(hdl->flags & F_GH_HASEXC)) {
			hdl->exec_args.exc = exc;

			if (!hdl->exec_args.exc) {
				print_str(
						"ERROR: %s: no exec call pointer (this is probably a bug)\n",
						hdl->file);
				return 2002;
			}
			hdl->flags |= F_GH_HASEXC;
		}
		if (exec_v && !hdl->exec_args.argv) {
			hdl->exec_args.argv = exec_v;
			hdl->exec_args.argc = exec_vc;

			if (!hdl->exec_args.argc) {
				print_str("ERROR: %s: no exec arguments\n", hdl->file);
				return 2001;
			}

			if (g_build_argv_c(hdl)) {
				print_str("ERROR: %s: failed building exec arguments\n",
						hdl->file);
				return 2005;
			}

			int r;

			if ((r = find_absolute_path(hdl->exec_args.argv_c[0],
					hdl->exec_args.exec_v_path))) {
				if (gfl & F_OPT_VERBOSE2) {
					print_str(
							"WARNING: %s: [%d]: exec unable to get absolute path\n",
							hdl->file, r);
				}
				snprintf(hdl->exec_args.exec_v_path, PATH_MAX, "%s",
						hdl->exec_args.argv_c[0]);
			}
		}
	}

	return 0;
}

int g_load_lom(__g_handle hdl) {
	g_setjmp(0, "g_load_lom", NULL, NULL);

	if ((hdl->flags & F_GH_HASLOM)) {
		return 0;
	}

	int rt = 0;

	p_md_obj ptr = md_first(&hdl->_match_rr);
	__g_match _m_ptr;
	int r, ret, c = 0;
	while (ptr) {
		_m_ptr = (__g_match) ptr->ptr;
		if ( _m_ptr->flags & F_GM_ISLOM ) {
			if ((r = g_process_lom_string(hdl, _m_ptr->data, _m_ptr, &ret,
									_m_ptr->flags))) {
				print_str("ERROR: %s: [%d] [%d]: could not load LOM string\n",
						hdl->file, r, ret);
				rt = 1;
				break;
			}
			c++;
		}
		ptr = ptr->next;
	}

	if (!rt) {
		hdl->flags |= F_GH_HASLOM;
		if (gfl & F_OPT_VERBOSE) {
			print_str("NOTICE: %s: loaded %d LOM matches\n", hdl->file, c);
		}
	} else {
		print_str("ERROR: %s: [%d] LOM specified, but none was loaded\n",
				hdl->file, rt);
	}

	return rt;
}

#define MAX_LOM_VLEN 	254

int g_process_lom_string(__g_handle hdl, char *string, __g_match _gm, int *ret,
		uint32_t flags) {
	g_setjmp(0, "g_process_lom_string", NULL, NULL);
	char *ptr = string, *w_ptr;
	char left[MAX_LOM_VLEN + 1], right[MAX_LOM_VLEN + 1], comp[4], oper[4];
	int i;
	while (ptr[0]) {
		bzero(left, MAX_LOM_VLEN + 1);
		bzero(right, MAX_LOM_VLEN + 1);
		bzero(comp, 4);
		bzero(oper, 4);
		while (is_ascii_alphanumeric((uint8_t) ptr[0])) {
			ptr++;
		}

		w_ptr = ptr;
		i = 0;
		while (!is_ascii_alphanumeric((uint8_t) ptr[0]) || ptr[0] == 0x2E) {
			i++;
			ptr++;
		}

		if (i > MAX_LOM_VLEN) {
			return 1;
		}

		g_strncpy(left, w_ptr, i);

		while (ptr[0] == 0x20) {
			ptr++;
		}

		i = 0;

		w_ptr = ptr;
		while (!is_opr(ptr[0])) {
			ptr++;
			i++;
		}

		if (i > 2) {
			return 2;
		}

		if (get_opr(w_ptr) == 1) {
			g_strncpy(oper, w_ptr, i);
			goto end_loop;
		} else {
			g_strncpy(comp, w_ptr, i);
		}

		while (ptr[0] == 0x20) {
			ptr++;
		}

		w_ptr = ptr;
		i = 0;
		while (!is_ascii_alphanumeric((uint8_t) ptr[0]) || ptr[0] == 0x2E) {
			i++;
			ptr++;
		}

		if (i > MAX_LOM_VLEN) {
			return 3;
		}

		g_strncpy(right, w_ptr, i);

		while (ptr[0] == 0x20) {
			ptr++;
		}

		i = 0;

		w_ptr = ptr;
		while (!is_opr(ptr[0])) {
			ptr++;
			i++;
		}

		if (i > 2) {
			return 4;
		}

		if (get_opr(w_ptr) == 1) {
			g_strncpy(oper, w_ptr, i);
		}

		end_loop: ;

		if (!strlen(left)) {
			return 5;
		}

		char *r_ptr = (char*) right, *c_ptr = (char*) comp, *o_ptr =
				(char*) oper;

		if (!strlen(r_ptr)) {
			r_ptr = NULL;
		}

		size_t comp_l = strlen(comp), oper_l = strlen(oper);

		if (!strlen(c_ptr)) {
			c_ptr = NULL;
		}

		if (!strlen(o_ptr)) {
			o_ptr = NULL;
		}

		*ret = g_build_lom_packet(hdl, left, r_ptr, c_ptr, comp_l, o_ptr,
				oper_l, _gm, NULL, flags);

		if (*ret) {
			return 6;
		}

	}

	return 0;
}

void *ref_to_val_ptr_dirlog(void *arg, char *match, size_t *output) {
	struct dirlog *data = (struct dirlog *) arg;

	if (!strcmp(match, "time")) {
		*output = sizeof(data->uptime);
		return &data->uptime;
	} else if (!strcmp(match, "user")) {
		*output = sizeof(data->uploader);
		return &data->uploader;
	} else if (!strcmp(match, "group")) {
		*output = sizeof(data->group);
		return &data->group;
	} else if (!strcmp(match, "files")) {
		*output = sizeof(data->files);
		return &data->files;
	} else if (!strcmp(match, "size")) {
		*output = sizeof(data->bytes);
		return &data->bytes;
	} else if (!strcmp(match, "status")) {
		*output = sizeof(data->status);
		return &data->status;
	}
	return NULL;
}

void *ref_to_val_ptr_nukelog(void *arg, char *match, size_t *output) {

	struct nukelog *data = (struct nukelog *) arg;

	if (!strcmp(match, "time")) {
		*output = sizeof(data->nuketime);
		return &data->nuketime;
	} else if (!strcmp(match, "size")) {
		*output = -1;
		return &data->bytes;
	} else if (!strcmp(match, "mult")) {
		*output = sizeof(data->mult);
		return &data->mult;
	} else if (!strcmp(match, "status")) {
		*output = sizeof(data->status);
		return &data->status;
	}
	return NULL;
}

void *ref_to_val_ptr_dupefile(void *arg, char *match, size_t *output) {

	struct dupefile *data = (struct dupefile *) arg;

	if (!strcmp(match, "time")) {
		*output = sizeof(data->timeup);
		return &data->timeup;
	}
	return NULL;
}

void *ref_to_val_ptr_lastonlog(void *arg, char *match, size_t *output) {
	struct lastonlog *data = (struct lastonlog *) arg;

	if (!strcmp(match, "logon")) {
		*output = sizeof(data->logon);
		return &data->logon;
	} else if (!strcmp(match, "logoff")) {
		*output = sizeof(data->logoff);
		return &data->logoff;
	} else if (!strcmp(match, "download")) {
		*output = sizeof(data->download);
		return &data->download;
	} else if (!strcmp(match, "upload")) {
		*output = sizeof(data->upload);
		return &data->upload;
	}
	return NULL;
}

void *ref_to_val_ptr_oneliners(void *arg, char *match, size_t *output) {

	struct oneliner *data = (struct oneliner *) arg;

	if (!strcmp(match, "timestamp")) {
		*output = sizeof(data->timestamp);
		return &data->timestamp;
	}
	return NULL;
}

void *ref_to_val_ptr_online(void *arg, char *match, size_t *output) {

	struct ONLINE *data = (struct ONLINE *) arg;

	if (!strcmp(match, "btxfer")) {
		*output = sizeof(data->bytes_txfer);
		return &data->bytes_txfer;
	} else if (!strcmp(match, "bxfer")) {
		*output = sizeof(data->bytes_xfer);
		return &data->bytes_xfer;
	} else if (!strcmp(match, "group")) {
		*output = sizeof(data->groupid);
		return &data->groupid;
	} else if (!strcmp(match, "PID")) {
		*output = sizeof(data->procid);
		return &data->procid;
	} else if (!strcmp(match, "ssl")) {
		*output = sizeof(data->ssl_flag);
		return &data->ssl_flag;
	} else if (!strcmp(match, "time")) {
		*output = sizeof(data->login_time);
		return &data->login_time;
	}

	return NULL;
}

void *ref_to_val_ptr_imdb(void *arg, char *match, size_t *output) {

	__d_imdb data = (__d_imdb) arg;

	if (!strcmp(match, "score")) {
		*output = -1;
		return &data->rating;
	} else if (!strcmp(match, "released")) {
		*output = sizeof(data->released);
		return &data->released;
	} else if (!strcmp(match, "runtime")) {
		*output = sizeof(data->runtime);
		return &data->runtime;
	} else if (!strcmp(match, "time")) {
		*output = sizeof(data->timestamp);
		return &data->timestamp;
	} else if (!strcmp(match, "votes")) {
		*output = sizeof(data->votes);
		return &data->votes;
	}

	return NULL;
}

void *ref_to_val_ptr_game(void *arg, char *match, size_t *output) {

	__d_game data = (__d_game) arg;

	if (!strcmp(match, "score")) {
		*output = -1;
		return &data->rating;
	} else if (!strcmp(match, "time")) {
		*output = sizeof(data->timestamp);
		return &data->timestamp;
	}

	return NULL;
}

void *ref_to_val_ptr_tv(void *arg, char *match, size_t *output) {

	__d_tvrage data = (__d_tvrage) arg;

	if (!strcmp(match, "started")) {
		*output = sizeof(data->started);
		return &data->started;
	} else if (!strcmp(match, "runtime")) {
		*output = sizeof(data->runtime);
		return &data->runtime;
	} else if (!strcmp(match, "time")) {
		*output = sizeof(data->timestamp);
		return &data->timestamp;
	} else if (!strcmp(match, "ended")) {
		*output = sizeof(data->ended);
		return &data->ended;
	} else if (!strcmp(match, "showid")) {
		*output = sizeof(data->showid);
		return &data->showid;
	} else if (!strcmp(match, "seasons")) {
		*output = sizeof(data->seasons);
		return &data->seasons;
	}

	return NULL;
}

void *ref_to_val_ptr_gen1(void *arg, char *match, size_t *output) {
	__d_generic_s2044 data = (__d_generic_s2044) arg;

	if (!strcmp(match, "i32")) {
		*output = sizeof(data->i32);
		return &data->i32;
	}

	return NULL;
}

void *ref_to_val_ptr_dummy(void *arg, char *match, size_t *output) {
	return NULL;
}

int ref_to_val_dirlog(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct dirlog *data = (struct dirlog *) arg;

	if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->dirname);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname), *base = basename(s_buffer);
		snprintf(output, max_size, "%s", base);
		g_free(s_buffer);
	} else if (!strcmp(match, "user")) {
		snprintf(output, max_size, "%d", data->uploader);
	} else if (!strcmp(match, "group")) {
		snprintf(output, max_size, "%d", data->group);
	} else if (!strcmp(match, "files")) {
		snprintf(output, max_size, "%u", (uint32_t) data->files);
	} else if (!strcmp(match, "size")) {
		snprintf(output, max_size, "%llu", (ulint64_t) data->bytes);
	} else if (!strcmp(match, "status")) {
		snprintf(output, max_size, "%d", (int) data->status);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->uptime);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->dirname, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_nukelog(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct nukelog *data = (struct nukelog *) arg;

	if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->dirname);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname), *base = basename(s_buffer);
		snprintf(output, max_size, "%s", base);
		g_free(s_buffer);
	} else if (!strcmp(match, "nuker")) {
		snprintf(output, max_size, "%s", data->nuker);
	} else if (!strcmp(match, "nukee")) {
		snprintf(output, max_size, "%s", data->nukee);
	} else if (!strcmp(match, "unnuker")) {
		snprintf(output, max_size, "%s", data->unnuker);
	} else if (!strcmp(match, "reason")) {
		snprintf(output, max_size, "%s", data->reason);
	} else if (!strcmp(match, "size")) {
		snprintf(output, max_size, "%.2f", (float) data->bytes);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->nuketime);
	} else if (!strcmp(match, "status")) {
		snprintf(output, max_size, "%d", (int) data->status);
	} else if (!strcmp(match, "mult")) {
		snprintf(output, max_size, "%d", (int) data->mult);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->dirname, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_dupefile(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct dupefile *data = (struct dupefile *) arg;

	if (!strcmp(match, "file")) {
		g_strncpy(output, data->filename, sizeof(data->filename));
	} else if (!strcmp(match, "user")) {
		snprintf(output, max_size, "%s", data->uploader);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->timeup);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->filename, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_lastonlog(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct lastonlog *data = (struct lastonlog *) arg;

	if (!strcmp(match, "user")) {
		snprintf(output, max_size, "%s", data->uname);
	} else if (!strcmp(match, "group")) {
		snprintf(output, max_size, "%s", data->gname);
	} else if (!strcmp(match, "stats")) {
		snprintf(output, max_size, "%s", data->stats);
	} else if (!strcmp(match, "tag")) {
		snprintf(output, max_size, "%s", data->tagline);
	} else if (!strcmp(match, "logon")) {
		snprintf(output, max_size, "%u", (uint32_t) data->logon);
	} else if (!strcmp(match, "logoff")) {
		snprintf(output, max_size, "%u", (uint32_t) data->logoff);
	} else if (!strcmp(match, "upload")) {
		snprintf(output, max_size, "%u", (uint32_t) data->upload);
	} else if (!strcmp(match, "download")) {
		snprintf(output, max_size, "%u", (uint32_t) data->download);
	} else if (!is_char_uppercase(match[0])) {
		char *buffer = calloc(max_size + 1, 1);
		void *ptr = ref_to_val_get_cfgval(data->uname, match,
		DEFPATH_USERS,
		F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			snprintf(output, max_size, "%s", (char*) ptr);
			g_free(buffer);
			return 0;
		}

		ptr = ref_to_val_get_cfgval(data->gname, match, DEFPATH_GROUPS,
		F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			snprintf(output, max_size, "%s", (char*) ptr);
			g_free(buffer);
			return 0;
		}
		g_free(buffer);
		return 1;
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_oneliners(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct oneliner *data = (struct oneliner *) arg;

	if (!strcmp(match, "user")) {
		snprintf(output, max_size, "%s", data->uname);
	} else if (!strcmp(match, "group")) {
		snprintf(output, max_size, "%s", data->gname);
	} else if (!strcmp(match, "tag")) {
		snprintf(output, max_size, "%s", data->tagline);
	} else if (!strcmp(match, "msg")) {
		snprintf(output, max_size, "%s", data->message);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->timestamp);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_online(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	struct ONLINE *data = (struct ONLINE *) arg;

	if (!strcmp(match, "user")) {
		snprintf(output, max_size, "%s", data->username);
	} else if (!strcmp(match, "tag")) {
		snprintf(output, max_size, "%s", data->tagline);
	} else if (!strcmp(match, "status")) {
		snprintf(output, max_size, "%s", data->status);
	} else if (!strcmp(match, "host")) {
		snprintf(output, max_size, "%s", data->host);
	} else if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->currentdir);
	} else if (!strcmp(match, "ssl")) {
		snprintf(output, max_size, "%u", (uint32_t) data->ssl_flag);
	} else if (!strcmp(match, "group")) {
		snprintf(output, max_size, "%u", (uint32_t) data->groupid);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->login_time);
	} else if (!strcmp(match, "lupdtime")) {
		snprintf(output, max_size, "%u", (uint32_t) data->tstart.tv_sec);
	} else if (!strcmp(match, "lxfertime")) {
		snprintf(output, max_size, "%u", (uint32_t) data->txfer.tv_sec);
	} else if (!strcmp(match, "bxfer")) {
		snprintf(output, max_size, "%llu", (ulint64_t) data->bytes_xfer);
	} else if (!strcmp(match, "btxfer")) {
		snprintf(output, max_size, "%llu", (ulint64_t) data->bytes_txfer);
	} else if (!strcmp(match, "pid")) {
		snprintf(output, max_size, "%u", (uint32_t) data->procid);
	} else if (!strcmp(match, "rate")) {
		int32_t tdiff = (int32_t) time(NULL) - data->tstart.tv_sec;
		uint32_t kbps = 0;

		if (tdiff > 0 && data->bytes_xfer > 0) {
			kbps = data->bytes_xfer / tdiff;
		}
		snprintf(output, max_size, "%u", kbps);
	} else if (!is_char_uppercase(match[0])) {
		char *buffer = calloc(max_size + 1, 1);
		void *ptr = ref_to_val_get_cfgval(data->username, match,
		DEFPATH_USERS,
		F_CFGV_BUILD_FULL_STRING | F_CFGV_BUILD_DATA_PATH, buffer, max_size);
		if (ptr && strlen(ptr) < max_size) {
			snprintf(output, max_size, "%s", (char*) ptr);
		}
		g_free(buffer);
		return 0;
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_imdb(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	__d_imdb data = (__d_imdb) arg;

	if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->dirname);
	} else if (!strcmp(match, "basedir")) {
		char *s_buffer = strdup(data->dirname), *base = basename(s_buffer);
		snprintf(output, max_size, "%s", base);
		g_free(s_buffer);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", (uint32_t) data->timestamp);
	} else if (!strcmp(match, "imdbid")) {
		snprintf(output, max_size, "%s", data->imdb_id);
	} else if (!strcmp(match, "score")) {
		snprintf(output, max_size, "%.1f", data->rating);
	} else if (!strcmp(match, "votes")) {
		snprintf(output, max_size, "%u", (uint32_t) data->votes);
	} else if (!strcmp(match, "genres")) {
		snprintf(output, max_size, "%s", data->genres);
	} else if (!strcmp(match, "rated")) {
		snprintf(output, max_size, "%s", data->rated);
	} else if (!strcmp(match, "title")) {
		snprintf(output, max_size, "%s", data->title);
	} else if (!strcmp(match, "director")) {
		snprintf(output, max_size, "%s", data->director);
	} else if (!strcmp(match, "actors")) {
		snprintf(output, max_size, "%s", data->actors);
	} else if (!strcmp(match, "runtime")) {
		snprintf(output, max_size, "%u", data->runtime);
	} else if (!strcmp(match, "released")) {
		snprintf(output, max_size, "%u", data->released);
	} else if (!strcmp(match, "year")) {
		snprintf(output, max_size, "%s", data->year);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->dirname, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_game(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	__d_game data = (__d_game) arg;

	if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->dirname);
	} else if (!strcmp(match, "score")) {
		snprintf(output, max_size, "%.1f", data->rating);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", data->timestamp);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->dirname, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_tv(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	__d_tvrage data = (__d_tvrage) arg;

	if (!strcmp(match, "dir")) {
		snprintf(output, max_size, "%s", data->dirname);
	} else if (!strcmp(match, "time")) {
		snprintf(output, max_size, "%u", data->timestamp);
	} else if (!strcmp(match, "airday")) {
		snprintf(output, max_size, "%s", data->airday);
	} else if (!strcmp(match, "airtime")) {
		snprintf(output, max_size, "%s", data->airtime);
	} else if (!strcmp(match, "country")) {
		snprintf(output, max_size, "%s", data->country);
	} else if (!strcmp(match, "link")) {
		snprintf(output, max_size, "%s", data->link);
	} else if (!strcmp(match, "name")) {
		snprintf(output, max_size, "%s", data->name);
	} else if (!strcmp(match, "status")) {
		snprintf(output, max_size, "%s", data->status);
	} else if (!strcmp(match, "ended")) {
		snprintf(output, max_size, "%u", data->ended);
	} else if (!strcmp(match, "started")) {
		snprintf(output, max_size, "%u", data->started);
	} else if (!strcmp(match, "seasons")) {
		snprintf(output, max_size, "%u", data->seasons);
	} else if (!strcmp(match, "showid")) {
		snprintf(output, max_size, "%u", data->showid);
	} else if (!strcmp(match, "runtime")) {
		snprintf(output, max_size, "%u", data->runtime);
	} else if (!strcmp(match, "class")) {
		snprintf(output, max_size, "%s", data->class);
	} else if (!strcmp(match, "genres")) {
		snprintf(output, max_size, "%s", data->genres);
	} else if (!strcmp(match, "mode")) {
		return g_l_fmode(data->dirname, max_size, output);
	} else {
		return 1;
	}
	return 0;
}

int ref_to_val_gen1(void *arg, char *match, char *output, size_t max_size) {
	if (!output) {
		return 2;
	}

	bzero(output, max_size);

	if (!ref_to_val_generic(NULL, match, output, max_size)) {
		return 0;
	}

	__d_generic_s2044 data = (__d_generic_s2044) arg;

	if (!strcmp(match, "ge1")) {
		snprintf(output, max_size, "%s", data->s_1);
	} else if (!strcmp(match, "ge2")) {
		snprintf(output, max_size, "%s", data->s_2);
	} else if (!strcmp(match, "ge3")) {
		snprintf(output, max_size, "%s", data->s_3);
	} else if (!strcmp(match, "ge4")) {
		snprintf(output, max_size, "%s", data->s_4);
	} else if (!strcmp(match, "ge5")) {
		snprintf(output, max_size, "%s", data->s_5);
	} else if (!strcmp(match, "ge6")) {
		snprintf(output, max_size, "%s", data->s_6);
	} else if (!strcmp(match, "ge7")) {
		snprintf(output, max_size, "%s", data->s_7);
	} else if (!strcmp(match, "ge8")) {
		snprintf(output, max_size, "%s", data->s_8);
	} else if (!strcmp(match, "i32")) {
		snprintf(output, max_size, "%u", data->i32);
	} else {
		return 1;
	}
	return 0;
}

int process_exec_args(void *data, __g_handle hdl) {
	g_setjmp(0, "process_exec_args", NULL, NULL);

	p_md_obj ptr = md_first(&hdl->exec_args.ac_ref);

	int *t;
	while (ptr) {

		t = (int*) ptr->ptr;
		//bzero(hdl->exec_args.argv_c[*t], 8192);

		if (process_exec_string(hdl->exec_args.argv[*t],
				hdl->exec_args.argv_c[*t], 8191, (void*) hdl->g_proc1, data)) {
			bzero(hdl->exec_args.argv_c[*t], 8192);
		}

		ptr = ptr->next;
	}

	return 0;
}

int process_exec_string(char *input, char *output, size_t max_size,
		void *callback, void *data) {

	int (*call)(void *, char *, char *, size_t) = callback;

	size_t blen = strlen(input);

	if (!blen || blen > MAX_EXEC_STR) {
		return 1;
	}

	size_t b_l_1;
	char buffer[255] = { 0 }, buffer2[MAX_VAR_LEN] = { 0 };
	int i, i2, pi, r, f;

	for (i = 0, pi = 0; i < blen; i++, pi++) {
		if (input[i] == 0x7B) {
			for (i2 = 0, i++, r = 0, f = 0; i < blen && i2 < 255; i++, i2++) {
				if (input[i] == 0x7D) {
					buffer[i2] = 0x0;
					if (!i2 || strlen(buffer) > MAX_VAR_LEN
							|| (r = call(data, buffer, buffer2, MAX_VAR_LEN))) {
						if (r) {
							b_l_1 = strlen(buffer);
							snprintf(&output[pi],
							MAX_EXEC_STR - pi - 2, "%c%s%c", 0x7B, buffer,
									0x7D);

							pi += b_l_1 + 2;
						}
						f |= 0x1;
						break;
					}
					b_l_1 = strlen(buffer2);
					g_memcpy(&output[pi], buffer2, b_l_1);

					pi += b_l_1;
					f |= 0x1;
					break;
				}
				buffer[i2] = input[i];
			}

			if ((f & 0x1)) {
				pi -= 1;
				continue;
			}
		}
		output[pi] = input[i];
	}

	output[pi] = 0x0;

	return 0;
}

void child_sig_handler(int signal, siginfo_t * si, void *p) {
	switch (si->si_code) {
	case CLD_KILLED:
		print_str(
				"NOTICE: Child process caught SIGINT (hit CTRL^C again to quit)\n");
		usleep(1000000);
		break;
	case CLD_EXITED:
		break;
	default:
		if (gfl & F_OPT_VERBOSE3) {
			print_str("NOTICE: Child caught signal: %d \n", si->si_code);
		}
		break;
	}
}

#define SIG_BREAK_TIMEOUT_NS (useconds_t)1000000.0

void sig_handler(int signal) {
	switch (signal) {
	case SIGTERM:
		print_str("NOTICE: Caught SIGTERM, terminating gracefully.\n");
		gfl |= F_OPT_KILL_GLOBAL;
		break;
	case SIGINT:
		if (gfl & F_OPT_KILL_GLOBAL) {
			print_str("NOTICE: Caught SIGINT twice, terminating..\n");
			exit(0);
		} else {
			print_str(
					"NOTICE: Caught SIGINT, quitting (hit CTRL^C again to terminate by force)\n");
			gfl |= F_OPT_KILL_GLOBAL;
		}
		usleep(SIG_BREAK_TIMEOUT_NS);
		break;
	default:
		usleep(SIG_BREAK_TIMEOUT_NS);
		print_str("NOTICE: Caught signal %d\n", signal);
		break;
	}
}

int g_shmap_data(__g_handle hdl, key_t ipc) {
	g_setjmp(0, "g_shmap_data", NULL, NULL);
	if (hdl->shmid) {
		return 1001;
	}

	if ((hdl->shmid = shmget(ipc, 0, 0)) == -1) {
		return 1002;
	}

	if ((hdl->data = shmat(hdl->shmid, NULL, SHM_RDONLY)) == (void*) -1) {
		hdl->data = NULL;
		return 1003;
	}

	if (shmctl(hdl->shmid, IPC_STAT, &hdl->ipcbuf) == -1) {
		return 1004;
	}

	if (!hdl->ipcbuf.shm_segsz) {
		return 1005;
	}

	hdl->total_sz = (off_t) hdl->ipcbuf.shm_segsz;

	return 0;
}

void *shmap(key_t ipc, struct shmid_ds *ipcret, size_t size, uint32_t *ret,
		int *shmid) {
	g_setjmp(0, "shmap", NULL, NULL);

	void *ptr;
	int ir = 0;

	int shmflg = 0;

	int i_shmid = -1;

	if (!shmid) {
		shmid = &i_shmid;
	}

	if (*ret & R_SHMAP_ALREADY_EXISTS) {
		ir = 1;
	} else if ((*shmid = shmget(ipc, size,
	IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR)) == -1) {
		if ( errno == EEXIST) {

			if (ret) {
				*ret |= R_SHMAP_ALREADY_EXISTS;
			}
			if ((*shmid = shmget(ipc, 0, 0)) == -1) {
				if (ret) {
					*ret |= R_SHMAP_FAILED_ATTACH;
				}
				return NULL;
			}
			//shmflg |= SHM_RDONLY;
		} else {
			return NULL;
		}
	}

	if ((ptr = shmat(*shmid, NULL, shmflg)) == (void*) -1) {
		if (ret) {
			*ret |= R_SHMAP_FAILED_SHMAT;
		}
		return NULL;
	}

	if (!ipcret) {
		return ptr;
	}

	if (ir != 1) {
		if (shmctl(*shmid, IPC_STAT, ipcret) == -1) {
			return NULL;
		}
	}

	return ptr;
}

pmda search_cfg_rf(pmda md, char * file) {
	p_md_obj ptr = md_first(md);
	p_cfg_r ptr_c;
	size_t fn_len = strlen(file);
	while (ptr) {
		ptr_c = (p_cfg_r) ptr->ptr;
		if (ptr_c && !strncmp(ptr_c->file, file, fn_len)) {
			return &ptr_c->cfg;
		}
		ptr = ptr->next;
	}
	return NULL;
}

pmda register_cfg_rf(pmda md, char *file) {

	if (!md->count) {
		if (md_init(md, 128)) {
			return NULL;
		}
	}

	pmda pmd = search_cfg_rf(md, file);

	if (pmd) {
		return pmd;
	}

	size_t fn_len = strlen(file);

	if (fn_len >= PATH_MAX) {
		return NULL;
	}

	p_cfg_r ptr_c = md_alloc(md, sizeof(cfg_r));

	g_strncpy(ptr_c->file, file, fn_len);
	md_init(&ptr_c->cfg, 256);

	return &ptr_c->cfg;
}

int free_cfg_rf(pmda md) {

	p_md_obj ptr = md_first(md);
	p_cfg_r ptr_c;
	while (ptr) {
		ptr_c = (p_cfg_r) ptr->ptr;
		free_cfg(&ptr_c->cfg);
		ptr = ptr->next;
	}

	return md_g_free(md);
}

off_t s_string(char *input, char *m, off_t offset) {
	off_t i, m_l = strlen(m), i_l = strlen(input);
	for (i = offset; i <= i_l - m_l; i++) {
		if (!strncmp(&input[i], m, m_l)) {
			return i;
		}
	}
	return offset;
}

off_t s_string_r(char *input, char *m) {
	off_t i, m_l = strlen(m), i_l = strlen(input);

	for (i = i_l - 1 - m_l; i >= 0; i--) {
		if (!strncmp(&input[i], m, m_l)) {
			return i;
		}
	}
	return (off_t) -1;
}

#define MAX_SDENTRY_LEN		20000

int m_load_input(__g_handle hdl, char *input) {
	g_setjmp(0, "m_load_input", NULL, NULL);

	if (!hdl->w_buffer.objects) {
		md_init(&hdl->w_buffer, 256);
	}

	if (!hdl->g_proc0) {
		return -1;
	}

	if (!hdl->d_memb) {
		return -2;
	}

	off_t p_c = 0, p_n, p_l;
	char *buffer = malloc(MAX_SDENTRY_LEN + 1);

	mda md_s = { 0 };
	p_md_obj ptr;

	int i, rs, rf = 0;

	while ((p_n = s_string(input, "\n\n", p_c)) > p_c) {
		p_l = p_n - p_c;

		if (p_l < 2) {
			goto e_loop;
		}

		if (p_l > MAX_SDENTRY_LEN) {
			if (gfl & F_OPT_VERBOSE2) {
				print_str(
						"WARNING: DATA IMPORT: failed processing input, too large [%llu]\n",
						(uint64_t) p_l);
			}
			rf++;
			continue;
		}

		g_strncpy(buffer, &input[p_c], p_l);
		bzero(&buffer[p_l], 1);

		p_c = p_n + 2;

		md_init(&md_s, 32);

		if ((rs = split_string(buffer, 0xA, &md_s)) != hdl->d_memb) {
			if (gfl & F_OPT_VERBOSE2) {
				print_str(
						"WARNING: DATA IMPORT: [%d/%d] parameter count mismatch\n",
						rs, hdl->d_memb);
			}
			rf++;
			goto e_loop;
		}

		ptr = md_s.objects;
		char *s_ptr;
		uint32_t rw = 0;
		void *st_buffer = md_alloc(&hdl->w_buffer, hdl->block_sz);

		if (!st_buffer) {
			rf++;
			goto e_loop;
		}

		while (ptr) {
			s_ptr = (char*) ptr->ptr;
			i = 0;
			while (s_ptr[i] && s_ptr[i] != 0x20) {
				i++;
			}

			memset(&s_ptr[i], 0x0, 1);
			char *s_p1 = &s_ptr[i + 1];

			int bd = hdl->g_proc0(st_buffer, s_ptr, s_p1);
			if (!bd && (gfl & F_OPT_VERBOSE)) {
				print_str("WARNING: DATA IMPORT: failed extracting '%s'\n",
						s_ptr);
			}

			rw += (uint32_t) bd;
			ptr = ptr->next;
		}

		if (rw != hdl->d_memb) {
			md_unlink(&hdl->w_buffer, hdl->w_buffer.pos);
			rf++;
		}

		e_loop:

		md_g_free(&md_s);
	}

	g_free(buffer);

	return rf;
}

int gcb_dirlog(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l, v_i;
	uint32_t k_ui;
	struct dirlog * ptr = (struct dirlog *) buffer;
	if (k_l == 3 && !strncmp(key, "dir", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "user", 4)) {
		v_i = (int) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->uploader = v_i;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "group", 5)) {
		v_i = (int) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->group = v_i;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "files", 5)) {
		k_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->files = k_ui;
		return 1;
	} else if (k_l == 4 && !strncmp(key, "size", 4)) {
		ulint64_t k_uli = (ulint64_t) strtoll(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->bytes = k_uli;
		return 1;
	} else if (k_l == 4 && !strncmp(key, "time", 4)) {
		k_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->uptime = k_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "status", 6)) {
		uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->status = k_us;
		return 1;
	}
	return 0;
}

int gcb_nukelog(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;

	struct nukelog* ptr = (struct nukelog*) buffer;
	if (k_l == 3 && !strncmp(key, "dir", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 6 && !strncmp(key, "reason", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->reason, val, v_l > 59 ? 59 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "bytes", 5)) {
		float v_f = strtof(val, NULL);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->bytes = v_f;
		return 1;
	} else if (k_l == 4 && !strncmp(key, "mult", 4)) {
		uint16_t v_ui = (uint16_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->mult = v_ui;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "nukee", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->nukee, val, v_l > 12 ? 12 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "nuker", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->nuker, val, v_l > 12 ? 12 : v_l);
		return 1;
	} else if (k_l == 7 && !strncmp(key, "unnuker", 7)) {
		if (!(v_l = strlen(val))) {
			return 1;
		}
		g_memcpy(ptr->unnuker, val, v_l > 12 ? 12 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "time", 4)) {
		uint32_t k_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->nuketime = k_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "status", 6)) {
		uint16_t k_us = (uint16_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->status = k_us;
		return 1;
	}
	return 0;
}

int gcb_oneliner(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	struct oneliner* ptr = (struct oneliner*) buffer;

	if (k_l == 5 && !strncmp(key, "uname", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->uname, val, v_l > 23 ? 23 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "gname", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->gname, val, v_l > 23 ? 23 : v_l);
		return 1;

	} else if (k_l == 7 && !strncmp(key, "tagline", 7)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->tagline, val, v_l > 63 ? 63 : v_l);
		return 1;
	} else if (k_l == 9 && !strncmp(key, "timestamp", 9)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->timestamp = v_ui;
		return 1;
	} else if (k_l == 7 && !strncmp(key, "message", 7)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->message, val, v_l > 99 ? 99 : v_l);
		return 1;
	}
	return 0;
}

int gcb_dupefile(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	struct dupefile* ptr = (struct dupefile*) buffer;

	if (k_l == 8 && !strncmp(key, "filename", 8)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->filename, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 8 && !strncmp(key, "uploader", 8)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->uploader, val, v_l > 23 ? 23 : v_l);
		return 1;

	} else if (k_l == 6 && !strncmp(key, "timeup", 6)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->timeup = v_ui;
		return 1;
	}
	return 0;
}

int gcb_lastonlog(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	struct lastonlog* ptr = (struct lastonlog*) buffer;

	if (k_l == 5 && !strncmp(key, "uname", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->uname, val, v_l > 23 ? 23 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "gname", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->gname, val, v_l > 23 ? 23 : v_l);
		return 1;
	} else if (k_l == 7 && !strncmp(key, "tagline", 7)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->tagline, val, v_l > 63 ? 63 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "logon", 5)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->logon = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "logoff", 6)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->logoff = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "upload", 6)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->upload = v_ui;
		return 1;
	} else if (k_l == 8 && !strncmp(key, "download", 8)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->download = v_ui;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "stats", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->stats, val, v_l > 6 ? 6 : v_l);
		return 1;
	}
	return 0;
}

int gcb_game(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	__d_game ptr = (__d_game) buffer;
	if (k_l == 3 && !strncmp(key, "dir", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "time", 4)) {
		int32_t v_ui = (int32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->timestamp = v_ui;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "score", 5)) {
		float v_f = strtof(val, NULL);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->rating = v_f;
		return 1;
	}
	return 0;
}

int gcb_tv(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	__d_tvrage ptr = (__d_tvrage) buffer;
	if (k_l == 3 && !strncmp(key, "dir", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "time", 4)) {
		int32_t v_ui = (int32_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->timestamp = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "airday", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->airday, val, v_l > 31 ? 31 : v_l);
		return 1;
	} else if (k_l == 7 && !strncmp(key, "airtime", 7)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->airtime, val, v_l > 5 ? 5 : v_l);
		return 1;
	} else if (k_l == 7 && !strncmp(key, "country", 7)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->country, val, v_l > 23 ? 23 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "link", 4)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->link, val, v_l > 127 ? 127 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "name", 4)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->name, val, v_l > 127 ? 127 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "ended", 5)) {
		int32_t v_ui = (int32_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->ended = v_ui;
		return 1;
	} else if (k_l == 7 && !strncmp(key, "started", 7)) {
		int32_t v_ui = (int32_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->started = v_ui;
		return 1;
	} else if (k_l == 7 && !strncmp(key, "runtime", 7)) {
		int32_t v_ui = (int32_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->runtime = v_ui;
		return 1;
	} else if (k_l == 7 && !strncmp(key, "seasons", 7)) {
		int16_t v_ui = (int16_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->seasons = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "showid", 6)) {
		int32_t v_ui = (int32_t) strtoul(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->showid = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "status", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->status, val, v_l > 63 ? 63 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "class", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->class, val, v_l > 63 ? 63 : v_l);
		return 1;
	} else if (k_l == 6 && !strncmp(key, "genres", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->genres, val, v_l > 255 ? 255 : v_l);
		return 1;
	}
	return 0;
}

int gcb_imdbh(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	__d_imdb ptr = (__d_imdb) buffer;
	if (k_l == 3 && !strncmp(key, "dir", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->dirname, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "time", 4)) {
		int32_t v_ui = (int32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->timestamp = v_ui;
		return 1;
	} else if (k_l == 6 && !strncmp(key, "imdbid", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->imdb_id, val, v_l > 63 ? 63 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "score", 5)) {
		float v_f = strtof(val, NULL);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->rating = v_f;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "votes", 5)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->votes = v_ui;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "genre", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->genres, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 4 && !strncmp(key, "year", 4)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->year, val, v_l > 4 ? 4 : v_l);
		return 1;
	} else if (k_l == 5 && !strncmp(key, "title", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->title, val, v_l > 127 ? 127 : v_l);
		return 1;
	} else if (k_l == 8 && !strncmp(key, "released", 8)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->released = v_ui;
		return 1;
	} else if (k_l == 7 && !strncmp(key, "runtime", 7)) {
		uint32_t v_ui = (uint32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->runtime = v_ui;
		return 1;
	} else if (k_l == 5 && !strncmp(key, "rated", 5)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->rated, val, v_l > 7 ? 7 : v_l);
		return 1;
	} else if (k_l == 6 && !strncmp(key, "actors", 6)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->actors, val, v_l > 127 ? 127 : v_l);
		return 1;
	} else if (k_l == 8 && !strncmp(key, "director", 8)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->director, val, v_l > 63 ? 63 : v_l);
		return 1;
	}

	return 0;
}

int gcb_gen1(void *buffer, char *key, char *val) {
	size_t k_l = strlen(key), v_l;
	__d_generic_s2044 ptr = (__d_generic_s2044) buffer;

	if (k_l == 3 && !strncmp(key, "ge1", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_1, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge2", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_2, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge3", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_3, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge4", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_4, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge5", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_5, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge6", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_6, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge7", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_7, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "ge8", 3)) {
		if (!(v_l = strlen(val))) {
			return 0;
		}
		g_memcpy(ptr->s_8, val, v_l > 254 ? 254 : v_l);
		return 1;
	} else if (k_l == 3 && !strncmp(key, "i32", 3)) {
		int32_t v_ui = (int32_t) strtol(val, NULL, 10);
		if ( errno == ERANGE) {
			return 0;
		}
		ptr->i32 = v_ui;
		return 1;
	}

	return 0;
}

int load_cfg(pmda pmd, char *file, uint32_t flags, pmda *res) {
	g_setjmp(0, "load_cfg", NULL, NULL);
	int r = 0;
	FILE *fh;
	pmda md;

	if (flags & F_LCONF_NORF) {
		md_init(pmd, 256);
		md = pmd;
	} else {
		md = register_cfg_rf(pmd, file);
	}

	if (!md) {
		return 1;
	}

	off_t f_sz = get_file_size(file);

	if (!f_sz) {
		return 2;
	}

	if (!(fh = gg_fopen(file, "r"))) {
		return 3;
	}

	char *buffer = calloc(V_MB, 1);
	p_cfg_h pce;
	int rd, i;

	while (fgets(buffer, V_MB, fh)) {
		if (strlen(buffer) < 3) {
			continue;
		}

		for (i = 0; buffer[i] == 0x20 || buffer[i] == 0x9; i++) {
		}

		if (buffer[i] == 0x23) {
			continue;
		}

		pce = (p_cfg_h) md_alloc(md, sizeof(cfg_h));
		md_init(&pce->data, 32);
		if ((rd = split_string_sp_tab(buffer, &pce->data)) < 1) {
			md_g_free(&pce->data);
			md_unlink(md, md->pos);
			continue;
		}

		pce->key = pce->data.objects->ptr;
	}

	g_fclose(fh);
	g_free(buffer);

	if (res) {
		*res = md;
	}

	return r;
}

void free_cfg(pmda md) {
	g_setjmp(0, "free_cfg", NULL, NULL);

	if (!md->objects) {
		return;
	}

	p_md_obj ptr = md_first(md);
	p_cfg_h pce;

	while (ptr) {
		pce = (p_cfg_h) ptr->ptr;
		if (pce) {
			md_g_free(&pce->data);
		}
		ptr = ptr->next;
	}

	md_g_free(md);
}

p_md_obj get_cfg_opt(char *key, pmda md, pmda *ret) {
	if (!md->count) {
		return NULL;
	}

	p_md_obj ptr = md_first(md);
	size_t pce_key_sz, key_sz = strlen(key);
	p_cfg_h pce;

	while (ptr) {
		pce = (p_cfg_h) ptr->ptr;
		pce_key_sz = strlen(pce->key);
		if (pce_key_sz == key_sz && !strncmp(pce->key, key, pce_key_sz)) {
			p_md_obj r_ptr = md_first(&pce->data);
			if (r_ptr) {
				if (ret) {
					*ret = &pce->data;
				}
				return (p_md_obj) r_ptr->next;
			} else {
				return NULL;
			}
		}
		ptr = ptr->next;
	}

	return NULL;
}

int self_get_path(char *out) {
	g_setjmp(0, "self_get_path", NULL, NULL);

	char path[PATH_MAX];
	int r;

	snprintf(path, PATH_MAX, "/proc/%d/exe", getpid());

	if (file_exists(path)) {
		snprintf(path, PATH_MAX, "/compat/linux/proc/%d/exe", getpid());
	}

	if (file_exists(path)) {
		snprintf(path, PATH_MAX, "/proc/%d/file", getpid());
	}

	if ((r = readlink(path, out, PATH_MAX)) == -1) {
		return 2;
	}
	out[r] = 0x0;
	return 0;
}

int is_ascii_text(uint8_t in_c) {
	if ((in_c >= 0x0 && in_c <= 0x7F)) {
		return 0;
	}

	return 1;
}

int is_ascii_lowercase_text(uint8_t in_c) {
	if ((in_c >= 0x61 && in_c <= 0x7A)) {
		return 0;
	}

	return 1;
}

int is_ascii_alphanumeric(uint8_t in_c) {
	if ((in_c >= 0x61 && in_c <= 0x7A) || (in_c >= 0x41 && in_c <= 0x5A)
			|| (in_c >= 0x30 && in_c <= 0x39)) {
		return 0;
	}

	return 1;
}

int is_ascii_uppercase_text(uint8_t in_c) {
	if ((in_c >= 0x41 && in_c <= 0x5A)) {
		return 0;
	}

	return 1;
}

char *replace_char(char w, char r, char *string) {
	int s_len = strlen(string), i;
	for (i = 0; i < s_len + 100 && string[i] != 0; i++) {
		if (string[i] == w) {
			string[i] = r;
		}
	}

	return string;
}

#define SSD_MAX_LINE_SIZE 	262144
#define SSD_MAX_LINE_PROC 	15000

int ssd_4macro(char *name, unsigned char type, void *arg, __g_eds eds) {
	off_t name_sz;
	switch (type) {
	case DT_REG:
		name_sz = get_file_size(name);
		if (!name_sz) {
			break;
		}

		FILE *fh = gg_fopen(name, "r");

		if (!fh) {
			break;
		}

		char *buffer = calloc(1, SSD_MAX_LINE_SIZE + 16);

		size_t b_len, lc = 0;
		int hit = 0, i;

		while (fgets(buffer, SSD_MAX_LINE_SIZE, fh) && lc < SSD_MAX_LINE_PROC) {
			lc++;
			b_len = strlen(buffer);
			if (b_len < 8) {
				continue;
			}

			for (i = 0; i < b_len && i < SSD_MAX_LINE_SIZE; i++) {
				if (is_ascii_text((unsigned char) buffer[i])) {
					break;
				}
			}

			if (strncmp(buffer, "#@MACRO:", 8)) {
				continue;
			}

			__si_argv0 ptr = (__si_argv0) arg;

			char buffer2[4096] = { 0 };
			snprintf(buffer2, 4096, "%s:", ptr->p_buf_1);

			size_t pb_l = strlen(buffer2);

			if (!strncmp(buffer2, &buffer[8], pb_l)) {
				buffer = replace_char(0xA, 0x0, buffer);
				buffer = replace_char(0xD, 0x0, buffer);
				b_len = strlen(buffer);
				size_t d_len = b_len - 8 - pb_l;
				if (d_len > sizeof(ptr->s_ret)) {
					d_len = sizeof(ptr->s_ret);
				}
				bzero(ptr->s_ret, sizeof(ptr->s_ret));
				g_strncpy(ptr->s_ret, &buffer[8 + pb_l], d_len);
				g_strncpy(ptr->p_buf_2, name, strlen(name));
				ptr->ret = d_len;
				gfl |= F_OPT_TERM_ENUM;
				break;
			}
			hit++;
		}

		g_fclose(fh);
		g_free(buffer);
		break;
	case DT_DIR:
		enum_dir(name, ssd_4macro, arg, 0, eds);
		break;
	}

	return 0;
}

int get_file_type(char *file) {
	struct stat sb;

	if (stat(file, &sb) == -1)
		return errno;

	return IFTODT(sb.st_mode);
}

int find_absolute_path(char *exec, char *output) {
	char *env = getenv("PATH");

	if (!env) {
		return 1;
	}

	mda s_p = { 0 };

	md_init(&s_p, 64);

	int p_c = split_string(env, 0x3A, &s_p);

	if (p_c < 1) {
		return 2;
	}

	p_md_obj ptr = md_first(&s_p);

	while (ptr) {
		snprintf(output, PATH_MAX, "%s/%s", (char*) ptr->ptr, exec);
		if (!access(output, R_OK | X_OK)) {
			md_g_free(&s_p);
			return 0;
		}
		ptr = ptr->next;
	}

	md_g_free(&s_p);

	return 3;
}