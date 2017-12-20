#include <ctime>
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
std::unordered_map<std::string, std::string> profiles;
std::vector<Message> messages;

using Json = nlohmann::json;

/** Try to register ("register" is a C++ keyword)
 *  @throw : CmdFailed
 */
void signUp(const std::string &name, const std::string &password)
{
    if (users.count(name))
        throw CmdFailed("Register failed: Name already in use");
    if (name.empty())
        throw CmdFailed("Register failed: Name can't be empty");
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

/** Get messages since a timestamp
 *  @return MAX_LOG_NUM logs at maximum, except that there are same timestamps
 */
std::vector<Message> getLogSince(const std::string &name, time_t since)
{
    if (!users.count(name))
        throw CmdFailed("Get log failed: No such user");
    const auto &usrMsgs = users.at(name).messages;
    int l = 0, r = usrMsgs.size();
    while (l < r)
    {
        int mid = (l + r) / 2;
        if (messages.at(usrMsgs.at(mid)).timestamp < since)
            l = mid + 1;
        else
            r = mid;
    }
    assert(l == int(usrMsgs.size()) || messages.at(usrMsgs.at(l)).timestamp >= since);
    assert(l == 0 || messages.at(usrMsgs.at(l - 1)).timestamp < since);
    std::vector<Message> ret;
    ret.reserve(MAX_LOG_NUM);
    for (int i = 0; l + i < int(usrMsgs.size()); i++)
    {
        if (i >= MAX_LOG_NUM && messages.at(usrMsgs.at(l + i)).timestamp != messages.at(usrMsgs.at(l + i - 1)).timestamp)
            break;
        ret.push_back(messages.at(usrMsgs.at(l + i)));
    }
    return ret;
}

/** Update contacts of user `myName`
 *  @param op : "add" or "del"
 *  @return : Updated contacts
 */
const std::vector<std::string> &updContact(const std::string &myName, const std::string &op, const std::string &name)
{
    User &me = users.at(myName);
    if (name == myName)
        throw CmdFailed("Contact: Contact can't be yourself");
    if (!users.count(name))
        throw CmdFailed("Contact: No such user");
    if (op == "add")
    {
        if (std::find(me.contacts.begin(), me.contacts.end(), name) != me.contacts.end())
            throw CmdFailed("Contact: Contact already exists");
        me.contacts.push_back(name);
    } else
    if (op == "del")
    {
        me.contacts.erase(
            std::remove(me.contacts.begin(), me.contacts.end(), name),
            me.contacts.end()
        );
    } else
        throw CmdFailed("Contact: Unrecognized op " + op);
    return me.contacts;
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
                    } else
                    if (msg["cmd"] == "login")
                    {
                        if (!context.getUser().empty())
                            throw CmdFailed("Login failed: You are already logged in");
                        login(msg["name"], msg["password"]);
                        context.setUser(msg["name"]);
                        context.send(Json({
                            {"login", "ok"},
                            {"contact", users.at(msg["name"]).contacts},
                            {"profile", profiles}
                        }).dump());
                        std::clog << msg["name"] << " logged in" << std::endl;
                    } else
                    {
                        if (context.getUser().empty())
                            throw CmdFailed("Failed: You have not been logged in");
                        if (msg["cmd"] == "contact")
                        {
                            context.send(Json({
                                {"contact", updContact(context.getUser(), msg["op"], msg["name"])}
                            }).dump());
                            const auto &opposite = updContact(msg["name"], msg["op"], context.getUser());
                            for (const auto c : Context::getByUser(msg["name"]))
                                c->send(Json({{"contact", opposite}}).dump());
                            std::clog << context.getUser() << " updated contact " << msg["name"] << std::endl;
                        }
                        if (msg["cmd"] == "chat")
                        {
                            if (!users.count(msg["name"]))
                                throw CmdFailed("Chat: No such user");
                            User &from = users.at(context.getUser());
                            User &to = users.at(msg["name"]);
                            int id = messages.size();
                            messages.push_back({time(0), from.name, to.name, msg["message"]});
                            from.messages.push_back(id);
                            to.messages.push_back(id);
                            for (Context *c : Context::getByUser(to.name))
                                c->send(Json({{"income", {messages.at(id)}}}).dump());
                            context.send(Json({{"chat", "ok"}}).dump());
                            std::clog << "Received message from " << from.name << " to " << to.name << std::endl;
                        }
                        if (msg["cmd"] == "log")
                        {
                            auto log = getLogSince(context.getUser(), msg["since"]);
                            context.send(Json({
                                {"income", log},
                                {"log", log.empty() ? "fin" : "more"}
                            }).dump());
                            std::clog << "Returned log to " << context.getUser() << std::endl;
                        }
                        if (msg["cmd"] == "profile")
                        {
                            profiles[context.getUser()] = msg["data"];
                            std::clog << "Updated profile of " << context.getUser() << std::endl;
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

