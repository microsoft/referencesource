//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Checks for usage of obsoleteness in the symbols.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//=============================================================================
// Check for an obsolete symbol and report either an error or a warning.
// Return value is true if an Error (as opposed to Warning) was reported, false otherwise.
//=============================================================================

void
ObsoleteChecker::CheckObsolete
(
    BCSYM *SymbolToCheck,
    Location *ErrorLocation,
    BCSYM_Container *ContainingContext
) const
{
    VSASSERT(m_pErrorTable, "CheckObsolete: ErrorTable hasn't been set!");

    CheckObsoleteAfterEnsuringAttributesAre----ed(
        SymbolToCheck,
        ErrorLocation,
        ContainingContext,
        m_pErrorTable);
}

void
ObsoleteChecker::CheckObsoleteAfterEnsuringAttributesAre----ed
(
    BCSYM *SymbolToCheck,
    Location *ErrorLocation,
    BCSYM_Container *ContainingContext,
    ErrorTable *ErrorTable
)
{
    // If we're dealing with a null or bad symbol, just get out of here because we can't do anything about it
    if (SymbolToCheck == NULL || SymbolToCheck->IsBad())
    {
        return;
    }

    if (SymbolToCheck->IsGenericTypeBinding())
    {
        BCSYM_GenericTypeBinding *GenericBinding = SymbolToCheck->PGenericTypeBinding();

        CheckObsoleteAfterEnsuringAttributesAre----ed(
            GenericBinding->GetGenericType(),
            ErrorLocation,
            ContainingContext,
            ErrorTable);

        unsigned ArgumentCount = GenericBinding->GetArgumentCount();

        for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
        {
            CheckObsoleteAfterEnsuringAttributesAre----ed(
                GenericBinding->GetArgument(ArgumentIndex)->ChaseToType(),
                ErrorLocation,
                ContainingContext,
                ErrorTable);
        }

        return;
    }

    VSASSERT(SymbolToCheck->IsNamedRoot() || SymbolToCheck->IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");

    // Why this assert ? We cannot not ---- attributes for other containers before the
    //      Context Container's attributes have been ----ed. Otherwise it would cause circular
    //      dependencies in bindable
    //
    VSASSERT( Bindable::IsContainerReadyForObsoleteChecking(ContainingContext),
                "Obsolete checking invoked before the containing context is ready for obsolete checking!!");

    // Ensure attributes are ----ed because this will be invoked before CS_Bound is reached
    // Nothing to do for BCSYM_Param because parameters cannot have Obsolete attributes
    //
    if (SymbolToCheck->IsContainer())
    {
        // 
        Bindable::----AttributesOnAllSymbolsInContainer(SymbolToCheck->PContainer(), NULL);
    }
    else if (SymbolToCheck->IsNamedRoot())
    {
        // 
        Bindable::----AttributesOnAllSymbolsInContainer(SymbolToCheck->PNamedRoot()->GetContainer(), NULL);
    }

    // Need the IsBad check again because the ----AttributesOnAllSymbolsInContainer
    // above might cause this to become bad.
    //
    if ( !SymbolToCheck->IsBad())
    {
        ObsoleteChecker::CheckObsolete(
            SymbolToCheck,
            ErrorLocation,
            ContainingContext,
            ErrorTable);
    }

    return;
}

// Static helper for semantics
void
ObsoleteChecker::CheckObsolete
(
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
    Declaration *ContextOfSymbolUsage
)
{
    // If we're dealing with a null or bad symbol, or a constant symbol just get out of here because we can't do anything about it
    if (SymbolToCheck == NULL || SymbolToCheck->IsBad() || SymbolToCheck->IsCCConstant())
    {
        return;
    }

    VSASSERT(SymbolToCheck->IsNamedRoot() || SymbolToCheck->IsParam(), "Must be BCSYM_NamedRoot or BCSYM_Param");

    // When file is less than declared, we do get here when evaluating
    // conditional constants and in a few other cases. But in those cases
    // we just want to ignore because they are either not valid check for
    // obsoleteness or are going to happen again (eg: when trying to check
    // if an expression can be evaluated in declared by trying to Interpret
    // it).
    //
    CompilationState CurrentStateOfFile;
    if (!ContainingFileContext ||
#if HOSTED
        ((CurrentStateOfFile = ContainingFileContext->GetCompState()) < CS_Declared)
        && !SymbolToCheck->GetExternalSymbol())
#else
        (CurrentStateOfFile = ContainingFileContext->GetCompState()) < CS_Declared)
#endif
    {
        return;
    }

    // When file state is less than CS_Bound, then there is no guarantee that any
    // symbols attributes are ----ed. By doing this, we filter out any obsolete
    // checks that come prior to CS_Bound and thus avoid showing or not showing
    // obsolete errors due to symbol ordering.
    //
    // So instead we delay any obsolete checks which get here before < CS_Bound
    // and do the checking later in bindable when the context container gets
    // its turn for obsolete symbol scanning.
    //
    // A question might be why not ---- attributes when needed ? But doing
    // so before a particular stage in bindable will cause circular dependencies.
    //

#if HOSTED
    if (!SymbolToCheck->GetExternalSymbol() && CurrentStateOfFile < CS_Bound)
#else
    if (CurrentStateOfFile < CS_Bound)
#endif
    {
        if (ContainerContextForSymbolUsage && pErrorTable)
        {
            // Microsoft:
            // If we have the same attribute in the unnamed namespace and in a class somewhere,
            // then this assert will fire. We'll keep this assert, but protect the code below
            // so that we don't AV.
            AssertIfFalse(!ContainerContextForSymbolUsage->IsBindingDone());

            if( !ContainerContextForSymbolUsage->IsBindingDone() )
            {
                Bindable *BindableInstance = ContainerContextForSymbolUsage->GetBindableInstance();

                BindableInstance->AddToListOfDelayedObsoleteChecks(
                    SymbolToCheck,
                    ErrorLocation,
                    pErrorTable,
                    ContextOfSymbolUsage);
            }
        }

        return;
    }


    CheckObsolete(
        SymbolToCheck,
        ErrorLocation,
        ContainingContainerForSymbolUsage ,
        pErrorTable);
}



void
ObsoleteChecker::CheckObsolete
(
    BCSYM *SymbolToCheck,
    Location *ErrorLocation,
    BCSYM_Container *ContainingContext,
    ErrorTable *pErrorTable
)
{
    VSASSERT( SymbolToCheck != NULL && !SymbolToCheck->IsBad(),
                "NULL or bad symbols should not allowed in here!!");

    WCHAR *ObsoleteErrorMessage = NULL;

    bool ShouldDisplayObsoleteError = false;

    // HACK:Microsoft - Needed because currently we don't propagate any attributes applied to a withevents
    // variable to its underlying property.
    // This should go away once we decide on how we want to allow users to specify attributes on synthetic
    // symbols and how we would infer about what attributes should be propagated from the real symbol in
    // code to the synthetic symbols.
    //
    if (SymbolToCheck->IsNamedRoot() &&
        SymbolToCheck->PNamedRoot()->CreatedByWithEventsDecl())
    {
        SymbolToCheck = SymbolToCheck->PNamedRoot()->CreatedByWithEventsDecl();
    }

    bool IsObsolete = IsSymbolObsolete(SymbolToCheck, ShouldDisplayObsoleteError, &ObsoleteErrorMessage);

    if (IsObsolete)
    {
        // 


        if (pErrorTable)
        {
            ReportObsoleteError(
                pErrorTable,
                ErrorLocation,
                ObsoleteErrorMessage,
                ShouldDisplayObsoleteError,
                SymbolToCheck,
                ContainingContext);
        }

        return;
    }

    return;
}

bool ObsoleteChecker::IsSymbolObsolete
(
    BCSYM *SymbolToCheck
)
{
    bool IsUseOfSymbolError = false; // it is an output variable
    return IsSymbolObsolete(SymbolToCheck,IsUseOfSymbolError,NULL);
}



bool ObsoleteChecker::IsSymbolObsolete
(
    BCSYM *SymbolToCheck,
    bool &IsUseOfSymbolError,
    _Deref_opt_out_opt_z_ WCHAR **ObsoleteMessage
)
{
#if DEBUG
    // Verify that attributes have already been ----ed by this point of time.
    //
    // Nothing to do for BCSYM_Param because parameters cannot have Obsolete
    // attributes
    //
    BCSYM_Container *ContainerOfSymbol = NULL;

    if (SymbolToCheck->IsContainer())
    {
        ContainerOfSymbol = SymbolToCheck->PContainer();
    }
    else if (SymbolToCheck->IsNamedRoot())
    {
        ContainerOfSymbol = SymbolToCheck->PNamedRoot()->GetContainer();
    }

    VSASSERT( !ContainerOfSymbol ||
              Bindable::IsContainerReadyForObsoleteChecking(ContainerOfSymbol),
                    "Too early to use this helper!!!");
#endif

    WCHAR *ObsoleteMessageTemp;

    if (!ObsoleteMessage)
    {
        ObsoleteMessage = &ObsoleteMessageTemp;
    }

    // No NULL check here for PWellKnownAttrVals because GetObsoleteData is safe to call on a NULL pointer
    //
    return SymbolToCheck->GetPWellKnownAttrVals()->GetObsoleteData(ObsoleteMessage, &IsUseOfSymbolError);
}



bool 
ObsoleteChecker::IsObsoleteOrHasObsoleteContainer(BCSYM_NamedRoot *pSymbol, bool p----Attr)
{
    AssertIfFalse(pSymbol != NULL);

    for (BCSYM_NamedRoot *container = pSymbol;
         container != NULL && !container->IsNamespace();
         container = container->GetContainer())
    {
        if (p----Attr)
        {
            Bindable::----AttributesOnAllSymbolsInContainer(pSymbol->GetContainer(), NULL);
        }
        if (ObsoleteChecker::IsSymbolObsolete(container))
        {
            return true;
        }
    }
    return false;
}

void
ObsoleteChecker::ReportObsoleteError
(
    ErrorTable *pErrorTable,
    Location *ErrorLocation,
    _In_z_ WCHAR *ObsoleteErrorMessage,
    bool ShouldDisplayObsoleteError,
    BCSYM *ObsoleteSymbol,
    BCSYM_Container *ContainingContext
)
{
    StringBuffer TextBuffer;

    // If Obsolete symbol is property accessor, then give a slightly different error
    //
    if (ObsoleteSymbol->IsProc() &&
        (ObsoleteSymbol->PProc()->IsPropertyGet() ||
            ObsoleteSymbol->PProc()->IsPropertySet()))
    {
        const WCHAR *AccessorName =
            ObsoleteSymbol->PProc()->IsPropertyGet() ?
                    L"Get" :
                    L"Set";

        WCHAR *PropertyRep =
            pErrorTable->ExtractErrorName(
                ObsoleteSymbol->PProc()->GetAssociatedPropertyDef(),
                ContainingContext,
                TextBuffer);

        if (ObsoleteErrorMessage)
        {
            pErrorTable->CreateError(
                ShouldDisplayObsoleteError ?
                    ERRID_UseOfObsoletePropertyAccessor3 :
                    WRNID_UseOfObsoletePropertyAccessor3,
                ErrorLocation,
                AccessorName,
                PropertyRep,
                ObsoleteErrorMessage);
        }
        else
        {
            pErrorTable->CreateError(
                ShouldDisplayObsoleteError ?
                    ERRID_UseOfObsoletePropertyAccessor2 :
                    WRNID_UseOfObsoletePropertyAccessor2,
                ErrorLocation,
                AccessorName,
                PropertyRep);
        }

        return;
    }


    if (ObsoleteErrorMessage)
    {
        pErrorTable->CreateError(
            ShouldDisplayObsoleteError ?
                ERRID_UseOfObsoleteSymbol2 :
                WRNID_UseOfObsoleteSymbol2,
            ErrorLocation,
            pErrorTable->ExtractErrorName(
                ObsoleteSymbol,
                ContainingContext,
                TextBuffer),
            ObsoleteErrorMessage);
    }
    else
    {
        pErrorTable->CreateError(
            ShouldDisplayObsoleteError ? ERRID_UseOfObsoleteSymbolNoMessage1 : WRNID_UseOfObsoleteSymbolNoMessage1,
            ErrorLocation,
            pErrorTable->ExtractErrorName(
                ObsoleteSymbol,
                ContainingContext,
                TextBuffer));
    }

    return;
}

// 
void
ObsoleteChecker::CheckForLongFullyQualifiedNames
(
    BCSYM_Container *Container
) const
{
    VSASSERT( Container, "How can this be ?");
    const WCHAR *MaxAllowedSizeForFullyQualifiedNames = L"1023";

    if (!Container->IsType() ||
        // This check should only be done on the main type
        //
        Container->IsPartialType())
    {
        return;
    }

    BCSYM_NamedRoot *Parent = Container->GetParent();

    if (Parent && Parent->IsNamespace() && (Parent->PNamespace() != m_pSourceFile->GetUnnamedNamespace()))
    {
        STRING *FullNameOfContainer =
            Container->GetQualifiedEmittedName();

        if (StringPool::StringLength(FullNameOfContainer) > MaxIdentifierLength)    // including the "."s in the name
        {
            m_pErrorTable->CreateError(
                ERRID_FullyQualifiedNameTooLong1,
                Container->GetLocation(),
                MaxAllowedSizeForFullyQualifiedNames);
        }
    }
}

//============================================================================
// Verify that no Obsolete types are imported using the Imports statement.
//============================================================================

void
ObsoleteChecker::ScanImportsForObsoleteUsage
(
    BCSYM_Namespace *pNamespace
) const
{
    VSASSERT(pNamespace, "Expecting a namespace");

    ImportedTarget *pImports = pNamespace->GetImports();

    while (pImports)
    {
        if (!pImports->m_hasError && !pImports->m_IsXml && !pImports->m_pTarget->IsNamespace())
        {
            CheckObsolete(
                pImports->m_pTarget,
                &pImports->m_loc,
                pNamespace);
        }
        pImports = pImports->m_pNext;
    }

    // Now dig through any contained namespaces

    // Iterate through any nested namespaces
    // 

    BCITER_CHILD NamespaceIterator(pNamespace);
    while (BCSYM_NamedRoot * pCurrentNamespaceSymbol = NamespaceIterator.GetNext())
    {
        if (pCurrentNamespaceSymbol->IsNamespace())
        {
            ScanImportsForObsoleteUsage(pCurrentNamespaceSymbol->PNamespace());
        }
    }
}


//============================================================================
// Scan all members in a container for usages of Obsolete items
//============================================================================

void
ObsoleteChecker::ScanContainerForObsoleteUsage
(
    BCSYM_Container *pContainer,
    bool NeedToCheckObsolete
) const
{

    switch (pContainer->GetKind())
    {
        case SYM_Class:
        {
            BCSYM_Class *pClass = pContainer->PClass();

            // Do nothing in the case of Enum.  Neither its base class (System.Enum) nor its
            // base type (Integral intrinsics), nor its members can be obsolete.
            //
            if (pClass->IsEnum())
            {
                return;
            }

            // If a class is created by an event and this event is obsolete then do not check obsolete usage
            BCSYM_EventDecl *createdByEventDecl = pClass->PNamedRoot()->CreatedByEventDecl();
            if ( createdByEventDecl != NULL &&
                 IsSymbolObsolete(createdByEventDecl))
            {
                NeedToCheckObsolete = false;
            }
            // Intrinsics are not obsolete
            //
            if (NeedToCheckObsolete &&
                pClass->GetRawBase() &&
                pClass->GetRawBase()->IsNamedType())
            {
                // If the base is specified on a different partial type, then no reason to check here.
                // If we insisted on checking it here, then we would be putting the error in the wrong
                // error table.
                //
                
                BCSYM_NamedType* pBase = pClass->GetRawBase()->PNamedType();
                if (pBase->GetContext() == pClass)
                {
                    // Is the baseclass obsolete?
                    CheckObsolete(
                        pBase->DigThroughNamedType(), 
                        pClass->GetInheritsLocation(),
                        pClass);
                }
            }

            // Are the implemented interfaces obsolete?
            BCITER_ImplInterfaces ImplInterfacesIter(
                pClass,
                true); /* Look only in current partial type */

            if (NeedToCheckObsolete)
            {
                while (BCSYM_Implements *pCursor = ImplInterfacesIter.GetNext())
                {
                    CheckObsolete(
                        pCursor->GetRoot(),
                        pCursor->GetLocation(),
                        pClass);
                }
            }
        }
        break;

        case SYM_Interface:
        {
           if (NeedToCheckObsolete)
           {
                BCSYM_Interface *pInterface = pContainer->PInterface();

                // Are the implemented interfaces obsolete?
                for (BCSYM_Implements *pCursor = pInterface->GetFirstImplements();
                     pCursor;
                     pCursor = pCursor->GetNext())
                {
                    CheckObsolete(
                        pCursor->GetRoot(),
                        pCursor->GetLocation(),
                        pInterface);
                }
            }
        }
        break;

        case SYM_Namespace:
        {
            if (NeedToCheckObsolete)
            {
                ScanImportsForObsoleteUsage(pContainer->PNamespace());
            }
            return;
        }

        default:
            VSFAIL("What other kinds of containers do we have?");
    }

    // Check the constraints of the generic params
    //
    if (NeedToCheckObsolete)
    {
        ScanGenericParamsForObsoleteUsage(
            pContainer->GetFirstGenericParam(),
            pContainer);
    }

    //
    // Check all of the datamembers and methods of the class.
    //
    BCITER_CHILD iter(
        pContainer,
        false,  // Don't bind to unbindable members
        true);  // look only in current partial type

    while (BCSYM_NamedRoot *pnamed = iter.GetNext()->DigThroughAlias()->PNamedRoot())
    {
        // We don't want to report errors for synthetic members.
        // Yes, some of these are bindable. Eg: withevents property,
        // the withevents variable _prop, etc.
        // If we don't skip errors for these, we would end up reporting
        // the same errors on both these and the original symbols from
        // which they were synthesized resulting in duplicate errors.
        //
        // 






        if (pnamed->IsProc())
        {
            ScanMethodForObsoleteUsage(pnamed->PProc(), pContainer, NeedToCheckObsolete && !IsSymbolObsolete(pnamed));
            continue;
        }

        if (pnamed->IsVariable() &&
            pnamed->PVariable()->CreatedByEventDecl() == NULL &&       // exclude field created by an Event Decl
            pnamed->PVariable()->CreatedByWithEventsDecl() == NULL && // exclude backing field created by a WithEvents Decl
            pnamed->PVariable()->GetAutoPropertyThatCreatedSymbol() == NULL)    // exclude backing field created by an AutoProperty Decl
        {
            // what about the fields that get created for static locals?  How come static locals don't come through here?
            // Is the field's type obsolete?
            // 
            // If pnamed is obsolete, then there is no need to check obsolete for its type.
            if (NeedToCheckObsolete && ! IsSymbolObsolete(pnamed))
            {
                CheckObsolete(
                    pnamed->PVariable()->GetType()->ChaseToType(),
                    pnamed->PVariable()->GetRawType()->GetTypeReferenceLocation(),
                    pContainer);
            }

            // check for restricted types. no fields of restricted type, no fields arrays of restricted types.
            // as the support fields for static locals do not reach here, they are checked when making locals
            CheckRestrictedType(
                ERRID_RestrictedType1,
                pnamed->PVariable()->GetType()->ChaseToType(),
                pnamed->PVariable()->GetRawType()->GetTypeReferenceLocation(),
                m_pSourceFile->GetCompilerHost(),
                m_pErrorTable);

            continue;
        }
    }
}


//============================================================================
// Scan all components of a method declaration for usages of Obsolete items
//============================================================================

void
ObsoleteChecker::ScanMethodForObsoleteUsage
(
    BCSYM_Proc *pproc,
    BCSYM_Container *pParent,
    bool NeedToCheckObsolete
) const
{
    // 

    // Ignore all code the compiler generates because it won't contain additional Obsolete usage information.
    if (pproc->IsSyntheticMethod())
    {
        return;
    }

    // We'll do obsolete checking on the Property symbol proper when it comes around instead of
    // duplicating our efforts on the Get() and Set() methods.
    //
    if (pproc->IsPropertyGet() || pproc->IsPropertySet())
    {
        return;
    }

    // We'll do obsolete checking on the WithEvents member itself, so skip the WithEvents property.
    if (pproc->CreatedByWithEventsDecl())
    {
        return;
    }

    // We won't do obsolete checking on BeginInvoke or EndInvoke (only Invoke) to avoid
    // duplicating our efforts.
    //
    if (pParent->IsDelegate() &&
        (pParent->SimpleBind(NULL, STRING_CONST(m_pSourceFile->GetCompiler(), DelegateBeginInvoke)) == pproc ||
         pParent->SimpleBind(NULL, STRING_CONST(m_pSourceFile->GetCompiler(), DelegateEndInvoke)) == pproc))
    {
        return;
    }

    // Are the implemented members obsolete?
    //
    VSASSERT(!pproc->GetImplementsList() ||
             pproc->IsEventDecl() ||
             pproc->IsMethodImpl() ||
             pproc->IsProperty(),
                "How can anything else implement an interface member ?");

    if (NeedToCheckObsolete)
    {
        for (BCSYM_ImplementsList *pImplements = pproc->GetImplementsList();
                pImplements;
                pImplements = pImplements->GetNext())
        {
            CheckObsolete(
                pImplements->GetImplementedMember(),
                pImplements->GetLocation(),
                pParent);
        }
    }

    // For Events, only do obsolete checking on the underlying delegate type and Implements list.
    // All other checking (e.g., params) happens when we analyze the underlying delegate container.
    //
    if (NeedToCheckObsolete && pproc->IsEventDecl())
    {
        BCSYM * pRawDelegate = pproc->PEventDecl()->GetRawDelegate();

        if (pRawDelegate && pRawDelegate->DigThroughNamedType()->IsClass())
        {
            BCSYM * pDelegate = pproc->PEventDecl()->GetDelegate();

            // Is the underlying delegate type obsolete?
            // Underlying delegate type can be obsolete only if we didn't create it.
            //
            if (pDelegate && pDelegate->PClass()->CreatedByEventDecl() == NULL)
            {
                CheckObsolete(
                    pDelegate,
                    pproc->PEventDecl()->GetRawDelegate()->GetTypeReferenceLocation(),
                    pParent);
            }
        }

        return;
    }

    // Is the Return Type obsolete?
    if (pproc->GetType())
    {
        if (NeedToCheckObsolete)
        {
            CheckObsolete(
                pproc->GetType()->ChaseToType(),
                pproc->GetRawType()->GetTypeReferenceLocation(),
                pParent);
        }

        //check for restricted types: no returns as restricted types or arrays of restricted types
        CheckRestrictedType(
            ERRID_RestrictedType1,
            pproc->GetType()->ChaseToType(),
            pproc->GetRawType()->GetTypeReferenceLocation(),
            m_pSourceFile->GetCompilerHost(),
            m_pErrorTable);
    }

    // Are the Param Types obsolete?
    for (BCSYM_Param *pparam = pproc->GetFirstParam();
         pparam;
         pparam = pparam->GetNext())
    {
        if (NeedToCheckObsolete)
        {
            CheckObsolete(
                pparam->GetType()->ChaseToType(),
                pparam->GetRawType()->GetTypeReferenceLocation(),
                pParent);
        }

        //check for ByRef parameters to resumable methods
        if (pparam->IsByRefKeywordUsed() && (pproc->IsAsyncKeywordUsed() || pproc->IsIteratorKeywordUsed()))
        {
            m_pErrorTable->CreateError(pproc->IsAsyncKeywordUsed() ? ERRID_BadAsyncByRefParam : ERRID_BadIteratorByRefParam,
                                       pparam->GetLocation());
        }

        //restricted types can't be used in arrays
        else if (pparam->GetType()->IsArrayType())
        {
            CheckRestrictedArrayType(
                pparam->GetType(),
                pparam->GetRawType()->GetTypeReferenceLocation(),
                m_pSourceFile->GetCompilerHost(),
                m_pErrorTable);
        }
        //
        // For Byref parameters
        //
        else if (pparam->IsByRefKeywordUsed() && TypeHelpers::IsPointerType(GetDataType(pparam)))
        {
            CheckRestrictedType(
                ERRID_RestrictedType1,
                pparam->GetType()->PPointerType()->ChaseToType(),
                pparam->GetRawType()->GetTypeReferenceLocation(),
                m_pSourceFile->GetCompilerHost(),
                m_pErrorTable);
        }

        //nor in any parameters to resumable methods
        if (pproc->IsAsyncKeywordUsed() || pproc->IsIteratorKeywordUsed())
        {
            CheckRestrictedType(
                ERRID_RestrictedResumableType1,
                pparam->GetType(),
                pparam->GetRawType()->GetTypeReferenceLocation(),
                m_pSourceFile->GetCompilerHost(),
                m_pErrorTable);
        }


/*
        // Microsoft 5-2-2001:  Checking for obsolete usage within optional param values
        // won't be supported in VS7 because it's too hard.

        if (pparam->IsOptional() &&
            pparam->PParamWithValue()->GetExpression()->GetReferringDeclaration())
        {

            CheckObsolete(
                pparam->PParamWithValue()->GetExpression()->GetReferringDeclaration(),
                pparam->PParamWithValue()->GetExpression()->GetLocation(),
                pParent);
        }
*/
    }

    // Check Handles and Implements lists (which appear only on method impls)
    if (pproc->IsMethodImpl())
    {
        // Are the handled events obsolete?
        BCITER_Handles iterHandles(pproc->PMethodImpl());
        if (NeedToCheckObsolete)
        {
            for (BCSYM_HandlesList *pHandles = iterHandles.GetNext();
                 pHandles;
                 pHandles = iterHandles.GetNext())
            {
                BCSYM_EventDecl *eventSym = pHandles->GetEvent();

                CheckObsolete(
                    eventSym,
                    pHandles->GetLocationOfEvent(),
                    pParent);

                CheckObsolete(
                    pHandles->GetWithEventsProperty(),
                    pHandles->GetLocationOfWithEventsVar(),
                    pParent);

                CheckObsolete(
                    pHandles->GetEventSourceProperty(),
                    pHandles->GetLocation(),
                    pParent);

                if (eventSym && eventSym->IsWindowsRuntimeEvent()) 
                {
                    CheckObsolete(
                        eventSym->GetProcAdd(),
                        pHandles->GetLocationOfEvent(),
                        pParent);
                }
            }
        }
    }

    // Check the constraints of the generic params
    //
    if (NeedToCheckObsolete)
    {
        ScanGenericParamsForObsoleteUsage(
            pproc->GetFirstGenericParam(),
            pParent);
    }

    /*
    // Is the overriden member obsolete?
    if (pproc->GetOverriddenProc())
    {
        CheckObsolete(
            pproc->GetOverriddenProc(),
            pproc->GetLocation(),
            pParent,
            "overrides1");
    }
    */
}

void
ObsoleteChecker::ScanGenericParamsForObsoleteUsage
(
    BCSYM_GenericParam *FirstGenericParam,
    BCSYM_Container *Parent
) const
{
    for(BCSYM_GenericParam *GenericParam = FirstGenericParam;
        GenericParam;
        GenericParam = GenericParam->GetNextParam())
    {
        for(BCSYM_GenericTypeConstraint *Constraint = GenericParam->GetTypeConstraints();
            Constraint;
            Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            BCSYM *ConstraintType = Constraint->GetType()->ChaseToType();

            if (ConstraintType->IsBad())
            {
                continue;
            }

            CheckObsolete(
                ConstraintType,
                Constraint->GetLocation(),
                Parent);
        }
    }
}
