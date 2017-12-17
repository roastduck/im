#ifndef NETWORK_ERROR_H_
#define NETWORK_ERROR_H_

#include <cerrno>
#include <cstring>
#include <string>
#include <stdexcept>

class NetworkError : public std::runtime_error
{
public:
    NetworkError(const std::string &what) :
        std::runtime_error(what + " - ERR: " + strerror(errno))
    {}
};

#endif // NETWORK_ERROR_H_
