/**
 * @file common.cpp
 * @author Qian Hao (qh2333@my.swjtu.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2021-03-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "common.h"

/**
 * @brief Returns a sockaddr_in struct representing host:port
 * 
 * @param host 32bit IPv4 address in host order
 * @param port 16bit TCP port in host order
 * @return sockaddr_in 
 */
sockaddr_in get_ip_addr(uint32_t host, uint16_t port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(host);
    addr.sin_port = htons(port);
    return addr;
}

std::string format_ip_addr(uint32_t host)
{
    std::stringstream sstream;
    sstream << ((host & 0xFF000000) >> 24);
    sstream << ".";
    sstream << ((host & 0x00FF0000) >> 16);
    sstream << ".";
    sstream << ((host & 0x0000FF00) >> 8);
    sstream << ".";
    sstream << (host & 0x000000FF);
    return sstream.str();
}

/**
 * @brief Format a TCP over IPv4 in the format of A.B.C.D:port
 * 
 * @param host 32bit IPv4 address in host order
 * @param port 16bit TCP port in host order
 * @return std::string 
 */
std::string format_ip_port(uint32_t host, uint16_t port)
{
    std::stringstream sstream;
    sstream << format_ip_addr(host);
    sstream << ":";
    sstream << port;
    return sstream.str();
}

std::string format_tcp_tuple(tcp_tuple_info_t data, uint8_t which)
{
    std::stringstream sstream;
    if (which & SERVER_SERVER) sstream << format_ip_port(data.s_server_ip, data.s_server_port);
    if ((which & SERVER_SERVER) && (which & SERVER_CLIENT)) sstream << " <-> ";
    if (which & SERVER_CLIENT) sstream << format_ip_port(data.s_client_ip, data.s_client_port);

    if (which & CLIENT_SERVER) sstream << format_ip_port(data.c_server_ip, data.c_server_port);
    if ((which & CLIENT_SERVER) && (which & CLIENT_CLIENT)) sstream << " <-> ";
    if (which & CLIENT_CLIENT) sstream << format_ip_port(data.c_client_ip, data.c_client_port);

    return sstream.str();
}

void sync_op(pthread_mutex_t *mutex, void(op)(void))
{
    pthread_mutex_lock(mutex);
    op();
    pthread_mutex_unlock(mutex);
}


int read_packet(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server)
{
    protocol_t packet_info;

    int state = 0;
    int ret_value = -1;
    char recv_buff[1024] = {};
    int msg_len;

    while (state != -1)
    {
        switch (state) 
        {
        case 0: // Read common header
            msg_len = recv(tcp_conn_fd, recv_buff, 4, 0);
            if (msg_len > 0)
            {
                packet_info.type = recv_buff[0];
                packet_info.version = recv_buff[1];
                packet_info.length = recv_buff[2] << 8 + recv_buff[3];
            }
            state = packet_info.type;
            break;
        case 1: // Register service
            state = -1;
            break;
        case 2: // Negotiate peer infomation
            msg_len = recv(tcp_conn_fd, recv_buff, 1, 0);
            if (msg_len > 0)
            {
                int method = recv_buff[0];
                switch (method)
                {
                case 1: // Request peer TCP tuple -> Reply
                    send_type2_method2(tcp_conn_fd, tcp_status, is_server);
                    break;
                case 2: // Reply TCP tuple -> Parse
                    msg_len = recv(tcp_conn_fd, recv_buff, 12, 0);
                    if (msg_len == 12)
                    {
                        if (is_server)
                        {
                            tcp_status->tcp_tuple_info.c_server_ip = ntohl(*(uint32_t *)(recv_buff + 0)); // Server IP
                            tcp_status->tcp_tuple_info.c_server_port = ntohs(*(uint16_t *)(recv_buff + 4)); // Server port
                            tcp_status->tcp_tuple_info.c_client_ip = ntohl(*(uint32_t *)(recv_buff + 6)); // Client IP
                            tcp_status->tcp_tuple_info.c_client_port = ntohs(*(uint16_t *)(recv_buff + 10)); // Client port
                        }
                        else // is client
                        {
                            tcp_status->tcp_tuple_info.s_server_ip = ntohl(*(uint32_t *)(recv_buff + 0)); // Server IP
                            tcp_status->tcp_tuple_info.s_server_port = ntohs(*(uint16_t *)(recv_buff + 4)); // Server port
                            tcp_status->tcp_tuple_info.s_client_ip = ntohl(*(uint32_t *)(recv_buff + 6)); // Client IP
                            tcp_status->tcp_tuple_info.s_client_port = ntohs(*(uint16_t *)(recv_buff + 10)); // Client port
                        }
                        ret_value = 1;
                    }
                    break;
                }
            }
            state = -1;
            break;
        default:
            state = -1;
            break;
        }
    }
    return ret_value;
}



int send_type2_method1(int tcp_conn_fd) // Request
{
    char send_buff[5];
    *(uint8_t *)(send_buff + 0) = 2; // Type
    *(uint8_t *)(send_buff + 1) = 1; // Version
    *(uint16_t *)(send_buff + 2) = htons(0); // Length
    *(uint8_t *)(send_buff + 4) = 1; // Method: request
    send(tcp_conn_fd, send_buff, 5, 0);
}

int send_type2_method2(int tcp_conn_fd, tcp_status_t *tcp_status, const bool is_server) // Reply
{
    char send_buff[20];
    *(uint8_t *)(send_buff + 0) = 2; // Type
    *(uint8_t *)(send_buff + 1) = 1; // Version
    *(uint16_t *)(send_buff + 2) = htons(0); // Length
    *(uint8_t *)(send_buff + 4) = 2; // Method: reply
    if (is_server)
    {
        *(uint32_t *)(send_buff + 5) = htonl(tcp_status->tcp_tuple_info.s_server_ip); // Server IP
        *(uint16_t *)(send_buff + 9) = htons(tcp_status->tcp_tuple_info.s_server_port); // Server port
        *(uint32_t *)(send_buff + 11) = htonl(tcp_status->tcp_tuple_info.s_client_ip); // Client IP
        *(uint16_t *)(send_buff + 15) = htons(tcp_status->tcp_tuple_info.s_client_port); // Client port
    }
    else // is client
    {
        *(uint32_t *)(send_buff + 5) = htonl(tcp_status->tcp_tuple_info.c_server_ip); // Server IP
        *(uint16_t *)(send_buff + 9) = htons(tcp_status->tcp_tuple_info.c_server_port); // Server port
        *(uint32_t *)(send_buff + 11) = htonl(tcp_status->tcp_tuple_info.c_client_ip); // Client IP
        *(uint16_t *)(send_buff + 15) = htons(tcp_status->tcp_tuple_info.c_client_port); // Client port
    }
    send(tcp_conn_fd, send_buff, 17, 0);
}