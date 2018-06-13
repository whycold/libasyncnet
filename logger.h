#ifndef LOGGER_H_
#define LOGGER_H_

#include <syslog.h>
#include <sys/time.h>
#include "common.h"

extern int syslog_level;

enum{
	Fatal = LOG_EMERG,
	Error = LOG_ERR,
	Warn = LOG_WARNING,
	Notice = LOG_NOTICE,
	Info = LOG_INFO,
	Debug = LOG_DEBUG
};

void initLog();
void setLogLevel(int l);
void log(int  error, const char *fmt, ...);

#define DEBUG_LOG(...) if (syslog_level >= Debug) log(Debug, ##__VA_ARGS__);
//======================================================================================
string pointer2string(void *p);
string ip2string(uint32_t ip);
uint32_t string2ip(const char* in);

string uri2string(uint32_t uri);
string int2string(int64_t i);
string time2string(uint64_t input);
string string2log(string in);

inline char *ip2str(uint32_t ip) {
    union ip_addr {
        uint32_t addr;
        uint8_t s[4];
    } a;
    a.addr = ip;
    static char s[16];
    sprintf(s, "%u.%u.%u.%u", a.s[0], a.s[1], a.s[2], a.s[3]);
    return s;
}
inline char* int2str(uint32_t i) {
    static char s[16];
    sprintf(s, "%u", i);
    return s;
}
inline char* int642str(uint64_t i) {
    static char s[22];
    sprintf(s, "%lu", i);
    return s;
}
inline char *uri2str(uint32_t uri) {
    uint32_t uri_h = uri / 256;
    uint32_t uri_t = uri % 256;

    static char s[16];
    sprintf(s, "%3d|%3d", uri_h, uri_t);
    return s;
}

string hex2string(const char *bin, uint32_t len);

inline string hex2string(const string &input) {
    return hex2string(input.c_str(), input.size());
}

map<uint32_t, string> explode(char c, string input);
string parse_hex(string input, char separator = ' ');

uint32_t string2ip(string input);

string get_backtrace (void);
#endif 
