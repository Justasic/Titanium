#pragma once
#include "Socket.h"

void ProcessUDP(Socket *s, socket_t client, void *buf, size_t len);
