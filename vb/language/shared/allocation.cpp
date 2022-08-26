
#include "StdAfx.h"

HANDLE g_vbCommonHeap=NULL;
const zeromemory_t zeromemory;

void* _cdecl operator new(size_t cbSize, const zeromemory_t&)
{
#if DEBUG && _X86_ && IDE  // see vsproject\vb\vbprj\vbprj.cpp
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

// no need to ZeroMemory: we used HEAP_ZERO_MEMORY flag 

#else


    void *mem = ::operator new(cbSize);
    ZeroMemory(mem, cbSize);

#endif
    
    return mem;
}

void* _cdecl operator new[](size_t cbSize, const zeromemory_t&)
{
#if DEBUG && _X86_ && IDE //  // see vsproject\vb\vbprj\vbprj.cpp
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
// no need to ZeroMemory: we used HEAP_ZERO_MEMORY flag 

#else


    void *mem = ::operator new(cbSize);
    ZeroMemory(mem, cbSize);
#endif
    return mem;
}

void* _cdecl operator new(size_t cbSize, NorlsAllocator &norls)
{
    return norls.Alloc(cbSize);
}

