#include "connmanager.h"

ConnManager * g_ptr_conn_manager = new ConnManager();

ConnManager& g_conn_manager = *g_ptr_conn_manager;

ConnManager::ConnManager() : m_next_conn_id(0) {
}

void ConnManager::newConn(TcpSocket* tsocket) {
    m_next_conn_id++;
    tsocket->m_conn_id = m_next_conn_id;
    m_conn_map.insert(std::make_pair(m_next_conn_id, tsocket));
}

void ConnManager::eraseConn(TcpSocket* tsocket) {
    m_conn_map.erase(tsocket->m_conn_id);
}

void ConnManager::eraseConn(uint32_t conn_id) {
    m_conn_map.erase(conn_id);
}
    
TcpSocket* ConnManager::getConn(uint32_t conn_id) {
    map<uint32_t, TcpSocket* >::iterator find = m_conn_map.find(conn_id);
    if (find != m_conn_map.end()) {
        return find->second;
    } else {
        return NULL;
    }
}