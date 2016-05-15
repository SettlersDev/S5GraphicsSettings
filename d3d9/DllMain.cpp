#include <Windows.h>
#include <d3d9.h>
#include "D3D9Proxy.h"
#include "detourxs.h"


#pragma comment(linker, "/EXPORT:Direct3DCreate9=_Direct3DCreate9Fake@4")
#pragma comment(linker, "/EXPORT:Direct3DCreate9Ex=_Direct3DCreate9ExFake@8")

extern "C"
{
	BOOL WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
	{
		switch (nReason)
		{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hDllHandle);

			InitD3D9Proxy();

			break;

		case DLL_PROCESS_DETACH:
			break;

		}

		return true;
	}


	IDirect3D9* WINAPI Direct3DCreate9Fake(UINT SDKVersion)
	{
		return CreateBaseInterface(SDKVersion);
	}

	HRESULT WINAPI Direct3DCreate9ExFake(UINT SDKVersion, IDirect3D9Ex** ppD3D9)
	{
		return CreateBaseExInterface(SDKVersion, ppD3D9);
	}
}