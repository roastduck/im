#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <ctime>
#include <string>
#include "3rd-party/json.hpp"

using Json = nlohmann::json;

struct Message
{
    time_t timestamp;
    std::string from, to, body;
};

/// Called by JSON library
void to_json(Json &j, const Message &m)
{
    j = {
        {"timestamp",   m.timestamp },
        {"from",        m.from      },
        {"to",          m.to        },
        {"body",        m.body      }
    };
}

#endif // MESSAGE_H_
