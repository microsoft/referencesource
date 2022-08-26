#include "stdafx.h"

VBHostedAllocator::VBHostedAllocator()
    :
    m_Allocator(NORLSLOC), 
    m_COMReleaseList()
{
}

VBHostedAllocator::~VBHostedAllocator()
{
    ConstIterator<IUnknown*> pCurrent;
    m_Allocator.FreeHeap();

    pCurrent = m_COMReleaseList.GetConstIterator();
     while (pCurrent.MoveNext())
    {
        pCurrent.Current()->Release();
    }

    m_COMReleaseList.Clear();
}

NorlsAllocator* VBHostedAllocator::GetNorlsAllocator()
{
    return &m_Allocator;
}

IUnknown* VBHostedAllocator::AddToReleaseList(IUnknown *punk)
{
    punk->AddRef();
    m_COMReleaseList.Add(punk);
    return punk;
}
