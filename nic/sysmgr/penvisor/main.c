// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "penvisor.h"

#define LINE_SIZE 2048
#define PATH_MAX 2048
#define MOUNT_TRIES 10
#define MOUNT_SLEEP_MS 50
#define WAITPID_TRIES 10
#define WAITPID_SLEEP_MS 50
#define STACK_SIZE (1024 * 1024 * 8)
#define PIPE_WRITE 1
#define PIPE_READ  0

typedef enum instance_status_e_ {
    INST_STATUS_NULL = 0,
    INST_STATUS_LOADED = 1,
    INST_STATUS_ACTIVE = 2,
} instance_status_e;

typedef enum pipe_command_e_ {
    PIPE_COMMAND_SWITCHOVER = 0,
    PIPE_COMMAND_SHUTDOWN   = 1,
    PIPE_COMMAND_EXIT       = 2,
} pipe_command_e;

typedef enum pipe_result_e_ {
    PIPE_RESULT_NOK = -1,
    PIPE_RESULT_OK  = 0,
} pipe_result_e;

typedef struct instance_state_t_ {
    instance_status_e status;
    pid_t             pid;
    int               request_fds[2];
    int               response_fds[2];
} instance_state_t;

typedef struct mount_point_t_ {
    const char *src;
    const char *dst;
    const char *opts;
} mount_point_t;

typedef enum mount_e_ {
    SQUASHFS     = 0,
    TMPFS        = 1,
    UPPER        = 2,
    WORK         = 3,
    OVERLAY      = 4,
    ROOT_BIND    = 5,
    DEV_BIND     = 6,
    SYS_BIND     = 7,
    UPDATE_BIND  = 8,
    CONFIG0_BIND = 9,
    CONFIG1_BIND = 10,
    OBFL_BIND    = 11,
    DATA_BIND    = 12,
    SHARE_BIND   = 13,
} mount_e;

typedef struct clone_ctx_t_ {
    penvisor_instance_e instance;
    instance_status_e status;
    int request_fds[2];
    int response_fds[2];
} clone_ctx_t;

static const char *local_instance_id_files[2] = {
    "/mnt/a/rw/.instance_a",
    "/mnt/b/rw/.instance_b",
};

static const char *instance_pid_files[2] = {
    "/var/run/instance_a",
    "/var/run/instance_b",
};

static penvisor_instance_e g_active_instance;

static instance_state_t g_instances[2] = {
    {
        .status = INST_STATUS_NULL,
        .pid    = 0,
    },
    {
        .status = INST_STATUS_NULL,
        .pid    = 0,
    }
};

static const mount_point_t mounts_a[] = {
    {
        .src = "/dev/mmcblk0p3",
        .dst = "/mnt/a",
        .opts = "",
    },
    {
        .src = "tmpfs",
        .dst = "/mnt/a/mnt",
        .opts = "",
    },
    {
        .src = "",
        .dst = "/mnt/a/mnt/upper",
        .opts = "",
    },
    {
        .src = "",
        .dst= "/mnt/a/mnt/work",
        .opts = "",
    },
    {
        .src = "overlay",
        .dst = "/mnt/a/rw",
        .opts = "lowerdir=/mnt/a,upperdir=/mnt/a/mnt/upper,workdir=/mnt/a/mnt/work",
    },
    {
        .src = "/mnt/a",
        .dst = "/mnt/a/rw/ro",
        .opts = "",
    },
    {
        .src = "/dev",
        .dst = "/mnt/a/rw/dev",
        .opts = "",
    },
    {
        .src = "/sys",
        .dst = "/mnt/a/rw/sys",
        .opts = "",
    },
    {
        .src = "/update",
        .dst = "/mnt/a/rw/update",
        .opts = "",
    },
    {
        .src = "/sysconfig/config0",
        .dst = "/mnt/a/rw/sysconfig/config0",
        .opts = "",
    },
    {
        .src = "/sysconfig/config1",
        .dst = "/mnt/a/rw/sysconfig/config1",
        .opts = "",
    },
    {
        .src = "/obfl",
        .dst = "/mnt/a/rw/obfl",
        .opts = "",
    },
    {
        .src = "/data",
        .dst = "/mnt/a/rw/data",
        .opts = "",
    },
    {
        .src = "/share",
        .dst = "/mnt/a/rw/share",
        .opts = "",
    },
};

static const mount_point_t mounts_b[] = {
    {
        .src = "/dev/mmcblk0p4",
        .dst = "/mnt/b",
        .opts = "",
    },
    {
        .src = "tmpfs",
        .dst = "/mnt/b/mnt",
        .opts = "",
    },
    {
        .src = "",
        .dst = "/mnt/b/mnt/upper",
        .opts = "",
    },
    {
        .src = "",
        .dst= "/mnt/b/mnt/work",
        .opts = "",
    },
    {
        .src = "overlay",
        .dst = "/mnt/b/rw",
        .opts = "lowerdir=/mnt/b,upperdir=/mnt/b/mnt/upper,workdir=/mnt/b/mnt/work",
    },
    {
        .src = "/mnt/b",
        .dst = "/mnt/b/rw/ro",
        .opts = "",
    },
    {
        .src = "/dev",
        .dst = "/mnt/b/rw/dev",
        .opts = "",
    },
    {
        .src = "/sys",
        .dst = "/mnt/b/rw/sys",
        .opts = "",
    },
    {
        .src = "/update",
        .dst = "/mnt/b/rw/update",
        .opts = "",
    },
    {
        .src = "/sysconfig/config0",
        .dst = "/mnt/b/rw/sysconfig/config0",
        .opts = "",
    },
    {
        .src = "/sysconfig/config1",
        .dst = "/mnt/b/rw/sysconfig/config1",
        .opts = "",
    },
    {
        .src = "/obfl",
        .dst = "/mnt/b/rw/obfl",
        .opts = "",
    },
    {
        .src = "/data",
        .dst = "/mnt/b/rw/data",
        .opts = "",
    },
    {
        .src = "/share",
        .dst = "/mnt/b/rw/share",
        .opts = "",
    },
};

static penvisor_instance_e
get_boot_instance (void)
{
    char line[LINE_SIZE];
    int fd;

    fd = open("/proc/cmdline", O_RDONLY);
    assert(fd != -1);

    read(fd, line, LINE_SIZE);
    close(fd);

    if (strstr(line, "mainfwa") != NULL) {
        return PENVISOR_INSTANCE_A;
    }
    if (strstr(line, "mainfwb") != NULL) {
        return PENVISOR_INSTANCE_B;
    }

    assert(0);
}

static const mount_point_t *
get_instance_mounts (penvisor_instance_e instance)
{
    if (instance == PENVISOR_INSTANCE_A) {
        return mounts_a;
    }

    if (instance == PENVISOR_INSTANCE_B) {
       return mounts_b;
    }
    
    assert(0);
}

static int
try_mount(const char *src, const char *dst, const char *fs, unsigned long flags,
          const char *data)
{
    int rc;

    fprintf(stderr, "Mounting %s to %s\n", src, dst);
    for (int i = 0; i < MOUNT_TRIES; i++) {
        rc = mount(src, dst, fs, flags, data);
        if (rc != -1) {
            break;
        }
        fprintf(stderr, "Try %d to mount %s to %s failed. Will try again\n",
                i + 1, src, dst);
        usleep(MOUNT_SLEEP_MS * 1000);
    }
    
    return rc;
}

static void
exists_or_mkdir (const char *dir)
{
    struct stat sb;
    if (stat(dir, &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "%s is not a directory\n", dir);
            exit(-1);
        }
        return;
    }
    
    mkdir(dir, S_IRWXU);
    fprintf(stderr, "Creating directory %s\n", dir);
}


static void
mkdirs (const char *dir)
{
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    struct stat sb;

    // if file exists bail out
    if (stat(dir, &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) {
            fprintf(stderr, "%s is not a directory\n", dir);
            exit(-1);
        }
        return;
    }
    
    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);

    if(tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }
                
    for(p = tmp + 1; *p; p++)
    {
        if(*p == '/')
        {
            *p = 0;
            exists_or_mkdir(tmp);
            *p = '/';
        }
    }
    exists_or_mkdir(tmp);
}

static void
touch_file (const char *path)
{
    int fd;

    fd = open(path, O_RDWR | O_CREAT, 0644);
    assert(fd != -1);
    close(fd);
}

static void
write_pid (penvisor_instance_e instance, pid_t pid)
{
    int fd;
    int rc;

    assert(instance == PENVISOR_INSTANCE_A || instance == PENVISOR_INSTANCE_B);
    
    fd = open(instance_pid_files[instance], O_RDWR | O_CREAT, 0644);
    assert(fd != -1);

    rc = ftruncate(fd, 0);
    assert(rc != -1);

    dprintf(fd, "%d", pid);

    close(fd);
}

static void
sigchld_handler (int sig)
{
    while (1) {
        int rc;

        rc = waitpid((pid_t)(-1), 0, WNOHANG);
        if (rc <= 0) {
            return;
        }
    }
}

static void
run_shell_hook (penvisor_instance_e instance, const char *action)
{
    const char *instance_letter;
    char cmd[LINE_SIZE];

    switch (instance) {
    case PENVISOR_INSTANCE_A:
        instance_letter = "a";
        break;
    case PENVISOR_INSTANCE_B:
        instance_letter = "b";
        break;
    default:
        assert(0);
    }

    snprintf(cmd, LINE_SIZE, "/bin/penvisor_hooks.sh %s %s", action,
             instance_letter);
    fprintf(stderr, "Running %s\n", cmd);
    system(cmd);
    fprintf(stderr, "%s done\n", cmd);
}

static void
boot_script (void)
{
    system("/bin/penvisor_boot.sh");
}

static void
system_init (void)
{
    fprintf(stderr, "Calling init\n");
    system("/nic/tools/instinit.sh init");
    fprintf(stderr, "init done\n");
}

static void
system_standby (void)
{
    fprintf(stderr, "Calling standby\n");
    system("/nic/tools/instinit.sh standby");
    fprintf(stderr, "standby done\n");
}

static void
system_switchover (void)
{
    fprintf(stderr, "Calling switchover\n");
    system("/nic/tools/instinit.sh switchover");
    fprintf(stderr, "switchover done\n");
}

static void
system_shutdown (void)
{
    fprintf(stderr, "Calling shutdown\n");
    system("/nic/tools/instinit.sh shutdown");
    fprintf(stderr, "shutdown done\n");
}

static int
instance_entry (void *arg)
{
    int rc;
    pid_t pid;
    clone_ctx_t *ctx = (clone_ctx_t *)arg;
    const mount_point_t *mounts = get_instance_mounts(ctx->instance);

    signal(SIGCHLD, sigchld_handler);

    rc = chroot(mounts[OVERLAY].dst);
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }
    
    rc = try_mount("proc", "/proc", "proc", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }
    
    mkdirs("/var/log/pensando/");
    mkdirs("/dev/shm/");

    if (ctx->status == INST_STATUS_ACTIVE) {
        system_init();
    } else {
        system_standby();
    }
    
    while (1) {
        pipe_command_e next_action;
        int n;

        n = read(ctx->request_fds[PIPE_READ], &next_action,
                 sizeof(next_action));
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "read: %d, %d, %s\n", n, errno, strerror(errno));
        }
        if (n == 0) {
            fprintf(stderr, "read: 0\n");
            break;
        }
        if (next_action == PIPE_COMMAND_SWITCHOVER) {
            pipe_result_e result = PIPE_RESULT_OK;
            fprintf(stderr, "Got PIPE_COMMAND_SWITCHOVER\n");
            system_switchover();
            fprintf(stderr, "Sending PIPE_RESULT_OK\n");
            write(ctx->response_fds[PIPE_WRITE], &result, sizeof(result));
        } else if (next_action == PIPE_COMMAND_SHUTDOWN) {
            pipe_result_e result = PIPE_RESULT_OK;
            fprintf(stderr, "Got PIPE_COMMAND_SHUTDOWN\n");
            system_shutdown();
            fprintf(stderr, "Sending PIPE_RESULT_OK\n");
            write(ctx->response_fds[PIPE_WRITE], &result, sizeof(result));
        } else if (next_action == PIPE_COMMAND_EXIT) {
            fprintf(stderr, "Exiting\n");
            break;
        }
    }
    
    free(ctx);
    return 0;
}

static void
ramfs_mounts(void)
{
    int rc;
    
    rc = try_mount("proc", "/proc", "proc", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount("sysfs", "/sys", "sysfs", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount("/dev/mmcblk0p5", "/update", "ext4", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    } 

    rc = try_mount("/dev/mmcblk0p6", "/sysconfig/config0", "ext4", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount("/dev/mmcblk0p7", "/sysconfig/config1", "ext4", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount("/dev/mmcblk0p8", "/obfl", "ext4", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount("/dev/mmcblk0p10", "/data", "ext4", 0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    mkdirs("/share");
    rc = try_mount("tmpfs", "/share", "tmpfs", 0, "size=20M");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    mkdirs("/mnt/a");
    mkdirs("/mnt/b");
}

static void
instance_mounts(penvisor_instance_e instance)
{
    int rc;
    const mount_point_t *mounts = get_instance_mounts(instance);

    rc = try_mount(mounts[SQUASHFS].src, mounts[SQUASHFS].dst, "squashfs",
                   0, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }
    
    rc = try_mount(mounts[TMPFS].src, mounts[TMPFS].dst, "tmpfs", 0,
                   "size=20M");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    mkdirs(mounts[UPPER].dst);

    mkdirs(mounts[WORK].dst);

    rc = try_mount(mounts[OVERLAY].src, mounts[OVERLAY].dst, "overlay", 0,
               mounts[OVERLAY].opts);
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[DEV_BIND].src, mounts[DEV_BIND].dst, "", MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[SYS_BIND].src, mounts[SYS_BIND].dst, "", MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[UPDATE_BIND].src, mounts[UPDATE_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[CONFIG0_BIND].src, mounts[CONFIG0_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[CONFIG1_BIND].src, mounts[CONFIG1_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[OBFL_BIND].src, mounts[OBFL_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[DATA_BIND].src, mounts[DATA_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    mkdirs(mounts[SHARE_BIND].dst);
    
    rc = try_mount(mounts[SHARE_BIND].src, mounts[SHARE_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    rc = try_mount(mounts[ROOT_BIND].src, mounts[ROOT_BIND].dst, "",
                   MS_BIND, "");
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
    }

    touch_file(local_instance_id_files[instance]);
}

static int
change_ns (int pid)
{
    char ns_path[PATH_MAX];
    int fd;
    int rc;

    snprintf(ns_path, PATH_MAX, "/proc/%d/ns/pid", pid);

    fd = open(ns_path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        fprintf(stderr, "Failed to open %s\n", ns_path);
        return -1;
    }

    rc = setns(fd, CLONE_NEWPID);
    if (rc == -1) {
        fprintf(stderr, "setns failed (%d, %s)\n", errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);
    
    return 0;
}

static int
read_pid (const char *path)
{
    int fd;
    int n;
    char line[LINE_SIZE];
    int pid;
    
    fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        fprintf(stderr, "Failed to open %s\n", path);
        return -1;
    }

    n = read(fd, line, LINE_SIZE - 1);
    if (n < 1) {
        fprintf(stderr, "Couldn't read from %s\n", path);
        close(fd);
        return -1;
    }
    close(fd);
    line[n] = '\0';

    pid = atoi(line);
    if (pid == 0) {
        fprintf(stderr, "PID read from %s is not a number: %s\n", path, line);
        return -1;
    }

    return pid;
}

static penvisor_instance_e
str_to_inst (const char *inst)
{
    if (strcasecmp("a", inst) == 0) {
        return PENVISOR_INSTANCE_A;
    } else if (strcasecmp("b", inst) == 0) {
        return PENVISOR_INSTANCE_B;
    }

    return -1;
}

static int
attach (const char *inst)
{
    int rc;
    int pid;
    pid_t new_pid;
    const char *pid_file;
    const mount_point_t *mounts;
    penvisor_instance_e instance;

    instance = str_to_inst(inst);
    if (instance == -1) {
        fprintf(stderr, "Unknown instance %s. Use 'A' or 'B'\n", inst);
        return -1;
    }

    pid_file = instance_pid_files[instance];
    mounts = get_instance_mounts(instance);

    pid = read_pid(pid_file);
    if (pid == -1) {
        fprintf(stderr, "Couldn't read pid from %s\n", pid_file);
        return -1;
    }

    rc = change_ns(pid);
    if (rc == -1) {
        fprintf(stderr, "Couldn't change namsepace to %d\n", pid);
        return -1;
    }

    new_pid = fork();
    if (new_pid != 0) {
        waitpid(new_pid, NULL, 0);
        fprintf(stderr, "%d exited\n", new_pid);
        exit(0);
    }
    
    rc = chroot(mounts[OVERLAY].dst);
    if (rc == -1) {
        fprintf(stderr, "Couldn't chroot to %s\n", mounts[OVERLAY].dst);
        return -1;
    }

    printf("Attaching...\n");
    rc = execl("/bin/sh", "/bin/sh", NULL);

    fprintf(stderr, "Failed to spawn shell");
    return rc;
}

static int
load_instance (penvisor_instance_e instance, instance_status_e new_status)
{
    char *new_stack;
    int pid;
    int rc;
    clone_ctx_t *ctx;

    instance_mounts(instance);

    if (new_status == INST_STATUS_ACTIVE) {
        run_shell_hook(g_active_instance, "mount-active");
    } else {
        run_shell_hook(g_active_instance, "mount-standby");
    }

    new_stack = (char *)malloc(STACK_SIZE);
    assert(new_stack != NULL);

    ctx = (clone_ctx_t *)malloc(sizeof(*ctx));
    ctx->instance = instance;
    ctx->status = new_status;
    rc = pipe(g_instances[instance].request_fds);
    assert(rc != -1);
    rc = pipe(g_instances[instance].response_fds);
    assert(rc != -1);
    memcpy(ctx->request_fds, g_instances[instance].request_fds,
           sizeof(ctx->request_fds));
    memcpy(ctx->response_fds, g_instances[instance].response_fds,
           sizeof(ctx->response_fds));
    
    pid = clone(instance_entry, new_stack + STACK_SIZE,
                CLONE_NEWPID | SIGCLD, ctx);
    
    if (pid == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
        return -1;
    }
    
    write_pid(instance, pid);
    g_instances[instance].status = new_status;
    g_instances[instance].pid = pid;
    
    return 0;
}

static int
kill_instance (penvisor_instance_e instance)
{
    char cmd[LINE_SIZE];
    const mount_point_t *mounts;
    int status;
    int rc;

    mounts = get_instance_mounts(instance);

    usleep(WAITPID_SLEEP_MS * 1000);
    fprintf(stderr, "Killing instance\n");
    kill(g_instances[instance].pid, SIGKILL);
    for (int i = 0; i < WAITPID_TRIES; i++) {
        rc = waitpid(g_instances[instance].pid, &status, WNOHANG);
        if (rc > 0) {
            break;
        }
        fprintf(stderr, "Waiting for penvisor to die %d/%d\n",
                i + 1, WAITPID_TRIES);
        usleep(WAITPID_SLEEP_MS * 1000);
    }

    fprintf(stderr, "Unmounting\n");
    snprintf(cmd, LINE_SIZE, "umount -Rf %s", mounts[SQUASHFS].dst);
    system(cmd);

    fprintf(stderr, "umounted\n");
}

static penvisor_instance_e
non_active_instance (void)
{
    if (g_active_instance == PENVISOR_INSTANCE_A) {
        return PENVISOR_INSTANCE_B;
    }
    return PENVISOR_INSTANCE_A;
}
        
static void
run_server (void)
{
    int server_fd;
    int rc;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert(server_fd != -1);

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, PENVISOR_PATH, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    unlink(PENVISOR_PATH);
    
    rc = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        fprintf(stderr, "oops %d, %d, %s\n", __LINE__, errno, strerror(errno));
        return;
    }

    while (1) {
        int len;
        penvisor_command_t command;
        pipe_command_e action;
        pipe_result_e result;
        
        len = recvfrom(server_fd, &command, sizeof(command), 0, NULL, NULL);
        if (len != sizeof(command)) {
            fprintf(stderr, "Received command of wrong size: %d\n", len);
            continue;
        }
        switch(command.action) {
        case PENVISOR_SYSRESET:
            fprintf(stderr, "Got sysreset. Not implemented yet\n");
            close(server_fd);
            unlink(PENVISOR_PATH);
            return;
        case PENVISOR_INSTLOAD:
            fprintf(stderr, "Got instload\n");
            rc = load_instance(non_active_instance(), INST_STATUS_LOADED);
            assert(rc != -1);
            break;
        case PENVISOR_INSTSWITCH:
            fprintf(stderr, "Got instswitch\n");

            fprintf(stderr, "Sending shutdown\n");
            action = PIPE_COMMAND_SHUTDOWN;
            write(g_instances[g_active_instance].request_fds[PIPE_WRITE],
                  &action, sizeof(action));
            fprintf(stderr, "Waiting for reply\n");
            rc = read(
                g_instances[g_active_instance].response_fds[PIPE_READ],
                &result, sizeof(result));
            assert(rc == sizeof(result));

            run_shell_hook(g_active_instance, "post-shutdown");

            run_shell_hook(non_active_instance(), "pre-switchover");

            fprintf(stderr, "Sending switchover\n");
            action = PIPE_COMMAND_SWITCHOVER;
            write(g_instances[non_active_instance()].request_fds[PIPE_WRITE],
                  &action, sizeof(action));
            g_active_instance = non_active_instance();
            break;
        case PENVISOR_INSTUNLOAD:
            fprintf(stderr, "Got instunload\n");
            fprintf(stderr, "Sending PIPE_COMMAND_EXIT\n");
            action = PIPE_COMMAND_EXIT;
            write(g_instances[non_active_instance()].request_fds[PIPE_WRITE],
                  &action, sizeof(action));
            kill_instance(non_active_instance());
            break;
        }
    }
}

int
main (int argc, const char *argv[])
{
    int pid;
    int rc;
    char *new_stack;

    if (argc > 1) {
        if ((argc == 3) && (strcmp("attach", argv[1]) == 0)) {
            return attach(argv[2]);
        } else if ((argc == 3) && (strcmp("load", argv[1]) == 0)) {
            penvisor_instance_e instance;
            instance = str_to_inst(argv[1]);
            if (instance == -1) {
                fprintf(stderr, "Unknown instance %s. Use 'A' or 'B'\n",
                        argv[1]);
                return -1;
            }
            rc = load_instance(instance, INST_STATUS_LOADED);
            if (rc == 0) {
                printf("Instance %s loaded\n", argv[1]);
            }
            return rc;
        } else {
            fprintf(stderr, "Usage: %s attach {A | B}", argv[0]);
            return -1;
        }
    }
    
    ramfs_mounts();

    boot_script();

    g_active_instance = get_boot_instance();
    printf("Boot instance: %s\n",
           g_active_instance == PENVISOR_INSTANCE_A ? "A" : "B");

    rc = load_instance(g_active_instance, INST_STATUS_ACTIVE);
    if (rc == -1) {
        fprintf(stderr, "oops: load_instance\n");
        return -1;
    }

    run_server();

    return 0;
}
