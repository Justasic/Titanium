#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Include the serialization library
#include "binn.h"

// Include our information_t
#include "misc.h"

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

///////////////////////////////////////
// Function: SerializeData
//
// Description:
// This function is responsible for
// taking the data in the information_t
// structure and converting it to a memory
// block suitable for sending in a series
// of datapackets over a UDP socket.
binn *SerializeData(information_t *info)
{
		void *ptr = NULL;
		size_t alloclen = 0;
		binn *mainmap = binn_map();

		// Add system and boot times.
		binn_map_set_uint64(mainmap, SYSTEM_TIME, info->CurrentTime);
		binn_map_set_uint64(mainmap, BOOT_TIME, info->StartTime);
		
		// Set the list up.
		binn *loadlist = binn_list();
		binn_list_add_float(loadlist, info->Loads[0]); // 5-min
		binn_list_add_float(loadlist, info->Loads[1]); // 15-min
		binn_list_add_float(loadlist, info->Loads[2]); // 30-min
		binn_map_set_list(mainmap, LOADS, loadlist);

		// Seconds idle and uptime.
		binn_map_set_float(mainmap, SECONDS_IDLE, info->SecondsIdle);
		binn_map_set_float(mainmap, SECONDS_UPTIME, info->SecondsUptime);

		// Counts
		binn_map_set_uint32(mainmap, PROC_CNT, info->ProcessCount);
		binn_map_set_uint32(mainmap, RUNNING_PROC_CNT, info->RunningProcessCount);
		binn_map_set_uint32(mainmap, ZOMBIES, info->Zombies);
		binn_map_set_uint32(mainmap, USERCNT, info->UserCount);

		// Hostname
		binn_map_set_str(mainmap, HOSTNAME, info->Hostname);

		// Generate the map for CPU info.
		binn *cpumap = binn_map();
		binn_map_set_str(cpumap, CPU_ARCH, info->cpu_info.Architecture);
		binn_map_set_str(cpumap, CPU_MODEL, info->cpu_info.Model);
		binn_map_set_uint32(cpumap, CPU_CORES, info->cpu_info.Cores);
		binn_map_set_uint32(cpumap, CPU_PHYS_CORES, info->cpu_info.PhysicalCores);
		binn_map_set_float(cpumap, CPU_CUR_SPEED, info->cpu_info.CurrentSpeed);
		binn_map_set_uint32(cpumap, CPU_PERCENT, info->cpu_info.CPUPercent);
		binn_map_set_map(mainmap, CPUINFO, cpumap);

		// Generate the map for Memory Information.
		binn *memorymap = binn_map();
		binn_map_set_uint64(memorymap, MEM_FREE, info->memory_info.FreeRam);
		binn_map_set_uint64(memorymap, MEM_USED, info->memory_info.UsedRam);
		binn_map_set_uint64(memorymap, MEM_TOTAL, info->memory_info.TotalRam);
		binn_map_set_uint64(memorymap, MEM_AVAIL, info->memory_info.AvailRam);
		binn_map_set_uint64(memorymap, MEM_SWAP_FREE, info->memory_info.SwapFree);
		binn_map_set_uint64(memorymap, MEM_SWAP_TOTAL, info->memory_info.SwapTotal);
		binn_map_set_map(mainmap, MEMORY_INFO, memorymap);

		// Now generate the list of hard drives.
		binn *hddlist = binn_list();
		for (hdd_info_t *iter = info->hdd_start; iter; iter = iter->next)
		{
				binn *hdd = binn_map();
				binn_map_set_str(hdd, HDD_NAME, iter->Name);
				binn_map_set_uint64(hdd, HDD_BYTES_WRITTEN, iter->BytesWritten);
				binn_map_set_uint64(hdd, HDD_BYTES_READ, iter->BytesRead);
				binn_map_set_uint64(hdd, HDD_SPACE_AVAIL, iter->SpaceAvailable);
				binn_map_set_uint64(hdd, HDD_PARTITION_SIZE, iter->PartitionSize);
				binn_map_set_str(hdd, HDD_MOUNT, iter->MountPoint);
				binn_map_set_str(hdd, HDD_FS_TYPE, iter->FileSystemType);
				binn_list_add_map(hddlist, hdd);
		}
		binn_map_set_list(mainmap, HDD_INFO, hddlist);

		// Generate the network information
		binn *netlist = binn_list();
		for (network_info_t *iter = info->net_start; iter; iter = iter->next)
		{
				binn *net = binn_map();
				binn_map_set_str(net, NET_IFACE, iter->InterfaceName);
				binn_map_set_str(net, NET_IPV6ADDR, iter->IPv6Address);
				binn_map_set_str(net, NET_IPV4ADDR, iter->IPv4Address);
				binn_map_set_str(net, NET_MAC, iter->MACAddress);
				binn_map_set_str(net, NET_SUBNET, iter->SubnetMask);
				binn_map_set_uint64(net, NET_TXCNT, iter->TX);
				binn_map_set_uint64(net, NET_RXCNT, iter->RX);
				binn_list_add_map(netlist, net);
		}
		binn_map_set_list(mainmap, NETWORK_INFO, netlist);

		// Kernel Information
		binn *kern = binn_map();
		binn_map_set_str(kern, KERN_TYPE, info->kernel_info.Type);
		binn_map_set_str(kern, KERN_VERS, info->kernel_info.Version);
		binn_map_set_str(kern, KERN_REL, info->kernel_info.Release);
		binn_map_set_bool(kern, KERN_TAINTED, info->kernel_info.IsTainted);
		binn_map_set_map(mainmap, KERN_INFO, kern);

		// LSB information
		binn *lsb = binn_map();
		binn_map_set_str(lsb, LSB_VERS, info->lsb_info.Version);
		binn_map_set_str(lsb, LSB_DISTRO, info->lsb_info.Dist_id);
		binn_map_set_str(lsb, LSB_REL, info->lsb_info.Release);
		binn_map_set_str(lsb, LSB_DESC, info->lsb_info.Description);
		binn_map_set_map(mainmap, LSB_INFO, lsb);

		return mainmap;
}
