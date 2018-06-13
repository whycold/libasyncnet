//======================================================================================
#include "consoleprocessor.h"
#include <assert.h>

#include "logger.h"
#include "tcpsocket.h"
#include "measure.h"
#include "connmanager.h"

#include <unistd.h>
#include <string>

//======================================================================================


struct StringUtil {

    enum {
        eMaxLineSize = 1024
    };

    static int getLine(std::string& stream, std::string& line, uint32_t npos = 0) {
        int nsize = 0;
        int i = 0;
        for (; (i < eMaxLineSize) && (npos + i < stream.size()); ++i) {
            if (stream[i + npos] == '\n') {
                nsize = i + 1;
                break;
            }
            if (stream[i + npos] == '\r') {
                if (((i + npos + 1) < stream.size()) && stream[i + npos + 1] == '\n') {
                    nsize = i + 2;
                    break;
                } else {
                    nsize = i + 1;
                    break;
                }
            }
        }
        line = stream.substr(npos, i);
        return nsize;
    }
};
//======================================================================================

ConsoleProcessor::ConsoleProcessor() : m_console_handler(NULL){    
}

ConsoleProcessor::~ConsoleProcessor() {
}

//======================================================================================

void ConsoleProcessor::init( ConsoleHandler* handler) {
    m_console_handler = handler;
}

int ConsoleProcessor::doProcess(const char* buf, int len, TcpSocket* psocket) {
    assert(psocket);
    if (len > 10000) {
        log(Error, "ConsoleProcessor::doProcess len = %d, from:%s", len, ip2str(psocket->m_ip));
        return -1;
    }
    std::string inputData(buf, len);

    std::string line;
    StringUtil::getLine(inputData, line);
    if (line.size()) {
        if (line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        processCommand(line,  psocket);
        return len;
    }
    return len;
}

//======================================================================================

void ConsoleProcessor::processCommand(std::string cmd, TcpSocket* psocket) {
    assert(psocket);
    log(Info, "ConsoleProcessor::processCommand: %s from %s", cmd.data(), ip2str(psocket->m_ip));

    map<uint32_t, string> argvs = explode(' ', cmd);
    for(map<uint32_t, string>::iterator it=argvs.begin(); it!=argvs.end(); ++it){
        log(Debug, "argvs[%d] = [%d]%s", it->first, it->second.size(), it->second.c_str());
    }
    
    if(argvs[0] == "exit" || argvs[0] == "quit"){
        psocket->close();
        return;
    }

    string response = m_console_handler->onConsoleCallback(argvs);

    log(Info, "response size:%lu", response.size());

    psocket->sendBin((const char*) response.data(), response.size());
}


