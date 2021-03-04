/**
 * @file common.h
 * @author Qian Hao (qh2333@my.swjtu.edu.cn)
 * @brief Common headers and functions used in tcp_balancer
 * @version 0.1
 * @date 2021-03-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <algorithm>
#include <unordered_map>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

sockaddr_in get_inet_addr(uint32_t host, uint16_t port);
std::string format_inet_addr(uint32_t host, uint16_t port);
void sync_op(pthread_mutex_t *mutex, void(op)(void));

typedef struct tcp_peer_info
{
    sockaddr_in tcp_peer_addr;
    unsigned int tcp_peer_len;
    std::string tcp_peer_addr_s;
} tcp_peer_info;

typedef struct tcp_handler_arg
{
    int tcp_conn_id;
    int tcp_conn_fd;
    std::string tcp_peer_addr_s;
    bool *tcp_conn_flag;
}tcp_handler_arg;