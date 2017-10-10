#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cstring>
#include "Unserialize.h"
#include "json.hpp"

///////////////////////////////////////////////
// Function: UnserializeData
//
// Description:
// Accept a block of data and unserialize that data
// into an information_t structure so we can use
// the data internally for graphing and such.
information_t *UnserializeData(const std::vector<uint8_t> data)
{
        using namespace nlohmann;
        // Allocate a new structure.
		information_t *info = new information_t;
        // Get the data from the message pack.
        json message = json::from_msgpack(data);

        // Debug.
        #ifdef _DEBUG
        tfm::printf("Received the following MessagePack data:\n\n=========BEGIN=========\n%s\n==========END==========\n\n", json.dump(4));
        #endif

        // Unserialize the data.
        /// Get current system time and time since system boot.
        info->CurrentTime = message["SYSTEM_TIME"];
        info->StartTime = message["BOOT_TIME"];

        // Get current loadtimes.
        info->Loads[0] = message["LOADS"][0];
        info->Loads[1] = message["LOADS"][1];
        info->Loads[2] = message["LOADS"][2];

        // More generic data
        info->SecondsIdle         = message["SECONDS_IDLE"];
        info->SecondsUptime       = message["SECONDS_UPTIME"];
        info->ProcessCount        = message["PROC_CNT"];
        info->RunningProcessCount = message["RPROC_CNT"];
        info->Zombies             = message["ZOMBIES"];
        info->UserCount           = message["USRCNT"];
        info->Hostname            = message["HOSTNAME"];

        // Get the CPU information (model, cores, etc.)
        info->cpu_info.Architecture  = message["CPU"]["ARCH"];
        info->cpu_info.Model         = message["CPU"]["MODEL"];
        info->cpu_info.Cores         = message["CPU"]["CORES"];
        info->cpu_info.PhysicalCores = message["CPU"]["PCORES"];
        info->cpu_info.CurrentSpeed  = message["CPU"]["CURSPEED"];
        info->cpu_info.CPUPercent    = message["CPU"]["PERCENT"];

        // Unserialize the memory information
        info->memory_info.FreeRam   = message["MEM"]["FREE"];
        info->memory_info.UsedRam   = message["MEM"]["USED"];
        info->memory_info.TotalRam  = message["MEM"]["TOTAL"];
        info->memory_info.AvailRam  = message["MEM"]["AVAIL"];
        info->memory_info.SwapFree  = message["MEM"]["SWAP_FREE"];
        info->memory_info.SwapTotal = message["MEM"]["SWAP_TOTAL"];

		// Unserialize the hard drive device list/mount list.
		for (json::iterator it = message["HDD"].begin(), it_end = message["HDD"].end(); it != it_end; ++it)
		{
            json mount = *it;
			hdd_info_t *hdd = new hdd_info_t;
			hdd->Name           = mount["NAME"];
            hdd->BytesWritten   = mount["BYTES_WRITTEN"];
            hdd->BytesRead      = mount["BYTES_READ"];
            hdd->SpaceAvailable = mount["SPACE_AVAIL"];
            hdd->SpaceUsed      = mount["SPACE_USED"];
            hdd->PartitionSize  = mount["PART_SIZE"];
            hdd->MountPoint     = mount["MOUNT"];
            hdd->FileSystemType = mount["FS_TYPE"];
            info->hdd_start.push_back(hdd);
		}

		// Unserialize the network interface list.
		for (json::iterator it = message["NETWORK"].begin(), it_end = message["NETWORK"].end(); it != it_end; ++it)
		{
            json interface = *it;
			network_info_t *net = new network_info_t;
            net->InterfaceName = interface["IFACE"];
            net->IPv6Address   = interface["IPV6ADDR"];
            net->IPv4Address   = interface["IPV4ADDR"];
            net->MACAddress    = interface["MAC"];
            net->SubnetMask    = interface["SUBNET"];
            net->TX            = interface["TX"];
            net->RX            = interface["RX"];
            info->net_start.push_back(net);
		}

		// Unserialize the kernel info.
		info->kernel_info.Type      = message["KERN_TYPE"];
		info->kernel_info.Version   = message["KERN_VERS"];
		info->kernel_info.Release   = message["KERN_REL"];
		info->kernel_info.IsTainted = message["KERN_TAINTED"];

		// Unserialize LSB Information
		info->lsb_info.Version     = message["LSB_VERS"];
		info->lsb_info.Dist_id     = message["LSB_DISTRO"];
		info->lsb_info.Release     = message["LSB_REL"];
		info->lsb_info.Description = message["LSB_DESC"];

#if 0

		// Unserialize the generic data first.
		info->CurrentTime = binn_map_uint64(ptr, SYSTEM_TIME);
		info->StartTime = binn_map_uint64(ptr, BOOT_TIME);

		// Unserialize load times.
		binn *loadlist = (binn*)binn_map_list(ptr, LOADS);
		info->Loads[0] = binn_list_float(loadlist, 0); // 5-min
		info->Loads[1] = binn_list_float(loadlist, 1); // 15-min
		info->Loads[2] = binn_list_float(loadlist, 2); // 30-min

		// More generic data
		info->SecondsIdle         = binn_map_float(ptr, SECONDS_IDLE);
		info->SecondsUptime       = binn_map_float(ptr, SECONDS_UPTIME);
		info->ProcessCount        = binn_map_uint32(ptr, PROC_CNT);
		info->RunningProcessCount = binn_map_uint32(ptr, RUNNING_PROC_CNT);
		info->Zombies             = binn_map_uint32(ptr, ZOMBIES);
		info->UserCount           = binn_map_uint32(ptr, USERCNT);
        char *str = binn_map_str(ptr, HOSTNAME);



        size_t strlength = strnlen(str, len);
		info->Hostname            = std::string(str, strlength);

		// Unserialize the CPU information
		binn *cpuinfo = (binn*)binn_map_map(ptr, CPUINFO);
		info->cpu_info.Architecture  = binn_map_str(cpuinfo, CPU_ARCH);
		info->cpu_info.Model         = binn_map_str(cpuinfo, CPU_MODEL);
		info->cpu_info.Cores         = binn_map_uint32(cpuinfo, CPU_CORES);
		info->cpu_info.PhysicalCores = binn_map_uint32(cpuinfo, CPU_PHYS_CORES);
		info->cpu_info.CurrentSpeed  = binn_map_float(cpuinfo, CPU_CUR_SPEED);
		info->cpu_info.CPUPercent    = binn_map_uint32(cpuinfo, CPU_PERCENT);

		// Unserialize the memory information
		binn *meminfo = (binn*)binn_map_map(ptr, MEMORY_INFO);
		info->memory_info.FreeRam   = binn_map_uint64(meminfo, MEM_FREE);
		info->memory_info.UsedRam   = binn_map_uint64(meminfo, MEM_USED);
		info->memory_info.TotalRam  = binn_map_uint64(meminfo, MEM_TOTAL);
		info->memory_info.AvailRam  = binn_map_uint64(meminfo, MEM_AVAIL);
		info->memory_info.SwapFree  = binn_map_uint64(meminfo, MEM_SWAP_FREE);
		info->memory_info.SwapTotal = binn_map_uint64(meminfo, MEM_SWAP_TOTAL);

		// Unserialize hard drive information.
		// First get the list of maps.
		binn *hddlist = (binn*)binn_map_list(ptr, HDD_INFO);
		binn_iter iter;
		binn value;
		binn_list_foreach(hddlist, value)
		{
			hdd_info_t *hdd = new hdd_info_t();
			hdd->Name           = binn_map_str(&value, HDD_NAME);
            hdd->BytesWritten   = binn_map_uint64(&value, HDD_BYTES_WRITTEN);
            hdd->BytesRead      = binn_map_uint64(&value, HDD_BYTES_READ);
            hdd->SpaceAvailable = binn_map_uint64(&value, HDD_SPACE_AVAIL);
            hdd->SpaceUsed      = binn_map_uint64(&value, HDD_SPACE_USED);
            hdd->PartitionSize  = binn_map_uint64(&value, HDD_PARTITION_SIZE);
            hdd->MountPoint     = binn_map_str(&value, HDD_MOUNT);
            hdd->FileSystemType = binn_map_str(&value, HDD_FS_TYPE);
            info->hdd_start.push_back(hdd);
		}

        // Unserialize the network information now
        binn *netlist = (binn*)binn_map_list(ptr, NETWORK_INFO);
        binn_list_foreach(netlist, value)
        {
            network_info_t *net = new network_info_t;
            net->InterfaceName = binn_map_str(&value, NET_IFACE);
            net->IPv6Address = binn_map_str(&value, NET_IPV6ADDR);
            net->IPv4Address = binn_map_str(&value, NET_IPV4ADDR);
            net->MACAddress = binn_map_str(&value, NET_MAC);
            net->SubnetMask = binn_map_str(&value, NET_SUBNET);
            net->TX = binn_map_uint64(&value, NET_TXCNT);
            net->RX = binn_map_uint64(&value, NET_RXCNT);
            info->net_start.push_back(net);
        }

        // Unserialize the kernel info.
        info->kernel_info.Type = binn_map_str(ptr, KERN_TYPE);
        info->kernel_info.Version = binn_map_str(ptr, KERN_VERS);
        info->kernel_info.Release = binn_map_str(ptr, KERN_REL);
        info->kernel_info.IsTainted = binn_map_bool(ptr, KERN_TAINTED);

        // Unserialize LSB Information
        info->lsb_info.Version     = binn_map_str(ptr, LSB_VERS);
        info->lsb_info.Dist_id     = binn_map_str(ptr, LSB_DISTRO);
        info->lsb_info.Release     = binn_map_str(ptr, LSB_REL);
        info->lsb_info.Description = binn_map_str(ptr, LSB_DESC);
#endif
        return info;
}
