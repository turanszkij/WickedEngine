#pragma once


#ifndef WINSTORE_SUPPORT
#include <Pdh.h>
#endif

class wiCpuInfo
{
#ifndef WINSTORE_SUPPORT

private:
	static bool m_canReadCpu;
	static HQUERY m_queryHandle;
	static HCOUNTER m_counterHandle;
	static unsigned long m_lastSampleTime;
	static long m_cpuUsage;
public:
	wiCpuInfo();
	~wiCpuInfo();

	static void Initialize();
	static void Shutdown();
	static void Frame();
	static int GetCpuPercentage();

#else
public:
	wiCpuInfo(){}
	~wiCpuInfo(){}

	static void Initialize(){}
	static void Shutdown(){}
	static void Frame(){}
	static int GetCpuPercentage(){ return -1; }
#endif
};


