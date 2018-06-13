#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "measure.h"

Measure* g_ptr_measure = new Measure();
Measure& g_measure = *g_ptr_measure;

ScopedMeasure::ScopedMeasure(URI_TYPE uri, uint32_t ip, uint32_t serverId) {
    if (g_measure.m_enable_measure == false) {
        return;
    }
    m_startTime = g_measure.get_cpu_cycle();
    m_uri = uri;
    m_ip = ip;
    m_serverId = serverId;
}

ScopedMeasure::~ScopedMeasure() {
    if (g_measure.m_enable_measure == false) {
        return;
    }

    uint64_t costTime = count_cycles();

    {//URI
        MeasureData_t &md = g_measure.uriTimeCount[m_uri];
        md.costTime += costTime;
        md.allCostTime += costTime;
        md.callTimes += 1;
        md.allCallTime += 1;
        if (costTime > md.maxTime) {
            md.maxTime = costTime;
        }
        if (costTime > md.allMaxTime) {
            md.allMaxTime = costTime;
        }
    }
    if (g_measure.m_enable_from && m_serverId != 0) {//ServerId
        MeasureData_t &md = g_measure.ServerIdTimeCount[m_serverId];
        md.costTime += costTime;
        md.allCostTime += costTime;
        md.callTimes += 1;
        md.allCallTime += 1;
        if (costTime > md.maxTime) {
            md.maxTime = costTime;
        }
        if (costTime > md.allMaxTime) {
            md.allMaxTime = costTime;
        }
    } else if (g_measure.m_enable_fromIp && m_ip != 0) {//IP
        MeasureData_t &md = g_measure.IpTimeCount[m_ip];
        md.costTime += costTime;
        md.allCostTime += costTime;
        md.callTimes += 1;
        md.allCallTime += 1;
        if (costTime > md.maxTime) {
            md.maxTime = costTime;
        }
        if (costTime > md.allMaxTime) {
            md.allMaxTime = costTime;
        }
    }
}
//======================================================================================




Measure::Measure() {
    m_enable_measure = false;
    m_enable_from = false;
    m_enable_fromIp = false;
    m_cpu_freq = 1; /*MHz*/
    m_cpu_freq_magnification = 1; /*MHz*/
    
    m_begin_us = getTimeOfDay64();
    m_begin_tick = get_cpu_cycle();
}

Measure::~Measure() {
}

void Measure::setEnable(bool enableMeasure){
    m_enable_measure = enableMeasure;
}

void Measure::init(bool enableMeasure, uint64_t measureDumpInterval_us) {    
    setEnable(enableMeasure);
    if(enableMeasure == false){
        return;
    }
    //setTimer_us(measureDumpInterval_us);
    initCpuFreq();
}

void Measure::measureDumpToLog() {
    for (uriTimeCount_t::iterator iter = uriTimeCount.begin(); iter != uriTimeCount.end(); iter++) {
        MeasureData_t &md = iter->second;
        uint64_t costTime = md.costTime;
        if (costTime > 0) {
            uint32_t uri_h = iter->first >> 8;
            uint32_t uri_t = iter->first & 0x000000ff;
            log(Notice, "measure: %4u|%3u = %8lu, %7u, %7lu, %7lu", uri_h, uri_t,
                    get_usec_interval(0, costTime),
                    md.callTimes,
                    get_usec_interval(0, costTime / md.callTimes),
                    get_usec_interval(0, md.maxTime));
        }

        md.callTimes = 0;
        md.costTime = 0;
        md.maxTime = 0;
    }
}

std::string Measure::measureDumpJSON(bool getAndClear) {
    std::ostringstream os;
    m_enable_measure = true;

    os << "	\"stat\":" << std::endl;
    os << "	{" << std::endl;

    {//URI
        bool firstOutputElement = true;

        os << "		\"uri\":" << std::endl << "		[" << std::endl;
        uriTimeCount_t::iterator iter;
        for (iter = uriTimeCount.begin(); iter != uriTimeCount.end(); ++iter) {
            if (iter->second.allCallTime == 0) continue;
            if (firstOutputElement == false) {
                os << "," << std::endl;
            } else {
                firstOutputElement = false;
                os << std::endl;
            }
            os << "			{" << std::endl;
            os << "				\"uri\":\"" << uri2str(iter->first) << "\"," << std::endl;
            os << "				\"counter64\":" << int642str(iter->second.allCallTime) << "," << std::endl;
            os << "				\"time_cost\":" << int642str(get_usec_interval(0, iter->second.allCostTime)) << "," << std::endl;
            os << "				\"max_time\":" << int642str(get_usec_interval(0, iter->second.allMaxTime)) << std::endl;
            os << "			}";
            if (getAndClear) {
                iter->second.allCostTime = 0;
                iter->second.allCallTime = 0;
                iter->second.allMaxTime = 0;
            }
        }
        os << std::endl << "		]," << std::endl;
    }

    if (m_enable_fromIp) {//IP
        bool firstOutputElement = true;

        os << "		\"ip_src\":" << std::endl << "		[" << std::endl;
        IpTimeCount_t::iterator iter;
        for (iter = IpTimeCount.begin(); iter != IpTimeCount.end();) {
            if (firstOutputElement == false) {
                os << "," << std::endl;
            } else {
                firstOutputElement = false;
                os << std::endl;
            }
            os << "			{" << std::endl;
            os << "				\"ip\":\"" << ip2str(iter->first) << "\"," << std::endl;
            os << "				\"counter64\":" << int642str(iter->second.allCallTime) << "," << std::endl;
            os << "				\"time_cost\":" << int642str(get_usec_interval(0, iter->second.allCostTime)) << "," << std::endl;
            os << "				\"max_time\":" << int642str(get_usec_interval(0, iter->second.allMaxTime)) << std::endl;
            os << "			}";
            if (getAndClear) {
                IpTimeCount.erase(iter++);
            } else {
                ++iter;
            }
        }
        os << std::endl << "		]," << std::endl;
    }
    if (m_enable_from) {//ServerId
        bool firstOutputElement = true;

        os << "		\"server_id_src\":" << std::endl << "		[" << std::endl;
        ServerIdTimeCount_t::iterator iter;
        for (iter = ServerIdTimeCount.begin(); iter != ServerIdTimeCount.end();) {
            if (firstOutputElement == false) {
                os << "," << std::endl;
            } else {
                firstOutputElement = false;
                os << std::endl;
            }
            os << "			{" << std::endl;
            os << "				\"server_id\":" << int2str(iter->first) << "," << std::endl;
            os << "				\"counter64\":" << int642str(iter->second.allCallTime) << "," << std::endl;
            os << "				\"time_cost\":" << int642str(get_usec_interval(0, iter->second.allCostTime)) << "," << std::endl;
            os << "				\"max_time\":" << int642str(get_usec_interval(0, iter->second.allMaxTime)) << std::endl;
            os << "			}";
            if (getAndClear) {
                ServerIdTimeCount.erase(iter++);
            } else {
                ++iter;
            }
        }
        os << std::endl << "		]" << std::endl;
    }
    os << "	}";
    return std::string(os.str());
}


uint64_t Measure::get_cpu_cycle() {

    union cpu_cycle {
        struct t_i32 {
            uint32_t l;
            uint32_t h;
        } i32;
        uint64_t t;
    } c;

    
#define rdtsc(low,high) __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

    rdtsc(c.i32.l, c.i32.h);

    return c.t;
}

uint64_t Measure::get_usec_interval(uint64_t start, uint64_t stop) {
    if (stop < start)
        return 0;
    return (uint64_t) ((stop - start) / m_cpu_freq);
}

uint32_t Measure::get_msec_interval(uint64_t start, uint64_t stop) {
    if (stop < start)
        return 0;
    return (uint32_t) ((stop - start) / m_cpu_freq_magnification);
}

uint64_t Measure::get_tick_interval(uint64_t start, uint64_t stop) {
    if (stop < start)
        return 0;
    return (stop - start);
}

int Measure::initCpuFreq() {
    FILE * fp;
    char * str;
    //char *p;
    const char * cmd;
    int ratio = 1;

    str = (char*) malloc(1024);

    fp = popen("cat /proc/cpuinfo | grep -m 1 \"model name\"", "r");
    //p = 
    fgets(str, 1024, fp);
    fclose(fp);

    if (strstr(str, "AMD") || strstr(str, "QEMU")) {
        cmd = "cat /proc/cpuinfo | grep -m 1 \"cpu MHz\" | sed -e \'s/.*:[^0-9]//\'";
    } else {
        cmd = "cat /proc/cpuinfo | grep -m 1 \"model name\" | sed -e \"s/^.* //g\" -e \"s/GHz//g\"";
        ratio = 1000;
    }

    fp = popen(cmd, "r");
    if (fp == NULL) {
        log(Error, "Measure::get cpu info failed\n");
        return -1;
    } else {
        //p = 
        fgets(str, 1024, fp);
        m_cpu_freq = atof(str) * ratio;
        m_cpu_freq_magnification = m_cpu_freq * 1000;
        fclose(fp);
        log(Info, "Measure::get cpu info cpu_freq %f cpu_freq_magnification %f ", m_cpu_freq, m_cpu_freq_magnification);
    }

    free(str);
    uint64_t t_start = get_cpu_cycle();
    uint64_t t_stop = get_cpu_cycle();
    log(Debug, "Measure:: one diff is %lu", t_stop - t_start);
    
    t_start = get_cpu_cycle();
    uint64_t t = get_usec_interval(t_start, t_stop);
    t_stop = get_cpu_cycle();
    log(Debug, "Measure:: get_usec_interval is %lu, %lu",  t_stop - t_start, t);
    
    t_start = get_cpu_cycle();
    t = getTimeOfDay64();
    t_stop = get_cpu_cycle();
    log(Debug, "Measure:: gettimeofday64 is %lu, %lu",  t_stop - t_start, t);    

    return 0;
}

//TODO: improve time precise to 0.001 us.

//TODO: work around the ntp adjust time make time go back.
// keep time grow up.

uint64_t Measure::getTimeOfDay64() {
    struct timeval stv;
    struct timezone stz;
    gettimeofday(&stv, &stz);
    uint64_t ret = ((uint64_t) stv.tv_sec) * 1000 * 1000 + stv.tv_usec;
        
    return ret;
}

uint64_t Measure::getPreciseTime() {
    uint64_t ret;
    uint64_t tick_now = get_cpu_cycle();
    if(get_msec_interval(m_begin_tick, tick_now) > 1000){
        m_begin_us = getTimeOfDay64();
        m_begin_tick = get_cpu_cycle();
        ret = m_begin_us;
    }else{
        ret = m_begin_us + get_usec_interval(m_begin_tick, tick_now);
    }
    
    return ret;
} 

