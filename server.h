#ifndef SERVER_H_
#define	SERVER_H_

#include "tcpsockethandler.h"
#include "thriftprotocolprocessor.h"
#include "consoleprocessor.h"

class WrapServerStart {
public:
    static void init() {
        initLog();
        const int kmeasureLogIntervalseconds = 10;
        g_measure.init(true, kmeasureLogIntervalseconds * 1000000);
        g_epoll.init(); 
    };
    static void run() {
        g_epoll.run();
    }
};

class ThriftServer {
public:
    ThriftServer();

    void init(uint16_t port, ::boost::shared_ptr<TAsyncProcessor> processor, ::boost::shared_ptr<TProtocolFactory> prot_factory);
    
private:
    ThriftProtocolProcessor m_processor;
    TcpSocketHandler m_socket_handler;
    
};

class ConsoleServer {
public:
    ConsoleServer();
    
    void init(uint16_t port, ConsoleHandler* console_handler);  
  
private:    
    ConsoleProcessor m_processor;
    TcpSocketHandler m_socket_handler; 
};

#endif	/* SERVER_H_ */

