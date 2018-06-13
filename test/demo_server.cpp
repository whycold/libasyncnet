#include "server.h"   
#include "ThriftDemo.h"
#include <transport/TTransport.h>
#include <async/TAsyncProcessor.h>
#include <protocol/TProtocol.h>
#include <protocol/TBinaryProtocol.h>

using namespace test::demo;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using apache::thrift::async::TAsyncProcessor;

class DemoHandler : public ConsoleHandler, public ThriftDemoCobSvIf {
public:
    DemoHandler() {
    };

    
    /*for ConsoleHandler test*/
    virtual string onConsoleCallback(map<uint32_t, string> &argvs) {
        /*get hello, return world*/
        log(Info, "onConsoleCallback", argvs[0].c_str(), argvs[1].c_str());
        return "world";        
    }

    /*for ThriftDemoCobClIf test*/
    virtual void sendPing(std::tr1::function<void(std::string const& _return)> cob, const std::string& ping) {
        log(Info, "sendPing : %s", ping.c_str());
        printf("sendPing : %s", ping.c_str());
        cob("pong");
    }
    
public:

};


static uint16_t yy_server_port = 7700;
static uint16_t thrift_server_port = 7701;
static uint16_t console_port = 2500;


int main(int argc, char *argv[]) {
    WrapServerStart::init();
    
    DemoHandler demo_handler;
      
    shared_ptr<ThriftDemoCobSvIf> ptr_handler(&demo_handler);
    shared_ptr< TProtocolFactory > protocol_factory( new TBinaryProtocolFactory() );
    shared_ptr< TAsyncProcessor > processor( new ThriftDemoAsyncProcessor( ptr_handler ) );       
    ThriftServer thrift_server;
    thrift_server.init(thrift_server_port, processor, protocol_factory);
    
    ConsoleServer console_server;
    console_server.init(console_port, &demo_handler);
    
    WrapServerStart::run();
    return 1;
}

