#ifndef USER_H_
#define USER_H_

#include <vector>
#include <string>
#include "Message.h"

struct User
{
    std::string name, password;
    std::vector<std::string> contacts;
    std::vector<int> messages;
};

#endif // USER_H_

