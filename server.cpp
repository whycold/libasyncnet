#include "server.h"
#include "tcpsockethandler.h"
#include "logger.h"

/************************************************************************************/

ThriftServer::ThriftServer(){
}

void ThriftServer::init(uint16_t port, ::boost::shared_ptr<TAsyncProcessor> processor, ::boost::shared_ptr<TProtocolFactory> prot_factory) {
    m_processor.init(processor, prot_factory);
    m_socket_handler.init(&m_processor);
    
    if (m_socket_handler.createTcpListener(0, port, &m_socket_handler, 0, false) == NULL) {
        log(Error, "ThriftServer open port %d fail", port);
        printf("ThriftServer server create listen fail.\n");
        exit(1);
    }
    log(Info, "ThriftServer listen at %d.", port);         
}


/********************************************************************************/
ConsoleServer::ConsoleServer(){
}

void ConsoleServer::init(uint16_t port, ConsoleHandler* console_handler) {
    m_processor.init(console_handler);
    m_socket_handler.init(&m_processor);
    
    uint32_t ip = g_epoll.getIpByName("127.0.0.1");
    if (m_socket_handler.createTcpListener(ip, port, &m_socket_handler, 0, false) == NULL) {
        log(Error, "ConsoleServer open port %d fail", port);
        printf("ConsoleServer server create listen fail.\n");
        exit(1);
    }
    log(Info, "ConsoleServer listen at %d.", port);   
        
}
