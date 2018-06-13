#ifndef MEASHRE_H_
#define MEASHRE_H_

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include "logger.h"


typedef struct MeasureData {
    MeasureData() {
        callTimes = 0;
        costTime = 0;
        maxTime = 0;
        allCostTime = 0;
        allCallTime = 0;
        allMaxTime = 0;
    }
    int callTimes;
    uint64_t costTime;
    uint64_t maxTime;
    uint64_t allCostTime;
    uint64_t allCallTime;
    uint64_t allMaxTime;
} MeasureData_t;

class Measure
{
public:
    Measure();
    ~Measure();
    void init(bool enableMeasure, uint64_t measureDumpInterval_us = 10000000);
    void setEnable(bool enableMeasure);

    void measureDumpToLog();
    std::string measureDumpJSON(bool getAndClear = false);

    uint64_t getTimeOfDay64();
    uint64_t getPreciseTime();

    int initCpuFreq();
    
    uint64_t get_cpu_cycle();
    uint64_t get_usec_interval(uint64_t start, uint64_t stop);
    uint32_t get_msec_interval(uint64_t start, uint64_t stop);
    uint64_t get_tick_interval(uint64_t start, uint64_t stop);

    bool m_enable_measure;
    bool m_enable_from;
    bool m_enable_fromIp;    
    
    typedef std::map<URI_TYPE, MeasureData_t> uriTimeCount_t;
    typedef std::map<uint32_t, MeasureData_t> IpTimeCount_t;
    typedef std::map<uint32_t, MeasureData_t> ServerIdTimeCount_t;
    uriTimeCount_t uriTimeCount;
    IpTimeCount_t IpTimeCount;
    ServerIdTimeCount_t ServerIdTimeCount;
    
private:
    double m_cpu_freq; /*MHz*/
    double m_cpu_freq_magnification; /*MHz*/
    
    uint64_t m_begin_us;
    uint64_t m_begin_tick;
};
extern Measure& g_measure;

class ScopedMeasure {
public:
    ScopedMeasure(URI_TYPE uri, uint32_t ip, uint32_t serverId);
    ~ScopedMeasure();

    inline uint64_t count_cycles() {
        return g_measure.get_tick_interval(m_startTime, g_measure.get_cpu_cycle());
    }

    inline uint64_t count_us() {
        return g_measure.get_usec_interval(0, count_cycles());
    }
    inline uint32_t passed_ms() {
        return g_measure.get_msec_interval(m_startTime, g_measure.get_cpu_cycle());
    }

private:
    uint64_t m_startTime;
    URI_TYPE m_uri;
    uint32_t m_ip;
    uint32_t m_serverId;
    
};

#endif /*MEASHRE_H_*/
