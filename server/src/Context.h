#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <list>
#include <string>
#include "exception/InvalidMessage.h"

/** Context of a connection
 */
class Context
{
private:
    static constexpr char DILIMITER = '$';
    static constexpr char TERMINATOR = '.';

    bool escaping;
    std::string buff; // Unfinished (being receved) message
    std::list<std::string> msgs; /// List of received messages

public:
    const int fd; /// File descriptor

    const std::string ip;
    const unsigned short port;

    Context(int _fd, const char *_ip, unsigned short _port);
    ~Context();

    /** Add received bytes from Conn
     *  Protocal:
     *  | Body: of any length        | Dilimiter | Terminator
     *  | Escaped text, "$$" for "$" | "$"       | '.'
     *  @throw : InvalidMessage
     */
    void addBytes(const char *bytes, int len);

    /** If there are available messages?
     */
    bool isAvail() const { return !msgs.empty(); }

    /** Pop next message
     */
    std::string next() { auto ret = msgs.front(); msgs.pop_front(); return ret; }
};

#endif // CONTEXT_H_

