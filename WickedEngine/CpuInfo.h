#pragma once
#include <Pdh.h>

static class CpuInfo
{
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
};

