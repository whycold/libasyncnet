#ifndef CONNMANAGER_H_
#define	CONNMANAGER_H_

#include <map>
#include "tcpsocket.h"

using ::std::map;

class ConnManager {
public:
    ConnManager();

    void newConn(TcpSocket* tsocket);
    void eraseConn(TcpSocket* tsocket);
    void eraseConn(uint32_t conn_id);
    
    TcpSocket* getConn(uint32_t conn_id);
    
private:    
    map<uint32_t, TcpSocket* > m_conn_map;
    uint32_t                   m_next_conn_id;

};

extern ConnManager& g_conn_manager;

#endif	/* CONNMANAGER_H_ */

