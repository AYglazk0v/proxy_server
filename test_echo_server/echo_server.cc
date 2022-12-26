//This is an echo server for checking the proxy_server operation.
//There are no checks and protections here.

#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <set>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

constexpr size_t MAX_EVENTS = 1024;

int main (int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "NEED PORT!";
        exit(1);
    }
    int MasterSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(std::atoi(argv[1]));
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(bind(MasterSocket, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        exit(-1);
    }
    if(fcntl(MasterSocket, F_SETFL, O_NONBLOCK) < 0) {
        exit(-1);
    }
    listen(MasterSocket, SOMAXCONN);

    int epoll_fd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = MasterSocket;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, MasterSocket, &ev);
    while (true) {
        struct epoll_event events[MAX_EVENTS];
        int N = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (unsigned int i = 0; i < N; ++i) {
            if (events[i].data.fd == MasterSocket) {
                int SlaveSocket = accept(MasterSocket, 0, 0);
                fcntl(SlaveSocket, F_SETFL, O_NONBLOCK);
                struct epoll_event ev;
                ev.events = EPOLLIN;
                ev.data.fd = SlaveSocket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, SlaveSocket, &ev);
            } else {
                static char buffer[1024];
                int recv_result = recv(events[i].data.fd, buffer, 1024, MSG_NOSIGNAL);
                if ((recv_result == 0 ) && (errno != EAGAIN)) {
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);
                } else if (recv_result > 0) {
                    send(events[i].data.fd, buffer, recv_result, MSG_NOSIGNAL);
                }
            }
        }
    }

}