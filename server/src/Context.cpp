#include <iostream>
#include <unistd.h>
#include "Context.h"

Context::Context(int _fd, const char *_ip, unsigned short _port)
    : fd(_fd), ip(_ip), port(_port)
{
    std::clog << "Accepting incoming connection from " + ip + ":" + std::to_string(port) << std::endl;
}

Context::~Context()
{
    std::clog << "Connection from " + ip + ":" + std::to_string(port) << " closed" << std::endl;
}

void Context::addBytes(const char *bytes, int len)
{
    for (int i = 0; i < len; i++)
        std::clog << "Received : " << bytes[i] << std::endl;
    write(fd, bytes, len);
}

