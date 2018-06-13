#include <stdint.h>
#include <netdb.h>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>
#include <vector>
#include <set>
#include <map>
#include "ThriftDemo.h"
#include "logger.h"

using namespace std;
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::boost;
using namespace ::test::demo;


static void DoThriftTest(
  const std::string& host, int port
  )
{
  printf( "running DoThriftTest( %s, %d) ...\n",
    host.c_str(), port);
  TSocket* socket = new TSocket( host, port );
  socket->setRecvTimeout( 10000 );// 10 seconds
  shared_ptr< TTransport > transport_socket( socket );
  shared_ptr< TTransport > transport( new TFramedTransport( transport_socket ) );
  shared_ptr< TProtocol > protocol( new TBinaryProtocol( transport ) );
  ThriftDemoClient client( protocol );
  try
  {
    transport->open();

    string res;
    client.sendPing(res, "ping");
    printf("sendPing end, res:%s\n", res.c_str());
    
    transport->close();
  }
  catch ( TException& tx )
  {
    fprintf( stderr, "Caught TException: %s\n", tx.what() );
	transport->close();
  }
  printf( "done DoThriftTest().\n" );
}

int main( int argc, char* argv[] )
{
  DoThriftTest( "127.0.0.1", 7701 );
  
  return 0;
}

