/**
 * @file socket_server.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
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

constexpr auto MAX_CONCURRENCY = 10;

sockaddr_in get_inet_addr(uint32_t host, uint16_t port);
std::string format_inet_addr(uint32_t host, uint16_t port);
void cleanup(int);
void *tcp_holder_thread(void *arg);
inline int find_next_available(bool *tcp_conn_bitmap);

int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
bool tcp_listen_started = false;
static pthread_mutex_t output_mutex;

struct tcp_holder_arg
{
    int tcp_conn_id;
    int tcp_conn_fd;
    std::string tcp_peer_addr_s;
    bool *tcp_conn_flag;
};

int main()
{
    signal(SIGINT, cleanup);
    // Create socket
    if (tcp_listen_fd == -1)
    {
        std::cerr << "Error: Failed to create socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    // Bind 0.0.0.0:2333
    sockaddr_in serv_addr = get_inet_addr(INADDR_ANY, 2333);
    bind(tcp_listen_fd, (sockaddr *)&serv_addr, sizeof(serv_addr));

    // Strat listening
    if (listen(tcp_listen_fd, MAX_CONCURRENCY) == 0)
    {
        tcp_listen_started = true;
        std::cout << "Start listening on port 2333." << std::endl;
    }
    else
    {
        std::cerr << "Error: Failed to start listening: " << strerror(errno) << std::endl;
        exit(1);
    }

    int tcp_conn_fd_list[MAX_CONCURRENCY];
    sockaddr_in tcp_peer_addr_list[MAX_CONCURRENCY];
    unsigned int tcp_peer_len_list[MAX_CONCURRENCY];
    std::string tcp_peer_addr_s_list[MAX_CONCURRENCY];
    bool tcp_conn_bitmap[MAX_CONCURRENCY];
    for (auto iter = tcp_conn_bitmap; iter != tcp_conn_bitmap + MAX_CONCURRENCY; iter++)
        *iter = false;
    pthread_t thread_id[MAX_CONCURRENCY];
    tcp_holder_arg thread_arguments_list[MAX_CONCURRENCY];

    while (1)
    {
        int id = -1;
        while (id == -1)
            id = find_next_available(tcp_conn_bitmap);
        tcp_peer_len_list[id] = sizeof(sockaddr_in);
        tcp_conn_fd_list[id] = accept(tcp_listen_fd, (sockaddr *)&(tcp_peer_addr_list[id]), &tcp_peer_len_list[id]);
        if (tcp_conn_fd_list[id] == -1)
        {
            pthread_mutex_lock(&output_mutex);
            std::cerr << "[" << id << "]Error: Failed to establish connection: " << strerror(errno) << std::endl;
            pthread_mutex_unlock(&output_mutex);
        }
        else
        {
            tcp_conn_bitmap[id] = true;
            tcp_peer_addr_s_list[id] = format_inet_addr(ntohl(tcp_peer_addr_list[id].sin_addr.s_addr), ntohs(tcp_peer_addr_list[id].sin_port));

            pthread_mutex_lock(&output_mutex);
            std::cout << "[" << id << "]Got connection from " << tcp_peer_addr_s_list[id] << std::endl;
            pthread_mutex_unlock(&output_mutex);

            thread_arguments_list[id] = (tcp_holder_arg){id, tcp_conn_fd_list[id], tcp_peer_addr_s_list[id], tcp_conn_bitmap + id};
            pthread_create(&thread_id[id], nullptr, tcp_holder_thread, &thread_arguments_list[id]);
        }
    }
    close(tcp_listen_fd);

    return 0;
}

/**
 * @brief 
 * 
 */
// Returns a sockaddr_in struct representing host:port, parameters are in host order
sockaddr_in get_inet_addr(uint32_t host, uint16_t port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(host);
    addr.sin_port = htons(port);
    return addr;
}

// Returns a string in the format of A.B.C.D:port
std::string format_inet_addr(uint32_t host, uint16_t port)
{
    std::string result;
    std::stringstream sstream;
    sstream << ((host & 0xFF000000) >> 24);
    sstream << ".";
    sstream << ((host & 0x00FF0000) >> 16);
    sstream << ".";
    sstream << ((host & 0x0000FF00) >> 8);
    sstream << ".";
    sstream << (host & 0x000000FF);
    sstream << ":";
    sstream << port;
    sstream >> result;
    return result;
}

void cleanup(int)
{
    if (tcp_listen_started)
    {
        close(tcp_listen_fd);
        std::cout << "\nStopped listening." << std::endl;
    }
    exit(0);
}

void *tcp_holder_thread(void *arg)
{
    int tcp_conn_id = ((tcp_holder_arg *)arg)->tcp_conn_id;
    int tcp_conn_fd = ((tcp_holder_arg *)arg)->tcp_conn_fd;
    std::string tcp_peer_addr_s = ((tcp_holder_arg *)arg)->tcp_peer_addr_s;
    bool *tcp_conn_flag = ((tcp_holder_arg *)arg)->tcp_conn_flag;

    char buff[1024] = {};
    int msg_len = recv(tcp_conn_fd, buff, 1024, 0);
    buff[msg_len] = '\0';

    pthread_mutex_lock(&output_mutex);
    std::cout << "[" << tcp_conn_id << "]Recv msg from [" << tcp_peer_addr_s << "]: " << buff;
    ;
    pthread_mutex_unlock(&output_mutex);

    close(tcp_conn_fd);
    *tcp_conn_flag = false;

    return nullptr;
}

inline int find_next_available(bool *tcp_conn_bitmap)
{
    for (int i = 0; i < MAX_CONCURRENCY; i++)
    {
        if (tcp_conn_bitmap[i] == false)
            return i;
    }
    return -1;
}