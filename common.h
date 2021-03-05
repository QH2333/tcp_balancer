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


#define SERVER_ALL    0xF0
#define SERVER_SERVER 0xC0
#define SERVER_CLIENT 0x30
#define CLIENT_ALL    0x0F
#define CLIENT_SERVER 0x0C
#define CLIENT_CLIENT 0x03


typedef struct tcp_tuple_info_t // All in host order
{
    uint32_t s_server_ip;
    uint16_t s_server_port;
    uint32_t s_client_ip;
    uint16_t s_client_port;
    uint32_t c_server_ip;
    uint16_t c_server_port;
    uint32_t c_client_ip;
    uint16_t c_client_port;
}tcp_tuple_info_t;

typedef struct tcp_status_t
{
    tcp_tuple_info_t tcp_tuple_info;
} tcp_status_t;

typedef struct tcp_handler_arg
{
    int tcp_conn_id;
    int tcp_conn_fd;
    tcp_status_t *tcp_status;
    bool *tcp_conn_flag;
}tcp_handler_arg;

struct tcp_client_arg
{
    int thread_id;
    int tcp_client_fd;
    tcp_status_t *tcp_status;
};

typedef struct protocol_t
{
    uint8_t type;
    uint8_t version;
    uint16_t length; // Body length in byte
    void *body;
} protocol_t;


sockaddr_in get_ip_addr(uint32_t host, uint16_t port);
std::string format_ip_addr(uint32_t host);
std::string format_ip_port(uint32_t host, uint16_t port);
std::string format_tcp_tuple(tcp_tuple_info_t data, uint8_t which = 0xFF);
void sync_op(pthread_mutex_t *mutex, void(op)(void));

int read_packet(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server = true);
int send_type2_method1(int tcp_conn_fd); // Request
int send_type2_method2(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server = true); // Reply