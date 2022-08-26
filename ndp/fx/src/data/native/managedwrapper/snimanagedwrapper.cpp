//------------------------------------------------------------------------------
// <copyright file="SniManagedWrapper.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <owner current="true" primary="true">Microsoft</owner>
// <owner current="true" primary="false">Microsoft</owner>
// <owner current="true" primary="false">Microsoft</owner>
//------------------------------------------------------------------------------

#using   <mscorlib.dll>
#using   <System.dll>
#include <windows.h>
#include "sni.hpp"
#include "sni_rc.h"
#include <assert.h>
#include <vcclr.h>
#include <msdasc.h>
#include <oledb.h>
#include <corerror.h>
#include <oledberr.h>
#include <mscoree.h>


using namespace System;
using namespace System::Diagnostics;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::ConstrainedExecution;
using namespace System::Runtime::InteropServices;
using namespace System::Text;
using namespace System::Runtime::Versioning;
using namespace System::Security;

[module: System::CLSCompliant(true)];

// This is a compile time Assert used to verify at compile time that a condition is true.
// Comments should be placed by the assert to indicate what's failing
#define COMPILE_TIME_ASSERT(pred)       \
{switch(0){case 0:case pred:;}}

static inline
System::Guid FromGUID(GUID const & guid)
{
   return System::Guid( guid.Data1, guid.Data2, guid.Data3, 
                        guid.Data4[ 0 ], guid.Data4[ 1 ], 
                        guid.Data4[ 2 ], guid.Data4[ 3 ], 
                        guid.Data4[ 4 ], guid.Data4[ 5 ], 
                        guid.Data4[ 6 ], guid.Data4[ 7 ] );
}

static inline
IUnknown * GetDefaultAppDomain()
{
    ICorRuntimeHost *pHost = NULL;

    try
    {
        // Throws HR exception on failure.
        pHost = reinterpret_cast<ICorRuntimeHost*>(
            RuntimeEnvironment::GetRuntimeInterfaceAsIntPtr(
                FromGUID(__uuidof(CorRuntimeHost)),
                FromGUID(__uuidof(ICorRuntimeHost))).ToPointer());
    }
    catch (Exception^)
    {
        return NULL;
    }

    // GetDefaultDomain will not throw.
    IUnknown *pAppDomainPunk = NULL;
    HRESULT hr = pHost->GetDefaultDomain(&pAppDomainPunk);
    pHost->Release();

    return SUCCEEDED(hr) ? pAppDomainPunk : NULL;
}

#pragma unmanaged

extern void SNISetLastError( ProviderNum Provider,
                      DWORD       dwNativeError,
                      DWORD          dwSNIError,
                      WCHAR     * pszFileName,
                      WCHAR     * pszFunction,
                      DWORD       dwLineNumber );

extern void SNIGetLastError( __out_opt SNI_ERROR * pSNIerror);

// The following unmanaged section is used by SqlNotifications to store the reference to the managed 
// SqlDependencyProcessDispatcher in native.  It is used by all AppDomains in the process.  We need to
// store this in native since this is the best place for process wide state for a managed app.

class SqlDependencyProcessDispatcherStorage {

    static VOID*         data;     
    static int           size;
    static volatile long lock; // Int used for a spin-lock.

public:

    static VOID* NativeGetData(int &passedSize) {
        passedSize = size;
        return data;
    }

    static bool NativeSetData(void* passedData, int passedSize) {
        bool success = false;

        while (0 != InterlockedCompareExchange(&lock, 1, 0)) { // Spin until we have the lock.
            Sleep(50); // Sleep with short-timeout to prevent starvation.
        }
        assert ( 1 == lock ); // Now that we have the lock, lock should be equal to 1.

        if (NULL == data) {
            data = GlobalAlloc(GPTR, passedSize);

            assert ( NULL != data);
            
            memcpy(data, passedData, passedSize);

            assert ( 0 == size ); // Size should still be zero at this point.
            size    = passedSize;
            success = true;
        }

        int result = InterlockedCompareExchange(&lock, 0, 1);
        assert ( 1 == result ); // The release of the lock should have been successfull.  

        return success;
    }
};

// Data, Size - will store the serialized SqlDependencyProcessDispatcher for the process.  This will remain for the
// lifetime of the process.
VOID*         SqlDependencyProcessDispatcherStorage::data;     
int           SqlDependencyProcessDispatcherStorage::size;
volatile long SqlDependencyProcessDispatcherStorage::lock; // Int used for a spin-lock.

#undef Assert

struct SNI_ConnWrapper
{
    SNI_ConnWrapper(__in SNI_CONSUMER_INFO * pConsumerInfo) :
        m_pConn(NULL),
        m_fnReadComp(pConsumerInfo->fnReadComp),
        m_fnWriteComp(pConsumerInfo->fnWriteComp),
        m_ConsumerKey(pConsumerInfo->ConsumerKey),
        m_pPacket(NULL),
        m_fSyncOverAsyncRead(false),
        m_fSyncOverAsyncWrite(false),
        m_fSupportsSyncOverAsync(false),
        m_fPendingRead(false)
    {
        m_ReadResponseReady = ::CreateSemaphore(NULL, 0, 1, NULL);
        m_WriteResponseReady = ::CreateSemaphore(NULL, 0, 1, NULL);
        ::InitializeCriticalSection(&m_ReadLock);
    }

    ~SNI_ConnWrapper()
    {
        CloseHandle(m_ReadResponseReady);
        CloseHandle(m_WriteResponseReady);
        DeleteCriticalSection(&m_ReadLock);
    }

    SNI_Conn* m_pConn;
    PIOCOMP_FN m_fnReadComp;
    PIOCOMP_FN m_fnWriteComp;
    LPVOID m_ConsumerKey;
    
    CRITICAL_SECTION m_ReadLock;
    HANDLE m_ReadResponseReady;
    HANDLE m_WriteResponseReady;
    bool m_fPendingRead;

    SNI_Packet *m_pPacket;
    SNI_ERROR m_WriteError;
    SNI_ERROR m_Error;
    bool m_fSyncOverAsyncRead;
    bool m_fSyncOverAsyncWrite;
    bool m_fSupportsSyncOverAsync;
};

DWORD SNIWriteAsyncWrapper(__in SNI_ConnWrapper * pConn, __in SNI_Packet * pPacket) {
    pConn->m_fSyncOverAsyncWrite = false;
    return SNIWriteAsync(pConn->m_pConn, pPacket);
}

DWORD SNIWriteSyncOverAsync(__in SNI_ConnWrapper * pConn, __in SNI_Packet * pPacket) {
    if (pConn->m_fSupportsSyncOverAsync) {
        return SNIWriteSync(pConn->m_pConn, pPacket, NULL);
    }
            
    pConn->m_fSyncOverAsyncWrite = true;
    DWORD dwError = SNIWriteAsync(pConn->m_pConn, pPacket);
    if(ERROR_IO_PENDING == dwError ) {

        dwError = ::WaitForSingleObject(pConn->m_WriteResponseReady, INFINITE);
        if (ERROR_SUCCESS == dwError) {

            if (pConn->m_WriteError.dwNativeError != ERROR_SUCCESS) {
                SNISetLastError( pConn->m_WriteError.Provider, pConn->m_WriteError.dwNativeError, pConn->m_WriteError.dwSNIError, NULL, NULL, 0);
            }
            dwError = pConn->m_WriteError.dwNativeError;
        }
        else {
            SNISetLastError( INVALID_PROV, dwError, SNIE_SYSTEM, NULL, NULL, 0);
        }
    }
    
    assert(dwError != ERROR_IO_PENDING); // should never return pending
    return dwError;
}

DWORD SNIReadAsyncWrapper(__in SNI_ConnWrapper * pConn, __out SNI_Packet ** ppNewPacket) {
    pConn->m_fSyncOverAsyncRead = false;
    return SNIReadAsync(pConn->m_pConn, ppNewPacket, NULL);
}

DWORD SNIReadSyncOverAsync(__in SNI_ConnWrapper * pConn, __out SNI_Packet ** ppNewPacket, int timeout) {
    *ppNewPacket = NULL;

    if (pConn->m_fSupportsSyncOverAsync) {
        return SNIReadSync(pConn->m_pConn, ppNewPacket, timeout);
    }

    ::EnterCriticalSection(&pConn->m_ReadLock);
    DWORD dwError;
    if (!pConn->m_fPendingRead) {
        pConn->m_fSyncOverAsyncRead = true;
        pConn->m_fPendingRead = true;
        dwError = SNIReadAsync(pConn->m_pConn, ppNewPacket, NULL);
        assert((*ppNewPacket == NULL && dwError != ERROR_SUCCESS) || (*ppNewPacket != NULL && dwError == ERROR_SUCCESS));
    }
    else {
        assert(pConn->m_fSyncOverAsyncRead); // should be syncOverAsync from last call to SNIReadSyncOverAsync
        dwError = ERROR_IO_PENDING;
    }

    if( ERROR_IO_PENDING == dwError ) {
        dwError = ::WaitForSingleObject(pConn->m_ReadResponseReady, timeout);
        
        if (dwError == ERROR_TIMEOUT) {
            // treat ERROR_TIMEOUT as WAIT_TIMEOUT as that is what is expected by callers
            dwError = WAIT_TIMEOUT;
        }
        else if( ERROR_SUCCESS == dwError ) {
            pConn->m_fPendingRead = false;
            *ppNewPacket = pConn->m_pPacket;
            pConn->m_pPacket = NULL;
            if (pConn->m_Error.dwNativeError != ERROR_SUCCESS) {
                SNISetLastError( pConn->m_Error.Provider, pConn->m_Error.dwNativeError, pConn->m_Error.dwSNIError, NULL, NULL, 0);
            }
            dwError = pConn->m_Error.dwNativeError;
            assert((*ppNewPacket == NULL && dwError != ERROR_SUCCESS) || (*ppNewPacket != NULL && dwError == ERROR_SUCCESS));
        }
        else {
            SNISetLastError( INVALID_PROV, dwError, SNIE_SYSTEM, NULL, NULL, 0);
        }
    }
    else if (dwError == ERROR_SUCCESS) {
        pConn->m_fPendingRead = false;
    }

    ::LeaveCriticalSection(&pConn->m_ReadLock);

    assert((*ppNewPacket == NULL && dwError != ERROR_SUCCESS) || (*ppNewPacket != NULL && dwError == ERROR_SUCCESS));
    return dwError;
}

void UnmanagedReadCallback(LPVOID ConsKey, SNI_Packet * pPacket, DWORD dwError)
{
    SNI_ConnWrapper* pConn = (SNI_ConnWrapper*)ConsKey;

    if (!pConn->m_fSyncOverAsyncRead) {
        pConn->m_fnReadComp(pConn->m_ConsumerKey, pPacket, dwError);
    }
    else {
        if (dwError == ERROR_SUCCESS) {
            SNIPacketAddRef(pPacket);
            pConn->m_pPacket = pPacket;
            pConn->m_Error.dwNativeError = ERROR_SUCCESS;
        }
        else {
            pConn->m_pPacket = NULL;
            SNIGetLastError(&pConn->m_Error);
            // SNIGetLastError strips SNI_STRING_ERROR_BASE out of the code
            pConn->m_Error.dwSNIError += SNI_STRING_ERROR_BASE;
            assert(pConn->m_Error.dwNativeError != ERROR_SUCCESS);
        }

        ::ReleaseSemaphore(pConn->m_ReadResponseReady, 1, NULL);
    }

}

void UnmanagedWriteCallback(LPVOID ConsKey, SNI_Packet * pPacket, DWORD dwError)
{
    SNI_ConnWrapper* pConn = (SNI_ConnWrapper*)ConsKey;

     if (!pConn->m_fSyncOverAsyncWrite) {
        pConn->m_fnWriteComp(pConn->m_ConsumerKey, pPacket, dwError);
     }
     else {
        if (dwError == ERROR_SUCCESS) {
            pConn->m_WriteError.dwNativeError = ERROR_SUCCESS;
        }
        else {
            SNIGetLastError(&pConn->m_WriteError);
            // SNIGetLastError strips SNI_STRING_ERROR_BASE out of the code
            pConn->m_WriteError.dwSNIError += SNI_STRING_ERROR_BASE;
            assert(pConn->m_WriteError.dwNativeError != ERROR_SUCCESS);
        }

        ::ReleaseSemaphore(pConn->m_WriteResponseReady, 1, NULL);
     }
}

DWORD SNIOpenWrapper( __in SNI_CONSUMER_INFO * pConsumerInfo,
                       __inout_opt LPWSTR wszConnect,
                       __in LPVOID pOpenInfo,
                       __out SNI_ConnWrapper ** ppConn,
                       __in BOOL fSync)
{
    SNI_ConnWrapper* pConnWrapper = NULL;
    SNI_Conn* pConn = NULL;
    DWORD dwError = ERROR_SUCCESS;

    pConnWrapper = new SNI_ConnWrapper(pConsumerInfo);
    if (pConnWrapper == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        SNISetLastError( INVALID_PROV, dwError, SNIE_SYSTEM, NULL, NULL, 0);
        goto ErrorExit;
    }

    pConsumerInfo->fnReadComp = UnmanagedReadCallback;
    pConsumerInfo->fnWriteComp = UnmanagedWriteCallback;
    pConsumerInfo->ConsumerKey = pConnWrapper;

    dwError = SNIOpen(pConsumerInfo, wszConnect, pOpenInfo, &pConn, fSync);
    if (dwError != ERROR_SUCCESS) {
        goto ErrorExit;
    }

    pConnWrapper->m_pConn = pConn;

    BOOL fSupportsSyncOverAsync;
    dwError = SNIGetInfo(pConn, SNI_QUERY_CONN_SUPPORTS_SYNC_OVER_ASYNC, &fSupportsSyncOverAsync);
    assert(dwError == ERROR_SUCCESS); // SNIGetInfo cannot fail with this QType

    // convert BOOL to bool
    pConnWrapper->m_fSupportsSyncOverAsync = !!fSupportsSyncOverAsync;

    *ppConn = pConnWrapper;
    return ERROR_SUCCESS;

ErrorExit:
    if (pConnWrapper) {
        delete pConnWrapper;
    }
    return dwError;
}

DWORD SNIOpenSyncExWrapper( __inout SNI_CLIENT_CONSUMER_INFO * pClientConsumerInfo, __deref_out SNI_ConnWrapper ** ppConn)
{
    SNI_ConnWrapper* pConnWrapper = NULL;
    SNI_Conn* pConn = NULL;
    DWORD dwError = ERROR_SUCCESS;

    pConnWrapper = new SNI_ConnWrapper(&pClientConsumerInfo->ConsumerInfo);
    if (pConnWrapper == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        SNISetLastError( INVALID_PROV, dwError, SNIE_SYSTEM, NULL, NULL, 0);
        goto ErrorExit;
    }

    pClientConsumerInfo->ConsumerInfo.fnReadComp = UnmanagedReadCallback;
    pClientConsumerInfo->ConsumerInfo.fnWriteComp = UnmanagedWriteCallback;
    pClientConsumerInfo->ConsumerInfo.ConsumerKey = pConnWrapper;

    dwError = SNIOpenSyncEx(pClientConsumerInfo, &pConn);
    if (dwError != ERROR_SUCCESS) {
        goto ErrorExit;
    }

    pConnWrapper->m_pConn = pConn;

    BOOL fSupportsSyncOverAsync;
    dwError = SNIGetInfo(pConn, SNI_QUERY_CONN_SUPPORTS_SYNC_OVER_ASYNC, &fSupportsSyncOverAsync);
    assert(dwError == ERROR_SUCCESS); // SNIGetInfo cannot fail with this QType

    // convert BOOL to bool
    pConnWrapper->m_fSupportsSyncOverAsync = !!fSupportsSyncOverAsync;

    *ppConn = pConnWrapper;
    return ERROR_SUCCESS;

ErrorExit:
    if (pConnWrapper) {
        delete pConnWrapper;
    }
    return dwError;
}

// Dev11 34043 - move IsTokenRestricted to be pure native to avoid CLR/PInvoke and JIT interception with native SetLastError
DWORD UnmanagedIsTokenRestricted(__in HANDLE token, __out BOOL *isRestricted)
{
    // SQLBUDT #373863 - IsTokenRestricted doesn't clear this in all cases.
    ::SetLastError(0);

    *isRestricted = ::IsTokenRestricted(token); // calls into win32 API
    return ::GetLastError();
}

DWORD SNIPacketGetDataWrapper(__in SNI_Packet * packet, __in BYTE * readBuffer, __in DWORD readBufferLength, __in DWORD * dataSize)
{
    BYTE* byteData = NULL;
    SNIPacketGetData(packet, &byteData, dataSize);
    DWORD dwError = memcpy_s(readBuffer, readBufferLength, byteData, *dataSize);
    return dwError;
}

#pragma managed

[class:CLSCompliant(false)]
ref class  SNINativeMethodWrapper
{
     internal :

        // The following three methods are used by SqlDependency to obtain/store the serialized
        // SqlDependencyProcessDispatcher in native, as well as obtaining the default AppDomain.

        [ResourceExposure(ResourceScope::Process)] // SxS: there is no way to set scope = Instance, using Process which is wider
        [ResourceConsumption(ResourceScope::Process, ResourceScope::Process)]
        static cli::array<System::Byte>^ GetData() {
            System::Int32                          size;
            System::IntPtr                         ptr = 
                    static_cast<System::IntPtr>(SqlDependencyProcessDispatcherStorage::NativeGetData(size));
            cli::array<System::Byte>^ result;

            if (ptr != System::IntPtr::Zero) {
                result = gcnew cli::array<System::Byte>(size);
                Marshal::Copy(ptr, result, 0, size);
            }            

            return result;
        }

        [ResourceExposure(ResourceScope::Process)] // SxS: there is no way to set scope = Instance, using Process which is wider
        [ResourceConsumption(ResourceScope::Process, ResourceScope::Process)]
        static void SetData(cli::array<System::Byte>^ data) {
            cli::pin_ptr<System::Byte> pin_dispatcher = &data[0];
            SqlDependencyProcessDispatcherStorage::NativeSetData(pin_dispatcher, data->Length);
        }

        [ResourceExposure(ResourceScope::Process)] // SxS: there is no way to set scope = Instance, using Process which is wider
        [ResourceConsumption(ResourceScope::Process, ResourceScope::Process)]
        static _AppDomain^ GetDefaultAppDomain() {
            System::IntPtr ptr    = static_cast<System::IntPtr>(::GetDefaultAppDomain());
            System::Object ^myObj = Marshal::GetObjectForIUnknown(ptr);

            Marshal::Release(ptr);

            return dynamic_cast<_AppDomain^>(myObj);
        }

        // End SqlDependency related code.

        enum class QTypes  {
            SNI_QUERY_CONN_INFO               = ::SNI_QUERY_CONN_INFO,
            SNI_QUERY_CONN_BUFSIZE            = ::SNI_QUERY_CONN_BUFSIZE,
            SNI_QUERY_CONN_KEY                = ::SNI_QUERY_CONN_KEY,
            SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE = ::SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE,
            SNI_QUERY_SERVER_ENCRYPT_POSSIBLE = ::SNI_QUERY_SERVER_ENCRYPT_POSSIBLE,
            SNI_QUERY_CERTIFICATE             = ::SNI_QUERY_CERTIFICATE,
            SNI_QUERY_CONN_ENCRYPT            = ::SNI_QUERY_CONN_ENCRYPT,
            SNI_QUERY_CONN_PROVIDERNUM        = ::SNI_QUERY_CONN_PROVIDERNUM,
            SNI_QUERY_CONN_CONNID             = ::SNI_QUERY_CONN_CONNID,
            SNI_QUERY_CONN_PARENTCONNID       = ::SNI_QUERY_CONN_PARENTCONNID,
            SNI_QUERY_CONN_SECPKG             = ::SNI_QUERY_CONN_SECPKG,
            SNI_QUERY_CONN_NETPACKETSIZE    = ::SNI_QUERY_CONN_NETPACKETSIZE,
            SNI_QUERY_CONN_NODENUM        = ::SNI_QUERY_CONN_NODENUM,
            SNI_QUERY_CONN_PACKETSRECD    = ::SNI_QUERY_CONN_PACKETSRECD,
            SNI_QUERY_CONN_PACKETSSENT    = ::SNI_QUERY_CONN_PACKETSSENT,
            SNI_QUERY_CONN_PEERADDR        = ::SNI_QUERY_CONN_PEERADDR,
            SNI_QUERY_CONN_PEERPORT        = ::SNI_QUERY_CONN_PEERPORT,
            SNI_QUERY_CONN_LASTREADTIME    = ::SNI_QUERY_CONN_LASTREADTIME,
            SNI_QUERY_CONN_LASTWRITETIME    = ::SNI_QUERY_CONN_LASTWRITETIME,
            SNI_QUERY_CONN_CONSUMER_ID    = ::SNI_QUERY_CONN_CONSUMER_ID,
            SNI_QUERY_CONN_CONNECTTIME    = ::SNI_QUERY_CONN_CONNECTTIME,
            SNI_QUERY_CONN_HTTPENDPOINT        = ::SNI_QUERY_CONN_HTTPENDPOINT,
            SNI_QUERY_CONN_LOCALADDR    = ::SNI_QUERY_CONN_LOCALADDR,
            SNI_QUERY_CONN_LOCALPORT    = ::SNI_QUERY_CONN_LOCALPORT,
            SNI_QUERY_CONN_SSLHANDSHAKESTATE= ::SNI_QUERY_CONN_SSLHANDSHAKESTATE,
            SNI_QUERY_CONN_SOBUFAUTOTUNING    = ::SNI_QUERY_CONN_SOBUFAUTOTUNING,
            SNI_QUERY_CONN_SECPKGNAME    = ::SNI_QUERY_CONN_SECPKGNAME,
            SNI_QUERY_CONN_SECPKGMUTUALAUTH    = ::SNI_QUERY_CONN_SECPKGMUTUALAUTH,
            SNI_QUERY_CONN_CONSUMERCONNID    = ::SNI_QUERY_CONN_CONSUMERCONNID,
            SNI_QUERY_CONN_SNIUCI        = ::SNI_QUERY_CONN_SNIUCI,
            SNI_QUERY_LOCALDB_HMODULE        = ::SNI_QUERY_LOCALDB_HMODULE,
            SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS = ::SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS,

        };

        enum  class ProviderEnum
        {
            HTTP_PROV       = ::HTTP_PROV,
            NP_PROV         = ::NP_PROV,
            SESSION_PROV    = ::SESSION_PROV,
            SIGN_PROV       = ::SIGN_PROV,
            SM_PROV         = ::SM_PROV,
            SMUX_PROV       = ::SMUX_PROV,
            SSL_PROV        = ::SSL_PROV,
            TCP_PROV        = ::TCP_PROV,
            VIA_PROV        = ::VIA_PROV,
            MAX_PROVS       = ::MAX_PROVS,
            INVALID_PROV    = ::INVALID_PROV
        };

        enum class IOType
        {
               READ  = ::SNI_Packet_Read,
               WRITE = ::SNI_Packet_Write
        };

        enum class ConsumerNumber
        {
            SNI_Consumer_SNI = ::SNI_Consumer_SNI,
            SNI_Consumer_SSB = ::SNI_Consumer_SSB,
            SNI_Consumer_PacketIsReleased = ::SNI_Consumer_PacketIsReleased,
            SNI_Consumer_Invalid = ::SNI_Consumer_Invalid
        };

        static const int SniMaxComposedSpnLength = ::SNI_MAX_COMPOSED_SPN;

        delegate void SqlAsyncCallbackDelegate(System::IntPtr ptr1, System::IntPtr ptr2, System::UInt32 num);

        [class:CLSCompliant(false)]
        ref class ConsumerInfo
        {
                internal:
                    System::Int32             defaultBufferSize;
                    SqlAsyncCallbackDelegate^ readDelegate;
                    SqlAsyncCallbackDelegate^ writeDelegate;
                    System::IntPtr            key;
        };

        [StructLayout(LayoutKind::Sequential, CharSet=CharSet::Unicode)]
        [class:CLSCompliant(false)]
        ref struct SNI_Error
        {
                internal:
                    ProviderEnum            provider;
                    cli::array<System::Char>^ errorMessage;
                    System::UInt32          nativeError;
                    System::UInt32          sniError;
                    //
                    // Debug Info - only present on debug bits.
                    //
                    System::String^         fileName;
                    System::String^         function;
                    System::UInt32          lineNumber;
         };

         enum class SniSpecialErrors
         {
             LocalDBErrorCode = (SNIE_LocalDB - SNI_STRING_ERROR_BASE),

             // multi-subnet-failover specific error codes
             MultiSubnetFailoverWithMoreThan64IPs = (SNIE_47 - SNI_STRING_ERROR_BASE),
             MultiSubnetFailoverWithInstanceSpecified = (SNIE_48 - SNI_STRING_ERROR_BASE),
             MultiSubnetFailoverWithNonTcpProtocol = (SNIE_49 - SNI_STRING_ERROR_BASE),

             // max error code value
             MaxErrorValue = SNIE_MAX
         };

     // SxS: although server information is network resource, the handle returned by this method is safe in SxS environment
     [ResourceExposure(ResourceScope::None)]
     [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
     static System::IntPtr SNIServerEnumOpen()
     {
        return System::IntPtr(::SNIServerEnumOpen (NULL, true));
     }

     [ResourceExposure(ResourceScope::None)]
     [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
     static System::Int32 SNIServerEnumRead(System::IntPtr handle,
                                           cli::array<System::Char>^ wStr,
                                           System::Int32 pcbBuf,
                                           System::Boolean %  fMore)
     {
         cli::pin_ptr<WCHAR>  pin_pWCh = &wStr[0];
         BOOL     localpfMore = static_cast<BOOL>(fMore);
         DWORD ret = ::SNIServerEnumRead ( static_cast<HANDLE>(handle.ToPointer()), pin_pWCh, pcbBuf, &localpfMore);
         fMore = localpfMore;
         return ret;
     }

    [ResourceExposure(ResourceScope::None)]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    static void SNIServerEnumClose(System::IntPtr handle)
    {
        // SQL BU DT 394619 - call through function pointer rather than directly to avoid autogen'ed 
        // PInvoke by CLR.  Autogen'ed PInvoke lacks reliability contract and stack walk will then allow
        // asynchronous exceptions to be injected.  The same stack walk will not allow injection on
        // function pointer in manner below.
        void (*g_SNIServerEnumClose)(HANDLE) = ::SNIServerEnumClose;

        return g_SNIServerEnumClose(static_cast<HANDLE>(handle.ToPointer()));    
    }


    [ResourceExposure(ResourceScope::None)]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    static System::UInt32 SNIClose(System::IntPtr pConn)    // Used in destucting a safe handle
    {
        // SQL BU DT 394619 - call through function pointer rather than directly to avoid autogen'ed 
        // PInvoke by CLR.  Autogen'ed PInvoke lacks reliability contract and stack walk will then allow
        // asynchronous exceptions to be injected.  The same stack walk will not allow injection on
        // function pointer in manner below.
        DWORD (*g_SNIClose)(SNI_Conn *) = ::SNIClose;

        SNI_ConnWrapper *local_pConn = static_cast<SNI_ConnWrapper *>(pConn.ToPointer());
        System::UInt32 ret = g_SNIClose(local_pConn->m_pConn);
        delete local_pConn;
        return ret;
    }

    [ResourceExposure(ResourceScope::None)]
    [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
    static System::UInt32 SNIInitialize()
    {
        return ::SNIInitialize(NULL);
    }

private:
    [ResourceExposure(ResourceScope::None)]
    static void MarshalConsumerInfo (ConsumerInfo^ consumerInfo, SNI_CONSUMER_INFO &native_consumerInfo)
    {
        native_consumerInfo.DefaultUserDataLength = consumerInfo->defaultBufferSize;
        native_consumerInfo.fnReadComp = static_cast<PIOCOMP_FN>((nullptr == consumerInfo->readDelegate) ? NULL : Marshal::GetFunctionPointerForDelegate (consumerInfo->readDelegate).ToPointer ());
        native_consumerInfo.fnWriteComp = static_cast<PIOCOMP_FN>((nullptr == consumerInfo->writeDelegate) ? NULL : Marshal::GetFunctionPointerForDelegate (consumerInfo->writeDelegate).ToPointer ());
        native_consumerInfo.ConsumerKey =  consumerInfo->key.ToPointer();
    }

    static DWORD (*SNICheckConnectionPtr)(SNI_Conn *) = ::SNICheckConnection;
    static DWORD (*SNIWriteAsyncWrapperPtr)(SNI_ConnWrapper*, SNI_Packet*) = ::SNIWriteAsyncWrapper;
    static DWORD (*SNIReadAsyncWrapperPtr)(SNI_ConnWrapper*, SNI_Packet**) = ::SNIReadAsyncWrapper;
    static SNI_Packet* (*SNIPacketAllocatePtr)(SNI_Conn*, SNI_Packet_IOType) = ::SNIPacketAllocate;
    static void (*SNIPacketReleasePtr)(SNI_Packet* pPacket) = ::SNIPacketRelease;
    static void (*SNIPacketResetPtr)(SNI_Conn*, SNI_Packet_IOType, SNI_Packet*, ConsumerNum) = ::SNIPacketReset;
    static DWORD (*SNIPacketGetDataWrapperPtr)(SNI_Packet*, BYTE*, DWORD, DWORD*) = ::SNIPacketGetDataWrapper;
    static void (*SNIPacketSetDataPtr)(SNI_Packet*, const BYTE*, DWORD) = ::SNIPacketSetData;

internal:
    // creates a new physical connection
    // SxS: from SxS point of view DB Resources are scoped as None.
    [ResourceExposure(ResourceScope::None)]
    [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
    static System::UInt32 SNIOpenSyncEx (
        ConsumerInfo^ consumerInfo,
        System::String^ constring,
        System::IntPtr% pConn,    // returns a handle to a safe handle
        cli::array<System::Byte>^ spnBuffer, // must be valid (non-null) if and only if integrated security if true
        cli::array<System::Byte>^ instanceName,
        System::Boolean fOverrideCache,
        System::Boolean fSync,
        System::Int32 timeout,
        System::Boolean fParallel,
        System::Int32 transparentNetworkResolutionStateNo,
        System::Int32 totalTimeout,
        System::Boolean isAzureSqlServerEndpoint)
    {
        ::SNI_CLIENT_CONSUMER_INFO clientConsumerInfo;  // native SNI_CLIENT_CONSUMER_INFO

        cli::pin_ptr<const WCHAR>  pin_constring =  PtrToStringChars (constring);
        Debug::Assert (NULL == pConn.ToPointer() , "Verrifying variable is really not initallized.");
        SNI_ConnWrapper*  local_pConn = NULL;
        cli::pin_ptr<System::Byte>   pin_spnBuffer = (spnBuffer == nullptr) ? static_cast<System::Byte*>(NULL) : &spnBuffer[0];
        cli::pin_ptr<System::Byte>   pin_instanceName = &instanceName[0];

        // initialize client ConsumerInfo part first
        MarshalConsumerInfo(consumerInfo, clientConsumerInfo.ConsumerInfo);

        clientConsumerInfo.wszConnectionString = pin_constring;
        clientConsumerInfo.networkLibrary = ::UNKNOWN_PREFIX;

        if (spnBuffer != nullptr) 
        {
            clientConsumerInfo.wszSPN = reinterpret_cast<LPWSTR>(pin_spnBuffer);
            clientConsumerInfo.cchSPN =  spnBuffer->Length;
        }
        // else leave null (SQL Auth)

        clientConsumerInfo.szInstanceName = reinterpret_cast<LPSTR>(pin_instanceName);
        clientConsumerInfo.cchInstanceName = instanceName->Length;
        clientConsumerInfo.fOverrideLastConnectCache = fOverrideCache;
        clientConsumerInfo.fSynchronousConnection = fSync;
        clientConsumerInfo.timeout = timeout;
        clientConsumerInfo.fParallel = fParallel;
        clientConsumerInfo.isAzureSqlServerEndpoint = isAzureSqlServerEndpoint;
		
        switch (transparentNetworkResolutionStateNo)
        {
        case (0):
            clientConsumerInfo.transparentNetworkResolution = DisabledMode;
            break;
        case (1):
            clientConsumerInfo.transparentNetworkResolution = SequentialMode;
            break;
        case (2):
            clientConsumerInfo.transparentNetworkResolution = ParallelMode;
            break;
        };
        clientConsumerInfo.totalTimeout = totalTimeout;

        System::UInt32 ret  =  ::SNIOpenSyncExWrapper (&clientConsumerInfo, &local_pConn);
        pConn =  static_cast<System::IntPtr>(local_pConn);
        return ret;
    }

    // creates a new MARS session
    // SxS: from SxS point of view DB Resources are scoped as None.
    [ResourceExposure(ResourceScope::None)]
    [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
    static  System::UInt32 SNIOpenMarsSession (ConsumerInfo^ consumerInfo,
                                    SafeHandle^ parent,
                                    System::IntPtr% pConn,                             // Used to initalize a safe handle
                                    System::Boolean fSync)
    {
        System::UInt32 ret = 0;
        ::SNI_CONSUMER_INFO native_consumerInfo;
        WCHAR wszMarsConString[] = L"session:";

        // initialize consumer info for MARS
        MarshalConsumerInfo(consumerInfo, native_consumerInfo);

        SNI_ConnWrapper*  local_pConn = NULL;

         // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
        bool mustRelease = false;
        RuntimeHelpers::PrepareConstrainedRegions();
        __try
        {
            parent->DangerousAddRef(mustRelease);
            Debug::Assert (mustRelease,"AddRef Failed!");

            ret = ::SNIOpenWrapper( &native_consumerInfo,
                             wszMarsConString,
                             static_cast<SNI_ConnWrapper*>(parent->DangerousGetHandle().ToPointer())->m_pConn,
                             &local_pConn,
                             fSync);
        }
        __finally
        {
             if (mustRelease)
             {
                parent->DangerousRelease();
             }
        }
        pConn =  static_cast<System::IntPtr>(local_pConn);     // return  connection ptr;
        return ret;
    }


    [ResourceExposure(ResourceScope::None)]
    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::MayFail)]
    static void SNIPacketAllocate(SafeHandle^ pConn,     // Used to initalize a safe handle
                                  IOType ioType,
                                  System::IntPtr % ret)
    {
            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");

                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer());                

                RuntimeHelpers::PrepareConstrainedRegions();
                __try {} __finally 
                { 
                    // SQL BU DT 401411 - call through function pointer rather than directly to avoid autogen'ed 
                    // PInvoke by CLR.  Autogen'ed PInvoke lacks reliability contract and stack walk will then allow
                    // asynchronous exceptions to be injected.  The same stack walk will not allow injection on
                    // function pointer in manner below.
                    
                    ret = static_cast<System::IntPtr>(SNIPacketAllocatePtr( local_pConn->m_pConn, static_cast<SNI_Packet_IOType>(ioType)));
                }
            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }     
    }
    
        // further optimization - peek and avoid call
        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNIPacketGetData (System::IntPtr packet, 
                                      cli::array<System::Byte>^ readBuffer,
                                      System::UInt32%   dataSize)
        {
            cli::pin_ptr<SNI_Packet>  pin_packet = static_cast<SNI_Packet*>(packet.ToPointer ());
            cli::pin_ptr<System::Byte> pin_readBuffer = &readBuffer[0];
            DWORD _dataSize = 0;
            DWORD ret = SNIPacketGetDataWrapperPtr(pin_packet, pin_readBuffer, readBuffer->Length, &_dataSize);
            dataSize = _dataSize;
            return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        static void SNIPacketReset (SafeHandle^ pConn,
                                    IOType ioType,
                                    SafeHandle^ packet,
                                    ConsumerNumber consNum)
        {
             // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool pConnMustRelease = false;
            bool packetMustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(pConnMustRelease);
                Debug::Assert (pConnMustRelease,"AddRef Failed!");
                packet->DangerousAddRef(packetMustRelease);
                Debug::Assert (packetMustRelease,"AddRef Failed!");
                SNI_ConnWrapper*  local_pConn = static_cast <SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                SNI_Packet*  local_packet = static_cast<SNI_Packet*>(packet->DangerousGetHandle().ToPointer ());
                SNIPacketResetPtr (local_pConn->m_pConn, static_cast<SNI_Packet_IOType>(ioType), local_packet, static_cast<ConsumerNum>(consNum));
            }
            __finally
            {
                if (pConnMustRelease)
                {
                    pConn->DangerousRelease();
                }
                if (packetMustRelease)
                {
                    packet->DangerousRelease();
                }
            }
        }

        [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
        [ResourceExposure(ResourceScope::None)]
        static void SNIPacketRelease ( System::IntPtr  packet)  // Releaseing safehandle base ptr
        {
            cli::pin_ptr<SNI_Packet> pin_packet = static_cast<SNI_Packet*>(packet.ToPointer ());

            // SQL BU DT 394619 - call through function pointer rather than directly to avoid autogen'ed 
            // PInvoke by CLR.  Autogen'ed PInvoke lacks reliability contract and stack walk will then allow
            // asynchronous exceptions to be injected.  The same stack walk will not allow injection on
            // function pointer in manner below.

            SNIPacketReleasePtr(pin_packet);
        }


        [ResourceExposure(ResourceScope::None)]
        //
        // Notes on SecureString: Writing out security sensitive information to managed buffer should be avoid as these can be moved
        //    around by GC. There are two set of information which falls into this category: passwords and new changed password which
        //    are passed in as SecureString by a user. Writing out clear passwords information is delayed until this layer to ensure that
        //    the information is written out to buffer which is pinned in this method already. This also ensures that processing a clear password
        //    is done right before it is written out to SNI_Packet where gets encrypted properly. 
        //    TdsParserStaticMethods.EncryptPassword operation is also done here to minimize the time the clear password is held in memory. Any changes
        //    to loose encryption algorithm is changed it should be done in both in this method as well as TdsParserStaticMethods.EncryptPassword.
        //  Up to current release, it is also guaranteed that both password and new change password will fit into a single login packet whose size is fixed to 4096
        //        So, there is no splitting logic is needed.
        static void SNIPacketSetData (SafeHandle^ packet,
                                      cli::array<System::Byte>^ data,
                                      System::Int32 length,
                                      cli::array<SecureString^>^ passwords,            // pointer to the passwords which need to be written out to SNI Packet
                                      cli::array<System::Int32>^ passwordOffsets    // Offset into data buffer where the password to be written out to
                                      )
        {
            Debug::Assert(passwords == nullptr || (passwordOffsets != nullptr && passwords->Length == passwordOffsets->Length), "The number of passwords does not match the number of password offsets");

            cli::pin_ptr<const BYTE>   pin_data = &data[0];

            bool mustRelease = false;
            bool mustClearBuffer = false;
            IntPtr clearPassword = IntPtr::Zero;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                if (passwords != nullptr)
                {
                    // Process SecureString
                    for(int i = 0; i < passwords->Length; ++i)
                    {
                        // SecureString is used
                        if (passwords[i] != nullptr)
                        {
                            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
                            RuntimeHelpers::PrepareConstrainedRegions();
                            __try
                            {
                                // ==========================================================================
                                //  Get the clear text of secure string without converting it to String type
                                // ==========================================================================
                                clearPassword = Marshal::SecureStringToCoTaskMemUnicode(passwords[i]);

                                // ==========================================================================================================================
                                //  Losely encrypt the clear text - The encryption algorithm should exactly match the TdsParserStaticMethods.EncryptPassword
                                // ==========================================================================================================================
                        
                                WCHAR *pwChar = static_cast<WCHAR*> (clearPassword.ToPointer());
                                byte *pByte = static_cast<byte*> (clearPassword.ToPointer());

                                int s;
                                byte bLo;
                                byte bHi;
                                int passwordsLength = passwords[i]->Length;
                                for (int j = 0; j < passwordsLength; ++j)
                                {
                                    s = (int) *pwChar;
                                    bLo = (byte) (s & 0xff);
                                    bHi = (byte) ((s >> 8) & 0xff);
                                    *(pByte++) = (Byte) ( (((bLo & 0x0f) << 4) | (bLo >> 4)) ^  0xa5 );
                                    *(pByte++) = (Byte) ( (((bHi & 0x0f) << 4) | (bHi >> 4)) ^  0xa5);
                                    ++pwChar;
                                }

                                // ===========================================================
                                //  Write out the losely encrypted passwords to data buffer
                                // ===========================================================
                                mustClearBuffer = true;
                                Marshal::Copy(clearPassword, data, passwordOffsets[i], passwordsLength * 2);
                            }
                            finally
                            {
                                // Make sure that we clear the security sensitive information
                                if (clearPassword != IntPtr::Zero)
                                {
                                    Marshal::ZeroFreeCoTaskMemUnicode(clearPassword);
                                }
                            }
                        }
                    }
                }

                packet->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");
                SNI_Packet*   local_packet = static_cast<SNI_Packet*>(packet->DangerousGetHandle().ToPointer ());
                SNIPacketSetDataPtr (local_packet, pin_data, length);
            }
            __finally
            {
                if (mustRelease)
                {
                    packet->DangerousRelease();
                }

                // Make sure that we clear the security sensitive information
                // data->Initialize() is not safe to call under CER
                if (mustClearBuffer)
                {
                    for (int i = 0; i < data->Length; ++i)
                    {
                        data[i] = 0;
                    }
                }
            }
        }

        // SxS: this method queries registry for SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE and SNI_QUERY_SERVER_ENCRYPT_POSSIBLE
        [ResourceExposure(ResourceScope::None)]
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static int SNIQueryInfo (QTypes qType,
                                  System::UInt32% qInfo)
        {
            DWORD _qInfo  = static_cast<DWORD>(qInfo);
            DWORD _qType = static_cast<DWORD>(qType);

            // we should not be calling this method with SNI_QUERY_CERTIFICATE or unsupported qType
            Debug::Assert(_qType == SNI_QUERY_CLIENT_ENCRYPT_POSSIBLE || _qType == SNI_QUERY_SERVER_ENCRYPT_POSSIBLE || _qType == SNI_QUERY_TCP_SKIP_IO_COMPLETION_ON_SUCCESS, "qType is unsupported or unknown");
            int result=::SNIQueryInfo ( _qType,  &_qInfo);
            qInfo = _qInfo;
            return result;
        }

        // SxS: this method queries registry for LocalDB dll, loads it and returns handle (for SNI_QUERY_LOCALDB_INFO)
        [ResourceExposure(ResourceScope::None)]
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static int SNIQueryInfo (QTypes qType,
                                  System::IntPtr% qInfo)
        {
            DWORD _qType = static_cast<DWORD>(qType);
             // we should not be calling this method with unsupported qType
            Debug::Assert(_qType == SNI_QUERY_LOCALDB_HMODULE , "qType is unsupported or unknown");

            pin_ptr<System::IntPtr> pInfo= &qInfo ;            
            return ::SNIQueryInfo ( _qType,  pInfo); 
        }

        [ResourceExposure(ResourceScope::None)] // SxS: connection settings
        static System::UInt32 SNISetInfo (SafeHandle^  pConn,
                                          QTypes qtype,
                                          System::UInt32 % qInfo)
        {
            System::UInt32 ret;
            DWORD  local_qInfo = static_cast<DWORD>(qInfo);

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
               pConn->DangerousAddRef(mustRelease);
               Debug::Assert (mustRelease,"AddRef Failed!");
               SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer());
               ret = ::SNISetInfo (local_pConn->m_pConn, static_cast<DWORD>(qtype), &local_qInfo);

            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }
            qInfo = local_qInfo;
            return ret;
        }


        [ResourceExposure(ResourceScope::None)] // SxS: connection settings
        static System::UInt32 SniGetConnectionId (SafeHandle^  pConn, System::Guid % connId)
        {
            System::UInt32 ret;
            DWORD local_qInfo = static_cast<DWORD>(SNI_QUERY_CONN_CONNID);
            GUID local_connectionId;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
               pConn->DangerousAddRef(mustRelease);
               Debug::Assert (mustRelease,"AddRef Failed!");
               SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer());
               ret = ::SNIGetInfo (local_pConn->m_pConn, local_qInfo, &local_connectionId);

               if (ERROR_SUCCESS == ret)
               {
                   connId = FromGUID(local_connectionId);
               }
            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }

            return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNIReadAsync (SafeHandle^  pConn,
                                            System::IntPtr%  packet)
        {
            SNI_Packet*  local_packet = NULL;
            System::UInt32 ret;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");
                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                ret = SNIReadAsyncWrapperPtr ( local_pConn, &local_packet);

            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }
            packet = static_cast<System::IntPtr>(local_packet);
            return ret;
        }
                
        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNIReadSyncOverAsync (SafeHandle^  pConn,
                                                    System::IntPtr% packet,
                                                    Int32 timeout)
        {
           SNI_Packet*  local_packet = NULL;
           System::UInt32 ret;

           // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
           bool mustRelease = false;
           RuntimeHelpers::PrepareConstrainedRegions();
           __try
           {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");
                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                // Need to call SyncOverAsync via PInvoke (instead of a pointer) such that the CLR notifies our hoster (e.g. SQLCLR) that we are doing a managed\native transition
                ret = ::SNIReadSyncOverAsync ( local_pConn, &local_packet, timeout);
           }
           __finally
           {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
           }
           packet = static_cast<System::IntPtr>(local_packet);
           return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNICheckConnection (SafeHandle^  pConn)
        {
           // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
           bool mustRelease = false;
           RuntimeHelpers::PrepareConstrainedRegions();
           __try
           {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");
                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer());
                return SNICheckConnectionPtr ( local_pConn->m_pConn );
           }
           __finally
           {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
           }
        }

        [ResourceExposure(ResourceScope::None)]
        [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
        static System::UInt32 SNITerminate ()
        {
            // SQL BU DT 394619 - call through function pointer rather than directly to avoid autogen'ed 
            // PInvoke by CLR.  Autogen'ed PInvoke lacks reliability contract and stack walk will then allow
            // asynchronous exceptions to be injected.  The same stack walk will not allow injection on
            // function pointer in manner below.
            DWORD (*g_SNITerminate)() = ::SNITerminate;
        
            return g_SNITerminate();
        }

        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNIWritePacket (SafeHandle^  pConn,
                                             SafeHandle^  packet,
                                             bool sync)
        {
            System::UInt32 ret;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool pConnMustRelease = false;
            bool packetMustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(pConnMustRelease);
                Debug::Assert (pConnMustRelease,"AddRef Failed!");
                packet->DangerousAddRef(packetMustRelease);
                Debug::Assert (packetMustRelease,"AddRef Failed!");


                SNI_ConnWrapper*  local_pConn =    static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                SNI_Packet*  local_packet =  static_cast<SNI_Packet*>(packet->DangerousGetHandle().ToPointer ());

                if (sync) {
                    // Need to call SyncOverAsync via PInvoke (instead of a pointer) such that the CLR notifies our hoster (e.g. SQLCLR) that we are doing a managed\native transition
                    return ::SNIWriteSyncOverAsync(local_pConn, local_packet);
                }
                else {
                    return SNIWriteAsyncWrapperPtr(local_pConn, local_packet);
                }
            }
            __finally
            {
                if (pConnMustRelease)
                {
                    pConn->DangerousRelease();
                }
                if (packetMustRelease)
                {
                    packet->DangerousRelease();
                }
            }
            return ret;
        }

        [ResourceExposure(ResourceScope::None)]        
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static System::UInt32 SNIAddProvider (SafeHandle^  pConn,
                                              ProviderEnum providerEnum,
                                              System::UInt32% info)
        {
            DWORD _info =  info;
            System::UInt32 ret;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");

                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                ret = ::SNIAddProvider ( local_pConn->m_pConn, static_cast<ProviderNum>(providerEnum), &_info);

                if (ret == ERROR_SUCCESS) 
                {
                    // added a provider, need to requery for sync over async support
                    BOOL fSupportsSyncOverAsync;
                    ret = SNIGetInfo(local_pConn->m_pConn, SNI_QUERY_CONN_SUPPORTS_SYNC_OVER_ASYNC, &fSupportsSyncOverAsync);
                    Debug::Assert(ret == ERROR_SUCCESS, "SNIGetInfo cannot fail with this QType");

                    // convert BOOL to bool
                    local_pConn->m_fSupportsSyncOverAsync = !!fSupportsSyncOverAsync;
                }
            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }
            info = _info;
            return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        static System::UInt32 SNIRemoveProvider (SafeHandle^ pConn,
                                                 ProviderEnum providerEnum)
        {
            System::UInt32 ret;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");

                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                ret = ::SNIRemoveProvider ( local_pConn->m_pConn, static_cast<ProviderNum>(providerEnum));

            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }
            return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        static void SNIGetLastError (SNI_Error^ error)
        {
               ::SNI_ERROR   local_error;
               ::SNIGetLastError (&local_error);

               const int SizeOfReturnErrorMsg = ARRAYSIZE (local_error.pszErrorMessage);

               // Verrify that the size of the mananged buffer is at least as big as we think it should be
               COMPILE_TIME_ASSERT (ARRAYSIZE (local_error.pszErrorMessage) >= MAX_PATH+1);
               error->provider = static_cast<ProviderEnum>(local_error.Provider);
               error->errorMessage = gcnew cli::array<System::Char>(SizeOfReturnErrorMsg);
               for (int x = 0; x < MAX_PATH+1; ++x)
               {  error->errorMessage[x] = local_error.pszErrorMessage[x]; }

               error->nativeError = local_error.dwNativeError;
               error->sniError  = local_error.dwSNIError;

               error->fileName = Marshal::PtrToStringUni (static_cast<System::IntPtr>(local_error.pszFileName));
               error->function  = Marshal::PtrToStringUni (static_cast<System::IntPtr>(local_error.pszFunction));
               error->lineNumber = local_error.dwLineNumber;
        }

        // SNI functions used for SSPI security...
        [ResourceExposure(ResourceScope::None)]
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static System::UInt32 SNISecInitPackage (System::UInt32 %  maxLength)
        {
            DWORD local_maxLength = static_cast<System::UInt32>(maxLength);
            DWORD ret = ::SNISecInitPackage (&local_maxLength);
            maxLength = local_maxLength;
            return ret;
        }

        [ResourceExposure(ResourceScope::None)]
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static System::UInt32 SNISecGenClientContext (SafeHandle^ pConnectionObject,
                                                      cli::array<System::Byte>^ inBuff,
                                                      System::UInt32 receivedLength,
                                                      cli::array<System::Byte>^ OutBuff,
                                                      System::UInt32% sendLength,
                                                      cli::array<System::Byte>^  serverUserName)
        {
            DWORD ret;
            DWORD local_outBuffLen = sendLength;
            BOOL local_fDone;

            cli::pin_ptr<BYTE>   pin_inBuff =  inBuff == nullptr ? static_cast<BYTE*>(NULL) : &inBuff[0];
            cli::pin_ptr<BYTE>   pin_outBuff = &OutBuff[0];
            cli::pin_ptr<BYTE>   pin_serverUserName =  &serverUserName[0];

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConnectionObject->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");

                SNI_ConnWrapper*   local_pConnectionObject = static_cast<SNI_ConnWrapper*>(pConnectionObject->DangerousGetHandle().ToPointer ());
                ret = ::SNISecGenClientContext ( local_pConnectionObject->m_pConn,
                                                 pin_inBuff,
                                                 receivedLength,
                                                 pin_outBuff,
                                                 &local_outBuffLen,
                                                 &local_fDone,                                                 
                                                 reinterpret_cast<WCHAR*>(pin_serverUserName),
                                                 serverUserName == nullptr ? 0 : serverUserName->Length,   //cbServerInfo,
                                                 NULL,
                                                 NULL);

            }
            __finally
            {
                if (mustRelease)
                {
                    pConnectionObject->DangerousRelease();
                }
            }
            sendLength = local_outBuffLen;
            return ret;
        }
                
        [ResourceExposure(ResourceScope::None)]        
        [ResourceConsumption(ResourceScope::Machine, ResourceScope::Machine)]
        static System::UInt32 SNIWaitForSSLHandshakeToComplete( SafeHandle^ pConn, 
                                                                System::Int32 timeoutMilliseconds )
        {
            System::UInt32 ret;

            // provides a guaranteed finally block – without this it isn’t guaranteed – non interruptable by fatal exceptions
            bool mustRelease = false;
            RuntimeHelpers::PrepareConstrainedRegions();
            __try
            {
                pConn->DangerousAddRef(mustRelease);
                Debug::Assert (mustRelease,"AddRef Failed!");

                SNI_ConnWrapper*  local_pConn = static_cast<SNI_ConnWrapper*>(pConn->DangerousGetHandle().ToPointer ());
                ret = ::SNIWaitForSSLHandshakeToComplete (local_pConn->m_pConn, (DWORD)timeoutMilliseconds);
            }
            __finally
            {
                if (mustRelease)
                {
                    pConn->DangerousRelease();
                }
            }
            
            return ret;
        }
};


// This class is used to validate the native structures haven't changed from what
// we think they are.  If they have changed then we will need to update the managed classes above to reflect
// the change and update the structs below.
class STRUCTSNAPSHOT
{
private:
        typedef struct SNI_ERROR
        {
            ProviderNum Provider;
            WCHAR   pszErrorMessage[MAX_PATH+1];
            DWORD   dwNativeError;
            DWORD   dwSNIError;
            WCHAR * pszFileName;
            WCHAR * pszFunction;
            DWORD   dwLineNumber;
        };

#pragma warning(disable:4101)
        static void VerrifyStateAtCompileTime ()
        {
           // Verrify Managed / Native Class Structures
           // If a violation is seen here, we will need to update our managed structures
           // becouse the native structures are different from what we think they should be.

           // Now Verrify the size of the internal components..
           // This checks that nothing was removed or changed in size, changes in type are NOT checked
           {
               STRUCTSNAPSHOT::SNI_ERROR SNIErrorSnapShot;
               ::SNI_ERROR SNIError;
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.Provider) == sizeof(SNIError.Provider));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.pszErrorMessage) == sizeof(SNIError.pszErrorMessage));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.dwNativeError) == sizeof(SNIError.dwNativeError));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.dwSNIError) == sizeof(SNIError.dwSNIError));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.pszFileName) == sizeof(SNIError.pszFileName));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.pszFunction) == sizeof(SNIError.pszFunction));
               COMPILE_TIME_ASSERT (sizeof (SNIErrorSnapShot.dwLineNumber) == sizeof(SNIError.dwLineNumber));
           }
           {
               ::SNI_CLIENT_CONSUMER_INFO SniClientConsumerInfo;

               COMPILE_TIME_ASSERT (sizeof (int) == sizeof(SniClientConsumerInfo.ConsumerInfo.DefaultUserDataLength));
               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.ConsumerInfo.ConsumerKey));
               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.ConsumerInfo.fnReadComp));
               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.ConsumerInfo.fnWriteComp));

               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.wszConnectionString));
               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.wszSPN));
               COMPILE_TIME_ASSERT (sizeof(int) == sizeof(SniClientConsumerInfo.cchSPN));
               COMPILE_TIME_ASSERT (sizeof(void*) == sizeof(SniClientConsumerInfo.szInstanceName));
               COMPILE_TIME_ASSERT (sizeof(int) == sizeof(SniClientConsumerInfo.cchInstanceName));
               COMPILE_TIME_ASSERT (sizeof(BOOL) == sizeof(SniClientConsumerInfo.fOverrideLastConnectCache));
               COMPILE_TIME_ASSERT (sizeof(BOOL) == sizeof(SniClientConsumerInfo.fSynchronousConnection));
               COMPILE_TIME_ASSERT (sizeof(int) == sizeof(SniClientConsumerInfo.timeout));
               COMPILE_TIME_ASSERT (sizeof(BOOL) == sizeof(SniClientConsumerInfo.fParallel));
               COMPILE_TIME_ASSERT(sizeof(BYTE) == sizeof(SniClientConsumerInfo.transparentNetworkResolution));
               COMPILE_TIME_ASSERT (sizeof(int) == sizeof(SniClientConsumerInfo.totalTimeout));
           }

           // This check makes sure that nothing was added
           COMPILE_TIME_ASSERT (sizeof (STRUCTSNAPSHOT::SNI_ERROR) == sizeof(::SNI_ERROR));
        }
#pragma warning(default:4101)
};

[class:CLSCompliant(false)]
ref class  Win32NativeMethods
{

internal:

    [ResourceExposure(ResourceScope::None)]
    static bool IsTokenRestrictedWrapper(System::IntPtr token)
    {
        BOOL isRestricted = FALSE;

        DWORD win32Error = ::UnmanagedIsTokenRestricted(static_cast<HANDLE>(token.ToPointer()), &isRestricted);

        if (0 != win32Error) {
            Marshal::ThrowExceptionForHR(HRESULT_FROM_WIN32(win32Error)); // will only throw if (hresult < 0)
        }

        return !!isRestricted; // convert BOOL to bool
    }
};

[class:CLSCompliant(false)]
ref class  NativeOledbWrapper
{
    internal :

    // used by C#
    const static int SizeOfPROPVARIANT = sizeof(PROPVARIANT);

    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    [ResourceExposure(ResourceScope::None)]
    static HRESULT IChapteredRowsetReleaseChapter(System::IntPtr ptr, System::IntPtr chapter)
    {
        HRESULT hr = E_UNEXPECTED;
        DBREFCOUNT refcount = 0;
        HCHAPTER hchapter = reinterpret_cast<HCHAPTER>(chapter.ToPointer());
        IChapteredRowset *chapteredRowset = NULL;
        IUnknown *unknown = static_cast<IUnknown*>(ptr.ToPointer());
        RuntimeHelpers::PrepareConstrainedRegions();
        __try { }
        __finally
        {
            hr = unknown->QueryInterface(IID_IChapteredRowset, reinterpret_cast<void**>(&chapteredRowset));
            if (NULL != chapteredRowset)
            {
                hr = chapteredRowset->ReleaseChapter(hchapter, &refcount);
                chapteredRowset->Release();
            }
        }
        return hr;
    }

    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    [ResourceExposure(ResourceScope::None)]
    static HRESULT ITransactionAbort(System::IntPtr ptr)
    {
        HRESULT hr = E_UNEXPECTED;
        ITransactionLocal *transaction = NULL;
        IUnknown *unknown = static_cast<IUnknown*>(ptr.ToPointer());
        RuntimeHelpers::PrepareConstrainedRegions();
        __try { }
        __finally
        {
            hr = unknown->QueryInterface(IID_ITransactionLocal, reinterpret_cast<void**>(&transaction));
            if (NULL != transaction)
            {
                hr = transaction->Abort(NULL, 0, 0);
                transaction->Release();
            }
        }
        return hr;
    }

    [ReliabilityContract(Consistency::WillNotCorruptState, Cer::Success)]
    [ResourceExposure(ResourceScope::None)]
    static HRESULT ITransactionCommit(System::IntPtr ptr)
    {
        HRESULT hr = E_UNEXPECTED;
        ITransactionLocal *transaction = NULL;
        IUnknown *unknown = static_cast<IUnknown*>(ptr.ToPointer());
        RuntimeHelpers::PrepareConstrainedRegions();
        __try { }
        __finally
        {
            hr = unknown->QueryInterface(IID_ITransactionLocal, reinterpret_cast<void**>(&transaction));
            if (NULL != transaction)
            {
                hr = transaction->Commit(FALSE, XACTTC_SYNC_PHASETWO, 0);
                transaction->Release();
            }
        }
        return hr;
    }

    [ResourceExposure(ResourceScope::None)]
    static bool MemoryCompare(System::IntPtr buf1, System::IntPtr buf2, System::Int32 count)
    {
        Debug::Assert (buf1 != buf2, "buf1 and buf2 are the same");
        Debug::Assert (buf1.ToInt64() < buf2.ToInt64() || buf2.ToInt64() + count <= buf1.ToInt64(), "overlapping region buf1");
        Debug::Assert (buf2.ToInt64() < buf1.ToInt64() || buf1.ToInt64() + count <= buf2.ToInt64(), "overlapping region buf2");
        Debug::Assert (0 <= count, "negative count");

        int result = memcmp(buf1.ToPointer(), buf2.ToPointer(), static_cast<size_t>(count));
        return (0 != result);
    }

    [ResourceExposure(ResourceScope::None)]
    static void MemoryCopy (System::IntPtr dst, System::IntPtr src, System::Int32 count)
    {
        Debug::Assert (dst != src, "dst and src are the same");
        Debug::Assert (dst.ToInt64() < src.ToInt64() || src.ToInt64() + count <= dst.ToInt64(), "overlapping region dst");
        Debug::Assert (src.ToInt64() < dst.ToInt64() || dst.ToInt64() + count <= src.ToInt64(), "overlapping region src");
        Debug::Assert (0 <= count, "negative count");

        memcpy(dst.ToPointer(), src.ToPointer(), static_cast<size_t>(count));
    }

#if 0
    // pending approval (to use managed C++ code and link to ole32.dll, oleaut32.dll) instead of implementing in C#
    static void FreePropVariant(System::IntPtr memptr, int byteOffset)
    {
        // two contigous PROPVARIANT structures that need to be freed
        // the second should only be freed if different from the first
        COMPILE_TIME_ASSERT (0 == (sizeof(PROPVARIANT) % 8));
        Debug::Assert (0 == (memptr.ToInt64() + byteOffset) % 8, "unexpected unaligned ptr offset");

        PROPVARIANT *ptr1 = reinterpret_cast<PROPVARIANT*>(static_cast<byte*>(memptr.ToPointer()) + byteOffset);
        PROPVARIANT *ptr2 = reinterpret_cast<PROPVARIANT*>(static_cast<byte*>(memptr.ToPointer()) + byteOffset + sizeof(PROPVARIANT));
        RuntimeHelpers::PrepareConstrainedRegions();
        __try { }
        __finally
        {
            if (0 != memcmp(ptr1, ptr2, sizeof(PROPVARIANT)))
            {
                // second structure different from the first
                PropVariantClear(ptr2);
            }
            else
            {
                // second structure same as the first, just clear the field
                PropVariantInit(ptr2);
            }
            // always clear the first structure
            PropVariantClear(ptr1);
        }
    }

    static void FreeVariant(System::IntPtr memptr, int byteOffset)
    {
        // two contigous VARIANT structures that need to be freed
        // the second should only be freed if different from the first
        COMPILE_TIME_ASSERT (0 == (sizeof(VARIANT) % 8));
        Debug::Assert (0 == (memptr.ToInt64() + byteOffset) % 8, "unexpected unaligned ptr offset");

        VARIANTARG *ptr1 = reinterpret_cast<VARIANTARG*>(static_cast<byte*>(memptr.ToPointer()) + byteOffset);
        VARIANTARG *ptr2 = reinterpret_cast<VARIANTARG*>(static_cast<byte*>(memptr.ToPointer()) + byteOffset + sizeof(VARIANT));
        __try { }
        __finally
        {
            if (0 != memcmp(ptr1, ptr2, sizeof(VARIANT)))
            {
                // second structure different from the first
                VariantClear(ptr2);
            }
            else
            {
                // second structure same as the first, just clear the field
                VariantInit(ptr2);
            }
            // always clear the first structure
            VariantClear(ptr1);
        }
    }
#endif
};

ref class AdalException : public System::Exception
{
private:
    // Internal Adal error category used in retry logic and building error message in managed code
    initonly unsigned int _category;
    // Public facin failing status returned from Adal APIs in SNISecADALGetAccessToken
    initonly unsigned int _status;
    // Internal last Adal API called in SNISecADALGetAccessToken for troubleshooting
    initonly unsigned int _state;

internal:
    AdalException(String^ message, unsigned int category, unsigned int status, unsigned int state) : System::Exception(message)
    {
        _category = category;
        _status = status;
        _state = state;
    }

    unsigned int GetCategory() 
    { 
        return _category; 
    }

    unsigned int GetStatus() 
    { 
        return _status; 
    }

    unsigned int GetState()
    {
        return _state;
    }
};

ref class ADALNativeWrapper
{
private:
    static inline
    GUID ToGUID(System::Guid^ guid) // This is from msdn http://msdn.microsoft.com/en-us/library/wb8scw8f.aspx.
    {
        array<Byte>^ guidData = guid->ToByteArray();
        pin_ptr<Byte> data = &(guidData[0]);

        return *(GUID *)data;
    }

internal:
    static int ADALInitialize()
    {
        return SNISecADALInitialize();
    }

    static array<byte>^ ADALGetAccessToken( String^ username,
                                            SecureString^ password,
                                            String^ stsURL,
                                            String^ servicePrincipalName,
                                            System::Guid^ correlationId,
                                            String^ clientId,
                                            System::Int64% fileTime)
    {
        Debug::Assert(password != nullptr, "Password from SecureString is null.");

        IntPtr clearPassword = IntPtr::Zero;

        try
        {
            clearPassword = Marshal::SecureStringToGlobalAllocUnicode(password);
            Debug::Assert(clearPassword != IntPtr::Zero, "clearPassword is Intptr::Zero.");

            array<byte>^ result = ADALGetAccessToken( username,
                                                      clearPassword,
                                                      stsURL,
                                                      servicePrincipalName,
                                                      correlationId,
                                                      clientId,
                                                      false /*fWindowsIntegrated*/,
                                                      fileTime);

            return result;
        }
        finally 
        {
            if (clearPassword != IntPtr::Zero) 
            {
                Marshal::ZeroFreeGlobalAllocUnicode(clearPassword);
            }
        }
    }

    static array<byte>^ ADALGetAccessToken( String^ username,
                                            String^ password,
                                            String^ stsURL,
                                            String^ servicePrincipalName,
                                            System::Guid^ correlationId,
                                            String^ clientId,
                                            System::Int64% fileTime)
    {
        Debug::Assert(password != nullptr, "Password is null.");

        IntPtr clearPassword = IntPtr::Zero;

        try 
        {
            clearPassword = Marshal::StringToHGlobalUni(password);

            array<byte>^ result = ADALGetAccessToken( username,
                                                      clearPassword,
                                                      stsURL,
                                                      servicePrincipalName,
                                                      correlationId,
                                                      clientId,
                                                      false /*fWindowsIntegrated*/,
                                                      fileTime);

            return result;
        }
        finally 
        {
            if (clearPassword != IntPtr::Zero) 
            {
                Marshal::FreeHGlobal(clearPassword);
            }
        }
    }

    // The version of API that performs windows integrated authentication.
    static array<byte>^ ADALGetAccessTokenForWindowsIntegrated( String^ stsURL,
                                                                String^ servicePrincipalName,
                                                                System::Guid^ correlationId,
                                                                String^ clientId,
                                                                System::Int64% fileTime)
    {
        return ADALGetAccessToken( nullptr /*username*/,
                                   IntPtr::Zero /*password*/,
                                   stsURL,
                                   servicePrincipalName,
                                   correlationId,
                                   clientId,
                                   true /*fWindowsIntegrated*/,
                                   fileTime);
    }

private:
    static array<byte>^ ADALGetAccessToken( String^ username,
                                            IntPtr password,
                                            String^ stsURL,
                                            String^ servicePrincipalName,
                                            System::Guid^ correlationId,
                                            String^ clientId,
                                            const bool& fWindowsIntegrated,
                                            System::Int64% fileTime)
    {
        Debug::Assert(username != nullptr || fWindowsIntegrated, "User name is null and its not windows integrated authentication.");
        Debug::Assert(password != IntPtr::Zero || fWindowsIntegrated, "Password is null and its not windows integrated authentication.");
        Debug::Assert(stsURL != nullptr, "stsURL is null.");
        
        Debug::Assert(servicePrincipalName != nullptr, "ServicePrincipalName is null.");
        Debug::Assert(clientId != nullptr, "Ado ClientId is null.");
        
        Debug::Assert(correlationId != Guid::Empty, "CorrelationId is Guid::Empty.");

        pin_ptr<const wchar_t> pUsername = nullptr; 
        const wchar_t* pPassword = nullptr;

        if (!fWindowsIntegrated)
        {
            pUsername = PtrToStringChars(username);
            pPassword = static_cast<wchar_t*>(password.ToPointer());
        }

        pin_ptr<const wchar_t> pStsURL = PtrToStringChars(stsURL);
        pin_ptr<const wchar_t> pServicePrincipalName = PtrToStringChars(servicePrincipalName);
        pin_ptr<const wchar_t> pClientId = PtrToStringChars(clientId);
        
        LPWSTR pToken = nullptr;
        LPWSTR pErrorDescription = nullptr;
        DWORD cbToken = 0, errorDescriptionLength = 0;
        DWORD adalStatus = ERROR_SUCCESS;
        DWORD errorState = 0;
        _FILETIME fileTimeLocal = {0};

        GUID correlationIdGUID = ToGUID(correlationId);
        try
        {
            DWORD statusCategory = SNISecADALGetAccessToken(pUsername,
                                                            pPassword,
                                                            pStsURL,
                                                            pServicePrincipalName,
                                                            correlationIdGUID,
                                                            pClientId,
                                                            fWindowsIntegrated,
                                                            &pToken,
                                                            cbToken,
                                                            &pErrorDescription,
                                                            errorDescriptionLength,
                                                            adalStatus,
                                                            errorState,
                                                            fileTimeLocal);

            if (statusCategory == 0)
            {
                Debug::Assert(pToken != nullptr, "pToken is null.");
                Debug::Assert(cbToken > 0, "token length is less than or equal to 0.");
                Debug::Assert(pErrorDescription == nullptr, "pErrorDescription is not null");
                Debug::Assert(errorDescriptionLength == 0, "ErrorDescription length is not 0.");

                IntPtr ptrToken = (IntPtr)pToken;

                array<byte>^ result = gcnew array<byte>(cbToken);
                Marshal::Copy(ptrToken, result, 0, cbToken);

                fileTime = (((ULONGLONG)fileTimeLocal.dwHighDateTime) << 32) + fileTimeLocal.dwLowDateTime;
                return result;
            }
            else
            {
                Debug::Assert(pToken == nullptr, "pToken is not null in error case.");
                Debug::Assert(cbToken == 0, "Token length is not 0 in error case.");

                String^ errorString = String::Empty;
                if (pErrorDescription != nullptr && errorDescriptionLength >= 0)
                {
                    errorString = Marshal::PtrToStringUni((IntPtr)pErrorDescription, errorDescriptionLength);
                }
                throw gcnew AdalException(errorString, statusCategory, adalStatus, errorState);
            }
        }
        finally 
        {
            if (pToken != nullptr) 
            {
                delete[] pToken;
            }
            if (pErrorDescription != nullptr)
            {
                delete[] pErrorDescription;
            }
        }
    }
};
