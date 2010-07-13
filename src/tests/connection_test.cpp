#include "connection_test.hpp"
#include "../exception.hpp"
#include "testserver.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION( ConnectionTest );

void ConnectionTest::setUp()
{}

void ConnectionTest::tearDown()
{}

void ConnectionTest::TestConstructor()
{
    unsigned short testPort = 9999;
    TestServer testServer(testPort);

    Connection connection;

    Connection::ReceiverHandle handle 
	= connection.RegisterReceiver(boost::bind(&ConnectionTest::Receive, this, _1, _2));

    boost::unique_lock<boost::mutex> lock(receiverMutex_);

    connection.Connect("localhost", testPort);

    receiverReady_.wait(lock);
}

void ConnectionTest::Receive(Connection& connection, const std::vector<char>& data)
{
    std::vector<char>::const_iterator t=std::find(data.begin(), data.end(), 0);
    
    CPPUNIT_ASSERT(t != data.end());
    
    std::string receivedString = &data[0];

    CPPUNIT_ASSERT(receivedString == TEST_SERVER_WELCOME_MESSAGE);

    receiverReady_.notify_all();
}
