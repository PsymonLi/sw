// {C} Copyright 2020 Pensando Systems Inc. All rights reserved.

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

#include "../penvisor/penvisor.h"

int
main (int argc, const char *argv[])
{
    int fd;
    int rc;
    struct sockaddr_un addr;
    penvisor_command_t command;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s {load | switch | unload | sysreset}\n",
                argv[0]);
        return -1;
    }
    if (strcasecmp(argv[1], "load") == 0) {
        command.action = PENVISOR_INSTLOAD;
    } else if (strcasecmp(argv[1], "switch") == 0) {
        command.action = PENVISOR_INSTSWITCH;
    } else if (strcasecmp(argv[1], "unload") == 0) {
        command.action = PENVISOR_INSTUNLOAD;
    } else if (strcasecmp(argv[1], "sysreset") == 0) {
        command.action = PENVISOR_SYSRESET;
    } else {
        fprintf(stderr, "Unknown command\n");
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) {
        fprintf(stderr, "Couldn't create socket: %d(%s)\n", errno,
                strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, PENVISOR_PATH, sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

    rc = sendto(fd, &command, sizeof(command), 0, (struct sockaddr *)&addr,
                sizeof(addr));
    if (rc == -1) {
        fprintf(stderr, "Couldn't send command: %d(%s)\n", errno,
                strerror(errno));
        return -1;
    }

    printf("Command sent\n");
    
    return 0;
}
