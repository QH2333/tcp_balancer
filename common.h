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
#include <fstream>
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


#define RS_PREPARE    0
#define RS_REG_SERV   1
#define RS_NEG_PEER   2
#define RS_END        (-1)

#define OP_PREPARE    0
#define OP_S          1
#define OP_CLOSE      (-1)

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

typedef struct server_status_t
{
    pthread_mutex_t servstat_mutex;
    int target_channel_count;
    int target_conn_per_channel;
    int curr_channel_count;
    std::unordered_map<uint32_t, int> channel_dict;
} server_status_t;

typedef struct client_status_t
{

} client_status_t;

typedef struct tcp_status_t
{
    int tcp_conn_fd;
    bool tcp_conn_flag;
    int tcp_conn_id;
    tcp_tuple_info_t tcp_tuple_info;
    void *status_info;
} tcp_status_t;

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
    void *body; // TODO: remove this
} protocol_t;

inline int find_next_available(tcp_status_t *tcp_status, const int MAX_CONCURRENCY)
{
    for (int i = 0; i < MAX_CONCURRENCY; i++)
    {
        if (tcp_status[i].tcp_conn_flag == false)
            return i;
    }
    return -1;
}

sockaddr_in get_ip_addr(uint32_t host, uint16_t port);
std::string format_ip_addr(uint32_t host);
std::string format_ip_port(uint32_t host, uint16_t port);
std::string format_tcp_tuple(tcp_tuple_info_t data, uint8_t which = 0xFF);
int read_packet(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server = true);
int send_type2_method1(int tcp_conn_fd); // Request
int send_type2_method2(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server = true); // Reply