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

#define _unused __attribute__((unused))

// Based on https://gitlab.com/procps-ng/procps/blob/master/proc/sysinfo.c
static char buf[8192];

///////////////////////////////////////////////////
// Function: GetKernInfo
//
// description:
// Parses several files to get kernel versions,
// when the kernel was built, what OS this is,
// the hostname, and whether the kernel is tainted.
int GetKernInfo(information_t *info)
{
    char *blah = NULL;
    if (!(info->kernel_info.Version = ReadEntireFile("/proc/sys/kernel/version", NULL)))
        goto fail;

    if (!(info->kernel_info.Release = ReadEntireFile("/proc/sys/kernel/osrelease", NULL)))
        goto fail;

    if (!(info->kernel_info.Type = ReadEntireFile("/proc/sys/kernel/ostype", NULL)))
        goto fail;

    if (!(info->Hostname = ReadEntireFile("/proc/sys/kernel/hostname", NULL)))
        goto fail;

    // Make the boolean value
    if (!(blah = ReadEntireFile("/proc/sys/kernel/hostname", NULL)))
        goto fail;

    // Are we tainted?
    info->kernel_info.IsTainted = (atoi(blah) != 0);
    free(blah);
    return 0;
fail:
    free(info);
    return -1;
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

    FILE *f = fopen("/etc/lsb-release", "r");
    if (!f)
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
    FILE *data = fopen("/etc/os-release", "r");
    if (!data && (errno == EEXIST || errno == EACCES))
    {
        // Try to handle the special snowflake distros
        data = fopen("/etc/gentoo-release", "r");
        if (!data)
        {
            data = fopen("/etc/fedora-release", "r");
            if (!data)
            {
                data = fopen("/etc/redhat-release", "r");
                if (!data)
                {
                    data = fopen("/etc/debian_version", "r");
                    if (!data)
                        return -1; // Okay we're done. If you're this much of a special snowflake then you can go fuck yourself.
                }
            }
        }
    }
    else if (!data)
        return -1; // we failed and don't know why.

    info->lsb_info.dist_id = malloc(1024);
    info->lsb_info.description = malloc(1024);

    while (fgets(buf, sizeof(buf), data))
    {
        char *s = strstr("ID", buf);
        if (s)
            sscanf(s, "ID=\"%s\"", info->lsb_info.dist_id);

        s = strstr("PRETTY_NAME", buf);
        if (s)
            sscanf(s, "PRETTY_NAME=\"%s\"", info->lsb_info.description);
    }
    fclose(data);

    info->lsb_info.dist_id = realloc(info->lsb_info.dist_id, strlen(info->lsb_info.dist_id)+1);
    info->lsb_info.description = realloc(info->lsb_info.description, strlen(info->lsb_info.description)+1);
    info->lsb_info.Version = strdup("0.0");
    return 0;
}

///////////////////////////////////////////////////
// Function: GetMemoryInfo
//
// description:
// Parses /proc/meminfo for information on the RAM
// and swap usage. This also converts it from
// kilobytes to bytes.
int GetMemoryInfo(information_t *info)
{
    // Idiot check
    assert(info);

    FILE *data = fopen("/proc/meminfo", "r");
    if (!data)
        return -1;

    uint64_t TotalkBMemory, UsedkBMemory, FreekBMemory, AvailRam, SwapFree, SwapTotal;
    TotalkBMemory = FreekBMemory = UsedkBMemory = AvailRam = SwapFree = SwapTotal = 0;

    while (fgets(buf, sizeof(buf), data))
    {
        char *s = strstr("MemTotal:", buf);
        if (s)
            sscanf(s, "MemTotal: %Lu kB", &TotalkBMemory);

        s = strstr("MemFree:", buf);
        if (s)
            sscanf(s, "MemFree: %Lu kB", &FreekBMemory);

        s = strstr("MemAvailable:", buf);
        if (s)
            sscanf(s, "MemAvailable: %Lu kB", &AvailRam);

        s = strstr("SwapTotal:", buf);
        if (s)
            sscanf(s, "SwapTotal: %Lu kB", &SwapTotal);

        s = strstr("SwapFree:", buf);
        if (s)
            sscanf(s, "SwapFree: %Lu kB", &SwapFree);
    }

    // Get used memory.
    UsedkBMemory = TotalkBMemory - FreekBMemory;

    // Shift by 10 to get byte counts instead of kilobyte counts.
    info->memory_info.FreeRam = (FreekBMemory << 10);
    info->memory_info.UsedRam = (UsedkBMemory << 10);
    info->memory_info.TotalRam = (TotalkBMemory << 10);
    info->memory_info.AvailRam = (AvailRam << 10);
    info->memory_info.SwapFree = (SwapFree << 10);
    info->memory_info.SwapTotal = (SwapTotal << 10);

    // cleanup and return.
    fclose(data);
    return 0;
}

// Parse /proc/net/dev
int GetNetworkInfo(information_t *info)
{
    // Idiot check
    assert(info);

}

// Parse /proc/diskinfo
int GetDiskInfo(information_t *info)
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

///////////////////////////////////////////////////
// Function: GetLoadAvg
//
// description:
// This function gets the load averages. It also
// gets the last pid and a few scheduler stats
// which may be useful in the future (for now they
// are ignored).
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

///////////////////////////////////////////////////
// Function: GetStatisticalInfo
//
// description:
// This Function gets the CPU percentage, process
// count, processes active, time since boot in
// EPOCH format, and processes waiting on I/O.
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

    unsigned long user_k, // Time spent in user mode
    nice_k,               // Time spent in user mode with low priority
    sys_k,                // Time spent in kernel space
    idle_k,               // Time spent idling the CPU
    wait_k,               // Time spent in waiting for I/O operations
    irq_k,                // Time spent servicing interrupts
    sirq_k,               // Time spent servcing softirqs
    stolen_k,             // Time spent in other operating systems when running in a virtual environment
    guest_k,              // Time spent running a virtual CPU for guests in KVM
    gnice_k;              // Time spent running niced guest vCPU in KVM

    // Differences.
    unsigned long diff_user, diff_system, diff_nice, diff_idle;

    // Get the information.
    while(fgets(buf, sizeof(buf), f))
    {
        sscanf(buf, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", &user_j, &nice_j, &sys_j, &idle_j, &wait_j, &irq_j, &sirq_j, &stolen_j, &guest_j, &gnice_j);
        // time since kernel started in EPOCH format
        sscanf(buf, "btime %Lu", &info->StartTime);
        // Number of running processes on the system
        sscanf(buf, "processes %Lu", &info->ProcessCount);
        // Number of ACTIVE running processes on the system
        sscanf(buf, "procs_running %Lu", &info->RunningProcessCount);
        // Number of proceses waiting on system I/O operations.
        sscanf(buf, "procs_blocked %Lu", &info->Zombies);
    }

    // Sleep while we wait for the kernel to do some stuff
    usleep(900000);

    // Now get it all again.
    fclose(f);
    f = fopen("/proc/stat", "r");
    fscanf(f, "cpu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu", &user_k, &nice_k, &sys_k, &idle_k, &wait_k, &irq_k, &sirq_k, &stolen_k, &guest_k, &gnice_k);

    // Calculate the difference and produce a CPU percentage.
    diff_user = user_k - user_j;
    diff_nice = nice_k - nice_j;
    diff_system = sys_k - sys_j;
    diff_idle = idle_k - idle_j;

    info->CPUPercent = (unsigned int)(((float)(diff_user + diff_nice + diff_system))/((float)(diff_user + diff_nice + diff_system + diff_idle))*100.0);

    fclose(f);
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

    if (GetLSBInfo(info) != 0)
    {
        if (GetOSRelease(info) != 0)
        {
            // Allocate a string so we don't free non-freeable memory.
            info->lsb_info.dist_id = info->lsb_info.Version =
            info->lsb_info.release = info->lsb_info.description = strdup("Unknown");
        }
    }

    if (GetMemoryInfo(info) != 0)
        goto fail;

    if (GetStatisticalInfo(info) != 0)
        goto fail;

    if (GetKernInfo(info) != 0)
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
