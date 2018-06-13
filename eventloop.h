//======================================================================================
#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_
//======================================================================================
#include <algorithm>
#include "common.h"
#include "socketbase.h"
#include "measure.h"
#include "tcpsocket.h"

class TimerHandler;
class IdleHandler;
class TcpSocket;
class ExitHandler;

enum {
    SEL_NONE = 0, SEL_ALL = -1, SEL_READ = 1, SEL_WRITE = 2, SEL_RW = 3, // SEL_ERROR = 4,

    // notify only
    SEL_TIMEOUT = 8,

    // setup only, never notify
    // SEL_R_ONESHOT = 32, SEL_W_ONESHOT = 64, SEL_RW_ONESHOT = 96,
    SEL_CONNECTING = 128
};

//======================================================================================

class SelectorEPoll {
public:
    SelectorEPoll();
    ~SelectorEPoll();

    bool init();
    void stop();
    void run();
    void timerCheck(uint64_t tNow);

    void ePollCtl(int iMethod, int iSocket, epoll_event ev);

    void setEvent(SocketBase* s, int remove, int add);
    int onReadSocket(SocketBase *pSocket);
    int onWriteSocket(SocketBase *pSocket);

    void addTimerHandler(TimerHandler *pHandler);
    void removeTimerHandler(TimerHandler *pHandler);
    void printTimer();

    void addIdleHandler(IdleHandler *pHandler);
    void removeIdleHandler(IdleHandler *pHandler);

    void addExitHandler(ExitHandler *pHandler);
    void removeExitHandler(ExitHandler *pHandler);

    uint32_t getIpByName(const char* hostname);
    TcpSocket* connectServerByName(const char* hostname, uint16_t port);
    TcpSocket* connectServerByIp(const uint32_t ip, uint16_t port);
    // check : destroy in loop

    void removeSocket(SocketBase* s);
    void socketDeleted(SocketBase* s);

    bool isRemoved(SocketBase *s) const {
        if (m_setSocket.find(s) == m_setSocket.end()) {
            return true;
        }
        return m_removed_socket_queue.find(s) != m_removed_socket_queue.end();
    }
    void clearRemovedSocketsQueue();

    bool isSocketExist(SocketBase *s) const {
        return m_setSocket.find(s) != m_setSocket.end();
    }

    void set_exit(bool is_exit) {
        m_exit = is_exit;
    }

public:
    uint64_t now_us;

protected:
    int m_poll_timeout;
    int m_hEPoll;
    bool m_bRuning;
    bool m_exit;
    std::set<SocketBase *> m_setSocket;
    std::set<TimerHandler*> m_setTimer;
    std::set<IdleHandler*> m_idleTasks;
    std::set<ExitHandler*> m_exitTasks;
    std::set<SocketBase *> m_removed_socket_queue;
};

extern SelectorEPoll& g_epoll;

class TimerHandler {
public:

    TimerHandler() {
        m_nextTime_us = 0;
        m_iInterval_us = 0;
    }

    virtual ~TimerHandler() {
    }

    void setTimer_us(uint64_t iInterval_us) {
        m_iInterval_us = iInterval_us;
        m_nextTime_us = g_measure.getPreciseTime() + m_iInterval_us;
        g_epoll.addTimerHandler(this);
    }

    virtual void timerCallback() = 0;

    void timerCheck(uint64_t tNow_us) {
        if (tNow_us >= m_nextTime_us) {
            timerCallback();
            m_nextTime_us = tNow_us + m_iInterval_us;
        }
    }

    //protected:
    uint64_t m_iInterval_us;
    uint64_t m_nextTime_us;
};

class IdleHandler {
public:

    void enableIdleTask() {
        g_epoll.addIdleHandler(this);
    };

    void disableIdleTask() {
        g_epoll.removeIdleHandler(this);
    };

    virtual void idleCallback() = 0;
};

class ExitHandler {
public:

    void enableExitTask() {
        g_epoll.addExitHandler(this);
    };

    void disableExitTask() {
        g_epoll.removeExitHandler(this);
    };

    virtual void exitCallback() = 0;
};

//======================================================================================
#endif

