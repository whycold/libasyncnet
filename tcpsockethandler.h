#ifndef TCPSOCKETHANDLER_
#define TCPSOCKETHANDLER_

#include "tcpsocket.h"
#include "eventloop.h"

class ITcpProcessor;

class TcpSocketHandler {
public:
    TcpSocketHandler();

    virtual ~TcpSocketHandler() {
    };
        
    void init(ITcpProcessor* processor) {
        m_processor = processor;
    }
    
    TcpSocket* createTcpListener(uint32_t uiIP, uint16_t &iPort, TcpSocketHandler *handler, uint32_t timeout, bool auto_increase_port = true);   
           
    virtual int onListenConnect(TcpSocket*);
    virtual int onDataRecv(const char*, int, TcpSocket*);
    virtual void onClose(TcpSocket*);
    virtual void onConnected(TcpSocket*); 
    
protected:
    ITcpProcessor* m_processor;
};


#endif



