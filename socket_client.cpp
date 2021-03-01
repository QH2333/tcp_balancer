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

constexpr auto MAX_CONCURRENCY = 5;

sockaddr_in get_inet_addr(uint32_t host, uint16_t port);
std::string format_inet_addr(uint32_t host, uint16_t port);
void cleanup(int);
void* tcp_client_thread(void* arg);
inline int find_next_available(bool* tcp_conn_bitmap);

struct tcp_client_arg
{
    int thread_id;
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
    exit(0);
}

void* tcp_client_thread(void* arg)
{
    int thread_id = ((tcp_client_arg*)arg)->thread_id;
    // Create socket
    int tcp_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_fd == -1)
    {
        std::cerr << "Error: Failed to create socket!" << std::endl;
        exit(1);
    }

    sockaddr_in serv_addr = get_inet_addr(0x51445E12, 2333);
    std::string tcp_server_addr_s = format_inet_addr(ntohl(serv_addr.sin_addr.s_addr), ntohs(serv_addr.sin_port));

    if (connect(tcp_client_fd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        std::cerr << "Error: Failed to establish connection with " << tcp_server_addr_s << "!" << std::endl;
        std::cerr << strerror(errno) << std::endl;
    }
    
    char msg[1024];
    sprintf(msg, "Hello from thread %d\n", thread_id);
    send(tcp_client_fd, msg, strlen(msg), 0);
    close(tcp_client_fd);

    return nullptr;
}

