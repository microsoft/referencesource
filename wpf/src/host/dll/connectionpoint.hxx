//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines connection classes.
//
// History:
//      2001/10/01-michdav
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

class CConnectionPoint : public IConnectionPoint
{    
public:
    CConnectionPoint(const IID&, __in IConnectionPointContainer *);
    ~CConnectionPoint();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IConnectionPointMethods
    STDMETHODIMP GetConnectionInterface(__out IID *);
    STDMETHODIMP GetConnectionPointContainer(__out IConnectionPointContainer **);
    STDMETHODIMP Advise(__in IUnknown *, __out DWORD *);
    STDMETHODIMP Unadvise(DWORD);
    STDMETHODIMP EnumConnections(__out_opt IEnumConnections **);

private:
    long                        m_ObjRefCount;
    IID                         m_IID;
    int                         m_NextCookie;
    IConnectionPointContainer * m_pCPC;    

    // 


    static const int        MAXCONNECTIONS = 10;
    struct tagCONNECTDATA   m_ConAry[MAXCONNECTIONS];

    // inner class that handles enumeration, all private

    class CConnectionPointEnum : IEnumConnections
    {
        friend class CConnectionPoint;

        CConnectionPointEnum(__in CConnectionPoint * pOuter, int cur = 0) : 
            m_pOuter(pOuter), m_ObjRefCount(1) , m_Current(cur)
        {
            m_pOuter->AddRef();
        }
        ~CConnectionPointEnum() 
        {
            m_pOuter->Release();
        }

        // IUnkown members
        STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
        STDMETHODIMP_(DWORD) AddRef();
        STDMETHODIMP_(DWORD) Release();

        //IEnumConnections methods
        STDMETHODIMP Next(ULONG, LPCONNECTDATA, __out ULONG *);
        STDMETHODIMP Skip(ULONG);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(__out_opt IEnumConnections**);

        CConnectionPoint *   m_pOuter;
        long                 m_ObjRefCount;
        UINT                 m_Current;
    };
};

class CConnectionPointContainer : public IConnectionPointContainer
{    
public:
    CConnectionPointContainer(__in IUnknown *);
    ~CConnectionPointContainer();

    STDMETHODIMP Init();

    //IUnknown methods
    STDMETHODIMP QueryInterface(REFIID, __out LPVOID*);
    STDMETHODIMP_(DWORD) AddRef();
    STDMETHODIMP_(DWORD) Release();

    //IConnectionPointContainerMethods
    STDMETHODIMP EnumConnectionPoints(__out IEnumConnectionPoints **);
    STDMETHODIMP FindConnectionPoint(REFIID, __out_opt IConnectionPoint **);

private:
    IUnknown         *  m_pUnkOuter;
    CConnectionPoint ** m_CPAry;  

    static const int    m_arySize = 1;
};

