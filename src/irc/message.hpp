#pragma once

#include "prefix.hpp"
#include "command.hpp"

#include <vector>
#include <boost/regex.hpp>
#include <unicode/unistr.h>
#include <converter.hpp>

namespace Irc
{

class Message
{
public:
    /**
     * @throw Exception if analyzing of the data fails catastrophically.
     */
    explicit Message(const std::string& data);

    typedef std::vector<std::string> ParameterContainer;
    typedef std::pair<ParameterContainer::const_iterator,
		      ParameterContainer::const_iterator> ParameterIterators;

    const Prefix& GetPrefix() const { return prefix_; }
    const Command::Command& GetCommand() const { return command_; }
    ParameterIterators GetParameters() const
	{ return ParameterIterators(parameters_.begin(), parameters_.end()); }

    typedef ParameterContainer::const_iterator const_iterator;

    const_iterator begin() const { return parameters_.begin(); }
    const_iterator end() const { return parameters_.end(); }

    template<class T>
    std::string operator[](T index) const { return parameters_[index]; }
    ParameterContainer::size_type size() const { return parameters_.size(); }

    const std::string& GetTarget() const;
    const std::string& GetText() const;
    const std::string& GetReplyTo() const;

    bool IsCtcp() const { return isCtcp_; }

private:
    void CheckCtcp();

    Prefix prefix_;
    Command::Command command_;
    ParameterContainer parameters_;
    bool isCtcp_;

    static boost::regex dataRegex_;
};

} // namespace Irc
