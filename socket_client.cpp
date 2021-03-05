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

constexpr uint32_t SERVER_IP = 0x51445E12;
constexpr uint16_t SERVER_PORT = 2333;
constexpr auto MAX_CONCURRENCY = 10;
constexpr bool IS_SERVER = false;

void cleanup(int);
void *tcp_client_thread(void *arg);
inline int find_next_available(bool *tcp_conn_bitmap);

tcp_status_t tcp_status_list[MAX_CONCURRENCY];
pthread_t thread_id[MAX_CONCURRENCY];
tcp_client_arg thread_arguments_list[MAX_CONCURRENCY];

int main()
{
    signal(SIGINT, cleanup);

    for (int id = 0; id < MAX_CONCURRENCY; id++)
    {
        thread_arguments_list[id] = (tcp_client_arg){id, 0, tcp_status_list + id};
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
    // Unpackage arguments
    int thread_id = ((tcp_client_arg *)arg)->thread_id;
    tcp_status_t *tcp_status = ((tcp_client_arg *)arg)->tcp_status;

    // Create socket
    int tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ((tcp_client_arg *)arg)->tcp_client_fd = tcp_client_fd;
    if (tcp_client_fd == -1)
    {
        std::cerr << "[Error] Failed to create socket: "
                  << strerror(errno) << std::endl;
    }

    sockaddr_in serv_addr = get_ip_addr(SERVER_IP, SERVER_PORT);
    std::string tcp_server_addr_s = format_ip_port(ntohl(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

    if (connect(tcp_client_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        std::cerr << "[Error] Failed to establish connection with "
                  << tcp_server_addr_s << ": "
                  << strerror(errno) << std::endl;
    }

    // Get TCP info
    uint32_t tcp_addr_len = sizeof(sockaddr_in);
    sockaddr_in tcp_peer_addr;
    sockaddr_in tcp_local_addr;
    getsockname(tcp_client_fd, (sockaddr *)&tcp_local_addr, &tcp_addr_len);
    getpeername(tcp_client_fd, (sockaddr *)&tcp_peer_addr, &tcp_addr_len);
    tcp_status->tcp_tuple_info.c_server_ip = ntohl(tcp_local_addr.sin_addr.s_addr);
    tcp_status->tcp_tuple_info.c_server_port = ntohs(tcp_local_addr.sin_port);
    tcp_status->tcp_tuple_info.c_client_ip = ntohl(tcp_peer_addr.sin_addr.s_addr);
    tcp_status->tcp_tuple_info.c_client_port = ntohs(tcp_peer_addr.sin_port);

    std::cout << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_ALL) << std::endl;
    std::cout << format_tcp_tuple(tcp_status->tcp_tuple_info, CLIENT_ALL) << std::endl;

    // char msg[1024];
    // sprintf(msg, "Hello from thread %d\n", thread_id);
    // send(tcp_client_fd, msg, strlen(msg), 0);

    int op_code = 0;
    while ((op_code = read_packet(tcp_client_fd, tcp_status, false)) != -1)

    // usleep(500);

    // char buff[1024] = {};
    // int msg_len = recv(tcp_client_fd, buff, 1024, 0);
    // buff[msg_len] = '\0';
    // std::cout << msg_len << std::endl;

    close(tcp_client_fd);

    return nullptr;
}
