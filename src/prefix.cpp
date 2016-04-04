#include "prefix.hpp"
#include "exception.hpp"

boost::regex Prefix::textRegex_;

Prefix::Prefix(const std::string& text)
{
    try
    {
        if (textRegex_.empty())
        {
            textRegex_ = "([^!]+)(?:!([^@]+)@(.+))?";
        }
        boost::smatch matches;
        if (boost::regex_match(text, matches, textRegex_))
        {
            // matches[0] is the entire match, submatches start at 1
            nick_ = matches[1];
            user_ = matches[2];
            host_ = matches[3];
        }
    } catch (boost::regex_error& e)
    {
        throw Exception(__FILE__, __LINE__, e.what());
    }
}

