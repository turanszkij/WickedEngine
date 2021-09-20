#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>

#include <FAudio.h>

const IID DEVINTERFACE_AUDIO_RENDER = { 0xe6327cad, 0xdcec, 0x4949, {0xae, 0x8a, 0x99, 0x1e, 0x97, 0x6a, 0x79, 0xd2 } };

const DWORD MAX_FRIENDLY_NAME = 2048;
static wchar_t friendlyName[MAX_FRIENDLY_NAME];

LPCWSTR win_device_id_to_friendly_name(LPCWSTR deviceId) 
{
	LPCWSTR result = NULL;
	HDEVINFO devInfoSet = SetupDiGetClassDevs(&DEVINTERFACE_AUDIO_RENDER, 0, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (devInfoSet == INVALID_HANDLE_VALUE) 
	{
		return NULL;
	}

	SP_DEVINFO_DATA devInfo;
	devInfo.cbSize = sizeof(SP_DEVINFO_DATA);

	SP_DEVICE_INTERFACE_DATA devInterface;
	devInterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	DWORD deviceIndex = 0;

	for (;;) 
	{
		if (!SetupDiEnumDeviceInterfaces(devInfoSet, 0, &DEVINTERFACE_AUDIO_RENDER, deviceIndex, &devInterface)) 
		{
			break;
		}
		deviceIndex++;

		DWORD reqSize = 0;

		SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInterface, NULL, 0, &reqSize, NULL);
		SP_DEVICE_INTERFACE_DETAIL_DATA *interfaceDetail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA *>(new uint8_t[reqSize]);

		if (!interfaceDetail) 
		{
			break;
		} 

		interfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if (!SetupDiGetDeviceInterfaceDetail(devInfoSet, &devInterface, interfaceDetail, reqSize, NULL, &devInfo)) 
		{
			delete[] interfaceDetail;
			continue;
		}
		
		if (_wcsicmp(deviceId, interfaceDetail->DevicePath) != 0) 
		{
			delete[] interfaceDetail;
			continue;
		}

		delete[] interfaceDetail;

		if (!SetupDiGetDeviceRegistryProperty(devInfoSet, &devInfo, SPDRP_FRIENDLYNAME, NULL, (LPBYTE) friendlyName, MAX_FRIENDLY_NAME * sizeof(wchar_t), NULL)) 
		{
			continue;
		}

		result = friendlyName;
		break;
	}

	if (devInfoSet) 
	{
		SetupDiDestroyDeviceInfoList(devInfoSet);
	}

	return result;
}

uint32_t device_index_from_device_id(FAudio *faudio, LPCWSTR deviceId) 
{
	LPCWSTR friendlyName = win_device_id_to_friendly_name(deviceId);

	uint32_t dev_count;
	FAudio_GetDeviceCount(faudio, &dev_count);

	for (uint32_t i = 0; i < dev_count; ++i) 
	{
		FAudioDeviceDetails dev_details;
		FAudio_GetDeviceDetails(faudio, i, &dev_details);

		if (_wcsicmp((LPCWSTR) dev_details.DisplayName, friendlyName) == 0) 
		{
			return i;
		}
	}

	return 0;
}
