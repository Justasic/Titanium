#pragma once
#include <stdint.h>
#include <vector>
#include <string>

typedef struct hdd_info_s
{
	// TODO
	std::string Name; // Device/partition name.
	size_t BytesWritten;
	size_t BytesRead;

	size_t SpaceAvailable; // In bytes
	size_t SpaceUsed; // in bytes
	size_t PartitionSize; // in bytes
	std::string MountPoint;
	std::string FileSystemType; // NTFS or FAT32 on windows.
} hdd_info_t;

typedef struct network_info_s
{
	std::string InterfaceName;
	std::string IPv6Address;
	std::string IPv4Address;
	std::string MACAddress; // Includes colons
	std::string SubnetMask; // This is the IPv4 Subnet Mask and IPv6 CIDR mask
	uint64_t TX;
	uint64_t RX;
} network_info_t;

typedef struct information_s
{
	// CPU, RAM, Load time, Distro info, Uptime, Kernel Version, hard drive info,
	// # of total processes, # of active processes, # of users, hostname, current time, IPv4 Address(es),
	// IPv6 Address(es), Mac address(es), Interface names, TX/RX counts, subnet/cidr masks
	// kernel command line options (if available), CPU Architecture
	time_t CurrentTime;
	time_t StartTime; // Seconds in EPOCH format since the system booted.
	float Loads[3]; // Null on windows. -- for now.
	float SecondsIdle; // Seconds spent idle (idfk why the kernel gives it as a float)
	float SecondsUptime; // ???
	unsigned long ProcessCount;
	unsigned long RunningProcessCount;
	unsigned long Zombies;
	unsigned long UserCount;
	std::string Hostname;

	struct
	{
		std::string Architecture;   // arm, i386, x86_64, etc.
		std::string Model;          // Model from the kernel (eg, Intel(R) Core(TM) i7-4930K CPU @ 3.40GHz)
		unsigned int Cores;         // How many logical processors the kernel sees (including hyperthreaded ones)
		unsigned int PhysicalCores; // How many physical cores exist on the die
		float CurrentSpeed;         // Current speed of the CPU.
		unsigned int CPUPercent;    // Calculated by us.
	} cpu_info;

	struct
	{
		uint64_t FreeRam; // In bytes
		uint64_t UsedRam; // In bytes
		uint64_t TotalRam; // In bytes
		uint64_t AvailRam; // In bytes, An estimate of how much memory is available for starting new applications
		uint64_t SwapFree; // In bytes
		uint64_t SwapTotal; // In bytes
	} memory_info;

	// Hard drives.
	std::vector<hdd_info_t*> hdd_start;

	// Network interfaces
	std::vector<network_info_t*> net_start;

	struct
	{
		std::string Type;
		std::string Version;
		std::string Release;
		uint8_t IsTainted;
	} kernel_info;

	struct
	{
		std::string Version;
		std::string Dist_id;
		std::string Release;
		std::string Description;
	} lsb_info;
} information_t;

information_t *UnserializeData(void *ptr, size_t len);
