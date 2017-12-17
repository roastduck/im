#include <iostream>
#include <unistd.h>
#include "Context.h"

Context::Context(int _fd, const char *_ip, unsigned short _port)
    : escaping(false), fd(_fd), ip(_ip), port(_port)
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
        switch (bytes[i])
        {
        case DILIMITER:
            if (escaping)
                buff.push_back(DILIMITER);
            escaping ^= 1;
            break;
        case TERMINATOR:
            if (escaping)
            {
                escaping = false;
                msgs.push_back(std::move(buff));
                buff.clear();
                break;
            }
            // no break
        default:
            if (escaping)
            {
                escaping = false;
                throw InvalidMessage((std::string)"Unexcepted '" + bytes[i] + "' after dilimiter");
            }
            buff.push_back(bytes[i]);
        }
}

