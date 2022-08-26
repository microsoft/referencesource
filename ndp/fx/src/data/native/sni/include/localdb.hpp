//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: localDB.hpp
// @Owner: see SQLDevDash
// @Test: 
//
// <owner current="true" primary="true">see SQLDevDash</owner>
//
// Purpose: SNI LocalDB Provider
//
// Notes:
//          
// @EndHeader@
//****************************************************************************

///////////////////////////////////////////////////////////////////
//
//  The constants and the function pointers are defined in sqluserInstance.h
//  They are duplicated here to avoid dependency on a header file outside SNI 
//
///////////////////////////////////////////////////////////////////

#define CONNECT_MAX 260

// Truncate flag used to singnal LocalDBFormatMessage() to truncate
// error message in case the buffer size is not sufficient.
//
#define LOCALDB_TRUNCATE_ERR_MESSAGE        0x0001L

typedef HRESULT __cdecl
FnLocalDBStartInstance(
        __in                PCWSTR  pInstanceName,
        __in                DWORD   dwFlags,
        __out               LPWSTR  wszSqlConnection,
        __inout             LPDWORD lpcchSqlConnection
);

typedef HRESULT __cdecl
FnLocalDBFormatMessage(
            __in        HRESULT     hrLocalDB,
            __in        DWORD       dwFlags,
            __in        DWORD       dwLanguageId,
            __out       LPWSTR      wszMessage,
            __inout     LPDWORD     lpcchMessage
);

class LocalDB 
{

private:    

    static LocalDB* s_pInstance;

	// PRIVATE: Creating the singleton LocalDB object must be done only through the static
	// method FInitialize().
    LocalDB()
    {
        m_hUserInstanceDll = NULL;
        m_pfnLocalDBStartInstance = NULL;
        m_pfnLocalDBFormatMessage = NULL;
		m_CS = NULL;
    };

	// PRIVATE: Destroying the singleton LocalDB object must be done only through the static
	// method Terminate().
	~LocalDB()
	{
		// no op
	};   

	// PRIVATE: Make sure we don't break the singleton pattern unintentionally
	LocalDB(const LocalDB &)
	{
		// no op
	};

	// PRIVATE: Make sure we don't break the singleton pattern unintentionally
	LocalDB& operator=(const LocalDB &)
	{
		// no op
	}

public:

	volatile HMODULE m_hUserInstanceDll;

    FnLocalDBFormatMessage* m_pfnLocalDBFormatMessage;

    FnLocalDBStartInstance* m_pfnLocalDBStartInstance;

	SNICritSec* m_CS;

    // Returns a Singleton Instance
    //
    static DWORD LDBInstance(__out_opt LocalDB** ppLdbInstnace);

	static void Terminate();

	DWORD FInitialize();
    
    DWORD loadUserInstanceDll();

    DWORD getLocalDBConnectionString( __in LPCWSTR wszInstanceName,
                                           __out_ecount(pcchLocalDBConnectBuf) LPWSTR wszlocalDBConnect,
                                           __inout LPDWORD pcchLocalDBConnectBuf);

    static DWORD __cdecl getLocalDBErrorMessage(__in DWORD dwNativeError,
                                                    __out_ecount(pcchErrMsgBuf)LPWSTR wszErrorMessage,
                                                    __inout LPDWORD pcchErrMsgBuf);
};
    

