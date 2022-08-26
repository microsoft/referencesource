//****************************************************************************
//              Copyright (c) Microsoft Corporation.
//
// @File: sni_rc.h
// @Owner: petergv, nantu
// @test: milu
//
// <owner current="true" primary="true">petergv</owner>
// <owner current="true" primary="false">nantu</owner>
//
// Purpose:
//	Identifiers of SNI resource strings.  
//
// Notes:
//	SNI reservers th erange 50000-54999 within the client-side 
//	SNAC (sqlncli).  
//
// @EndHeader@
//****************************************************************************

//	Beginning of SNI range.  
//
#define SNI_STRING_BASE				50000U


//	Beginning of SNI provider name range.  
//
#define SNI_STRING_PROV_BASE		(SNI_STRING_BASE + 0)

#define SNI_STRING_ID_FROM_PROV(uiProv)	(SNI_STRING_PROV_BASE + uiProv)

//	!!! Important !!!
//	These ID's must match:
//	- the ProviderNum enum defined in sni.hpp.  
//
#define SNI_STRING_PROV_HTTP		(SNI_STRING_PROV_BASE + 0)
#define SNI_STRING_PROV_NP			(SNI_STRING_PROV_BASE + 1)
#define SNI_STRING_PROV_SESSION		(SNI_STRING_PROV_BASE + 2)
#define SNI_STRING_PROV_SIGN		(SNI_STRING_PROV_BASE + 3)
#define SNI_STRING_PROV_SM			(SNI_STRING_PROV_BASE + 4)
#define SNI_STRING_PROV_SMUX		(SNI_STRING_PROV_BASE + 5)
#define SNI_STRING_PROV_SSL			(SNI_STRING_PROV_BASE + 6)
#define SNI_STRING_PROV_TCP			(SNI_STRING_PROV_BASE + 7)
#define SNI_STRING_PROV_VIA			(SNI_STRING_PROV_BASE + 8)
#define SNI_STRING_PROV_MAX			(SNI_STRING_PROV_BASE + 9)
#define SNI_STRING_PROV_INVALID		(SNI_STRING_PROV_BASE + 10)

//	Beginning of SNI error message range.  
//
#define SNI_STRING_ERROR_BASE		(SNI_STRING_BASE + 100)

#include <intsafe.h>
__inline DWORD SniErrorIdFromStringId (DWORD dwErrorId)
{
	if (FAILED (DWordSub(dwErrorId,SNI_STRING_ERROR_BASE,&dwErrorId)))
		return 0;

	return dwErrorId;
}
#define SNI_ERROR_ID_FROM_STRING_ID(dwErrorId)	(SniErrorIdFromStringId(dwErrorId))

//	!!! Important !!!
//	These ID's must match:
//	- the SNI errors in sqlncli's local.rc file, 
//	- the errors in the SNI_Error_String arry in sni_error.cpp.  
//
#define SNIE_0						(SNI_STRING_ERROR_BASE + 0)
#define SNIE_SYSTEM 				SNIE_0

#define SNIE_1						(SNI_STRING_ERROR_BASE + 1)
#define SNIE_2						(SNI_STRING_ERROR_BASE + 2)
#define SNIE_3						(SNI_STRING_ERROR_BASE + 3) 
#define SNIE_4						SNIE_SYSTEM
#define SNIE_5						(SNI_STRING_ERROR_BASE + 5)
#define SNIE_6						(SNI_STRING_ERROR_BASE + 6)
#define SNIE_7						(SNI_STRING_ERROR_BASE + 7)
#define SNIE_8						(SNI_STRING_ERROR_BASE + 8)
#define SNIE_9						(SNI_STRING_ERROR_BASE + 9)
#define SNIE_10						SNIE_SYSTEM
#define SNIE_11						(SNI_STRING_ERROR_BASE + 11)
#define SNIE_12						(SNI_STRING_ERROR_BASE + 12)
#define SNIE_13						(SNI_STRING_ERROR_BASE + 13)
#define SNIE_14						(SNI_STRING_ERROR_BASE + 14)
#define SNIE_15						(SNI_STRING_ERROR_BASE + 15)
#define SNIE_16						(SNI_STRING_ERROR_BASE + 16)
#define SNIE_17						(SNI_STRING_ERROR_BASE + 17)
#define SNIE_18						(SNI_STRING_ERROR_BASE + 18)
#define SNIE_19						(SNI_STRING_ERROR_BASE + 19)
#define SNIE_20						(SNI_STRING_ERROR_BASE + 20)
#define SNIE_21						(SNI_STRING_ERROR_BASE + 21)
#define SNIE_22						(SNI_STRING_ERROR_BASE + 22)
#define SNIE_23						(SNI_STRING_ERROR_BASE + 23)
#define SNIE_24						(SNI_STRING_ERROR_BASE + 24)
#define SNIE_25						(SNI_STRING_ERROR_BASE + 25)
#define SNIE_26						(SNI_STRING_ERROR_BASE + 26)
#define SNIE_27						(SNI_STRING_ERROR_BASE + 27)
#define SNIE_28						(SNI_STRING_ERROR_BASE + 28)
#define SNIE_29						(SNI_STRING_ERROR_BASE + 29)
#define SNIE_30						(SNI_STRING_ERROR_BASE + 30)
#define SNIE_31						(SNI_STRING_ERROR_BASE + 31)
#define SNIE_32						(SNI_STRING_ERROR_BASE + 32)
#define SNIE_33						(SNI_STRING_ERROR_BASE + 33)
#define SNIE_34						(SNI_STRING_ERROR_BASE + 34)
#define SNIE_35						(SNI_STRING_ERROR_BASE + 35)
#define SNIE_36						(SNI_STRING_ERROR_BASE + 36)
#define SNIE_37						(SNI_STRING_ERROR_BASE + 37)
#define SNIE_38						(SNI_STRING_ERROR_BASE + 38)
#define SNIE_39						(SNI_STRING_ERROR_BASE + 39)
#define SNIE_40						(SNI_STRING_ERROR_BASE + 40)
#define SNIE_41						(SNI_STRING_ERROR_BASE + 41)
#define SNIE_42						(SNI_STRING_ERROR_BASE + 42)
#define SNIE_43						(SNI_STRING_ERROR_BASE + 43)
#define SNIE_44						(SNI_STRING_ERROR_BASE + 44)
#define SNIE_45						(SNI_STRING_ERROR_BASE + 45)
#define SNIE_46						(SNI_STRING_ERROR_BASE + 46)
#define SNIE_47						(SNI_STRING_ERROR_BASE + 47)
#define SNIE_48						(SNI_STRING_ERROR_BASE + 48)
#define SNIE_49						(SNI_STRING_ERROR_BASE + 49)

#define SNIE_50						(SNI_STRING_ERROR_BASE + 50)
#define SNIE_LocalDB				SNIE_50

#define SNIE_51						(SNI_STRING_ERROR_BASE + 51)
#define SNIE_52						(SNI_STRING_ERROR_BASE + 52)
#define SNIE_53						(SNI_STRING_ERROR_BASE + 53)
#define SNIE_54						(SNI_STRING_ERROR_BASE + 54)
#define SNIE_55						(SNI_STRING_ERROR_BASE + 55)
#define SNIE_56						(SNI_STRING_ERROR_BASE + 56)
#define SNIE_57						(SNI_STRING_ERROR_BASE + 57)

#define SNIE_58						(SNI_STRING_ERROR_BASE + 58)
#define SNIE_59						(SNI_STRING_ERROR_BASE + 59)
#define SNIE_60						(SNI_STRING_ERROR_BASE + 60)
#define SNIE_61						(SNI_STRING_ERROR_BASE + 61)

#define SNIE_MAX					SNIE_61

#define SNI_STRING_IS_SYSTEM_ERROR(dwErrorId)	(dwErrorId == SNIE_SYSTEM)

#define SNI_STRING_IS_LOCALDB_ERROR(dwErrorId)	(dwErrorId == SNIE_LocalDB)

