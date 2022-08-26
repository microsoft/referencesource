//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implements the file-level logic of the VB compiler.
//
//-------------------------------------------------------------------------------------------------

#pragma once

//****************************************************************************
// Implements the file-level compiler logic for an entire metadata database.
//****************************************************************************

struct TokenHashValue
{
    BCSYM * pSym;
    bool fTypeForward;
};

class MetaDataFile:
    public CompilerFile
#if FV_DEADBEEF
    , public Deadbeef<MetaDataFile> // Must be last base class!
#endif
{
    friend CompilerFile;
    friend CompilerProject;

public:

    //========================================================================
    // Creation methods.
    //========================================================================

    // Initializes a compilerfile that holds all of the classes
    // defined in a metadatafile.
    //
    static
    HRESULT Create(
        _In_ CompilerProject * pProject,
        _In_z_ WCHAR * wszMetaFile,
        _Out_ MetaDataFile * * ppfile);

    static 
    HRESULT Create(
        _In_ CompilerProject *pProject,
        _In_z_ WCHAR* wszMetaFile,
        _In_ IMetaDataImport* pImport,
        _Out_ MetaDataFile **ppFile);


    virtual ~MetaDataFile();

    //========================================================================
    // Accessors.
    //========================================================================

    STRING *GetName()
    {
        return m_pstrName;
    }

    // Set the name of the file.  This does no decompilation.
    void SetName(_In_z_ STRING *pstrName)
    {
        m_pstrName = pstrName;
    }

    IMetaDataImport *GetImport()
    {
        return m_pmdImport;
    }

    IMetaDataImport2 *GetImport2()
    {
        return m_pmdImport2;
    }

    IMetaDataWinMDImport *GetWinMDImport()
    {
        return m_pwinmdImport;
    }

    void ReleaseImport()
    {
        RELEASE(m_pmdImport);
        RELEASE(m_pmdImport2);
        RELEASE(m_pwinmdImport);
    }

    DynamicFixedSizeHashTable<mdTokenKey, TokenHashValue> *GetTokenHash()
    {
        return m_TokenHash;
    }

    void SetTokenHash(DynamicFixedSizeHashTable<mdTokenKey, TokenHashValue> *Hash)
    {
        m_TokenHash = Hash;
    }

    DynamicFixedSizeHashTable<mdTokenKey, AttrIdentity *> *GetAttributeTokenHash()
    {
        return m_AttributeTokenHash;
    }

    void SetAttributeTokenHash(DynamicFixedSizeHashTable<mdTokenKey, AttrIdentity *> *Hash)
    {
        m_AttributeTokenHash = Hash;
    }

    DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *GetNameHash()
    {
        return m_NameHash;
    }

    void SetNameHash(DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *Hash)
    {
        m_NameHash = Hash;
    }

    DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *GetNameHashForTypeFwds()
    {
        return m_NameHashForTypeFwds;
    }

    void SetNameHashForTypeFwds(DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *Hash)
    {
        m_NameHashForTypeFwds = Hash;
    }

#if IDE 
    void _DemoteToNoState()
    {
    }
#endif IDE

    AssemblyRefCollection *GetAssemblyRefs()
    {
        return &m_AssemblyRefs;
    }

    // Whether the metadatafile has one of the attributes indicating that this is a PIA
    TriState<bool> IsPIA();

protected:

    //========================================================================
    // The following methods are used within the MetaDataFile class only.
    // These are used to read in the assembly refs in a metadata file and
    // store them in a specialized collection for later use.
    //========================================================================

    HRESULT LoadAssemblyRefs();

    HRESULT GetAssemblyRefProps(
        IMetaDataAssemblyImport * pAssemblyImport,
        mdAssemblyRef tkAssemRef,
        AssemblyRefInfo * pAssemblyRefInfo);

    //========================================================================
    // The following methods are used to bind the assembly refs in a metadata
    // file to the other projects in the solution.
    //========================================================================

public:
    void BindAssemblyRefs(ActionKindEnum action=ActionKind::NoForce);

    static
    bool CanBindToVBProject(
        AssemblyRefInfo * pAssemblyRefInfo,
        CompilerProject * pProject,
        Compiler * pCompiler,
        CompilerHost * pCompilerHost,
        bool CheckOnlyByNameAndProjectLevelPKInfoIfAvailable,
        ERRID * pOptionalErrorID,
        StringBuffer * pOptionalErrorString);

protected:
    void BindAssemblyRef(AssemblyRefInfo *pAssemblyRefInfo);

#if IDE  
    CompilerProject *GetPartialMatchVenusAppCodeProject(AssemblyRefInfo *pAssemblyRefInfo);
#endif IDE

    void DisambiguateIdenticalReferences(
        CompilerProject * pNewFound,
        CompilerProject * * ppMatch,
        CompilerProject * * ppMatchReferencingProject,
        CompilerProject * * ppAmbiguousMatch,
        CompilerProject * * ppAmbiguousMatchReferencingProject,
        bool * pfVenusProjectsInvolved,
        bool * pfWarn);

    //========================================================================
    // The following methods should only be called from the master
    // project compilation routines.  They should never be called directly.
    //========================================================================

    // Methods that move this file up states.
    virtual bool _StepToBuiltSymbols();
    virtual bool _StepToBoundSymbols();
    virtual bool _StepToCheckCLSCompliance();
    virtual bool _StepToEmitTypes();
    virtual bool _StepToEmitTypeMembers();
    virtual bool _StepToEmitMethodBodies();

    // invoked by _PromoteToBound to complete the task started by _StepToBoundSymbols()
    virtual void CompleteStepToBoundSymbols();

protected:

    MetaDataFile(Compiler * pCompiler)
    : CompilerFile(pCompiler)
    , m_AssemblyRefs(pCompiler)
    , m_fAssemblyRefsLoaded(false)
    {
    }

    //
    // Data members.
    //

    // The name of the scope.
    STRING *m_pstrName;

    // The IMetaDataImport2 we will use when delay-loading
    // the containers defined in this library.  This structure owns the
    // reference to this.
    //
    IMetaDataImport *m_pmdImport;
    IMetaDataImport2 *m_pmdImport2;
    IMetaDataWinMDImport *m_pwinmdImport;

    // A table to speed lookup when resolving typerefs.
    DynamicFixedSizeHashTable<mdTokenKey, TokenHashValue> *m_TokenHash;
    DynamicFixedSizeHashTable<mdTokenKey, AttrIdentity *> *m_AttributeTokenHash;
    DynamicFixedSizeHashTable<STRING *, BCSYM_Container *> *m_NameHash;
    DynamicFixedSizeHashTable<STRING *, BCSYM_TypeForwarder *> *m_NameHashForTypeFwds;

    // The assembly refs in this metadata file.
    AssemblyRefCollection m_AssemblyRefs;

    // Boolean to indicate whether the assembly refs have already been loaded from the metadata file.
    bool m_fAssemblyRefsLoaded;

private:

    static
    HRESULT CreateCore(
        _In_ CompilerProject * pProject,
        _In_z_ WCHAR * wszMetaFile,
        _Out_ MetaDataFile ** ppfile);

    HRESULT BuildNamespacesForMetaDataFile(); // worker func to build the namespace for this metadata file
    HRESULT CreateImport();

};

