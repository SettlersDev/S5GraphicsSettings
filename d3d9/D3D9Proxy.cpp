#include <d3d9.h>
#include "detourxs.h"
#include "D3D9Proxy.h"

typedef IDirect3D9* (WINAPI *LPDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT (WINAPI *LPDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex** ppD3D9);
typedef HRESULT (_stdcall *LPGetAdapterIdentifier)(IDirect3D9 *d3d9, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier);
typedef HRESULT (_stdcall *LPCreateDevice)(IDirect3D9 *d3d9, 
										   UINT Adapter, 
										   D3DDEVTYPE DeviceType, 
										   HWND hFocusWindow, 
										   DWORD BehaviorFlags, 
										   D3DPRESENT_PARAMETERS* pPresentationParameters, 
										   IDirect3DDevice9** ppReturnedDeviceInterface);

typedef HRESULT (_stdcall *LPSetSamplerState)(IDirect3DDevice9 *dev, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
typedef HRESULT (_stdcall *LPGetRenderTargetData)(IDirect3DDevice9 *dev, IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface);

LPCreateDevice origCreateDevice = NULL;
LPGetAdapterIdentifier origGetAdapterIdentifier = NULL;
LPSetSamplerState origSetSamplerState = NULL;
LPGetRenderTargetData origGetRenderTargetData = NULL;

DWORD maxAnisotropy = 1; 

DWORD settingAA = 0;
DWORD settingAF = 0;
DWORD settingVSync = 0;

const char configTemplate[] = 
	"[S5Graphics]\r\n"
	"; Vertical Sync: prevents tearing\r\n"
	"; Allowed Values:  Off: 0  /  On: 1\r\n"
	"VSync = 1\r\n"
	"\r\n"
	"; Multi Sample Anti Aliasing: Smooth Borders\r\n"
	"; Allowed Values:  Off: 0  /  On: 2-16\r\n"
	"AA = 2\r\n"
	"\r\n"
	"; Anisotropic Filtering: Sharper Textures\r\n"
	"; Allowed Values:  Off: 0  /  On: 2, 4, 8, 16\r\n"
	"AF = 16";


#ifdef _DEBUG
#define Debug(str, ...) ShowMessageBox(str, __VA_ARGS__)

inline void Int3()
{
	__asm
	{
		int 3;
	}
}

#else
#define Debug(str, ...) do {} while(false)
#define Int3() do {} while(false)
#endif


inline void ShowMessageBox(char* str, ...)
{
	va_list args;
	va_start(args, str);

	char buf[256];
	wvsprintfA(buf, str, args);
	MessageBoxA(GetActiveWindow(), buf, "S5Graphics", MB_ICONINFORMATION);

	va_end(args);
}

HMODULE LoadOriginalDll(void)
{
	char buffer[MAX_PATH];

	GetSystemDirectoryA(buffer, MAX_PATH);
	lstrcatA(buffer, "\\d3d9.dll");
	HMODULE d3d9Library = LoadLibraryA(buffer);

	if (!d3d9Library)
	{
		ShowMessageBox("Couldn't load original d3d9.dll!");
		ExitProcess(0);
	}

	return d3d9Library;
}

inline void WritePointer(void *address, void *data)
{
	WriteProcessMemory(GetCurrentProcess(), address, &data, sizeof(void*), 0);
}

HRESULT _stdcall FakeSetSamplerState(IDirect3DDevice9 *dev, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
	if(Sampler == 0)
	{
		switch(Type)
		{
		case D3DSAMP_MINFILTER:
			Value = D3DTEXF_ANISOTROPIC;
			break;

		case D3DSAMP_MAXANISOTROPY:
			Value = settingAF;
			break;
		}
	} 
	return origSetSamplerState(dev, Sampler, Type, Value);
}

HRESULT _stdcall FakeGetRenderTargetData(IDirect3DDevice9 *dev, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface)
{
	D3DSURFACE_DESC rtDesc;
	pRenderTarget->GetDesc(&rtDesc);

	if(rtDesc.MultiSampleType == D3DMULTISAMPLE_NONE)
		return origGetRenderTargetData(dev, pRenderTarget, pDestSurface);

	// ok, we must get rid of MS first

	HRESULT res;
	IDirect3DSurface9 *noMSAARenderTarget;

	res = dev->CreateRenderTarget(rtDesc.Width, rtDesc.Height, rtDesc.Format, D3DMULTISAMPLE_NONE, 0, true, &noMSAARenderTarget, NULL);
	res = dev->StretchRect(pRenderTarget, NULL, noMSAARenderTarget, NULL, D3DTEXF_POINT);
	res = origGetRenderTargetData(dev, noMSAARenderTarget, pDestSurface);
	noMSAARenderTarget->Release();

	return res;
}

HRESULT _stdcall FakeCreateDevice(IDirect3D9 *d3d9, 
								  UINT Adapter, 
								  D3DDEVTYPE DeviceType, 
								  HWND hFocusWindow, 
								  DWORD BehaviorFlags, 
								  D3DPRESENT_PARAMETERS* pPresentationParameters, 
								  IDirect3DDevice9** ppReturnedDeviceInterface)
{

	D3DCAPS9 caps;

	if(D3D_OK == d3d9->GetDeviceCaps(Adapter, DeviceType, &caps))
		maxAnisotropy = caps.MaxAnisotropy;

	if(settingAF > maxAnisotropy)
	{
		if(maxAnisotropy <= 1)
			ShowMessageBox("This GPU doesn't support AF!");
		else
			ShowMessageBox("This GPU supports max. %dxAF!", maxAnisotropy);
		settingAF = maxAnisotropy;
	}

	Debug("CreateDevice(), pPresentationParameters: 0x%x", (void*)pPresentationParameters);

	DWORD msaaLevel = settingAA;
	DWORD qualityLevel = 1;

	for(; msaaLevel > 0; msaaLevel--, qualityLevel = 1)
	{
		if(D3D_OK == d3d9->CheckDeviceMultiSampleType(Adapter, DeviceType, 
			pPresentationParameters->BackBufferFormat, pPresentationParameters->Windowed, 
			(D3DMULTISAMPLE_TYPE)msaaLevel, &qualityLevel))
			break;
	}

	Debug("%dxMSAA, %d Levels", msaaLevel, qualityLevel);

	if(msaaLevel != settingAA)
	{
		if(msaaLevel == 0)
			ShowMessageBox("This GPU doesn't support AA!");
		else
			ShowMessageBox("This GPU supports max. %dxAA!", msaaLevel);
	}

	pPresentationParameters->BackBufferCount = 1;
	pPresentationParameters->MultiSampleType = (D3DMULTISAMPLE_TYPE)msaaLevel;
	pPresentationParameters->MultiSampleQuality = msaaLevel ? qualityLevel - 1 : 0;
	pPresentationParameters->PresentationInterval = settingVSync? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	pPresentationParameters->SwapEffect = msaaLevel? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_FLIP;

	HRESULT result = origCreateDevice(d3d9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

	auto *dev = *ppReturnedDeviceInterface;
	auto *vtable = *((void ***)dev);

	if(msaaLevel && origGetRenderTargetData == NULL)
		origGetRenderTargetData = (LPGetRenderTargetData)DetourCreate(vtable[32], FakeGetRenderTargetData, DETOUR_TYPE_JMP);

	if(settingAF && origSetSamplerState == NULL)
		origSetSamplerState = (LPSetSamplerState)DetourCreate(vtable[69], FakeSetSamplerState, DETOUR_TYPE_JMP);

	return result;
}

HRESULT _stdcall FakeGetAdapterIdentifier(IDirect3D9 *d3d9, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
	auto res = origGetAdapterIdentifier(d3d9, Adapter, Flags, pIdentifier);

	auto *nVidiaDriverVersion = (WORD *)&pIdentifier->DriverVersion.LowPart;
	if(pIdentifier->VendorId == 0x10DE)
		*nVidiaDriverVersion = 3788; 

	return res;
}

IDirect3D9* CreateBaseInterface(UINT SDKVersion)
{
	Debug("Direct3DCreate9(%d)", SDKVersion);

	auto d3d9Library = LoadOriginalDll();
	auto D3D9OrigCreate = (LPDirect3DCreate9)GetProcAddress(d3d9Library, "Direct3DCreate9");
	auto *d3d9 = D3D9OrigCreate(SDKVersion);

	auto *vtable = *((int**)d3d9);

	if(origCreateDevice == NULL)
	{
		origCreateDevice = (LPCreateDevice)vtable[16];
		WritePointer(&vtable[16], FakeCreateDevice);

		origGetAdapterIdentifier = (LPGetAdapterIdentifier)vtable[5];
		WritePointer(&vtable[5], FakeGetAdapterIdentifier);
	}

	return d3d9;
}

HRESULT CreateBaseExInterface(UINT SDKVersion, IDirect3D9Ex** ppD3D9)
{
	Debug("ignoring Direct3DCreate9Ex(%d, XYZ)", SDKVersion);

	auto d3d9Library = LoadOriginalDll();
	auto D3D9OrigCreateEx = (LPDirect3DCreate9Ex)GetProcAddress(d3d9Library, "Direct3DCreate9Ex");
	return D3D9OrigCreateEx(SDKVersion, ppD3D9);
}

void InitD3D9Proxy() 
{
	char iniPath[MAX_PATH] = {0};

	GetModuleFileNameA(GetModuleHandle(NULL), iniPath, MAX_PATH);

	for(int i = lstrlenA(iniPath) - 1; i > 0; i--)
	{
		if(iniPath[i] == '\\')
		{
			iniPath[i+1] = 0;
			break;
		}
	}

	lstrcatA(iniPath, "graphics.ini");

	HANDLE hConfigFile = CreateFileA(iniPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hConfigFile == INVALID_HANDLE_VALUE)
	{
		ShowMessageBox("graphics.ini not found, creating new file...");
		hConfigFile = CreateFileA(iniPath, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);

		DWORD bytesWritten;

		if(hConfigFile == INVALID_HANDLE_VALUE)
			ShowMessageBox("Couldn't create graphics.ini!");
		else
			WriteFile(hConfigFile, configTemplate, sizeof(configTemplate) - 1, &bytesWritten, 0);
	}
	CloseHandle(hConfigFile);

	settingVSync = GetPrivateProfileIntA("S5Graphics", "VSync", 0, iniPath);
	settingAA = GetPrivateProfileIntA("S5Graphics", "AA", 0, iniPath);
	settingAF = GetPrivateProfileIntA("S5Graphics", "AF", 0, iniPath);
}