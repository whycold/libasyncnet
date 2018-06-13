//======================================================================================
#ifndef CONSOLEPROCESSOR_H_
#define CONSOLEPROCESSOR_H_

#include "common.h"
#include "socketbase.h"
#include "eventloop.h"
#include "itcpprocessor.h"

class ConsoleHandler {
public:
    virtual string onConsoleCallback(map<uint32_t, string> &argvs) = 0;
};
//======================================================================================

class ConsoleProcessor : public ITcpProcessor {
public:
    ConsoleProcessor();
    virtual ~ConsoleProcessor();

    void init(ConsoleHandler* handler);

    virtual int doProcess(const char* buf, int len, TcpSocket* psocket); 

    void processCommand(std::string cmd, TcpSocket* psocket);

private:
    ConsoleHandler* m_console_handler;
};

//======================================================================================
#endif



