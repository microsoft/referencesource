//-------------------------------------------------------------------------------------------------
//
// Code which allows us to replace new/delete for our dll/exe's.  Compile in VBHeap.cpp otherwise
// we will operate off of the standard new/delete
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

extern HANDLE g_vbCommonHeap;

void * _cdecl operator new(size_t cbSize)
{
#if DEBUG && _X86_
// if you get a leak report from VSAssert you'll get a line like:
//          d:\dd\vsleditor\src\vb\language\include\vbheap.cpp(10189270):	0x0DF2E258, bytes = 8, nAlloc=133514, TID=0x1104
// Break into the debugger. Take the # in parens, put it in Watch window: turn on hex display->it shows addr of caller
// Go to disassembly, put the address in the Address bar hit enter. Bingo: you're at the caller that didn't free!
// see http://blogs.msdn.com/calvin_hsia/archive/2009/01/19/9341632.aspx

    UINT *EbpRegister ;
    _asm { mov EbpRegister, ebp};
    UINT CallerAddr = *(((size_t *)EbpRegister)+1) ;

//    CallerAddr -=  (size_t)g_hinstDll;

    void *mem = VsDebugAllocInternal(g_vbCommonHeap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, (cbSize), __FILE__,CallerAddr , INSTANCE_GLOBAL, NULL);
#else
    void *mem = VBAlloc(cbSize);
#endif
    return mem;
}

void _cdecl operator delete(void * pv)
{
    VBFree(pv);
}

void * _cdecl operator new [](size_t cbSize)
{
#if DEBUG && _X86_
// if you get a leak report from VSAssert you'll get a line like:
//          d:\dd\vsleditor\src\vb\language\include\vbheap.cpp(10189270):	0x0DF2E258, bytes = 8, nAlloc=133514, TID=0x1104
// Break into the debugger. Take the # in parens, put it in Watch window: turn on hex display->it shows addr of caller
// Go to disassembly, put the address in the Address bar hit enter. Bingo: you're at the caller that didn't free!
// see http://blogs.msdn.com/calvin_hsia/archive/2009/01/19/9341632.aspx
    UINT *EbpRegister ;
    _asm { mov EbpRegister, ebp};
    UINT CallerAddr = *(((size_t *)EbpRegister)+1) ;

//    CallerAddr -=  (size_t)g_hinstDll;

    void *mem = VsDebugAllocInternal(g_vbCommonHeap, HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, (cbSize), __FILE__,CallerAddr , INSTANCE_GLOBAL, NULL);
#else
    void *mem = VBAlloc(cbSize);
#endif
    return mem;
}
 
void _cdecl operator delete[](void * pv)
{
    VBFree(pv);
}
 
void * _cdecl operator new(size_t byteSize, const std::nothrow_t&)
{
#if DEBUG && BUILDING_VBC
    return VBAllocOpt(byteSize,0);  // 0 = NoZero & NoThrow
#else
    return VBAllocOpt(byteSize, HEAP_ZERO_MEMORY);
#endif
}

void _cdecl operator delete(void * pv, const std::nothrow_t&)
{
    VBFree(pv);
}

void * _cdecl operator new [](size_t byteSize, const std::nothrow_t&)
{
#if DEBUG && BUILDING_VBC
    return VBAllocOpt(byteSize,0);  // 0 = NoZero & NoThrow
#else
    return VBAllocOpt(byteSize, HEAP_ZERO_MEMORY);
#endif
}

void _cdecl operator delete[](void * pv, const std::nothrow_t&)
{
    VBFree(pv);
}

