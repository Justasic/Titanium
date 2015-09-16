#pragma once
#include "Socket.h"
#include "Unserialize.h"

void ProcessUDP(Socket *s, socket_t client, void *buf, size_t len);
void ProcessInformation(information_t *info, time_t timeout);
