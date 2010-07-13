#pragma once

#include <cppunit/extensions/HelperMacros.h>

#include "../connection/connection.hpp"

#include <vector>

class ConnectionTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( ConnectionTest );

    CPPUNIT_TEST( TestConstructor );

    CPPUNIT_TEST_SUITE_END();
public:
    void setUp();
    void tearDown();

    void TestConstructor();

    void Receive(Connection& connection, const std::vector<char>& data);

private:

    boost::condition_variable receiverReady_;
    boost::mutex receiverMutex_;

};
