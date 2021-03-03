#pragma once

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

sockaddr_in get_inet_addr(uint32_t host, uint16_t port);
std::string format_inet_addr(uint32_t host, uint16_t port);