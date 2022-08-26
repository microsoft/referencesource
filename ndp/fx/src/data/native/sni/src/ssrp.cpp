//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: Ssrp.cpp
//
// Purpose: client implementation of the SSRP (officially called MC-SQLR) protocol, to resolve
//		    SQL Server instance names to connectivity information, such as TCP port #s.
//
//          
// @EndHeader@
//****************************************************************************


#include "snipch.hpp"
#include "ssrp.hpp"
#include <lm.h>

namespace SSRP 
{

#define INSTREGKEY "SOFTWARE\\Microsoft\\Microsoft SQL Server\\"
#define UDP_ADV_PORT 1434
#define SSRP_MULTICAST_ADDR        "ff02:0000:0000:0000:0000:0000:0000:0001"     

// This timeout value specified how long we should wait if there is no response from the network in each socket read. This timeout can happend when 
// (1) there is no sqlbrowser in the multicast/broadcast domain of the ssrp request.
// (2) all request has been received by previous read and we are still expect more record.
#define SSRP_WAIT_TIMEOUT 1000				//millisec, 
// This timeout value specified a hypothetical time that all sqlbrowser should have been received. By default, this is ARP value, 5 second.
#define SSRP_WAIT_TIME_LEAST 5000			//millisec
// This is timeout is client cutoff time to prevent DOS attach from malicious/malfunctioned sqlbrowers. in following cases.
// (1) legitimate ssrp record, the consumer either timeout the loop or bufferlimiting on the record receiced.
// (2) non-legitimate ssrp record, we will drop the record directly and keep reading until this timeout expired.
#define DEFAULT_SSRP_ENUM_TIMEOUT 15000	//millisec

#define DEFAULT_SSRPGETINFO_TIMEOUT 1000	//millisec, This value should be less than TDS timeout value 15000 for now.

#define UDP_RECV_BUFSIZE	65536		//used for broadcast and multicast

#define MAX_SOCKET_NUM	64		//we can have at most 64 sockets

enum
{
    CLNT_BCAST = 0x01,
    CLNT_BCAST_EX,
    CLNT_UCAST_EX,
    CLNT_UCAST_INST,
    SVR_RESP,
    SVR_ACTIVATE,
    SVR_CONNRESP,
    SVR_JOIN,
    SVR_LEAVE,
    SVR_KEEPALIVE,
    SVR_PASSIVE,
    CLNT_BROWSER_SERVERS,
    CLNT_ACTIVE,
    SVR_TIMEOUT,
    CLNT_UCAST_DAC,
    CLNT_UCAST_DEBUG,
} SSRPMSGTYPE;

class SsrpSocket {
	__field_range(0, MAX_SOCKET_NUM) int 	m_nSockets;
	__field_ecount(MAX_SOCKET_NUM) SOCKET 	m_pSockets[MAX_SOCKET_NUM];
	DWORD m_ssrp_wait_time_least; 
	DWORD m_ssrp_wait_timeout;
	DWORD m_time_start_query;
	
	//
	// this is only for ipv4
	// sends a broadcast ssrp request to network
	//
	SOCKET OpenBroadcast()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

		SOCKET sock;

		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if( sock == INVALID_SOCKET )
			goto ErrorExit;

		BOOL fVal = TRUE;
		if( SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&fVal, sizeof fVal))
			goto ErrorExit;

		int recvbufsize = UDP_RECV_BUFSIZE;
		if( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&recvbufsize, sizeof(recvbufsize)) )
		{			
			DWORD dwError = WSAGetLastError();
			BidTrace1(ERROR_TAG _T("setsockopt SO_RCVBUF: %d{WINERR}\n"), dwError);
		}
		
OACR_WARNING_SUPPRESS(IPV6_ADDRESS_STRUCTURE_IPV4_SPECIFIC , " TEMPORARY SUPPRESSION UNTIL DEVELOPER'S ANALYSIS. See VSTS 376830 ")
		SOCKADDR_IN broadcast;
		broadcast.sin_family = AF_INET;
		broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		broadcast.sin_port = htons(UDP_ADV_PORT);

		char pBuf[1];
		pBuf[0] = CLNT_BCAST_EX;
		if( SOCKET_ERROR == sendto( sock, pBuf, 1, 0, (SOCKADDR *)&broadcast, sizeof broadcast ))
			goto ErrorExit;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("sock: %Iu{SOCKET}\n"), sock);

		return sock;
		
	ErrorExit:

		if( sock != INVALID_SOCKET )
			closesocket( sock );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("sock: %Iu{SOCKET}\n"), INVALID_SOCKET);

		return INVALID_SOCKET;
	}

	//
	// this is only for ipv6
	// ipv6 doesn't support broadcast messages so we do multicast instead
	//
	SOCKET OpenMulticast()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

		SOCKET sock;

		sock = socket( AF_INET6, SOCK_DGRAM, 0);
		if( sock == INVALID_SOCKET )
			goto ErrorExit;

		//setting this to 1 is equal to broadcasting for ipv4
		int OptVal = 1;
		if ( SOCKET_ERROR == setsockopt (sock, IPPROTO_IPV6, 
								IPV6_MULTICAST_HOPS, (char *)&OptVal, sizeof (int)))
			goto ErrorExit;
		
		int recvbufsize = UDP_RECV_BUFSIZE;
		if( setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&recvbufsize, sizeof(recvbufsize)) )
		{			
			DWORD dwError = WSAGetLastError();
			BidTrace1(ERROR_TAG _T("setsockopt SO_RCVBUF: %d{WINERR}\n"), dwError);
		}

		SOCKADDR_IN6 multiaddr;
		int len = sizeof multiaddr;

		//convert ipv6 numeric address to fill in SOCKADDR_IN6 struct
		if(WSAStringToAddressA( SSRP_MULTICAST_ADDR, AF_INET6,	
				0, (LPSOCKADDR)&multiaddr,		&len))
			goto ErrorExit;

		multiaddr.sin6_family = AF_INET6;
		multiaddr.sin6_port = htons( UDP_ADV_PORT );
		multiaddr.sin6_flowinfo = 0;
		multiaddr.sin6_scope_id = 0;

		char pBuf[1];
		pBuf[0] = CLNT_BCAST_EX;
		
		if( SOCKET_ERROR == sendto( sock, pBuf, 1, 0, 
					(SOCKADDR *)&multiaddr, sizeof multiaddr))
			goto ErrorExit;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("sock: %Iu{SOCKET}\n"), sock);

		return sock;
		
	ErrorExit:

		if( sock != INVALID_SOCKET )
			closesocket( sock );

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("sock: %Iu{SOCKET}\n"), INVALID_SOCKET);

		return INVALID_SOCKET;
	}
	
public:

	SsrpSocket():m_ssrp_wait_timeout(SSRP_WAIT_TIMEOUT), m_ssrp_wait_time_least(SSRP_WAIT_TIME_LEAST)
	{
		m_nSockets = 0;
		m_time_start_query = 0;
	}
	~SsrpSocket()
	{
		while( m_nSockets )
			closesocket( m_pSockets[--m_nSockets]);
	}

	void SetSsrpWaitTimeLeast(int timeout_millis)
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "timeout_millis %d\n"), timeout_millis);
		Assert( timeout_millis > 0 || timeout_millis == INFINITE);		
		m_ssrp_wait_time_least = timeout_millis;
	}
	
	void SetSsrpWaitTimeout(int timeout_millis)
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "timeout_millis %d\n"), timeout_millis);
		Assert( timeout_millis > 0 || timeout_millis == INFINITE);		
		m_ssrp_wait_timeout = timeout_millis;
	}

		//opens udp sockets to server and sends a ssrp request
	bool OpenUnicast( const WCHAR *wszServer, __in_bcount(cBuf) char *pBuf, int cBuf )
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T( "wszServer: '%s', pBuf: %p, cBuf: %d\n"),
						wszServer, pBuf, cBuf);

		ADDRINFOW 			* AddrInfoW=0;
		ADDRINFOW 			Hints;

		if(m_nSockets>ARRAYSIZE(m_pSockets)-2)   //this method could add 2 more sockets, fail out if we don't have enough room. 
		{
			BidTrace0( ERROR_TAG _T("Wrong usage of this method.\n"));
			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));
			return false; 
		}

		memset(&Hints, 0, sizeof(Hints));
		Hints.ai_family = PF_UNSPEC;
		Hints.ai_socktype = SOCK_DGRAM;

		if( GetAddrInfoW_l( wszServer, L"1434", &Hints, &AddrInfoW, GetDefaultLocale()))
		{
			DWORD dwRet = WSAGetLastError();
			if ( EAI_NONAME == dwRet )
			{
				//For numeric name in form of three-part address, e.g. 127.0.1 or two-part address, e.g. 127.1, retry getaddrinfo with AI_NUMERICHOST 
				// as hint.ai_flags;			
				Hints.ai_flags |=  AI_NUMERICHOST;			
				if( GetAddrInfoW_l( wszServer, L"1434", &Hints, &AddrInfoW, GetDefaultLocale()))				
				{
					BidTrace1(ERROR_TAG _T("wszServer: '%s' not found\n"), wszServer);

					BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));
					
					return false;
				}
			}
			else
			{
				BidTrace1(ERROR_TAG _T("wszServer: '%s' not found\n"), wszServer);

				BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

				return false;
			}
		}

		// Only attempt to send SSRP request to at most the first 64 addresses - in practice, this shouldn't actually limit anybody.
		// The limit comes from the maximum default # of objects we can pass to MultiSubnetFailover, which will affect us at TCP connect time. It's also 
		// the default maximum for select() on Windows, though in theory we could increase this number in ReadEx().
		// Note on socket usage:
		//    - SSRP "broadcast" targets exactly 4 addresses - the IPv4 broadcast, IPv6 broadcast for some specific local group, IPv4 local loopback, and IPv6 local loopback
		//    - SSRP "unicast" targets all the addresses resolved for a given machine, up to 64 of them.
		// This is a behavior change from previous client SSRP "unicast" implementation - previously, it targeted only the first IPv4 address it resolved and the first IPv6 address
		// it resolved.
		for( ADDRINFOW *AIW=AddrInfoW; AIW !=0 && m_nSockets < 64; AIW = AIW->ai_next )
		{
			if( AIW->ai_family != AF_INET && AIW->ai_family != AF_INET6 )
				continue;

			m_pSockets[m_nSockets] = socket(AIW->ai_family, AIW->ai_socktype, AIW->ai_protocol);

			if( m_pSockets[m_nSockets] == INVALID_SOCKET )
				continue;
			
			if( SOCKET_ERROR == sendto( m_pSockets[m_nSockets], pBuf, cBuf, 0, 
				AIW->ai_addr, (int)AIW->ai_addrlen))
			{
				closesocket( m_pSockets[m_nSockets] );
				continue;
			}

			//everything went fine, add to list
			m_nSockets++;
		}

		FreeAddrInfoW( AddrInfoW );

		if( m_nSockets == 0 )
		{
			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

			return false;
		}

		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));
		
		return true;

	}

	bool Open( __in LPCWSTR wszServer, __in LPCSTR szInstance )
	{
		BidxScopeAutoSNI2( SNIAPI_TAG _T( "wszServer: '%s', szInstance: '%hs'\n"),
						wszServer, szInstance);

		Assert( wszServer );
		Assert( szInstance );
		Assert(m_nSockets==0);

		if(m_nSockets!=0)
		{
			BidTrace0( ERROR_TAG _T("Wrong usage of this method.\n"));
			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));
			return false; 
		}
		
		//if server name is not specified it is a broadcast request
		if( !wszServer[0] )
		{
			Assert( !szInstance[0] );

			//ipv4
			m_pSockets[m_nSockets] = OpenBroadcast();
			if( m_pSockets[m_nSockets] != INVALID_SOCKET )
				m_nSockets++;

			//ipv6
			m_pSockets[m_nSockets] = OpenMulticast();
			if( m_pSockets[m_nSockets] != INVALID_SOCKET )
				m_nSockets++;

			//do unicast to loopback to enable enumrate local instances even if firewall is on and/or no network is available
			char pBuf[1];
			pBuf[0] = CLNT_BCAST_EX;		
			(void)OpenUnicast(NULL, pBuf, 1);	//do a Broascast request in Unicast way
			
			if( m_nSockets == 0)
			{
				BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

				return false;
			}

			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));

			return true;
		}
		else
		{
			int 				cBuf;
			char 				pBuf[MAX_NAME_SIZE+2];
			
			if( szInstance[0] )
			{
				cBuf = (int)strlen(szInstance)+2;
				if( cBuf > sizeof(pBuf))
				{
					BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

					return false;
				}
				
				pBuf[0] = CLNT_UCAST_INST;
				// We verified the destination size above, so ignore 
				// the return value.  
				(void)StringCchCopyA(pBuf+1, CCH_ANSI_STRING(pBuf)-1, szInstance);
			}
			else
			{
				cBuf = 1;
				pBuf[0] = CLNT_UCAST_EX;
			}

			return OpenUnicast( wszServer, pBuf, cBuf );
		}
	}

	void Starttimer()
	{
		m_time_start_query = GetTickCount();
	}
	//receives ssrp packets
	//return value 0 means timeout in recv operation
	//SOCKET_ERROR means error
	//anything greater than 0 is the length of bytes read
	int Read( char *pBuf, int cBuf)
	{
		return ReadEx(pBuf, cBuf,  SSRP_WAIT_TIMEOUT);
	}
	
	int ReadEx( char *pBuf, int cBuf, int timeout_millis)
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T( "pBuf: %p, cBuf: %d, timeout_millis %d\n"),
						pBuf, cBuf, timeout_millis);
		Assert( timeout_millis > 0 || timeout_millis == INFINITE);
		int retval = SOCKET_ERROR;
		int i = 0;
		int nBytes = SOCKET_ERROR;
		
		do
		{
			fd_set SockSet;

			FD_ZERO(&SockSet);
			for( i =0; i<m_nSockets; i++)
			{
				FD_SET(m_pSockets[i], &SockSet);
			}
			
			if(timeout_millis ==INFINITE)
			{
				retval = select(m_nSockets, &SockSet, 0, 0, NULL);
			}
			else
			{
				struct timeval timeout;
				
				// SQL BU 322360:
				// To allow enough time to get the initial response from every server, we enforce to wait at lease
				// ssrp_wait_time_least. After it elapse, we use ssrp_wait_timeout.
				DWORD time_query_elapsed = GetTickCount() - m_time_start_query;				
				int timeout_cap =  m_ssrp_wait_time_least > m_ssrp_wait_timeout 
									&&time_query_elapsed < m_ssrp_wait_time_least - m_ssrp_wait_timeout
									  ?
									m_ssrp_wait_time_least - time_query_elapsed:m_ssrp_wait_timeout;

				timeout_millis = timeout_millis > timeout_cap? timeout_cap:timeout_millis;
 
				timeout.tv_sec = timeout_millis/1000;
				timeout.tv_usec =timeout_millis%1000*1000;
				retval = select(m_nSockets, &SockSet, 0, 0, &timeout);
			}
			
			if( retval <= 0 )
				goto ErrorExit;

			for( i =0; i<m_nSockets; i++)
				if( FD_ISSET(m_pSockets[i], &SockSet) )
				{
					FD_CLR( m_pSockets[i], &SockSet);

					nBytes = recv( m_pSockets[i], pBuf, cBuf, 0);
					if( nBytes == SOCKET_ERROR )
					{
						if( WSAEMSGSIZE == WSAGetLastError() ) 
							continue;
						
						closesocket( m_pSockets[i] );
						m_nSockets--;
						for( int j=i; j<m_nSockets; j++ )
							m_pSockets[j] = m_pSockets[j+1];
						i--;
						continue;
					}
					break;
				}
				
			if( 0 == m_nSockets )
			{
				retval = SOCKET_ERROR;
				goto ErrorExit;
			}
		}while ( i == m_nSockets );
		
		retval = nBytes;
		
	ErrorExit:

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%d\n"), retval);

		return retval;

	}
};

bool GetAdminPort( const WCHAR *wszServer, const WCHAR *wszInstance, __inout USHORT *pPort)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "wszServer: '%s', wszInstance: '%s', pPort: %p\n"),
						wszServer, wszInstance, pPort);

	char	pBuf[MAX_NAME_SIZE+3];
	int		cBuf;

   	SsrpSocket ssrpSocket;
	
	char szInstance[MAX_NAME_SIZE+3];
	int ret = WideCharToMultiByte(CP_ACP, 0, wszInstance, wcslen(wszInstance), NULL, 0, NULL, NULL);
	if (ret > MAX_NAME_SIZE+3)
	{
		goto ErrorExit;
	}
	ret = WideCharToMultiByte(CP_ACP, 0, wszInstance, wcslen(wszInstance), szInstance, sizeof(szInstance), NULL, NULL);
	if (ret == 0)
	{
		goto ErrorExit;
	}
	szInstance[ret]='\0';

	//format of the request packet is
	//first byte CLNT_UCAST_DAC
	//one byte protocol version number which is 1 for this implementation
	//then null terminated instance name
	
	pBuf[0] = CLNT_UCAST_DAC;
	pBuf[1] = 1;
	if (FAILED (StringCchCopyA( pBuf+2, CCH_ANSI_STRING(pBuf)-2, szInstance )))
		goto ErrorExit;

	cBuf = (int)strlen( szInstance )+3;
	
	if( false == ssrpSocket.OpenUnicast( wszServer, pBuf, cBuf ))
		goto ErrorExit;

	cBuf = ssrpSocket.Read( pBuf, 6);

	//format of the reply packet is
	//first byte SVR_RESP
	//two bytes length of the whole packet( it has to be 6 for version 1 )
	//one byte version number( it has to be 1 for this implementation)
	//last two bytes specifies the tcp port number for admin connection
	
	if( cBuf != 6 || pBuf[0]!=SVR_RESP || 
		*(UNALIGNED USHORT *)(pBuf+1)!=6 || pBuf[3]!=1 )
	{
		goto ErrorExit;
	}

	*pPort = *(UNALIGNED USHORT *)(pBuf+4);

	if( !*pPort )
		goto ErrorExit;

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));

	return true;

ErrorExit:
	
	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

	return false;
}

DWORD ParseSsrpString(const WCHAR * wszServer, __in LPSTR ssrpString, size_t cchSsrpString, __inout ProtList *pProtList)
{
	ProviderNum rgPSupported[10];
	int nSupported=0;
	LPWSTR wszClusEnv = NULL; 

	//
	// locate start of first protocol info
	// it should be after Version info
	//

	if ( cchSsrpString > INT_MAX )
	{
		BidTraceU0( SNI_BID_TRACE_ON,SNI_TAG _T("String length is longer than int_max.\n"));
		goto ErrorExit;
	}
	
	char * szProtStart = StrStrA_SYS(0, ssrpString, static_cast<int>(cchSsrpString), "Version;", sizeof("Version;") - 1);
	if( !szProtStart)
	{
		goto ErrorExit;
	}

	szProtStart += strlen("Version;");
	szProtStart = strchr(szProtStart, ';');
	if( !szProtStart)
	{
		goto ErrorExit;
	}

	// skip ';' char
	szProtStart++;

	// now szProtStart points to the first protocol
	
	// Process each protocol info

	while( *szProtStart != ';' )
	{
		if( nSupported >= sizeof(rgPSupported)/sizeof(rgPSupported[0]))
		{
			goto ErrorExit;
		}

		//
		// szProtStart is the start of protocol name
		// szParamStart is the start of parameter for this protocol
		// szParamEnd is the end of parameter
		//

		char * szParamStart;
		
		szParamStart = strchr(szProtStart, ';');

		if( !szParamStart)
		{
			goto ErrorExit;
		}

		*szParamStart = 0;

		// skip ';' at the end of protocol name

		szParamStart++;
		
		char * const szParamEnd = strchr(szParamStart, ';');

		if( !szParamEnd )
		{
			goto ErrorExit;
		}

		*szParamEnd = 0;

		if( MAX_PROTOCOLPARAMETER_LENGTH < static_cast<int>(szParamEnd-szParamStart))
		{
			goto ErrorExit;
		}
		
		ProtElem *pProtElem;

		switch( szProtStart[0] )
		{
			case 'n':   // Named Pipe

				pProtElem = pProtList->Find(NP_PROV);

				if( !pProtElem )
				{
					break;
				}

				rgPSupported[nSupported++] = NP_PROV;

				//fix pipe name by replacing the server part
				//with the user provided server
				if( szParamStart[0]!='\\' || szParamStart[1]!='\\' )
					goto ErrorExit;
            	
				szParamStart += 2;

				//skip machine name
				while( *szParamStart && *szParamStart!='\\' )
					szParamStart++;

				if( !*szParamStart )
					goto ErrorExit;

				//now szParamStart contains the the remainder without the server name
				
				if( IsLocalHost(wszServer) )
				{
					// _dupenv_s() returns non-zero for failure.  
					//
					if( _wdupenv_s(&wszClusEnv, NULL, L"_CLUSTER_NETWORK_NAME_") )
					{
						goto ErrorExit;
					}

					// If not a local connection to a clusterred machine,
					// then go ahead and remove machine name.
					//
OACR_WARNING_PUSH
OACR_WARNING_DISABLE(SYSTEM_LOCALE_MISUSE , " INTERNATIONALIZATION BASELINE AT KATMAI RTM. FUTURE ANALYSIS INTENDED. ")
					if( !wszClusEnv
					|| CSTR_EQUAL != CompareStringW(LOCALE_SYSTEM_DEFAULT,
									 NORM_IGNORECASE|NORM_IGNOREWIDTH,
									 wszClusEnv, -1,
									 wszServer, -1))
OACR_WARNING_POP
					{
						pProtElem->Np.Pipe[MAX_NAME_SIZE]=L'\0';
						
						if( FAILED(StringCchPrintfW(	pProtElem->Np.Pipe, 
											ARRAYSIZE(pProtElem->Np.Pipe), 
											L"\\\\.%hs", 
											szParamStart)))
						{
								goto ErrorExit;
						}

						break;
					}
				}

				pProtElem->Np.Pipe[MAX_NAME_SIZE]=L'\0';

				if( FAILED(StringCchPrintfW(	pProtElem->Np.Pipe, 
									ARRAYSIZE(pProtElem->Np.Pipe), 
									L"\\\\%s%hs", 
									wszServer, 
									szParamStart)))
				{
						goto ErrorExit;
				}
				
				break;
		
			case 't':   // Tcp

				pProtElem = pProtList->Find(TCP_PROV);

				if( !pProtElem )
				{
					break;
				}

				rgPSupported[nSupported++] = TCP_PROV;

				if( 0 == atoi(szParamStart))
					goto ErrorExit;
				
				if( FAILED(StringCchPrintfW( pProtElem->Tcp.wszPort,
									ARRAYSIZE(pProtElem->Tcp.wszPort),
									L"%hs",
									szParamStart )))
				{
					goto ErrorExit;
				}

				break;
				
			case 'v':   // Via

				pProtElem = pProtList->Find(VIA_PROV);

				if( !pProtElem )
				{
					break;
				}

				rgPSupported[nSupported++] = VIA_PROV;

				// The Via string is in the foll format - 
				// "MachineName,nicA:XXXX,nicB:YYYY"
				// The string we create to pass to Open() of the Via Netlib is in
				// the form - "MachineName[:ClusterName],A:XXXX",
				// where the ':ClusterName' is an optional field added to support
				// clusters


				// Get the 'real' server name from the szParam string
				char * szSvrEnd;

				szSvrEnd = strchr(szParamStart, ',');

				if( !szSvrEnd )
				{
					goto ErrorExit;
				}
				
				*szSvrEnd = 0;

				/// isn't this risky? why are we doing this?
				
				pProtElem->Via.Host[MAX_NAME_SIZE]='\0';

				if( FAILED(StringCchPrintfW(	pProtElem->Via.Host, 
									ARRAYSIZE(pProtElem->Via.Host), 
									L"%hs:%s",
									szParamStart,
									wszServer)))
				{
						goto ErrorExit;
				}

				szParamStart = szSvrEnd+1;

				pProtElem->Via.Param[0] = 0;
				pProtElem->Via.Param[MAX_PROTOCOLPARAMETER_LENGTH] = 0;
				while(1)
				{
					char *szNicEnd;

					szNicEnd = strchr( szParamStart, ':');

					if( !szNicEnd )
					{
						goto ErrorExit;
					}
					
					*szNicEnd = 0;

					char *szPortEnd;
					
					szPortEnd = strchr(szNicEnd+1, ',');

					if( szPortEnd )
					{
						*szPortEnd = 0;
					}

					int cLen = (int)wcslen(pProtElem->Via.Param);

					if( FAILED(StringCchPrintfW(	pProtElem->Via.Param + cLen, 
										ARRAYSIZE(pProtElem->Via.Param) - cLen, 
										(cLen==0) ? L"%hs,%hs":L",%hs,%hs",
										szNicEnd+1, // port start
										szParamStart )))// nic start
					{
						goto ErrorExit;
					}

					// if this was the last ',' in the string then finish
					if( !szPortEnd )
					{
						break;
					}

					szParamStart = szPortEnd+1;
				}
        	    
				break;

			case 'r':   // Rpc 
			case 'b':   // Banyan Vines
			case 's':   // Spx
			case 'a':   // Adsp
				break;
 			   
			default:
				goto ErrorExit;
		}

		szProtStart = szParamEnd+1;
	}

	//
	// Here we try to find protocols which are asked by user
	// and at the same time supported on the server
	//

	ProtElem * pProtElem = pProtList->Head;
	
	while( pProtElem )
	{
		int i;
		for(i=0;i<nSupported;i++)
		{
			if(rgPSupported[i]==pProtElem->GetProviderNum())
			{
				break;
			}
		}
		
		if(i==nSupported) 	//not found
		{
			ProviderNum PN = pProtElem->GetProviderNum();

			pProtElem = pProtElem->m_Next;

			if(PN==SM_PROV)
				continue;

			ProtElem *pDelete = pProtList->RemoveProt(PN);

			Assert( pDelete );

			delete pDelete;
		}
		else //found
		{
			rgPSupported[i] = rgPSupported[--nSupported];

			pProtElem = pProtElem->m_Next;
		}
	}

	Assert( !nSupported );

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("success\n"));

	return ERROR_SUCCESS;
	
ErrorExit:

	if( NULL != wszClusEnv )
	{
		free(wszClusEnv); 
	}

	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("fail\n"));

	return ERROR_FAIL;
}

DWORD SsrpGetInfo( __in LPWSTR wszServer, __in LPWSTR wszInstance, __inout ProtList * pProtList)
{
	BidxScopeAutoSNI3( SNIAPI_TAG _T( "wszServer: '%s', wszInstance: '%s', pProtList: %p\n"),
					wszServer, wszInstance, pProtList);

	Assert( wszInstance[0] );

	Assert( wszServer[0] );

	SsrpSocket ssrpSocket;
	
	char szInstance[MAX_NAME_SIZE+3];
	int ret = WideCharToMultiByte(CP_ACP, 0, wszInstance, wcslen(wszInstance), NULL, 0, NULL, NULL);
	if (ret > MAX_NAME_SIZE+3)
	{
		goto ErrorExit;
	}
	ret = WideCharToMultiByte(CP_ACP, 0, wszInstance, wcslen(wszInstance), szInstance, sizeof(szInstance), NULL, NULL);
	if (ret == 0)
	{
		goto ErrorExit;
	}
	szInstance[ret]='\0';

	if( false == ssrpSocket.Open( wszServer, szInstance ))
	{
		goto ErrorExit;
	}

	ssrpSocket.Starttimer();
	
	char pBuf[1024];
	int cBuf = 1024;
	
	// The time out here must be less than TDS timeout value that currently is 15 seconds
	cBuf = ssrpSocket.ReadEx( pBuf, sizeof(pBuf)-1, DEFAULT_SSRPGETINFO_TIMEOUT);

	if ((cBuf <= 15) //$ SPT:351275, skip the "servername;", etc. so it has to be at least 15 char long!
		|| (pBuf[0] != SVR_RESP)
		|| (cBuf-3 != *(UNALIGNED USHORT *)(pBuf+1)))
	{
		goto ErrorExit;
	}

	pBuf[cBuf] = 0;
	
	// Get server name from string returned. This is cos' 
	// the server may be clustered and you'll need the cluster
	// name rather than machine name
	//
	char *szSvrEnd = strchr( &pBuf[14], ';');

	if( !szSvrEnd )
	{
		goto ErrorExit;
	}

	*szSvrEnd = 0;
	
	if( MAX_NAME_SIZE < static_cast<int>(szSvrEnd - &pBuf[14]))
	{
		goto ErrorExit;
	}

	// Do not allow "." or "(local)" connections to 
	// a clustered instance

	WCHAR pwBuf[1024];
	if (FAILED(StringCchPrintfW(pwBuf,
							ARRAYSIZE(pwBuf),
							L"%hs",
							&pBuf[14])))
	{
		goto ErrorExit;
	}
	pwBuf[1023] = L'\0';

	if( IsLocalHost(wszServer) && 
		!IsLocalHost(pwBuf))
	{
		goto ErrorExit;
	}

	return ParseSsrpString( wszServer, szSvrEnd+1, strlen(szSvrEnd+1), pProtList);

ErrorExit:
	
	BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("fail\n"));

	return ERROR_FAIL;
}

typedef NET_API_STATUS (NET_API_FUNCTION * FUNCNETSERVERENUM)( char *,
							       DWORD,
							       LPBYTE *,
							       DWORD,
							       DWORD *,
							       DWORD *,
							       DWORD,
							       char *,
							       DWORD * );

typedef NET_API_STATUS (NET_API_FUNCTION * FUNCNETAPIBUFFERFREE)( LPVOID );

class ServerEnum
{
	//
	// ssrp related
	//

	bool 		m_fSsrpFinished;

	bool 		m_fExtendedInfo;
	
	SsrpSocket 	m_SsrpSocket;

	char 		m_rgReadBuffer[4096];

	char 		* m_pCurrent;

	char		* m_pSavedRecord;

	int			m_cSavedRecord;
	
	//
	// lan related members
	//

	bool 		m_fLanFinished;
	
	LPSERVER_INFO_100 m_pLanInfo;

	DWORD 		m_nEntries;

	DWORD		m_iEntry;

	//
	// members which enable access to netapi32 library
	//
	
	HMODULE 				m_hLan;
	FUNCNETSERVERENUM 		m_pfuncNetServerEnum;
	FUNCNETAPIBUFFERFREE 	m_pfuncNetApiBufferFree;

public:

	ServerEnum( bool fExtendedInfo ):
		m_fSsrpFinished(false),
		m_fExtendedInfo(fExtendedInfo),
		m_pCurrent(0),
		m_pSavedRecord(0),
		m_cSavedRecord(0),
		m_fLanFinished(false),
		m_pLanInfo(0),
		m_nEntries(0),
		m_iEntry(0),
		m_hLan(0),
		m_pfuncNetServerEnum(0),
		m_pfuncNetApiBufferFree(0)
	{
	}

	~ServerEnum()
	{
		if( m_hLan )
		{
			Assert( m_pfuncNetApiBufferFree );

			if( m_pLanInfo )
			{
				m_pfuncNetApiBufferFree( m_pLanInfo );

				m_pLanInfo = 0;
			}

			FreeLibrary( m_hLan );

			m_hLan = 0;
		}
	}

	bool InitializeLan()
	{
		BidxScopeAutoSNI0( SNIAPI_TAG _T( "\n"));

		HMODULE hLan;
		
		hLan = SNILoadSystemLibraryA("netapi32.dll");

		if( NULL == hLan )
		{
			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

			return false;
		}

		m_pfuncNetServerEnum = (FUNCNETSERVERENUM)GetProcAddress( hLan, "NetServerEnum");
		
		m_pfuncNetApiBufferFree = (FUNCNETAPIBUFFERFREE)GetProcAddress( hLan, "NetApiBufferFree");


		if( !m_pfuncNetServerEnum || !m_pfuncNetApiBufferFree )
		{
			FreeLibrary( hLan );

			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

			return false;
		}

		DWORD dwTotalEntries = 0;

		NET_API_STATUS status;

		status = m_pfuncNetServerEnum( 	NULL,
										100,
										(LPBYTE *)&m_pLanInfo,
										MAX_PREFERRED_LENGTH,
										&m_nEntries,
										&dwTotalEntries,
										SV_TYPE_SQLSERVER,
										NULL,
										NULL);
		
		if(	status != NERR_Success && 
			status != ERROR_MORE_DATA )

		{
			Assert( !m_pLanInfo );
			Assert( !m_nEntries );

			FreeLibrary( hLan );

			BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

			return false;
		}

		Assert( m_nEntries == dwTotalEntries);

		m_hLan = hLan;
		
		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));

		return true;
	}
	
	bool Initialize(__in LPCWSTR wszServer, __in DWORD waittime_least, __in DWORD waittimeout)
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T( "wszServer: '%s waittime_lease: %d, waittimeout: %d'\n"), wszServer,waittime_least,waittimeout);

		if(waittime_least > 0 ) SetSsrpWaitTimeLeast(waittime_least); //don't support infinite -1;
		if(waittimeout > 0 ) SetSsrpWaitTimeout(waittimeout); //don't support infinite -1 at this point

		if( !m_SsrpSocket.Open( wszServer, ""))
		{
			Assert(wszServer[0]);

			SetSsrpFinished();
		}

		//
		// when a server is specified we don't do Lan enum
		//

		if( wszServer[0] || !InitializeLan())
		{
			SetLanFinished();
			
			if( IsSsrpFinished() )
			{
				BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("false\n"));

				return false;
			}
		}

		m_SsrpSocket.Starttimer();
		BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("true\n"));

		return true;
	}

	inline bool IsSsrpFinished() const
	{
		return m_fSsrpFinished;
	}

	inline void SetSsrpFinished()
	{
		Assert( !m_fSsrpFinished );
		m_fSsrpFinished = true;
	}

	inline void SetSsrpWaitTimeout(int timeout_millis)
	{
		m_SsrpSocket.SetSsrpWaitTimeout(timeout_millis);
	}

	inline void SetSsrpWaitTimeLeast(int timeout_millis)
	{
		m_SsrpSocket.SetSsrpWaitTimeLeast(timeout_millis);
	}
	
	inline bool HaveSavedRecord() const
	{
		return ( 0 != m_cSavedRecord );
	}

	inline bool IsValidVersionString( __in LPCSTR szStr )
	{
		if( !*szStr )
			return false;

		while( (*szStr <= '9' && *szStr >= '0') || *szStr == '.' )
			szStr++;

		if( !*szStr )
			return true;

		return false;
	}
	
	bool SsrpGetNextRecord(int timeout)
	{
		BidxScopeAutoSNI1( SNIAPI_TAG _T( "timeout:%d\n"), timeout);
		
		Assert( timeout > 0 || timeout == INFINITE);
		int timeleft = timeout;
		bool fisFirst = true;
		DWORD timestart=GetTickCount();
		
		while(1)
		{
			Assert( !m_cSavedRecord && !m_pSavedRecord );

			if(!fisFirst)
			{
				if (timeout != INFINITE)
				{
					timeleft = timeout -(GetTickCount() - timestart);
					if(timeleft <=0 || timeleft >timeout) 
					{
						BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("Timeout timeleft:%d\n"), timeleft);						
						return false;					
					}
				}
				
			}
			else
				fisFirst = false;

			//BidTrace1(ERROR_TAG _T("timeleft:%d\n"),timeleft);
			if( !m_pCurrent )
			{
				int cBuf;
				
				cBuf = m_SsrpSocket.ReadEx( m_rgReadBuffer, sizeof(m_rgReadBuffer)-1, timeleft);

				if( cBuf == SOCKET_ERROR || cBuf == 0) // error (-1) or timeout (0)
				{
					BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("false, timeleft:%d\n"), timeleft);

					return false;
				}

				if( 	cBuf<=3 || 
						m_rgReadBuffer[0] != SVR_RESP ||
						cBuf-3 != *(UNALIGNED USHORT *)(m_rgReadBuffer+1) )
				{
					// drop this packet

					BidTrace0( ERROR_TAG _T("ssrp format\n"));

					continue;
				}

				m_rgReadBuffer[cBuf] = 0;
				
				m_pCurrent = m_rgReadBuffer+3;
			}

			Assert( m_pCurrent );
			Assert( !m_cSavedRecord && !m_pSavedRecord );

			static const char sc_szServerName[] = "ServerName;";

			// Note: in the following statement, strlen() is safe to call 
			// because the buffer was forcibly terminated // above (see 
			// 'm_rgReadBuffer[cBuf] = 0').
			//

			if ( strlen(m_pCurrent) > INT_MAX )
			{
				BidTraceU0( SNI_BID_TRACE_ON,RETURN_TAG _T("String length is longer than int_max.\n"));
				return false;
				
			}
			char * szServerStart = StrStrA_SYS(0, m_pCurrent, static_cast<int>(strlen(m_pCurrent)), 
										sc_szServerName, sizeof(sc_szServerName) - 1);
			m_pCurrent = 0;
			if( !szServerStart )
			{
				//
				// if no records has been processed from this buffer 
				// then it is a wrong formatted ssrp packet
				//
				
				if( szServerStart == m_rgReadBuffer )
				{
					BidTraceU0( SNI_BID_TRACE_ON,ERROR_TAG _T("ssrp format\n"));
				}
				
				continue;
			}

			szServerStart += (sizeof(sc_szServerName) - 1);
			char *szServerEnd = strchr(szServerStart,';');
			if( !szServerEnd )
			{
				BidTrace0(ERROR_TAG _T("ssrp format\n"));

				continue;
			}

			*szServerEnd = 0;
			int cServer = static_cast<int>(szServerEnd-szServerStart);
			if( MAX_NAME_SIZE < cServer)
			{
				BidTrace0(ERROR_TAG _T("ssrp format\n"));

				continue;
			}

			char *szInstanceStart = szServerEnd+1;
			if( strncmp( szInstanceStart, "InstanceName;", sizeof("InstanceName;")-1))
			{
				BidTrace0(ERROR_TAG _T("ssrp format\n"));

				continue;
			}

			szInstanceStart += (sizeof("InstanceName;")-1);
			char *szInstanceEnd = strchr( szInstanceStart, ';');
			if( !szInstanceEnd )
			{
				BidTrace0(ERROR_TAG _T("ssrp format\n"));

				continue;
			}

			*szInstanceEnd = 0;
			int cInstance = static_cast<int>(szInstanceEnd-szInstanceStart);
			if( MAX_NAME_SIZE < cInstance )
			{
				BidTrace0(ERROR_TAG _T("ssrp format\n"));

				continue;
			}

			int cRecordLen = cServer+1;

			//
			// append instance name if it is not the default instance
			//
			
			if( _stricmp( "MSSQLSERVER", szInstanceStart) )
			{
				szServerStart[cServer] = '\\';

				memmove( szServerStart+cServer+1, szInstanceStart, cInstance+1 );

				cRecordLen += (cInstance+1);
			}

			if( m_fExtendedInfo )
			{
				char *szExtendedInfo = szInstanceEnd+1;

				char *szVersionStart;

				if( !strncmp(szExtendedInfo,"IsClustered;No;Version;",sizeof("IsClustered;No;Version;")-1) )
				{
					//
					// make szVersionStart point to ';' after Version
					//
					
					szVersionStart = szExtendedInfo + sizeof("IsClustered;No;Version;")-2;					

					//
					// replace ';' with ':'
					//
					
					*szVersionStart++ = ':';
				}
				else if( !strncmp(szExtendedInfo,"IsClustered;Yes;Version;",sizeof("IsClustered;Yes;Version;")-1))
				{
					//
					// make szVersionStart point to ';' after Version
					//
					
					szVersionStart = szExtendedInfo + sizeof("IsClustered;Yes;Version;")-2;

					//
					// replace ';' with ':'
					//
					
					*szVersionStart++ = ':';
				}
				else
				{
					BidTrace0(ERROR_TAG _T("ssrp format\n"));

					continue;
				}

				//
				// skip "Is" part of "IsClustered"
				//

				szExtendedInfo += 2;

				//
				// replace ';' at the end of Clustered with ':'
				//

				szExtendedInfo[sizeof("Clustered")-1] = ':';
				
				char *szVersionEnd = strchr(szVersionStart,';');
				
				if( !szVersionEnd )
				{
					BidTrace0(ERROR_TAG _T("ssrp format\n"));

					continue;
				}

				*szVersionEnd = 0;

				int cVersion = static_cast<int>(szVersionEnd-szVersionStart);

				if( cVersion >= 16 )
				{
					BidTrace0(ERROR_TAG _T("ssrp format\n"));

					continue;
				}
				
				if( !IsValidVersionString(szVersionStart) )
				{
					BidTrace0(ERROR_TAG _T("ssrp format\n"));

					continue;
				}

				//
				// replace null terminating char with ';'
				//

				szServerStart[cRecordLen-1] = ';';

				int cExtendedInfo = static_cast<int>(szVersionEnd-szExtendedInfo);
				
				memmove( szServerStart+cRecordLen, szExtendedInfo, cExtendedInfo+1);

				cRecordLen += cExtendedInfo+1;

				Assert( !m_pCurrent );

				m_pCurrent = szVersionEnd+1;
			}
			else
			{
				Assert( !m_pCurrent );
				
				m_pCurrent = szInstanceEnd+1;
			}

			m_pSavedRecord = szServerStart;
			m_cSavedRecord = cRecordLen;

			BidTraceU0( SNI_BID_TRACE_ON, RETURN_TAG _T("true\n"));

			return true;
		}
	}

	int SsrpGetNext( __out_ecount_part(cwBuf, return) LPWSTR pwBuf, __in int cwBuf, __inout BOOL *pfMore, __in int timeout )
	{
		BidxScopeAutoSNI4( SNIAPI_TAG _T( "pwBuf: %p, cwBuf: %d, pfMore: %p, timeout: %d\n"),
					pwBuf, cwBuf, pfMore, timeout);

		Assert( !*pfMore );
		Assert( !IsSsrpFinished());
		
		int cBufTotal = 0;

		Assert( timeout > 0 || timeout == INFINITE);
		int timeleft = timeout;
		DWORD timestart=GetTickCount();
		bool fisFirst = true;

		while(1)
		{
			if( m_cSavedRecord )
			{
				Assert( m_pSavedRecord );
				
				if( cwBuf-cBufTotal < m_cSavedRecord )
				{
					Assert(cBufTotal);

					*pfMore = TRUE;
					
					BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("cBufTotal: %d, *pfMore: %d{BOOL)\n"), cBufTotal, *pfMore);

					return cBufTotal;
				}

				int cRet;
				
				cRet = MultiByteToWideChar(
								CP_ACP,         // code page
								MB_ERR_INVALID_CHARS,  // character-type options
								m_pSavedRecord, // string to map
								m_cSavedRecord,       // number of bytes in string
								pwBuf+cBufTotal,  // wide-character buffer
								m_cSavedRecord        // size of buffer
								);
				if( 0 >= cRet || MAX_PATH < cRet || m_cSavedRecord < cRet)
				{
					BidTrace0(ERROR_TAG _T("Invalid params in ASCII to Unicode\n"));
				}
				else
				{
					cBufTotal += cRet;
				}	
			

				m_cSavedRecord = 0;
				m_pSavedRecord = 0;
			}

			if(!fisFirst)
			{
				if (timeout != INFINITE)
				{
					timeleft = timeout -(GetTickCount() - timestart);
					if(timeleft <=0 || timeleft >timeout) 
					{
						BidTraceU3( SNI_BID_TRACE_ON, RETURN_TAG _T("Timeleft %d, cBufTotal: %d, *pfMore: %d{BOOL)\n"), timeleft, cBufTotal, *pfMore);
						return cBufTotal;
					}
				}				
			}
			else
				fisFirst = false;
			
			//BidTrace1(ERROR_TAG _T("timeleft:%d\n"),timeleft);
			if( !SsrpGetNextRecord(timeleft))
			{
				Assert( !m_cSavedRecord );
				Assert( !m_pSavedRecord );
				Assert( !m_pCurrent );

				BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("cBufTotal: %d, *pfMore: %d{BOOL)\n"), cBufTotal, *pfMore);

				return cBufTotal;
			}

			Assert( m_cSavedRecord );
			Assert( m_pSavedRecord );
			Assert( m_pCurrent );
		}
	}

	inline bool IsLanFinished() const
	{
		return m_fLanFinished;
	}

	inline void SetLanFinished()
	{
		Assert( !m_fLanFinished );
		m_fLanFinished = true;
	}

	int LanGetNext( __out_ecount_part(cwBuf, return) LPWSTR pwBuf, __in int cwBuf, __inout BOOL *pfMore )
	{
		BidxScopeAutoSNI3( SNIAPI_TAG _T( "pwBuf: %p, cwBuf: %d, pfMore: %p\n"),
					pwBuf, cwBuf, pfMore);

		Assert( !*pfMore );
		Assert( IsSsrpFinished());
		Assert( !IsLanFinished());

		int cBufTotal = 0;

		for( ;m_iEntry < m_nEntries; m_iEntry++ )
		{
			int cRecord;

			cRecord = (int) wcslenInWChars( (WCHAR *)m_pLanInfo[m_iEntry].sv100_name );

			// Once in a while we'll see a NULL character name returned from
			// the NETWORK.
			//
			if( !cRecord )
			{
				BidTrace0(ERROR_TAG _T("record len 0\n"));

				continue;
			}

			// inlude null terminator

			cRecord++;

			if( cwBuf-cBufTotal < cRecord )
			{
				*pfMore = TRUE;
				
				BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("cBufTotal: %d, *pfMore: %d{BOOL)\n"), cBufTotal, *pfMore);

				return cBufTotal;
			}

			memcpy( pwBuf+cBufTotal, m_pLanInfo[m_iEntry].sv100_name, cRecord*sizeof(pwBuf[0]));

			cBufTotal += cRecord;
		}

		Assert( m_iEntry == m_nEntries );

		SetLanFinished();
		
		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("cBufTotal: %d, *pfMore: %d{BOOL)\n"), cBufTotal, *pfMore);

		return cBufTotal;
	}
};
}

using namespace SSRP;

HANDLE SNIServerEnumOpenEx( LPWSTR pwszServer, 
							BOOL fExtendedInfo, 
							DWORD waittime_least, 
							DWORD waittimeout)
{
	BidxScopeAutoSNI4( SNIAPI_TAG _T("pwszServer: \"%ls\", ")
							  _T("fExtendedInfo: %d{BOOL}")
							  _T("waittime_least: %d")
							  _T("waittimeout: %d\n"), 
					pwszServer, fExtendedInfo, waittime_least, waittimeout);

	ServerEnum * pServerEnum = NULL;

	WCHAR wszServer[MAX_NAME_SIZE+1] = L"";
	if (pwszServer && wcslen(pwszServer) == 0)
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p\n"), NULL);

		return NULL;
	}
	else if(!pwszServer)
	{
		pwszServer = wszServer;
	}
	
	if( !_wcsicmp(pwszServer, L"(local)") || !_wcsicmp(pwszServer, L"."))
	{
		(void)StringCchCopyW( pwszServer, sizeof(pwszServer)/sizeof(WCHAR), L"localhost");
	}
	
	pServerEnum = NewNoX(gpmo) ServerEnum( fExtendedInfo ? true : false);

	if( !pServerEnum )
	{
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p\n"), NULL);

		return NULL;
	}

	WSADATA       wsadata;

	if( WSAStartup((WORD)0x0202, &wsadata) && WSAStartup((WORD)0x0101, &wsadata))
	{
		delete pServerEnum;

		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p\n"), NULL);

		return NULL;
	}

	if( !pServerEnum->Initialize( pwszServer,waittime_least, waittimeout ))
	{
		delete pServerEnum;
		
		WSACleanup();
		
		BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p\n"), NULL);

		return NULL;
	}

	BidTraceU1( SNI_BID_TRACE_ON, RETURN_TAG _T("%p\n"), pServerEnum);

	return pServerEnum;

}

HANDLE SNIServerEnumOpen( LPWSTR pwszServer, BOOL fExtendedInfo)
{
	return SNIServerEnumOpenEx(pwszServer, fExtendedInfo,SSRP_WAIT_TIME_LEAST ,SSRP_WAIT_TIMEOUT);  //  use default timeout value;
}


__success(return > 0) int SNIServerEnumReadEx(__in HANDLE handle, __out_ecount_part(cBuf, return)  LPWSTR pwBuf, __in int cBuf, __out BOOL *pfMore, __in int timeout)
{
	BidxScopeAutoSNI5( SNIAPI_TAG _T( "handle: %p, pwBuf: %p, cBuf: %d, pfMore: %p{BOOL*}, timeout: %d\n"), 
					handle, pwBuf, cBuf, pfMore, timeout);

	ServerEnum * pServerEnum = (ServerEnum *)handle;

	*pfMore = FALSE;
	
	int cBufTotal = 0;

	if( NULL == pServerEnum )
	{
		//this is assert is used to catch unexpected coding error/usage .
		Assert( 0 && " pServerEnum is NULL\n" );
		BidTrace0( ERROR_TAG _T("pSererEnum is NULL\n") );
		
		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("bytes: %d, *pfMore: %d{BOOL}\n"), cBufTotal, *pfMore);
		return 0;
	}

	if( !pServerEnum->IsSsrpFinished() )
	{
		cBufTotal = pServerEnum->SsrpGetNext( pwBuf, cBuf, pfMore, timeout );

		if( TRUE == *pfMore )
		{
			BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("bytes: %d, *pfMore: %d{BOOL}\n"), cBufTotal, *pfMore);

			return cBufTotal;
		}
		
		pServerEnum->SetSsrpFinished();

		//
		// reading will continue with lan enum
		//
	}

	Assert( pServerEnum->IsSsrpFinished() );
	Assert( cBufTotal <= cBuf );

	if( !pServerEnum->IsLanFinished())
	{
		cBufTotal += pServerEnum->LanGetNext( pwBuf+cBufTotal, cBuf-cBufTotal, pfMore);

		BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("bytes: %d, *pfMore: %d{BOOL}\n"), cBufTotal, *pfMore);

		return cBufTotal;
	}

	BidTraceU2( SNI_BID_TRACE_ON, RETURN_TAG _T("Bytes Read: %d, *pfMore: %d{BOOL}\n"), cBufTotal, *pfMore);

	return cBufTotal;
}

__success(return > 0) 
int SNIServerEnumRead(__in HANDLE handle, __out_ecount_part(cBuf, return)  LPWSTR pwBuf, __in int cBuf, __out BOOL *pfMore)
{
	return SNIServerEnumReadEx(handle, pwBuf, cBuf, pfMore, DEFAULT_SSRP_ENUM_TIMEOUT);
}

void SNIServerEnumClose(__inout_opt HANDLE handle)
{
	BidxScopeAutoSNI1( SNIAPI_TAG _T( "handle: %p\n"), handle);

    delete (ServerEnum *) handle;

	WSACleanup();
}

