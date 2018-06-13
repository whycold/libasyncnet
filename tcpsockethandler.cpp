#include "tcpsockethandler.h"
#include <assert.h>
#include "measure.h"
#include "itcpprocessor.h"
#include "connmanager.h"


TcpSocketHandler::TcpSocketHandler() {
}

TcpSocket* TcpSocketHandler::createTcpListener(uint32_t uiIP, uint16_t &iPort, TcpSocketHandler *handler, uint32_t timeout, bool auto_increase_port) {
    TcpSocket* listenRes = new TcpSocket(&g_epoll);
    if (listenRes->listen(uiIP, iPort, auto_increase_port) == false) {
        log(Error, "TcpSocketHandler::createTcpListener tcp port %u listen failed", iPort);
        return NULL;
    }
    listenRes->m_pHandler = handler;
    listenRes->setListenSocket(true);
    //log(Debug, "Calling g_epoll.SetEvent");
    g_epoll.setEvent(listenRes, 0, SEL_READ);
    return listenRes;
}

int TcpSocketHandler::onListenConnect(TcpSocket *pSocket){
    assert(pSocket);
    TcpSocket *pNewSocket = ((TcpSocket*) pSocket)->accept();
    if (pNewSocket == NULL) {
        log(Error, "server accept fail");
        return 0;
    }
    
    log(Info, "accept %s:%d", ip2str(pNewSocket->m_ip), pNewSocket->m_iPort);
    pNewSocket->m_pHandler = this;
    g_epoll.setEvent(pNewSocket, 0, SEL_READ);
    g_conn_manager.newConn(pNewSocket);
    return 0;
}

void TcpSocketHandler::onConnected(TcpSocket *p) {
    log(Info, "TcpSocketHandler::onConnected");
}
void TcpSocketHandler::onClose(TcpSocket *p) {
    log(Info, "TcpSocketHandler::onClose");
}

int TcpSocketHandler::onDataRecv(const char* buf, int len, TcpSocket* pSocket) {
    assert(pSocket);
    return m_processor->doProcess(buf, len, pSocket);
}
