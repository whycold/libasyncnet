#ifndef SOCKBUFFER_H_
#define SOCKBUFFER_H_

#include <stdexcept>
#include "socketbase.h"
#include "blockbuffer.h"
#include "measure.h"

class FilterDefault {
protected:
    void filterRead(char *, size_t) {
    }

    const char *filterWrite(const char *data, size_t size) {
        return data;
    }
};

template<class BlockBufferClass, class FilterClass = FilterDefault>
struct SockBuffer : public BlockBufferClass, public FilterClass {
    using BlockBufferClass::eNPos;
    using BlockBufferClass::freeSpace;
    using BlockBufferClass::blockSize;
    using BlockBufferClass::block;
    using BlockBufferClass::eMaxBlocks;
    using BlockBufferClass::tail;
    using BlockBufferClass::size;
    using BlockBufferClass::increaseCapacity;
    using BlockBufferClass::append;
    using BlockBufferClass::empty;
    using BlockBufferClass::data;
    using BlockBufferClass::erase;

    using FilterClass::filterWrite;

    typedef FilterClass Filter;
    // 0 : normal close ; >0 : current pump bytes

    int pump(SocketBase& so, size_t n = eNPos) {
        if (freeSpace() < (blockSize() >> 1) && block() < eMaxBlocks)
            // ignore increase_capacity result.
            increaseCapacity(blockSize());

        size_t nrecv = freeSpace();
        if (nrecv == 0)
            return -1;
        if (n < nrecv)
            nrecv = n; // min(n, freespace());

        int ret;
        {
            //scoped_measure m(68, 0, 0);
            ret = ::recv(so.m_iSocket, (char*) tail(), nrecv, 0);
        }
        if (ret > 0) {
            FilterClass::filterRead(tail(), ret);
            size(size() + ret);
        }
        return ret;
    }

    ////////////////////////////////////////////////////////////////////
    // append into buffer only

    int write(const char * msg, size_t size) {
        if (msg == NULL || size == 0)
            return 0;

        const char *enc = filterWrite(msg, size);
        if (!append(enc, size)) {
            log(Error, "output buffer overflow[all]");
            return -1;
        }
        return 0;
    }

    int write(SocketBase& so, const char * msg, size_t size) { // write all / buffered
        if (msg == NULL || size == 0)
            return 0;

        const char *enc = filterWrite(msg, size);

        size_t nsent = 0;
        if (empty())
            nsent = ::send(so.m_iSocket, enc, size, 0); //so.send(enc, size);
        if (nsent < 0) {
            nsent = 0;
        }
        if (!append(enc + nsent, size - nsent)) {
            // all or part append error .
            if (nsent > 0)
                log(Error, "output buffer overflow");
            else
                log(Error, "output buffer overflow [all]");
            return -1;
        }
        return 0;
    }

    int flush(SocketBase& so, size_t n = eNPos) {
        size_t nflush = size();
        if (n < nflush)
            nflush = n; // std::min(n, size());
        int ret = ::send(so.m_iSocket, (const char*) data(), nflush, 0); //so.send(data(), nflush);
        if (ret < 0) {
            ret = 0;
        }
        erase(0, ret); // if call flush in loop, maybe memmove here
        return ret;
    }
};

#endif // SOCKBUFFER_H_
