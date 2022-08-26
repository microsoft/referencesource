//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Implements connection classes.
//
// History:
//      2001/10/01-michdav
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

/**************************************************************************
   #include statements
/**************************************************************************/

#include "Precompiled.hxx"
#include "ConnectionPoint.hxx"

/**************************************************************************
/
/  CConnectionPointContainer - constructor/destructor
/
**************************************************************************/

CConnectionPointContainer::CConnectionPointContainer(__in IUnknown * pUnkOuter):
    m_pUnkOuter(pUnkOuter), m_CPAry(NULL)
{
}

CConnectionPointContainer::~CConnectionPointContainer()
{
    if (m_CPAry)
    {
        for (int i = 0; i < m_arySize; i++)
        {
            if ( m_CPAry[i] != NULL)
            {
                m_CPAry[i]->Release();
            }
        }

        delete [] m_CPAry;
    }
}

STDMETHODIMP
CConnectionPointContainer::Init()
{
    HRESULT hr = S_OK;

    m_CPAry = new CConnectionPoint*[m_arySize];
    CK_ALLOC(m_CPAry);

    // 


    m_CPAry[0] = new CConnectionPoint(IID_IPropertyNotifySink, this);
    if (!m_CPAry[0])
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    return hr;

Cleanup:      
    if (m_CPAry)
    {
        if (m_CPAry[0])
        {
            delete m_CPAry[0];
        }

        delete [] m_CPAry;
        m_CPAry = NULL;
    }

    return hr;
}

/**************************************************************************
/
/  CConnectionPointContainer - IUnknown implementation
/
/  this just forwards to the pUnkOuter
/
**************************************************************************/

STDMETHODIMP 
CConnectionPointContainer::QueryInterface(REFIID riid, __out LPVOID* ppVoid)
{
    return m_pUnkOuter->QueryInterface(riid, ppVoid);
}


STDMETHODIMP_(DWORD) 
CConnectionPointContainer::AddRef()
{    
     return m_pUnkOuter->AddRef();
}

   
STDMETHODIMP_(DWORD) 
CConnectionPointContainer::Release()
{
    return m_pUnkOuter->Release();
}

/**************************************************************************
/
/  CConnectionPointContainer - IConnectionPointContainer implementation
/
**************************************************************************/
   
STDMETHODIMP 
CConnectionPointContainer::EnumConnectionPoints(__out IEnumConnectionPoints ** ppIECP)
{
    // 




    if(ppIECP != NULL)
    {
        *ppIECP = NULL;
    }
    return E_NOTIMPL;
}

   
STDMETHODIMP 
CConnectionPointContainer::FindConnectionPoint(REFIID riid, __out_opt IConnectionPoint ** ppICP)
{
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppICP)
    {
        return E_INVALIDARG;
    }

    *ppICP = NULL;

    if (riid == IID_IPropertyNotifySink)
    {
        *ppICP = m_CPAry[0];
        (*ppICP)->AddRef();
        return S_OK;        
    }

    return hr;
}

/**************************************************************************
/
/  CConnectionPoint- constructor / destructor
/
**************************************************************************/

CConnectionPoint::CConnectionPoint(const IID &iid, __in IConnectionPointContainer * pCPC)
{
    m_ObjRefCount = 1;
    m_IID = iid;
    m_NextCookie = 1;
    m_pCPC = pCPC;

    for (int i = 0; i < MAXCONNECTIONS; i++)
    {
        m_ConAry[i].dwCookie = 0;
        m_ConAry[i].pUnk = NULL;
    }
}

CConnectionPoint::~CConnectionPoint()
{
    for (int i = 0; i < MAXCONNECTIONS; i++)
    {
        // these should be released when the
        // sink does an Unadvise, but just 
        // in case...
        if (m_ConAry[i].pUnk != NULL)
        {
            m_ConAry[i].pUnk->Release();
        }
    }
}



/**************************************************************************
/
/  CConnectionPoint- IUnknown implementation
/
**************************************************************************/

STDMETHODIMP 
CConnectionPoint::QueryInterface(REFIID riid, __out LPVOID* ppVoid)
{
    if(ppVoid == NULL)
    {
        return E_INVALIDARG;
    }

    if (riid == IID_IConnectionPoint || riid == IID_IUnknown)
    {
        *ppVoid = this;
        AddRef();
        return S_OK;
    }

    *ppVoid = NULL;
    return E_NOINTERFACE;
}

   
STDMETHODIMP_(DWORD) 
CConnectionPoint::AddRef()
{
    return InterlockedIncrement(&m_ObjRefCount);
}

   
STDMETHODIMP_(DWORD) 
CConnectionPoint::Release()
{
    if (InterlockedDecrement(&m_ObjRefCount) == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return m_ObjRefCount;
    }
}

/**************************************************************************
/
/  CConnectionPoint- IConnectionPoint implementation
/
**************************************************************************/
      
STDMETHODIMP 
CConnectionPoint::GetConnectionInterface(__out IID * pIID)
{
    if(pIID == NULL)
    {
        return E_INVALIDARG;
    }

    *pIID = m_IID;
    return S_OK;
}

   
STDMETHODIMP 
CConnectionPoint::GetConnectionPointContainer(__out IConnectionPointContainer ** ppCPC)
{
    if(ppCPC == NULL)
    {
        return E_INVALIDARG;
    }
    *ppCPC = m_pCPC;
    return S_OK;
}

   
STDMETHODIMP 
CConnectionPoint::Advise(__in IUnknown * pUnk, __out DWORD * pdwCookie)
{
    if(pdwCookie == NULL || pUnk == NULL )
    {
        return E_INVALIDARG;
    }

    for (int i = 0; i < MAXCONNECTIONS; i++)
    {
        if (m_ConAry[i].dwCookie == 0)
        {
            // this addrefs the pointer and ensures it's the right type
            HRESULT hr = pUnk->QueryInterface(m_IID, (void**)&m_ConAry[i].pUnk);
            if (FAILED(hr))
            {
                *pdwCookie = 0;
                return CONNECT_E_CANNOTCONNECT;
            }
            m_ConAry[i].dwCookie = m_NextCookie++;
            *pdwCookie = m_ConAry[i].dwCookie;
            return S_OK;
        }
    }

    return CONNECT_E_ADVISELIMIT;
}
   
STDMETHODIMP 
CConnectionPoint::Unadvise(DWORD dwCookie)
{
    if (dwCookie) // IE is suspected to pass in zero in stress conditions.
    {
        for (int i = 0; i < MAXCONNECTIONS; i++)
        {
            if (m_ConAry[i].dwCookie == dwCookie)
            {
                m_ConAry[i].dwCookie = 0;
                m_ConAry[i].pUnk->Release();
                m_ConAry[i].pUnk = NULL;
                return S_OK;
            }
        }
    }
    return CONNECT_E_NOCONNECTION;
}
   
STDMETHODIMP 
CConnectionPoint::EnumConnections(__out_opt IEnumConnections ** ppEnum)
{
    if (!ppEnum)
    {
        return E_INVALIDARG;
    }

    CConnectionPointEnum * pCPE = new CConnectionPointEnum(this);
    if (!pCPE)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }

    *ppEnum = pCPE;
    return S_OK;
}


/**************************************************************************
/
/  CConnectionPoint::CConnectionPointEnum
/
/  This is an object that implements an enumerator for the connection
/  points. It keeps a ref on the CConnectionPoint and releases that ref
/  when it's destroyed.
/
**************************************************************************/

STDMETHODIMP
CConnectionPoint::CConnectionPointEnum::QueryInterface(REFIID riid, __out LPVOID* ppVoid)
{
    if(ppVoid == NULL)
    {
        return E_INVALIDARG;
    }

    if (riid == IID_IUnknown || riid == IID_IEnumConnectionPoints)
    {
        *ppVoid = this;
        AddRef();     
        return S_OK;
    }
    
    *ppVoid = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(DWORD)
CConnectionPoint::CConnectionPointEnum::AddRef()
{
   return InterlockedIncrement(&m_ObjRefCount);
}

STDMETHODIMP_(DWORD)
CConnectionPoint::CConnectionPointEnum::Release()
{
    if (InterlockedDecrement(&m_ObjRefCount) == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return m_ObjRefCount;
    }
}

STDMETHODIMP
CConnectionPoint::CConnectionPointEnum::Next(ULONG oConnections, LPCONNECTDATA rgpcd, __out ULONG * pcFetched)
{
    if(pcFetched == NULL)
    {
        return E_INVALIDARG;
    }

    __bound ULONG fetched = 0;    

    for (ULONG i=0; i<oConnections && m_Current<MAXCONNECTIONS; m_Current++, i++)
    {
        // m_Current is limited by MAXCONNECTIONS, so no overflow is caused by the ++ operation
        if (m_pOuter->m_ConAry[m_Current].pUnk != NULL)
        {
            rgpcd[fetched] = m_pOuter->m_ConAry[m_Current];
            rgpcd[fetched].pUnk->AddRef();
            fetched++;            
        }
    }

    *pcFetched = fetched;
    return S_OK;
}

STDMETHODIMP
CConnectionPoint::CConnectionPointEnum::Skip(ULONG oConnections)
{
    HRESULT hr = S_OK;
    CKHR(UIntAdd(oConnections, m_Current, &m_Current));
    if ( m_Current >= MAXCONNECTIONS )
    {
        m_Current = MAXCONNECTIONS - 1;
    }

Cleanup: 
    return hr;
}

STDMETHODIMP
CConnectionPoint::CConnectionPointEnum::Reset()
{
    m_Current = 0;
    return S_OK;
}

STDMETHODIMP
CConnectionPoint::CConnectionPointEnum::Clone(__out_opt IEnumConnections** ppEnum)
{
    if (!ppEnum)
    {
        return E_INVALIDARG;
    }

    CConnectionPointEnum * cp = new CConnectionPointEnum(m_pOuter, m_Current);
    if (!cp)
    {
        *ppEnum = NULL;
        return E_OUTOFMEMORY;
    }

    *ppEnum = cp;

    return S_OK;
}

