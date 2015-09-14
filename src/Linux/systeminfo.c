#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h>

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
#include <sys/statvfs.h>   // For filesystem information

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
static int GetKernInfo(information_t *info)
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
    if (!(blah = ReadEntireFile("/proc/sys/kernel/tainted", NULL)))
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
static int GetLSBInfo(information_t *info)
{
    // Idiot check
    assert(info);

    FILE *f = fopen("/etc/lsb-release", "r");
    if (!f)
        return -1;

    info->lsb_info.Dist_id = malloc(1024);     // Anyone who has a DISTRIB_ID longer than a kilobyte is retarded.
    info->lsb_info.Release = malloc(1024);     // same as above
    info->lsb_info.Description = malloc(1024);
    info->lsb_info.Version = malloc(1024);

    while((fgets(buf, sizeof(buf), f)))
    {
        sscanf(buf, "LSB_VERSION=%s", info->lsb_info.Version);
        sscanf(buf, "DISTRIB_ID=%s", info->lsb_info.Dist_id);
        sscanf(buf, "DISTRIB_RELEASE=%s", info->lsb_info.Release);
        sscanf(buf, "DISTRIB_DESCRIPTION=\"%s\"", info->lsb_info.Description);
    }

    fclose(f);
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
static int GetOSRelease(information_t *info)
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

    info->lsb_info.Dist_id = malloc(1024);
    info->lsb_info.Description = malloc(1024);

    while (fgets(buf, sizeof(buf), data))
    {
        char *s = strstr("ID", buf);
        if (s)
            sscanf(s, "ID=\"%s\"", info->lsb_info.Dist_id);

        s = strstr("PRETTY_NAME", buf);
        if (s)
            sscanf(s, "PRETTY_NAME=\"%s\"", info->lsb_info.Description);
    }
    fclose(data);

    info->lsb_info.Dist_id = realloc(info->lsb_info.Dist_id, strlen(info->lsb_info.Dist_id)+1);
    info->lsb_info.Description = realloc(info->lsb_info.Description, strlen(info->lsb_info.Description)+1);
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
static int GetMemoryInfo(information_t *info)
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
        char *s = strstr(buf, "MemTotal: ");
        if (s)
            sscanf(s, "MemTotal: %lu kB\n", &TotalkBMemory);

        s = strstr(buf, "MemFree: ");
        if (s)
            sscanf(s, "MemFree: %lu kB", &FreekBMemory);

        s = strstr(buf, "MemAvailable:");
        if (s)
            sscanf(s, "MemAvailable: %lu kB\n", &AvailRam);

        s = strstr(buf, "SwapTotal:");
        if (s)
            sscanf(s, "SwapTotal: %lu kB\n", &SwapTotal);

        s = strstr(buf, "SwapFree:");
        if (s)
            sscanf(s, "SwapFree: %lu kB\n", &SwapFree);
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
static int GetNetworkInfo(information_t *info)
{
    // Idiot check
    assert(info);
    FILE *f = fopen("/proc/net/dev", "r");
    if (!f)
        return -1;

    while(fgets(buf, sizeof(buf), f))
    {

    }

    fclose(f);

    return 0;
}

// Parse /proc/diskinfo
static int GetDiskInfo(information_t *info)
{
    // Idiot check
    assert(info);

    FILE *f = fopen("/proc/diskstats", "r");
    if (!f)
    {
        perror("fopen");
        return -1;
    }

    #if 0
    Parsing the lines are fairly easy. The format of each line goes as follows:
    fields:
     1 - major number
     2 - minor mumber
     3 - device name
     4 - reads completed successfully
     5 - reads merged
     6 - sectors read
     7 - time spent reading (ms)
     8 - writes completed
     9 - writes merged
    10 - sectors written
    11 - time spent writing (ms)
    12 - I/Os currently in progress
    13 - time spent doing I/Os (ms)
    14 - weighted time spent doing I/Os (ms)

    Field  1 -- # of reads completed
        This is the total number of reads completed successfully.
    Field  2 -- # of reads merged, field 6 -- # of writes merged
        Reads and writes which are adjacent to each other may be merged for
        efficiency.  Thus two 4K reads may become one 8K read before it is
        ultimately handed to the disk, and so it will be counted (and queued)
        as only one I/O.  This field lets you know how often this was done.
    Field  3 -- # of sectors read
        This is the total number of sectors read successfully.
    Field  4 -- # of milliseconds spent reading
        This is the total number of milliseconds spent by all reads (as
        measured from __make_request() to end_that_request_last()).
    Field  5 -- # of writes completed
        This is the total number of writes completed successfully.
    Field  6 -- # of writes merged
        See the description of field 2.
    Field  7 -- # of sectors written
        This is the total number of sectors written successfully.
    Field  8 -- # of milliseconds spent writing
        This is the total number of milliseconds spent by all writes (as
        measured from __make_request() to end_that_request_last()).
    Field  9 -- # of I/Os currently in progress
        The only field that should go to zero. Incremented as requests are
        given to appropriate struct request_queue and decremented as they finish.
    Field 10 -- # of milliseconds spent doing I/Os
        This field increases so long as field 9 is nonzero.
    Field 11 -- weighted # of milliseconds spent doing I/Os
        This field is incremented at each I/O start, I/O completion, I/O
        merge, or read of these stats by the number of I/Os in progress
        (field 9) times the number of milliseconds spent doing I/O since the
        last update of this field.  This can provide an easy measure of both
        I/O completion time and the backlog that may be accumulating.
    #endif

    hdd_info_t *iter;
    info->hdd_start = iter = malloc(sizeof(hdd_info_t));

    while(fgets(buf, sizeof(buf), f))
    {
        // Variables
        uint32_t major, minor;
        size_t CompletedReads, MergedReads, SectorsRead, TimeSpentReading, CompletedWrites,
        MergedWrites, SectorsWritten, TimeSpentWriting, IOInProgress, TimeSpentIOProcessing,
        WeightedTimeSpentIOProcessing;
        char bufstr[sizeof(buf)];

        // Get the data (yes this is nasty.)
        sscanf(buf, "%d %d %s %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
        &major, &minor, bufstr, &CompletedReads, &MergedReads, &SectorsRead, &TimeSpentReading,
        &CompletedWrites, &MergedWrites, &SectorsWritten, &TimeSpentWriting,
        &IOInProgress, &TimeSpentIOProcessing, &WeightedTimeSpentIOProcessing);

        // Fill out the struct.
        memset(iter, 0, sizeof(hdd_info_t));


        iter->Name = strdup(bufstr);
        iter->BytesRead = CompletedReads;
        iter->BytesWritten = CompletedWrites;
        iter->next = malloc(sizeof(hdd_info_t));
        iter = iter->next;
    }

    fclose(f);
    f = fopen("/proc/partitions", "r");
    if (!f)
    {
        perror("fopen");
        goto end;
    }

    // Parse /proc/partitions
    while (fgets(buf, sizeof(buf), f))
    {
        // the format of this file was a bit confusing but the procfs man page says
        // the format is as follows:
        // major  minor  1024-byte blockcnt    part-name
        //   8      0      3907018584             sda

        uint32_t major, minor;
        size_t blocks;
        char buffer[8192];

        // Skip 1st line.
        static int cnt = 0;
        if (!cnt)
        {
            cnt++;
            continue;
        }

        // Parse the values
        sscanf(buf, "%d, %d, %lu, %s", &major, &minor, &blocks, &buffer);

        // make the byte count and add the value to the array.
        for (iter = info->hdd_start; iter; iter = iter->next)
        {
            if (!strcmp(iter->Name, buffer))
                iter->PartitionSize = blocks * 1024;
        }

    }

    #if 0
    typedef struct hdd_info_s
    {
    	// TODO
    	char *Name; // Device/partition name.
    	size_t BytesWritten;
    	size_t BytesRead;

    	size_t SpaceAvailable; // In bytes
    	size_t SpaceUsed; // in bytes
    	size_t PartitionSize; // in bytes
    	char *MountPoint;
    	char *FileSystemType; // NTFS or FAT32 on windows.
    	struct hdd_info_s *next;
    } hdd_info_t;
    #endif

end:

    fclose(f);
    return 0;
}

// Parse /proc/cpuinfo
static int GetCPUInfo(information_t *info)
{
    // Idiot check
    assert(info);

    return 0;
}

///////////////////////////////////////////////////
// Function: GetLoadAvg
//
// description:
// This function gets the load averages. It also
// gets the last pid and a few scheduler stats
// which may be useful in the future (for now they
// are ignored).
static int GetLoadAvg(information_t *info)
{
    // Idiot check
    assert(info);
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f)
        return -1;

    unsigned int _unused sched_runnable = 0;
    unsigned int _unused sched_existed = 0;
    pid_t _unused lastpid = 0;

    fscanf(f, "%f %f %f %u/%u %lu", &info->Loads[0], &info->Loads[1], &info->Loads[2], &sched_runnable, &sched_existed, (unsigned long*)&lastpid);
    return 0;
}

///////////////////////////////////////////////////
// Function: GetStatisticalInfo
//
// description:
// This Function gets the CPU percentage, process
// count, processes active, time since boot in
// EPOCH format, and processes waiting on I/O.
static int GetStatisticalInfo(information_t *info)
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
        sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user_j, &nice_j, &sys_j, &idle_j, &wait_j, &irq_j, &sirq_j, &stolen_j, &guest_j, &gnice_j);
        // time since kernel started in EPOCH format
        sscanf(buf, "btime %lu", &info->StartTime);
        // Number of running processes on the system
        sscanf(buf, "processes %lu", &info->ProcessCount);
        // Number of ACTIVE running processes on the system
        sscanf(buf, "procs_running %lu", &info->RunningProcessCount);
        // Number of proceses waiting on system I/O operations.
        sscanf(buf, "procs_blocked %lu", &info->Zombies);
    }

    // Sleep while we wait for the kernel to do some stuff
    usleep(900000);

    // Now get it all again.
    fclose(f);
    f = fopen("/proc/stat", "r");
    fscanf(f, "cpu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &user_k, &nice_k, &sys_k, &idle_k, &wait_k, &irq_k, &sirq_k, &stolen_k, &guest_k, &gnice_k);

    // Calculate the difference and produce a CPU percentage.
    diff_user = user_k - user_j;
    diff_nice = nice_k - nice_j;
    diff_system = sys_k - sys_j;
    diff_idle = idle_k - idle_j;

    info->cpu_info.CPUPercent = (unsigned int)(((float)(diff_user + diff_nice + diff_system))/((float)(diff_user + diff_nice + diff_system + diff_idle))*100.0);

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
information_t *GetSystemInformation()
{
    information_t *info = malloc(sizeof(information_t));
    memset(info, 0, sizeof(information_t));
    info->CurrentTime = time(NULL);

    if (GetLoadAvg(info) != 0)
        goto fail;

    if (GetLSBInfo(info) != 0)
    {
        if (GetOSRelease(info) != 0)
        {
            // Allocate a string so we don't free non-freeable memory.
            info->lsb_info.Dist_id = info->lsb_info.Version =
            info->lsb_info.Release = info->lsb_info.Description = strdup("Unknown");
        }
    }

    if (GetDiskInfo(info) != 0)
    goto fail;

    if (GetMemoryInfo(info) != 0)
        goto fail;

    if (GetStatisticalInfo(info) != 0)
        goto fail;

    if (GetKernInfo(info) != 0)
        goto fail;

    return info;
fail:
    FreeSystemInformation(info);
    return NULL;
}
