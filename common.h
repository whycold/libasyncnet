#ifndef COMMON_H_
#define COMMON_H_
#include <iostream>
#include <string>
#include <stdarg.h>
#include <ctime>
#include <stdexcept>
#include <map>
#include <set>
#include <queue>
#include <sstream>

#include <time.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/types.h>                  /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <signal.h>



#include <iostream>
#include <string>
#include <stdarg.h>
#include <ctime>
#include <stdexcept>
#include <map>
#include <set>
#include <queue>
#include <vector>

using namespace std;

#include <time.h>
#include <syslog.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <signal.h>


typedef unsigned int URI_TYPE;

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

#ifdef __x86_64__
typedef signed long int64_t;
typedef unsigned long uint64_t;
#else
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#endif

# if __WORDSIZE == 64
#  define __INT64_C(c)	c ## L
#  define __UINT64_C(c)	c ## UL
# else
#  define __INT64_C(c)	c ## LL
#  define __UINT64_C(c)	c ## ULL
# endif

/* Maximum of signed integral types.  */
# define INT8_MAX		(127)
# define INT16_MAX		(32767)
# define INT32_MAX		(2147483647)
# define INT64_MAX		(__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
# define UINT8_MAX		(255)
# define UINT16_MAX		(65535)
# define UINT32_MAX		(4294967295U)
# define UINT64_MAX		(__UINT64_C(18446744073709551615))

union Address {
	int64_t ipport;
	struct {
		uint32_t ip;
		int port;
	} addrStruct;
};

typedef std::set<uint32_t> Set32_t;
typedef std::vector<uint32_t> Vector32_t;


#define FORCE_INLINE __attribute__((always_inline))
#endif
