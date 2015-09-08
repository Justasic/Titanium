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

// Based on https://gitlab.com/procps-ng/procps/blob/master/proc/sysinfo.c
static char buf[8192];

// Get info on the kernel
int GetKernInfo(information_t)
{
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

///////////////////////////////////////////////////
// Function: GetLSBInfo
//
// description:
// Parses the information from /etc/lsb-release.
// It is optional to have this information included.
void GetLSBInfo(information_t *info)
{
    // Idiot check
    assert(info);

    FILE *data = fopen("/etc/lsb-release", "r");
    if (!data)
        return -1;

    info->lsb_info.dist_id = malloc(1024);     // Anyone who has a DISTRIB_ID longer than a kilobyte is retarded.
    info->lsb_info.release = malloc(1024);     // same as above
    info->lsb_info.description = malloc(1024);

    while((fgets(buf, sizeof(buf), f)))
    {
        sscanf(buf, "LSB_VERSION=%f", &info->lsb_info.version);
        sscanf(buf, "DISTRIB_ID=%s", info->lsb_info.dist_id);
        sscanf(buf, "DISTRIB_RELEASE=%s", info->lsb_info.release);
        sscanf(buf, "DISTRIB_DESCRIPTION=\"%s\"", info->lsb_info.description);
    }

    free(data);
    return 0;
}

///////////////////////////////////////////////////
// Function: GetOSRelease
//
// description:
// Similar to GetLSBInfo, this gets information from
// /etc/os-release and tries to parse it. It will
// also handle special cases for certain popular
// distros which change the name away from os-release.
// Like GetLSBInfo this is also optional
void GetOSRelease(information_t *info)
{
    char *data = ReadEntireFile("/etc/os-release", "r");
    if (!data && (errno == EEXIST || errno == EACCES))
    {
        // Try to handle the special snowflake distros
        data = ReadEntireFile("/etc/gentoo-release", "r");
        if (!data)
        {
            data = ReadEntireFile("/etc/fedora-release", "r");
            if (!data)
            {
                data = ReadEntireFile("/etc/redhat-release", "r");
                if (!data)
                {
                    data = ReadEntireFile("/etc/debian_version", "r");
                    if (!data)
                        return -1; // Okay we're done. If you're this much of a special snowflake then you can go fuck yourself.
                }
            }
        }
    }
    else if (!data)
        return -1; // we failed and don't know why.


    sscanf(data, "ID=%s", info->lsb_info.dist_id)
    // TODO
}

// Parse /proc/meminfo
int GetMemoryInfo(information_t *info)
{
    // Idiot check
    assert(info);
}

// Parse /proc/cpuinfo
int GetCPUInfo(information_t *info)
{
    // Idiot check
    assert(info);
}

// Parse /proc/loadavg
int GetLoadAvg(information_t *info)
{
    // Idiot check
    assert(info);
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
    // Idiot check
    assert(info);

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

    // Get the information.
    fscanf(f, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", &user_j, &nice_j, &sys_j, &idle_j, &wait_j, &irq_j, &sirq_j, &stolen_j, &guest_j, &gnice_j);
    // time since kernel started in EPOCH format
    fscanf(f, "btime %Lu", info->StartTime);
    // Number of running processes on the system
    fscanf(f, "processes %Lu", info->ProcessCount);
    // Number of ACTIVE running processes on the system
    fscanf(f, "procs_running %Lu", info->RunningProcessCount);
    // Number of proceses waiting on system I/O operations.
    fscanf(f, "procs_blocked %Lu", info->Zombies);
    return 0;
}

///////////////////////////////////////////////////
// Function: GetSystemInformation (Linux variant)
//
// description:
// Collects the system information for the platform
// and fills out the platform-independent information_t
// structure. This function is called in main.c
information_t *GetSystemInformation(NULL)
{
    information_t *info = malloc(sizeof(information_t));
    memset(info, 0, sizeof(information_t));

    if (GetLoadAvg(info) != 0)
        goto fail;

    if (GetStatisticalInfo(info) != 0)
        goto fail;

    return info;
fail:
    if (info->hostname)
        free(info->hostname);

    if (info->lsb_info.version)
        free(info->lsb_info.version);

    if (info->lsb_info.dist_id)
        free(info->lsb_info.dist_id);

    if (info->lsb_info.release)
        free(info->lsb_info.release);

    if (info->lsb_info.description)
        free(info->lsb_info.description);

    free(info);
    return NULL;
}
