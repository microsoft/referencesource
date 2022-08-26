//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
//  Checks for usage of obsoleteness in the symbols.
//
class ObsoleteChecker
{
public:
    ObsoleteChecker(
        SourceFile *pSourceFile,
        ErrorTable *pErrorTable) :
        m_pSourceFile(pSourceFile),
        m_pErrorTable(pErrorTable)
    {
    }

    static
    void CheckObsolete(
        BCSYM *SymbolToCheck,
        _In_ Location *ErrorLocation,
        // The container context in which the Obsolete symbol was bound to
        BCSYM_Container *ContainingContainerForSymbolUsage ,
        // The type context in which the Obsolete symbol was used.
        // This should be the same as the ContainingContainerForSymbolUsage 
        // except for attributes applied to non-namespace containers in which
        // ContainingContainer will be the parent of the non-namespace
        // container, but ContainerContextForSymbolUsage would
        // be the non-namespace container.
        BCSYM_Container *ContainerContextForSymbolUsage,
        ErrorTable *pErrorTable,
        SourceFile *ContainingFileContext,
        Declaration *ContextOfSymbolUsage);

    static
    bool IsSymbolObsolete(
        BCSYM * SymbolToCheck);

    static
    bool IsSymbolObsolete(
        BCSYM *SymbolToCheck,
        bool &IsUseOfSymbolError,
        _Deref_opt_out_opt_z_ WCHAR **ObsoleteMessage);

    void ScanContainerForObsoleteUsage(BCSYM_Container *pContainer, bool NeedToCheckObsolete=true) const;

    void CheckForLongFullyQualifiedNames(BCSYM_Container *Container) const;

    static
    void CheckObsoleteAfterEnsuringAttributesAre----ed(
        BCSYM *SymbolToCheck,
        Location *ErrorLocation,
        BCSYM_Container *ContainingContext,
        ErrorTable *pErrorTable);

    static
    bool IsObsoleteOrHasObsoleteContainer(BCSYM_NamedRoot *pSymbol, bool p----Attr=false);

    

private:


    void CheckObsolete(
        BCSYM *SymbolToCheck,
        Location *ErrorLocation,
        BCSYM_Container *ContainingContext) const;

    // This is the main obsolete checker. Both the other static and non-static methods
    // invoke this after their checks.
    static
    void CheckObsolete(
        BCSYM *SymbolToCheck,
        Location *ErrorLocation,
        BCSYM_Container *ContainingContext,
        ErrorTable *pErrorTable);

    static
    void ReportObsoleteError(
        ErrorTable *pErrorTable,
        Location *ErrorLocation,
        _In_z_ WCHAR *ObsoleteErrorMessage,
        bool ShouldDisplayObsoleteError,
        BCSYM *ObsoleteSymbol,
        BCSYM_Container *ContainingContext);

    void ScanImportsForObsoleteUsage(BCSYM_Namespace *pNamespace) const;

    void ScanMethodForObsoleteUsage(
        BCSYM_Proc *pnamed,
        BCSYM_Container *pParent,
        bool NeedToCheckObsolete = true) const;

    void ScanGenericParamsForObsoleteUsage(
        BCSYM_GenericParam *FirstGenericParam,
        BCSYM_Container *Parent) const;

    SourceFile *m_pSourceFile;
    ErrorTable *m_pErrorTable;
};
