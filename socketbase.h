//======================================================================================
#ifndef _SOCKET_BASE_
#define _SOCKET_BASE_
//======================================================================================
#include "common.h"
//======================================================================================

class SocketBase {
public:

    SocketBase(uint8_t type = -1) {
        m_ip = -1;
        m_iPort = -1;
        m_iSocketType = type;
        m_iSocket = -1;
        m_pParam = NULL;
    }

    virtual ~SocketBase() {
    }

    virtual int onReadSocket() = 0;
    virtual int onWriteSocket() = 0;
    virtual void close() = 0;// called by app layer, main_loop will call onClose and delete socket after all event process.
    virtual int sendBin(const char* p, size_t len) = 0;

public:
    uint32_t m_ip;
    uint16_t m_iPort;
    uint8_t m_iSocketType; // 0:udp,1:tcp,2:event_fd

    int m_iSocket;
    void *m_pParam;
};

//======================================================================================
#endif

