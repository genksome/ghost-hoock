/* offsets_bza5.h — single-target offsets table for Samsung A17 BZA5
 * kernel 6.12.23-android16-5-abA175FXXS5BZD2-4k
 *
 * Extracted 1:1 from the shipped ghostlock devices/offsets.h.
 * Add new entries above the NULL terminator if you port to another device. */
#ifndef OFFSETS_BZA5_H
#define OFFSETS_BZA5_H

#include <stdint.h>

struct kernel_offsets {
  const char *uname_r;
  uint64_t kernel_phys_load;
  uint64_t off_init_task, off_init_cred, off_init_uts_ns, off_empty_zero_page;
  uint64_t off_root_task_group, off_selinux_enforcing, off_kptr_restrict;
  uint64_t off_selinux_blob_sizes, off_security_hook_heads, off_kmalloc_caches;
  uint64_t off_anon_pipe_buf_ops, off_ashmem_misc_fops, off_ashmem_fops;
  uint64_t off_ashmem_ioctl, off_ashmem_compat_ioctl, off_ashmem_mmap;
  uint64_t off_ashmem_open, off_ashmem_release, off_ashmem_show_fdinfo;
  uint64_t off_configfs_read_iter, off_configfs_bin_write_iter;
  uint64_t off_copy_splice_read, off_noop_llseek, off_cap_capable_active;
  uint64_t off_slide_nfulnl_logger, off_slide_loggers_0_1, off_slide_boot_id;
  uint64_t off_system_unbound_wq;
  uint64_t off_call_usermodehelper_exec_work;

  uint32_t task_prio, task_normal_prio, task_sched_task_group;
  uint32_t task_pi_lock, task_pi_waiters, task_pi_top_task, task_pi_blocked_on;
  uint32_t task_pid, task_tgid, task_real_parent, task_atomic_flags;
  uint32_t task_real_cred, task_cred, task_comm, task_tasks, task_seccomp;
  uint32_t mm_owner;
};

#define OFFSETS_ENTRY(uname, ...) { .uname_r = uname, __VA_ARGS__ }

#define STRUCT_OFFSETS_6_12 \
  .task_prio=0x94, .task_normal_prio=0x9C, .task_sched_task_group=0x420, \
  .task_pi_lock=0x9EC, .task_pi_waiters=0xA00, \
  .task_pi_top_task=0xA10, .task_pi_blocked_on=0xA18, \
  .task_pid=0x708, .task_tgid=0x70C, .task_real_parent=0x718, \
  .task_atomic_flags=0x6C8, .task_real_cred=0x8F8, .task_cred=0x900, \
  .task_comm=0x910, .task_tasks=0x638, .task_seccomp=0x9C8, \
  .mm_owner=0x410

static const struct kernel_offsets known_offsets[] = {
  OFFSETS_ENTRY("6.12.23-android16-5-abA175FXXS5BZD2-4k",
    .kernel_phys_load=0x40000000, STRUCT_OFFSETS_6_12,
    .off_init_task=0x024FCF40, .off_init_cred=0x02512B08,
    .off_init_uts_ns=0x02685A88, .off_empty_zero_page=0x02726000,
    .off_root_task_group=0x0272ED80, .off_selinux_enforcing=0x0277E560,
    .off_kptr_restrict=0x024FB678, .off_selinux_blob_sizes=0x018A10E8,
    .off_security_hook_heads=0x0, .off_kmalloc_caches=0x018974C0,
    .off_anon_pipe_buf_ops=0x0126EF88, .off_ashmem_misc_fops=0x0,
    .off_ashmem_fops=0x013FB018, .off_ashmem_ioctl=0x00DD8FA0,
    .off_ashmem_compat_ioctl=0x00DD9588, .off_ashmem_mmap=0x00DD9604,
    .off_ashmem_open=0x00DD9660, .off_ashmem_release=0x00DD9060,
    .off_ashmem_show_fdinfo=0x00DD9560,
    .off_configfs_read_iter=0x00512CE4, .off_configfs_bin_write_iter=0x00512F18,
    .off_copy_splice_read=0x0048E7D4, .off_noop_llseek=0x0043BB44,
    .off_cap_capable_active=0x0,
    .off_slide_nfulnl_logger=0x024F21A0, .off_slide_loggers_0_1=0x024F20F0,
    .off_slide_boot_id=0x028204F0,
    .off_system_unbound_wq=0x0, .off_call_usermodehelper_exec_work=0x0,
  ),
  { .uname_r = NULL }
};

#endif
