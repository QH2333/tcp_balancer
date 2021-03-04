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
sockaddr_in get_inet_addr(uint32_t host, uint16_t port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(host);
    addr.sin_port = htons(port);
    return addr;
}

/**
 * @brief Format a IPv4 address in the format of A.B.C.D:port
 * 
 * @param host 32bit IPv4 address in host order
 * @param port 16bit TCP port in host order
 * @return std::string 
 */
std::string format_inet_addr(uint32_t host, uint16_t port)
{
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
    return sstream.str();
}

std::string format_tcp_tuple(tcp_tuple_info_t data, uint8_t which)
{
    std::stringstream sstream;
    if (which & SERVER_SERVER) sstream << format_inet_addr(data.s_server_ip, data.s_server_port);
    if ((which & SERVER_SERVER) && (which & SERVER_CLIENT)) sstream << " <-> ";
    if (which & SERVER_CLIENT) sstream << format_inet_addr(data.s_client_ip, data.s_client_port);

    if (which & CLIENT_SERVER) sstream << format_inet_addr(data.c_server_ip, data.c_server_port);
    if ((which & CLIENT_SERVER) && (which & CLIENT_CLIENT)) sstream << " <-> ";
    if (which & CLIENT_CLIENT) sstream << format_inet_addr(data.c_client_ip, data.c_client_port);

    return sstream.str();
}

void sync_op(pthread_mutex_t *mutex, void(op)(void))
{
    pthread_mutex_lock(mutex);
    op();
    pthread_mutex_unlock(mutex);
}