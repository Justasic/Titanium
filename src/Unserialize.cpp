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
	
}
