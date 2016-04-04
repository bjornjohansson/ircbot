#pragma once

#include "prefix.hpp"

#include <vector>
#include <unicode/unistr.h>

class Message
{
public:
    /**
     * @throw Exception if analyzing of the data fails catastrophically.
     */
    Message() {}

    typedef std::vector<std::string> ParameterContainer;
    typedef std::pair<ParameterContainer::const_iterator,
                      ParameterContainer::const_iterator> ParameterIterators;

    const Prefix& GetPrefix() const { return prefix_; }

    typedef ParameterContainer::const_iterator const_iterator;

    const_iterator begin() const { return parameters_.begin(); }
    const_iterator end() const { return parameters_.end(); }

    template<class T>
    const std::string& operator[](T index) const { return parameters_[index]; }
    ParameterContainer::size_type size() const { return parameters_.size(); }

    virtual const std::string& GetTarget() const = 0;
    virtual const std::string& GetText() const = 0;
    virtual const std::string& GetReplyTo() const = 0;
protected:
    Prefix prefix_;
    ParameterContainer parameters_;
};

