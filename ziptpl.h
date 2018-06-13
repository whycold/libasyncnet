#ifndef ZIPTP_H_
#define ZIPTP_H_

#include "zlib.h"
#include <sys/time.h>
#include <unistd.h>

template<class M, int TU> class ZipMarshal : public M {
public:

    enum {
        uri = TU
    };
    mutable uint32_t m_size;
    mutable std::string m_zip_data;

    void marshal(sox::Pack & p) const {
        sox::PackBuffer pb;
        sox::Pack tmp(pb);

        M::marshal(tmp);


        int comressItor = 6;
#ifndef WIN32
        struct timeval enter_time;
        struct timeval leave_time;
        gettimeofday(&enter_time, NULL);

        if (!access("./non-compress.txt", F_OK))
            comressItor = 0;

        if (!access("./fast-compress.txt", F_OK))
            comressItor = 1;
#endif

        m_size = (uint32_t) tmp.size();
        uLong dlength = compressBound(m_size);
        Bytef *buf = new Bytef[dlength];
        memset(buf, 0, dlength);
        int ret = compress2(buf, &dlength, (const Bytef *) tmp.data(), (uLong) tmp.size(), comressItor);

#ifndef WIN32
        gettimeofday(&leave_time, NULL);
        int diff_tick = (leave_time.tv_sec - enter_time.tv_sec)*1000 + (leave_time.tv_usec - enter_time.tv_usec) / 1000;
        log(Debug, "compress %u --> %u cmd type: %u time:%d comressItor:%d", m_size, dlength, uri, diff_tick, comressItor);
#endif

        if (ret == Z_OK) {
            m_zip_data.assign((const char *) buf, dlength);
        } else {
            log(Info, "zip error");
        }
        delete[] buf;
        p << m_size;
        p.push_varstr32(m_zip_data.data(), m_zip_data.length());
    }

    void unmarshal(const sox::Unpack &p) {
        p >> m_size;
        m_zip_data = p.pop_varstr32();
        if (m_zip_data.size() == 0) {
            throw sox::UnpackError("zip data is zero");
        } else {
            Bytef *buf = new Bytef[m_size];
            uLong dLength = m_size;
            int ret = uncompress(buf, &dLength, (const Bytef *) m_zip_data.data(), (uLong) m_zip_data.length());

            if (ret != Z_OK) {
                delete[] buf;
                throw sox::UnpackError("unzip error");
            } else {
                log(Debug, "unpack %u -> %u", m_zip_data.length(), dLength);
                sox::Unpack up2(buf, dLength);
                M::unmarshal(up2);
                delete[] buf;
            }
        }

    }

};

#endif

