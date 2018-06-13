//======================================================================================
#include <algorithm>
#include "logger.h"
//======================================================================================
int syslog_level = Debug;
//======================================================================================

void initLog() {
    openlog(NULL, LOG_PID, LOG_LOCAL0);
    log(Notice, "==================program start======================");
}
void setLogLevel(int l){
    log(Notice, "set log level to :%d", l);
    syslog_level = l;
}

void log(int l, const char *fmt, ...) {
    if(l > syslog_level) {
        return;
    }
    if(l == Error){
        l = Error;
    }
    va_list param;

    va_start(param, fmt);
    vsyslog(l, fmt, param);
    va_end(param);
}


void agent_log(int l, const char *fmt, ...) {
    if(l > syslog_level) {
        return;
    }
    char buf[4096*64];
    string out;
    if(l == Error){
        out += "err ";
    }
    
    va_list param;
    va_start(param, fmt);
    vsprintf(buf, fmt, param);
    out += buf;
    va_end(param);
    
    log(l, "agent_log:%s", out.c_str());
}
//======================================================================================

string pointer2string(void *p) {
    char buf[32];
    sprintf(buf, "%p", p);
    return buf;
}

string ip2string(uint32_t ip) {

    union ip_addr {
        uint32_t addr;
        uint8_t s[4];
    } a;
    a.addr = ip;
    char s[16];
    sprintf(s, "%u.%u.%u.%u", a.s[0], a.s[1], a.s[2], a.s[3]);
    return s;
}

#include <arpa/inet.h>
uint32_t string2ip(const char* in){
    struct in_addr addr;
    inet_aton(in, &addr);
    return addr.s_addr;
}

string int2string(int64_t i) {
    char s[32];
    sprintf(s, "%ld", i);
    return s;
}


string uri2string(uint32_t uri) {
    uint32_t uri_h = uri / 256;
    uint32_t uri_t = uri % 256;

    char s[16];
    sprintf(s, "%3d|%3d", uri_h, uri_t);
    return s;
}

#include <sstream>

string hex2string(const char *bin, uint32_t len) {
    std::ostringstream os;
    for (uint32_t i = 0; i < len; i++) {
        char st[4];
        uint8_t c = bin[i];
        if (i == (len-1)) {
            sprintf(st, "%02x", c);
        } else {
            sprintf(st, "%02x ", c);
        }
        os << st;
    }
    return os.str();
}

string time2string(uint64_t input){
    char buf[64];
    time_t input_time = (time_t)(input/1000000);
    uint64_t ms = (input % 1000000)/1000;
    uint64_t us = input % 1000;
    struct tm  tio;
    localtime_r(&input_time, &tio);
    sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u.%03lu.%03lu", tio.tm_year + 1900, tio.tm_mon + 1, tio.tm_mday, tio.tm_hour, tio.tm_min, tio.tm_sec, ms, us);
    return buf;
}

#include <string.h>
map<uint32_t, string> explode(char c, string input){
    map<uint32_t, string> output;
    if (input.empty() == false) {
        uint32_t i = 0;
        char *last_pos = (char*)input.c_str();

        while(1){
            char *p_n = strchr(last_pos, c);
            if(p_n){
                int n = p_n - last_pos;
                output[i++].assign(last_pos, n);
                last_pos = p_n + 1;
            }else{
                output[i++] = last_pos;
                break;
            }
        }
    }
    return output;
}

string parse_hex(string input, char separator){
    map<uint32_t, string> map_str = explode(separator, input);
    
    string out;
    for(map<uint32_t, string>::iterator it=map_str.begin(); it!=map_str.end(); ++it){
        string &str = it->second;
        if (str.empty()) continue;
        uint32_t c_int;
        sscanf(str.c_str(), "%02x", &c_int);
        char c = (char)c_int;
        
        out += c;
    }
    return out;
}

uint32_t string2ip(string input){
    return inet_addr(input.c_str());
}
string string2log(string in){
    string out = in;
    std::replace( out.begin(), out.end(), '\n', ' ');
    return out;
}
//======================================================================================
#include <cxxabi.h>
#include <execinfo.h>

string get_backtrace (void) {
    string out;
    uint32_t max_frames = 10;
    
    // storage array for stack trace address data
    void* addrlist[max_frames+1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
    if (addrlen == 0) {
        return "<empty, possibly corrupt>";
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char** symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char* funcname = (char*)malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++){
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p){
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset && begin_name < begin_offset){
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char* ret = abi::__cxa_demangle(begin_name,
                            funcname, &funcnamesize, &status);
            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string
                out += string(funcname) + "+" + string(begin_offset) + ";";
            }else {
                out += string(begin_name) + "+" + begin_offset + ";";
            }
        }else{
            // couldn't parse the line? print the whole line.
            out += string(symbollist[i]) + ";";
        }
    }

    free(funcname);
    free(symbollist);
    
    return out;
}



