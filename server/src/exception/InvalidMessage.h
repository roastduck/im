#ifndef INVALID_MESSAGE_H_
#define INVALID_MESSAGE_H_

#include <string>
#include <stdexcept>

class InvalidMessage : public std::runtime_error
{
public:
    InvalidMessage(const std::string &what)
        : std::runtime_error(what)
    {}
};

#endif // INVALID_MESSAGE_H_
