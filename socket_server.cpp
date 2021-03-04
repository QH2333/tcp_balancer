/**
 * @file socket_server.cpp
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
constexpr uint16_t TCP_PORT = 2333;

void prepare();
void cleanup(int = 0);
void *tcp_handler_thread(void *arg);
inline int find_next_available(bool *tcp_conn_bitmap);

int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
bool tcp_listen_started = false;
std::unordered_map<uint32_t, int> ip_log;
static pthread_mutex_t output_mutex;

int main()
{
    prepare(); // Create, bind, listen

    int tcp_conn_fd_list[MAX_CONCURRENCY];
    bool tcp_conn_bitmap[MAX_CONCURRENCY];
    std::for_each(tcp_conn_bitmap, tcp_conn_bitmap + MAX_CONCURRENCY, [](bool &item) -> void { item = false; });
    tcp_status_t tcp_status_list[MAX_CONCURRENCY];
    pthread_t thread_id[MAX_CONCURRENCY];
    tcp_handler_arg thread_arguments_list[MAX_CONCURRENCY];

    while (1)
    {
        int id = -1;
        while (id == -1)
            id = find_next_available(tcp_conn_bitmap);

        uint32_t tcp_peer_len = sizeof(sockaddr_in);
        sockaddr_in tcp_peer_addr;
        tcp_conn_fd_list[id] = accept(tcp_listen_fd, (sockaddr *)&tcp_peer_addr, &tcp_peer_len);

        if (tcp_conn_fd_list[id] == -1)
        {
            pthread_mutex_lock(&output_mutex);
            std::cerr << "[" << id << "] Error: Failed to establish connection: " << strerror(errno) << std::endl;
            pthread_mutex_unlock(&output_mutex);
        }
        else
        {
            tcp_conn_bitmap[id] = true;

            tcp_status_list[id].tcp_tuple_info.s_server_ip = 0x7F000001;
            tcp_status_list[id].tcp_tuple_info.s_server_port = TCP_PORT;
            tcp_status_list[id].tcp_tuple_info.s_client_ip = ntohl(tcp_peer_addr.sin_addr.s_addr);
            tcp_status_list[id].tcp_tuple_info.s_client_port = ntohs(tcp_peer_addr.sin_port);

            pthread_mutex_lock(&output_mutex);
            std::cout << "[" << id << "] Got connection [" << format_tcp_tuple(tcp_status_list[id].tcp_tuple_info, SERVER_ALL) << "]" << std::endl;
            pthread_mutex_unlock(&output_mutex);

            thread_arguments_list[id] = (tcp_handler_arg){id, tcp_conn_fd_list[id], tcp_status_list+id, tcp_conn_bitmap + id};
            pthread_create(&thread_id[id], nullptr, tcp_handler_thread, &thread_arguments_list[id]);

            if (ip_log.find(tcp_status_list[id].tcp_tuple_info.s_client_ip) == ip_log.end())
            {
                ip_log.insert({tcp_status_list[id].tcp_tuple_info.s_client_ip, 1});
                pthread_mutex_lock(&output_mutex);
                std::cout << "[Info] New IP: " << tcp_status_list[id].tcp_tuple_info.s_client_ip << std::endl;
                pthread_mutex_unlock(&output_mutex);
            }
        }
    }
    cleanup();

    return 0;
}

/**
 * @brief Doing preparations:
 *          Set up SIGINT handler
 *          Create and bind socket
 *          Start listening
 */
void prepare()
{
    signal(SIGINT, cleanup);
    // Create socket
    if (tcp_listen_fd == -1)
    {
        std::cerr << "[Error] Failed to create socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    // Bind 0.0.0.0:2333
    sockaddr_in serv_addr = get_inet_addr(INADDR_ANY, TCP_PORT);
    if (bind(tcp_listen_fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        std::cerr << "[Error] Failed to bind socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    // Strat listening
    if (listen(tcp_listen_fd, MAX_CONCURRENCY) == 0)
    {
        tcp_listen_started = true;
        std::cout << "[Info] Start listening on port 2333." << std::endl;
    }
    else // listen == -1
    {
        std::cerr << "[Error] Failed to start listening: " << strerror(errno) << std::endl;
        exit(1);
    }
}

void *tcp_handler_thread(void *arg)
{
    int tcp_conn_id = ((tcp_handler_arg *)arg)->tcp_conn_id;
    int tcp_conn_fd = ((tcp_handler_arg *)arg)->tcp_conn_fd;
    tcp_status_t *tcp_status = ((tcp_handler_arg *)arg)->tcp_status;
    bool *tcp_conn_flag = ((tcp_handler_arg *)arg)->tcp_conn_flag;

    char buff[1024] = {};
    int msg_len = recv(tcp_conn_fd, buff, 1024, 0);
    buff[msg_len] = '\0';

    pthread_mutex_lock(&output_mutex);
    std::cout << "[" << tcp_conn_id << "] Recv msg from [" << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_CLIENT) << "]: " << buff;
    pthread_mutex_unlock(&output_mutex);

    close(tcp_conn_fd);
    *tcp_conn_flag = false;
    pthread_mutex_lock(&output_mutex);
    std::cout << "[" << tcp_conn_id << "] Disconncet [" << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_CLIENT) << "]" << std::endl;
    pthread_mutex_unlock(&output_mutex);

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

void cleanup(int) // There must be an int parameter, magic
{
    if (tcp_listen_started)
    {
        close(tcp_listen_fd);
        std::cout << "\nStopped listening." << std::endl;
    }
    exit(0);
}