#pragma once

#include <locale>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

class RegExp
{
public:
    /**
     * @throw Exception if the regexp cannot be compiled
     */
    RegExp(const std::string& regExp,
	   const std::string& reply,
	   const std::locale& locale = std::locale());

    std::string FindMatchAndReply(const std::string& message) const;
    
    const std::string& GetRegExp() const;
    const std::string& GetReply() const;

    void SetReply(const std::string& reply);
private:
    std::string regExp_, reply_;

    boost::regex pattern_;
};
