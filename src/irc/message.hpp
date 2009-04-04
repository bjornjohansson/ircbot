#pragma once

#include "prefix.hpp"
#include "command.hpp"

#include <string>
#include <vector>
#include <boost/regex.hpp>

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

    template<class T>
    const std::string& operator[](T index) const { return parameters_[index]; }
    ParameterContainer::size_type size() const { return parameters_.size(); }


private:
    Prefix prefix_;
    Command::Command command_;
    ParameterContainer parameters_;

    static boost::regex dataRegex_;
};

} // namespace Irc
