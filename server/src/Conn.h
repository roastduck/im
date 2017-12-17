#ifndef CONN_H_
#define CONN_H_

#include <cassert>
#include <string>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "config.h"
#include "Context.h"
#include "exception/NetworkError.h"

/** Connection manager
 */
class Conn
{
private:
    int listenFd, epollFd;
    std::unordered_map< int, std::unique_ptr<Context> > contexts;

public:
    /** Setup listening on a TCP port
     *  @throw NetworkError
     */
    Conn(unsigned short port);

    /** Close the file descriptors
     */
    ~Conn();

    /** Wait until a connection is readable
     *  This will block the thread forever
     *  @param callback : function(Context &context)
     */
    template <class Callback>
    void wait(const Callback &callback);
};

template <class Callback>
inline void Conn::wait(const Callback &callback)
{
    struct epoll_event events[EPOLL_NUM_ONE_WAIT];
    char ip[16];
    char msg[MSG_BUF_SIZE];

    while (true)
    {
        int num = epoll_wait(epollFd, events, EPOLL_NUM_ONE_WAIT, -1 /* Indefinitely */);
        if (!~num)
            throw NetworkError("Error when `epoll_wait`");
        for (int i = 0; i < num; i++)
            if (events[i].data.fd == listenFd)
            {
                struct sockaddr_in clientAddr;
                socklen_t addrLen = sizeof(clientAddr);
                int connFd = accept(listenFd, (struct sockaddr*)&clientAddr, &addrLen);
                if (!~connFd)
                    throw NetworkError("Error accepting incomming connection");

                if (!inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)))
                    throw NetworkError("Cannot translate IP address");
                unsigned short port = ntohs(clientAddr.sin_port);

                struct epoll_event event = {EPOLLIN, {.fd = connFd}};
                if (!~epoll_ctl(epollFd, EPOLL_CTL_ADD, connFd, &event))
                    throw NetworkError("Cannot add to epoll");

                contexts[connFd] = std::unique_ptr<Context>(new Context(connFd, ip, port));
            } else
            {
                const int fd = events[i].data.fd;
                Context &context = *contexts.at(fd);

                int len = read(fd, msg, sizeof(msg));
                if (!~len)
                    throw NetworkError("Error reading from message");
                if (!len) // EOF
                {
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
                    contexts.erase(fd);
                    close(fd);
                    continue;
                }
                try
                {
                    context.addBytes(msg, len);
                } catch (const InvalidMessage &e)
                {
                    std::clog << "Invalid message: " << e.what() << std::endl;
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
                    contexts.erase(fd);
                    close(fd);
                    continue;
                }

                if (context.isAvail())
                    callback(context);
            }
    }
}

#endif // CONN_H_

