#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <list>
#include <string>

/** Context of a connection
 */
class Context
{
private:
    std::list<std::string> msgs; /// List of received messages

public:
    const int fd; /// File descriptor

    const std::string ip;
    const unsigned short port;

    Context(int _fd, const char *_ip, unsigned short _port);
    ~Context();

    void addBytes(const char *bytes, int len);
};

#endif // CONTEXT_H_

