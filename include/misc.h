#pragma once

#include <stdint.h>

extern char *SizeReduce(size_t size);
extern char *ReadEntireFile(const char *filepath, size_t *size);

typedef struct information_s
{
	// CPU, RAM, Load time, Distro info, Uptime, Kernel Version, hard drive info,
	// # of total processes, # of active processes, # of users, hostname, current time, IPv4 Address(es),
	// IPv6 Address(es), Mac address(es), Interface names, TX/RX counts, subnet/cidr masks
	// kernel command line options (if available), CPU Architecture
	time_t CurrentTime;
	time_t StartTime; // Seconds in EPOCH format since the system booted.
	float loads[3]; // Null on windows. -- for now.
    float SecondsIdle; // Seconds spent idle (idfk why the kernel gives it as a float)
    float SecondsUptime; // ???
	unsigned long ProcessCount;
    unsigned long RunningProcessCount;
    unsigned long Zombies;
	unsigned long UserCount;
	const char *Hostname;

    struct
    {
        const char *Architecture;   // arm, i386, x86_64, etc.
        const char *Model;          // Model from the kernel (eg, Intel(R) Core(TM) i7-4930K CPU @ 3.40GHz)
        unsigned int Cores;         // How many logical processors the kernel sees (including hyperthreaded ones)
        unsigned int PhysicalCores; // How many physical cores exist on the die
        float CurrentSpeed;         // Current speed of the CPU.
        unsigned int CPUPercent;    // Calculated by us.
    } cpu_info;

    struct
    {
        uint64_t FreeRam; // In bytes
        uint64_t UsedRam; // In bytes
        unsigned int PercentUsed;
        unsigned int PercentFree;
    } memory_info;

	// TODO: Hard drive info.
    struct network_info_s
    {
        const char *InterfaceName;
        const char IPv6Address[INET6_ADDRSTRLEN];
        const char IPv4Address[INET_ADDRSTRLEN];
        const char MACAddress[17]; // Includes colons
        const char SubnetMask[INET_ADDRSTRLEN]; // This is the IPv4 Subnet Mask and IPv6 CIDR mask
        uint64_t TX;
        uint64_t RX;
        struct network_info_s *next;
    } network_info, *start;

    struct
    {
        const char *type;
        const char *version;
        const char *release;
        const uint8_t IsTainted;
    } kernel_info;

	struct
	{
		const char *version;
		const char *dist_id;
		const char *release;
		const char *description;
	} lsb_info;
} information_t;

// Gets the system info for each platform depending on what it is compiled for
extern information_t *GetSystemInformation(NULL);
