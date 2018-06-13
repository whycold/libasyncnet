//======================================================================================
#include "tcpsocket.h"
#include "eventloop.h"
#include "logger.h"
#include "measure.h"
#include "tcpsockethandler.h"
#include "connmanager.h"
//======================================================================================
#define DEBUG 0


TcpSocket::TcpSocket(SelectorEPoll* epoll) :
SocketBase(1), m_pSelector(epoll), m_bListenSocket(false), m_Enable(true),
m_bConnected(false), m_pHandler(NULL), m_conn_id(0){
}
//======================================================================================

TcpSocket::~TcpSocket() {
    //log(Info, "TcpSocket::~TcpSocket for socket:%d addr:%s", m_iSocket, ip2str(m_ip));
    g_epoll.socketDeleted((SocketBase*) this);
    g_conn_manager.eraseConn(this);
    if (m_iSocket != -1) {
        ::close(m_iSocket);
    }
}

void TcpSocket::close() {
    log(Debug, "TcpSocket::close iSocket: %d, %s", m_iSocket, ip2str(m_ip));
    g_epoll.removeSocket((SocketBase*) this);
    m_Enable = false;
}

void TcpSocket::onClose(){
    if(m_pHandler){
        m_pHandler->onClose(this);
        m_pHandler = NULL;
    }
}

bool TcpSocket::setListenSocket(bool isListen) {
    m_bListenSocket = isListen;
    return true;
}

//======================================================================================

int TcpSocket::onReadSocket() {
    //log(Debug, "TcpSocket::onReadSocket");

    if (m_pHandler == NULL) {
        log(Error, "TcpSocket::onReadSocket m_pHandler == NULL");
        return -1;
    }
    if (m_bListenSocket) {
        //log(Debug, "TcpSocket::onReadSocket is Listen Socket");
        m_pHandler->onListenConnect(this);
        return 0;
    }

    int ret = 0;
    try {
        ret = m_input.pump(*this);
        if (ret > 0) {
            ret = m_pHandler->onDataRecv(m_input.data(), m_input.size(), this);
            if (ret > 0) {
                m_input.erase(0, ret);
            } else if(ret < 0){
                log(Error, "TcpSocket::onReadSocket close the socket initiative socketL:%d addr:%s", m_iSocket, ip2str(m_ip));
                close();
            }
        } else {
            log(Info, "TcpSocket::onReadSocket close the socket reset by peer socketL:%d %s:%d", m_iSocket, ip2str(m_ip), m_iPort);
            close();
        }
    } catch (std::exception &ex) {
        log(Error, "excpetion, %s,%s", (std::string("process error:") + ex.what()).data(), ip2str(m_ip));
        close();
    }

    return ret;
}
//======================================================================================

int TcpSocket::onWriteSocket() {
    if (m_bConnected == false) {
        m_bConnected = true;
        if(m_pHandler){
            m_pHandler->onConnected(this);
        }
        return 0;
    }
    int res = m_output.flush(*this);
    if (res < 0) {
        log(Warn, "TcpSocket::onWriteSocket  reset by peer, id=%d %s:%d", m_iSocket, ip2str(m_ip), m_iPort);
        close();
    } else {
        if (m_output.empty()) {
            //log(Debug, "onWrite SetEvent remove");
            g_epoll.setEvent(this, SEL_WRITE, 0);
        }
    }
    return 0;
}

int TcpSocket::pushBin(const char* data, size_t len) {
    if (!m_bConnected) {
        log(Warn, "TcpSocket::pushBin not connected, %s:%d", ip2str(m_ip), m_iPort);
        return -1;
    }

    return m_output.write(data, len);
}

void TcpSocket::flush(){
    //scoped_measure m(63, 0, 0);
    if(m_Enable == false){
        log(Warn, "TcpSocket::flush but m_Enable=false");
        return;
    }
    if(m_output.empty()){
        return;
    }
    
    int res = m_output.flush(*this);
    if (res < 0) {
        log(Warn, "TcpSocket::flush close the socket reset by peer socketL:%u %s:%d", m_iSocket, ip2str(m_ip), m_iPort);
        close();
        return;
    }
    if (!m_output.empty()) {
        g_epoll.setEvent(this, 0, SEL_WRITE);
    }
    return;
}

int TcpSocket::sendBin(const char* data, size_t len) {
    if(m_Enable == false){
        log(Warn, "TcpSocket::sendBin but m_Enable=false");
        return -1;
    }
    //scoped_measure m(61, 0, 0);
    int ret = pushBin(data, len);
    if( ret < 0){
        return ret;
    }
    flush();
    //g_epoll.SetEvent(this, 0, SEL_WRITE);
    return 0;
}


//======================================================================================

bool TcpSocket::connect(uint32_t uiIP, uint16_t iPort) {
    m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_iSocket == -1) {
        log(Error, "TcpSocket::connect  socket fail.");
        return false;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = uiIP;
    address.sin_port = htons(iPort);

    m_ip = uiIP;
    m_iPort = iPort;
    m_bConnected = false;
    setNonBlock();
    g_epoll.setEvent(this, 0, SEL_RW);

    if (::connect(m_iSocket, (struct sockaddr*) &address, sizeof (address)) == -1) {
        if (errno == EINPROGRESS) {
            log(Debug, " TcpSocket::connect in progress. ip=%s", ip2str(uiIP));
            return true;
        }
        log(Warn, " TcpSocket::connect %s:%d fail.", ip2str(uiIP), iPort);
        close();
        return false;
    }
    
    log(Info, "TcpSocket::connect in time.");
    g_epoll.setEvent(this, 0, SEL_RW);// this will trigger onConnect next time.
    
    return true;
}
//======================================================================================

bool TcpSocket::listen(uint32_t uiIP, uint16_t &iPort, bool bTryNext) {
    m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_iSocket == -1) {
        log(Error, "TcpSocket::listen get socket fail");
        return false;
    }

    int op = 1;
    if (-1 == setsockopt(m_iSocket, SOL_SOCKET, SO_REUSEADDR, &op, sizeof (op))) {
        log(Error, "TcpSocket::listen set socket fail");
        close();
        return false;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof (address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = (uiIP);
    for (int i = 0; i < 100; i++) {
        m_iPort = iPort;
        address.sin_port = htons(m_iPort);
        if (::bind(m_iSocket, (struct sockaddr *) &address, sizeof (address)) < 0) {
            if (bTryNext == false) {
                log(Error, "TcpSocket::listen bind tcp socket port: %u failed", m_iPort);
                return false;
            }
            iPort++;
            continue;
        }

        //bind success
        if (::listen(m_iSocket, SOMAXCONN) == -1) {
            log(Error, "TcpSocket listen tcp socket port %u failed", m_iPort);
            close();
            return false;
        }

        setNonBlock();
        //log(Info, "bind tcp socket:%u port:%u successed", m_iSocket, m_iPort);
        return true;
    }
    close();
    log(Error, "try to bind tcp socket port: %u-%u fails", m_iPort - 100, m_iPort);
    return false;
}
//======================================================================================

TcpSocket* TcpSocket::accept() {
    struct sockaddr_in sa;
    socklen_t len = sizeof (sa);
    //log(Info, "TcpSocket::Accept m_iSocket=%d", m_iSocket);
    int ret = ::accept(m_iSocket, (struct sockaddr*) &sa, &len);
    if (-1 == ret) {
        log(Error, "TcpSocket::accept -1!");
        return NULL;
    }
    if (0 == ret) {
        log(Error, "TcpSocket::accept 0!");
        return NULL;
    }

    TcpSocket *pNew = new TcpSocket(m_pSelector);
    pNew->m_iSocket = ret;
    pNew->m_ip = htonl(sa.sin_addr.s_addr);
    pNew->m_iPort = htons(sa.sin_port);
    pNew->m_bConnected = true;
    
    pNew->setNonBlock();
    //log(Debug, "TcpSocket::Accept %s:%d", ip2str(pNew->m_ip), pNew->m_iPort);
    return pNew;
}
//======================================================================================

void TcpSocket::setNonBlock() {
    int fflags = fcntl(m_iSocket, F_GETFL);
    if (-1 == fflags) {
        log(Error, "TcpSocket::setNonBlock error, socket id:%d, %s:%d", m_iSocket, ip2str(m_ip), m_iPort);
        return;
    }
    fflags |= O_NONBLOCK;
    fcntl(m_iSocket, F_SETFL, fflags);
}
//======================================================================================

void TcpSocket::setRC4Key(const unsigned char *data, size_t len) {
    m_output.setRC4Key(data, len);
    m_input.setRC4Key(data, len);
}
//======================================================================================
#define ENCODE_BUFFER 1024*1024
//======================================================================================

RC4Alloc::RC4Alloc() {
    writeBuffer = new unsigned char[ENCODE_BUFFER];
    curSize = ENCODE_BUFFER;
}
//======================================================================================

RC4Alloc::~RC4Alloc() {
    delete[] writeBuffer;
}
//======================================================================================

void RC4Alloc::realloc(size_t sz) {
    delete[] writeBuffer;
    writeBuffer = new unsigned char[sz];
    curSize = sz;
    log(Info, "RC4Alloc::realloc %d", sz);
}
//======================================================================================

RC4Alloc RC4Filter::writeBuffer;

//======================================================================================

RC4Filter::RC4Filter() {
    encrypted = false;
}
//======================================================================================

RC4Filter::~RC4Filter() {

}

//======================================================================================

void RC4Filter::filterRead(char *data, size_t sz) {
    if (encrypted) {
        RC4(&rc4, sz, (unsigned char *) data, (unsigned char *) data);
    }
}
//======================================================================================

const char* RC4Filter::filterWrite(const char *data, size_t sz) {
    if (encrypted) {
        if (sz > writeBuffer.getCurSize()) {
            //log(Info, "RC4Filter::filterWrite size:%u", sz);
            writeBuffer.realloc(sz);
        }
        RC4(&rc4, sz, (unsigned char *) data, writeBuffer.getWriteBuffer());
        return (const char*)writeBuffer.getWriteBuffer();
    } else {
        //log(Info, "RC4Filter::filterWrite not encrypted size:%u", sz);
        return data;
    }

}
//======================================================================================

bool RC4Filter::isEncrypto() const {
    return encrypted;
}
//======================================================================================

void RC4Filter::setRC4Key(const unsigned char *data, size_t len) {
    RC4_set_key(&rc4, len, data);
    encrypted = true;
}

//======================================================================================
//end
