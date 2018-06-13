
#ifndef BLOCKBUFFER_H_
#define BLOCKBUFFER_H_

#include <new>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "measure.h"

// boost::pool min accord

template <unsigned BlockSize>
struct DefaultBlockAllocatorMallocFree {

    enum {
        eRequestedSize = BlockSize
    };

    static char * orderedMalloc(size_t n) {
        return (char *) malloc(eRequestedSize * n);
    }

    static void orderedFree(char * const block, size_t) {
        free(block);
    }
};

template <unsigned BlockSize>
struct DefaultBlockAllocatorNewDelete {

    enum {
        eRequestedSize = BlockSize
    };

    static char * orderedMalloc(size_t n) {
        return new (std::nothrow) char[eRequestedSize * n];
    }

    static void orderedFree(char * const block, size_t) {
        delete [] block;
    }
};

#ifdef  USE_ALLOCATOR_NEW_DELETE

typedef DefaultBlockAllocatorNewDelete < 1 * 1024 > DefBlockAlloc_1K;
typedef DefaultBlockAllocatorNewDelete < 2 * 1024 > DefBlockAlloc_2K;
typedef DefaultBlockAllocatorNewDelete < 4 * 1024 > DefBlockAlloc_4K;
typedef DefaultBlockAllocatorNewDelete < 8 * 1024 > DefBlockAlloc_8K;
typedef DefaultBlockAllocatorNewDelete < 16 * 1024 > DefBlockAlloc_16K;
typedef DefaultBlockAllocatorNewDelete < 32 * 1024 > DefBlockAlloc_32K;

#else

typedef DefaultBlockAllocatorMallocFree < 1 * 1024 > DefBlockAlloc_1K;
typedef DefaultBlockAllocatorMallocFree < 2 * 1024 > DefBlockAlloc_2K;
typedef DefaultBlockAllocatorMallocFree < 4 * 1024 > DefBlockAlloc_4K;
typedef DefaultBlockAllocatorMallocFree < 8 * 1024 > DefBlockAlloc_8K;
typedef DefaultBlockAllocatorMallocFree < 16 * 1024 > DefBlockAlloc_16K;
typedef DefaultBlockAllocatorMallocFree < 32 * 1024 > DefBlockAlloc_32K;

#endif

template <typename BlockAllocator = DefBlockAlloc_4K, unsigned MaxBlocks = 2 >
        class BlockBuffer {
public:
    typedef BlockAllocator allocator;

    enum {
        eMaxBlocks = MaxBlocks
    };

    enum {
        eNPos = size_t(-1)
    };

    BlockBuffer() {
        m_block = 0;
        m_size = 0;
        m_data = NULL;
    }

    virtual ~BlockBuffer() {
        free();
    }

    inline bool empty() const {
        return size() == 0;
    }

    inline size_t block() const {
        return m_block;
    }

    inline size_t blockSize() const {
        return allocator::eRequestedSize;
    }

    inline size_t capacity() const {
        return m_block * allocator::eRequestedSize;
    }

    inline size_t maxSize() const {
        return eMaxBlocks * allocator::eRequestedSize;
    }

    inline size_t maxFree() const {
        return maxSize() - size();
    }

    inline size_t freeSpace() const {
        return capacity() - size();
    }

    inline char * data() {
        return m_data;
    }

    inline size_t size() const {
        return m_size;
    }

    bool resize(size_t n, char c = 0);
    bool reserve(size_t n);
    bool append(const char * app, size_t len);
    bool replace(size_t pos, const char * rep, size_t n);
    void erase(size_t pos = 0, size_t n = eNPos, bool hold = false);

    static size_t current_total_blocks() {
        return s_current_total_blocks;
    }

    static size_t peak_total_blocks() {
        return s_peak_total_blocks;
    }

protected:
    bool increaseCapacity(size_t increase_size);

    char * tail() {
        return m_data + m_size;
    }

    void size(size_t size) {
        assert(size <= capacity());
        m_size = size;
    }

private:
    void free();
    static size_t s_current_total_blocks;
    static size_t s_peak_total_blocks;

    char * m_data;
    size_t m_size;
    size_t m_block;

    BlockBuffer(const BlockBuffer&);
    void operator =(const BlockBuffer &);
};

template <typename BlockAllocator, unsigned MaxBlocks>
        size_t BlockBuffer<BlockAllocator, MaxBlocks >::s_current_total_blocks = 0;

template <typename BlockAllocator, unsigned MaxBlocks>
        size_t BlockBuffer<BlockAllocator, MaxBlocks >::s_peak_total_blocks = 0;

template <typename BlockAllocator, unsigned MaxBlocks>
inline void BlockBuffer<BlockAllocator, MaxBlocks >::free() {
    if (m_block > 0) {
        allocator::orderedFree(m_data, m_block);
        s_current_total_blocks -= m_block;
        m_data = NULL;
        m_block = 0;
    }
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::append(const char * app, size_t len) {
    if (len == 0){
        return true; // no data
    }
    if (increaseCapacity(len)) {
        memmove(tail(), app, len); // append
        m_size += len;
        return true;
    }
    return false;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::reserve(size_t n) {
    return (n <= capacity() || increaseCapacity(n - capacity()));
}

template <typename BlockAllocator, unsigned MaxBlocks>
bool BlockBuffer<BlockAllocator, MaxBlocks >::resize(size_t n, char c) {
    if (n > size()) // increase
    {
        size_t len = n - size();
        if (!increaseCapacity(len)) {
            return false;
        }
        memset(tail(), c, len);
    }
    m_size = n;
    return true;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::replace(size_t pos, const char * rep, size_t n) {
    if (pos >= size()){ // out_of_range ?
        return append(rep, n);
    }
    if (pos + n >= size()) // replace all beginning with position pos
    {
        m_size = pos;
        return append(rep, n);
    }
    if (n > 0){
        memmove(m_data + pos, rep, n);
    }
    return true;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline void BlockBuffer<BlockAllocator, MaxBlocks >::erase(size_t pos, size_t n, bool hold) {
    assert(pos <= size()); // out_of_range debug.

    size_t m = size() - pos; // can erase
    if (n >= m) {
        m_size = pos; // all clear after pos
    } else {
        m_size -= n;
        memmove(m_data + pos, m_data + pos + n, m - n);
    }
}

/*
 * after success increase_capacity : freespace() >= increase_size
 * if false : does not affect exist data
 */
template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::increaseCapacity(size_t increase_size) {
    if (increase_size == 0){
        return true;
    }
    size_t newblock = m_block;

    size_t free = freeSpace();
    if (free >= increase_size){
        return true;
    }
    increase_size -= free;
    newblock += increase_size / allocator::eRequestedSize;
    if ((increase_size % allocator::eRequestedSize) > 0){
        newblock++;
    }
    if (newblock > eMaxBlocks){
        return false;
    }
    char * newdata = (char*) (allocator::orderedMalloc(newblock));
    if (0 == newdata){
        return false;
    }
    if (m_block > 0) {
        // copy old data and free old block
        memcpy(newdata, m_data, m_size);
        allocator::orderedFree(m_data, m_block);
    }

    s_current_total_blocks += newblock - m_block;
    if (s_current_total_blocks > s_peak_total_blocks) {
        s_peak_total_blocks = s_current_total_blocks;
    }

    m_data = newdata;
    m_block = newblock;
    return true;
}

#endif // BLOCKBUFFER_H_
