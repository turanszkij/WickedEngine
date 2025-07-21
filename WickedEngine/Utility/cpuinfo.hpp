#pragma once
#include <algorithm>
#include <stdexcept>
#include <stdint.h>
#include <string>

#ifdef _WIN32
#include <intrin.h>
#endif

class CPUInfo {
	class CPUID {
		uint32_t regs[4] = { 0 };

	public:
		inline explicit CPUID(uint32_t funcId, uint32_t subFuncId) {
#ifdef _WIN32
			::__cpuidex((int*)regs, (int)funcId, (int)subFuncId);
#else
			asm volatile
				("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
					: "a" (funcId), "c" (subFuncId));
			// ECX is set to zero for CPUID function 4
#endif
		}

		inline const uint32_t& EBX() const { return regs[1]; }
		inline const uint32_t& EAX() const { return regs[0]; }
		inline const uint32_t& ECX() const { return regs[2]; }
		inline const uint32_t& EDX() const { return regs[3]; }
	};

public:
	inline CPUInfo();
	inline std::string vendor()        const { return mVendorId; }
	inline std::string model()         const { return mModelName; }
	inline int     cores()             const { return mNumCores; }
	// WARNING! CPUID reports hardware CAPABILITIES. For Intel CPUs you will still get HT=on and logicalCpus() > cores() even if HT is OFF in the BIOS.
	// Query the OS for actual correct runtime info.
	inline int     logicalCpus()       const { return mNumLogCpus; }
	inline bool    isHyperThreaded()   const { return mIsHTT; }
	inline bool    haveSSE()           const { return mIsSSE; }
	inline bool    haveSSE2()          const { return mIsSSE2; }
	inline bool    haveSSE3()          const { return mIsSSE3; }
	inline bool    haveSSE41()         const { return mIsSSE41; }
	inline bool    haveSSE42()         const { return mIsSSE42; }
	inline bool    haveF16C()          const { return mIsF16C; }
	inline bool    haveFMA3()          const { return mIsFMA3; }
	inline bool    haveAVX()           const { return mIsAVX; }
	inline bool    haveAVX2()          const { return mIsAVX2; }
	inline bool    haveAVX512F()       const { return mIsAVX512F; }

private:
	// Bit positions for data extractions
	static constexpr uint32_t SSE_POS = 0x02000000;
	static constexpr uint32_t SSE2_POS = 0x04000000;
	static constexpr uint32_t SSE3_POS = 0x00000001;
	static constexpr uint32_t SSE41_POS = 0x00080000;
	static constexpr uint32_t SSE42_POS = 0x00100000;
	static constexpr uint32_t AVX_POS = 0x10000000;
	static constexpr uint32_t AVX2_POS = 0x00000020;
	static constexpr uint32_t FMA3_POS = 1u << 12;
	static constexpr uint32_t AVX512F_POS = 1u << 15; // Bit 16
	static constexpr uint32_t F16C_POS = 1u << 29;
	static constexpr uint32_t LVL_NUM = 0x000000FF;
	static constexpr uint32_t LVL_TYPE = 0x0000FF00;
	static constexpr uint32_t LVL_CORES = 0x0000FFFF;

	// Attributes
	std::string mVendorId;
	std::string mModelName;
	int    mNumSMT = 0;
	int    mNumCores = 0;
	int    mNumLogCpus = 0;
	bool   mIsHTT = 0;
	bool   mIsSSE = false;
	bool   mIsSSE2 = false;
	bool   mIsSSE3 = false;
	bool   mIsSSE41 = false;
	bool   mIsSSE42 = false;
	bool   mIsAVX = false;
	bool   mIsAVX2 = false;
	bool   mIsAVX512F = false;
	bool   mIsF16C = false;
	bool   mIsFMA3 = false;
};

CPUInfo::CPUInfo()
{
	// Get vendor name EAX=0
	CPUID cpuID0(0, 0);
	const uint32_t HFS = cpuID0.EAX();
	// Reinterpret bytes as ASCII characters
	mVendorId += std::string((const char*)&cpuID0.EBX(), 4);
	mVendorId += std::string((const char*)&cpuID0.EDX(), 4);
	mVendorId += std::string((const char*)&cpuID0.ECX(), 4);
	// Get SSE instructions availability
	CPUID cpuID1(1, 0);
	mIsHTT = cpuID1.EDX() & AVX_POS;
	mIsSSE = cpuID1.EDX() & SSE_POS;
	mIsSSE2 = cpuID1.EDX() & SSE2_POS;
	mIsSSE3 = cpuID1.ECX() & SSE3_POS;
	mIsSSE41 = cpuID1.ECX() & SSE41_POS;
	mIsSSE42 = cpuID1.ECX() & SSE41_POS;
	mIsAVX = cpuID1.ECX() & AVX_POS;
	mIsF16C = cpuID1.ECX() & F16C_POS;
	mIsFMA3 = cpuID1.ECX() & FMA3_POS;
	// Get AVX2 instructions availability
	CPUID cpuID7(7, 0);
	mIsAVX2 = cpuID7.EBX() & AVX2_POS;
	mIsAVX512F = cpuID7.EBX() & AVX512F_POS;

	std::string vendorIdUppercase = mVendorId;
	std::for_each(vendorIdUppercase.begin(), vendorIdUppercase.end(), [](char& character) { character = static_cast<char>(::toupper(character)); });
	// Get num of cores
	if (vendorIdUppercase.find("INTEL") != std::string::npos)
	{
		if (HFS >= 11)
		{
			static constexpr int MAX_INTEL_TOP_LVL = 4;
			for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl)
			{
				CPUID cpuID4(0x0B, lvl);
				uint32_t currLevel = (LVL_TYPE & cpuID4.ECX()) >> 8;
				switch (currLevel)
				{
				case 0x01: mNumSMT = LVL_CORES & cpuID4.EBX(); break; //  EAX=0xB, ECX=0 - EBX is the number of logical processors (threads) per core
				case 0x02: mNumLogCpus = LVL_CORES & cpuID4.EBX(); break; // EAX=0xB, ECX=1 - EBX is the number of logical processors per processor package
				default: break;
				}
			}
			mNumCores = mNumLogCpus / mNumSMT;
			mIsHTT = mNumSMT > 1;
		}
		else
		{
			if (HFS >= 1)
			{
				mNumLogCpus = (cpuID1.EBX() >> 16) & 0xFF;
				if (HFS >= 4)
				{
					mNumCores = 1 + (CPUID(4, 0).EAX() >> 26) & 0x3F;
				}
			}
			if (mIsHTT)
			{
				if (!(mNumCores > 1))
				{
					mNumCores = 1;
					mNumLogCpus = (mNumLogCpus >= 2 ? mNumLogCpus : 2);
				}
			}
			else
			{
				mNumCores = mNumLogCpus = 1;
			}
		}
	}
	else if (vendorIdUppercase.find("AMD") != std::string::npos)
	{
		// From https://github.com/time-killer-games/ween/blob/db69cafca2222c634a1d3a9e58262b5a2dc8d508/system.cpp#L1469-L1528
		mNumSMT = 1 + ((CPUID(0x8000001e, 0).EBX() >> 8) & 0xff);
		if (mNumLogCpus > 0 && mNumSMT > 0) {
			mNumCores = mNumLogCpus / mNumSMT;
		}
		else {
			if (HFS >= 1) {
				if (CPUID(0x80000000, 0).EAX() >= 8) {
					mNumCores = 1 + (CPUID(0x80000008, 0).ECX() & 0xFF);
				}
			}
			if (mIsHTT) {
				if (mNumCores < 1) {
					mNumCores = 1;
				}
			}
			else {
				mNumCores = 1;
			}
		}
	}
	else
	{
		throw std::runtime_error{"Unknown vendor! Reported vendor name is: " + mVendorId};
	}

	// Get processor brand string
	// This seems to be working for both Intel & AMD vendors
	for (int i = 0x80000002; i < 0x80000005; ++i)
	{
		CPUID cpuID(i, 0);
		mModelName += std::string((const char*)&cpuID.EAX(), 4);
		mModelName += std::string((const char*)&cpuID.EBX(), 4);
		mModelName += std::string((const char*)&cpuID.ECX(), 4);
		mModelName += std::string((const char*)&cpuID.EDX(), 4);
	}
}
