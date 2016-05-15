#include <Windows.h>
extern "C" void* __cdecl memset(void* _Dst, int _Val, size_t _Size);
#pragma intrinsic(memset)
extern "C" void* __cdecl memcpy(void* _Dst, const void* _Src, size_t _Size);
#pragma intrinsic(memcpy)
extern "C" void* __cdecl malloc(size_t _Size);
void* __cdecl operator new(unsigned int bytes);
void __cdecl operator delete(void *ptr);
extern "C" int __cdecl __purecall(void);