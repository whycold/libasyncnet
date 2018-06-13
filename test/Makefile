THRIFT_GEN_PATH=./gen-cpp

INCLUDE = -I../ -I/usr/local/include/thrift/ -I$(THRIFT_GEN_PATH)

NET_LIB = ../libasyncnet.a

all:demo_server_d demo_client_d

$(THRIFT_GEN_PATH)/ThriftDemo.h:thriftdemo.thrift
	thrift -r -strict --gen cpp:cob_style -o ./ thriftdemo.thrift

demo_server_d:demo_server.cpp $(THRIFT_GEN_PATH)/ThriftDemo.h
	$(CXX) -g -O3 -Wno-deprecated -DHAVE_INTTYPES_H $(INCLUDE) -o demo_server_d demo_server.cpp $(THRIFT_GEN_PATH)/ThriftDemo.cpp \
		 $(NET_LIB) \
	         -lcrypto \
		 /usr/local/lib/libthrift.a

demo_client_d:demo_client.cpp $(THRIFT_GEN_PATH)/ThriftDemo.h
	$(CXX) -g -O3 -Wno-deprecated -DHAVE_INTTYPES_H $(INCLUDE) -o demo_client_d demo_client.cpp $(THRIFT_GEN_PATH)/ThriftDemo.cpp \
		 $(NET_LIB) \
		 /usr/local/lib/libthrift.a	

clean:
	rm -f *.o demo_server_d demo_client_d