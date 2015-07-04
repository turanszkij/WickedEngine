#pragma once


#ifndef WINSTORE_SUPPORT
#include <Pdh.h>
#endif

static class CpuInfo
{
#ifndef WINSTORE_SUPPORT

private:
	static bool m_canReadCpu;
	static HQUERY m_queryHandle;
	static HCOUNTER m_counterHandle;
	static unsigned long m_lastSampleTime;
	static long m_cpuUsage;
public:
	CpuInfo();
	~CpuInfo();

	static void Initialize();
	static void Shutdown();
	static void Frame();
	static int GetCpuPercentage();

#else
public:
	CpuInfo(){}
	~CpuInfo(){}

	static void Initialize(){}
	static void Shutdown(){}
	static void Frame(){}
	static int GetCpuPercentage(){}
#endif
};


