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

constexpr uint16_t LISTEN_PORT = 2333;
constexpr auto MAX_CONCURRENCY = 10;
constexpr bool IS_SERVER = true;

int tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
bool tcp_listen_started = false;
std::ofstream fout;
static pthread_mutex_t output_mutex;
static pthread_mutex_t foutput_mutex;

void prepare();
void cleanup(int = 0);
void *tcp_handler_thread(void *arg);

int main()
{
    prepare(); // Create, bind, listen

    server_status_t server_status; // Status of the server
    tcp_status_t tcp_status_list[MAX_CONCURRENCY]; // Status per TCP connection
    std::for_each(tcp_status_list, tcp_status_list + MAX_CONCURRENCY, [](tcp_status_t &item) -> void { item.tcp_conn_flag = false; });
    pthread_t thread_id[MAX_CONCURRENCY];

    while (1)
    {
        int id = -1;
        while (id == -1)
            id = find_next_available(tcp_status_list, MAX_CONCURRENCY);

        tcp_status_list[id].tcp_conn_id = id;
        tcp_status_list[id].tcp_conn_fd = accept(tcp_listen_fd, NULL, NULL);
        tcp_status_list[id].status_info = &server_status;
        if (tcp_status_list[id].tcp_conn_fd == -1)
        {
            pthread_mutex_lock(&output_mutex);
            std::cerr << "[" << id << "] Error: Failed to establish connection: " << strerror(errno) << std::endl;
            pthread_mutex_unlock(&output_mutex);
        }
        else
        {
            tcp_status_list[id].tcp_conn_flag = true;
            pthread_create(&thread_id[id], nullptr, tcp_handler_thread, tcp_status_list + id);
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
    fout.open("server.log", std::ios::out);
    // Create socket
    if (tcp_listen_fd == -1)
    {
        std::cerr << "[Error] Failed to create socket: " << strerror(errno) << std::endl;
        exit(1);
    }

    // Bind 0.0.0.0:2333
    sockaddr_in serv_addr = get_ip_addr(INADDR_ANY, LISTEN_PORT);
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
    // Unpackage arguments
    tcp_status_t *tcp_status = (tcp_status_t *)arg;
    server_status_t *server_status = (server_status_t *)tcp_status->status_info;

    // Get TCP info
    uint32_t tcp_addr_len = sizeof(sockaddr_in);
    sockaddr_in tcp_peer_addr;
    sockaddr_in tcp_local_addr;
    getsockname(tcp_status->tcp_conn_fd, (sockaddr *)&tcp_local_addr, &tcp_addr_len);
    getpeername(tcp_status->tcp_conn_fd, (sockaddr *)&tcp_peer_addr, &tcp_addr_len);
    tcp_status->tcp_tuple_info.s_server_ip = ntohl(tcp_local_addr.sin_addr.s_addr);
    tcp_status->tcp_tuple_info.s_server_port = ntohs(tcp_local_addr.sin_port);
    tcp_status->tcp_tuple_info.s_client_ip = ntohl(tcp_peer_addr.sin_addr.s_addr);
    tcp_status->tcp_tuple_info.s_client_port = ntohs(tcp_peer_addr.sin_port);
    
    // State machine
    int op_code = OP_PREPARE;

    pthread_mutex_lock(&foutput_mutex);
    fout << "[" << tcp_status->tcp_conn_id << "] Got connection [" << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_ALL) << "]" << std::endl;
    pthread_mutex_unlock(&foutput_mutex);

    pthread_mutex_lock(&(server_status->servstat_mutex));
    auto kv_iter = server_status->channel_dict.find(tcp_status->tcp_tuple_info.s_client_ip);
    if (kv_iter == server_status->channel_dict.end())
    {
        server_status->channel_dict.insert({tcp_status->tcp_tuple_info.s_client_ip, 1});
        pthread_mutex_lock(&foutput_mutex);
        fout << "[Info] New IP: " << format_ip_addr(tcp_status->tcp_tuple_info.s_client_ip) << std::endl;
        pthread_mutex_unlock(&foutput_mutex);
    }
    else if (kv_iter->second >= 2)
    {
        pthread_mutex_lock(&output_mutex);
        std::cout << "Prohibited!" << std::endl;
        pthread_mutex_unlock(&output_mutex);
        op_code = OP_CLOSE;
    }
    else
    {
        server_status->channel_dict[tcp_status->tcp_tuple_info.s_client_ip]++;
    }
    pthread_mutex_unlock(&(server_status->servstat_mutex));


    while (op_code != OP_CLOSE)
    {
        switch (op_code)
        {
        case OP_PREPARE:
            send_type2_method1(tcp_status->tcp_conn_fd);
            op_code = read_packet(tcp_status->tcp_conn_fd, tcp_status);
            break;
        case OP_S:
            pthread_mutex_lock(&output_mutex);
            std::cout << "[" << tcp_status->tcp_conn_id << "] "
                    << "Server side: [" << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_ALL) << "] "
                    << "Client side: [" << format_tcp_tuple(tcp_status->tcp_tuple_info, CLIENT_ALL) << "] " << std::endl;
            pthread_mutex_unlock(&output_mutex);
            op_code = OP_CLOSE;
        default:
            break;
        }
    }

    close(tcp_status->tcp_conn_fd);
    tcp_status->tcp_conn_flag = false;

    pthread_mutex_lock(&foutput_mutex);
    fout << "[" << tcp_status->tcp_conn_id << "] Disconncet [" << format_tcp_tuple(tcp_status->tcp_tuple_info, SERVER_CLIENT) << "]" << std::endl;
    pthread_mutex_unlock(&foutput_mutex);

    return nullptr;
}

void cleanup(int) // There must be an int parameter, magic
{
    if (tcp_listen_started)
    {
        close(tcp_listen_fd);
        std::cout << "\nStopped listening." << std::endl;
    }
    fout.close();
    exit(0);
}