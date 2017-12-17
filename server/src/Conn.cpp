#include <iostream>
#include "Conn.h"

Conn::Conn(unsigned short port)
    : listenFd(-1), epollFd(-1)
{
    // Setup epoll
    epollFd = epoll_create(DEFAULT_EPOLL_POOL);
    if (!~epollFd)
        throw NetworkError("Cannot create epoll");

    // Setup listening
    listenFd = socket(AF_INET, SOCK_STREAM, 0 /* Not specifying */);
    if (!~listenFd)
        throw NetworkError("Cannot create a listening socket");

    struct sockaddr_in hostAddr;
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(port);
    hostAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    if (!~bind(listenFd, (struct sockaddr*)&hostAddr, 16 /* `sockaddr` struct size */))
        throw NetworkError("Cannot bind 0.0.0.0:" + std::to_string(port));

    if (!~listen(listenFd, MAX_PENDING_CONN /* Pending list size */))
        throw NetworkError("Cannot setup `listen`");

    // Add listenFd to epoll
    struct epoll_event epollEvent = {EPOLLIN, {.fd = listenFd}};
    if (!~epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &epollEvent))
        throw NetworkError("Cannot add into epoll list");

    std::clog << "Listening on 0.0.0.0:" << std::to_string(port) << std::endl;
}

Conn::~Conn()
{
    if (~epollFd)
        close(epollFd);
    if (~listenFd)
        close(listenFd);
}

