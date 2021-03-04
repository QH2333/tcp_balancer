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

void sync_op(pthread_mutex_t *mutex, void(op)(void))
{
    pthread_mutex_lock(mutex);
    op();
    pthread_mutex_unlock(mutex);
}