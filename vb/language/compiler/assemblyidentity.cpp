//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the support for dealing with assembly identities in the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#ifdef FEATURE_CORESYSTEM
#pragma warning(disable : 4996)
#endif

// Access the helper class in the CLR.
PCSTR const createAssemblyNameObjectEntryPoint = "CreateAssemblyNameObject";
typedef HRESULT (__stdcall *PfnCreateAssemblyNameObject)(LPASSEMBLYNAME *ppAssemblyNameObj, PCWSTR szAssemblyName, DWORD dwFlags, LPVOID pvReserved);
static PfnCreateAssemblyNameObject pfnCreateAssemblyNameObject = NULL;

static HRESULT LoadCreateAssemblyNameObjectFunction()
{
    HRESULT hr = S_OK;
#ifdef FEATURE_CORESYSTEM
    // its statically linked, but we use a function pointer to match what happens in the dynamic case
    pfnCreateAssemblyNameObject = (PfnCreateAssemblyNameObject) &CreateAssemblyNameObject;
#else
    if( pfnCreateAssemblyNameObject == NULL )
    {
        hr = LegacyActivationShim::GetRealProcAddress( createAssemblyNameObjectEntryPoint, ( void** )&pfnCreateAssemblyNameObject );
    }
#endif
    return( hr );
}

bool GetValidPathForFile
(
    _In_z_ LPCWSTR filename,
    _In_z_ LPCWSTR relPath,
    StringBuffer &NewPath
);

void AssemblyIdentity::SetAssemblyIdentity
(
    _In_z_ STRING *AssemblyName,
    _In_z_ STRING *Locale,
    BYTE *pvPublicKey,
    ULONG cbPublicKey,
    USHORT MajorVersion,
    USHORT MinorVersion,
    USHORT BuildNumber,
    USHORT RevisionNumber,
    DWORD flags
)
{
    if (m_AssemblyName)
    {
        VSFAIL("Why are we setting the Assembly Identity twice? Please contact Microsoft");
        ResetAssemblyIdentity(m_pCompiler, m_pCompilerHost, m_pCompilerProject);
    }

    m_AssemblyName   = AssemblyName;
    m_Locale         = Locale;

    m_cbPublicKey    = cbPublicKey;
	if (cbPublicKey != 0)
	{
        m_PublicKeyBlob  = new (zeromemory) BYTE[cbPublicKey];
        memcpy(m_PublicKeyBlob, pvPublicKey, cbPublicKey);
    }
	else 
	{
        m_PublicKeyBlob = NULL;
    }
		
    m_fPublicKeyAvailable = true;

    m_MajorVersion   = MajorVersion;
    m_MinorVersion   = MinorVersion;
    m_BuildNumber    = BuildNumber;
    m_RevisionNumber = RevisionNumber;
    m_flags          = flags;
}

// Note: Caller responsible for freeing the returned string
WCHAR *AssemblyIdentity::GetPublicKeyTokenString(ULONG *pPKStringLength)
{
    WCHAR *wszPublicKeyTokenName = NULL;
    *pPKStringLength = 0;

    if (GetPublicKeySize() > 0 &&
        // Microsoft 9/18/2004:  Added overflow checks
        VBMath::TryAdd(GetPublicKeySize(), 1) &&
         VBMath::TryMultiply((GetPublicKeySize() + 1), sizeof(WCHAR)))
    {
        wszPublicKeyTokenName = new (zeromemory )WCHAR[(GetPublicKeySize() + 1) * sizeof(WCHAR)];
        VSASSERT(wszPublicKeyTokenName, "new failed");

        if (m_flags & afPublicKey)
        {
            BYTE *pbStrongNameToken = NULL;
            ULONG cbStrongNameToken = 0;

#ifndef FEATURE_CORESYSTEM
            IfFalseThrow(LegacyActivationShim::StrongNameTokenFromPublicKey((BYTE*)GetPublicKeyBlob(), GetPublicKeySize(), &pbStrongNameToken, &cbStrongNameToken));
#else
            IfFalseThrow(::StrongNameTokenFromPublicKey((BYTE*)GetPublicKeyBlob(), GetPublicKeySize(), &pbStrongNameToken, &cbStrongNameToken));
#endif
            VSASSERT(pbStrongNameToken != NULL && cbStrongNameToken > 0, "Bad PK token");

            BinaryToUnicode(pbStrongNameToken, cbStrongNameToken, wszPublicKeyTokenName);
            wszPublicKeyTokenName[cbStrongNameToken * sizeof(WCHAR)] = UCH_NULL;

#ifndef FEATURE_CORESYSTEM
            LegacyActivationShim::StrongNameFreeBuffer(pbStrongNameToken);
#else
            ::StrongNameFreeBuffer(pbStrongNameToken);
#endif
        }
        else
        {
            BinaryToUnicode(GetPublicKeyBlob(), GetPublicKeySize(), wszPublicKeyTokenName);
            wszPublicKeyTokenName[GetPublicKeySize() * sizeof(WCHAR)] = UCH_NULL;
        }

        *pPKStringLength = (ULONG)wcslen(wszPublicKeyTokenName);
    }

    return wszPublicKeyTokenName;
}

STRING *AssemblyIdentity::GetAssemblyInfoString()
{
    if (!m_pstrAnnotated----semblyName && m_AssemblyName)
    {
#define FMT_ASSEM_LONG L"%ws, Version=%hu.%hu.%hu.%hu, Culture=%ws, PublicKeyToken=%ws%ws"
        ULONG PKStringLength = 0;
        WCHAR *wszPublicKeyTokenName = GetPublicKeyTokenString(&PKStringLength);

        // Alloc more mem than we could possibly need to store the string
        // Microsoft 9/18/2004:  Don't need to perform overflow checks; assembly names are already constrained to be certain lengths.
        size_t cchNeeded = StringPool::StringLength(m_AssemblyName) +   // Name
            4 * 6 +  // Version (four 16-bit numbers, max 65536) + DOT
            (m_Locale ? StringPool::StringLength(m_Locale) : 7) + // Locale if present, "neutral" if not
            (PKStringLength > 0 ? (PKStringLength * 2) : 4) + // Originator converted to a Unicode hex string, or "null"
            ((m_flags & afRetargetable) ? 18 : 0) + // ", Retargetable=Yes" if present
            DIM(FMT_ASSEM_LONG) + // The format string itself
                    1;                         // Terminating NUL

        TEMPBUF(wszBuf, WCHAR *, cchNeeded * sizeof(WCHAR));
        IfNullThrow(wszBuf);

#pragma warning (push)
#pragma warning (disable:6387) // wszBuf NULL check is above
        if (FAILED(StringCchPrintfW(wszBuf, cchNeeded,
                 FMT_ASSEM_LONG,
                 m_AssemblyName,
                 m_MajorVersion,
                 m_MinorVersion,
                 m_BuildNumber,
                 m_RevisionNumber,
                 (m_Locale && m_Locale[0]) ? m_Locale : L"neutral",
                 PKStringLength > 0 ? wszPublicKeyTokenName : L"null",
                 (m_flags & afRetargetable) ? L", Retargetable=Yes" : L"")))
        {
            VSFAIL("Bad call to StringCchPrintfW()");
        }
#pragma warning (pop)

#undef FMT_ASSEM_LONG

        m_pstrAnnotated----semblyName = m_pCompiler->AddString(wszBuf);
        delete[] wszPublicKeyTokenName;
    }

    return m_pstrAnnotated----semblyName;
}

STRING *AssemblyIdentity::GetAssemblyIdentityString()
{
    if (!m_CLRAssemblyName && m_AssemblyName)
    {
        if (GetCompilerHost()->GetRuntimeVersion() < RuntimeVersion2)
        {
            // Special handling for pre-whidbey releases since the latest assembly identity
            // support is not available in pre-whidbey releases.

            m_CLRAssemblyName =
                MakeStrongAssemblyName(
                    m_pCompiler,
                    m_AssemblyName,
                    m_Locale,
                    GetPublicKeyBlob(),
                    GetPublicKeySize(),
                    NULL,
                    0,
                    m_MajorVersion,
                    m_MinorVersion,
                    m_BuildNumber,
                    m_RevisionNumber,
                    m_flags);
        }
        else
        {
            HRESULT hr = S_OK;
            CComPtr<IAssemblyName> pAssemblyName;

            AssertIfNull(m_pCompiler->GetCreateAssemblyNameObject());

            IfFailGo(m_pCompiler->GetCreateAssemblyNameObject()(&pAssemblyName, NULL, 0, NULL));
            IfNullGo(pAssemblyName);

            // Name
            IfFailGo(pAssemblyName->SetProperty(ASM_NAME_NAME, (LPVOID)m_AssemblyName, ((UINT)StringPool::StringLength(m_AssemblyName) + 1) * sizeof(WCHAR)));

            // Version
            IfFailGo(pAssemblyName->SetProperty(ASM_NAME_MAJOR_VERSION,   (LPVOID)&m_MajorVersion, sizeof(m_MajorVersion)));
            IfFailGo(pAssemblyName->SetProperty(ASM_NAME_MINOR_VERSION,   (LPVOID)&m_MinorVersion, sizeof(m_MinorVersion)));
            IfFailGo(pAssemblyName->SetProperty(ASM_NAME_BUILD_NUMBER,    (LPVOID)&m_BuildNumber, sizeof(m_BuildNumber)));
            IfFailGo(pAssemblyName->SetProperty(ASM_NAME_REVISION_NUMBER, (LPVOID)&m_RevisionNumber, sizeof(m_RevisionNumber)));

            // Culture
            if (!m_Locale || m_Locale[0] == UCH_NULL)
            {
                IfFailGo(pAssemblyName->SetProperty(ASM_NAME_CULTURE, L"", sizeof(WCHAR)));
            }
            else
            {
                IfFailGo(pAssemblyName->SetProperty(ASM_NAME_CULTURE, (LPVOID)m_Locale, ((UINT)StringPool::StringLength(m_Locale) + 1) * sizeof(WCHAR)));
            }

            // Public Key (or Token)
            if (GetPublicKeySize() == 0)
            {
                IfFailGo(pAssemblyName->SetProperty(ASM_NAME_NULL_PUBLIC_KEY_TOKEN, NULL, 0));
            }
            else
            {
                if (m_flags & afPublicKey)
                {
                    IfFailGo(pAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY, GetPublicKeyBlob(), GetPublicKeySize()));
                }
                else
                {
                    IfFailGo(pAssemblyName->SetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, GetPublicKeyBlob(), GetPublicKeySize()));
                }
            }

            // Retargetable?
            if (m_flags & afRetargetable)
            {
                BOOL fRetargetable = TRUE;
                IfFailGo(pAssemblyName->SetProperty(ASM_NAME_RETARGET, &fRetargetable, sizeof(fRetargetable)));
            }

            // ContentType
            if (m_flags & afContentType_Mask)
            {
                DWORD contentType = AfContentType(m_flags);
                pAssemblyName->SetProperty(ASM_NAME_CONTENT_TYPE, &contentType, sizeof(contentType));
            }

            DWORD cchDisplayName = 0;
            hr = pAssemblyName->GetDisplayName(NULL, &cchDisplayName, 0); // 0 == default == ASM_DISPLAYF_VERSION | ASM_DISPLAYF_CULTURE | ASM_DISPLAYF_PUBLIC_KEY_TOKEN | ASM_DISPLAYF_RETARGET

            if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                VSFAIL("Failed to GetDisplayName");
            }
            else
            {
                TEMPBUF(wszDisplayName, WCHAR *, (cchDisplayName + 1) * sizeof(WCHAR));
                IfNullGo(wszDisplayName);

                IfFailGo(pAssemblyName->GetDisplayName(wszDisplayName, &cchDisplayName, 0));
                m_CLRAssemblyName = m_pCompiler->AddString(wszDisplayName);
            }
        }
    }

Error:
    return m_CLRAssemblyName;
}

STRING *AssemblyIdentity::GetAssemblyVersion()
{
    if (!m_pstrComputedVersion)
    {
        WCHAR *wszBuf = NULL;
        NorlsAllocator nraTemp(NORLSLOC);

        // Microsoft:  


        int cchNeeded = 4 * 6 +                     // Version (four 16-bit numbers, max 65536) + DOT
                        1;                          // Terminating NUL

        wszBuf = (WCHAR *)nraTemp.Alloc(cchNeeded * sizeof(WCHAR));

        if (FAILED(StringCchPrintfW(wszBuf, cchNeeded,
                L"%hu.%hu.%hu.%hu",
                m_MajorVersion,
                m_MinorVersion,
                m_BuildNumber,
                m_RevisionNumber)))
        {
            VSFAIL("Bad call to StringCchPrintfW()");
        }

        m_pstrComputedVersion = m_pCompiler->AddString(wszBuf);
    }

    return m_pstrComputedVersion;
}

// Version computation code from ALink sources.
// 

// NOTE: rev is optional (may be NULL), but build is NOT
HRESULT MakeVersionFromTime(WORD *rev, WORD *build)
{
    tm temptime;
    time_t current, initial;
    double diff;
    bool BadTime = false;

    // Get the current time
    time( &current);
    if (current < 0 && rev)
        // We have a date previous to 1970, and they want a revision number
        // Can't do it, so just fail here
        return E_FAIL;
    else if (current < (60 * 60 * 24) && !rev) {
        // Keep adding a day until we are just above 1 day
        // Thus arriving at the number of seconds since midnight (GMT) plus 1 day
        // so it will always be after 1/1/1970 even in local time
        while (current < (60 * 60 * 24))
            current += (60 * 60 * 24);
        BadTime = true;
    }
    // Use local time
    temptime = *localtime( &current);
    if (temptime.tm_year < 100 && !rev) {
        // If we only want a build number, then
        // make sure the year is greater than 2000
        temptime.tm_year = 100;
        BadTime = true;
    }
    // convert to useful struct
    current = mktime(&temptime);
    if (current == (time_t)-1)
        return E_FAIL;

    // Make Jan. 1 2000, 00:00:00.00
    memset(&temptime, 0, sizeof(temptime));
    temptime.tm_year = 100; // 2000 - 1900
    temptime.tm_mday = 1; // Jan. 1
    temptime.tm_isdst = -1; // Let the CRT figure out if it is Daylight savings time

    {
        // See if the user wants to 'override' the start date
        HKEY key;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\ALink", 0, KEY_QUERY_VALUE, &key)) {
            DWORD rval = 0;
            DWORD type;
            DWORD size = sizeof(DWORD);
            if (ERROR_SUCCESS == RegQueryValueEx(key, L"VersionStartDate", NULL, &type, (BYTE*)&rval, &size) &&
                type == REG_DWORD && rval != 0) {
                temptime.tm_year = (rval >> 16);
                temptime.tm_mon = ((rval >> 8) & 0xF);
                temptime.tm_mday = (rval & 0x1F);
            }
            RegCloseKey(key);
        }
    }
    initial = mktime(&temptime);

    // Get difference in seconds
    diff = (double)current - (double)initial;
    if (diff < 0.0 && rev) {
        return E_FAIL;
    }

    if (rev != NULL) {
        AssertIfFalse(diff >= 0.0);
        *rev = (WORD)floor(diff / (60 * 60 * 24)); // divide by 60 seconds, 60 minutes, 24 hours, to get difference in days
    }

    if (build != NULL) {
        if (diff < 0.0) {
            diff = fmod(diff, 60 * 60 * 24) + (60 * 60 * 24); // make it positive by moving it and adding 1 day
            AssertIfFalse(diff >= 0.0);
            BadTime = true;
        }
        *build = (WORD)(fmod(diff, 60 * 60 * 24) / 2); // Get the seconds since midnight div 2 for 2 second granularity
        AssertIfFalse(*build != 65535);
    } else {
        return E_INVALIDARG;
    }

    return BadTime ? E_FAIL : S_OK;
}

bool MakeVersionFromString
(
    _In_z_ STRING *pstrVersion,
    USHORT *pMajorVersion,
    USHORT *pMinorVersion,
    USHORT *pBuildNumber,
    USHORT *pRevisionNumber
)
{
    // String encoded as: "Major[.Minor[.Build[.Revision]]]"

    WCHAR *str = pstrVersion;
    WCHAR *internal = pstrVersion;
    WCHAR *end = NULL;
    size_t len = StringPool::StringLength(pstrVersion);
    HRESULT hr = S_OK;

    ULONG val[4] = {0,0,0,0};

#pragma warning(disable:22004)//,"i is properly bound.")
    for (int i = 0; i < 4; i++) {
        if (i > 1 && *internal == L'*' && internal + 1 == str + len) {
            // Auto-gen
            WORD rev, bld;
            hr = MakeVersionFromTime( i == 2 ? &rev : NULL, &bld);
            if (hr != S_OK)
                return false; // BadTime = true;
            else if (FAILED(hr))
                return true; // goto SUCCEED;
            // all done
            val[3] = bld;
            if (i == 2)
                val[2] = rev;
            break;
        }

        if (iswspace(*internal) || *internal == L'-' || *internal == L'+')
            // we don't allow these
            return false; // goto FAIL;
        val[i] = wcstoul( internal, &end, 10); // Allow only decimal
        if (end == internal || val[i] >= USHRT_MAX)  // 65535 is not valid (because of metadata restrictions)
            // We didn't parse anything, or it wasn't valid
            return false; // goto FAIL;
        if (end == str + len)
            // we're done
            break;
        if (*end != L'.' || i == 3)
            // Need a dot to continue (or this should have been the end)
            return false; // goto FAIL;
        internal = end + 1;
    }
#pragma warning(default:22004)//,"i is properly bound.")

    *pMajorVersion = (unsigned short)val[0];
    *pMinorVersion = (unsigned short)val[1];
    *pBuildNumber = (unsigned short)val[2];
    *pRevisionNumber = (unsigned short)val[3];

    return true;
}

void AssemblyIdentity::SetAssemblyVersion
(
    _In_opt_z_ STRING *pstrVersion
)
{
    if (StringPool::IsEqual(pstrVersion, m_pstrVersion) &&
        !wcsstr(pstrVersion, L"*"))
    {
        return;
    }

    ClearCachedValues();

    m_pstrVersion = pstrVersion;

    m_MajorVersion = 0;
    m_MinorVersion = 0;
    m_BuildNumber = 0;
    m_RevisionNumber = 0;

    if (pstrVersion == NULL)
    {
        m_fErrorInVersionString = true;
        return;
    }

    m_fErrorInVersionString =
        !MakeVersionFromString(
            pstrVersion,
            &m_MajorVersion,
            &m_MinorVersion,
            &m_BuildNumber,
            &m_RevisionNumber);
}

bool AssemblyIdentity::IsAssemblyWithSameNameAndPKToken(AssemblyIdentity *pThatAssemblyIdentity)
{
    bool ReturnValue = false;

    if (IsStrongNameAssembly() != pThatAssemblyIdentity->IsStrongNameAssembly() ||
        !GetAssemblyName() ||
        !pThatAssemblyIdentity->GetAssemblyName() ||
        !StringPool::IsEqual(GetAssemblyName(), pThatAssemblyIdentity->GetAssemblyName()))
    {
        return ReturnValue;
    }

    ULONG ThisAssemblyPKStringLength = 0;
    WCHAR *ThisAssemblyPKSring = GetPublicKeyTokenString(&ThisAssemblyPKStringLength);

    ULONG ThatAssemblyPKStringLength = 0;
    WCHAR *ThatAssemblyPKSring = pThatAssemblyIdentity->GetPublicKeyTokenString(&ThatAssemblyPKStringLength);

    if (ThisAssemblyPKStringLength == ThatAssemblyPKStringLength &&
        !memcmp(ThisAssemblyPKSring, ThatAssemblyPKSring, ThisAssemblyPKStringLength))
    {
        ReturnValue = true;
    }

    delete[] ThisAssemblyPKSring;
    delete[] ThatAssemblyPKSring;

    return ReturnValue;
}

//============================================================================
// Compares two assemblies to determine if they are unifiable or not. Generates
// warnings or errors in the process of doing that.
//
// Also when it returns ACR_NonEquivalent, return whether non equivalent is due
// to only a lower version.
//============================================================================
AssemblyComparisonResult AssemblyIdentity::CompareAssemblies
(
    AssemblyIdentity *pDirectlyReferencedAssembly,
    AssemblyIdentity *pInDirectlyReferencedAssembly,
    ERRID *pOptionalErrorID,
    StringBuffer *pOptionaErrorMessage,
    bool *pNonEquivalentOnlyDueToLowerVersion,
    Compiler *pCompiler,
    CompilerHost *pCompilerHost
)
{
    AssemblyComparisonResult CompareResults = ACR_NonEquivalent;

    if (pNonEquivalentOnlyDueToLowerVersion)
    {
        *pNonEquivalentOnlyDueToLowerVersion = false;
    }

    // Backwards compatibility with the Everett CLR
    if (pCompilerHost->GetRuntimeVersion() >= RuntimeVersion2)
    {
        HRESULT hr = S_OK;
        STRING *pDirectlyReferencedAssemblyID   = pDirectlyReferencedAssembly->GetAssemblyIdentityString();
        STRING *pInDirectlyReferencedAssemblyID = pInDirectlyReferencedAssembly->GetAssemblyIdentityString();

        // If the string is the same, then there is no reason to call fusion's CompareAssemblyIdentity API cause
        // we know the two assemblies are exactly the same (doesn't really matter if they are FX assemblies or not).
        if (pDirectlyReferencedAssemblyID == pInDirectlyReferencedAssemblyID)
        {
            return ACR_EquivalentFullMatch;
        }

        BOOL Equivalent = false;
        LONG VersionDifference = pDirectlyReferencedAssembly->CompareAssemblyVersion(pInDirectlyReferencedAssembly);

        AssertIfNull(pCompiler->GetCompareAssemblyIdentity());
#ifndef FEATURE_CORESYSTEM
        // VSWhidbey #594242: Pass indirectly referenced assembly ID first to make sure Retargetable references are handled properly.
        hr = pCompiler->GetCompareAssemblyIdentity()(pInDirectlyReferencedAssemblyID,
                                             true,      // Unify up to this version.
                                             pDirectlyReferencedAssemblyID,
                                             true,      // Unify up to this version.
                                             &Equivalent,
                                             &CompareResults);
#else
        CComPtr<IAssemblyName> pDirectlyReferencedAssemblyObject ;
        ParseAssemblyString(pDirectlyReferencedAssemblyID, &pDirectlyReferencedAssemblyObject);

        CComPtr<IAssemblyName> pInDirectlyReferencedAssemblyObject ;
        ParseAssemblyString(pInDirectlyReferencedAssemblyID, &pInDirectlyReferencedAssemblyObject);
        
        hr = CompareNoFusion(pInDirectlyReferencedAssemblyObject, pDirectlyReferencedAssemblyObject, Equivalent, CompareResults);
#endif

        if (FAILED(hr))
        {
            VSFAIL("Assembly Identity Compare failed!");
            CompareResults = ACR_NonEquivalent;
        }
        else
        {
            if (!Equivalent)
            {
                CompareResults = ACR_NonEquivalent;
            }
            else
            {
                switch (CompareResults)
                {
                    case ACR_EquivalentFullMatch:
                    case ACR_EquivalentFXUnified:
                        // No warnings or errors.
                        break;

                    case ACR_EquivalentUnified:
                    case ACR_EquivalentWeakNamed:
                        {
                            if (VersionDifference < 0)
                            {
                                // Error: Project has to reference D2.0
                                if (pOptionalErrorID)
                                {
                                    *pOptionalErrorID = ERRID_SxSIndirectRefHigherThanDirectRef1;
                                }

                                if (pOptionaErrorMessage)
                                {
                                    ResLoadStringRepl(ERRID_SxSIndirectRefHigherThanDirectRef4,
                                                      pOptionaErrorMessage,
                                                      pInDirectlyReferencedAssembly->GetAssemblyName(),
                                                      pInDirectlyReferencedAssembly->GetAssemblyVersion(),
                                                      L"|1",
                                                      pDirectlyReferencedAssembly->GetAssemblyVersion());
                                }

                                CompareResults = ACR_NonEquivalent;

                                if (pNonEquivalentOnlyDueToLowerVersion)
                                {
                                    *pNonEquivalentOnlyDueToLowerVersion = true;
                                }
                            }
                        }

                        break;

                    case ACR_NonEquivalentVersion:
                    case ACR_NonEquivalent:
                        break;

                    case ACR_Unknown:
                    default:
                        VSFAIL("Unknown ACR value");
                        break;
                }
            }
        }
    }
    else
    {
        // Everett behaviour.
        CompareResults =
            StringPool::IsEqual(pDirectlyReferencedAssembly->GetAssemblyName(), pInDirectlyReferencedAssembly->GetAssemblyName()) ?
                ACR_EquivalentFullMatch :
                ACR_NonEquivalent;
    }

    return CompareResults;
}

#ifdef FEATURE_CORESYSTEM
HRESULT AssemblyIdentity::CompareNoFusion(
        IAssemblyName* panRef,
        IAssemblyName* panDef,
        BOOL& Equivalent, 
        AssemblyComparisonResult& result)
{
    HRESULT hr = S_OK;
    bool fStronglyNamedRef = false;
    bool fStronglyNamedDef = false;
    DWORD cbTemp;

    if (!panRef || !panDef)
    {
        hr = E_INVALIDARG;
        goto EXIT;
    }

    hr = panRef->IsEqual(panDef, ASM_CMPF_NAME);
    if (FAILED(hr))
    {
        goto EXIT;
    }
    if (hr == S_FALSE)
    {
        // Not equivalent names, so nothing else matters
        hr = S_OK;
        goto EXIT;
    }

    // Special case for "mscorlib"
    const WCHAR szMscorlib[] = L"mscorlib";
    WCHAR szMscorlibBuffer [lengthof(szMscorlib)];
    cbTemp = sizeof(szMscorlibBuffer); // cbTemp should include the terminating NULL
    hr = panRef->GetProperty( ASM_NAME_NAME, szMscorlibBuffer, &cbTemp);
    if (hr == S_OK && cbTemp == sizeof(szMscorlib) && 0 == _wcsicmp(szMscorlibBuffer, szMscorlib))
    {
        // both names are "mscorlib", as far as the runtime is concerned, they're identical to the one and only mscorlib
        Equivalent = true; 
        result = ACR_EquivalentFullMatch;
        goto EXIT;
    }

    cbTemp = 0;
    hr = panRef->GetProperty( ASM_NAME_PUBLIC_KEY_TOKEN, NULL, &cbTemp);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        fStronglyNamedRef = true;
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        goto EXIT;
    }

    cbTemp = 0;
    hr = panDef->GetProperty( ASM_NAME_PUBLIC_KEY_TOKEN, NULL, &cbTemp);
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        fStronglyNamedDef = true;
        hr = S_OK;
    }
    else if (FAILED(hr))
    {
        goto EXIT;
    }
#ifdef DEBUG
    else
    {
        // There's either no public key token specified (includes no public key)
        // Or the name explicitly states NO public key (PublicKeyToken=NULL)?
        // Since this is a def it should never be 'not specified'
        hr = panDef->GetProperty( ASM_NAME_NULL_PUBLIC_KEY_TOKEN, NULL, NULL);
        ASSERT(hr == S_OK);
    }
#endif // DEBUG

    // If the ref has a key then the def has to have one to match
    if (fStronglyNamedRef && !fStronglyNamedDef)
        goto EXIT;

    if (fStronglyNamedDef || fStronglyNamedRef)
    {
        const DWORD dwCmpFlagsNoVersion = ASM_CMPF_NAME | ASM_CMPF_CULTURE | ASM_CMPF_PUBLIC_KEY_TOKEN;

        hr = panRef->IsEqual(panDef, dwCmpFlagsNoVersion);
        if (FAILED(hr))
        {
            goto EXIT;
        }

        if (hr == S_FALSE)
        {
            // Not equivalent

            hr = S_OK;
            goto EXIT;
        }

        // Everything except version must be the same at this point (or unspecified).
        hr = panRef->IsEqual(panDef, ASM_CMPF_VERSION);
        if (hr == S_OK)
        {
            // Everything matches
            Equivalent = true; 
            result = ACR_EquivalentFullMatch;
        }
        else if (hr == S_FALSE)
        {
            // Version mismatch (and the the ref specified something)
            DWORD dwVerDef1, dwVerDef2;
            DWORD dwVerRef1, dwVerRef2;

            hr = panRef->GetVersion( &dwVerRef1, &dwVerRef2);
            if (FAILED(hr))
            {
                goto EXIT;
            }

            hr = panDef->GetVersion( &dwVerDef1, &dwVerDef2);
            if (FAILED(hr))
            {
                goto EXIT;
            }

            _ASSERTE(dwVerDef1 != dwVerRef1 || dwVerDef2 != dwVerRef2);
            Equivalent = true; 
            if (dwVerDef1 > dwVerRef1 || (dwVerDef1 == dwVerRef1 && dwVerDef2 >= dwVerDef2))
            {
                result = ACR_EquivalentUnified;
            }
            else
            {
                result = ACR_NonEquivalentVersion;
            }

            hr = S_OK;
        }
    }
    else
    {
        hr = panRef->IsEqual(panDef, ASM_CMPF_DEFAULT);
        if (hr == S_OK)
        {
            // Equivalent according to what they've given us
            Equivalent = true; 
            result = ACR_EquivalentWeakNamed;
        }
        else if (hr == S_FALSE)
        {
            Equivalent = false; 
            // Not equivalent. Reset HRESULT.
            hr = S_OK;
        }
    }

EXIT:
    return hr;
}
#endif

void AssemblyIdentity::SetKeyFileName
(
    _In_opt_z_ STRING *pstrFileName
)
{
    DestroyPublicKeyBlob();
    ClearCachedValues();

    // Treat "" as equivalent to NULL
    if (StringPool::IsEqual(pstrFileName, STRING_CONST(m_pCompiler, EmptyString)))
    {
        pstrFileName = NULL;
    }

    // 
    m_pstrKeyFileName = pstrFileName;
}

void AssemblyIdentity::SetKeyContainerName
(
    _In_opt_z_ STRING *pstrKeyContainerName
)
{
    DestroyPublicKeyBlob();
    ClearCachedValues();

    // Treat "" as equivalent to NULL
    if (StringPool::IsEqual(pstrKeyContainerName, STRING_CONST(m_pCompiler, EmptyString)))
    {
        pstrKeyContainerName = NULL;
    }

    // 
    m_pstrKeyContainerName = pstrKeyContainerName;
}

void AssemblyIdentity::ComputePKTokenFromFileOrContainer()
{
    m_cbPublicKey = 0;
    m_PublicKeyBlob = NULL;

    WCHAR *pwszFileName = m_pstrKeyFileName;
    StringBuffer ModifiedFileName;

    // Handle relative path
    //
    if (pwszFileName &&
        m_pstrOutputAssemblyPath)
    {
        if (!GetValidPathForFile(
                m_pstrKeyFileName,
                m_pstrOutputAssemblyPath,
                ModifiedFileName))
        {
            return;
        }

        pwszFileName = ModifiedFileName.GetString();
    }

    if (pwszFileName || m_pstrKeyContainerName)
    {
        CComPtr<IMetaDataDispenserEx> spMetaDataDispenser;
        CComPtr<IALink2> spALink;
        HRESULT hr = S_OK;

        // Create the ALink interface and do some minimal initialization

        PEBuilder::CreateALink(&spALink);

        IfFailThrow(GetCompilerHost()->GetDispenser(&spMetaDataDispenser));
        IfFailThrow(spALink->Init(spMetaDataDispenser, NULL));


        // Read the public key token from the file or container.
        // Microsoft: priority is given to key file.

        hr = spALink->GetPublicKeyToken(
                pwszFileName,
                pwszFileName ? NULL : m_pstrKeyContainerName,
                NULL,
                &m_cbPublicKey);

        if (m_cbPublicKey != 0)
        {
            m_PublicKeyBlob = new (zeromemory )BYTE[m_cbPublicKey];

            hr = spALink->GetPublicKeyToken(
                    pwszFileName,
                    pwszFileName ? NULL : m_pstrKeyContainerName,     // Priority given to key file. 
                    m_PublicKeyBlob,
                    &m_cbPublicKey);
        }

        if (hr != S_OK)
        {
            m_PublicKeyBlob =  NULL;
            m_cbPublicKey = 0;
        }
    }

    m_fPublicKeyAvailable = true;
}

CompilerHost *AssemblyIdentity::GetCompilerHost()
{
    if (m_pCompilerHost)
    {
        return m_pCompilerHost;
    }

    AssertIfNull(m_pCompilerProject);

    return m_pCompilerProject->GetCompilerHost();
}

int AssemblyIdentity::CompareMVIDs(AssemblyIdentity *pIdentity1, AssemblyIdentity *pIdentity2)
{
    return CompareGUIDs(pIdentity1->GetMVID(), pIdentity2->GetMVID());
}

void AssemblyIdentity::CacheAssemblyLastModifiedTime()
{
    AssertIfNull(m_pCompilerProject);
    AssertIfFalse(m_pCompilerProject->IsMetaData());

    if (!m_fAssemblyLastModifiedTimeCached)
    {
        HANDLE hFile =
                CreateFile(
                    m_pCompilerProject->GetFileName(),          // file to open
                    GENERIC_READ,                               // open for reading
#if IDE
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,         // share for reading, writing and deleting
#else
                    FILE_SHARE_READ | FILE_SHARE_WRITE,         // share for reading and writing - FILE_SHARE_DELETE not supported on Windows Me/98/95
#endif
                    NULL,                                       // default security
                    OPEN_EXISTING,                              // existing file only
                    FILE_ATTRIBUTE_NORMAL,                      // normal file
                    NULL);                                      // no attr. template

        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, NULL, NULL, &m_AssemblyLastModifiedTime);
            CloseHandle(hFile);
        }
        else
        {
            memset(&m_AssemblyLastModifiedTime, 0, sizeof(FILETIME));
        }

#pragma prefast(suppress: 26010, "The following variable is not a buffer")
        m_fAssemblyLastModifiedTimeCached = true;
    }
}

bool AssemblyIdentity::HasMatchingPublicKeyToken(LPCWSTR pTokens, size_t nTokenSize, size_t nTokenCount)
{
    bool fResult = false;
    if (pTokens)
    {
        ULONG pkStringLength = 0;
        LPWSTR pkString = GetPublicKeyTokenString(&pkStringLength);
        if (pkString && pkStringLength == (nTokenSize - 1))
        {
            LPCWSTR pToken = pTokens;
            for (size_t i = 0; i < nTokenCount; i++)
            {
                if (!_wcsnicmp(pkString, pToken, nTokenSize - 1))
                {
                    fResult = true;
                    break;
                }
                pToken += nTokenSize;
            }
            delete[] pkString;
        }
    }
    return fResult;
}

HRESULT AssemblyIdentity::ParseAssemblyString( _In_z_ STRING* pstrAssembly, IAssemblyName** ppiOut )
{
    AssertIfNull( pstrAssembly );
    AssertIfNull( ppiOut );

    HRESULT hr = LoadCreateAssemblyNameObjectFunction();

    if( FAILED( hr ) || pfnCreateAssemblyNameObject == NULL )
    {
        // If we cannot load the method, we will ignore the error.

        hr = S_FALSE;
        goto FAIL;
    }

    hr = pfnCreateAssemblyNameObject( ppiOut, pstrAssembly, CANOF_PARSE_DISPLAY_NAME, NULL);

FAIL:

    return( hr );
}

#if 1
// Code borrowed from alink.
// 


DWORD WINAPI W_GetShortPathName
(
    _In_z_ PCWSTR pLongPath,
    _Out_cap_(cchBuffer) PWSTR pShortPath,
    DWORD cchBuffer
)
{
    if (NULL == pLongPath || NULL == *pLongPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (W_IsUnicodeSystem())
        return ::GetShortPathNameW (pLongPath, pShortPath, cchBuffer);

    DWORD cbLongPathA = WideCharToMultiByte(CP_ACP, 0, pLongPath, -1, 0, 0, 0, 0);
    if (cbLongPathA == 0)
        return 0;
    else
        cbLongPathA++; // Add one for NULL

    TEMPBUF(szLongPath, char *, cbLongPathA);
    VSASSERT(szLongPath, "Stack Alloc Failed");

    if (0 == WideCharToMultiByte(CP_ACP, 0, pLongPath, -1, szLongPath, cbLongPathA, 0, 0)) {
        return 0;
    }

    // Prefix complains if we pass a NULL path into GetShortPathNameA, we'll make a one byte buffer to pass the first time.
    char buf[1] = "";
    DWORD cbShortPathA = ::GetShortPathNameA(szLongPath, buf, sizeof(buf)/sizeof(buf[0])); 
    if (cbShortPathA == 0)
        return 0;
    else
        cbShortPathA++; // Add one for NULL

    TEMPBUF(szShortPath, char *, cbShortPathA);
    IfNullThrow(szShortPath);

#pragma warning (push)
#pragma warning (disable:6387) // szShortPath is checked for NULL above
    DWORD cch2 = ::GetShortPathNameA(szLongPath, szShortPath, cbShortPathA);
#pragma warning (pop)

    if (cch2 != 0 && cch2 < cbShortPathA)
    {
        cch2 = MultiByteToWideChar(CP_ACP, 0, szShortPath, -1, pShortPath, cchBuffer);
    }

    return cch2;
}

size_t SafeStrLower
(
    _In_count_(cchSrc) LPCWSTR wszSrc,
    size_t cchSrc,
    _Out_cap_(cchDest) LPWSTR wszDest,
    size_t cchDest
)
{
    if (cchSrc == (size_t)-1)
        cchSrc = wcslen(wszSrc);

    if (cchSrc >= cchDest) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    LPWSTR wszDestEnd = ToLowerCase( wszSrc, wszDest, cchSrc);
    wszDestEnd[0] = L'\0';
    return (size_t)(wszDestEnd - wszDest);
}

size_t SafeStrCopy
(
    _In_count_(cchSrc) _Pre_z_ LPCWSTR wszSrc,
    size_t cchSrc,
    _Out_z_cap_(cchDest) LPWSTR wszDest,
    size_t cchDest
)
{
    if (cchSrc == (size_t)-1)
        cchSrc = wcslen(wszSrc);

    if (cchSrc >= cchDest) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    if (FAILED(StringCchCopyNW( wszDest, cchDest, wszSrc, cchSrc))) {
        VSFAIL("How did this fail?  We already checked that the buffer was big enough!");
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }
    AssertIfFalse(wszDest[cchSrc] == L'\0');
    return cchSrc;
}


DWORD GetCanonFilePath
(
    _In_z_ LPCWSTR wszSrcFileName,
    _Out_cap_(cchDestFileName) LPWSTR wszDestFileName,
    DWORD cchDestFileName,
    bool fPreserveSrcCasing
)
{
    // Although this would be a nice AssertIfFalse, we cannot make it because the source is a potentially untrusted
    // string, and thus may be unbounded (that's the purpose of this API, is to bound it and make sure it's safe)
    // So make no assumptions about wszSrcFileName!
    // AssertIfFalse(wcslen(wszSrcFileName) <= cchDestFileName );
    bool hasDrive = false;
    DWORD full_len;

    TEMPBUF(full_path, WCHAR *, cchDestFileName * sizeof(WCHAR)); // an intermediate buffer
    IfNullThrow(full_path);

    TEMPBUF(temp_path, WCHAR *, cchDestFileName * sizeof(WCHAR)); // an intermediate buffer
    IfNullThrow(temp_path);                                       // Used if FindFile fails

    WCHAR * full_cur;
    WCHAR * out_cur;
    WCHAR * out_end;

    memset(full_path, 0, cchDestFileName * sizeof(WCHAR));
    out_cur = wszDestFileName;
    out_end = out_cur + cchDestFileName;
    if (wszSrcFileName != wszDestFileName)
        *out_cur = L'\0';
    full_cur = full_path;

    // Replace '\\' with single backslashes in paths, because W_GetFullPathName fails to do this on win9x.
    size_t i = 0;
    size_t j = 0;
    size_t length = wcslen(wszSrcFileName);
    while (j<length)
    {
        // UNC paths start with '\\' so skip the first character if it is a backslash.
        if (j!= 0 && wszSrcFileName[j] == '\\' && wszSrcFileName[j+1] == '\\')
            j++;
        else
            temp_path[i++] = wszSrcFileName[j++];
        if (i >= cchDestFileName) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto FAIL;
        }
    }
    temp_path[i] = L'\0';

    WCHAR * file_part = NULL;

#pragma warning (push)
#pragma warning (disable:6387) // temp_path is checked for NULL above
    full_len = W_GetFullPathName(temp_path, cchDestFileName, full_path, &file_part);
#pragma warning (pop)

    if (wszSrcFileName == wszDestFileName)
        wszDestFileName[cchDestFileName-1] = L'\0';
    if (full_len == 0) {
        goto FAIL;
    } else if (full_len >= cchDestFileName) {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        goto FAIL;
    }

    // Allow only 1 ':' for drives and no long paths with "\\?\"
    if (((full_path[0] >= L'a' && full_path[0] <= L'z') ||
        (full_path[0] >= L'A' && full_path[0] <= L'Z')) &&
        full_path[1] == L':')
        hasDrive = true;

    // We don't allow colons (except after the drive letter)
    // long paths beginning with "\\?\"
    // devices beginning with "\\.\"
    // or wildcards
    // or characters 0-31
    if (wcschr( full_path + (hasDrive ? 2 : 0), L':') != NULL ||
        wcsncmp( full_path, L"\\\\?\\", 4) == 0 ||
        wcsncmp( full_path, L"\\\\.\\", 4) == 0 ||
        wcspbrk(full_path, L"?*\x1\x2\x3\x4\x5\x6\x7\x8\x9"
            L"\xA\xB\xC\xD\xE\xF\x10\x11\x12\x13\x14\x15"
            L"\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\0") != NULL) {
        SetLastError(ERROR_INVALID_NAME);
        goto FAIL;
    }


    if (hasDrive) {
        AssertIfFalse(full_path[2] == L'\\');
        size_t len = SafeStrLower( full_path, 3, out_cur, out_end - out_cur);
        if (len == 0)
            goto FAIL;

        full_cur += 3;
        out_cur += len;

    } else if (full_path[0] == L'\\' && full_path[1] == L'\\') {
        // Must be a UNC pathname, so lower-case the server and share
        // since there's no known way to get the 'correct casing'
        WCHAR * slash = wcschr(full_path + 2, L'\\');
        // slash should now point to the backslash between the server and share
        if (slash == NULL || slash == full_path + 2) {
            SetLastError(ERROR_INVALID_NAME);
            goto FAIL;
        }

        slash = wcschr(slash + 1, L'\\');
        if (slash == NULL) {
            slash = full_path + wcslen(full_path);
        } else if (slash[-1] == L'\\') {
            // An empty share-name?
            SetLastError(ERROR_INVALID_NAME);
            goto FAIL;
        } else
            slash++;
        // slash should now point to char after the slash after the share name
        // or the end of the sharename if there's no trailing slash

        size_t len = SafeStrLower( full_path, slash - full_path, out_cur, out_end - out_cur);
        if (len == 0)
            goto FAIL;

        full_cur = slash;
        out_cur += len;

    } else {
        // Not a drive-leter path or a UNC path, so assume it's invalid
        SetLastError(ERROR_INVALID_NAME);
        goto FAIL;
    }

    // We either have a lower-cased drive letter or a UNC name
    // with it's trailing slash
    // out_cur points to the trailing NULL
    // full_cur points to the character after the slash

    // Now iterate over each element of the path and attempt to canonicalize it
    // It's possible for this loop to never run
    //  (for strings like "C:\" or "\\unc\share" or "\\unc\share2\")
    while (*full_cur) {
        WIN32_FIND_DATAW find_data;
        bool hasSlash = true;
        WCHAR * slash = wcschr(full_cur, '\\');
        if (slash == NULL) {
            // This means we're on the last element of the path
            // so work with everything left in the string
            hasSlash = false;
            slash = full_cur + wcslen(full_cur);
        }

        // Check to make sure we have enough room for the next part of the path
        if (out_cur + (slash - full_cur) >= out_end) {
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto FAIL;
        }

        // Copy over the next path part into the output buffer
        // so we can run FindFile to get the correct casing/long filename
#pragma prefast(suppress: 26018, "The condition above verifies that the size is correct")
        memcpy(out_cur, full_cur, slash - full_cur);
        out_cur[slash - full_cur] = L'\0';
        HANDLE hFind = W_FindFirstFile(wszDestFileName, &find_data);
        if (hFind == INVALID_HANDLE_VALUE) {
            size_t temp_len;

            // We coundn't find the file, the general causes are the file doesn't exist
            // or we don't have access to it.  Either way we still try to get a canonical filename
            // but preserve the passed in casing for the filename

            if (!hasSlash && fPreserveSrcCasing) {
                // This is the last component in the filename, we should preserve the user's input text
                // even if we can't find it
                out_cur += slash - full_cur;
                full_cur = slash;
                break;
            }

            // This will succeed even if we don't have access to the file
            // (And on NT4 if the filename is already in 8.3 form)
            temp_len = W_GetShortPathName(wszDestFileName, temp_path, cchDestFileName);
            if (temp_len == 0) {
                // GetShortPathName failed, we have no other way of figuring out the
                // The filename, so just lowercase it so it hashes in a case-insensitive manner

                if (!hasSlash) {
                    // If it doesn't have a slash, then it must be the last part of the filename,
                    // so don't lowercase it, preserve whatever casing the user gave
                    temp_len = SafeStrCopy( full_cur, slash - full_cur, out_cur, out_end - out_cur);
                } else {
                    temp_len = SafeStrLower( full_cur, slash - full_cur, out_cur, out_end - out_cur);
                }
                if (temp_len == 0)
                    goto FAIL;

                full_cur = slash;
                out_cur += temp_len;

            } else if (temp_len >= cchDestFileName) {
                // The short filename is longer than the whole thing?
                // This shouldn't ever happen, right?
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                goto FAIL;
            } else {
                // GetShortPathName succeeded with a path that is less than BUFFER_LEN
                // find the last slash and copy it.  (We don't want to copy previous
                // path components that we've already 'resolved')
                // However, GetShortPathName doesn't always correct the casing
                // so as a safe-guard, lower-case it (unless it's the last filename)
                WCHAR * temp_slash = wcsrchr(temp_path, L'\\');

                AssertIfFalse(temp_slash != NULL);
                temp_slash++;
                size_t len = 0;
                if (!hasSlash) {
                    len = SafeStrCopy( temp_slash, -1, out_cur, out_end - out_cur);
                } else {
                    len = SafeStrLower( temp_slash, -1, out_cur, out_end - out_cur);
                }
                if (len == 0)
                    goto FAIL;

                full_cur = slash;
                out_cur += len;

            }
        } else {
            // Copy over the properly cased long filename
            FindClose(hFind);
            size_t name_len = wcslen(find_data.cFileName);
            if (out_cur + name_len + (hasSlash ? 1 : 0) >= out_end) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                goto FAIL;
            }

            // out_cur already has the filename with the input casing, so we can just leave it alone
            // if this is not a directory name and the caller asked to perserve the casing
            if (hasSlash || !fPreserveSrcCasing) {
                memcpy(out_cur, find_data.cFileName, name_len * sizeof(WCHAR));
            }
            else if (name_len != (slash - full_cur) || _wcsnicmp(find_data.cFileName, full_cur, name_len) != 0) {
                // The user asked us to preserve the casing of the filename
                // and the filename is different by more than just casing so report
                // an error indicating we can't create the file
                SetLastError(ERROR_FILE_EXISTS);
                goto FAIL;
            }

            out_cur += name_len;
            full_cur = slash;
        }

        if (hasSlash) {
            if (out_cur + 1 >= out_end) {
                SetLastError(ERROR_FILENAME_EXCED_RANGE);
                goto FAIL;
            }
            full_cur++;
            *out_cur++ = L'\\';
        }
        *out_cur = '\0';
    }

    return (DWORD)(out_cur - wszDestFileName);

FAIL:
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// MakePath
//
// Joins a relative or absolute filename to the given path and stores the new
// filename in lpBuffer

bool MakePath
(
    _In_z_ LPCWSTR lpPath,
    _In_z_ LPCWSTR lpFileName,
    _Out_cap_(nBufferLength) LPWSTR lpBuffer,
    DWORD nBufferLength
)
{
    AssertIfFalse(lpBuffer);

    TEMPBUF(lpNewFile, WCHAR *, nBufferLength * sizeof(WCHAR));
    IfNullThrow(lpNewFile);

    // Yes, the unicode forms work, even on Win98
    // PathIsRelative doesn't seem to catch root-relative paths (like "\rootdir\subdir\file.txt" )
    // See bug vSWhideby #64301
    if (PathIsURLW(lpFileName))
    {
        StringCchCopyW(lpBuffer, nBufferLength, lpFileName);

        // Do not pass URLs through GetCanonFilePath()
        return true;
    }

    if (PathIsUNCW(lpFileName) || lpFileName[0] !=L'\0' && lpFileName[1] == L':')
    {
        // This is already a fully qualified name beginning with
        // a drive letter or UNC path, just return it
        AssertIfFalse(!PathIsRelativeW(lpFileName));
        StringCchCopyW(lpNewFile, nBufferLength, lpFileName);
    }
    else if (lpFileName[0] == L'\\' && lpPath[0] != L'\0')
    {
        // This a root-relative path name, just cat the drive
        if (lpPath[0] == L'\\')
        {
            LPWSTR lpSlash = lpNewFile;

            // a UNC path, use "\\server\share" as the drive
            StringCchCopyW(lpNewFile, nBufferLength, lpPath);

            // Set the 4th slash to NULL to terminate the new file-name
            for (int x = 0; x < 3 && lpSlash; x++)
            {
                lpSlash = wcschr(lpSlash + 1, L'\\');
                AssertIfFalse(lpSlash);
            }

            if (lpSlash)
                *lpSlash = L'\0';
        }
        else
        {
            // a drive letter
            StringCchCopyNW(lpNewFile, nBufferLength, lpPath, 2);
        }
        StringCchCatW(lpNewFile, nBufferLength, lpFileName);

    }
    else
    {
        AssertIfFalse(PathIsRelativeW(lpFileName));

#pragma warning (push)
#pragma warning (disable:6387) // lpNewFile is checked for NULL above

        // This is a relative path name, just cat everything
        StringCchCopyW(lpNewFile, nBufferLength, lpPath);

        if (lpNewFile[wcslen(lpNewFile)-1] != '\\')
            StringCchCatW(lpNewFile, nBufferLength, L"\\");

        StringCchCatW(lpNewFile, nBufferLength, lpFileName);
#pragma warning (pop)
    }

    // Now fix everything up to use cannonical form
    return (0 != GetCanonFilePath(lpNewFile, lpBuffer, nBufferLength, false));
}

bool GetValidPathForFile
(
    _In_z_ LPCWSTR filename,
    _In_z_ LPCWSTR relPath,
    StringBuffer &NewPath
)
{
    HANDLE local = INVALID_HANDLE_VALUE;

    if (filename == NULL || wcslen(filename) >= MAX_PATH ||
        relPath == NULL || wcslen(relPath) >= MAX_PATH)
    {
        return false;
    }

    AssertIfFalse(relPath == NULL || wcslen(relPath) < MAX_PATH);

    local = W_CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (local != INVALID_HANDLE_VALUE)
    {
        NewPath.AppendString(filename);
    }
    else
    {
        // Try a relative path
        WCHAR newPath[MAX_PATH];
        WCHAR fullPath[MAX_PATH+1];
        if( relPath &&
            SUCCEEDED(StringCchCopyW(newPath, DIM(newPath), relPath)) &&
            PathRemoveFileSpecW(newPath) &&
            MakePath(newPath, filename, fullPath, DIM(fullPath)))
        {
            local = W_CreateFile( fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        }

        if (local != INVALID_HANDLE_VALUE)
        {
            NewPath.AppendString(fullPath);
        }
    }

    if (local == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    CloseHandle( local);
    return true;
}

#endif 1
