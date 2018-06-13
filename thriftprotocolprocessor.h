#ifndef THRIFTPROTOCOLPROCESSOR_H_
#define	THRIFTPROTOCOLPROCESSOR_H_

#include <stdint.h>
#include <map>
#include <transport/TBufferTransports.h>
#include <transport/TTransport.h>
#include <async/TAsyncProcessor.h>
#include <protocol/TProtocol.h>
#include <boost/shared_ptr.hpp>

#include "itcpprocessor.h"
#include "tcpsocket.h"
#include "connmanager.h"

using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using apache::thrift::async::TAsyncProcessor;
using ::boost::shared_ptr;

class ThriftBridgeTransport : public TVirtualTransport<ThriftBridgeTransport> {
public:

    ThriftBridgeTransport(uint32_t conn_id) : m_conn_id(conn_id) {
    };

    ~ThriftBridgeTransport() {
    };

    virtual bool isOpen() {
        return true;
    };

    virtual void open() {
    };

    virtual void close() {
    };

    virtual void write(const uint8_t* buf, uint32_t len) {
        m_buffer.write(buf, len);
    }

    virtual uint32_t writeEnd() {
        return m_buffer.writeEnd();
    };

    virtual void flush() {
        TcpSocket* psocket = g_conn_manager.getConn(m_conn_id);
        if ( psocket && psocket->m_Enable) {
            uint8_t* obuf;
            uint32_t sz;
            m_buffer.getBuffer(&obuf, &sz);
            psocket->sendBin((char *) (obuf), sz); 
            psocket->flush();
            m_buffer.resetBuffer();
        } else {
            log(Warn, "ThriftBridgeTransport::flush socket is already closed");
        }
    }

private:
    uint32_t m_conn_id;
    apache::thrift::transport::TMemoryBuffer m_buffer;
};

class ThriftProtocolProcessor : public ITcpProcessor{
public:
    ThriftProtocolProcessor();
    
    void init(::boost::shared_ptr<TAsyncProcessor> processor, ::boost::shared_ptr<TProtocolFactory> prot_factory);
 
    virtual int doProcess(const char* buf, int len, TcpSocket* psocket);   
    
    int  doRequest(::boost::shared_ptr<TMemoryBuffer> recv_buffer, uint32_t conn_id);    
    
private:
    int readFrame(const char* data, size_t size, size_t& frame_size);
    void compere(::boost::shared_ptr<TProtocol>& oport, bool ok);   

private:
    ::boost::shared_ptr<TAsyncProcessor>  m_asyn_processor;
    ::boost::shared_ptr<TProtocolFactory> m_protocol_factory;   
    
};

#endif	/* THRIFTPROTOCOLPROCESSOR_H_ */

