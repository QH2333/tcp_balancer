/**
 * @file socket_client.cpp
 * @author Qian Hao (qh2333@my.swjtu.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "common.h"

constexpr auto MAX_CONCURRENCY = 10;

void cleanup(int);
void *tcp_client_thread(void *arg);
inline int find_next_available(bool *tcp_conn_bitmap);

struct tcp_client_arg
{
    int thread_id;
    int tcp_client_fd;
};

int main()
{
    signal(SIGINT, cleanup);

    pthread_t thread_id[MAX_CONCURRENCY];
    tcp_client_arg thread_arguments_list[MAX_CONCURRENCY];

    for (int id = 0; id < MAX_CONCURRENCY; id++)
    {
        thread_arguments_list[id] = (tcp_client_arg){id};
        pthread_create(&thread_id[id], nullptr, tcp_client_thread, &thread_arguments_list[id]);
    }

    for (int id = 0; id < MAX_CONCURRENCY; id++)
    {
        pthread_join(thread_id[id], nullptr);
    }

    return 0;
}


void cleanup(int)
{
    exit(0);
}

void *tcp_client_thread(void *arg)
{
    int thread_id = ((tcp_client_arg *)arg)->thread_id;
    // Create socket
    int tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ((tcp_client_arg *)arg)->tcp_client_fd = tcp_client_fd;
    if (tcp_client_fd == -1)
    {
        std::cerr << "[Error] Failed to create socket: "
                  << strerror(errno) << std::endl;
    }

    sockaddr_in serv_addr = get_inet_addr(0x51445E12, 2333);
    std::string tcp_server_addr_s = format_inet_addr(ntohl(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

    if (connect(tcp_client_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        std::cerr << "[Error] Failed to establish connection with "
                  << tcp_server_addr_s << ": "
                  << strerror(errno) << std::endl;
    }

    char msg[1024];
    sprintf(msg, "Hello from thread %d\n", thread_id);
    send(tcp_client_fd, msg, strlen(msg), 0);

    // usleep(500);

    // char buff[1024] = {};
    // int msg_len = recv(tcp_client_fd, buff, 1024, 0);
    // buff[msg_len] = '\0';
    // std::cout << msg_len << std::endl;

    close(tcp_client_fd);

    return nullptr;
}
