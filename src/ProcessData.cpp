#include <cstdio>
#include <cstdlib>
#include "misc.h"
#include "MySQL.h"
#include "Process.h"
#include "Unserialize.h"

#ifdef _DEBUG
void DumpSystemInformation(information_t *info)
{
	tfm::printf("================ DUMPING STATS =================\n");
	// General system info.
	tfm::printf("General system information:\n");
	tfm::printf("Current Time: %ld\n", info->CurrentTime);
	tfm::printf("StartTime: %ld\n", info->StartTime);
	tfm::printf("Loads: 5 %f, 15 %f, 30 %f\n", info->Loads[0], info->Loads[1], info->Loads[2]);
	tfm::printf("SecondsIdle: %f\n", info->SecondsIdle);
	tfm::printf("SecondsUptime: %f\n", info->SecondsUptime);
	tfm::printf("ProcessCount: %ld\n", info->ProcessCount);
	tfm::printf("RunningProcessCount: %ld\n", info->RunningProcessCount);
	tfm::printf("Zombies: %ld\n", info->Zombies);
	tfm::printf("UserCount: %ld\n", info->UserCount);
	tfm::printf("Hostname: %s\n\n", info->Hostname);

	// Processor information.
	tfm::printf("Processor information:\n");
	tfm::printf("Architecture: %s\n", info->cpu_info.Architecture);
	tfm::printf("Model: %s\n", info->cpu_info.Model);
	tfm::printf("Cores: %d\n", info->cpu_info.Cores);
	tfm::printf("PhysicalCores: %d\n", info->cpu_info.PhysicalCores);
	tfm::printf("CurrentSpeed: %f\n", info->cpu_info.CurrentSpeed);
	tfm::printf("CPUPercent: %d\n\n", info->cpu_info.CPUPercent);

	// Ram information
	tfm::printf("RAM information\n");
	tfm::printf("FreeRam: %lu bytes\n", info->memory_info.FreeRam);
	tfm::printf("UsedRam: %lu bytes\n", info->memory_info.UsedRam);
	tfm::printf("TotalRam: %lu bytes\n", info->memory_info.TotalRam);
	tfm::printf("AvalableRam: %lu bytes\n", info->memory_info.AvailRam);
	tfm::printf("SwapFree: %lu bytes\n", info->memory_info.SwapFree);
	tfm::printf("SwapTotal: %lu bytes\n\n", info->memory_info.SwapTotal);

	// HDD info
	tfm::printf("Hard disk information:\n");
	for (auto it : info->hdd_start)
	{
		tfm::printf("Name: %s\n", it->Name);
        tfm::printf("BytesWritten: %ld\n", it->BytesWritten);
        tfm::printf("BytesRead: %ld\n", it->BytesRead);
        tfm::printf("SpaceAvailable: %ld\n", it->SpaceAvailable);
        tfm::printf("SpaceUsed: %ld\n", it->SpaceUsed);
        tfm::printf("PartitionSize: %ld\n", it->PartitionSize);
        tfm::printf("MountPoint: %s\n", it->MountPoint);
        tfm::printf("FileSystemType: %s\n", it->FileSystemType);
	}

	// Network information
	tfm::printf("Network Information:\n");
	for (auto iter : info->net_start)
	{
		tfm::printf("InterfaceName: %s\n", iter->InterfaceName);
		tfm::printf("IPv6Address: %s\n", iter->IPv6Address);
		tfm::printf("IPv4Address: %s\n", iter->IPv4Address);
		tfm::printf("MACAddress: %s\n", iter->MACAddress);
		tfm::printf("SubnetMask: %s\n", iter->SubnetMask);
		tfm::printf("Sent Byte Count: %lu\n", iter->TX);
		tfm::printf("Received Byte Count: %lu\n\n", iter->RX);
	}

	// Kernel information
	tfm::printf("Kernel Information:\n");
	tfm::printf("Type: %s\n", info->kernel_info.Type);
	tfm::printf("Version: %s\n", info->kernel_info.Version);
	tfm::printf("Release: %s\n", info->kernel_info.Release);
	tfm::printf("IsTainted: %d\n\n", info->kernel_info.IsTainted);

	// LSB (Distro) information
	tfm::printf("LSB (distro) information:\n");
	tfm::printf("Version: %s\n", info->lsb_info.Version);
	tfm::printf("Distro ID: %s\n", info->lsb_info.Dist_id);
	tfm::printf("Release: %s\n", info->lsb_info.Release);
	tfm::printf("Description: %s\n\n", info->lsb_info.Description);

	// ALL done.
	tfm::printf("================ END DUMPING STATS =================\n");
}
#else
# define DumpSystemInformation(x)
#endif

extern MySQL *ms;

void ProcessInformation(information_t *info, time_t timeout)
{
    // Print the system information to the front.
    DumpSystemInformation(info);

    // Get the id based on what hostname we're using.
    MySQL_Result res = ms->Query("SELECT id FROM systems WHERE hostname = '%s'", ms->Escape(info->Hostname));
    int id = 0;
	if (!res.rows.empty())
	{
		id = strtol(res.rows[0][0].c_str(), NULL, 10);
		if (errno == ERANGE)
			id = 0;
	}

    // Submit it to the database.
    ms->Query("INSERT INTO systems (id, uptime, processes, hostname, architecture, os, lastupdate)"
			  " VALUES(%d, %ld, %ld, '%s', '%s', '%s', FROM_UNIXTIME(%d)) ON DUPLICATE KEY UPDATE uptime=VALUES(uptime), processes=VALUES(processes),"
			  " architecture=VALUES(architecture), os=VALUES(os), lastupdate=VALUES(lastupdate), hostname=VALUES(hostname)",
			  id, info->StartTime, info->ProcessCount, ms->Escape(info->Hostname), ms->Escape(info->cpu_info.Architecture), ms->Escape(info->lsb_info.Description), time(NULL));
}
