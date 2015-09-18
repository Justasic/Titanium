#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Include the serialization library
extern "C" {
  #include "binn.h"
}

// An enumoration of all the values so we can make a map
// out of it then send all the data over a socket.
enum
{
		SYSTEM_TIME = 1,
		BOOT_TIME,
		LOADS,            // This is a list of 3 floats
		SECONDS_IDLE,
		SECONDS_UPTIME,
		PROC_CNT,
		RUNNING_PROC_CNT,
		ZOMBIES,
		USERCNT,
		HOSTNAME,
		CPUINFO,         // nested map.
		MEMORY_INFO,     // Nested map.
		HDD_INFO,        // List of nested maps.
		NETWORK_INFO,    // List of nested maps.
		KERN_INFO,       // nested map.
		LSB_INFO         // nested map.
};

// CPU information indexes
enum
{
		CPU_ARCH = 1,
		CPU_MODEL,
		CPU_CORES,
		CPU_PHYS_CORES,
		CPU_CUR_SPEED,
		CPU_PERCENT
};

// Memory Information indexes
enum
{
		MEM_FREE = 1,
		MEM_USED,
		MEM_TOTAL,
		MEM_AVAIL,
		MEM_SWAP_FREE,
		MEM_SWAP_TOTAL
};

// Hard drive information indexes
enum
{
		HDD_NAME = 1,
		HDD_BYTES_WRITTEN,
		HDD_BYTES_READ,
		HDD_SPACE_AVAIL,
		HDD_SPACE_USED,
		HDD_PARTITION_SIZE,
		HDD_MOUNT,
		HDD_FS_TYPE,
};

// Network information indexes
enum
{
		NET_IFACE = 1,
		NET_IPV6ADDR,
		NET_IPV4ADDR,
		NET_MAC,
		NET_SUBNET,
		NET_TXCNT,
		NET_RXCNT,
};

// kernel information indexes
enum
{
		KERN_TYPE = 1,
		KERN_VERS,
		KERN_REL,
		KERN_TAINTED
};

// LSB information indexes
enum
{
		LSB_VERS = 1,
		LSB_DISTRO,
		LSB_REL,
		LSB_DESC
};

information_t *UnserializeData(void *ptr, size_t len)
{
		information_t *info = new information_t;

		// Unserialize the generic data first.
		info->CurrentTime = binn_map_uint64(ptr, SYSTEM_TIME);
		info->StartTime = binn_map_uint64(ptr, BOOT_TIME);

		// Unserialize load times.
		binn *loadlist = binn_map_list(ptr, LOADS);
		info->Loads[0] = binn_list_float(loadlist, 0); // 5-min
		info->Loads[1] = binn_list_float(loadlist, 1); // 15-min
		info->Loads[2] = binn_list_float(loadlist, 2); // 30-min

		// More generic data
		info->SecondsIdle = binn_map_float(ptr, SECONDS_IDLE);
		info->SecondsUptime = binn_map_float(ptr, SECONDS_UPTIME);
		info->ProcessCount = binn_map_uint32(ptr, PROC_CNT);
		info->RunningProcessCount = binn_map_uint32(ptr, RUNNING_PROC_CNT);
		info->Zombies = binn_map_uint32(ptr, ZOMBIES);
		info->UserCount = binn_map_uint32(ptr, USERCNT);
		info->Hostname = binn_map_str(ptr, HOSTNAME);

		// Unserialize the CPU information
		binn *cpuinfo = binn_map_map(ptr, CPUINFO);
		info->cpu_info.Architecture = binn_map_str(cpuinfo, CPU_ARCH);
		info->cpu_info.Model = binn_map_str(cpuinfo, CPU_MODEL);
		info->cpu_info.Cores = binn_map_uint32(cpuinfo, CPU_CORES);
		info->cpu_info.PhysicalCores = binn_map_uint32(cpuinfo, CPU_PHYS_CORES);
		info->cpu_info.CurrentSpeed = binn_map_float(cpuinfo, CPU_CUR_SPEED);
		info->cpu_info.CPUPercent = binn_map_uint32(cpuinfo, CPU_PERCENT);

		// Unserialize the memory information
		binn *meminfo = binn_map_map(ptr, MEMORY_INFO);
		info->memory_info.FreeRam   = binn_map_uint64(meminfo, MEM_FREE);
		info->memory_info.UsedRam   = binn_map_uint64(meminfo, MEM_USED);
		info->memory_info.TotalRam  = binn_map_uint64(meminfo, MEM_TOTAL);
		info->memory_info.AvailRam  = binn_map_uint64(meminfo, MEM_AVAIL);
		info->memory_info.SwapFree  = binn_map_uint64(meminfo, MEM_SWAP_FREE);
		info->memory_info.SwapTotal = binn_map_uint64(meminfo, MEM_SWAP_TOTAL);

		// Unserialize hard drive information.
		// First get the list of maps.
		binn *hddlist = binn_map_list(ptr, HDD_INFO);
}
