#include "minilib.h"

#pragma function(memset)
extern "C" void* __cdecl memset(void* _Dst, int _Val, size_t _Size)
{
	for(size_t i = 0; i < _Size; i++)
		((BYTE*)_Dst)[i] = (BYTE)_Val;

	return _Dst;
}

#pragma function(memcpy)
extern "C" void* __cdecl memcpy(void* _Dst, const void * _Src, size_t _Size)
{
	for(size_t i = 0; i < _Size; i++)
		((BYTE*)_Dst)[i] = ((BYTE*)_Src)[i];

	return _Dst;
}

extern "C" void* __cdecl malloc(size_t _Size)
{
  return HeapAlloc(GetProcessHeap(), 0, _Size);
}

void * __cdecl operator new(unsigned int bytes)
{
  return HeapAlloc(GetProcessHeap(), 0, bytes);
}

void * __cdecl operator new[](unsigned int bytes)
{
	return HeapAlloc(GetProcessHeap(), 0, bytes);
}
 
void __cdecl operator delete(void *ptr)
{
  if(ptr) HeapFree(GetProcessHeap(), 0, ptr);
}

void __cdecl operator delete(void *ptr, unsigned int size)
{
	if (ptr && size) HeapFree(GetProcessHeap(), 0, ptr);
}
 
extern "C" int __cdecl __purecall(void)
{
  return 0;
}