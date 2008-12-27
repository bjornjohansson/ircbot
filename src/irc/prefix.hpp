#pragma once

#include <string>

#include <boost/regex.hpp>

namespace Irc
{

class Prefix
{
public:
    /**
     * @throw Exception if parsing of the prefix fails catastrophically.
     */
    Prefix() {};
    explicit Prefix(const std::string& text);

    const std::string& GetComplete() const { return complete_; }
    const std::string& GetNick() const { return nick_; }
    const std::string& GetUser() const { return user_; }
    const std::string& GetHost() const { return host_; }

private:
    std::string complete_;
    std::string nick_;
    std::string user_;
    std::string host_;

    static boost::regex textRegex_;
};



} // namespace Irc
