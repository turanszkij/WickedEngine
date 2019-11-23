//#include "wiRawInput.h"
//
//#ifndef WINSTORE_SUPPORT
//
//wiRawInput::wiRawInput(HWND hWnd)
//{
//	RegisterJoys(hWnd);
//	//RegisterKeyboardMouse(hWnd);
//
//	//RAWINPUTDEVICE Rid[3];
//
//	//Rid[0].usUsagePage = 0x01;
//	//Rid[0].usUsage = 0x02;
//	//Rid[0].dwFlags = 0;
//	////Rid[0].dwFlags = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
//	//Rid[0].hwndTarget = hWnd;
//
//	//Rid[1].usUsagePage = 0x01;
//	//Rid[1].usUsage = 0x06;
//	//Rid[1].dwFlags = 0;
//	////Rid[1].dwFlags = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
//	//Rid[1].hwndTarget = hWnd;
//
//	//Rid[2].usUsagePage = 0x01;
//	//Rid[2].usUsage = 0x04;
//	//Rid[2].dwFlags = 0;                 // adds joystick
//	//Rid[2].hwndTarget = hWnd;
//
//	//if (RegisterRawInputDevices(Rid, 3, sizeof(Rid[0])) == FALSE) {
//	//	DWORD error = GetLastError();
//	//}
//}
//
//wiRawInput::~wiRawInput()
//{
//}
//
//bool wiRawInput::RegisterJoys(HWND hWnd)
//{
//	RAWINPUTDEVICE Rid[2];
//
//	Rid[0].usUsagePage = 0x01;
//	Rid[0].usUsage = 0x05;
//	Rid[0].dwFlags = 0;                 // adds game pad
//	Rid[0].hwndTarget = hWnd;
//
//	Rid[1].usUsagePage = 0x01;
//	Rid[1].usUsage = 0x04;
//	Rid[1].dwFlags = 0;                 // adds joystick
//	Rid[1].hwndTarget = hWnd;
//
//	if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE) {
//		DWORD error = GetLastError();
//		return false;
//	}
//
//	return true;
//}
//
//bool wiRawInput::RegisterKeyboardMouse(HWND hWnd)
//{
//	RAWINPUTDEVICE Rid[2];
//
//	Rid[0].usUsagePage = 0x01;
//	Rid[0].usUsage = 0x02;
//	Rid[0].dwFlags = 0;
//	//Rid[0].dwFlags = RIDEV_NOLEGACY;   // adds HID mouse and also ignores legacy mouse messages
//	Rid[0].hwndTarget = hWnd;
//
//	Rid[1].usUsagePage = 0x01;
//	Rid[1].usUsage = 0x06;
//	Rid[1].dwFlags = 0;
//	//Rid[1].dwFlags = RIDEV_NOLEGACY;   // adds HID keyboard and also ignores legacy keyboard messages
//	Rid[1].hwndTarget = hWnd;
//
//	if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE) {
//		DWORD error = GetLastError();
//		return false;
//	}
//
//	return true;
//}
//
//void wiRawInput::RetrieveData(LPARAM lParam)
//{
//	uint32_t dwSize;
//
//	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
//	LPBYTE lpb = new BYTE[dwSize];
//	if (lpb == NULL)
//	{
//		return;
//	}
//
//	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
//		OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
//
//	raw = RAWINPUT( (const RAWINPUT&)*lpb );
//
//	if (raw.header.dwType == RIM_TYPEHID)
//	{
//		int asd = 2;
//		asd = asd;
//	}
//
//	delete[] lpb;
//	return;
//}
//
//void wiRawInput::RetrieveBufferedData()
//{
//	//// Some temp global buffer, 1KB is overkill.
//	//static uint64_t rawBuffer[1024 / 8];
//	//
//	//// Then in some function,
//	//uint32_t bytes = sizeof(rawBuffer);
//	//// Loop through reading raw input until no events are left,
//	//while (1) {
//	//	// Fill up buffer,
//	//	int32_t count = GetRawInputBuffer((PRAWINPUT)rawBuffer, &bytes, sizeof(RAWINPUTHEADER));
//	//	if (count <= 0) return;
//	//	
//	//	// Process all the events, 
//	//	const RAWINPUT* raw = (const RAWINPUT*) rawBuffer;
//	//	while (1) {
//	//		// Process raw event.
//	//		this->raw = *raw;
//	//		// Goto next raw event.
//	//		count--;
//	//		if (count <= 0) break;
//	//		raw = NEXTRAWINPUTBLOCK(raw);
//	//	}
//	//}
//
//	//////while (true){
//	////	uint32_t cbSize;
//	////	//Sleep(1000);
//
//	////	GetRawInputBuffer(NULL, &cbSize, sizeof(RAWINPUTHEADER));
//	////	cbSize *= 16;            // this is a wild guess
//	////	PRAWINPUT pRawInput = (PRAWINPUT)malloc(cbSize);
//	////	if (pRawInput == NULL)
//	////	{
//	////		return;
//	////	}
//	////	for (;;)
//	////	{
//	////		uint32_t cbSizeT = cbSize;
//	////		uint32_t nInput = GetRawInputBuffer(pRawInput, &cbSizeT, sizeof(RAWINPUTHEADER));
//	////		if (nInput == 0)
//	////		{
//	////			break;
//	////		}
//	////		assert(nInput > 0);
//	////		PRAWINPUT* paRawInput = (PRAWINPUT*)malloc(sizeof(PRAWINPUT) * nInput);
//	////		if (paRawInput == NULL)
//	////		{
//	////			break;
//	////		}
//	////		PRAWINPUT pri = pRawInput;
//	////		for (uint32_t i = 0; i < nInput; ++i)
//	////		{
//	////			paRawInput[i] = pri;
//	////			pri = NEXTRAWINPUTBLOCK(pri);
//	////			if (pri->header.dwType == RIM_TYPEHID)
//	////			{
//	////				int asd=32;
//	////				asd = asd;
//	////			}
//	////		}
//	////		// to clean the buffer
//	////		DefRawInputProc(paRawInput, nInput, sizeof(RAWINPUTHEADER));
//
//	////		free(paRawInput);
//	////	}
//	////	free(pRawInput);
//	//////}
//}
//
//#endif //WINSTORE_SUPPORT
