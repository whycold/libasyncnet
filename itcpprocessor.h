#ifndef ITCPPROCESSOR_H_
#define	ITCPPROCESSOR_H_

#include <stdint.h>

class TcpSocket;

class ITcpProcessor {
public:
    virtual ~ITcpProcessor(){};

    virtual int doProcess(const char* buf, int len, TcpSocket* psocket) = 0; 

};

#endif	/* ITCPPROCESSOR_H_ */

