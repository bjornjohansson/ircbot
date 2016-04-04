#pragma once

#include "../message.hpp"
#include "command.hpp"

#include <vector>
#include <boost/regex.hpp>
#include <unicode/unistr.h>

namespace Irc
{

class IrcMessage : public Message
{
public:
    /**
     * @throw Exception if analyzing of the data fails catastrophically.
     */
    explicit IrcMessage(const std::string& data);

    typedef std::vector<std::string> ParameterContainer;
    typedef std::pair<ParameterContainer::const_iterator,
                      ParameterContainer::const_iterator> ParameterIterators;

    const Prefix& GetPrefix() const { return prefix_; }
    const Command::Command& GetCommand() const { return command_; }

    typedef ParameterContainer::const_iterator const_iterator;

    const_iterator begin() const { return parameters_.begin(); }
    const_iterator end() const { return parameters_.end(); }

    template<class T>
    std::string operator[](T index) const { return parameters_[index]; }
    ParameterContainer::size_type size() const { return parameters_.size(); }

    virtual const std::string& GetTarget() const;
    virtual const std::string& GetText() const;
    virtual const std::string& GetReplyTo() const;

    bool IsCtcp() const { return isCtcp_; }

private:
    void CheckCtcp();

    Command::Command command_;
    bool isCtcp_;

    static boost::regex dataRegex_;
};

} // namespace Irc
