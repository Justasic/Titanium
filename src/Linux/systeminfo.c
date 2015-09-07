#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>

// Linux-specific includes.
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/select.h>      // also available via <sys/types.h>
#include <sys/time.h>
#include <sys/types.h>       // also available via <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <netdb.h>

#include "misc.h"

#define _unused __attribute((unused))

// Kernel info struct
struct
{
    const char *buildinfo;
    const char *version;
    const char *type; // Obviously going to be linux but someone might be a special snowflake and change this
    const char *hostname;
    const uint8_t IsTainted; // If the kernel has a non-oss module loaded. See https://www.kernel.org/doc/Documentation/oops-tracing.txt
} kernel_t;

// Get info on the kernel
kernel_t *GetKernInfo(NULL)
{
    kernel_t *info = malloc(sizeof(kernel_t));
    char *blah = NULL;
    if (!(info->buildinfo = ReadEntireFile("/proc/sys/kernel/version", NULL)))
        goto fail;

    if (!(info->version = ReadEntireFile("/proc/sys/kernel/osrelease", NULL)))
        goto fail;

    if (!(info->type = ReadEntireFile("/proc/sys/kernel/ostype", NULL)))
        goto fail;

    if (!(info->hostname = ReadEntireFile("/proc/sys/kernel/hostname", NULL)))
        goto fail;

    // Make the boolean value
    if (!(blah = ReadEntireFile("/proc/sys/kernel/hostname", NULL)))
        goto fail;

    // Are we tainted?
    info->IsTainted = (atoi(blah) != 0);
    free(blah);
    return info;
fail:
    free(info);
    return NULL;
}

// Parse /proc/meminfo
int GetMemoryInfo(information_t *info)
{

}

// Parse /proc/cpuinfo
int GetCPUInfo(information_t *info)
{

}

// Parse /proc/loadavg
int GetLoadAvg(information_t *info)
{
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f)
        return -1;

    unsigned int _unused sched_runnable = 0;
    unsigned int _unused sched_existed = 0;
    pid_t _unused lastpid = 0;

    fscanf(f, "%f %f %f %u/%u %Lu", &info->float[0], &info->float[1], &info->float[2], &sched_runnable, &sched_existed, (unsigned long*)&lastpid);
    return 0;
}

// Parse /proc/stats
int GetStatisticalInfo(information_t *info)
{

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;

    unsigned long user_j, // Time spent in user mode
    nice_j,               // Time spent in user mode with low priority
    sys_j,                // Time spent in kernel space
    idle_j,               // Time spent idling the CPU
    wait_j,               // Time spent in waiting for I/O operations
    irq_j,                // Time spent servicing interrupts
    sirq_j,               // Time spent servcing softirqs
    stolen_j,             // Time spent in other operating systems when running in a virtual environment
    guest_j,              // Time spent running a virtual CPU for guests in KVM
    gnice_j;              // Time spent running niced guest vCPU in KVM

    fscanf(f, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", &user_j, &nice_j, &sys_j, &idle_j, &wait_j, &irq_j, &sirq_j, &stolen_j, &guest_j, &gnice_j);
    fscanf(f, "btime %Lu", info->StartTime);
    fscanf(f, "processes %Lu", info->ProcessCount);
    fscanf(f, "procs_running %Lu", info->RunningProcessCount);
    fscanf(f, "procs_blocked %Lu", info->Zombies);
    return 0;
}

// Main function for getting system info
information_t *GetSystemInformation(NULL)
{
    information_t *info = malloc(sizeof(information_t));

    if (GetLoadAvg(info) != 0)
        goto fail;

    if (GetStatisticalInfo(info))


    return info;
fail:
    free(info);
    return NULL;
}
