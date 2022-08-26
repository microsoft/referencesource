//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to verify partial method semantics.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// This method checks to see if the signatures match as a potential partial
// method interaction. Logical errors are validated after.
//
bool Bindable::DoSignaturesMatchAsPartialMethods(
    BCSYM_NamedRoot *Member,
    BCSYM_NamedRoot *OtherMember,
    unsigned& CompareFlags)
{
    if( IsMethod( Member ) &&
        IsMethod( OtherMember ) &&
        ( Member->PProc()->IsPartialMethodDeclaration() || OtherMember->PProc()->IsPartialMethodDeclaration() )
      )
    {
        CompareFlags = BCSYM::CompareProcs(
            Member->PProc(),
            ( GenericBinding* )NULL,
            OtherMember->PProc(),
            ( GenericBinding* )NULL,
            NULL,
            true        // We want to ignore overload differences.
            );

        // Only if they are exactly the same can we replace.
        // We ignore param name differences.

        if( CompareFlags == EQ_Match ||
            CompareFlags == EQ_ParamName ||
            CompareFlags == EQ_GenericMethodTypeParams )
        {
            return( true );
        }
    }

    return( false );
}

// ----------------------------------------------------------------------------
// This method validates the generic constraints and type parameter names
// on the partial and implementing methods. It will also report errors.
// ----------------------------------------------------------------------------

bool
Bindable::ValidatePartialMethodsMatchGenericConstraints
(
    BCSYM_Proc *Partial,
    BCSYM_Proc *Implementing
)
{
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );

    bool bRet = true;

    // First, we validate the type parameter names, and make sure that they are the same.

    BCSYM_GenericParam* Param1 = Partial->GetFirstGenericParam();
    BCSYM_GenericParam* Param2 = Implementing->GetFirstGenericParam();

    while( Param1 != NULL && Param2 != NULL )
    {
        // Verify the type param name consistency

        if( !StringPool::IsEqual( Param1->GetName(), Param2->GetName() ) )
        {
            // Type parameter name '|1' does not match the name '|2' of the corresponding type
            // parameter defined on one of the other partial types of '|3'.

            ReportErrorOnSymbol(
                ERRID_PartialMethodTypeParamNameMismatch3,
                CurrentErrorLog(Param2),
                Param2,
                Param2->GetName(),
                Param1->GetName(),
                Implementing->GetName()
                );

            bRet = false;
        }

        Param1 = Param1->GetNextParam();
        Param2 = Param2->GetNextParam();
    }

    // Next, we validate the constraints.

    if( !CompareConstraints(
            Partial,
            NULL,
            Implementing,
            NULL,
            NULL
            )
      )
    {
        ReportErrorOnSymbol(
            ERRID_PartialMethodGenericConstraints2,
            CurrentErrorLog( Implementing ),
            Implementing,
            Implementing->GetName(),
            Partial->GetName()
            );
        bRet = false;
    }

    return( bRet );
}

// ----------------------------------------------------------------------------
// Validate the parameter names on the partial methods. Note that if this
// is a perf problem, we can merge this with the linking of the parameters,
// but I don't expect this to be a perf issue, since this only executes for
// methods with errors.
// ----------------------------------------------------------------------------

bool
Bindable::ValidateParameterNamesOnPartialMethods
(
    BCSYM_Proc* Partial,
    BCSYM_Proc* Implementing
)
{
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );

    bool bRet = true;

    BCITER_Parameters partialIter( Partial );
    BCITER_Parameters implIter( Implementing );

    while( true )
    {
        BCSYM_Param* partialParam = partialIter.PparamNext();
        BCSYM_Param* implParam = implIter.PparamNext();

        // Do some checking:

        VSASSERT( ( partialParam != NULL && implParam != NULL ) ||
                  ( partialParam == NULL && implParam == NULL ), "Mismatched partial parameters?" );

        if( partialParam == NULL )
        {
            break;
        }

        // Generate an error if the parameter names do not match.

        if( !StringPool::IsEqual( partialParam->GetName(), implParam->GetName() ) )
        {
            ReportErrorOnSymbol(
                ERRID_PartialMethodParamNamesMustMatch3,
                CurrentErrorLog( Implementing ),
                implParam,
                implParam->GetName(),
                partialParam->GetName(),
                Implementing->GetName()
                );

            bRet = false;
        }
    }

    return( bRet );
}

// ----------------------------------------------------------------------------
// This will create attribute values for the symbols in the proc.
// This can be either the proc symbols themselves or the arguments of the proc.
// ----------------------------------------------------------------------------

void
Bindable::CreateAttrValsForPartialMethodSymbols
(
    BCSYM_Proc* PartialProc,
    BCSYM_Proc* ImplementingProc,
    BCSYM*      Partial,
    BCSYM*      Implementing,
    WellKnownAttrVals* Common
)
{
    ThrowIfNull( PartialProc );
    ThrowIfNull( ImplementingProc );
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );
    ThrowIfNull( Common );

    // We only create attribute values for implementation methods.

    if( Partial->GetPAttrVals() != NULL )
    {
        Partial->GetPAttrVals()->SetPWellKnownAttrVals( Common );
    }

    if( Implementing->GetPAttrVals() == NULL )
    {
        Implementing->AllocAttrVals(
            CurrentAllocator(),
            CS_Bound,
            ImplementingProc->GetSourceFile(),
            ImplementingProc->GetPhysicalContainer()
            );

        VSASSERT(
            Implementing->GetPAttrVals()->GetPWellKnownAttrVals() == NULL,
            "How can the well known attribute values be set? decompilation bug."
            );

        // Set the context of the created attributes to the implementing symbol. This will cause
        // well-known attribute verifications to use the error log corresponding to the implementing
        // symbol.

        Implementing->GetPAttrVals()->GetPNon----edData()->m_psymParamContext = ImplementingProc;
    }

    Implementing->GetPAttrVals()->SetPWellKnownAttrVals( Common );
}

// ----------------------------------------------------------------------------
// For partial methods, we only hook up the real method so that it will contain
// the full set of attribute symbols.
// ----------------------------------------------------------------------------

void
Bindable::FixUpAttributesForPartialMethods
(
    BCSYM_Proc *Partial,
    BCSYM_Proc *Implementing
)
{
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );
    VSASSERT( Partial->IsPartialMethodDeclaration(), "1st param must be partial" );

    // If the partial method has no attributes, we don't need to set anything up.

    if( Partial->GetPAttrVals() == NULL )
    {
        return;
    }

    // Now, construct the PAttrVals for both methods.

    WellKnownAttrVals* CommonAttrVals = CurrentAllocator()->Alloc<WellKnownAttrVals>();
    CommonAttrVals = new (CommonAttrVals) WellKnownAttrVals(CurrentAllocator());

    CreateAttrValsForPartialMethodSymbols( Partial, Implementing, Partial, Implementing, CommonAttrVals );
}

// ----------------------------------------------------------------------------
// For parameters of partial methods, we link the parameters because we need
// to be able to iterate through all the attributes. Therefore, we also set
// up the attributes as needed.
// ----------------------------------------------------------------------------

void
Bindable::LinkParametersForPartialMethods
(
    BCSYM_Proc *Partial,
    BCSYM_Proc *Implementing
)
{
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );
    VSASSERT( Partial->IsPartialMethodDeclaration(), "1st param must be partial method decl" );

    BCITER_Parameters partialIter( Partial );
    BCITER_Parameters implIter( Implementing );

    while( true )
    {
        BCSYM_Param* partialParam = partialIter.PparamNext();
        BCSYM_Param* implParam = implIter.PparamNext();

        // Do some checking:

        VSASSERT( ( partialParam != NULL && implParam != NULL ) ||
                  ( partialParam == NULL && implParam == NULL ), "Mismatched partial parameters?" );

        if( partialParam == NULL )
        {
            break;
        }

        // Link the parameters.

        partialParam->SetImplementationForPartialParam( implParam );

        // Setup the attrvals only if we have attributes on the partial method.

        if( partialParam->GetPAttrVals() == NULL )
        {
            continue;
        }

        // Ok, construct the attributes.

        WellKnownAttrVals* CommonAttrVals = CurrentAllocator()->Alloc<WellKnownAttrVals>();
        CommonAttrVals = new (CommonAttrVals) WellKnownAttrVals(CurrentAllocator());

        CreateAttrValsForPartialMethodSymbols( Partial, Implementing, partialParam, implParam, CommonAttrVals );
    }
}

// ----------------------------------------------------------------------------
// Main API that links partial methods. The method DoSignaturesMatchAsPartialMethods
// must be called before this.
// ----------------------------------------------------------------------------

bool
Bindable::AttemptToLinkPartialMethods
(
    BCSYM_Proc *Partial,
    BCSYM_Proc *Implementing,
    unsigned PartialCompareFlags
)
{
    ThrowIfNull( Partial );
    ThrowIfNull( Implementing );
    VSASSERT( Partial->IsPartialMethodDeclaration(), "1st param must be partial method decl" );
    VSASSERT( !Implementing->IsPartialMethodDeclaration(), "2nd param must be implementing method" );

    // Do this so we report as many errors as possible.

    bool bHasError = false;

#if DEBUG
    unsigned __temp;
    VSASSERT( DoSignaturesMatchAsPartialMethods( Partial, Implementing, __temp ), "Must be partial method candidates!" );
#endif

    // At this point, we must do some more validation before we can link and say that the partial
    // methods are good.

    // First, lets check the param names and see if they match.

    if( PartialCompareFlags == EQ_ParamName &&
        !ValidateParameterNamesOnPartialMethods( Partial, Implementing )
        )
    {
        // Errors have already been generated.

        MarkOverloadedMemberBad( Implementing );
        bHasError = true;
    }

    // Check if these methods already have an implementation. If it does, and it is not the same method,
    // then report an error.

    BCSYM_Proc* AlreadyImplemented = Partial->GetAssociatedMethod();

    if( AlreadyImplemented != NULL &&
        AlreadyImplemented != Implementing )
    {
        ReportErrorOnSymbol(
            ERRID_OnlyOneImplementingMethodAllowed3,
            CurrentErrorLog( Implementing ),
            Implementing,
            Implementing->GetName(),
            Implementing->GetName(),
            Implementing->GetName()
            );

        MarkOverloadedMemberBad( Implementing );
        bHasError = true;
    }

    // Validate that the partial method matches on constraints. This is because we checked
    // CompareProc for EQ_GenericMethodTypeParams as well.

    if( !ValidatePartialMethodsMatchGenericConstraints( Partial, Implementing ) )
    {
        // Errors will have been reported already.

        MarkOverloadedMemberBad( Implementing );
        bHasError = true;
    }

    // The implementing method must be declared private.

    if( !( Implementing->GetDeclFlags() & DECLF_Private ) )
    {
        ReportErrorOnSymbol(
            ERRID_ImplementationMustBePrivate2,
            CurrentErrorLog( Implementing ),
            Implementing,
            Implementing->GetName(),
            Partial->GetName()
            );

        MarkOverloadedMemberBad( Implementing );
        bHasError = true;
    }

    // If we had an error, break now.

    if( bHasError )
    {
        return( false );
    }

    // NOTE: Everything done here needs to be undone in bindable decompilation.

    FixUpAttributesForPartialMethods( Partial, Implementing );
    LinkParametersForPartialMethods( Partial, Implementing );

    // Ok, everything checked out. Link the methods, and move the partial method
    // to the unbindable hash.

    Partial->SetImplementationForPartialMethod( Implementing );

    BCSYM_NamedRoot* pTemp = NULL;

    pTemp = Symbols::RemoveSymbolFromHash(
        Partial->GetPhysicalContainer()->GetHash(),
        Partial
        );

    VSASSERT( pTemp != NULL, "How can this be NULL, probably a bindable decompilation problem" );

    Symbols::AddSymbolToHash(
        Partial->GetPhysicalContainer()->GetUnBindableChildrenHash(),
        Partial,
        true,
        Partial->GetPhysicalContainer()->IsClass() ? Partial->GetPhysicalContainer()->PClass()->IsStdModule() : false,
        false
        );

    return( true );
}

