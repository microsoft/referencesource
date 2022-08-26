//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Checks for usage of non-CLSCompliant types in public exposed entities.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class CLSComplianceChecker
{
public:
    static
    void VerifyCLSCompliance(SourceFile * SourceFile);

    static
    void VerifyNameofProjectRootNamespaceIsCLSCompliant(
        CompilerProject * Project,
        ErrorTable * ErrorLog);

private:
    CLSComplianceChecker(SourceFile * File)
    {
        VSASSERT( File,
                    "NULL Source File Unexpected!!!");

        m_Compiler = File->GetCompiler();
    }

    void VerifyCLSComplianceForContainerAndNestedTypes(BCSYM_Container * Container);

    void VerifyCLSComplianceForContainer(BCSYM_Container * Container);

    void VerifyThatMembersAreNotMarkedCLSCompliant(BCSYM_Container * Container);

    void VerifyMemberNotMarkedCLSCompliant(BCSYM_NamedRoot * Member);

    static
    bool ContainerIsPartOfRootNamespace(BCSYM_Container * Container);

    void VerifyNameIsCLSCompliant(BCSYM_NamedRoot * NamedEntity);

    static
    bool IsNameCLSCompliant(_In_opt_z_ STRING * Name);

    static
    bool IsSyntheticMember(BCSYM_NamedRoot * Member);

    static
    bool IsExplicitlyMarkedCLSCompliant(BCSYM_NamedRoot * Member);

    static
    bool IsExplicitlyMarkedNonCLSCompliant(BCSYM_NamedRoot * Member);

    void VerifyBasesForCLSCompliance(BCSYM_Container * Container);

    void VerifyMembersForCLSCompliance(BCSYM_Container * Container);

    void ValidateNonCLSCompliantMemberInCLSCompliantContainer(
        BCSYM_NamedRoot * Member,
        BCSYM_Container * ParentOfMember);

    void VerifyProcForCLSCompliance(
        BCSYM_Proc * Proc,
        BCSYM_Container * ParentOfProc);

    void VerifyOverloadsForCLSCompliance(BCSYM_Proc * PossiblyOverloadedProc);

    void VerifyOverloadsForCLSCompliance(
        BCSYM_Proc * OverloadedProc,
        BCSYM_Proc * OverloadingProc);

    void VerifyEnumUnderlyingTypeForCLSCompliance(BCSYM_Container * PossibleEnum);

    void VerifyConstraintsAreCLSCompliant(BCSYM_NamedRoot * ContainerOrProc);

    bool IsTypeCLSCompliant(
        BCSYM * RawType,
        BCSYM_NamedRoot * NamedContext);

    static
    bool IsAccessibleOutsideAssembly(BCSYM_NamedRoot * Member);

    void ReportErrorOnSymbol(
        ERRID ErrID,
        BCSYM_NamedRoot * SymbolToReportErrorOn,
        _In_opt_z_ STRING * ErrorReplString1 = NULL,
        _In_opt_z_ STRING * ErrorReplString2 = NULL,
        _In_opt_z_ STRING * ErrorReplString3 = NULL,
        _In_opt_z_ STRING * ErrorReplString4 = NULL,
        _In_opt_z_ STRING * ErrorReplString5 = NULL);

    void ReportErrorOnSymbol(
        ERRID ErrID,
        BCSYM * SymbolToReportErrorOn,
        BCSYM_NamedRoot * NamedContext,
        _In_opt_z_ STRING * ErrorReplString1 = NULL,
        _In_opt_z_ STRING * ErrorReplString2 = NULL,
        _In_opt_z_ STRING * ErrorReplString3 = NULL,
        _In_opt_z_ STRING * ErrorReplString4 = NULL,
        _In_opt_z_ STRING * ErrorReplString5 = NULL);

    void ReportErrorAtLocation(
        ERRID ErrID,
        Location * ErrorLocation,
        BCSYM_NamedRoot * NamedContext,
        _In_opt_z_ STRING * ErrorReplString1 = NULL,
        _In_opt_z_ STRING * ErrorReplString2 = NULL,
        _In_opt_z_ STRING * ErrorReplString3 = NULL,
        _In_opt_z_ STRING * ErrorReplString4 = NULL,
        _In_opt_z_ STRING * ErrorReplString5 = NULL);

    Compiler *m_Compiler;
};
