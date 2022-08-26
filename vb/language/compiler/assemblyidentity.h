//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the support for dealing with assembly identities in the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class MetaDataFile;
class CompilerProject;

// The content type is encoded as 3 bits in the flag. These macros help extract the content type
// from the flags
#define afContentType_Shift 9
#define AfContentType(x) (x & afContentType_Mask) >> afContentType_Shift

//-------------------------------------------------------------------------------------------------
//
// Implements the logic required to store assembly identities and simple comparisons.
// This has to match the declaration of ASSEMBLYMETADATA in cor.h
//
class AssemblyIdentity
{
public:
    NEW_MUST_ZERO()

    AssemblyIdentity(
        Compiler * pCompiler,
        CompilerHost * pCompilerHost)
    {
        InitAssemblyIdentityInfo(pCompiler, pCompilerHost, NULL);
    }

    AssemblyIdentity(
        Compiler * pCompiler,
        CompilerProject * pCompilerProject)
    {
        InitAssemblyIdentityInfo(pCompiler, NULL, pCompilerProject);
    }

    ~AssemblyIdentity() { Destroy(); }

    void ResetAssemblyIdentity(
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        CompilerProject * pCompilerProject)
    {
        Destroy();
        InitAssemblyIdentityInfo(pCompiler, pCompilerHost, pCompilerProject);
    }

    void InitAssemblyIdentityInfo(
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        CompilerProject * pCompilerProject)
    {
        memset(this, 0, sizeof(AssemblyIdentity));
        m_AssemblyFlagsAttributeCtor = RuntimeMembers::AssemblyFlagsAttributeIntCtor;

	m_pCompiler = pCompiler;
        m_pCompilerHost = pCompilerHost;
        m_pCompilerProject = pCompilerProject;
    }

    void SetAssemblyName(_In_z_ STRING * pAssemblyName)
    {
        m_AssemblyName = pAssemblyName; ClearCachedValues();
    }

    void SetAssemblyIdentity(
        _In_z_ STRING * AssemblyName,
        _In_z_ STRING * Locale,
        BYTE * pvPublicKey,
        ULONG cbPublicKey,
        USHORT MajorVersion,
        USHORT MinorVersion,
        USHORT BuildNumber,
        USHORT RevisionNumber,
        DWORD flags);

    void SetAssemblyFlags(int flags, RuntimeMembers AssemblyFlagsAttributeCtor)
    {
        VSASSERT( 
            AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeIntCtor ||
            AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeUIntCtor ||
            AssemblyFlagsAttributeCtor == AssemblyFlagsAttributeAssemblyNameFlagsCtor,
            "The runtime member is not support in this function.");

        m_AssemblyFlags = flags;
        m_AssemblyFlagsAttributeCtor = AssemblyFlagsAttributeCtor;
    }

    int GetAssemblyFlags(){ return m_AssemblyFlags; }
    RuntimeMembers GetAssemblyFlagsAttributeCtor(){ return m_AssemblyFlagsAttributeCtor; }

    STRING *GetAssemblyInfoString();

    STRING *GetAssemblyIdentityString();

    bool IsStrongNameAssembly() 
    {
         return GetPublicKeySize() != 0; 
    }

    bool IsAssemblyWithSameNameAndPKToken(AssemblyIdentity * pAssemblyIdentity);

    LONG CompareAssemblyVersion(AssemblyIdentity * pAssemblyIdentity)
    {
        LONG MajorCompare = CompareAssemblyFeatureVersion(pAssemblyIdentity);
        return (MajorCompare == 0 ? CompareAssemblyServicingVersion(pAssemblyIdentity) : MajorCompare);
    }

    LONG CompareAssemblyFeatureVersion(AssemblyIdentity * pAssemblyIdentity)
    {
        if (m_MajorVersion != pAssemblyIdentity->m_MajorVersion)
        {
            return m_MajorVersion - pAssemblyIdentity->m_MajorVersion;
        }

        return m_MinorVersion - pAssemblyIdentity->m_MinorVersion;
    }

    LONG CompareAssemblyServicingVersion(AssemblyIdentity * pAssemblyIdentity)
    {
        if (m_BuildNumber != pAssemblyIdentity->m_BuildNumber)
        {
            return m_BuildNumber - pAssemblyIdentity->m_BuildNumber;
        }

        return m_RevisionNumber - pAssemblyIdentity->m_RevisionNumber;
    }

    STRING * GetAssemblyName() 
    {
         return m_AssemblyName; 
    }

    STRING *GetAssemblyVersion();
    void SetAssemblyVersion(_In_opt_z_ STRING * pstrVersion);

    // Returns the raw string that can have user errors, "*", etc.
    STRING * GetRawAssemblyVersionString()
    {
        return m_pstrVersion;
    }

    bool ErrorInVersionString()
    {
        return m_fErrorInVersionString;
    }

    USHORT GetMajorVersion() 
    {
         return m_MajorVersion; 
    }

    USHORT GetMinorVersion() 
    {
         return m_MinorVersion; 
    }

    USHORT GetBuildNumber() 
    {
         return m_BuildNumber; 
    }

    USHORT GetRevisionNumber() 
    {
         return m_RevisionNumber; 
    }

    // Compares two assemblies to determine if they are unifiable or not. Generates warnings or errors in the process of doing that.
    static
    AssemblyComparisonResult CompareAssemblies(
        AssemblyIdentity * pDirectlyReferencedAssembly,
        AssemblyIdentity * pInDirectlyReferencedAssembly,
        ERRID * pOptionalErrorID,
        StringBuffer * pOptionaErrorMessage,
        bool * pNonEquivalentOnlyDueToLowerVersion,
        Compiler * pCompiler,
        CompilerHost * pCompilerHost);


    STRING * GetKeyFileName()
    {
        return m_pstrKeyFileName;
    }

    void SetKeyFileName(_In_opt_z_ STRING * pstrFileName);

    STRING *GetKeyContainerName()
    {
        return m_pstrKeyContainerName;
    }

    void SetKeyContainerName(_In_opt_z_ STRING * pstrKeyContainerName);

    STRING * GetOutputAssemblyPath()
    {
        return m_pstrOutputAssemblyPath;
    }

    void SetOutputAssemblyPath(_In_z_ STRING * pstrOutputAssemblyPath)
    {
        DestroyPublicKeyBlob();
        ClearCachedValues();

        m_pstrOutputAssemblyPath = pstrOutputAssemblyPath;
    }

    GUID *GetMVID()
    {
        return &m_MVID;
    }

    static
    int CompareMVIDs(
        AssemblyIdentity * pIdentity1,
        AssemblyIdentity * pIdentity2);

    void SetImportedFromCOMTypeLib()
    {
        m_fImportedFromCOMTypeLib = true;
    }

    bool IsImportedFromCOMTypeLib()
    {
        return m_fImportedFromCOMTypeLib;
    }

    void SetPrimaryInteropAssembly()
    {
        m_fPrimaryInteropAssembly = true;
    }

    bool IsPrimaryInteropAssembly()
    {
        return m_fPrimaryInteropAssembly;
    }

    FILETIME * GetAssemblyLastModifiedTime()
    {
        CacheAssemblyLastModifiedTime();
        return &m_AssemblyLastModifiedTime;
    }

    void CacheAssemblyLastModifiedTime();

    bool HasMatchingPublicKeyToken(
        LPCWSTR pTokens,
        size_t nTokenSize,
        size_t nTokenCount);

    static
    int CompareTimestamps(
        AssemblyIdentity * pIdentity1,
        AssemblyIdentity * pIdentity2)
    {
        return CompareFileTime(pIdentity1->GetAssemblyLastModifiedTime(), pIdentity2->GetAssemblyLastModifiedTime());
    }

    static
    HRESULT ParseAssemblyString(
        _In_z_ STRING * pstrAssembly,
        IAssemblyName * * ppiOut);

    WCHAR * GetPublicKeyTokenString(ULONG * PKStringLength);

    bool IsWindowsRuntimeAssembly()
    {
        return IsAfContentType_WindowsRuntime(m_flags);
    }

private:

    BYTE * GetPublicKeyBlob()
    {
        if (!m_fPublicKeyAvailable)
        {
            ComputePKTokenFromFileOrContainer();
        }

        return m_PublicKeyBlob;
    }

    ULONG GetPublicKeySize()
    {
        if (!m_fPublicKeyAvailable)
        {
            ComputePKTokenFromFileOrContainer();
        }

        return m_cbPublicKey;
    }

    void ClearCachedValues()
    {
        m_pstrAnnotated----semblyName = NULL;
        m_CLRAssemblyName = NULL;
        m_pstrComputedVersion = NULL;
    }

    void Destroy()
    {
        DestroyPublicKeyBlob();
    }

    void DestroyPublicKeyBlob()
    {
        delete[] m_PublicKeyBlob;
        m_PublicKeyBlob = NULL;
        m_cbPublicKey = 0;
        m_fPublicKeyAvailable = false;
    }

#ifdef FEATURE_CORESYSTEM
    static 
    HRESULT CompareNoFusion(
        IAssemblyName* panRef,
        IAssemblyName* panDef,
        BOOL& equivalent, 
        AssemblyComparisonResult& result);
#endif

    void ComputePKTokenFromFileOrContainer();

    CompilerHost *GetCompilerHost();

    // Assembly name and locale.

    STRING *m_AssemblyName;         // The name of this assembly.
    STRING *m_Locale;               // Locale.

    // Public key information.

    BYTE   *m_PublicKeyBlob;        // PublicKey string.
    ULONG   m_cbPublicKey;          // PublicKey string length.

    STRING *m_pstrKeyFileName;      // File containing the key pair.
    STRING *m_pstrKeyContainerName; // Container name for the public key token.

    // Version information.

    STRING *m_pstrVersion;          // This may contain "*" too, so GetAssemblyVersion should not return this string.

    USHORT  m_MajorVersion;         // Major Version.
    USHORT  m_MinorVersion;         // Minor Version.
    USHORT  m_BuildNumber;          // Build Number.
    USHORT  m_RevisionNumber;       // Revision Number.
    STRING *m_pstrComputedVersion;  // String representing m_MajorVersion.m_MinorVersion.m_BuildNumber.m_RevisionNumber

    // Misc information.

    GUID    m_MVID;                 // MVID for the assembly

    FILETIME m_AssemblyLastModifiedTime;    // The time the metadata assembly file was created. Applicable only for
                                            // metadata projects.

    STRING *m_pstrOutputAssemblyPath;// Path to the output assembly without the assembly name. This is used to compute
                                    // the absolute path for key files with relative paths.
    DWORD   m_flags;                // Retargetable, PK or token, etc.

    int m_AssemblyFlags;
    RuntimeMembers m_AssemblyFlagsAttributeCtor;

    // The full, annotated name of the assembly.  (The annotations include
    // things like the assembly's version number.)  Currently, the only
    // client of this is the MetaEmit code for System.Type arguments
    // used in custom attributes.  Those args are persisted as strings
    // that include the annotated assembly name and the name of the type.
    STRING   *m_pstrAnnotated----semblyName;
    STRING   *m_CLRAssemblyName;            // CLR assembly identity.
    Compiler *m_pCompiler;                  // Compiler.
    CompilerHost *m_pCompilerHost;          // Compiler Host. - Can be NULL, but both m_pCompilerHost and m_pCompilerProject cannot be NULL at the same time.
    CompilerProject *m_pCompilerProject;    // The associated compiler project. - Can be NULL, but both m_pCompilerHost and m_pCompilerProject cannot be NULL at the same time.

    bool    m_fPublicKeyAvailable : 1;  // Indicates whether the public key blob has been set/computed.
    bool    m_fErrorInVersionString : 1;// The version string in m_pstrVersion was found to have an error.
    bool    m_fImportedFromCOMTypeLib : 1;// Indicates that the corresponding assembly is a wrapper for a COM TypeLib.
    bool    m_fPrimaryInteropAssembly : 1;// The assembly is a PIA for some typelib.
    bool    m_fAssemblyLastModifiedTimeCached : 1; // Indicates that the assembly modification time has already been read and cached.
};


//****************************************************************************
// This structure holds the information associated with one Assemblyref in a
// metadata file.
//****************************************************************************
struct AssemblyRefInfo
{
    mdAssemblyRef m_tkAssemblyRef;          // AssemblyRef token

    AssemblyIdentity m_Identity;            // The assembly identity (name, publickey, version, culture)
                                            // for the assembly given by m_tkAssemblyRef.

    CompilerProject *m_pReferencingProject; // The project in which this assembly ref is defined in.

    CompilerProject *m_pReferencedProject;  // The project to which the identity of this assemblyref matches.

    CompilerProject *m_pAmbiguousReferencedProject; // Project that is identical to m_pReferencedProject and thus
                                            // causes ambiguity. This is used to avoid creating the dynamic array
                                            // for the metadata ambiguity cases which would be more common if the
                                            // same assembly is referenced from different locations.

    DynamicArray<CompilerProject *> *m_pPossiblyAmbiguousReferencedVBProjects;  // This is NULL unless
                                            // there is ambiguity between VB projects that can only be resolved later

    ERRID m_ErrorID;                        // Error that occured when binding this assembly ref
    STRING *m_pstrErrorExtra;               // Extra information regarding the assembly ref binding error.
    bool m_fCausesReferenceCycle;           // This reference causes circular references.

#if IDE 
    bool m_fVenusPartialNameMatch;          // Referenced assembly found using the venus partial name matching scheme.
#endif IDE

    ~AssemblyRefInfo()
    {
        if (m_pPossiblyAmbiguousReferencedVBProjects)
        {
            delete m_pPossiblyAmbiguousReferencedVBProjects;
        }
    }
};


//****************************************************************************
// This class holds the information associated with a list of AssemblyRefs. It
// is typically used to hold the information pertaining to all the AssemblyRefs
// in a metadata file.
//****************************************************************************
class AssemblyRefCollection
{
    friend class MetaDataFile;

public:

    AssemblyRefCollection(Compiler *pCompiler)
    : m_pCompiler(pCompiler)
    , m_CanIndexIntoAssemblyRefList(false)
    {
    }

    ~AssemblyRefCollection()
    {
        m_daAssemblyRefs.Destroy();
    }

    AssemblyRefInfo * GetAssemblyRefInfo(unsigned int Index)
    {
        if (Index >= m_daAssemblyRefs.Count())
        {
            return NULL;
        }

        return &m_daAssemblyRefs.Element(Index);
    }

    AssemblyRefInfo * GetAssemblyRefInfoByToken(mdAssemblyRef tkAssemblyRef)
    {
        if (m_CanIndexIntoAssemblyRefList)
        {
            return GetAssemblyRefInfo(RidFromToken(tkAssemblyRef) - 1);
        }

        for(unsigned int Index = 0; Index < m_daAssemblyRefs.Count(); Index++)
        {
            if (m_daAssemblyRefs.Element(Index).m_tkAssemblyRef == tkAssemblyRef)
            {
                return &m_daAssemblyRefs.Element(Index);
            }
        }

        return NULL;
    }

    ULONG GetAssemblyRefCount()
    {
        return m_daAssemblyRefs.Count();
    }

protected:

    void InitAssemblyRefList(unsigned long Count)
    {
        // This invokes Destroy and then re-initializes with new Count number of elements.
        m_daAssemblyRefs.SizeArray(Count);
    }

    AssemblyRefInfo& AddNew()
    {
        return m_daAssemblyRefs.Add();
    }

    void SetCanIndexIntoAssemblyRefList(bool CanIndex)
    {
        m_CanIndexIntoAssemblyRefList = CanIndex;
    }

private:

    Compiler *m_pCompiler;

    DynamicArray<AssemblyRefInfo> m_daAssemblyRefs;     // List of AssemblyRefs

    bool m_CanIndexIntoAssemblyRefList;                 // When searching for a particular AssemblyRef based
                                                        // on the token, this indicates whether the list can
                                                        // indexed with the token, or a linear search is required.
                                                        // The common case is indexing, so no optimizations
                                                        // implemented for the linear search.
};

