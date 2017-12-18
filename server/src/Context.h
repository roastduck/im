#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <list>
#include <vector>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include "exception/InvalidMessage.h"

/** Context of a connection
 */
class Context
{
private:
    static constexpr char DILIMITER = '$';
    static constexpr char TERMINATOR = '.';

    static std::unordered_map<int, Context*> contexts;
    static std::unordered_map<std::string, std::vector<Context*>> userContexts;

    bool escaping;
    const int fd; /// File descriptor
    std::string buff; // Unfinished (being receved) message
    std::list<std::string> msgs; /// List of received messages

    std::string user;

    Context(int _fd, const char *_ip, unsigned short _port);
    ~Context();

public:
    static void add(int fd, const char *ip, unsigned short port);
    static void del(int fd);
    static Context &getByFd(int fd) { return *contexts.at(fd); }
    static const std::vector<Context*> &getByUser(const std::string &name) { return userContexts.at(name); }

    const std::string ip;
    const unsigned short port;

    void setUser(const std::string &_user);
    const std::string &getUser() const { return user; }

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

    /** Send message
     */
    void send(const std::string &_msg);
};

#endif // CONTEXT_H_

