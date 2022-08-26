#include "StdAfx.h"



NorlsAllocatorManager* g_pvbNorlsManager;

#if NRLSTRACK
NrlsAllocTracker *g_NrlsAllocTracker = NULL;
#endif NRLSTRACK


NorlsAllocatorManager::NorlsAllocatorManager() 
{
#if NRLSTRACK
    if (g_NrlsAllocTracker == NULL)
    {
        g_NrlsAllocTracker = new NrlsAllocTracker();
    }
#endif NRLSTRACK

}



NorlsAllocatorManager::~NorlsAllocatorManager()
{
#if NRLSTRACK_GETSTACKS    
    if (NorlsAllocator::g_nraSymbolHeap)
    {
        VBHeapDestroy(NorlsAllocator::g_nraSymbolHeap);
    }
#endif NRLSTRACK_GETSTACKS    
    
#if NRLSTRACK
    if (g_NrlsAllocTracker != NULL)
    {
        delete g_NrlsAllocTracker;
        g_NrlsAllocTracker = NULL;
    }
#endif NRLSTRACK

}




