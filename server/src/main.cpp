#include <vector>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include "User.h"
#include "Conn.h"
#include "config.h"
#include "Context.h"
#include "Message.h"
#include "3rd-party/json.hpp"
#include "exception/CmdFailed.h"

std::unordered_map<std::string, User> users;
std::vector<Message> messages;

using Json = nlohmann::json;

/** Try to register ("register" is a C++ keyword)
 *  @throw : CmdFailed
 */
void signUp(const std::string &name, const std::string &password)
{
    if (users.count(name))
        throw CmdFailed("Register failed: Name already in use");
    if (password.empty())
        throw CmdFailed("Register failed: Password can't be empty");
    users[name] = (User){name, password, {}, {}};
}

/** Try to login
 *  @throw : CmdFailed
 */
void login(const std::string &name, const std::string &password)
{
    if (!users.count(name))
        throw CmdFailed("Login failed: Name not exists");
    if (users.at(name).password != password)
        throw CmdFailed("Login failed: Wrong password");
}

int main()
{
    Conn conn(PORT);
    conn.wait([](Context &context){
        while (context.isAvail())
        {
            try
            {
                Json msg = Json::parse(context.next());
                try
                {
                    if (msg["cmd"] == "register")
                    {
                        signUp(msg["name"], msg["password"]);
                        context.send(Json({{"register", "ok"}}).dump());
                        std::clog << "Registered " << msg["name"] << std::endl;
                    }
                    if (msg["cmd"] == "login")
                    {
                        login(msg["name"], msg["password"]);
                        context.setUser(msg["name"]);
                        context.send(Json({
                            {"login", "ok"},
                            {"contact", users.at(msg["name"]).contacts}
                        }).dump());
                        std::clog << msg["name"] << " logged in" << std::endl;
                    }
                    if (msg["cmd"] == "contact")
                    {
                        User &me = users.at(context.getUser());
                        if (!users.count(msg["name"]))
                            throw CmdFailed("Contact: No such user");
                        if (msg["op"] == "add")
                        {
                            me.contacts.push_back(msg["name"]);
                            context.send(Json({{"contact", "ok"}}).dump());
                        }
                        if (msg["op"] == "del")
                        {
                            me.contacts.erase(
                                std::remove(me.contacts.begin(), me.contacts.end(), msg["name"]),
                                me.contacts.end()
                            );
                            context.send(Json({{"contact", "ok"}}).dump());
                        }
                    }
                } catch (const CmdFailed &e)
                {
                    context.send(Json({{"info", e.what()}}).dump());
                }
            } catch (const std::invalid_argument &e)
            {
                throw InvalidMessage((std::string)"Invalid JSON: " + e.what());
            } catch (const std::domain_error &e)
            {
                throw InvalidMessage((std::string)"Invalid JSON: " + e.what());
            }
        }
    });
    return 0;
}

