#ifndef _IREQUEST_H_
#define _IREQUEST_H_

#include "packet.h"
#include "common.h"
#include "pcommon.h"

typedef int SOCKET;
#define INVALID_SOCKE -1
#define SOCKET_ERROR -1


class Request  : public Header
{
public:
	sox::Unpack up;
	const char *od; 
	uint32_t os;
public:
	std::string key;	
	int connectType;
public:
	Request(const char *data, uint32_t sz):up(data, sz), od(data), os(sz){};
	virtual ~Request(){};
	
	void head(){length = up.pop_uint32();uri = up.pop_uint32();resCode = up.pop_uint16();};
	static uint32_t peeklen(const void * d){uint32_t i = *((uint32_t*)d);return XHTONL(i);};
	void setKey(const std::string &k){key = k;};
	std::string getKey(){return key;};
	void packOrgin(sox::Pack &pk) const{pk.push(od, os);};
	void leftPack(std::string &out){out.assign(up.data(), up.size());};
};

#endif 


