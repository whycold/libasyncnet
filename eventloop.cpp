//======================================================================================
#include <netdb.h>
#include "eventloop.h"
#include "logger.h"
#include "measure.h"
#include "tcpsocket.h"

//======================================================================================
#define EPOLL_SIZE 100
#define EPOLL_TIMEOUT 100
static int pipe_err = 0;

#define DEBUG 0

SelectorEPoll * g_ptr_epoll = new SelectorEPoll();

SelectorEPoll& g_epoll = *g_ptr_epoll;
//======================================================================================

class BaseTimer: public TimerHandler{
    void timerCallback(){
        g_measure.measureDumpToLog();
    }
} g_base_timer;

//======================================================================================

static void epollPipeHandler(int) {
    pipe_err++;
}

static void segvHandler(int s){
    log(Error, "received Segmentation fault signal:%d", s);
    log(Error, "stack:%s", get_backtrace().c_str());
    exit(0);
}
static void fpeHandler(int s){
    log(Error, "received floating point error signal:%d", s);
    log(Error, "stack:%s", get_backtrace().c_str());
    exit(0);
}
static void termHandler(int s){
    log(Info, "received term signal:%d", s);
    g_epoll.set_exit(true);
}
//======================================================================================

SelectorEPoll::SelectorEPoll() :
m_hEPoll(-1), m_exit(false) {
    now_us = 0;
    
    m_poll_timeout = EPOLL_TIMEOUT;
}
//======================================================================================

SelectorEPoll::~SelectorEPoll() {
    if (-1 != m_hEPoll){
        close(m_hEPoll);
    }
}
//======================================================================================

bool SelectorEPoll::init() {
    if (signal(SIGPIPE, epollPipeHandler) == SIG_ERR) {
        printf("init register SIGPIPE fail.\n");
        exit(1);
    }
    if (signal(SIGSEGV, segvHandler) == SIG_ERR) {
        printf("init register SIGSEGV fail.\n");
        exit(2);
    }
    if (signal(SIGFPE, fpeHandler) == SIG_ERR) {
        printf("init register SIGFPE fail.\n");
        exit(3);
    }  

    m_hEPoll = epoll_create(65535);
    if (-1 == m_hEPoll){
        log(Error, "SelectorEPoll::Init epoll_create fail");
        printf("Epoll create error!\n");
        exit(-1);
    }

    g_base_timer.setTimer_us(10 * 1000000);
    now_us = g_measure.getPreciseTime();
    
    m_bRuning = true;
    return true;
}
//======================================================================================

void SelectorEPoll::stop() {
    log(Notice, "SelectorEPoll::Stop");
    m_bRuning = false;
}
//======================================================================================

void SelectorEPoll::run() {
    epoll_event events[EPOLL_SIZE];
    uint64_t tOldTime = g_measure.getPreciseTime();
    
    while (m_bRuning) {
        now_us = g_measure.getPreciseTime();

        // Check per second
        if (tOldTime + (10 * 1000) < now_us) {
            ScopedMeasure m(1, 0, 0);
            timerCheck(now_us);
            tOldTime = now_us;
        }

        int iActive;
        {
            ScopedMeasure m(0, 0, 0);
            iActive = epoll_wait(m_hEPoll, events, EPOLL_SIZE, m_poll_timeout);
        }
        if (m_exit) {
            ScopedMeasure m(4, 0, 0); 
            for (set<ExitHandler*>::iterator it = m_exitTasks.begin(); it != m_exitTasks.end(); it++) {
                ExitHandler* handler = *it;
                handler->exitCallback();
                if(m_exitTasks.find(handler) == m_exitTasks.end()){
                    // handler removed, continue next time.
                    break;
                }
            }           
        }        
        if (iActive < 0) {
            if (EINTR == errno){
                continue;
            }else{
                log(Error, "SelectorEPoll::Run epoll_wait error:%d", errno);
            }
        }
        if(iActive == 0 && m_poll_timeout == 0){           
            ScopedMeasure m(2, 0, 0); 
            for (set<IdleHandler*>::iterator it = m_idleTasks.begin(); it != m_idleTasks.end(); it++) {
                IdleHandler* handler = *it;
                handler->idleCallback();
                if(m_idleTasks.find(handler) == m_idleTasks.end()){
                    // handler removed, continue next time.
                    break;
                }
            }
        }

        for (int i = 0; i < iActive; ++i) {
            
            if (tOldTime + (10 * 1000) < now_us) {
                ScopedMeasure m(1, 0, 0);
                timerCheck(now_us);
                tOldTime = now_us;
            }
            
            //scoped_measure m(4, 0, 0);
            SocketBase *s = (SocketBase *) (events[i].data.ptr);
            if(s == NULL){
                log(Error, "SelectorEPoll::Run event SocketBase is NULL");
                continue;
            }
            //log(Debug, "epoll event %u", events[i].events);

            if (isRemoved(s)) {
                continue;
            }
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
                s->close();
                continue;
            }
            if (events[i].events & EPOLLIN) {
                onReadSocket(s);
            }
            if (isRemoved(s)) {
                continue;
            }
            if (events[i].events & EPOLLOUT) {
                onWriteSocket(s);
            }
        }

        {
            ScopedMeasure m(3, 0, 0);
            clearRemovedSocketsQueue();
        }
        if (iActive == EPOLL_SIZE) {
            log(Warn, "epoll reach the max size:%d m_setSocket:%d", 100,m_setSocket.size());
        }
    }
    log(Info, "exit the main loop");
}
//======================================================================================

void SelectorEPoll::timerCheck(uint64_t tNow) {
    for (set<TimerHandler*>::iterator it = m_setTimer.begin(); it != m_setTimer.end(); it++) {
        if (*it == NULL) {
            log(Error, "SelectorEPoll::timerCheck = NULL");
            continue;
        }
        //log(Debug, "TimeCheck %s %s", g_measure.time2string(tNow).c_str(), g_measure.time2string((*it)->m_nextTime).c_str());
        (*it)->timerCheck(tNow);
    }
}
//======================================================================================

void SelectorEPoll::ePollCtl(int iMethod, int iSocket, epoll_event ev) {
    int ret = epoll_ctl(m_hEPoll, iMethod, iSocket, &ev);

    if (ret != 0) {
        switch (errno) {
            case EBADF:
                log(Error, "m_hEPoll or fd is not a valid file descriptor. iSocket: %u method: %u", iSocket, iMethod);
                return;
            case EEXIST:
                log(Error, "was EPOLL_CTL_ADD, and the supplied file descriptor fd is already in m_hEPoll.");
                return;
            case EINVAL:
                log(Error, "m_hEPoll is not an epoll file descriptor, or fd is the same as m_hEPoll, or the requested operation op is not supported by this interface.");
                return;
            case ENOENT:
                log(Error, "op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not in m_hEPoll.iSocket: %u method: %u", iSocket, iMethod);
                return;
            case ENOMEM:
                log(Error, "There was insufficient memory to handle the requested op control operation.");
                return;
            case EPERM:
                log(Error, "The target file fd does not support epoll.");
                return;
        }
    }
}
//======================================================================================

void SelectorEPoll::setEvent(SocketBase* s, int remove, int add) {
    if (s == NULL) {
        log(Error, "setEvent s = NULL");
        return;
    }
    
    if (m_removed_socket_queue.find(s) != m_removed_socket_queue.end()) {
        log(Error, "setEvent from %s", get_backtrace().c_str());
        return;
    }
    
    epoll_event ev;
    ev.data.ptr = s;
    ev.events = EPOLLIN;

    //log(Debug, "SelectorEPoll::SetEvent socket:%u, remove:%d, add:%d", s->m_iSocket, remove, add);
    if (m_setSocket.find(s) == m_setSocket.end()) {
        m_setSocket.insert(s);
        log(Debug, "setEvent remove:%d add:%d m_setSocket.insert(%p)", remove, add, s);
        if (SEL_WRITE & add) {
            ev.events |= EPOLLOUT;
        }
        //log(Debug, "insert into the epoll, sockid:%u ip: %s", s->m_iSocket, ip2str(s->m_ip));
        ePollCtl(EPOLL_CTL_ADD, s->m_iSocket, ev);
    } else {
        if (SEL_READ & remove) {
            ePollCtl(EPOLL_CTL_MOD, s->m_iSocket, ev);
        }

        if (SEL_WRITE & remove) {
            ePollCtl(EPOLL_CTL_MOD, s->m_iSocket, ev);
        }

        if (SEL_READ & add) {
            ePollCtl(EPOLL_CTL_MOD, s->m_iSocket, ev);
        }

        if (SEL_WRITE & add) {
            ev.events |= EPOLLOUT;
            ePollCtl(EPOLL_CTL_MOD, s->m_iSocket, ev);
        }
    }
}
//======================================================================================

int SelectorEPoll::onReadSocket(SocketBase *pSocket) {
    int ret = 0;
    try {
        ret = pSocket->onReadSocket();
    } catch (std::exception & ex) {
        log(Error, "SelectorEPoll::onReadSocket Exception:%s, id=%u, %s:%d", ex.what(), pSocket->m_iSocket, ip2str(pSocket->m_ip), pSocket->m_iPort);
    }

    return ret;
}
//======================================================================================

int SelectorEPoll::onWriteSocket(SocketBase *pSocket) {
    int ret = 0;
    try {
        ret = pSocket->onWriteSocket();
    } catch (std::exception & ex) {
        log(Error, "SelectorEPoll::onWriteSocket Exception:%s, id=%u, %s:%d", ex.what(), pSocket->m_iSocket, ip2str(pSocket->m_ip), pSocket->m_iPort);
    }

    return ret;
}
//======================================================================================

void SelectorEPoll::removeSocket(SocketBase* s) {
    if (s == NULL) {
        log(Error, "SelectorEPoll::removeSocket s=NULL");
        return;
    }
    log(Debug, "removeSocket,sockid:%u sockaddr: %s:%d", s->m_iSocket, ip2str(s->m_ip), s->m_iPort);
    m_removed_socket_queue.insert(s);


    std::set<SocketBase *>::iterator it = m_setSocket.find(s);
    if (it == m_setSocket.end()) {
        return;
    }else{        
        if (s->m_iSocket != -1){
            epoll_event ev;
            ePollCtl(EPOLL_CTL_DEL, s->m_iSocket, ev);
        }
        
        m_setSocket.erase(it);
        log(Debug, "removeSocket m_setSocket.erase(%p)", s);
    }
}
void SelectorEPoll::socketDeleted(SocketBase* s) {
    if (s == NULL) {
        log(Error, "socketDeleted s=NULL");
        return;
    }
    
    std::set<SocketBase *>::iterator it = m_setSocket.find(s);
    if (it != m_setSocket.end()) {
        log(Error, "socketDeleted, sockid:%u in m_setSocket sockaddr: %s:%d from %s", s->m_iSocket, ip2str(s->m_ip), s->m_iPort, get_backtrace().c_str());
        if (s->m_iSocket != -1){
            //epoll_event ev;
            //EPollCtl(EPOLL_CTL_DEL, s->m_iSocket, ev);
        }
        
        m_setSocket.erase(it);
        log(Debug, "socketDeleted m_setSocket.erase(%p)", s);
    }
    
    std::set<SocketBase *>::iterator rit = m_removed_socket_queue.find(s);
    if(rit != m_removed_socket_queue.end()){
        log(Error, "socketDeleted, sockid:%u in m_removed_socket_queue sockaddr: %s:%d from %s", s->m_iSocket, ip2str(s->m_ip), s->m_iPort, get_backtrace().c_str());
        if (s->m_iSocket != -1){
            //epoll_event ev;
            //EPollCtl(EPOLL_CTL_DEL, s->m_iSocket, ev);
        }
        
        m_removed_socket_queue.erase(rit);
        log(Debug, "socketDeleted m_removed_socket_queue.erase(%p)", s);
    }
}

void SelectorEPoll::clearRemovedSocketsQueue() {

    for(set<SocketBase *>::iterator it=m_removed_socket_queue.begin(); it!=m_removed_socket_queue.end(); ){
        SocketBase *sb = *it;
        if(sb->m_iSocketType == 1){//  tcp
            TcpSocket *pSocket = (TcpSocket*)sb;
            pSocket->onClose();
        }
        m_removed_socket_queue.erase(it++);
        delete sb;
    }
    
}

//======================================================================================

void SelectorEPoll::addTimerHandler(TimerHandler *pHandler) {
    log(Info, "SelectorEPoll::addTimerHandler, %p", pHandler);
    if (m_setTimer.find(pHandler) != m_setTimer.end()) {
        log(Warn, "SelectorEPoll::addTimerHandler %p exist!", pHandler);
        return;
    }
    m_setTimer.insert(pHandler);
}

void SelectorEPoll::removeTimerHandler(TimerHandler *pHandler) {
    log(Info, "SelectorEPoll::removeTimerHandler");
    if (m_setTimer.find(pHandler) == m_setTimer.end()) {
        log(Error, "SelectorEPoll::removeTimerHandler not found %p", pHandler);
        return;
    }
    m_setTimer.erase(pHandler);
}

void SelectorEPoll::addIdleHandler(IdleHandler *pHandler) {
    if (m_idleTasks.find(pHandler) != m_idleTasks.end()) {
        log(Warn, "SelectorEPoll::addIdleHandler %p exist!", pHandler);
        return;
    }
    m_idleTasks.insert(pHandler);
    m_poll_timeout = 0;
    log(Info, "Add Idle Handler %p", pHandler);
}

void SelectorEPoll::removeIdleHandler(IdleHandler *pHandler) {
    if (m_idleTasks.find(pHandler) == m_idleTasks.end()) {
        log(Error, "SelectorEPoll::removeIdleHandler not found %p", pHandler);
        return;
    }
    m_idleTasks.erase(pHandler);
    if(m_idleTasks.size() == 0){
        m_poll_timeout = EPOLL_TIMEOUT;
    }
    log(Info, "Remove Idle Handler %p", pHandler);
}

void SelectorEPoll::addExitHandler(ExitHandler *pHandler)
{
    if (m_exitTasks.empty()) {
        if (signal(SIGTERM, termHandler) == SIG_ERR) {
            log(Error, "addExitHandler register SIGTERM fail.\n");
        }  
        if (signal(SIGINT, termHandler) == SIG_ERR) {
            log(Error, "addExitHandler register SIGTERM fail.\n");
        }        
    }
    if (m_exitTasks.find(pHandler) != m_exitTasks.end()) {
        log(Warn, "addExitHandler %p exist!", pHandler);
        return;
    }
    m_exitTasks.insert(pHandler);   
    log(Info, "Add exit Handler %p", pHandler);    
}

void SelectorEPoll::removeExitHandler(ExitHandler *pHandler)
{
    if (m_exitTasks.find(pHandler) == m_exitTasks.end()) {
        log(Error, "removeExitHandler not found %p", pHandler);
        return;
    }
    m_exitTasks.erase(pHandler);
    if (m_exitTasks.empty()) {
        if (signal(SIGTERM, SIG_DFL) == SIG_ERR) {
            log(Error, "AddExitHandler register SIGTERM fail.\n");
        }  
        if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
            log(Error, "AddExitHandler register SIGTERM fail.\n");
        }                 
    }
    log(Info, "Remove Exit Handler %p", pHandler);
}

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

uint32_t SelectorEPoll::getIpByName(const char* hostname) {
    struct hostent *host = gethostbyname(hostname);
    if (host == NULL) {
        log(Warn, "getIpByName %s fail", hostname);
        return 0;
    }
    return *(uint32_t *) host->h_addr;
}

#include <sys/socket.h>
#include <netdb.h>

TcpSocket* SelectorEPoll::connectServerByIp(const uint32_t uiIP, uint16_t port) {
    TcpSocket* pSocket = NULL;
        
    pSocket = new TcpSocket(this);
    if (pSocket->connect(uiIP, port) == false) {
		pSocket->close();
        log(Info, "connect fail to connect server:%s:%u", ip2str(uiIP), port);
        return NULL;
    }

    log(Info, "connect: %s:%u", ip2str(uiIP), port);
    return pSocket;
}

TcpSocket* SelectorEPoll::connectServerByName(const char* hostname, uint16_t port) {
    uint32_t uiIP = getIpByName(hostname);
    if (uiIP == 0) {
        log(Error, "fail to get IP address from name: %s", hostname);
        return NULL;
    }
    
    return connectServerByIp(uiIP, port);
}

void SelectorEPoll::printTimer() {
    log(Debug, "==============================================");
    for (set<TimerHandler*>::iterator it = m_setTimer.begin(); it != m_setTimer.end(); it++) {
        log(Debug, "SelectorEPoll TimerHandler = %p", *it);
    }
    log(Debug, "==============================================");
}
//======================================================================================
//end

