#include <iostream>
#include <algorithm>
#include <unistd.h>
#include "Context.h"

std::unordered_map<int, Context*> Context::contexts;
std::unordered_map<std::string, std::vector<Context*>> Context::userContexts;

void Context::add(int fd, const char *ip, unsigned short port)
{
    contexts[fd] = new Context(fd, ip, port);
}

void Context::del(int fd)
{
    delete contexts.at(fd);
    contexts.erase(fd);
}

Context::Context(int _fd, const char *_ip, unsigned short _port)
    : escaping(false), fd(_fd), ip(_ip), port(_port)
{
    std::clog << "Accepting incoming connection from " + ip + ":" + std::to_string(port) << std::endl;
}

Context::~Context()
{
    setUser("");
    std::clog << "Connection from " + ip + ":" + std::to_string(port) << " closed" << std::endl;
}

void Context::setUser(const std::string &_user)
{
    if (!user.empty())
    {
        auto &old = userContexts.at(user);
        old.erase(std::remove(old.begin(), old.end(), this), old.end());
    }
    user = _user;
    if (!user.empty())
        userContexts[user].push_back(this);
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

void Context::send(const std::string &_msg)
{
    std::string msg = _msg + "$.\n";
    write(fd, msg.c_str(), msg.length());
}

