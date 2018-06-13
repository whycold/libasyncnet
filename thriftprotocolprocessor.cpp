#include "thriftprotocolprocessor.h"
#include <assert.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <transport/TTransportException.h>

using ::apache::thrift::TException;

ThriftProtocolProcessor::ThriftProtocolProcessor(){
}

int ThriftProtocolProcessor::doProcess(const char* buf, int len, TcpSocket* psocket) {
    assert(psocket);
    char *input = (char*) buf;
    uint32_t buf_left = len;
    uint32_t buf_consumed = 0;
    boost::shared_ptr<TMemoryBuffer> recv_buffer(new TMemoryBuffer());
    size_t frame_size = 0;
    int ret_readframe = 0;
    while ((ret_readframe = readFrame(input, buf_left, frame_size)) == 1) {
        try {
            recv_buffer->write((const uint8_t *) (input), frame_size);
            if (doRequest(recv_buffer, psocket->m_conn_id) == -1) {
                log(Error, "TcpSocketHandler:: onDataRecv doThriftRequest error.");
                return -1;
            }
            buf_consumed += frame_size;
            buf_left -= frame_size;
            input += frame_size;
        } catch (TException& ex) {
            log(Error, "ThriftProtocolProcessor::doProcess TException,info:%s", ex.what());
            return -1;
        } catch (...) {
            log(Error, "ThriftProtocolProcessor::doProcess unkown error.");
            return -1;
        }
    }
    if (ret_readframe == -1) return -1;
    return buf_consumed;
}

void ThriftProtocolProcessor::init(shared_ptr<TAsyncProcessor> processor, shared_ptr<TProtocolFactory> prot_factory) {
    m_asyn_processor   = processor;
    m_protocol_factory = prot_factory;
}

int ThriftProtocolProcessor::doRequest(shared_ptr<TMemoryBuffer> recv_buffer, uint32_t conn_id) {
    std::tr1::function<void(bool ok) > cob;
    try {
        shared_ptr<TTransport> input_transport(new TFramedTransport(recv_buffer));
        shared_ptr<TTransport> bridge_transport(new ThriftBridgeTransport(conn_id));
        shared_ptr<TTransport> output_transport(new TFramedTransport(bridge_transport));
        shared_ptr<TProtocol> input_protocol = m_protocol_factory->getProtocol(input_transport);
        shared_ptr<TProtocol> output_protocol = m_protocol_factory->getProtocol(output_transport);
        cob = boost::bind(&ThriftProtocolProcessor::compere, this, output_protocol, _1);
        m_asyn_processor->process(cob, input_protocol, output_protocol);
    } catch (TException& ex) {
        log(Error, "ThriftProtocolProcessor::doRequest TException,info:%s", ex.what());
        cob(false);
        return -1;
    } catch (...) {
        cob(false);
        log(Error, "ThriftProtocolProcessor::doRequest unkown error.");
        return -1;
    }
    return 0;
}


int ThriftProtocolProcessor::readFrame(const char* data, size_t size, size_t& frame_size)
{
    if (size < 4) {
        return 0; // need more data
    }
    int32_t sz = 0;
    sz = *( (const int32_t*)(data) );
    sz = ntohl(sz);
    if (sz < 0) {
	log(Info, "ThriftProtocolProcessor::readFrame size is  negative:%d", sz);		
	return -1;
    }
    frame_size = static_cast<size_t>(sz + 4);
    if (frame_size > (10*1024*1024)) {
	log(Info, "ThriftProtocolProcessor::readFrame: data is too long!  frame_size=%Zu,  Limit=10MB", frame_size);	
	return -1;
    }	
    if (frame_size <= size)
	return 1;
    else
	return 0; // need more data
}

// 传oport只是为了ouputprotocol能在处理完请求后再析构
void ThriftProtocolProcessor::compere(shared_ptr<TProtocol>& oport, bool ok) {
    if (!ok) {
        log(Warn, "compere failed to compere");
    }  
}