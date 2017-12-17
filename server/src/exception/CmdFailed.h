#ifndef CMD_FAILED_H_
#define CMD_FAILED_H_

#include <string>
#include <stdexcept>

class CmdFailed : public std::runtime_error
{
public:
    CmdFailed(const std::string &what)
        : std::runtime_error(what)
    {}
};

#endif // CMD_FAILED_H_
