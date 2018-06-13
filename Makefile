SRC_COMM =  logger.cpp eventloop.cpp \
	    tcpsocket.cpp measure.cpp\
	    consoleprocessor.cpp tcpsockethandler.cpp \
	    thriftprotocolprocessor.cpp \
	    connmanager.cpp \
	    server.cpp
	

OBJ_COMM = $(SRC_COMM:.cpp=.o)


INCLUDE=-I/usr/local/include/thrift/

CXXFLAG =-g -O0 -Wall  -Wno-deprecated
LINK_CXXFLAG = -static

NET_LIB=libasyncnet.a

.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CXXFLAG) $(INCLUDE) -c -o $@ $<

all: $(NET_LIB) 

$(NET_LIB):$(OBJ_COMM)
	ar -ru $(NET_LIB) $(OBJ_COMM)
	
clean: 
	rm -f *.o *.ro $(PROGRAM) $(NET_LIB)
	
rebuild:clean all
