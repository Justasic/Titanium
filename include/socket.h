#pragma once
#include "misc.h"

// Protocol setup is fairly easy. The protocol is setup in a data-burst style.
// The following diagram will show how data is sent. No data is received
// by the client, making it a send-only protocol. The client does not care
// about whether all the packets made it to their destination. A corrupt
// burst results in the dumping of the entire burst chunk.
//
//
//              [DATABURST packet - defines the beginning of a burst]
//              [TIMEOUT packet - timeout until next burst]
//              [INFOPAKS packet - number of info packets to expect]
//              for item in dataitems
//              {
//                  [INFO packet - this is the main data packet, it's used to send actual system metadata]
//              }
//              [ENDBURST packet - signals end of the data burst]
//
// If the server fails to receive the ENDBURST packet or an invalid number of info packets is sent
// then it is considered a corrupt databurst. If the timeout packet is omitted then either rely
// on the previously set timeout value or the server-default.
// The following illistrates how the packets are designed:
//
//
//             DATABURST and ENDBURST are 1-byte packets
//             DATABURST = 1, ENDBURST = 2;
//
//             ------------------------
//             |  databurst (1-byte)  |
//             ------------------------
//
//
// The timeout packet has the id '3' as a 1-byte value and timeout as uint32_t (4-byte unsigned integer)
// the value of timeout is defined as the wait in seconds. this is NOT a unix EPOCH value for when the
// server should expect another burst package but rather the time it should begin to expect seeing a
// burst package (but not exact, the package may come in early or late).
//
//
//           ----------------------------------------
//           | timeout (1-byte) | seconds (4-bytes) |
//           -----------------------------------------
//
//
// The info packet has the id '4', has a 1-byte identifier, and an n-1 byte data chunk. Each
// info packet is simply a chunk of metadata on the actual machine status. It's length varies
// and is dependent on the end of the packet. This is only the transport protocol so the
// containing data is up to a different part of this program.
//
//
//          -------------------------------------------
//          | info (1-byte) | info data (len-1 bytes) |
//          -------------------------------------------
//
// The infopaks packet defines how many info packets to expect. This value is used to determine
// whether we received all the data we were supposed to receive. this packet is similar to the
// timeout packet.


enum
{
    DATABURST = 1,
    ENDBURST,
    TIMEOUT,
    INFO,
    INFOPACKS
} packettypes_t;


extern void InitializeSocket();
extern void ShutdownSocket();
extern void SendDataBurst(information_t *info);
