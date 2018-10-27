#include "wiCpuInfo.h"
#include "wiBackLog.h"


#ifndef WINSTORE_SUPPORT
#include <Pdh.h>
#pragma comment(lib,"pdh.lib")
#endif

namespace wiCpuInfo
{

#ifndef WINSTORE_SUPPORT

	static bool m_canReadCpu;
	static HQUERY m_queryHandle;
	static HCOUNTER m_counterHandle;
	static unsigned long m_lastSampleTime;
	static long m_cpuUsage;

	void Initialize()
	{
		PDH_STATUS status;


		// Initialize the flag indicating whether this object can read the system cpu usage or not.
		m_canReadCpu = true;

		// Create a query object to poll cpu usage.
		status = PdhOpenQuery(NULL, 0, &m_queryHandle);
		if (status != ERROR_SUCCESS)
		{
			m_canReadCpu = false;
		}

		// Set query object to poll all cpus in the system.
		status = PdhAddCounter(m_queryHandle, TEXT("\\Processor(_Total)\\% processor time"), 0, &m_counterHandle);
		if (status != ERROR_SUCCESS)
		{
			m_canReadCpu = false;
		}

		m_lastSampleTime = GetTickCount();

		m_cpuUsage = 0;

		wiBackLog::post("wiCPUInfo Initialized");
	}

	void CleanUp()
	{
		if (m_canReadCpu)
		{
			PdhCloseQuery(m_queryHandle);
		}

		return;
	}

	void UpdateFrame()
	{
		PDH_FMT_COUNTERVALUE value;

		if (m_canReadCpu)
		{
			if ((m_lastSampleTime + 1000) < GetTickCount())
			{
				m_lastSampleTime = GetTickCount();

				PdhCollectQueryData(m_queryHandle);

				PdhGetFormattedCounterValue(m_counterHandle, PDH_FMT_LONG, NULL, &value);

				m_cpuUsage = value.longValue;
			}
		}

		return;
	}

	float GetCPUPercentage()
	{
		float usage;

		if (m_canReadCpu)
		{
			usage = (float)m_cpuUsage;
		}
		else
		{
			usage = -1;
		}

		return usage;
	}

#else

	void Initialize() {}
	void CleanUp() {}
	void UpdateFrame() {}
	float GetCPUPercentage() { return -1; }

#endif // WINSTORE_SUPPORT
}
