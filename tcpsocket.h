//======================================================================================
#ifndef TCPSOCKET_H_
#define TCPSOCKET_H_
//======================================================================================
#include <openssl/rc4.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
//======================================================================================
#include "socketbase.h"
#include "blockbuffer.h"
#include "sockbuffer.h"

#define DEF_SESSIONKEY_LENGTH 16
//======================================================================================
class SelectorEPoll;
class TcpSocketHandler;
//======================================================================================
struct RC4Alloc {
  RC4Alloc();
  ~RC4Alloc();

  unsigned char *writeBuffer;
  size_t curSize;

  unsigned char *getWriteBuffer() {
    return writeBuffer;
  }
  size_t getCurSize() {
    return curSize;
  }
  void realloc(size_t sz);
};
//======================================================================================
class RC4Filter {
  bool encrypted;
  RC4_KEY rc4;
  static RC4Alloc writeBuffer;
public:
  RC4Filter();
  ~RC4Filter();
public:
  void filterRead(char *, size_t);
  const char* filterWrite(const char *, size_t);
  bool isEncrypto() const;
  void setRC4Key(const unsigned char *, size_t);
};
//======================================================================================
typedef BlockBuffer<DefBlockAlloc_8K, 9> Buffer8x9k; // 72K WOW!
typedef BlockBuffer<DefBlockAlloc_8K, 128 * 8> Buffer8x128k; // 1M WOW!

typedef SockBuffer<Buffer8x128k, RC4Filter> InputBuffer;
typedef SockBuffer<Buffer8x128k, RC4Filter> OutputBuffer;
//======================================================================================
//======================================================================================
class TcpSocket: public SocketBase {
public:
  TcpSocket(SelectorEPoll*);
  virtual ~TcpSocket();

public:
  bool listen(uint32_t uiIP, uint16_t &iPort, bool bTryNext);
  bool connect(uint32_t uiIP, uint16_t iPort);
  void close();
  void onClose();
  

  TcpSocket *accept();

  int onReadSocket();
  int onWriteSocket();
  int sendBin(const char* p, size_t len);
  
  int pushBin(const char* data, size_t len);
  void flush();

  bool setListenSocket(bool isListen);
  void setRC4Key(const unsigned char *data, size_t len);

protected:
  void setNonBlock();

public:
  InputBuffer  m_input;
  OutputBuffer m_output;

  SelectorEPoll* m_pSelector;
  bool m_bListenSocket;
  bool m_Enable;
  bool m_bConnected;
  TcpSocketHandler *m_pHandler;
  
  uint32_t          m_conn_id;
};
//======================================================================================

//======================================================================================
#endif

