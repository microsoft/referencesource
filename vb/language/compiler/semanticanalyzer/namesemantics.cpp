//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB name reference semantic analysis.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

DefaultPartialGenericBindingFactory::DefaultPartialGenericBindingFactory(
    Compiler * pCompiler,
    CompilerHost * pCompilerHost,
    _In_ Symbols * pSymbols) :
    m_pCompiler(pCompiler),
    m_pCompilerHost(pCompilerHost),
    m_pSymbols(pSymbols)
{
    ThrowIfNull(pCompiler);
    ThrowIfNull(pCompilerHost);
    ThrowIfNull(pSymbols);
}

PartialGenericBinding * DefaultPartialGenericBindingFactory::Execute(
    Type * pReceiverType,
    _In_ Location * pReceiverLocation,
    Procedure * pProc)
{
    ThrowIfNull(pReceiverType);
    ThrowIfNull(pProc);
    ThrowIfNull(pProc->GetFirstParam());

    PartialGenericBinding * ret = NULL;

    if (pProc->IsGeneric())
    {
        unsigned int typeParameterCount = pProc->GetGenericParamCount();

        Type ** ppInferredTypeArguments = new (zeromemory) Type * [typeParameterCount];

        if
        (
            InferTypeArgumentsFromArgument
            (
                pReceiverLocation,
                pReceiverType,
                false, // not recevierTypeByAssumption
                GetDataType(pProc->GetFirstParam()),
                pProc->GetFirstParam(),
                NULL, /*typeInference*/
                ppInferredTypeArguments,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                pProc,
                m_pCompiler,
                m_pCompilerHost,
                *m_pSymbols,
                MatchBaseOfGenericArgumentToParameter,
                ConversionRequired::Any
            )
        )
        {
            BitVector<NorlsAllocWrapper> * pFixedArgumentBitVector =
                new (*(m_pSymbols->GetNorlsAllocator())) BitVector<NorlsAllocWrapper>
                (
                    NorlsAllocWrapper(m_pSymbols->GetNorlsAllocator()),
                    typeParameterCount
                );

            ThrowIfNull(pFixedArgumentBitVector);
            ThrowIfFalse(m_pSymbols && m_pSymbols->GetNorlsAllocator());

            Location * pArgumentLocations = (Location *)m_pSymbols->GetNorlsAllocator()->Alloc(sizeof(Location)*typeParameterCount);

            for (unsigned int i = 0; i < typeParameterCount; ++i)
            {
                pFixedArgumentBitVector->SetBit(i, ppInferredTypeArguments[i]);

                if (ppInferredTypeArguments[i] && pReceiverLocation)
                {
                    pArgumentLocations[i].SetLocation(pReceiverLocation);
                }
            }

            //Clear the generic binding cache so that the
            //partia; binding we create does not get stored in the cache.
            //And then restore the current cache value once we leave
            //the current scope.
            BackupGenericBindingCache backup_binding_cache(m_pSymbols);
            m_pSymbols->SetGenericBindingCache(NULL);

            ret =
                new (*(m_pSymbols->GetNorlsAllocator())) PartialGenericBinding
                (
                    pArgumentLocations,
                    m_pSymbols->GetGenericBinding
                    (
                        false,
                        pProc,
                        ppInferredTypeArguments,
                        typeParameterCount,
                        NULL,
                        true
                    )
                    ,pFixedArgumentBitVector
                );

        }

        delete [] ppInferredTypeArguments;

    }
    return ret;
}

bool Semantics::IsAccessible (Declaration * pDeclToCheck, Container * pContext)
{
    ThrowIfNull(pDeclToCheck);
    ThrowIfNull(pContext);

    return
        IsAccessible
        (
            pDeclToCheck,
            NULL,
            pContext,
            NULL,
            m_SymbolCreator,
            GetCompilerHost()
        );
}


Declaration * Semantics::ImmediateExtensionMethodLookup
(
    Container * pContainerToLookIn,
    _In_z_ Identifier * pNameToLookFor
)
{
    const bindSpaceMask = BINDSPACE_Normal;

#if HOSTED
    // For Host-provided symbols in the hosted compiler, no extension methods are supported.
    ASSERT(pContainerToLookIn, "[Semantics::ImmediateExtensionMethodLookup] 'pContainerToLookIn' parameter is NULL");
    if (pContainerToLookIn && pContainerToLookIn->GetExternalSymbol())
    {
        return NULL;
    }
#endif

    // Perf: We aren't interested in doing lookup on types that aren't marked as containing type extensions
    if (!pContainerToLookIn->IsTypeExtension())
    {
        return NULL;
    }

    // We try not to import children from metadata for types that aren't candidates for type extension lookup--see TypeExtensionIterator::MoveNext()
    // We want to Assert that we did import our children when we were supposed to though.  
    if ( pContainerToLookIn->GetChildrenNotLoaded() == true )
    {
        VSASSERT( false, "An optimization to not load children for type extensions unexpectedly left us without ever loading our children.");
        pContainerToLookIn->EnsureChildrenLoaded();
    }
    
    Declaration * pDecl =
        Semantics::ImmediateLookup
        (
            pContainerToLookIn->GetHash(),
            pNameToLookFor,
            bindSpaceMask
        );

     while
     (
        pDecl &&
        (
            ! pDecl->IsProc()  ||
            ! pDecl->IsExtensionMethod(IsInterpretingForAttributeExpression())
        )
     )
     {
        pDecl = pDecl->GetNextOfSameName(bindSpaceMask);
     }

     return pDecl;

}

bool
Semantics::IsInterpretingForAttributeExpression()
{
    return m_NamedContextForAppliedAttribute != NULL;
}

PartialGenericBinding *
Semantics::CreatePartialGenericBinding(Procedure * pProc)
{
    DefaultPartialGenericBindingFactory factory
    (
        GetCompiler(),
        GetCompilerHost(),
        &m_SymbolCreator
    );

    return factory.Execute(m_pReceiverType, m_pReceiverLocation, pProc);

}


Symbol *
Semantics::LookupInScope
(
    Scope *Lookup,
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    _In_z_ Identifier *Name,
    NameFlags Flags,
    Type *AccessingInstanceType,
    unsigned BindspaceMask,
    bool IgnoreImports,
    bool IgnoreModules,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    VSASSERT((Lookup && !TypeParamToLookupIn) || (!Lookup && TypeParamToLookupIn),
                "lookup hash and lookup type param should be mutually exclusive!!!");

    InitializeNameLookupGenericBindingContext(GenericBindingContext);

    if (TypeParamToLookupIn)
    {
        return
            LookupInGenericTypeParameter
            (
                TypeParamToLookupIn,
                Name,
                AccessingInstanceType,
                BindspaceMask,
                SourceLocation,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces,
                Flags
            );
    }

    if (IsNamespace(Lookup))
    {
        // only track if we're looking in the unnamed namespace of the source file where the extra import would be
        ImportTrackerEntry *pPassMe = NULL;
        if(m_SourceFile && ViewAsNamespace(Lookup)->IsSameNamespace( m_SourceFile->GetUnnamedNamespace()))
        {
            pPassMe = pImportTrackerEntry;
        }

        return
            LookupInNamespace(
                ViewAsNamespace(Lookup),
                Name,
                Flags,
                BindspaceMask,
                IgnoreImports,
                IgnoreModules,
                SourceLocation,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces,
                pPassMe);
    }

    bool IgnoreImmediateBases =
            (HasFlag(Flags, NameSearchIgnoreImmediateBases) &&
             m_Lookup->GetContainer() &&
             m_Lookup->GetContainer()->IsSameContainer(Lookup->GetContainer()));

    if (IsClassType(Lookup) &&
        !HasFlag(Flags, NameSearchImmediateOnly) &&
        !HasFlag(Flags, NameSearchIgnoreBases) &&
        !IgnoreImmediateBases)
    {
        return
            LookupInClass
            (
                ViewAsClass(Lookup),
                Name,
                AccessingInstanceType,
                BindspaceMask,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity,
                Flags
             );
    }

    bool ResultAccessibilityDetermined = false;
    bool IsResultAccessible = false;
    Declaration *Result = NULL;

    for(Declaration *CurrentResult = ImmediateLookup(Lookup, Name, BindspaceMask);
        CurrentResult;
        CurrentResult =
            CurrentResult->IsType() ?
                CurrentResult->GetNextOfSameName(BindspaceMask) :
                NULL)
    {
        if (!IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
        {
            continue;
        }

        if (Result)
        {
            bool IsCurrentResultAccessible = IsAccessible(CurrentResult, NULL, AccessingInstanceType);

            if (!ResultAccessibilityDetermined)
            {
                IsResultAccessible = IsAccessible(Result, NULL, AccessingInstanceType);
                ResultAccessibilityDetermined = true;
            }

            if (IsResultAccessible)
            {
                if (IsCurrentResultAccessible)
                {
                    // Ambiguous match
                    //
                    // Ideally the message should be general - "'|1' is ambiguous in |2 '|3'.")
                    // But based on current usage, this error will never be reported to the user.
                    // So instead set the NameIsBad flag to reflect this error only when error
                    // reporting is off. But if this code path is ever hit when error reporting
                    // is on, an assert is triggered and fails gracefully by returning a NULL
                    // which is okay since this function is expected to return NULL in some
                    // scenarios without setting the NameIsBad flag anyway.
                    //

                    if (m_ReportErrors)
                    {
                        VSFAIL("Unexpected error code path!!!");

                        //ReportSemanticError(
                        //    ERRID_AmbiguousInType3,
                        //    Name,
                        //    StringOfSymbol(m_Compiler, Lookup->GetParent()),
                        //    Lookup->GetParent());
                    }
                    else
                    {
                        NameIsBad = true;
                    }

                    return NULL;
                }
            }
            else if (IsCurrentResultAccessible)
            {
                Result = CurrentResult;
                IsResultAccessible = IsCurrentResultAccessible;
            }
        }
        else
        {
            Result = CurrentResult;
        }

        if (ContinueMemberLookupBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
        {
            continue;
        }

        break;
    }

    if (Result)
    {
        CreateNameLookupGenericBindingContext(Result, Lookup->GetParent(), GenericBindingContext);
    }


    // 


    if
    (
        IsInterface(Lookup) &&
        (
            Result ||
            (
                !HasFlag(Flags, NameSearchIgnoreBases) &&
                !IgnoreImmediateBases
            )
        )
    )
    {
        //We call into LookupInBaseInterfaces even in
        //the case where we have an immediate lookup result so that we can do an
        //extension method lookup to cover the case where our interface method is
        //overloaded with an extension method.
        //However, if we have a result we pass it in, which causes both the search in our
        //base interfaces for instance methods and the ambiguous base interface
        //member check to be suppressed. We supress the ambiguity check because if a method
        //exists in our interface and in a base interface, then it must be marked shadows
        //(and therefore must shadow the base member by name).
        //
        return
            LookupInBaseInterfaces
            (
                ViewAsInterface(Lookup),
                Name,
                Flags | (Result ? NameSearchIgnoreObjectMethodsOnInterfaces : NameNoFlags ),
                BindspaceMask,
                SourceLocation,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces,
                Result
            );
    }

    bool LookingForMethods = HasFlag(Flags, NameSearchMethodsOnly);

    if(LookingForMethods && Result &&
        !(
            IsProcedure(Result) &&
            !IsProperty(Result) &&
            !IsEvent(Result)
          )
        )
    {
        Result = NULL;
    }

    return Result;
}

Declaration *
Semantics::ImmediateLookup
(
    Scope *Lookup,
    _In_z_ Identifier *Name,
    unsigned BindspaceMask,
    CompilerProject *pCurrentProject
)
{
    Declaration *Result = NULL;
    Declaration *NameBinding = Lookup->SimpleBind(Name);

    while (NameBinding)
    {
        Declaration *nonAliasNameBinding = ChaseThroughAlias(NameBinding);
        VSASSERT(nonAliasNameBinding,"ImmediateLookup nonAliasNameBinding NULL");

        // Bug Dev11 29153
        // If the symbol is marked as Embedded, then it can be only used in the assembly where it is defined.
        // If the symbol is a member then we check its parent
        BCSYM_NamedRoot *pNameBindingContainer = nonAliasNameBinding->IsContainer() ? 
                                                     nonAliasNameBinding :  
                                                     nonAliasNameBinding->GetParent(); 

        if ((nonAliasNameBinding->GetBindingSpace() & BindspaceMask) &&
            (!pCurrentProject || !pNameBindingContainer || 
            !pNameBindingContainer->GetPWellKnownAttrVals()->GetEmbeddedData() || pNameBindingContainer->GetContainingProject() == pCurrentProject))
        {
            Result = NameBinding;
            return Result;
        }

        NameBinding = NameBinding->GetNextBound();
    }

    return Result;
}

//
// This method differs from ImmediateLookup because instead of calling
// GetNextBound() we need to call GetNextOfSameName() for symbols in the
// merged hash
//

Declaration *
Semantics::ImmediateLookupForMergedHash
(
    Scope *Lookup,
    _In_z_ Identifier *Name,
    unsigned BindspaceMask,
    CompilerProject *pCurrentProject
)
{
    Declaration *Result = NULL;
    Declaration *NameBinding = Lookup->SimpleBind(Name);

    while (NameBinding)
    {
        Declaration *nonAliasNameBinding = ChaseThroughAlias(NameBinding);
        VSASSERT(nonAliasNameBinding,"ImmediateLookupForMergedHash nonAliasNameBinding NULL");

        // Bug Dev11 29153
        // If the symbol is marked as Embedded, then it can be only used in the assembly where it is defined.
        // If the symbol is a member then we check its parent
        BCSYM_NamedRoot *pNameBindingContainer = nonAliasNameBinding->IsContainer() ? 
                                                     nonAliasNameBinding :  
                                                     nonAliasNameBinding->GetParent(); 

        if ((nonAliasNameBinding->GetBindingSpace() & BindspaceMask) &&
            (!pCurrentProject || !pNameBindingContainer || 
                !pNameBindingContainer->GetPWellKnownAttrVals()->GetEmbeddedData() || pNameBindingContainer->GetContainingProject() == pCurrentProject))
        {
            Result = NameBinding;
            return Result;
        }

        NameBinding = NameBinding->GetNextOfSameName();
    }

    return Result;
}

Type *
Semantics::GetTypeBeingExtended
(
    Procedure * pProc,
    _In_opt_ PartialGenericBinding * pPartialGenericBinding
)
{
    ThrowIfFalse(pProc && pProc->IsExtensionMethod(IsInterpretingForAttributeExpression()));
    ThrowIfNull(pProc->GetFirstParam());
    ThrowIfNull(pProc->GetFirstParam()->GetType());

    GenericBinding * pGenericBinding = NULL;

    if (pPartialGenericBinding)
    {
        pGenericBinding = pPartialGenericBinding->m_pGenericBinding;
    }

    return
        ReplaceGenericParametersWithArguments
        (
            pProc->GetFirstParam()->GetType(),
            pGenericBinding,
            m_SymbolCreator
        );
}

bool Semantics::IsInterfaceExtension(Procedure * pProc, _In_opt_ PartialGenericBinding * pPartialGenericBinding)
{
    ThrowIfNull(pProc);
    ThrowIfFalse(!pProc->IsExtensionMethod(IsInterpretingForAttributeExpression()) || pProc->GetFirstParam());
    ThrowIfFalse(!pProc->IsExtensionMethod(IsInterpretingForAttributeExpression()) || pProc->GetFirstParam()->GetType());

    return
        pProc->IsExtensionMethod(IsInterpretingForAttributeExpression()) &&
        GetTypeBeingExtended(pProc, pPartialGenericBinding)->IsInterface();
}


bool
Semantics::CanUseExtensionMethodCache()
{
    return
        m_PermitDeclarationCaching &&
        m_ExtensionMethodLookupCache;
}


ExtensionCallLookupResult *
Semantics::ExpandCacheEntryAndApplyFilters
(
    ExtensionMethodLookupCacheEntry * pEntry,
    Container * pContextOfCall
)
{
    ExtensionCallLookupResult * pRet = NULL;
    if (pEntry)
    {
        ExtensionMethodCacheEntryExplodingIterator iterator(m_Compiler, this, pEntry, pContextOfCall);

        while (iterator.MoveNext())
        {
            ExtensionCallAndPrecedenceLevel current = iterator.Current();

            PartialGenericBinding * pPartialBinding = CreatePartialGenericBinding(current.m_pProc);

            if (ApplySuperTypeFilter(m_pReceiverType, current.m_pProc, pPartialBinding))
            {
                if (!pRet)
                {
                    pRet = m_SymbolCreator.GetExtensionCallLookupResult();
                }

                ThrowIfNull(pRet);
                pRet->AddProcedure
                (
                    current.m_pProc,
                    current.m_precedenceLevel,
                    pPartialBinding
                );
            }
        }
    }

    return pRet;
}

void Semantics::DoUnfilteredExtensionMethodLookupInImportedTargets
(
    _In_z_ Identifier * pName ,
    ImportedTarget * pTargets,
    ImportedTarget * pExtraTargets,
    CompilerProject * pProject,
    _Inout_ ExtensionMethodLookupCacheEntry * & pCurrentItem,
    _Inout_ ExtensionMethodLookupCacheEntry * & pLastItem,
    _Inout_ ExtensionMethodLookupCacheEntry * & pRet,
    _Inout_ unsigned long & precedenceLevel,
    _Inout_ unsigned long & lastPrecedenceLevel
)
{
    TypeExtensionImportsIterator targetIterator(pTargets);
    TypeExtensionImportsIterator extraTargetIterator(pExtraTargets);

    ChainIterator<BCSYM_Class*> typeIterator(&targetIterator, &extraTargetIterator);

    while (typeIterator.MoveNext())
    {
        pCurrentItem =
            DoUnfilteredExtensionMethodLookupInClassImport
            (
                pName,
                typeIterator.Current()
            );

        if (pCurrentItem)
        {
            if (!pLastItem)
            {
                pLastItem = pCurrentItem;
            }
            else
            {
                if (! pRet)
                {
                    pRet = CreateExtensionMethodCacheEntry();
                    pRet->AddParentEntry(pLastItem, lastPrecedenceLevel);
                }
                pRet->AddParentEntry(pCurrentItem, precedenceLevel);
            }
        }
    }

    if (pLastItem)
    {
        ++precedenceLevel;
    }

    ImportedNamespaceRingIterator targetRingIterator(pTargets);
    ImportedNamespaceRingIterator extraTargetRingIterator(pExtraTargets);

    ChainIterator<BCSYM_NamespaceRing*> ringIterator(&targetRingIterator, &extraTargetRingIterator);


    while (ringIterator.MoveNext())
    {
        pCurrentItem =
            DoUnfilteredExtensionMethodLookupInNamespaceImport
            (
                pName,
                ringIterator.Current(),
                pProject
            );

        if (pCurrentItem)
     {
            if (! pLastItem)
            {
                pLastItem = pCurrentItem;
                lastPrecedenceLevel = precedenceLevel;
            }
            else
            {
                if (!pRet)
                {
                    pRet = CreateExtensionMethodCacheEntry();
                    pRet->AddParentEntry(pLastItem, lastPrecedenceLevel);
                }
                pRet->AddParentEntry(pCurrentItem, precedenceLevel);
            }
        }
    }

    //it's ok if there are gaps in the precedence level (as long as values are correctly relative to one another),
    //so if we increment twice when we only should have incremeneted once
    //it's not a big deal.
    if (pLastItem)
    {
        ++precedenceLevel;
    }
}


ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInSourceFileImports
(
    _In_z_ Identifier * pName ,
    SourceFile * pSourceFile
)
{
    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject(pSourceFile),
                Usage_InsideObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    unsigned long precedenceLevel = 0;
    ExtensionMethodLookupCacheEntry * pCurrentItem = NULL;
    ExtensionMethodLookupCacheEntry * pLastItem = NULL;
    unsigned long lastPrecedenceLevel = precedenceLevel;

    CompilerProject * pProject = pSourceFile->GetProject();

    DoUnfilteredExtensionMethodLookupInImportedTargets
    (
        pName,
        pSourceFile->GetUnnamedNamespace()->GetImports(),
        pSourceFile->ShouldSemanticsUseExtraImportForExtensionMethods() ? pSourceFile->GetExtraTrackedImport() : NULL,
        pProject,
        pCurrentItem,
        pLastItem,
        pRet,
        precedenceLevel,
        lastPrecedenceLevel
    );

    DoUnfilteredExtensionMethodLookupInImportedTargets
    (
        pName,
        pSourceFile->GetProject()->GetImportedTargets(),
        pSourceFile->ShouldSemanticsUseExtraImportForExtensionMethods() ? pSourceFile->GetExtraTrackedImport() : NULL,
        pProject,
        pCurrentItem,
        pLastItem,
        pRet,
        precedenceLevel,
        lastPrecedenceLevel
    );

    // Have the sourcefile remember that it has checked imports for extension methods.  This ensures that all metadata
    // for extension method lookup has been loaded and that the ExtensionMethodExistsCache in the project are fully up to
    // to date and can be used to avoid unnnecessary extension method lookup.
    pSourceFile->SetImportedExtensionMethodMetaDataLoaded(true);


    if (pLastItem && ! pRet)
    {
        pRet = pLastItem;
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject(pSourceFile),
            Usage_InsideObject,
            pRet
        );
    }

    return pRet;

}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInClassImport
(
    _In_z_ Identifier * pName,
    BCSYM_Class * pClass
)
{
    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject(pClass->GetMainType() ? pClass->GetMainType() : pClass),
                Usage_ReferenceObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    Declaration * pSym = ImmediateExtensionMethodLookup(pClass, pName);

    if (pSym)
    {
        pRet = CreateExtensionMethodCacheEntry();
        pRet->m_method = ViewAsProcedure(pSym);
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject(pClass->GetMainType() ? pClass->GetMainType() : pClass),
            Usage_ReferenceObject,
            pRet
        );
    }

    return pRet;

}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInNamespaceImport
(
    _In_z_ Identifier * pName,
    BCSYM_NamespaceRing * pRing,
    CompilerProject * pReferencingProject
)
{
    if (!DoesRingContainExtensionMethods(pRing))
    {
        return NULL;
    }

    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject(pRing),
                Usage_ReferenceObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    VisibleNamespacesInRingIterator nsIter(pRing, pReferencingProject);

    ExtensionMethodLookupCacheEntry * pLastTypeEntry = NULL;
    ExtensionMethodLookupCacheEntry * pCurrentTypeEntry = NULL;

    while (nsIter.MoveNext())
    {
        TypeExtensionIterator typeExtensionIterator(nsIter.Current());

        while (typeExtensionIterator.MoveNext())
        {
            pCurrentTypeEntry = DoUnfilteredExtensionMethodLookupInClassImport(pName, typeExtensionIterator.Current());
            if (pCurrentTypeEntry)
            {
                if (!pLastTypeEntry)
                {
                    pLastTypeEntry = pCurrentTypeEntry;
                }
                else
                {
                    if (!pRet)
                    {
                        pRet = CreateExtensionMethodCacheEntry();
                        pRet->AddParentEntry(pLastTypeEntry, 0);
                    }
                    pRet->AddParentEntry(pCurrentTypeEntry, 0);
                }
            }
        }
    }

    if (pLastTypeEntry && ! pRet)
    {
        pRet = pLastTypeEntry;
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject(pRing),
            Usage_ReferenceObject,
            pRet
        );
    }

    return pRet;
}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInNamespace
(
    _In_z_ Identifier * pName,
    BCSYM_NamespaceRing * pRing,
    CompilerProject * pReferencingProject
)
{
    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject(pRing),
                Usage_InsideObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    ExtensionMethodLookupCacheEntry * pImportLookupEntry =
        DoUnfilteredExtensionMethodLookupInNamespaceImport
        (
            pName,
            pRing,
            pReferencingProject
        );

    ExtensionMethodLookupCacheEntry * pParentNamespaceEntry = NULL;

    if (pRing->GetFirstNamespace()->GetParent())
    {
        pParentNamespaceEntry =
            DoUnfilteredExtensionMethodLookupInNamespace
            (
                pName,
                pRing->GetFirstNamespace()->GetParent()->PNamespace()->GetNamespaceRing(),
                pReferencingProject
            );
    }

    if (pImportLookupEntry && pParentNamespaceEntry)
    {
        pRet = CreateExtensionMethodCacheEntry();
        pRet->AddParentEntry(pImportLookupEntry, 0);
        pRet->AddParentEntry(pParentNamespaceEntry, 1);
    }
    else if (pImportLookupEntry)
    {
        pRet = pImportLookupEntry;
    }
    else
    {
        pRet = pParentNamespaceEntry;
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject(pRing),
            Usage_InsideObject,
            pRet
        );
    }

    return pRet;

}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInClass
(
    _In_z_ Identifier * pName,
    BCSYM_Class * pClass
)
{
    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject(pClass->GetMainType() ? pClass->GetMainType() : pClass),
                Usage_InsideObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    if (pClass->IsStdModule())
    {
        Declaration * pDecl = ImmediateExtensionMethodLookup(pClass, pName);

        if (pDecl)
        {
            pRet = CreateExtensionMethodCacheEntry();
            pRet->m_method = ViewAsProcedure(pDecl);
        }
    }

    BCSYM_NamespaceRing * pRing = pClass->GetContainingNamespace()->GetNamespaceRing();

    ExtensionMethodLookupCacheEntry * pNsEntry =
        DoUnfilteredExtensionMethodLookupInNamespace
        (
            pName,
            pRing,
            pClass->GetCompilerFile()->GetProject()
        );

    if (pRet && pNsEntry)
    {
        pRet->AddParentEntry(pNsEntry,1);
    }
    else if (pNsEntry)
    {
        pRet = pNsEntry;
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject(pClass->GetMainType() ? pClass->GetMainType() : pClass),
            Usage_InsideObject,
            pRet
        );
    }

    return pRet;
}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookupInContainer
(
    _In_z_ Identifier * pName,
    Container * pContainer
)
{
    if (!pContainer)
    {
        ReportSemanticError(ERRID_InternalCompilerError, Location());
        return NULL;
    }

    //Step 1.
    //        Check to see if pContainer is a class. If so, call DoUnfilteredExtensionMethodLookupInClass
    //Step 2.
    //        Otherwise its a namespace, so call DoUnfilteredExtensionMethodLookupInNamespace

    if (pContainer->IsClass())
    {
        return
            DoUnfilteredExtensionMethodLookupInClass
            (
                pName,
                pContainer->PClass()
            );
    }
    else
    {
        if (! pContainer->IsNamespace())
        {
            ReportSemanticError(ERRID_InternalCompilerError, Location());
            return NULL;
        }

        return
            DoUnfilteredExtensionMethodLookupInNamespace
            (
                pName,
                pContainer->PNamespace()->GetNamespaceRing(),
                pContainer->GetCompilerFile()->GetProject()
            );
    }
}

ExtensionMethodLookupCacheEntry *
Semantics::DoUnfilteredExtensionMethodLookup
(
    _In_z_ Identifier * pName,
    Container * pContainer
)
{
    //Step 1.
    //        If caching is enabled, use the cache.
    //Step 2.
    //        Otherwise Do the lookup for the containing class. This will then do the lookup in containing namespaces.
    //Step 3.
    //        Then Do the lookup in the source file. This will then do the lookup in the project.
    //    Dev10#813196: Note that the code will PREFER the results of Step3 over the results of Step2, in case it has both.
    //Step 4.
    //        Create the cache entry to return
    //Step 5.
    //        If caching is enable, insert the entry into the cache.
    //Step 6.
    //        Return the result

    SourceFile * pSourceFile = m_SourceFile ? m_SourceFile : pContainer->GetSourceFile();

    ExtensionMethodLookupCacheEntry * pRet = NULL;

    if (CanUseExtensionMethodCache())
    {
        if
        (
            m_ExtensionMethodLookupCache->LookupInCache
            (
                pName,
                ExtensionMethodLookupCacheKeyObject
                (
                    pSourceFile,
                    pContainer
                ),
                Usage_InsideObject,
                &pRet
            )
        )
        {
            return pRet;
        }
    }

    ExtensionMethodLookupCacheEntry * pContainingClassEntry =
        DoUnfilteredExtensionMethodLookupInContainer
        (
            pName,
            pContainer
        );

    ExtensionMethodLookupCacheEntry * pSourceFileEntry =
        DoUnfilteredExtensionMethodLookupInSourceFileImports
        (
            pName,
            pSourceFile
        );

    if (pContainingClassEntry && pSourceFileEntry)
    {
        pRet = CreateExtensionMethodCacheEntry();
    }

    if (pContainingClassEntry)
    {
        if (pRet)
        {
            pRet->AddParentEntry(pContainingClassEntry, 0);
        }
        else
        {
            pRet = pContainingClassEntry;
        }
    }

    if (pSourceFileEntry)
    {
        if (pRet)
        {
            pRet->AddParentEntry(pSourceFileEntry, 1);
        }
        else
        {
            pRet = pSourceFileEntry;
        }
    }

    if (CanUseExtensionMethodCache())
    {
        m_ExtensionMethodLookupCache->AddEntry
        (
            pName,
            ExtensionMethodLookupCacheKeyObject
            (
                pSourceFile,
                pContainer
            ),
            Usage_InsideObject,
            pRet
        );
    }

    return pRet;
}

ExtensionMethodLookupCacheEntry *
Semantics::CreateExtensionMethodCacheEntry()
{
    NorlsAllocator * pNorls = NULL;

    if (CanUseExtensionMethodCache())
    {
        pNorls = m_ExtensionMethodLookupCache->GetNorlsAllocator();
    }
    else
    {
        pNorls = &m_TreeStorage;
    }

    return new (*pNorls) ExtensionMethodLookupCacheEntry (NorlsAllocWrapper(pNorls));
}

ExtensionCallLookupResult *
Semantics::LookForExtensionMethods
(
    _In_z_ Identifier * pName,
    Container * pContextOfCall
)
{
    ThrowIfNull(pName);
    ThrowIfNull(pContextOfCall);

    // DevDiv 112740, 93014
    // When we are not in a bound state we should not be looking for Extension Methods.  They are
    // not valid until we hit a bound state.
    //
    // The only time Extension method lookup will occur before we hit a bound state is while
    // we are cracking attributes.  The resolution for 93014 is that we will not allow this to occur
    // so we should explicitly stop here.
    //
    // Also if we allow this to occur it will cause the extension method cache to be invalidated.
    // The cache assumes that we are in a bound state when it runs and thus that attributes are
    // ----ed.  If not all of the extension method attributes are ----ed and we lookup a name
    // in an attribute all of the extension methods attributes that are not ----ed will not be
    // put in the cache and future calls to ::LookForExtensionMethod will fail to find the value.
    if ( pContextOfCall
        && pContextOfCall->GetSourceFile()
        && pContextOfCall->GetSourceFile()->GetCompState() < CS_Bound )
    {
        return NULL;
    }

    if
    (
        ! m_pReceiverType ||
        (
            m_pReceiverType->DigThroughAlias()->IsClass() &&
            m_pReceiverType->DigThroughAlias()->PClass()->ContainsExtensionMethods()
        ) ||
        TypeHelpers::EquivalentTypes
        (
            m_pReceiverType,
            GetFXSymbolProvider()->GetType(FX::ObjectType)
        )
    )
    {
        return NULL;
    }

    pContextOfCall = pContextOfCall->DigUpToModuleOrNamespace();

    ExtensionCallLookupResult * pRet = NULL;
    ExtensionMethodLookupCacheEntry * pCacheEntry = DoUnfilteredExtensionMethodLookup(pName, pContextOfCall);

    if (pCacheEntry)
    {
        pRet = ExpandCacheEntryAndApplyFilters(pCacheEntry, pContextOfCall);
    }

    return pRet;
}

Symbol *
Semantics::LookupInClass
(
    ClassOrRecordType *OriginalLookup,
    _In_z_ Identifier *Name,
    Type *AccessingInstanceType,
    unsigned BindspaceMask,
    _Out_ bool &NameIsBad,
    _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    NameFlags Flags
)
{
    InitializeNameLookupGenericBindingContext(GenericBindingContext);

    Declaration *Result = NULL;

    //Iterate over the chain of base classes, starting from OriginalLookup
    for (ClassOrRecordType *Lookup = OriginalLookup;
         Lookup;
         Lookup = Lookup->GetCompilerBaseClass())
    {
        if (TypeHelpers::IsBadType(Lookup))
        {
            NameIsBad = true;
            return Symbols::GetGenericBadNamedRoot();
        }

        // We should load our children before attempting a lookup
        Lookup->EnsureChildrenLoaded();

        Result = NULL;

        bool IsResultAccessible = false;
        bool ResultAccessibilityDetermined = false;

        //Iterate over the members of the current class (Lookup)
        //with the given Name.
        for(Declaration *CurrentResult =
                ImmediateLookup(ViewAsScope(Lookup), Name, BindspaceMask);
            CurrentResult;
            CurrentResult =
                CurrentResult->IsType() ?
                    CurrentResult->GetNextOfSameName(BindspaceMask) :
                    NULL)
        {
            //If the current member does not match the generic arity we are seeking, then we
            //continue looking...
            if (!IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
            {
                continue;
            }

            if (Result)
            {
                //If we get here, it means we have found more than one member in the same class with the same
                //name tht matches the generic arity we are looking for.
                //We now check to see if they are both accessible from the accessing instance type.
                //If they both are accessible, we generate an ambiguity error and reutrn null.
                //If one is accessible and the other is not, we pick the accessible one as the current result and
                //then look for more matches.

                bool IsCurrentResultAccessible = IsAccessible(CurrentResult, NULL, AccessingInstanceType);

                if (!ResultAccessibilityDetermined)
                {
                    IsResultAccessible = IsAccessible(Result, NULL, AccessingInstanceType);
                    ResultAccessibilityDetermined = true;
                }

                if (IsResultAccessible)
                {
                    if (IsCurrentResultAccessible)
                    {
                        // Ambiguous match
                        //
                        // Ideally the message should be general - "'|1' is ambiguous in |2 '|3'.")
                        // But based on current usage, this error will never be reported to the user.
                        // So instead set the NameIsBad flag to reflect this error only when error
                        // reporting is off. But if this code path is ever hit when error reporting
                        // is on, an assert is triggered and fails gracefully by returning a NULL
                        // which is okay since this function is expected to return NULL in some
                        // scenarios without setting the NameIsBad flag anyway.
                        //

                        //This will only happen in the case where Result and CurrentResult are both types
                        //and both are accessible. In those cases it's not possible to bind to an extension method,
                        //so we don't need to do the search for extension methods that we do in other failure cases below.
                        //As a result it's ok to just return NULL here.

                        if (m_ReportErrors)
                        {
                            VSFAIL("Unexpected error code path!!!");

                            //ReportSemanticError(
                            //    ERRID_AmbiguousInType3,
                            //    Name,
                            //    StringOfSymbol(m_Compiler, Lookup),
                            //    Lookup);
                        }
                        else
                        {
                            NameIsBad = true;
                        }

                        return NULL;
                    }
                }
                else if (IsCurrentResultAccessible)
                {
                    Result = CurrentResult;
                    IsResultAccessible = IsCurrentResultAccessible;
                }
            }
            else
            {
                Result = CurrentResult;
            }

            //If the current match is a type and we don't care about generic arity (GenericTypeArity == -1)
            //then we will continue to look so that we can report an ambiguity error if necessary.
            //Otherwise we stop and use the current result as our result.
            if (ContinueMemberLookupBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
            {
                continue;
            }

            break;
        }

        // Ignore inaccessible members if an accessible member is available
        // in a base class.
        //
        // If the method is overloaded, then we check for the most permissible
        // of the overloads. If all of them are inaccessible, we can safely ignore them
        // because the first thing overloads does is drop all inaccessible members from
        // the set.
        //
        if (Result)
        {
            if (!(HasFlag(Flags, NameSearchIgnoreAccessibility) || CanBeAccessible(
                    Result,
                    NULL, // NULL binding good enough for base member
                    AccessingInstanceType)) &&
                (!IsProcedure(Result) || !ViewAsProcedure(Result)->IsAnyConstructor()))
            {
                bool BaseClassNameIsBad = false;
                Declaration *BaseResult =
                    EnsureNamedRoot
                    (
                        LookupInClass
                        (
                            Lookup->GetCompilerBaseClass(),
                            Name,
                            AccessingInstanceType,
                            BindspaceMask,
                            BaseClassNameIsBad,
                            NULL,                   // when looking up a member from the base class,
                                                    // don't try to obtain the generic binding. Instead
                                                    // compute it in the more derived class i.e. here
                                                    // where there is more generic context available.
                            GenericTypeArity,
                            Flags | NameSearchIgnoreExtensionMethods
                        )
                    );

                if (!BaseClassNameIsBad &&
                    BaseResult &&
                    CanBeAccessible(
                        BaseResult->PNamedRoot(),
                        NULL, // NULL binding good enough for base member
                        AccessingInstanceType))
                {
                    Result = BaseResult;
                }
            }

            break;

        }
    }

    bool LookingForMethods = HasFlag(Flags, NameSearchMethodsOnly);

    if(LookingForMethods && Result &&
        !(
            IsProcedure(Result) &&
            !IsProperty(Result) &&
            !IsEvent(Result)
          )
        )
    {
        Result = NULL;
    }

    if (Result && GenericBindingContext)
    {
        CreateNameLookupGenericBindingContext(Result->PNamedRoot(), OriginalLookup, GenericBindingContext, AccessingInstanceType);
    }

   return
    LookForExtensionMethods
    (
        Name,
        Result,
        AccessingInstanceType,
        GenericBindingContext,
        Flags
    );
}


bool 
Semantics::ExtensionMethodExists(CompilerProject *pProject, _In_z_ STRING *pName)
{

    STRING_INFO *pNameInfo = StringPool::Pstrinfo(pName);

    // Check current project
    
    HashSet<STRING_INFO*> * pExtensionMethodExistsCache =
        pProject->GetExtensionMethodExistsCache();

    if (pExtensionMethodExistsCache->Count() > 0 && pExtensionMethodExistsCache->Contains(pNameInfo))
    {
        return true;
    }         

    // Check referenced projects

    ReferenceIterator iter(pProject);
    CompilerProject *pCurrent;

    while (pCurrent = iter.Next())
    { 
        pExtensionMethodExistsCache = pCurrent->GetExtensionMethodExistsCache();        
        if (pExtensionMethodExistsCache->Count() > 0 && pExtensionMethodExistsCache->Contains(pNameInfo))
        {
            return true;
        }              
    }    
    
    return false;
}



Symbol *
Semantics::LookForExtensionMethods
(
    _In_z_ Identifier *Name,
    Declaration * Result,
    Type * AccessingInstanceType,
    BCSYM_GenericBinding **GenericBindingContext,
    NameFlags Flags
)
{ 

    // VB is looking for extension methods too early. Extension methods cannot shadow instance methods unless
    // the number of paramters of the extension method match better than the instance method.  Because it is too risky
    // in RC to move the extension method lookup, we use a cache of extension method names to check whether we should
    // try to find an extension method.  This cache should be removed in the future and the extension method lookup should
    // be deferred until it is really needed.
    //
    // Because meta data is loaded incrementally, the cache is  unusable until all of the metadata needed for a sourcefile is
    // loaded. Each sourcefile knows when the metadata for its imports and project level imports has been loaded.  If the metadata 
    // has been loaded then the cache is valid and we can use it to know whether an extension method exists.
   
    if (IsExtensionMethodExistsCacheValid() &&
        !ExtensionMethodExists(m_Project, Name))
    {
        return Result;
    }
   
  		
    GenericBinding * tmpGenericBindingContext = NULL;

    if (!GenericBindingContext)
    {
        GenericBindingContext = & tmpGenericBindingContext;
    }

    bool ResultCanBeAccessible =
        Result ?
        CanBeAccessible
        (
            Result,
            *GenericBindingContext,
            AccessingInstanceType
        ) :
        false;

    Symbol * ReturnValue = Result;

    if
    (
        FlagsAllowExtensionMethods(Flags) &&
        (
            !ResultCanBeAccessible ||
            ! Result ||
            IsBad(Result) ||
            Result->DigThroughAlias()->CanOverloadWithExtensionMethods()
        )
    )
    {

        BCSYM_ExtensionCallLookupResult *  ExtensionCallResult =
            LookForExtensionMethods
            (
                Name,
                ContainingContainer()
            );

        if
        (
            ExtensionCallResult
        )
        {

            if (Result && ResultCanBeAccessible && ! IsBad(Result))
            {
                BCSYM_GenericBinding * pGenericBinding = NULL;
                ExtensionCallResult->SetInstanceMethodResults(Result->PNamedRoot(), *GenericBindingContext, AccessingInstanceType);
            }

            ReturnValue = ExtensionCallResult;
            *GenericBindingContext = NULL;
        }
        else
        {
            if (!Result)
            {
                ReturnValue = ExtensionCallResult;
                *GenericBindingContext = NULL;
            }
        }

    }
    return ReturnValue;

}


Declaration *
Semantics::LookupInBaseClasses
(
    ClassOrRecordType *Lookup,
    _In_z_ Identifier *Name,
    unsigned BindspaceMask
)
{
    Declaration *Result = NULL;

    for (ClassOrRecordType *Base = Lookup->GetCompilerBaseClass();
         Base;
         Base = Base->GetCompilerBaseClass())
    {
        if (TypeHelpers::IsBadType(Base))
        {
            return Symbols::GetGenericBadNamedRoot();
        }

        Result = ImmediateLookup(ViewAsScope(Base), Name, BindspaceMask);

        if (Result)
        {
            return Result;
        }
    }

    return NULL;
}


Symbol *
Semantics::LookupInBaseInterfaces
(
    InterfaceType *Lookup,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
    Declaration * Result
)
{

    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext?
            GenericBindingContext:
            &TempGenericBindingContext;

    bool fallBackNameAbiguityField = false;
    if ( ! NameAmbiguousAcrossBaseInterfaces)
    {
        NameAmbiguousAcrossBaseInterfaces = & fallBackNameAbiguityField;
    }

    ThrowIfNull(NameAmbiguousAcrossBaseInterfaces);

    if (!Result)
    {

        InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

        // Special case for COM interfaces.
        //
        // Performing a lookup in a CoClass interface affects how we treat ambiguities between events and other members.
        // In COM, events are separated into their own binding space, thus it is possible for an event and member to have
        // the same name.  This is not possible in the .NET world, but for backwards compatibility, especially with Office,
        // the compiler will ignore ambiguities when performing a lookup in a CoClass interface.  Example:
        //
        //     Interface _Foo
        //        Sub Quit
        //
        //     Interface FooSource
        //        Event Quit
        //
        //     <System.Runtime.InteropServices.CoClass(GetType(FooClass))> _
        //     Interface Foo : Inherits _Foo, FooSource
        //
        //     Class FooClass : Implements Foo
        //         Event Quit Implements Foo.Quit
        //         Sub Quit Implements Foo.Quit
        //
        // Set the CoClass context flag here so we know deeper in the recursion to ignore the ambiguity (VS#513916).

        WCHAR *CoClassName;  // Just a dummy pointer.
        if (Lookup->GetPWellKnownAttrVals()->GetCoClassData(&CoClassName))
        {
            SetFlag(Flags, NameSearchCoClassContext);
        }

        for (InterfaceList *Base = Lookup->GetFirstImplements();
             Base;
             Base = Base->GetNext())
        {
            if (Base->IsBadImplements())
            {
                NameIsBad = true;
                return NULL;
            }

            Symbol *ChasedBase = Base->GetInterfaceIfExists();

            LookupInBaseInterface(
                ChasedBase,
                Name,
                Flags,
                BindspaceMask,
                SourceLocation,
                NameIsBad,
                Result,
                *ResultGenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces);

            if (NameIsBad)
            {
                return Result;
            }
        }
    }

    // Include object's members in lookup process
    if ( ! Result && FlagsAllowObjectMethodsOnInterfaces(Flags) && ! *NameAmbiguousAcrossBaseInterfaces)
    {
        Result =
            EnsureNamedRoot
            (
                LookupInClass
                (
                    GetFXSymbolProvider()->GetObjectType()->PClass(),
                    Name,
                    GetFXSymbolProvider()->GetObjectType(),
                    BindspaceMask,
                    NameIsBad,
                    GenericBindingContext,
                    GenericTypeArity,
                    Flags | NameSearchIgnoreExtensionMethods
                )
            );
    }

    bool LookingForMethods = HasFlag(Flags, NameSearchMethodsOnly);

    if(LookingForMethods && Result &&
        !(
            IsProcedure(Result) &&
            !IsProperty(Result) &&
            !IsEvent(Result)
          )
        )
    {
        Result = NULL;
    }

    return
            LookForExtensionMethods
            (
                Name,
                Result,
                NULL,
                GenericBindingContext,
                Flags
            );

}

void
Semantics::LookupInBaseInterface
(
    Type *PossibleBaseInterface,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Inout_ Declaration *&Result,
    _Inout_ GenericBinding *&ResultGenericBindingContext,
    int GenericTypeArity,
    _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces
)
{
    VSASSERT(&ResultGenericBindingContext, "NULL in/out buffer unexpected for generic binding context!!!");

    if (PossibleBaseInterface == NULL || !PossibleBaseInterface->IsInterface())
    {
        // At some points during Bindable, the base interface may not be filled in.
        // (In particular, this occurs during the interpretation of an Inherits
        // statement within the interface.)
        //
        // If the Interface is inheriting from anything other than an interface,
        // then this inheritance is bad, but we can't mark it as such because bindable
        // depends on the fact that this ImplementsNode is not bad to log the error.
        // VS#277555
        //
        return;
    }

    if (PossibleBaseInterface->IsBad())
    {
        NameIsBad = true;
        Result = NULL;
        return;
    }

    InterfaceType *BaseInterface = PossibleBaseInterface->PInterface();

    GenericBinding *CandidateGenericBindingContext = NULL;

    Declaration *Candidate = NULL;
    Declaration *CurrentCandidate;
    for(CurrentCandidate =
            ImmediateLookup(ViewAsScope(BaseInterface), Name, BindspaceMask);
        CurrentCandidate;
        CurrentCandidate =
            CurrentCandidate->IsType() ?
                CurrentCandidate->GetNextOfSameName(BindspaceMask) :
                NULL)
    {
        if (!IsMemberSuitableBasedOnGenericTypeArity(CurrentCandidate, GenericTypeArity))
        {
            continue;
        }

        if (Candidate)
        {
            // Ambiguous match
            //
            // Ideally the message should be general - "'|1' is ambiguous in |2 '|3'.")
            // But based on current usage, this error will never be reported to the user.
            // So instead set the NameIsBad flag to reflect this error only when error
            // reporting is off. But if this code path is ever hit when error reporting
            // is on, an assert is triggered and fails gracefully by returning a NULL
            // which is okay since this function is expected to return NULL in some
            // scenarios without setting the NameIsBad flag anyway.
            //

            if (m_ReportErrors)
            {
                VSFAIL("Unexpected error code path!!!");

                // The idea with not setting NameIsBad as true here is that if ever this
                // code path is hit, the rest of the compiler will still be able to handle
                // this as a member not found and report the corresponding errors rather than
                // as though this error has already been reported by this API when it really
                // is not reported.

                //ReportSemanticError(
                //    ERRID_AmbiguousInType3,
                //    Name,
                //    StringOfSymbol(m_Compiler, BaseInterface),
                //    BaseInterface);
            }
            else
            {
                if (NameAmbiguousAcrossBaseInterfaces)
                {
                    *NameAmbiguousAcrossBaseInterfaces = true;
                }

                NameIsBad = true;
            }

            Result = NULL;
            return;
        }

        Candidate = CurrentCandidate;

        if (ContinueMemberLookupBasedOnGenericTypeArity(CurrentCandidate, GenericTypeArity))
        {
            continue;
        }

        break;
    }


    if (Candidate == NULL)
    {
        Candidate =
            EnsureNamedRoot
            (
                LookupInBaseInterfaces(
                    BaseInterface,
                    Name,
                    Flags | NameSearchIgnoreExtensionMethods | NameSearchIgnoreObjectMethodsOnInterfaces,
                    BindspaceMask,
                    SourceLocation,
                    NameIsBad,
                    &CandidateGenericBindingContext,
                    GenericTypeArity,
                    NameAmbiguousAcrossBaseInterfaces,
                    NULL
                )
            );

        // Update the binding obtained based on the base binding
        UpdateNameLookupGenericBindingContext(PossibleBaseInterface, &CandidateGenericBindingContext);
    }
    else
    {
        bool LookingForMethods = HasFlag(Flags, NameSearchMethodsOnly);

        if(LookingForMethods &&
            !(
                IsProcedure(Candidate) &&
                !IsProperty(Candidate) &&
                !IsEvent(Candidate)
              )
            )
        {
            Candidate = NULL;
        }
        else
        {
            CreateNameLookupGenericBindingContext(Candidate, PossibleBaseInterface, &CandidateGenericBindingContext);
        }
    }

    if (NameIsBad)
    {
        Result = Candidate;
        return;
    }

    if (Candidate)
    {
        // Ignore the candidate if we are binding in a CoClass context and it doesn't match the kind of member
        // we are looking for (see above).
        if (HasFlag(Flags, NameSearchCoClassContext))
        {
            if (HasFlag(Flags, NameSearchEventReference) && !IsEvent(Candidate))
            {
                return;
            }

            if (!HasFlag(Flags, NameSearchEventReference) && IsEvent(Candidate))
            {
                return;
            }
        }

        // Finding a declaration on two separate paths is an
        // ambiguity error unless it results from finding the same
        // declaration from two different inheritances from the
        // same interface.
        //
        // Interface members are always public, so no checking of
        // accessibility is required.

        if (Result)
        {
            // When multiple members are found through different
            // inheritance paths, if the parents of the members
            // have an inheritance relation, then the member from
            // the more derived type wins.

            if (Result != Candidate)
            {
                BCSYM *ResultParent =
                    ResultGenericBindingContext ? ResultGenericBindingContext : Result->GetParent();

                BCSYM *CandidateParent =
                    CandidateGenericBindingContext ? CandidateGenericBindingContext : Candidate->GetParent();

                if (TypeHelpers::IsOrInheritsFrom(ResultParent, CandidateParent))
                {
                    // Result is from more derived, so ignore Candidate
                    return;
                }
                else if (TypeHelpers::IsOrInheritsFrom(CandidateParent, ResultParent))
                {
                    // Candidate is from more derived, so ignore Result

                    Result = Candidate;
                    ResultGenericBindingContext = CandidateGenericBindingContext;
                    return;
                }
            }
            else if (TypeHelpers::EquivalentTypeBindings(
                        ResultGenericBindingContext->PGenericTypeBinding(),
                        CandidateGenericBindingContext->PGenericTypeBinding()))
            {
                return;
            }

            ReportSemanticError(
                ERRID_AmbiguousAcrossInterfaces3,
                SourceLocation,
                Name,
                ResultGenericBindingContext ?
                    ResultGenericBindingContext :
                    Result->GetParent(),
                CandidateGenericBindingContext ?
                    CandidateGenericBindingContext :
                    Candidate->GetParent());

            if (NameAmbiguousAcrossBaseInterfaces)
            {
                *NameAmbiguousAcrossBaseInterfaces = true;
            }

            NameIsBad = true;
            Result = NULL;
            ResultGenericBindingContext = NULL;
            return;
        }

        Result = Candidate;
        ResultGenericBindingContext = CandidateGenericBindingContext;
    }

    return;
}

Symbol *
Semantics::LookupInGenericTypeParameter
(
    GenericParameter *TypeParameter,
    _In_z_ Identifier *Name,
    Type *AccessingInstanceType,
    unsigned BindspaceMask,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
    NameFlags Flags
)
{

    bool defaultAmbiguityField = false;

    if (!NameAmbiguousAcrossBaseInterfaces)
    {
        NameAmbiguousAcrossBaseInterfaces = & defaultAmbiguityField;
    }

    ThrowIfNull(NameAmbiguousAcrossBaseInterfaces);

    //When doing lookup inside of generic type parameters, we must first look only for instance
    //methods inside of each type constraint seperately, using the existing Whidbey name lookup rules,
    //which means that we have to supress looking for extension methods.
    //Then, in the event that no item is found, we can then look for extension methods, provided that
    //Flags does not contains "NameSearchIgnoreExtensionMethods.
    NameFlags InstanceMethodLookupFlags = Flags | NameSearchIgnoreExtensionMethods;


    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

    if (TypeParameter->IsBad())
    {
        return NULL;
    }

    // If a class constraint exists, look up in the class constraints
    //

    Declaration *Result = NULL;

    Type *ClassConstraint =
        GetClassConstraint(
            TypeParameter,
            m_CompilerHost,
            &m_TreeStorage,
            true,   // ReturnArraysAs"System.Array"
            true    // ReturnValuesAs"System.ValueType"or"System.Enum"
            );

    if (ClassConstraint)
    {
        Result =
            EnsureNamedRoot
            (
                LookupInClass
                (
                    ClassConstraint->PClass(),
                    Name,
                    AccessingInstanceType,
                    BindspaceMask,
                    NameIsBad,
                    ResultGenericBindingContext,
                    GenericTypeArity,
                    InstanceMethodLookupFlags
                )
            );

        UpdateNameLookupGenericBindingContext(ClassConstraint, ResultGenericBindingContext);
    }

    if (Result &&
        CanBeAccessible(
            Result,
            *ResultGenericBindingContext,
            AccessingInstanceType))
    {
        return Result;
    }

    // If no accessible member found in the class type, then lookup in the interface constraints.

    Declaration *InterfaceLookupResult = NULL;
    GenericBinding *InterfaceLookupResultGenericContext = NULL;

    BCITER_Constraints ConstraintsIter(TypeParameter, true, m_Compiler);


    while (GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (!Constraint->IsGenericTypeConstraint())
        {
            continue;
        }

        LookupInBaseInterface(
            Constraint->PGenericTypeConstraint()->GetType(),
            Name,
            NameNoFlags,
            BindspaceMask,
            SourceLocation,
            NameIsBad,
            InterfaceLookupResult,
            InterfaceLookupResultGenericContext,
            GenericTypeArity,
            NameAmbiguousAcrossBaseInterfaces);

        if (NameIsBad)
        {
            *ResultGenericBindingContext = InterfaceLookupResultGenericContext;
            return InterfaceLookupResult;
        }
    }

    if (InterfaceLookupResult)
    {
        *ResultGenericBindingContext = InterfaceLookupResultGenericContext;
        return InterfaceLookupResult;
    }

    if (!ClassConstraint)
    {
        // Finally, look in System.Object or System.ValueType

        if (TypeParameter->HasValueConstraint() && GetFXSymbolProvider()->IsTypeAvailable(FX::ValueTypeType))
        {
            Result =
                EnsureNamedRoot
                (
                    LookupInClass
                    (
                        GetFXSymbolProvider()->GetType(FX::ValueTypeType)->PClass(),
                        Name,
                        AccessingInstanceType,
                        BindspaceMask,
                        NameIsBad,
                        ResultGenericBindingContext,
                        GenericTypeArity,
                        InstanceMethodLookupFlags
                    )
                );

        }
        else
        {
            Result =
                EnsureNamedRoot
                (
                    LookupInClass
                    (
                        GetFXSymbolProvider()->GetObjectType()->PClass(),
                        Name,
                        AccessingInstanceType,
                        BindspaceMask,
                        NameIsBad,
                        ResultGenericBindingContext,
                        GenericTypeArity,
                        InstanceMethodLookupFlags
                    )
                );
        }
    }

    return
            LookForExtensionMethods
            (
                Name,
                Result,
                AccessingInstanceType,
                GenericBindingContext,
                Flags
            );

}

bool
NamespaceIsAvailable
(
    CompilerProject *LookupProject,
    Namespace *NamespaceToTest
)
{
    CompilerProject *NamespaceProject = NamespaceToTest->GetCompilerFile() ? NamespaceToTest->GetCompilerFile()->GetProject() : NULL;

    return
        LookupProject == NamespaceProject ||
        LookupProject == NULL ||
        NamespaceProject == NULL ||
        LookupProject->IsProjectReferenced(NamespaceProject);
}

bool
DeclarationIsAvailable
(
    CompilerProject *LookupProject,
    Declaration *DeclarationToTest
)
{
    CompilerProject *DeclarationProject = DeclarationToTest->GetContainingProject();

    return
        LookupProject == DeclarationProject ||
            DeclarationProject == NULL ||
            LookupProject == NULL ||
            LookupProject->IsProjectReferenced(DeclarationProject, true /* Allow different projects with the same identity to match */);

}

#if HOSTED
Declaration *
Semantics::LookupInNamespace
(
    Namespace *Lookup,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    _Inout_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Out_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    InitializeInterpretationState(
        NULL,       // No expression
        Lookup->GetHash(),
        NULL,       // No cycle detection
        NULL,       // No call graph
        NULL,       // Not interested in transient symbols
        true,       // preserve extra semantic information
        false,      // don't perform obsolete checks
        false,      // cannot interpret statements
        false,      // cannot interpret expressions
        true);      // can merge anonymous type templates
   
    Location location;

    return LookupInNamespace(
        Lookup,
        Name,
        Flags,
        BindspaceMask,
        true,
        false,
        location,
        NameIsBad,
        GenericBindingContext,
        GenericTypeArity,
        NameAmbiguousAcrossBaseInterfaces,
        pImportTrackerEntry);
}
#endif

Declaration *
Semantics::LookupInNamespace
(
    Namespace *Lookup,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    bool IgnoreImports,
    bool IgnoreModules,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Out_opt_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    LookupNode *pLookupResult = NULL;
    LookupKey key;

    // This transforms the search lookup from the unnamed namespace of the main type's sourcefile
    // to the unnamed namespace of the partial type's sourcefile. This is needed so that the lookup
    // in file level imports occurs correctly and also so that the correct declaration caches
    // are used.
    //
    // 





    if (!IgnoreImports)
    {
        Namespace *UnnamedNamespaceForNameLookupInImports =
            !m_UnnamedNamespaceForNameLookupInImports && m_SourceFile ?
                m_SourceFile->GetUnnamedNamespace() :
                m_UnnamedNamespaceForNameLookupInImports;

        if (Lookup->IsSameNamespace(UnnamedNamespaceForNameLookupInImports))
        {
            Lookup = UnnamedNamespaceForNameLookupInImports;
        }
    }

    bool UseDeclarationCache =
#if HOSTED
        !(Lookup->GetNamespaceRing()->GetContainsHostedDynamicHash()) &&
#endif
        m_PermitDeclarationCaching &&
        m_LookupCache &&
        !pImportTrackerEntry &&  // don't use the cache if we're tracking imports: it will cause us to miss some locations!
        !(HasFlag(Flags, NameSearchCanonicalInteropType) && m_EmbeddedTypeIdentity != NULL); // don't use cache when we are looking for specific identity

    if (UseDeclarationCache)
    {
        AssertIfNull(m_Project);

        key.InitKey(
            Name,
            Lookup,
            Flags,
            BindspaceMask,
            GenericTypeArity,
            IgnoreImports,
            IgnoreModules, 
            m_Project);

        if (m_LookupCache->Find(&key, &pLookupResult))
        {
            if (pLookupResult->Result)
            {
                LogDependency(pLookupResult->Result);
            }

            if (GenericBindingContext)
            {
                *GenericBindingContext = pLookupResult->GenericBindingContext;
            }

            return pLookupResult->Result;
        }
    }

    bool TempNameAmbiguousAcrossBaseInterfaces = false;
    GenericBinding *TempGenericBindingContext = NULL;

    if (GenericBindingContext == NULL)
    {
        GenericBindingContext = &TempGenericBindingContext;
    }

    InitializeNameLookupGenericBindingContext(GenericBindingContext);

    // Microsoft note: (see bug 6775: Perf - Improve Namespace hashing)
    //
    // LookupInNamespaceNoCaching_WithMergedHash and LookupInNamespaceNoCaching
    // are made to be behaviorial equivalent.
    // LookupInNamespaceNoCaching_WithMergedHash uses merged hash table for
    // name lookup and default backs to the old behavior (i.e., LookupInNamespaceNoCaching)
    // if memory allocation ever fails for constructing such a merged hash.
    //
    // The perf bottleneck identified here is the hit/miss ratio of name lookup about 3%
    // (under Maui2 project build scenario). In other words, majority of the time
    // spent on LookupInNamespaceNoCaching was a miss (Result returned NULL). Using
    // the merged hash approach as in LookupInNamespaceNoCaching_WithMergedHash shortens
    // the time it take to produce NULL Result in the order of O(1) instead of O(n), where n
    // is the number of BCSYM_Namespace's under one BCSYM_NamespaceRing.
    //

    //Declaration *Result = LookupInNamespaceNoCaching(
    Declaration *Result = LookupInNamespaceNoCaching_WithMergedHash(
        Lookup,
        Name,
        Flags,
        BindspaceMask,
        IgnoreImports,
        IgnoreModules,
        SourceLocation,
        NameIsBad,
        GenericBindingContext,
        GenericTypeArity,
        &TempNameAmbiguousAcrossBaseInterfaces,
        pImportTrackerEntry);

    if (NameAmbiguousAcrossBaseInterfaces)
    {
        *NameAmbiguousAcrossBaseInterfaces = TempNameAmbiguousAcrossBaseInterfaces;
    }

    if (UseDeclarationCache &&
        !NameIsBad &&
        !TempNameAmbiguousAcrossBaseInterfaces)
    {
        m_LookupCache->Insert(&key, &pLookupResult);

        pLookupResult->Result = Result;
        pLookupResult->GenericBindingContext = *GenericBindingContext;
    }

    return Result;
}

#if 0

// Important Note!!!!!!
// This function is to be deprecated. It is kept around until the function
// 'Semantics::LookupInNamespaceNoCaching_WithMergedHash' function stablizes and shows good field results.
//
// If you need to modify this function, you will need to do it in the function
// 'Semantics::LookupInNamespaceNoCaching_WithMergedHash' instead.
// Please see Microsoft if you have further question.
//

Declaration *
Semantics::LookupInNamespaceNoCaching
(
    Namespace *Lookup,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    bool IgnoreImports,
    bool IgnoreModules,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    Declaration *Result = NULL;

    // If the namespace comes from metadata, it is necessary to import
    // the members of all modules in the namespace, in order to apply
    // the short-circuiting logic that depends on knowing what names have
    // declarations in modules.

    Lookup->EnsureMemberNamesInAllModulesLoaded();

    if (StringPool::CanBeDeclaredInNamespace(Name, Lookup))
    {
        // Lookup the name in all instances of the namespace. The namespaces are arranged
        // in a ring, so loop through them until arriving back at the original namespace.

        Namespace *CurrentLookup = Lookup;
        bool IsPotentiallyAmbiguous = false;

        // We want to prefer types in the current assembly. We look through all the rings on the namespace.
        // If they are ambiguous and exactly one of them is the current compiling project, mark this and continue.
        // Once we get through all the rings:
        // 1. if we see exactly one match in the current compiling project, this is the one to keep.
        // 2. otherwise, we are ambiguous.
        // Note: If the current project (m_Project) is NULL, then things will fall back to the old behavior.

        bool AmbiguousUnlessBoundToCurrentProject = false;
        Declaration *CurrentProjectResult = NULL;

        bool IsAmbiguous = false;

        do
        {
            bool IsNamespaceIsAvailableToCurrentProject = false;

            for(Declaration *CurrentResult =
                    ImmediateLookup(
                        ViewAsScope(CurrentLookup),
                        Name,
                        BindspaceMask);
                CurrentResult;
                CurrentResult = CurrentResult->GetNextOfSameName(BindspaceMask))
            {
                // Ignore this result if this namespace is not part of the current
                // project or a project referenced by the current project.
                // (Test this here rather than before doing the lookup because
                // the name lookup is likely faster than the referenced test.)

                if (!IsNamespaceIsAvailableToCurrentProject)
                {
                    if (!NamespaceIsAvailableToCurrentProject(CurrentLookup))
                    {
                        break;
                    }

                    IsNamespaceIsAvailableToCurrentProject = true;
                }

                // An ambiguity exists if both the current and former result
                // are accessible, and the current and former result are not
                // both instances of the same namespace.


                if (Result == NULL)
                {
                    Result = CurrentResult;

                    if( Result->GetContainingProject() == m_Project )
                    {
                        CurrentProjectResult = Result;
                    }
                }
                else if (!IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
                {
                    Result = CurrentResult;

                    if( Result->GetContainingProject() == m_Project )
                    {
                        CurrentProjectResult = Result;
                    }
                }
                else if (IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
                {
                    if (!IsAccessible(Result, NULL, NULL))
                    {
                        Result = CurrentResult;

                        if( Result->GetContainingProject() == m_Project )
                        {
                            CurrentProjectResult = Result;
                        }
                    }
                    else if (IsAccessible(CurrentResult, NULL, NULL) &&
                            !(Result->IsNamespace() && CurrentResult->IsNamespace()))
                    {
                        // The name is ambiguous in the current namespace, which can
                        // be named or unnamed.

                        // 


                        // Note that a type forwarder and a normal type are not ambiguous. We do this
                        // so that a type forward can be ignored when its destination type is found
                        // and more importantly since a type forwarder does not have the access modifier
                        // we don't potentially internal type forwarders to cause ambiguity for valid
                        // normal types.
                        //
                        if (Result->IsTypeForwarder() != CurrentResult->IsTypeForwarder())
                        {
                            if (Result->IsTypeForwarder())
                            {
                                Result = CurrentResult;

                                if( Result->GetContainingProject() == m_Compiler->GetProjectBeingCompiled() )
                                {
                                    CurrentProjectResult = Result;
                                }
                            }
                        }
                        else
                        {
                            if (Result->IsTypeForwarder())
                            {
                                // Potential ambiguity. But might not be ambiguous if a non-type forward is
                                // found later.
                                IsPotentiallyAmbiguous = true;
                            }
                            else
                            {
                                // Prefer the type in the current assembly.

                                if( m_Project == NULL )
                                {
                                    IsAmbiguous = true;
                                    goto AmbiguousMatch;
                                }
                                else if( CurrentResult->GetContainingProject() == m_Project )
                                {
                                    // This new result is in our project; was there one that we bound to that
                                    // was also in this project?

                                    if( CurrentProjectResult == NULL )
                                    {
                                        Result = CurrentResult;
                                        CurrentProjectResult = CurrentResult;
                                    }
                                    else
                                    {
                                        IsAmbiguous = true;
                                        goto AmbiguousMatch;
                                    }
                                }
                                else
                                {
                                    // This type is not in the current project. We are potentially
                                    // ambiguous unless we bind to exactly one type in the current project.

                                    if( CurrentProjectResult == NULL )
                                    {
                                        AmbiguousUnlessBoundToCurrentProject = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            CurrentLookup = CurrentLookup->GetNextNamespace();

        } while (CurrentLookup != Lookup);

AmbiguousMatch:

        if( AmbiguousUnlessBoundToCurrentProject && CurrentProjectResult == NULL )
        {
            IsAmbiguous = true;
        }

        if (IsPotentiallyAmbiguous && Result->IsTypeForwarder())
        {
            IsAmbiguous = true;
        }

        // At this point, when checking for accessibility, we would have marked
        // the current project as "rejected binding because we haven't ----ed
        // attributes yet." If we do end up binding correctly to another symbol,
        // then just clear the project. If we do not bind correctly, then commit
        // the missed binding data, so that later on we will report an error.

        if( IsAmbiguous || Result == NULL || !IsAccessible( Result, NULL, NULL ))
        {
            m_Lookup->GetContainer()->GetContainingProject()->CommitMissedBindingOpportunity();
        }
        else
        {
            m_Lookup->GetContainer()->GetContainingProject()->ClearMissedBindingOpportunity();
        }

        if( IsAmbiguous )
        {
            if (StringPool::StringLength(Lookup->GetName()) > 0)
            {
                ReportSemanticError(
                    ERRID_AmbiguousInNamespace2,
                    SourceLocation,
                    Name,
                    Lookup);
            }
            else
            {
                ReportSemanticError(
                    ERRID_AmbiguousInUnnamedNamespace1,
                    SourceLocation,
                    Name);
            }

            NameIsBad = true;
            return Result;
        }

        // If the arity of the resulting type does not match the requested for type arity,
        // then return NULL indicating no match.
        //
        if (Result &&
            !IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
        {
            Result = NULL;
        }

        if (Result)
        {
            return Result;
        }
    }

    if (Result == NULL && !NameIsBad)
    {
        if (!IgnoreModules)
        {
            Result =
                LookupInModulesInNamespace(
                    Lookup,
                    Name,
                    BindspaceMask,
                    SourceLocation,
                    NameIsBad,
                    GenericTypeArity);
        }


        if (Result == NULL && !NameIsBad && !IgnoreImports)
        {
            // Lookup in the File-level imports.

            GenericBinding *TempGenericBindingContext;

            GenericBinding **ResultGenericBindingContext =
                GenericBindingContext?
                    GenericBindingContext:
                    &TempGenericBindingContext;

            Result =
                LookupInImports(
                    Lookup->GetImports(),
                    Name,
                    Flags,
                    BindspaceMask,
                    SourceLocation,
                    NameIsBad,
                    ResultGenericBindingContext,
                    GenericTypeArity,
                    NameAmbiguousAcrossBaseInterfaces,
                    pImportTrackerEntry);
        }
    }

    return Result;
}

#endif

// Lookup in namespace and utilitize hash across namespace rings.

Declaration *
Semantics::LookupInNamespaceNoCaching_WithMergedHash
(
    Namespace *Lookup,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    bool IgnoreImports,
    bool IgnoreModules,
    const Location &SourceLocation,
    bool &NameIsBad,
    GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    bool *NameAmbiguousAcrossBaseInterfaces,
    ImportTrackerEntry *pImportTrackerEntry
)
{
    Declaration *Result = NULL;
    Type * embeddedTypeIdentity = HasFlag(Flags, NameSearchCanonicalInteropType) ? m_EmbeddedTypeIdentity : NULL;

    AssertIfFalse(embeddedTypeIdentity == NULL || TypeHelpers::IsEmbeddedLocalType(embeddedTypeIdentity));
        
    // If the namespace comes from metadata, it is necessary to import
    // the members of all modules in the namespace, in order to apply
    // the short-circuiting logic that depends on knowing what names have
    // declarations in modules.

    Lookup->EnsureMemberNamesInAllModulesLoaded();

#if HOSTED
    if (Lookup->GetNamespaceRing()->GetContainsHostedDynamicHash() ||
        StringPool::CanBeDeclaredInNamespace(Name, Lookup))
#else
    if (StringPool::CanBeDeclaredInNamespace(Name, Lookup))
#endif
    {
        // We'll try to use merged hash table from NamespaceRing whenever possible.
        // The use of merged hash table at the NamespaceRing will improve the speed of name lookup
        // because traversal of linked namespaces is no longer necessary.
        //
        // bUseMergedHash indicates whether the following code-path should use the merged hash.
        // A false value of bUseMergedHash indicates that there was perhaps a memory allocation, though rare,
        // failure, and that we should default to the old code-path of iterating through the namespaces for
        // name binding.

        Scope *MergedHash = GetMergedHashTable(Lookup->GetNamespaceRing());

        // Lookup the name in all instances of the namespace. The namespaces are arranged
        // in a ring, so loop through them until arriving back at the original namespace.

        Namespace *CurrentLookup = Lookup;
        bool IsPotentiallyAmbiguous = false;

        // We want to prefer types in the current assembly. We look through all the rings on the namespace.
        // If they are ambiguous and exactly one of them is the current compiling project, mark this and continue.
        // Once we get through all the rings:
        // 1. if we see exactly one match in the current compiling project, this is the one to keep.
        // 2. otherwise, we are ambiguous.
        // Note: If the current project (m_Project) is NULL, then things will fall back to the old behavior.

        bool AmbiguousUnlessBoundToCurrentProject = false;
        Declaration *CurrentProjectResult = NULL;

        bool IsAmbiguous = false;

        do
        {
            bool IsNamespaceIsAvailableToCurrentProject = false;

            Declaration* CurrentResult = NULL;

            for((MergedHash) ?
                (CurrentResult =
                    ImmediateLookupForMergedHash(
                        MergedHash,
                        Name,
                        BindspaceMask, 
                        m_Project))
                        :
                (CurrentResult =
                    ImmediateLookup(
                        ViewAsScope(CurrentLookup),
                        Name,
                        BindspaceMask,
                        m_Project));
                CurrentResult;
                CurrentResult = CurrentResult->GetNextOfSameName(BindspaceMask))
            {
                Declaration* SavedCurrentResult = NULL;
                if (MergedHash)
                {
                    IsNamespaceIsAvailableToCurrentProject = false;
                    SavedCurrentResult = CurrentResult;

                    VSASSERT( CurrentResult->IsAlias(), "The merged hash table contains symbol of BCSYM_Alias only.");

                    CurrentResult = CurrentResult->PAlias()->GetAliasedSymbol()->PNamedRoot();
                }
				
                // #546428 Skip any symbols that can't be our canonical interop type, given that we know the target identity.
                if ( embeddedTypeIdentity != NULL )
                {
                    if (embeddedTypeIdentity != CurrentResult &&
                        !TypeHelpers::AreTypeIdentitiesEquivalent(embeddedTypeIdentity, CurrentResult))
                    {
                        goto GetNextOfSameName;
                    }
                    else if (TypeHelpers::IsEmbeddedLocalType(
                        CurrentResult
#if IDE 
                        , true // fIgnoreTemporaryEmbeddedStatus
#endif                            
                        ))
                    {
                        goto GetNextOfSameName;
                    }
                }

                // Ignore this result if this namespace is not part of the current
                // project or a project referenced by the current project.
                // (Test this here rather than before doing the lookup because
                // the name lookup is likely faster than the referenced test.)

                if (!IsNamespaceIsAvailableToCurrentProject)
                {
                    VSASSERT( ChaseThroughAlias(CurrentResult)->GetParent()->IsNamespace(), "" );
                    VSASSERT( NULL != ChaseThroughAlias(CurrentResult)->GetParent()->PNamespace(), "" );

                    // if (!NamespaceIsAvailableToCurrentProject(CurrentLookup))
                    if (!NamespaceIsAvailableToCurrentProject(CurrentResult->GetParent()->PNamespace()))
                    {
                        if (MergedHash)
                        {
                            goto GetNextOfSameName;
                        }
                        else
                        // (!bUseMergedHash)
                        {
                            break; // exit for() loop.
                        }
                    }

                    IsNamespaceIsAvailableToCurrentProject = true;
                }

                #if IDE 

                //If control reaches this point, then the namespace containing CurrentResult is available to the current project
                //This means that we are either dealing with a type in our project and that we have already reach bound state or are in the middle of transition from
                //declared state to bound state, or we are dealing with a type in a project that has alread reached bound state.
                //This is because: 
                //      1. all of our dependencies must be bound before we can start the transition to bound state
                //      2. this function is part of name lookup
                //      2. we should only be running name lookup during Declared => Bound, or post bound state
                VSASSERT
                (
                  CurrentResult->GetCompilerFile()->GetProject()->GetCompState() >= CS_Bound || 
                  CurrentResult->GetCompilerFile()->GetProject()->IsDeclaredToBoundCompilationInProgress(),
                  "The project containing the current result should either be in bound state, or we should be in the process of brining that project up to bound"
                );


                //It is possible for a type to end up in the merged hash table before it's partial types have been resolved.
                //This can happen when
                //    1. There are two projects that have types in the same namespace
                //    2. One project is brought from declared to bound before the other one.
                //The merged namespace hash table will then contain unresolved partials for the second project.
                //Later, when we come to bind the project containing the partials we need to deal with this.
                //Because of the assert above we know that it is safe (from a decompilation perspective) to
                //go ahead and resolve partial types for CurrentResult now.

                //Only classes and structures can be partial types.
                if (CurrentResult->IsClass() || CurrentResult->IsStruct())
                {
                    BCSYM_Container * CurrentContainer = CurrentResult->PContainer();

                    Bindable::ResolvePartialTypesForContainer(CurrentContainer, m_CompilationCaches);                                   

                    if (CurrentContainer->IsPartialType())
                    {
                        CurrentResult = CurrentContainer->GetMainType(); 

                        if (Result && BCSYM::AreTypesEqual(Result,CurrentResult))
                        {
                            //We have a "duplicate" result due to partials ending up in the merged hash table.
                            //this isn't a real duplicate, so act like only one type exists....

                            Result = NULL;
                        }                    
                    }
                }

                #endif


                // An ambiguity exists if both the current and former result
                // are accessible, and the current and former result are not
                // both instances of the same namespace.


                if (Result == NULL)
                {
                    Result = CurrentResult;

                    if( Result->GetContainingProject() == m_Project )
                    {
                        CurrentProjectResult = Result;
                    }
                }
                else if (!IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
                {
                    Result = CurrentResult;

                    if( Result->GetContainingProject() == m_Project )
                    {
                        CurrentProjectResult = Result;
                    }
                }
                else if (IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
                {
                    if (!IsAccessible(Result, NULL, NULL))
                    {
                        Result = CurrentResult;

                        if( Result->GetContainingProject() == m_Project )
                        {
                            CurrentProjectResult = Result;
                        }
                    }
                    else if (IsAccessible(CurrentResult, NULL, NULL) &&
                            !(Result->IsNamespace() && CurrentResult->IsNamespace()))
                    {
                        // The name is ambiguous in the current namespace, which can
                        // be named or unnamed.

                        // 


                        // Note that a type forwarder and a normal type are not ambiguous. We do this
                        // so that a type forward can be ignored when its destination type is found
                        // and more importantly since a type forwarder does not have the access modifier
                        // we don't potentially internal type forwarders to cause ambiguity for valid
                        // normal types.
                        //
                        if (Result->IsTypeForwarder() != CurrentResult->IsTypeForwarder())
                        {
                            if (Result->IsTypeForwarder())
                            {
                                Result = CurrentResult;

                                if( Result->GetContainingProject() == m_Compiler->GetProjectBeingCompiled() )
                                {
                                    CurrentProjectResult = Result;
                                }
                            }
                        }
                        else if (Result->IsTypeForwarder())
                        {
                            // Potential ambiguity. But might not be ambiguous if a non-type forward is
                            // found later.
                            IsPotentiallyAmbiguous = true;
                        }
                        else if (TypeHelpers::AreTypeIdentitiesEquivalent(Result, CurrentResult))
                        {
                            // A local copy of a PIA type is a possibly-incomplete local representation of a
                            // COM import type linked in via the No-PIA feature. We might encounter one of these
                            // if we reference an assembly which was built using No-PIA (the /link mode).
                            // There is no ambiguity if one of the types is a local copy of a PIA type,
                            // because we will just use the non-copy type instead. If both of the types are
                            // local copies, that's no big deal either; we'll arbitrarily pick one, and trust
                            // that the real type will show up later. We'll check later to make sure that our
                            // result is the original type and not one of these local copies.
                            if (TypeHelpers::IsEmbeddedLocalType(
                                Result
#if IDE 
                                , true // fIgnoreTemporaryEmbeddedStatus
#endif                            
                                ))
                            {
                                Result = CurrentResult;
                            }
                        }
                        else
                        {   
                            //note: Microsoft
                            //         This code is probably no longer needed
                            //         we should remove this after we ship orcas.
                            
                            // Bug 109902 - DevDiv Bugs: Do not treat as an ambiguouty the case when both symbols represent the same partial type 
                            // Bug 116920 - DevDiv Bugs: The same as above
                            BCSYM_Container * ResultAsContainer = Result->IsContainer() ? Result->PContainer() : NULL;
                            BCSYM_Container * CurrentResultAsContainer = CurrentResult->IsContainer() ? CurrentResult->PContainer() : NULL;
                            BCSYM_Container * ResultMainType = NULL;
                            BCSYM_Container * CurrentResultMainType = NULL;

                            if(ResultAsContainer && CurrentResultAsContainer)
                            {
                                ResultMainType = ResultAsContainer->GetMainType();

                                if(!ResultMainType)
                                {
                                    ResultMainType = ResultAsContainer;
                                }

                                CurrentResultMainType = CurrentResultAsContainer->GetMainType();

                                if(!CurrentResultMainType)
                                {
                                    CurrentResultMainType = CurrentResultAsContainer;
                                }

                                AssertIfFalse(ResultMainType && CurrentResultMainType);
                            }

                            if(!ResultMainType || (ResultMainType != CurrentResultMainType))
                            {
                                // Prefer the type in the current assembly.

                                if( m_Project == NULL )
                                {
                                    IsAmbiguous = true;
                                    goto AmbiguousMatch;
                                }
                                else if( CurrentResult->GetContainingProject() == m_Project )
                                {
                                    // This new result is in our project; was there one that we bound to that
                                    // was also in this project?

                                    if( CurrentProjectResult == NULL )
                                    {
                                        Result = CurrentResult;
                                        CurrentProjectResult = CurrentResult;
                                    }
                                    else
                                    {
                                        IsAmbiguous = true;
                                        goto AmbiguousMatch;
                                    }
                                }
                                else
                                {
                                    // This type is not in the current project. We are potentially
                                    // ambiguous unless we bind to exactly one type in the current project.

                                    if( CurrentProjectResult == NULL )
                                    {
                                        AmbiguousUnlessBoundToCurrentProject = true;
                                    }
                                }
                            }
                        }
                    }
                }

GetNextOfSameName:
                if (MergedHash)
                {
                    CurrentResult = SavedCurrentResult;
                }
            }

            if (MergedHash)
            {
                CurrentLookup = Lookup; // force an loop-exiting condition on "while (CurrentLookup != Lookup)"
            }
            else
            // (!bUseMergedHash)
            {
                CurrentLookup = CurrentLookup->GetNextNamespace();
            }

        } while (CurrentLookup != Lookup);

AmbiguousMatch:

        if( AmbiguousUnlessBoundToCurrentProject && CurrentProjectResult == NULL )
        {
            IsAmbiguous = true;
        }

        if (IsPotentiallyAmbiguous && Result->IsTypeForwarder())
        {
            IsAmbiguous = true;
        }

        // At this point, when checking for accessibility, we would have marked
        // the current project as "rejected binding because we haven't ----ed
        // attributes yet." If we do end up binding correctly to another symbol,
        // then just clear the project. If we do not bind correctly, then commit
        // the missed binding data, so that later on we will report an error.

        if( IsAmbiguous || Result == NULL || !IsAccessible( Result, NULL, NULL ))
        {
            m_Lookup->GetContainer()->GetContainingProject()->CommitMissedBindingOpportunity();
        }
        else
        {
            m_Lookup->GetContainer()->GetContainingProject()->ClearMissedBindingOpportunity();
        }

        if( IsAmbiguous )
        {
            if (StringPool::StringLength(Lookup->GetName()) > 0)
            {
                ReportSemanticError(
                    ERRID_AmbiguousInNamespace2,
                    SourceLocation,
                    Name,
                    Lookup);
            }
            else
            {
                ReportSemanticError(
                    ERRID_AmbiguousInUnnamedNamespace1,
                    SourceLocation,
                    Name);
            }

            NameIsBad = true;
            return Result;
        }

        if (TypeHelpers::IsEmbeddedLocalType(
            Result
#if IDE 
            , true // fIgnoreTemporaryEmbeddedStatus
#endif                            
            ))
        { 
            // You cannot link against an assembly which refers to a PIA in No-PIA mode unless you also include a 
            // reference to the PIA. If our lookup returned a copy of a PIA-type, then we must not have linked in 
            // the library containing the original type. There is a comprehensive check for this error condition
            // in MetaEmit, but by reporting the error here we can provide a more helpful source location.
            NameIsBad = true;
            ReportSemanticError(
                ERRID_AbsentReferenceToPIA1,
                SourceLocation,
                Result);
        }

        // If the arity of the resulting type does not match the requested for type arity,
        // then return NULL indicating no match.
        //
        if (Result &&
            !IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
        {
            Result = NULL;
        }

        // Microsoft:
        // If we bind to a result here that is inaccessible, try to look in imports, etc
        // for a better fit.

        if (Result && IsAccessible( Result, NULL, NULL ))
        {
            return Result;
        }
    }

    if ((Result == NULL || !IsAccessible( Result, NULL, NULL )) && !NameIsBad)
    {
        Declaration * SecondaryResult = NULL;

        if (!IgnoreModules)
        {
            SecondaryResult =
                LookupInModulesInNamespace(
                    Lookup,
                    Name,
                    BindspaceMask,
                    SourceLocation,
                    NameIsBad,
                    GenericTypeArity);
        }

        if (SecondaryResult == NULL && !NameIsBad && !IgnoreImports)
        {
            // Lookup in the File-level imports.

            GenericBinding *TempGenericBindingContext;

            GenericBinding **ResultGenericBindingContext =
                GenericBindingContext?
                    GenericBindingContext:
                    &TempGenericBindingContext;

            SecondaryResult =
                LookupInImports(
                    Lookup->GetImports(),
                    Name,
                    Flags,
                    BindspaceMask,
                    SourceLocation,
                    NameIsBad,
                    ResultGenericBindingContext,
                    GenericTypeArity,
                    NameAmbiguousAcrossBaseInterfaces,
                    pImportTrackerEntry);
        }

        // Microsoft:
        // Ok, if SecondaryResult is better, use it.

        if (Result == NULL)
        {
            Result = SecondaryResult;
        }
        else if ((!IsAccessible( Result, NULL, NULL )) &&
            SecondaryResult != NULL &&
            IsAccessible( SecondaryResult, NULL, NULL ))
        {
            Result = SecondaryResult;
        }
    }

    return Result;
}

bool
Semantics::IsMemberSuitableBasedOnGenericTypeArity
(
    Declaration *Member,
    int GenericTypeArity
)
{
    return
        GenericTypeArity == -1 ||
        (!Member->IsType() && !Member->IsNamespace()) ||    // !Member->IsNamespace() - Bug VSWhidbey 424148
        Member->GetGenericParamCount() == GenericTypeArity;
}

// returns false if ambiguous, else returns true and changes the Current appropriately.
//
bool
Semantics::ResolveGenericTypesByArity
(
    int GenericTypeArity,
    Declaration *Challenger,
    GenericBinding *ChallengerGenericBindingContext,
    _Inout_ Declaration *&Current,
    _Out_opt_ GenericBinding **CurrentGenericBindingContext
)
{
    // Return false if definitely ambiguous else true

    if (GenericTypeArity == -1 ||
        (!IsType(Challenger) || !IsType(Current)))
    {
        return false;
    }

    if (Challenger->GetGenericParamCount() == GenericTypeArity)
    {
        if (Current->GetGenericParamCount() == GenericTypeArity)
        {
            return false;
        }
        else
        {
            // Change current to be the one with the asked for arity
            //
            Current = Challenger;

            if (CurrentGenericBindingContext)
            {
                *CurrentGenericBindingContext = ChallengerGenericBindingContext;
            }

            return true;
        }
    }

    return true;
}


Declaration *
Semantics::LookupInModulesInNamespace
(
    Namespace *Lookup,
    _In_z_ Identifier *Name,
    unsigned BindspaceMask,
    const Location &SourceLocation,
    _Out_ bool &NameIsBad,
    int GenericTypeArity
)
{
    // Search through all modules in the namespace--declarations in modules
    // are effectively exported to their containing namespaces.
    //
    // In the interest of speeding up name lookup, skip this step
    // for names that are not declared in any modules.

    Declaration *Result = NULL;

    if (StringPool::DeclaredInModule(Name) && DoesRingContainModules(Lookup->GetNamespaceRing()))
    {
        Namespace *CurrentLookup = Lookup;

        // See the note above in LookupInNamespaceNoCaching for the algorithm on
        // determining how to bind to the current project.

        bool AmbiguousInModule = false;
        bool AmbiguousUnlessBoundToCurrentProject = false;
        Declaration *CurrentProjectResult = NULL;
        Declaration *AmbiguousResult = NULL;

        do
        {
            // Bug 437737. With the new compilation order for diamond references,
            // modules in metadata files that are not referenced by this project
            // either directly or indirectly cannot be loaded when binding this
            // project.
            //
            if (!(CurrentLookup->GetCompilerFile() &&
                  CurrentLookup->GetCompilerFile()->IsMetaDataFile() &&
                  CurrentLookup->GetCompilerFile()->GetCompState() < CS_Bound))
            {

                for (ClassOrRecordType *Module = CurrentLookup->GetFirstModule();
                    Module;
                    Module = Module->GetNextModule())
                {
                    VSASSERT(Module->IsStdModule(), "Expected standard module isn't.");

                    // 




                    if (IsAccessible(Module, NULL, NULL))
                    {
                        bool IsNamespaceIsAvailableToCurrentProject = false;

                        for(Declaration *CurrentResult =
                                ImmediateLookup(
                                    ViewAsScope(Module),
                                    Name,
                                    BindspaceMask,
                                    m_Project);
                            CurrentResult;
                            CurrentResult =
                                CurrentResult->IsType() ?
                                    CurrentResult->GetNextOfSameName(BindspaceMask) :
                                    NULL)
                        {
                            // Ignore this result if this namespace is not part of the current
                            // project or a project referenced by the current project.
                            // (Test this here rather than before doing the lookup because
                            // the name lookup is likely faster than the referenced test.)

                            if (!IsNamespaceIsAvailableToCurrentProject)
                            {
                                if (!NamespaceIsAvailableToCurrentProject(CurrentLookup))
                                {
                                    break;
                                }

                                IsNamespaceIsAvailableToCurrentProject = true;
                            }

                            // VB allows only access to types, shared members and constants in
                            // modules.
                            //
                            if (CurrentResult->IsMember() &&
                                !CurrentResult->PMember()->IsShared() &&
                                !IsConstant(CurrentResult))
                            {
                                continue;
                            }

                            // Returning inaccessible members fro here will prevent looking up in
                            // imports.
                            //
                            // 



                            if (!CanBeAccessible(CurrentResult, NULL /* Modules cannot be generic */, ContainingClass()))
                            {
                                continue;
                            }

                            // An ambiguity exists if both the current and former result
                            // are accessible, and the current and former result are not
                            // both instances of the same namespace.

                            if (Result == NULL)
                            {
                                Result = CurrentResult;

                                if( Result->GetContainingProject() == m_Project )
                                {
                                    CurrentProjectResult = Result;
                                }
                            }
                            else if (!IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
                            {
                                Result = CurrentResult;

                                if( Result->GetContainingProject() == m_Project )
                                {
                                    CurrentProjectResult = Result;
                                }
                            }
                            else if (IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
                            {
                                // Returning inaccessible members fro here will prevent looking up in
                                // imports.
                                //
                                // 



                                //if (!CanBeAccessible(Result, NULL /* Modules cannot be generic */, ContainingClass()))
                                //{
                                //    Result = CurrentResult;
                                //}
                                //else if (CanBeAccessible(CurrentResult, NULL /* Modules cannot be generic */, ContainingClass()))
                                if( m_Project == NULL )
                                {
                                    // The name is ambiguous in the current namespace, which can
                                    // be named or unnamed.

                                    AmbiguousResult = CurrentResult;
                                    AmbiguousInModule = true;
                                    break;
                                }
                                else if( CurrentResult->GetContainingProject() == m_Project )
                                {
                                    // This new result is in our project; was there one that we bound to that
                                    // was also in this project?

                                    if( CurrentProjectResult == NULL )
                                    {
                                        Result = CurrentResult;
                                        CurrentProjectResult = CurrentResult;
                                    }
                                    else
                                    {
                                        AmbiguousResult = CurrentResult;
                                        AmbiguousInModule = true;
                                        break;
                                    }
                                }
                                else
                                {
                                    // This type is not in the current project. We are potentially
                                    // ambiguous unless we bind to exactly one type in the current project.

                                    if( CurrentProjectResult == NULL )
                                    {
                                        AmbiguousResult = CurrentResult;
                                        AmbiguousUnlessBoundToCurrentProject = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            CurrentLookup = CurrentLookup->GetNextNamespace();

        } while (CurrentLookup != Lookup);

        if( AmbiguousUnlessBoundToCurrentProject && CurrentProjectResult == NULL )
        {
            AmbiguousInModule = true;
        }

        if( AmbiguousInModule )
        {
            // 


            AssertIfNull( AmbiguousResult );

            ReportSemanticError(
                ERRID_AmbiguousInModules3,
                SourceLocation,
                Name,
                Result->GetContainer(),
                AmbiguousResult->GetContainer());

            NameIsBad = true;
            return Result;
        }

        // If the arity of the resulting type does not match the requested for type arity,
        // then return NULL indicating no match.
        //
        if (Result &&
            !IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
        {
            Result = NULL;
        }
    }

    return Result;
}

static void
AppendErrorText
(
    StringBuffer &Text,
    unsigned ErrorId
)
{
    HRESULT ErrorTextDescription = HrMakeRepl(ErrorId);
    HRINFO ErrorTextInfo;
    GetHrInfo(ErrorTextDescription, &ErrorTextInfo);
    Text.AppendString(ErrorTextInfo.m_bstrDescription);
    ReleaseHrInfo(&ErrorTextInfo);
}

bool
Semantics::AreSameNamespace
(
    Namespace *Left,
    Namespace *Right
)
{
    while (true)
    {
        if (!StringPool::IsEqual(Left->GetName(), Right->GetName()))
        {
            return false;
        }

        Container *LeftContainer = Left->GetContainer();
        Container *RightContainer = Right->GetContainer();

        if (LeftContainer == NULL)
        {
            return RightContainer == NULL;
        }

        if (RightContainer == NULL)
        {
            return false;
        }

        Left = Left->GetContainer()->PNamespace();
        Right = Right->GetContainer()->PNamespace();
    }
}

bool
Semantics::LookupInImportedTarget
(
    _In_ ImportedTarget  *Import,
    _In_z_ Identifier      *Name,
    NameFlags        Flags,
    unsigned         BindspaceMask,
    const Location  &SourceLocation,
    int              GenericTypeArity,
    _Inout_ bool            &NameIsBad,
    _Inout_ bool            &AmbiguityExists,
    StringBuffer    *AmbiguityErrorsText,
    _Out_ bool            &BoundToAlias,
    _Inout_ bool            *NameAmbiguousAcrossBaseInterfaces,
    _Inout_ GenericBinding **ResultGenericBindingContext,
    _Inout_ Declaration    **Result
)
{
    // An import that defines a named alias of the appropriate name takes
    // precedence over any implicitly imported names.
    if (Import->m_pstrAliasName)
    {
        // Skip Xml aliases, as they are bound to Xml namespaces that do not contain Clr types
        if (!Import->m_IsXml && StringPool::IsEqual(Name, Import->m_pstrAliasName))
        {
            if (HasFlag(Flags, NameSearchDonotResolveImportsAlias))
            {
                *Result = Import->m_pAlias;
            }
            else
            {
                *Result = Import->m_pTarget->PNamedRoot();

                *ResultGenericBindingContext =
                    Import->m_pTarget && Import->m_pTarget->IsGenericBinding() ?
                    Import->m_pTarget->PGenericBinding() :
                NULL;
            }

            BoundToAlias = true;
            return true;
        }

        return false;
    }

    // If any imports are bad, then the interpretation of the name
    // is questionable. Setting NameIsBad true here and returning NULL
    // is potentially more sound.
    // (Microsoft) Bad imports is a warning now. Imports of empty Fx namespaces can happen.
    // Keep this behaviour.

    if (Import->m_hasError)
    {
        return false;
    }

    Container * pImportTarget = Import->m_pTarget->PContainer();
    BackupAllocator backupAllocator(&m_SymbolCreator);

    if (pImportTarget->IsClass() && m_LookupCache)
    {
        // don't switch the allocator if we're still compiling to bound.  Otherwise,
        // we can end up allocating a GenericBindingContext on the lookup cache allocator
        // which is freed at the end of background compilation.  Allocated contexts during 
        // Bindable::ResolveNamedType can be attached to other symbols which will outlive
        // the lookup cache allocator.  See Dev11 362754
        if (!m_SourceFile || m_SourceFile->GetCompState() >= CS_Bound)
        {
            // Note that this code was originally added to fix http://bugcheck/bugs/DevDivBugs/33999
            // The code referred to in that bug doesn't seem to exist any more, but there are too 
            // many other scenarios which may have taken a dependency on this behavior to remove
            // this line of code completely, so instead we're making a tactical fix to only call it
            // when we've already compiled past CS_Bound.
            m_SymbolCreator.SetNorlsAllocator(m_LookupCache->GetNorlsAllocator());
        }
    }

    GenericBinding *CurrentGenericBindingContext = NULL;
    Symbol * tmp =
        LookupInScope(
        ViewAsScope(Import->m_pTarget->PContainer()),
        NULL,   // No Type parameter lookup
        Name,
        Flags,
        ContainingClass(),
        BindspaceMask,
        true,
        true,
        SourceLocation,
        NameIsBad,
        &CurrentGenericBindingContext,
        GenericTypeArity,   // Imports don't have imports
        NameAmbiguousAcrossBaseInterfaces);

    Assume(!tmp || tmp->IsNamedRoot(), L"A named root was expected!");

    Declaration *CurrentResult = tmp->PNamedRoot();

    UpdateNameLookupGenericBindingContext(Import->m_pTarget, &CurrentGenericBindingContext);

    backupAllocator.Restore();

    if (NameIsBad)
    {
        // no ambiguities are caused by this: if there was a bad type found before, it'll still be an error afterwards so we should
        // just leave it alone
        *Result = NULL;
        return false;
    }

    if (CurrentResult)
    {
        if (CurrentResult->IsNamespace())
        {
            // Dev10 #832844: Check if we can possibly find something accessible within this namespace
            BCSYM_Namespace * pCurrentNamespace = CurrentResult->PNamespace(); 
            BCSYM_Namespace * pNamespaceToCheck = pCurrentNamespace;
            bool containsAccessibleType = false;

            if(pCurrentNamespace->GetCompiler() && pCurrentNamespace->GetCompiler()->CanOverrideAccessibilityCheck() )
            {
                // Allow the debugger to override the default accessibility behavior.
                containsAccessibleType = pCurrentNamespace->GetCompiler()->NamespaceContainsAnAccessibleType(pCurrentNamespace);
            }
            else
            {
                do
                {
                    if (NamespaceIsAvailableToCurrentProject(pNamespaceToCheck) &&
                        (pNamespaceToCheck->ContainsPublicType() || 
                        (pNamespaceToCheck->ContainsFriendType() && 
                        (m_Project == pNamespaceToCheck->GetContainingProject() ||
                        pNamespaceToCheck->GetContainingProject()->CheckFriendship( m_Project )))))
                    {
                        containsAccessibleType = true;
                        break;
                    }

                    pNamespaceToCheck = pNamespaceToCheck->GetNextNamespace();
                }
                while (pNamespaceToCheck != pCurrentNamespace);
            }

            if (!containsAccessibleType)
            {
                return false; // Dev10 #832844: Ignore this namespace
            }
        }
    
        // An ambiguity exists if both the current and former result
        // are accessible, are different symbols (OK to reach the same
        // symbol by multiple paths), and the current and former result are not
        // both instances of the same namespace.

        if (*Result == NULL)
        {
            *Result = CurrentResult;
            *ResultGenericBindingContext = CurrentGenericBindingContext;
        }
        else if (!IsMemberSuitableBasedOnGenericTypeArity(*Result, GenericTypeArity))
        {
            *Result = CurrentResult;
            *ResultGenericBindingContext = CurrentGenericBindingContext;
        }
        else if (IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
        {
            if (*Result == CurrentResult &&
                TypeHelpers::EquivalentTypeBindings(
                    (*ResultGenericBindingContext)->PGenericTypeBinding(),
                    CurrentGenericBindingContext->PGenericTypeBinding()))
            {
            }
            else if (!CanBeAccessible(*Result, *ResultGenericBindingContext, NULL) ||
                    //VSW 409140.break ambiguities between types/type members and namespaces in favor of the former
                    ( (*Result)->IsNamespace() && (CurrentResult->IsType() || CurrentResult->IsMember()) &&
                    CanBeAccessible(CurrentResult, CurrentGenericBindingContext, NULL))) // Dev10 #822985: Namespace should be preferred over an inaccessible type or member.
            {
                *Result = CurrentResult;
                *ResultGenericBindingContext = CurrentGenericBindingContext;
            }
            else if (CanBeAccessible(CurrentResult, CurrentGenericBindingContext, NULL) &&
                     (!(*Result)->IsNamespace() ||
                        !CurrentResult->IsNamespace() ||
                        !AreSameNamespace((*Result)->PNamespace(), CurrentResult->PNamespace())) &&
                     //VSW 409140.break ambiguities between types/type members and namespaces in favor of the former - keep Result
                     !( CurrentResult->IsNamespace() && ((*Result)->IsType() || (*Result)->IsMember())))
            {
                // The name is ambiguous


                // 


                if (m_Errors && AmbiguityErrorsText)
                {
                    // If this is the first ambiguity, add the enclosing namespace
                    // of the former result to the message text.

                    if (!AmbiguityExists)
                    {
                        if (*ResultGenericBindingContext)
                        {
                            StringBuffer TextBuffer1;

                            AmbiguityErrorsText->AppendString(
                                m_Errors->ExtractErrorName(
                                    *ResultGenericBindingContext,
                                    NULL,
                                    TextBuffer1));
                        }
                        else
                        {
                            AmbiguityErrorsText->AppendSTRING(
                                (*Result)->GetContainer()->GetQualifiedName(false));
                        }
                    }

                    // Add ", " to the message text.
                    AmbiguityErrorsText->AppendString(L", ");

                    // Add the enclosing namespace of the current result
                    // to the message text.

                    if (CurrentGenericBindingContext)
                    {
                        StringBuffer TextBuffer1;

                        AmbiguityErrorsText->AppendString(
                            m_Errors->ExtractErrorName(
                                CurrentGenericBindingContext,
                                NULL,
                                TextBuffer1));
                    }
                    else
                    {
                        AmbiguityErrorsText->AppendSTRING(
                            CurrentResult->GetContainer()->GetQualifiedName(false));
                    }
                }

                AmbiguityExists = true;
            }
        }
    }

    return false;
}

void Semantics::LookupInModulesInImportedTarget
(
    _In_ ImportedTarget *Import,
    _In_z_ Identifier     *Name,
    unsigned        BindspaceMask,
    const Location &SourceLocation,
    _Out_ bool           &NameIsBad,
    _Inout_ bool           &AmbiguityExists,
    StringBuffer   *AmbiguityErrorsText,
    int             GenericTypeArity,
    _Inout_ Declaration   **Result
)
{
    if (!Import->m_pstrAliasName &&
        !Import->m_hasError &&
        Import->m_pTarget->IsNamespace())
    {
        Declaration *CurrentResult =
            LookupInModulesInNamespace(
                Import->m_pTarget->PNamespace(),
                Name,
                BindspaceMask,
                SourceLocation,
                NameIsBad,
                GenericTypeArity);

        if (NameIsBad)
        {
            *Result = NULL;
            return;
        }

        if (CurrentResult)
        {
            // An ambiguity exists if both the current and former result
            // are accessible and are different symbols (OK to reach the same
            // symbol by multiple paths).

            if (*Result == NULL)
            {
                *Result = CurrentResult;
            }
            else if (!IsMemberSuitableBasedOnGenericTypeArity(*Result, GenericTypeArity))
            {
                *Result = CurrentResult;
            }
            else if (IsMemberSuitableBasedOnGenericTypeArity(CurrentResult, GenericTypeArity))
            {
                if (*Result == CurrentResult)
                {
                }
                else if (!CanBeAccessible(*Result, NULL, NULL))
                {
                    *Result = CurrentResult;
                }
                else if (CanBeAccessible(CurrentResult, NULL, NULL))
                {
                    // If this is the first ambiguity, add the enclosing namespace
                    // of the former result to the message text.

                    if(AmbiguityErrorsText)
                    {
                        if (!AmbiguityExists)
                        {
                            AmbiguityErrorsText->AppendSTRING((*Result)->GetContainer()->GetQualifiedName(false));
                        }

                        // Add ", " to the message text.

                        AmbiguityErrorsText->AppendString(L", ");

                        // Add the enclosing namespace of the current result
                        // to the message text.

                        AmbiguityErrorsText->AppendSTRING(
                            CurrentResult->GetContainer()->GetQualifiedName(false));
                    }

                    AmbiguityExists = true;
                }
            }
        }
    }
}


Declaration *
Semantics::LookupInImports
(
    _In_z_ ImportedTarget *ImportsList,
    _In_z_ Identifier *Name,
    NameFlags Flags,
    unsigned BindspaceMask,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    bool AmbiguityExists = false;

    StringBuffer AmbiguityErrorsText;

    Declaration *Result = NULL;
    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext?
            GenericBindingContext:
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);
    bool BoundToAlias = false;

    for (ImportedTarget *Import = ImportsList;
         Import;
         Import = Import->m_pNext)
    {
        if(LookupInImportedTarget( Import, Name, Flags, BindspaceMask, SourceLocation, GenericTypeArity, NameIsBad,
                                   AmbiguityExists, &AmbiguityErrorsText, BoundToAlias, NameAmbiguousAcrossBaseInterfaces,
                                   ResultGenericBindingContext, &Result
                                ))
        {
            break;
        }

        if(NameIsBad)
        {
            return NULL;
        }
    }

    if (AmbiguityExists)
    {
        ReportSemanticError(
            ERRID_AmbiguousInImports2,
            SourceLocation,
            Name,
            AmbiguityErrorsText);

        NameIsBad = true;
        return Result;
    }

    Declaration *ExtraImportResult = Result;
    bool ExtraBoundToAlias = BoundToAlias;
    GenericBinding *ExtraResultGenericBindingContext = *ResultGenericBindingContext;

    if(pImportTrackerEntry)
    {
        // look it up in the extra tracked imports statement as well
        // save it in the tracker, but don't spam it to the outside world!

        ImportedTarget *ExtraImport = m_SourceFile->GetExtraTrackedImport();

        VSASSERT(ExtraImport, "Why do we have an import tracker but no extra import??");

        bool ExtraNameIsBad = false;
        bool ExtraAmbiguityExists = false;
        bool ExtraNameAmbiguousAcrossBaseInterfaces = *NameAmbiguousAcrossBaseInterfaces;

        if(!LookupInImportedTarget( ExtraImport, Name, Flags, BindspaceMask, SourceLocation, GenericTypeArity, ExtraNameIsBad,
                                    ExtraAmbiguityExists, NULL, ExtraBoundToAlias, &ExtraNameAmbiguousAcrossBaseInterfaces,
                                    &ExtraResultGenericBindingContext, &ExtraImportResult
                                ))
        {
            if(ExtraNameIsBad || ExtraAmbiguityExists)
            {
                pImportTrackerEntry->newSymbolBound = NULL;
                pImportTrackerEntry->fIsCollision = true;
                pImportTrackerEntry = NULL;
            }
        }
    }


    // If the arity of the resulting type does not match the requested for type arity,
    // then return NULL indicating no match.
    //
    if (Result &&
        (!BoundToAlias && !IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity)) ||
        //
        // Since raw generics cannot be imported, the import aliases would always refer to
        // constructed types when referring to generics. So any other generic arity besides
        // -1 or 0 are invalid.
        //
        (BoundToAlias && GenericTypeArity != -1 && GenericTypeArity != 0))
    {
        Result = NULL;
        *ResultGenericBindingContext = NULL;
    }

    // don't consider it for shadowing things below if the arity is wrong.  (we've already considered it for ambiguity at this level)
    if(ExtraImportResult &&
        (!ExtraBoundToAlias && !IsMemberSuitableBasedOnGenericTypeArity(ExtraImportResult, GenericTypeArity)) ||
        (ExtraBoundToAlias && GenericTypeArity != -1 && GenericTypeArity != 0))
    {
        ExtraImportResult = NULL;
    }

    // If nothing has turned up, search the modules of imported namespaces.

    if (Result == NULL || 
        (!BoundToAlias && !CanBeAccessible(Result, *ResultGenericBindingContext, NULL))) // Dev10 #822985: Attempt lookup in modules if current result is inaccessible, unless 
                                                                                         // result is an alias. Though an alias can't refer to an inaccessible thing (compiler 
                                                                                         // reports an error for such alias), it is good to have explicit check anyway. 
    {
        // Condition above guarantees that we get here only if lookup in imported namespaces didn't produce any
        // result or the result is inaccessible.
        AssertIfFalse(Result == NULL || !CanBeAccessible(Result, *ResultGenericBindingContext, NULL));
        
        Declaration * saveResult = NULL;
        GenericBinding * saveResultGenericBindingContext = NULL;

        // Dev10 #822985: Save inaccessible result of lookup in imported namespaces. And clear the current result.
        if(Result != NULL)
        {
            saveResult = Result;
            saveResultGenericBindingContext = *ResultGenericBindingContext;
            
            Result = NULL;
            *ResultGenericBindingContext = NULL;
        }

        VSASSERT(!(*ResultGenericBindingContext), "Inconsistency during name lookup!!!");

        for (ImportedTarget *Import = ImportsList;
             Import;
             Import = Import->m_pNext)
        {
            LookupInModulesInImportedTarget( Import, Name, BindspaceMask, SourceLocation, NameIsBad,
                                             AmbiguityExists, &AmbiguityErrorsText, GenericTypeArity, &Result );
            if(NameIsBad)
            {
                return NULL;
            }
        }

        if (AmbiguityExists)
        {
            ReportSemanticError(
                ERRID_AmbiguousInImports2,
                SourceLocation,
                Name,
                AmbiguityErrorsText);

            NameIsBad = true;
            return Result;
        }

        if(pImportTrackerEntry && 
            (ExtraImportResult == NULL || 
             (!ExtraBoundToAlias && !CanBeAccessible(ExtraImportResult, ExtraResultGenericBindingContext, NULL)))) // Dev10 #822985: Same as the 'if' above.
        {
            Declaration *saveExtraImportResult = ExtraImportResult;
            ExtraImportResult = NULL;
            
            // lookup in the modules of the extra imports statement
            ImportedTarget *ExtraImport = m_SourceFile->GetExtraTrackedImport();

            VSASSERT(ExtraImport, "Why do we have an ImportTracker but no Extra Import??");

            bool ExtraNameIsBad = NameIsBad;
            bool ExtraAmbiguityExists = AmbiguityExists;

            LookupInModulesInImportedTarget( ExtraImport, Name, BindspaceMask, SourceLocation, ExtraNameIsBad,
                                             ExtraAmbiguityExists, NULL, GenericTypeArity, &ExtraImportResult );

            if(ExtraNameIsBad || ExtraAmbiguityExists)
            {
                pImportTrackerEntry->newSymbolBound = NULL;
                pImportTrackerEntry->fIsCollision = true;
                pImportTrackerEntry = NULL;
            }

            if (ExtraImportResult &&
                !IsMemberSuitableBasedOnGenericTypeArity(ExtraImportResult, GenericTypeArity))
            {
                ExtraImportResult = NULL;
            }

            // Dev10 #822985: To preserve previous behavior as much as possible, for a case
            // when lookup in imported namespaces produced inaccessible result, we
            // will return the same result if lookup in modules didn't produce any result, or
            // its result is inaccessible as well.
            if (saveExtraImportResult != NULL && 
                (ExtraImportResult == NULL || !CanBeAccessible(ExtraImportResult, NULL, NULL)))
            {
                ExtraImportResult = saveExtraImportResult;
            }
        }

        // If the arity of the resulting type does not match the requested for type arity,
        // then return NULL indicating no match.
        //
        if (Result &&
            !IsMemberSuitableBasedOnGenericTypeArity(Result, GenericTypeArity))
        {
            Result = NULL;
        }

        // Dev10 #822985: To preserve previous behavior as much as possible, for a case
        // when lookup in imported namespaces produced inaccessible result, we
        // will return the same result if lookup in modules didn't produce any result, or
        // its result is inaccessible as well.
        if (saveResult != NULL && 
            (Result == NULL || !CanBeAccessible(Result, NULL, NULL)))
        {
            Result = saveResult;
            *ResultGenericBindingContext = saveResultGenericBindingContext;
        }
    }

    // if we've found something along the regular path, but not along the new path, we'd have found the same thing
    // even with the new imports statement in there
    if(!ExtraImportResult)
    {
        ExtraImportResult = Result;
    }

    if(pImportTrackerEntry)
    {
        pImportTrackerEntry->newSymbolBound = ExtraImportResult;
    }

    return Result;
}

Scope *
Semantics::GetEnclosingNonLocalScope
(
)
{
    if (m_ProcedureTree)
    {
        return GetEnclosingScope(m_ProcedureTree->Locals);
    }
    else
    {
        // VSWhidbey[505361] When method body is not parsed we still need to find the procedure's non local scope.
        for (Scope *CurrentLookup = m_Lookup;
             CurrentLookup;
             CurrentLookup = GetEnclosingScope(CurrentLookup))
        {
            if (CurrentLookup->GetImmediateParentOrBuild() &&
                CurrentLookup->GetImmediateParentOrBuild()->IsMethodImpl())
            {
                return GetEnclosingScope(CurrentLookup);
            }
        }

        return GetEnclosingScope(m_Lookup); // Fall back to previous behavior.
    }
}

Scope *
Semantics::GetEnclosingScope
(
    Scope *Lookup,
    NameFlags flags
)
{
    // The containment relationship appears to be:
    //
    // The parent of a hash table is the container which has the hash table
    // as a lookup table, except a hash table for a local blocks has a containing
    // hash table as a parent and a hash table for a method body has the method
    // declaration as a parent.
    //
    // The parent of a container is the hash table containing an entry for
    // the container.
    //
    Declaration *Parent = Lookup;

    do
    {
        Parent = Parent->GetImmediateParentOrBuild();
                                                                              // #28472
        if (Parent == NULL || (! (Parent->IsHash() && Parent->PHash()->IsLocalsHash()) && HasFlag(flags, NameSearchLocalsOnly)))
        {
            return NULL;
        }

        VSASSERT(!(Parent->IsContainer() && Parent->PContainer()->GetChildrenNotLoaded()), "Children should be loaded");

        // If the parent is generic look in the hash table for the generic parameters.

        if (Parent->IsClass())
        {
            if (Parent->PClass()->IsGeneric() && Lookup != Parent->PClass()->GetGenericParamsHash())
            {
                return Parent->PClass()->GetGenericParamsHash();
            }
        }

        else if (Parent->IsInterface())
        {
            if (Parent->PInterface()->IsGeneric() && Lookup != Parent->PInterface()->GetGenericParamsHash())
            {
                return Parent->PInterface()->GetGenericParamsHash();
            }
        }

        else if (Parent->IsProc())
        {
            if (Parent->PProc()->IsGeneric() && Lookup != Parent->PProc()->GetGenericParamsHash())
            {
                return Parent->PProc()->GetGenericParamsHash();
            }
        }

    } while (!Parent->IsHash());

    return Parent->PHash();
}

static Namespace *
GetEnclosingNamespace
(
    Scope *Lookup
)
{
    Declaration *Parent = Lookup;

    do
    {
        Parent = Parent->GetImmediateParent();

        if (Parent == NULL)
        {
            return NULL;
        }

    } while (!Parent->IsNamespace());

    return Parent->PNamespace();
}

Declaration *
ChaseThroughAlias
(
    Symbol *Input
)
{
    Symbol *NonAliasResult = Input->DigThroughAlias();

    if (NonAliasResult->IsGenericBinding())
    {
        return NonAliasResult->PGenericBinding();
    }
    else if (NonAliasResult->IsNamedRoot())
    {
        return NonAliasResult->PNamedRoot();
    }

    return NULL;
}

// Note: LookupName must never chase through aliases: only CheckAccessibility()
// is allowed to chase through the alias.
Symbol *
Semantics::LookupName
(
    Scope *Lookup,
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    _In_z_ Identifier *Name,
    NameFlags Flags,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Inout_ bool &NameIsBad,
    _Out_opt_ BCSYM_GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_opt_ ImportTrackerEntry *pImportTrackerEntry
)
{
    VSASSERT((Lookup && !TypeParamToLookupIn) || (!Lookup && TypeParamToLookupIn),
                "lookup hash and lookup type param should be mutually exclusive!!!");

    unsigned BindspaceMask;

    if (HasFlag(Flags, NameSearchTypeReferenceOnly))
    {
        BindspaceMask = BINDSPACE_Type;
    }
    else
    {
        BindspaceMask = BINDSPACE_Normal | BINDSPACE_Type;
    }

    // If looking up in type param, then the search can only be
    // in the inheritance hierachy of the type param
    //
    // For the core compiler, this should be set correctly, but the
    // IDE helpers could call this in multiple ways. So in order to
    // be robust...
    //
    if (!Lookup)
    {
        Flags |= NameSearchIgnoreParent;
    }

    Symbol *Result;

    GenericBinding *TempGenericBindingContext = NULL;
    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);


    Result =
        LookupInScope(
            Lookup,
            TypeParamToLookupIn,
            Name,
            Flags,
            AccessingInstanceType,
            BindspaceMask,
            HasFlag(Flags, NameSearchIgnoreParent | NameSearchIgnoreImports),
            HasFlag(Flags, NameSearchIgnoreModule),  // can be set for GlobalNameSpace based names
            //false,
            SourceLocation,
            NameIsBad,
            ResultGenericBindingContext,
            GenericTypeArity,
            NameAmbiguousAcrossBaseInterfaces,
            pImportTrackerEntry);

    //This code would only change an instance method lookup result if NameSearchConditionalCompilation is on.
    //Because FlagsAllowExtensionMethods only returns true if NameSearchConditionalCompilation is off, we don't have to worry about
    //getting back an extension method result with an attached instance method result that would need to have this logic applied to it.
    if ((Result && (HasFlag(Flags, NameSearchIgnoreAccessibility) || CanBeAccessible(Result, *ResultGenericBindingContext, AccessingInstanceType))) ||
        NameIsBad ||
        HasFlag(Flags, NameSearchIgnoreParent | NameSearchImmediateOnly))
    {
        // OK, conditional compilation constants have some special semantics because
        // they are allowed to have multiple definitions in a file, and the ordering
        // is important
        if (HasFlag(Flags, NameSearchConditionalCompilation))
        {
            Assume(!Result || !Result->IsExtensionCallLookupResult(), L"Unexpected extension call lookup result!");

            // If the constant was first defined after the source location, don't
            // return it
            if (((Location &)SourceLocation).StartsBefore(Result->GetLocation()))
            {
                Result = NULL;
            }

            // If there is a constant defined before the source location, find the
            // last one before the declaration
            while (Result &&
                   Result->PCCConstant()->GetNextCCConstant() &&
                   !((Location &)SourceLocation).StartsBefore(Result->PCCConstant()->GetNextCCConstant()->GetLocation()))
            {
                Result = Result->PCCConstant()->GetNextCCConstant();
            }

            // If we didn't find it here, go on to the project level space.
            if (Result)
            {
                return Result;
            }
        }
        else
        {
            return Result;
        }

    }

    VSASSERT(Lookup, "NULL lookup hash unexpected!!!");

    for (Scope *CurrentLookup = GetEnclosingScope(Lookup, Flags);
         CurrentLookup;
         CurrentLookup = GetEnclosingScope(CurrentLookup, Flags))
    {
        Symbol *ScopeResult =
            LookupInScope(
                CurrentLookup,
                NULL,   // No Type parameter lookup
                Name,
                Flags,
                AccessingInstanceType,
                BindspaceMask,
                HasFlag(Flags, NameSearchIgnoreImports),
                false,
                SourceLocation,
                NameIsBad,
                ResultGenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces,
                pImportTrackerEntry);

        if (NameIsBad)
        {
            return ScopeResult;
        }

        if (ScopeResult)
        {
            if (CanBeAccessible(ScopeResult, *ResultGenericBindingContext, AccessingInstanceType))
            {
                return ScopeResult;
            }
            else if (Result == NULL)
            {
                Result = ScopeResult;
            }
        }
    }

    // We should have found conditional constants by now.
    if (m_EvaluatingConditionalCompilationConstants)
    {
        return NULL;
    }

    if (!HasFlag(Flags, NameSearchIgnoreImports))
    {
        // Lookup in the project-level imports.

        Declaration *ImportsResult =
            LookupInImports(
                Lookup->GetContainingProject()->GetImportedTargets(),
                Name,
                Flags,
                BindspaceMask,
                SourceLocation,
                NameIsBad,
                ResultGenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces); // Only track usage of file-level imports

        AssertIfTrue( (*ResultGenericBindingContext) != NULL && (*ResultGenericBindingContext)->HasLocation() ); // Cache is not used for bindings with location!

        if (NameIsBad)
        {
#if IDE 
            if ( ResultGenericBindingContext != (&TempGenericBindingContext) && (*ResultGenericBindingContext) != NULL)
            {
                // Dev10 #618745: Return new or cached GenericBinding equivalent to the one kept on the project.
                *ResultGenericBindingContext = m_SymbolCreator.GetEquivalentGenericBinding(*ResultGenericBindingContext);
            }
#endif
            
            return ImportsResult;
        }

        if (ImportsResult)
        {
            if (CanBeAccessible(ImportsResult, *ResultGenericBindingContext, ContainingClass()))
            {
                #if IDE 
                    m_nameFoundInProjectImports = true;

                    if ( ResultGenericBindingContext != (&TempGenericBindingContext) && (*ResultGenericBindingContext) != NULL)
                    {
                        // Dev10 #618745: Return new or cached GenericBinding equivalent to the one kept on the project.
                        *ResultGenericBindingContext = m_SymbolCreator.GetEquivalentGenericBinding(*ResultGenericBindingContext);
                    }
                #endif

                return ImportsResult;
            }
            else if (Result == NULL)
            {
#if IDE 
                if ( ResultGenericBindingContext != (&TempGenericBindingContext) && (*ResultGenericBindingContext) != NULL)
                {
                    // Dev10 #618745: Return new or cached GenericBinding equivalent to the one kept on the project.
                    *ResultGenericBindingContext = m_SymbolCreator.GetEquivalentGenericBinding(*ResultGenericBindingContext);
                }
#endif

                Result = ImportsResult;
            }
        }
    }

    return Result;
}

// If the current interpretation occurs within a class that is or is contained
// within ContainingClass, return true. Otherwise, return false.
static bool
ContainedWithin
(
    Container *Contained,
    Container *Containing
)
{
    for (;
         Contained;
         Contained = Contained->GetContainer())
    {
        if (Contained->IsSameContainer(Containing))
        {
            return true;
        }
    }

    return false;
}

// Return true iff Derived is or is derived from Base or is declared within
// a class that is or is derived from Base.

static bool
ContainedWithinDerived
(
    Type *Derived,
    Type *Base,
    _In_ Symbols &SymbolCreator
)
{
    Type *DerivedContainer = Derived;

    do
    {
        if (TypeHelpers::IsOrInheritsFrom(DerivedContainer, Base, SymbolCreator, false, NULL))
        {
            return true;
        }

        if (DerivedContainer->IsGenericTypeBinding() &&
            DerivedContainer->PGenericTypeBinding()->GetParentBinding())
        {
            DerivedContainer = DerivedContainer->PGenericTypeBinding()->GetParentBinding();
        }
        else
        {
            DerivedContainer = DerivedContainer->PContainer()->GetContainer();
        }

    } while (DerivedContainer && TypeHelpers::IsClassOrRecordType(DerivedContainer));

    return false;
}

static bool
IsPrivateAccessible
(
    Declaration *Result,
    Container *Context
)
{
    Declaration *Parent = Result->GetParent();

    return (!Parent || !IsNamespace(Parent)) && ContainedWithin(Context, Result->GetContainer());
}

static bool
IsProtectedAccessible
(
    Declaration *Result,
    GenericBinding *ResultGenericBindingContext,
    Container *Context,
    Type *AccessingInstanceType,
    _In_ Symbols &SymbolCreator,
    CompilerHost *CompilerHost
)
{
    if (IsPrivateAccessible(Result, Context))
    {
        return true;
    }

    // A non-instance is accessible to any derived class,
    // (obviously) without an instance. An instance  member is accessible
    // to a derived class only through an instance of the derived class.

    // In some cases like when binding to generic import aliases, the ResultBindingContext
    // actually is a binding of the Result and not the binding of it parent. So need to
    // dig to the parent binding in these cases.
    //
    if (ResultGenericBindingContext && ResultGenericBindingContext->GetGeneric() == Result)
    {
        ResultGenericBindingContext = ResultGenericBindingContext->GetParentBinding();
    }

    VSASSERT(!ResultGenericBindingContext ||
             ResultGenericBindingContext->GetGeneric() == Result->GetParent()->GetContainingClass(),
                "Inconsistent generic binding context!!!");


    // Find the result's containing class or generic binding

    Type *ClassContainingResult = NULL;

    if (Result->HasGenericParent())
    {
        // 

        ClassContainingResult =
            AccessingInstanceType ?
                DeriveGenericBindingForMemberReference(
                    AccessingInstanceType,
                    Result,
                    SymbolCreator,
                    CompilerHost) :
                ResultGenericBindingContext;

        VSASSERT(!AccessingInstanceType ||
                 !ClassContainingResult ||
                 (!ResultGenericBindingContext ||
                    ClassContainingResult->PGenericBinding()->GetGeneric() == ResultGenericBindingContext->GetGeneric()),
                    "Wrong accessing instance type passed in Or Inconsistency possibly during namelookup!!!");
    }

    if (!ClassContainingResult)
    {
        ClassContainingResult = Result->GetParent()->GetContainingClass();
    }


    // Determine whether accessible through inheritance

    ClassOrRecordType *ClassContext = Context->GetContainingClass();

    if (ClassContext == NULL ||
        ClassContainingResult == NULL ||
        !ContainedWithinDerived(ClassContext, ClassContainingResult, SymbolCreator))
    {
        return false;
    }

    if (!Result->IsMember() ||
        Result->PMember()->IsShared() ||
        IsConstant(Result))
    {
        return true;
    }

    // If the accessing instance is NULL, then we will assume that the
    // accessing instance type is the same as the current Class Context.

    if (AccessingInstanceType == NULL)
    {
        return true;
    }

    // Any protected instance members in or visible in the current context
    // through inheritance are accessible in the current context through an
    // instance of the current context or any type derived from the current
    // context.
    //
    // eg:
    // Class Cls1
    //    Protected Sub foo()
    //    End Sub
    // End Class
    //
    // Class Cls2
    //   Inherits Cls1
    //
    //    Sub Test()
    //        Dim obj1 as New Cls1
    //        Obj1.foo    'Not accesssible
    //
    //        Dim obj2 as New Cls2
    //        Obj2.foo    'Accessible
    //    End Sub
    // End Class

    if (TypeHelpers::IsOrInheritsFrom(AccessingInstanceType, ClassContext, SymbolCreator, false, NULL))
    {
        return true;
    }

    // Any protected instance members in or visible in an enclosing type through
    // inheritance are accessible in the current context through an instance of
    // that enclosing type.
    //
    // eg:
    // Class Cls1
    //    Protected Sub foo()
    //    End Sub
    // End Class
    //
    // Class Cls2
    //   Inherits Cls1
    //
    //    Protected Sub goo()
    //    End Sub
    //
    //    Class Cls2_1
    //      Sub Test()
    //        Dim obj2 as New Cls2
    //        Obj2.foo    'Accessible
    //
    //        Obj2.goo    'Accessible
    //      End Sub
    //    End Class
    // End Class

    return AccessingInstanceType->IsContainer() &&
           ContainedWithin(ClassContext, AccessingInstanceType->PContainer());
}

bool
Semantics::IsAccessible
(
    Declaration *Result,
    GenericBinding *ResultGenericBindingContext,
    Container *Context,
    Type *AccessingInstanceType,
    _In_ Symbols &SymbolCreator,
    CompilerHost *CompilerHost
)
{
    bool can = false;

    switch (Result ->GetAccess())
    {
    case ACCESS_Public:
        can = true;
        break;

    case ACCESS_Protected:
        can = IsProtectedAccessible(Result , ResultGenericBindingContext, Context, AccessingInstanceType, SymbolCreator, CompilerHost);
        break;

    case ACCESS_ProtectedFriend:
        if (IsProtectedAccessible(Result , ResultGenericBindingContext, Context, AccessingInstanceType, SymbolCreator, CompilerHost))
        {
            can = true;
            break;
        }
        __fallthrough;

    case ACCESS_Friend:
        if( Context->GetContainingProject() == Result ->GetContainingProject() )
        {
            can = true;
        }
        else
        {
            // Check if the target declared us as a friend.
            can = Result ->GetContainingProject()->CheckFriendship( Context->GetContainingProject() );
        }
        break;

    case ACCESS_Private:
    case ACCESS_CompilerControlled:
        can = IsPrivateAccessible(Result , Context);
        break;

    default:

        VSFAIL("Unexpected access level");
    }

    // Give the compiler a chance to override the accessiblity check
    if (!can && CompilerHost && CompilerHost->GetCompiler()->CanOverrideAccessibilityCheck() ) 
    {
        can = CompilerHost->GetCompiler()->IsAccessible(Result);
    }

    return can;
}

#if IDE 
bool
Semantics::IsAccessible
(
    Declaration *Result,
    GenericBinding *ResultGenericBindingContext,
    Container *Context,
    Type *AccessingInstanceType,
    Compiler *Compiler,
    CompilerHost *CompilerHost
)
{
    NorlsAllocator ScratchStorage(NORLSLOC);
    Symbols ScratchSymbolCreator(Compiler, &ScratchStorage, NULL);

    return IsAccessible(Result, ResultGenericBindingContext, Context, AccessingInstanceType, ScratchSymbolCreator, CompilerHost);
}
#endif

bool
Semantics::IsAccessible
(
    Symbol *Result,
    GenericBinding *ResultGenericBindingContext,
    Type *AccessingInstanceType
)
{
    if (Result->IsExtensionCallLookupResult())
    {
        return true;
    }
    else
    {
        Assume(Result->IsNamedRoot(), L"If Result is not an extension call lookup result then it should be a named root!");

        Declaration * pNamed = Result->PNamedRoot();
        bool can;

        if (m_Lookup == NULL)
        {
            can = pNamed->GetAccess() == ACCESS_Public;
        }
        else
        {
            can = IsAccessible(pNamed, ResultGenericBindingContext, m_Lookup->GetContainer(), AccessingInstanceType, m_SymbolCreator, m_CompilerHost);
        }

        if( !can && pNamed->GetCompiler() && pNamed->GetCompiler()->CanOverrideAccessibilityCheck() )
        {
            can = pNamed->GetCompiler()->IsAccessible(pNamed);
        }

        return can;
    }
}

bool
Semantics::CanBeAccessible
(
    Symbol *Result,
    GenericBinding *ResultGenericBindingContext,
    Type *AccessingInstanceType
)
{
    if (Result->IsExtensionCallLookupResult())
    {
        return true;
    }

    Assume(Result->IsNamedRoot(), L"If result is not an extension call lookup result it should be a named root!");

    if (IsAccessible(Result, ResultGenericBindingContext, AccessingInstanceType))
    {
        return true;
    }

    if (IsProcedure(Result) &&
        ViewAsProcedure(Result)->IsOverloads())
    {
        for (Declaration *NextProcedure = Result->PNamedRoot()->GetNextOverload();
             NextProcedure;
             NextProcedure = NextProcedure->GetNextOverload())
        {
            if (IsAccessible(NextProcedure, ResultGenericBindingContext, AccessingInstanceType))
            {
                return true;
            }
        }
    }

    return false;
}

Symbol *
Semantics::CheckAccessibility
(
    Symbol *Result,
    GenericBinding *ResultGenericBindingContext,
    const Location &SourceLocation,
    NameFlags Flags,
    Type *AccessingInstanceType,
    _Inout_ bool &ResultIsBad
)
{
    if (HasFlag(Flags, NameSearchIgnoreAccessibility))
    {
        return Result;
    }

    if (Result->IsNamedRoot())
    {
        return
            CheckNamedRootAccessibility
            (
                EnsureNamedRoot(Result),
                ResultGenericBindingContext,
                SourceLocation,
                Flags,
                AccessingInstanceType,
                ResultIsBad
            );
    }
    else
    {
        return Result;
    }
}

Declaration *
Semantics::CheckNamedRootAccessibility
(
    Declaration *Result,
    GenericBinding *ResultGenericBindingContext,
    const Location &SourceLocation,
    NameFlags Flags,
    Type *AccessingInstanceType,
    _Inout_ bool &ResultIsBad
)
{
    Declaration *NonAliasResult = NULL;

    if (HasFlag(Flags, NameSearchDonotResolveImportsAlias))

    {
        NonAliasResult = Result ;
    }
    else
    {
        NonAliasResult = ChaseThroughAlias(Result );
    }

    if (IsBad(NonAliasResult))
    {
        if (IsType(NonAliasResult))
        {
            ReportBadType(NonAliasResult, SourceLocation);
        }
        else
        {
            ReportBadDeclaration(NonAliasResult, SourceLocation);
        }

        ResultIsBad = true;
    }
    else if (NonAliasResult->IsProc() && NonAliasResult->PProc()->IsOverloads())
    {
        // It is not possible to check the access until the
        // overloading is resolved.

        return Result ;
    }
    else
    {
        CheckNamedRootAccessibilityCore (
            Result,
            ResultGenericBindingContext,
            SourceLocation,
            AccessingInstanceType,
            ResultIsBad
        );

        CheckObsolete(NonAliasResult, SourceLocation);

        if (!DeclarationIsAvailableToCurrentProject(NonAliasResult))
        {
            if (m_ReportErrors)
            {
                StringBuffer TextBuffer1;

                ReportSmartReferenceError(
                    ERRID_SymbolFromUnreferencedProject3,
                    m_Project,
                    NonAliasResult->GetContainingProject(),
                    m_Compiler,
                    m_Errors,
                    NonAliasResult->GetContainingProject()->GetFileName(),     // Bug VSWhidbey 395158 - Extra information used by error correction.
                    &SourceLocation,
                    ExtractErrorName(NonAliasResult, TextBuffer1),
                    GetErrorProjectName(NonAliasResult->GetContainingProject()),
                    GetErrorProjectName(m_Project));
            }

            ResultIsBad = true;
        }
    }

    // Procedures are maintained as aliases in order to keep track of their container.

    if (!NonAliasResult->IsProc())
    {
        Result  = NonAliasResult;
    }
    if (!Result ->IsGenericBadNamedRoot())
    {
        LogDependency(Result);
    }
    return Result ;
}

void
Semantics::CheckNamedRootAccessibilityCore
(
    Declaration *Result,
    GenericBinding *ResultGenericBindingContext,
    const Location &SourceLocation,
    Type *AccessingInstanceType,
    _Inout_ bool &ResultIsBad
)
{
    if (!IsAccessible(Result , ResultGenericBindingContext, AccessingInstanceType))
    {
        if (StringPool::StringLength(Result ->GetContainer()->GetName()) > 0 &&
            // The fully qualified name of a container appears in error
            // messages, so including the container of a container is
            // redundant.
            !Result ->IsContainer())
        {
            ReportSemanticError(
                ERRID_InaccessibleMember3,
                SourceLocation,
                Result ->GetContainer(),
                Result ,
                Result ->GetAccessConsideringAssemblyContext(ContainingClass()));
        }
        else
        {
            ReportSemanticError(
                ERRID_InaccessibleSymbol2,
                SourceLocation,
                Result ,
                Result ->GetAccessConsideringAssemblyContext(ContainingClass()));
        }

        ResultIsBad = true;
    }

    if (!ResultIsBad &&
        Result->GetContainer() != NULL &&
        Result->IsMember() &&
        Result->PMember()->GetRawType() != NULL &&
        m_Errors)
    {
        bool checkIsDelayed = false;
        Container* Container = Result->GetContainer();
        SourceFile *ContainingSourceFile = Container->GetSourceFile();
        if (ContainingSourceFile &&
            ContainingSourceFile->GetCompState() == (CS_Bound - 1))
        {
            CompilerProject *ContainingProject = ContainingSourceFile->GetProject();
            if (!ContainingProject->HaveImportsBeenResolved())
            {
                Bindable* bindable = Container->GetBindableInstance();
                bindable->AddToListOfDelayedReturnTypeChecks(
                    Result->PMember(),
                    &SourceLocation,
                    m_Errors);
                checkIsDelayed = true;
            }
        }

        if (!checkIsDelayed)
        {
            // This hack is needed to stop the const propagation.
            // We should consider unifying location parameters from either all byref or all pointers and then proper consting.
            Location SourceLocation2 = SourceLocation;
            CheckResultTypeAccessible(
                Result->PMember(),
                &SourceLocation2,
                m_Lookup->GetContainer(),
                m_SymbolCreator,
                m_CompilerHost,
                m_Errors);
        }
    }
}

void
Semantics::CheckResultTypeAccessible
(
    Member          *Result,
    Location        *SourceLocation,
    Container       *Context,
    Symbols         &SymbolCreator,
    CompilerHost    *CompilerHost,
    ErrorTable      *Errors
)
{
    if (!IsResultTypeAccessible(Result->GetRawType(), Context, SymbolCreator, CompilerHost))
    {
        if (StringPool::StringLength(Result ->GetContainer()->GetName()) > 0 &&
            // The fully qualified name of a container appears in error
            // messages, so including the container of a container is
            // redundant.
            !Result->IsContainer())
        {
            StringBuffer TextBuffer1;
            StringBuffer TextBuffer2;


            Errors->CreateError(
                ERRID_InaccessibleReturnTypeOfMember2,
                SourceLocation,
                Errors->ExtractErrorName(Result->GetContainer(), Context, TextBuffer1),
                Errors->ExtractErrorName(Result, Context, TextBuffer2));
        }
        else
        {
            StringBuffer TextBuffer1;

            Errors->CreateError(
                ERRID_InaccessibleReturnTypeOfSymbol1,
                SourceLocation,
                Errors->ExtractErrorName(Result, Context, TextBuffer1));
        }
    }
}

bool
Semantics::IsResultTypeAccessible
(
    Type            *ResultType,
    Container       *Context,
    Symbols         &SymbolCreator,
    CompilerHost    *CompilerHost
)
{
    if (ResultType == NULL)
    {
        return true;
    }
    else
    {
        Type* RawType = ResultType->DigThroughAlias();
        if (RawType->IsGenericTypeBinding())
        {
            GenericTypeBinding *GenericBinding = RawType->PGenericTypeBinding();

            if (!IsResultTypeAccessible(GenericBinding->GetGenericType(), Context, SymbolCreator, CompilerHost))
            {
                return false;
            }

            for (unsigned i = 0; i < GenericBinding->GetArgumentCount(); i++)
            {
                if (!IsResultTypeAccessible(GenericBinding->GetArgument(i)->ChaseToType(), Context, SymbolCreator, CompilerHost))
                {
                    return false;
                }
            }
        }
        else if (RawType->IsNamedRoot())
        {
            if (!Semantics::IsAccessible(
                    EnsureNamedRoot(RawType),
                    NULL,
                    Context,
                    NULL,
                    SymbolCreator,
                    CompilerHost))
            {
                return false;
            }
        }
    }

    return true;
}


Declaration *
Semantics::ApplyIdenticalNamesRule
(
    Scope *Lookup,
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    Member *MemberBinding,
    NameFlags Flags,
    Type *AccessingInstanceType,
    const Location &SourceLocation
)
{
    // Check the full context here in which a Me reference is needed to access M yet is illegal.
    if (!m_Procedure ||
        WithinSharedProcedure() ||
        m_DisallowMeReferenceInConstructorCall ||
        !TypeHelpers::IsOrInheritsFrom(ContainingClass(), MemberBinding->GetParent()))  // Nested class referring to its containing class' member.
    {
        Type *MemberType = MemberBinding->GetType();
        Identifier *MemberTypeName = MemberType->PNamedRoot()->GetName();
        bool NameIsBad = false;

        Declaration *Result =
            EnsureNamedRoot
            (
                LookupName
                (
                    Lookup,
                    TypeParamToLookupIn,
                    MemberTypeName,
                    Flags | NameSearchTypeReferenceOnly | NameSearchIgnoreExtensionMethods,
                    AccessingInstanceType,
                    SourceLocation,
                    NameIsBad,
                    NULL,   // 
                    -1      // 
                )
            );

        if (Result && !NameIsBad && TypeHelpers::EquivalentTypes(Result, MemberType))
        {
            return Result;
        }
    }

    return MemberBinding;
}

Symbol *
Semantics::InterpretName
(
    _In_z_ Identifier *Name,
    Scope *Lookup,
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    NameFlags Flags,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Out_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces,
    _Out_ ImportTrackerEntry *pImportTrackerEntry
)
{
    ImportedTarget *ImportUsed = NULL;

    NameIsBad = false;

    if (Lookup == NULL && TypeParamToLookupIn == NULL)
    {
        InitializeNameLookupGenericBindingContext(GenericBindingContext);
        return NULL;
    }

    // Devdiv Bug [21871] Anonymous Type Member methods special case
    // This is a terrible hack. We are replacing, for example, _Field with $Field
    // so that the anonymous types synthetic code gen can work. For constructor
    // we must do this ONLY if the flag is set. For other methods, we'll do this
    // all the time. For now. This is a terrible hack.

    bool DoSubstitution = false;
    bool IsAnonymousTypeField = false;

    if (Lookup && m_Procedure &&
        m_Procedure->IsSyntheticMethod() &&
        m_Procedure->GetContainer() &&
        (m_Procedure->GetContainer()->IsAnonymousType() || m_Procedure->GetContainer()->IsStateMachineClass()) &&
        Name &&
        !HasFlag(Flags, NameSearchDoNotSubstituteAnonymousTypeName))
    {
        DoSubstitution = true;

        if( !m_Procedure->GetContainer()->IsStateMachineClass() &&
            m_Procedure->IsInstanceConstructor() &&
            !HasFlag(Flags, NameSearchBindingAnonymousTypeFieldInSyntheticMethod))
        {
            IsAnonymousTypeField = false;
        }
        else
        {
            IsAnonymousTypeField = true;
        }
    }

    if( DoSubstitution )
    {
        STRING *ActualName = NULL;

        if( IsAnonymousTypeField )
        {
            ActualName = AnonymousTypeCreateFieldNameFromCode(Name);
        }
        else
        {
            ActualName = UnMapAnonymousTypeConstructorNameForBody(Name);
        }

        if(ActualName)
        {
            // When we do this, we must supress name fixing.
            SetFlag(Flags, NameSearchDoNotSubstituteAnonymousTypeName);
            return InterpretName(
                ActualName,
                Lookup,
                TypeParamToLookupIn,
                Flags,
                AccessingInstanceType,
                SourceLocation,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity,
                NameAmbiguousAcrossBaseInterfaces);
        }
    }

    GenericBinding *TempGenericBindingContext = NULL;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

    if (HasFlag(Flags, NameSearchAttributeReference))
    {
        ClearFlag(Flags, NameSearchAttributeReference);

        // An attribute reference is looked up first as
        // "FredAttribute" and then as "Fred".
        // This differs from the behavior of C#.
        // See VS RAID 224602 for details.

        // fire up the ImportTrackerEntry here so we can adjust the result to not have the "Attribute" ending
        pImportTrackerEntry = NULL;
        if(m_SourceFile && m_SourceFile->GetImportTracker())
        {
            pImportTrackerEntry = &m_SourceFile->GetImportTracker()->Add();
            pImportTrackerEntry->newSymbolBound = NULL;
            pImportTrackerEntry->fIsCollision = false;

            pImportTrackerEntry->pstrCorrection = NULL;
        }

        Declaration *AttributeResult =
            EnsureNamedRoot
            (
                InterpretName
                (
                    m_Compiler->ConcatStrings(Name, STRING_CONST(m_Compiler, ComAttribute)),
                    Lookup,
                    TypeParamToLookupIn,
                    Flags | NameSearchIgnoreExtensionMethods, // we are looking specifically for attributes, therefore we ingore extension methods
                    AccessingInstanceType,
                    SourceLocation,
                    NameIsBad,
                    ResultGenericBindingContext,
                    GenericTypeArity,
                    NameAmbiguousAcrossBaseInterfaces,
                    pImportTrackerEntry
                )
            );

        if(pImportTrackerEntry &&
           pImportTrackerEntry->pstrCorrection &&
           StringPool::StringLength(pImportTrackerEntry->pstrCorrection) > StringPool::StringLength(STRING_CONST(m_Compiler, ComAttribute)))
        {
            // trim off the extra "Attribute"
            pImportTrackerEntry->pstrCorrection = m_Compiler->AddStringWithLen(
                                                                       pImportTrackerEntry->pstrCorrection,
                                                                       StringPool::StringLength(pImportTrackerEntry->pstrCorrection) -
                                                                       StringPool::StringLength(STRING_CONST(m_Compiler, ComAttribute)));
        }

        if (AttributeResult || NameIsBad)
        {
            return AttributeResult;
        }
    }


    // do import tracking here: every access to LookupInImports that matters goes through us, but we're high enough up
    // that we won't miss things that a file-level imports would have shadowed.
    if(m_SourceFile && m_SourceFile->GetImportTracker() && !pImportTrackerEntry)
    {
        pImportTrackerEntry = &m_SourceFile->GetImportTracker()->Add();
        pImportTrackerEntry->newSymbolBound = NULL;
        pImportTrackerEntry->fIsCollision = false;

        pImportTrackerEntry->pstrCorrection = NULL;
    }

    Symbol * Result  =
        LookupName(
            Lookup,
            TypeParamToLookupIn,
            Name,
            Flags,
            AccessingInstanceType,
            SourceLocation,
            NameIsBad,
            ResultGenericBindingContext,
            GenericTypeArity,
            NameAmbiguousAcrossBaseInterfaces,
            pImportTrackerEntry);

    if (Result)
    {
        // Identical Names Special Rule:  If the member M and the type of M have the same name, and both the member
        // and the type are visible from a context K in which an instance is needed to access M yet is illegal,
        // then references to member M actually refer to the type of M.

        // Swap out the member for the type if the special case is satisfied.

        //This case only applies to fields or properties, so we don't need to worry about it in the case of extension methods,
        //or in the case where extension methods and instance methods overload each other.
        //This is because CanOverloadWithExtensionMethods will return false for a field or a property instance result.

        if (HasFlag(Flags, NameSearchFirstQualifiedName) &&
            !IsBad(Result) &&
            Result->IsMember())
        {
            Member *MemberBinding = Result->PMember();
            Type *MemberType = MemberBinding->GetType();

            // 
            if (MemberType &&
                MemberType->IsNamedRoot() &&
                !TypeHelpers::IsBadType(MemberType) &&
                StringPool::IsEqual(MemberType->PNamedRoot()->GetName(), MemberBinding->GetName()))
            {
                if (!MemberBinding->IsShared() &&
                        ((MemberBinding->IsVariable() &&
                            !MemberBinding->PVariable()->IsConstant() &&         // eliminate constants
                            MemberBinding->GetParent() &&
                            TypeHelpers::IsClassOrRecordType(MemberBinding->GetParent())) ||  // eliminate local variables
                        (MemberBinding->IsProc() &&
                            !MemberBinding->PProc()->IsOverloads() &&
                            !MemberBinding->PProc()->GetFirstParam() &&
                            MemberBinding->PProc()->IsProperty())))
                {
                    Result =
                        ApplyIdenticalNamesRule
                        (
                            Lookup,
                            TypeParamToLookupIn,
                            MemberBinding,
                            Flags,
                            AccessingInstanceType,
                            SourceLocation
                        );
                }
            }
        }

        Result =
            CheckAccessibility(
                    Result,
                    *ResultGenericBindingContext,
                    SourceLocation,
                    Flags,
                    AccessingInstanceType,
                    NameIsBad);
    }

    // grab the old symbol here, so all the rules get applied.
    if(pImportTrackerEntry)
    {
        pImportTrackerEntry->location = SourceLocation;

        if(Result && pImportTrackerEntry->newSymbolBound &&
           Result != pImportTrackerEntry->newSymbolBound  &&
           ! Result->IsIntrinsicType())
        {
            pImportTrackerEntry->fIsCollision = true;
        }

        if(pImportTrackerEntry->fIsCollision)
        {
            // 

            if (Result && Result->IsNamedRoot())
            {
                STRING *pstrName = Result->PNamedRoot()->GetQualifiedName();
                pImportTrackerEntry->pstrCorrection = pstrName;
            }
        }
    }

    return Result;
}

Symbol *
Semantics::InterpretName
(
    _In_ ParseTree::Name *Name,
    Scope *Lookup,
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    NameFlags Flags,
    Type *AccessingInstanceType,
    _Out_ bool &NameIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext,
    int GenericTypeArity
)
{
    InitializeNameLookupGenericBindingContext(GenericBindingContext);

    {
        if (Name->Opcode == ParseTree::Name::GlobalNameSpace)
        {
            Namespace *GlobalNameSpace =
                                        m_Project ?
                                            m_Compiler->GetUnnamedNamespace(m_Project) :
                                            m_Compiler->GetUnnamedNamespace();
            NameIsBad = (GlobalNameSpace == NULL || GlobalNameSpace->IsBad());
            return GlobalNameSpace;
        }

        if (Name->Opcode == ParseTree::Name::Simple)
        {
            if (Name->AsSimple()->ID.IsBad)
            {
                NameIsBad = true;
                return NULL;
            }

            return
                InterpretName(
                    Name->AsSimple()->ID.Name,
                    Lookup,
                    TypeParamToLookupIn,
                    Flags,
                    AccessingInstanceType,
                    Name->TextSpan,
                    NameIsBad,
                    GenericBindingContext,
                    GenericTypeArity);
        }

        if (Name->Opcode == ParseTree::Name::SimpleWithArguments)
        {
            ParseTree::SimpleWithArgumentsName *GenericName = Name->AsSimpleWithArguments();

            if (GenericName->ID.IsBad)
            {
                NameIsBad = true;
                return NULL;
            }

            ParseTree::TypeList *UnBoundTypeArguments = GenericName->Arguments.Arguments;
            unsigned TypeArgumentCount = 0;

            ParseTree::TypeList *TypeListElement;
            for (TypeListElement = UnBoundTypeArguments;
                    TypeListElement;
                    TypeListElement = TypeListElement->Next)
            {
                TypeArgumentCount++;
            }

            VSASSERT(TypeArgumentCount == 0 ||
                     GenericTypeArity == -1 ||
                     GenericTypeArity == TypeArgumentCount,
                            "Inconsistency in nametree and type arity passed in!!!");

            Declaration *PossiblyGeneric =
                EnsureNamedRoot
                (
                    InterpretName
                    (
                        GenericName->ID.Name,
                        Lookup,
                        TypeParamToLookupIn,
                        Flags | NameSearchIgnoreExtensionMethods ,
                        AccessingInstanceType,
                        GenericName->TextSpan,
                        NameIsBad,
                        GenericBindingContext,
                        TypeArgumentCount
                    )
                );

            if (!PossiblyGeneric ||NameIsBad)
            {
                return NULL;
            }

            // Bad parse tree
            //
            if (TypeArgumentCount == 0)
            {
                NameIsBad = true;
                return PossiblyGeneric;
            }

            Type **BoundTypeArguments;
            StackAllocTypeArgumentsIfPossible(BoundTypeArguments, TypeArgumentCount, &m_TreeStorage, m_SymbolCreator);

            unsigned CurrentTypeArgIndex = 0;

            if (!(Flags & NameSearchDoNotBindTypeArguments))
            {
                for (TypeListElement = UnBoundTypeArguments;
                    TypeListElement && CurrentTypeArgIndex < TypeArgumentCount;
                    TypeListElement = TypeListElement->Next)
                {
                    ParseTree::Type *UnBoundTypeArg = TypeListElement->Element;

                    Type *BoundTypeArg = InterpretTypeName(UnBoundTypeArg, NameIsBad, TypeResolveNoFlags);

                    if (!BoundTypeArg || NameIsBad)
                    {
                        NameIsBad = true;
                        return PossiblyGeneric;
                    }

                    BoundTypeArguments[CurrentTypeArgIndex++] = BoundTypeArg;
                }
            }

            return
                m_SymbolCreator.GetGenericBinding(
                    NameIsBad,
                    PossiblyGeneric,
                    BoundTypeArguments,
                    TypeArgumentCount,
                    NULL,
                    BoundTypeArgumentsStackAllocated);
        }

        Declaration *Base = NULL;

        if (Name->Opcode == ParseTree::Name::Qualified ||
             Name->Opcode == ParseTree::Name::QualifiedWithArguments)
        {
            // For the base symbol interpretation we should pass real generic arity instead of just -1,
            // which means "any arity". Consider the following code:
            //
            // Public Class VBGClassArity
            // Public Sub OverloadArity(Of S)()
            // End Sub
            // End Class
            // Public Class VBGClassArity(Of T, V)
            // End Class
            //
            // When interpreting VBGClassArity.OverloadArity(Of S) the base name that we want to find is
            // VBGClassArity with 0 arity, not VBGClassArity(Of T, V).
            // So if the base name is QualifiedWithArguments, count its arguments, otherwise consider
            // the base name to be non-generic and use 0 arity.

            ParseTree::Name *baseName = Name->AsQualified()->Base;
            unsigned baseArity = ParseTreeHelpers::GetGenericArgumentCount(ParseTreeHelpers::GetGenericArguments(baseName));
            if (Flags & NameSearchForceUnspecifiedBaseArity)
            {
                baseArity = -1;
            }
            Base =
                EnsureNamedRoot
                (
                    InterpretName
                    (
                        baseName,
                        Lookup,
                        TypeParamToLookupIn,
                        NameSearchTypeReferenceOnly |
                            (Flags &
                                (NameSearchIgnoreImports |
                                    NameSearchIgnoreParent |
                                    NameSearchIgnoreBases |
                                    NameSearchForceUnspecifiedBaseArity |
                                    NameSearchDoNotBindTypeArguments |
                                    NameSearchIgnoreAccessibility)),
                        AccessingInstanceType,
                        NameIsBad,
                        GenericBindingContext,
                        baseArity
                    )
                );

            if (NameIsBad || !Base)
            {
                return NULL;
            }

            if (Name->Opcode == ParseTree::Name::QualifiedWithArguments)
            {
                unsigned TypeArgumentCount = ParseTreeHelpers::GetGenericArgumentCount(ParseTreeHelpers::GetGenericArguments(Name));

                VSASSERT(TypeArgumentCount == 0 ||
                         GenericTypeArity == -1 ||
                         GenericTypeArity == TypeArgumentCount,
                                "Inconsistency in nametree and type arity passed in!!!");

                GenericTypeArity = TypeArgumentCount;
            }

            if (Name->AsQualified()->Qualifier.IsBad)
            {
                NameIsBad = true;
                return NULL;
            }

            if (Base->IsContainer())
            {
                Lookup = ViewAsScope(Base->PContainer());

                // For GlobalNameSpace based names ignore all
                if (Name->AsQualified()->Base->Opcode == ParseTree::Name::GlobalNameSpace)
                {
                    Flags |= NameSearchIgnoreModule | NameSearchIgnoreParent | NameSearchIgnoreImports;
                }
            }
            else
            {
                NameIsBad = true;
                return NULL;
            }

            Symbol *Result =
                InterpretName
                (
                    Name->AsQualified()->Qualifier.Name,
                    Lookup,
                    NULL,           // NULL type parameter lookup
                    //Flags,
                    Flags | NameSearchIgnoreParent,
                    AccessingInstanceType,
                    Name->TextSpan,
                    NameIsBad,
                    GenericBindingContext,
                    GenericTypeArity
                );

            if (Result &&
                Name->Opcode == ParseTree::Name::QualifiedWithArguments &&
                !(Flags & NameSearchDoNotBindTypeArguments))
            {
                // 

                Declaration *PossiblyGeneric = EnsureNamedRoot(Result);
                unsigned TypeArgumentCount = GenericTypeArity;

                if (TypeArgumentCount == 0)
                {
                    NameIsBad = true;
                    return PossiblyGeneric;
                }

                Type **BoundTypeArguments;
                StackAllocTypeArgumentsIfPossible(BoundTypeArguments, TypeArgumentCount, &m_TreeStorage, m_SymbolCreator);

                unsigned CurrentTypeArgIndex = 0;

                for (ParseTree::TypeList *TypeListElement = Name->AsQualifiedWithArguments()->Arguments.Arguments;
                       TypeListElement && CurrentTypeArgIndex < TypeArgumentCount;
                       TypeListElement = TypeListElement->Next)
                {
                    ParseTree::Type *UnBoundTypeArg = TypeListElement->Element;

                    Type *BoundTypeArg = InterpretTypeName(UnBoundTypeArg, NameIsBad, TypeResolveNoFlags);

                    if (!BoundTypeArg || NameIsBad)
                    {
                        NameIsBad = true;
                        return PossiblyGeneric;
                    }

                    BoundTypeArguments[CurrentTypeArgIndex++] = BoundTypeArg;
                }

                // Dev10 #468703 Do not ignore parent generic binding if we have one.
                return
                    m_SymbolCreator.GetGenericBinding(
                        NameIsBad,
                        PossiblyGeneric,
                        BoundTypeArguments,
                        TypeArgumentCount,
                        Base->IsGenericTypeBinding() ? Base->PGenericTypeBinding(): NULL,
                        BoundTypeArgumentsStackAllocated);

            }
            // Dev10 #468703 Do not ignore parent generic binding if we have one.
            else if (Result && 
                Name->Opcode == ParseTree::Name::Qualified &&
                !(Flags & NameSearchDoNotBindTypeArguments) &&
                Base->IsGenericTypeBinding())
            {
                return
                    m_SymbolCreator.GetGenericBinding(
                        NameIsBad,
                        EnsureNamedRoot(Result),
                        NULL, // no type arguments
                        0, // no type arguments
                        Base->PGenericTypeBinding(),
                        true); //AllocAndCopyArgumentsToNewList
            }
            else
            {
                return Result;
            }
        }
}
    NameIsBad = true;
    return NULL;
}

Symbol *
Semantics::InterpretName
(
    _In_z_ Identifier *Name,           // Name to interpret
    Location TextSpan,          // TextSpan to look in
    Scope *Lookup,              // Current hash scope
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    NameFlags Flags,            // Flags for name interpretation
    ErrorTable *Errors,         // Error table to put any errors in
    Compiler *TheCompiler,      // Current compiler
    CompilerHost *TheCompilerHost,      // Current compilerHost
    SourceFile *File,           // Current File
    _Out_ bool &NameIsBad,            // [out] Did we find the name?
    _Out_opt_ GenericBinding **GenericBindingContext,
    _In_ NorlsAllocator *GenericBindingStorage,
    int GenericTypeArity
)
{
    VSASSERT(!GenericBindingContext || GenericBindingStorage,
                "Allocator expected if generic bindings are expected!!!");

    NorlsAllocator Storage(NORLSLOC);

    Semantics Analyzer(
        GenericBindingContext ?
            GenericBindingStorage :
            &Storage,
        Errors,
        TheCompiler,
        TheCompilerHost,
        File,
        NULL,
        false);

    Analyzer.InitializeInterpretationState(NULL, Lookup, NULL, NULL, NULL, false, true, false, false, true);

    return
        Analyzer.InterpretName(
            Name,
            Lookup,
            TypeParamToLookupIn,
            Flags,
            NULL,
            TextSpan,
            NameIsBad,
            GenericBindingContext,
            GenericTypeArity);
}

// Another overload - needed by bindable
Symbol *
Semantics::InterpretName
(
    _In_z_ Identifier *Name,                   // Name to interpret
    Location TextSpan,                  // TextSpan to look in
    Scope *Lookup,                      // Current hash scope
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    NameFlags Flags,                    // Flags for name interpretation
    Type *AccessingInstanceType,        // The type through which the member is being accessed
    Scope *CurrentContextScope,         // The current context in which this access is occuring
    ErrorTable *Errors,                 // Error table to put any errors in
    Compiler *TheCompiler,              // Current compiler
    CompilerHost *TheCompilerHost,      // Current compilerHost
    CompilationCaches *IDECompilationCaches,
    SourceFile *File,                   // Current File
    bool PerformObsoleteChecks,         // To perform obsolete check or not
    _Out_ bool &NameIsBad,                    // [out] Did we find the name?
    _Out_opt_ GenericBinding **GenericBindingContext,
    _In_ NorlsAllocator *GenericBindingStorage,
    int GenericTypeArity,
    _Inout_ bool *NameAmbiguousAcrossBaseInterfaces
)
{
    VSASSERT(!GenericBindingContext || GenericBindingStorage,
                "Allocator expected if generic bindings are expected!!!");

    NorlsAllocator Storage(NORLSLOC);

    Semantics Analyzer(
        GenericBindingContext ?
            GenericBindingStorage :
            &Storage,
        Errors,
        TheCompiler,
        TheCompilerHost,
        File,
        NULL,
        false);

#if IDE 
    Analyzer.SetIDECompilationCaches(IDECompilationCaches);
#endif

    Analyzer.InitializeInterpretationState(NULL, CurrentContextScope, NULL, NULL, NULL, false, PerformObsoleteChecks, false, false, true);

    return
        Analyzer.InterpretName(
            Name,
            Lookup,
            TypeParamToLookupIn,
            Flags,
            AccessingInstanceType,
            TextSpan,
            NameIsBad,
            GenericBindingContext,
            GenericTypeArity,
            NameAmbiguousAcrossBaseInterfaces);
}


Symbol *
Semantics::InterpretQualifiedName
(
    _In_count_(NumberOfIdentifiers) Identifier *Names[],
    unsigned int NumberOfIdentifiers,
    _In_opt_ NameTypeArguments *Arguments[],
    _In_opt_ NorlsAllocator *GenericBindingStorage,
    Scope *Lookup,
    NameFlags Flags,
    const Location &SourceLocation,
    ErrorTable *Errors,
    Compiler *TheCompiler,
    CompilerHost *TheCompilerHost,
    CompilationCaches *IDECompilationCaches,
    SourceFile *File,
    bool PerformObsoleteChecks,
    _Out_ bool &NameIsBad,
    bool DisableDeclarationCaching,
    Scope *ContextOfQualifiedName,
    Declaration *AppliedAttributeContext,
    Type * targetEmbeddedTypeIdentity // #546428
)
{
    AssertIfFalse(!HasFlag(Flags, NameSearchCanonicalInteropType) || targetEmbeddedTypeIdentity!=NULL);
    
    Symbol *Result = NULL;
    GenericBinding *ParentBinding = NULL;
    NameIsBad = false;

    NorlsAllocator ScratchAllocator(NORLSLOC);

    Semantics Analyzer(
        GenericBindingStorage ?
            GenericBindingStorage :
            &ScratchAllocator,
        Errors,
        TheCompiler,
        TheCompilerHost,
        File,
        NULL,
        false);

#if IDE 
    VSASSERT(IDECompilationCaches ||  !GetCompilerSharedState()->IsInMainThread(),"Main thread calling semantics with no compilation cache: opportunity for optimization");
    Analyzer.SetIDECompilationCaches(IDECompilationCaches);
#endif

    if (DisableDeclarationCaching)
    {
        Analyzer.m_PermitDeclarationCaching = false;
    }

    // If we're under the debugger we need to try a more qualified name
    // as the debugger interfaces require this.
    StringBuffer QualifiedName;

    // if the context is not specified, then we assume the context to be
    // same as the Lookup given for the search.
    if (!ContextOfQualifiedName)
    {
        ContextOfQualifiedName = Lookup;
    }

    Analyzer.InitializeInterpretationState(
        NULL,
        ContextOfQualifiedName,
        NULL,
        NULL,                       // No call graph
        NULL,                       // Not interested in transient symbols
        false,                      // preserve extra semantic information
        PerformObsoleteChecks,
        false,
        false,
        true,                       // merge anonymous type templates
        AppliedAttributeContext);

    if (HasFlag(Flags, NameSearchDoNotMergeNamespaceHashes))
    {
        Analyzer.m_DoNotMergeNamespaceCaches = true;
    }
    
    for (unsigned int IdIndex = 0; IdIndex < NumberOfIdentifiers; IdIndex++)
    {
        if (Lookup == NULL)
        {
            return NULL;
        }

        if (Names[IdIndex] == NULL)
        {
            NameIsBad = true;
            return NULL;
        }

        Identifier *Name = Names[IdIndex];

        NameFlags InterpretNameFlags = NameNoFlags;

        // Looking up all but the first name uses IgnoreParent
        if (IdIndex > 0 )
        {
            InterpretNameFlags |= NameSearchIgnoreParent;
        }

        // Looking up the last name uses the supplied flags.
        // Looking up the other names uses TypeReferenceOnly and a subset of the
        // supplied flags.
        // Only the first one pass NameSearchIgnoreModule which is set when the qualified name is GlobalNameSpace based.
        if (IdIndex < (NumberOfIdentifiers - 1))
        {
            InterpretNameFlags |=
                (NameSearchTypeReferenceOnly | NameSearchLeadingQualifiedName) |
                    (Flags &
                        (NameSearchIgnoreImports |
                            NameSearchIgnoreParent |
                            NameSearchIgnoreModule |
                            NameSearchIgnoreBases));

            if (IdIndex == 0)
            {
                // Set the flag for the first name of a qualified name.
                InterpretNameFlags |= NameSearchFirstQualifiedName;

                // Clean after the first use.
                ClearFlag(Flags, NameSearchIgnoreModule);
            }

            ClearFlag(InterpretNameFlags, NameSearchCanonicalInteropType); // #546428 Don't use this flag for any name, but the last one. 
        }
        else
        {
            InterpretNameFlags |= Flags;

            // #546428 Set up target type identity for Semantics::LookupInNamespaceNoCaching_WithMergedHash to use.
            if (HasFlag(Flags, NameSearchCanonicalInteropType))
            {
                Analyzer.m_EmbeddedTypeIdentity = targetEmbeddedTypeIdentity;
            }
        }

        NameTypeArguments *NameArguments = Arguments ? Arguments[IdIndex] : NULL;

        if (NameArguments && !NameArguments->m_TypesForTypeArgumentsSpecified)
        {
            // This is used to suppress generic binding synthesis when
            // interpreting raw generic names. This is needed so that
            // in these cases bindings are not synthesized and thus
            // finding the raw generic through two different bindings
            // is not treated as ambiguous.
            //
            // Eg: C1(Of , ) in GetType(C1(Of , ))
            //
            Analyzer.m_SuppressNameLookupGenericBindingSynthesis = true;
        }

        GenericBinding *GenericBindingContext = NULL;

        Result =
            Analyzer.InterpretName
            (
                Name,
                Lookup,
                NULL,               // No type parameter to lookup in
                InterpretNameFlags,
                NULL,
                SourceLocation,
                NameIsBad,
                GenericBindingStorage ?
                    &GenericBindingContext :
                    NULL,
                NameArguments ?
                    NameArguments->m_ArgumentCount :
                    0
            );

        if (Result == NULL || NameIsBad)
        {
            // 









            if (Errors &&
                Result == NULL &&
                !NameIsBad &&
                Analyzer.m_ReportErrors)
            {
                // try with "any" i.e. -1 arity

                Location ArgumentsLoc;
                GenericBinding *TempGenericBindingContext = NULL;

                Analyzer.m_ReportErrors = false;

                Declaration *TempResult =
                    EnsureNamedRoot
                    (
                        Analyzer.InterpretName
                        (
                            Name,
                            Lookup,
                            NULL,               // No type parameter to lookup in
                            InterpretNameFlags | NameSearchIgnoreExtensionMethods,
                            NULL,
                            SourceLocation,
                            NameIsBad,
                            GenericBindingStorage ?
                                &TempGenericBindingContext :
                                NULL,
                            -1
                        )
                    );

                Analyzer.m_ReportErrors = true;

                if (TempResult || NameIsBad)
                {
                    Bindable::ValidateArity(
                        Name,
                        TempResult,
                        TempGenericBindingContext->PGenericTypeBinding(),
                        NameArguments ? NameArguments->m_ArgumentCount : 0,
                        NameArguments ? NameArguments->GetAllArgumentsLocation(&ArgumentsLoc) : (Location *)&SourceLocation,
                        Errors,
                        TheCompiler,
                        NameIsBad);

                    VSASSERT(NameIsBad == true, "Name binding error expected!!!");
                }
            }

            return Result;
        }

        Analyzer.UpdateNameLookupGenericBindingContext(
            Result,
            ParentBinding,
            &GenericBindingContext);

        ParentBinding = GenericBindingContext;

        if (NameArguments && Result->IsNamedRoot())
        {
            VSASSERT(GenericBindingStorage, "No generic binding allocator supplied in the presence of type arguments!!!");

            ParentBinding =
                Bindable::ResolveGenericType(
                    Result->PNamedRoot(),
                    NameArguments,
                    ParentBinding->PGenericTypeBinding(),
                    Errors,
                    TheCompiler,
                    &Analyzer.m_SymbolCreator);
        }

        // This can occur for imports aliases which are aliases to generic instantiations
        //
        // For generic import alias A=Class1(Of Integer), reference to "A" can result in this condition.
        //
        //
        else if (ParentBinding &&
                 ParentBinding->GetGeneric() == Result)
        {
        }

        else if (IsGeneric(Result))
        {
            Analyzer.ReportSemanticError(
                ERRID_GenericTypeRequiresTypeArgs1,
                SourceLocation,
                Result);

            NameIsBad = true;

            return Result;
        }

        else if (ParentBinding && Result->IsNamedRoot())
        {
            // No arguments appear in the name. Treat the name as a generic binding because
            // its parent is a generic binding.

            ParentBinding =
                Analyzer.m_SymbolCreator.GetGenericBinding(
                    false,
                    EnsureNamedRoot(Result),
                    NULL,
                    0,
                    ParentBinding->PGenericTypeBinding());
        }

        if (ParentBinding)
        {
            Result = ParentBinding;

            if (IsBad(Result))
            {
                NameIsBad = true;
                return Result;
            }
        }

        // For performance, skip this work if we've bound to the end of the chain.
        // Otherwise, the ViewAsScope below is going to force the children of this
        // scope to load when we may not need them.
        if (IdIndex < (NumberOfIdentifiers - 1))
        {
            if (TypeHelpers::IsGenericParameter(Result))
            {
                Analyzer.ReportSemanticError(
                    ERRID_TypeParamQualifierDisallowed,
                    SourceLocation);

                NameIsBad = true;
                return NULL;
            }

            // 





            Lookup = Result->IsContainer() ? ViewAsScope(Result->PContainer()) : NULL;
        }
    }

    if (Result->IsIntrinsicType() &&
        !StringPool::IsEqual(Result->PNamedRoot()->GetName(), Result->PNamedRoot()->GetEmittedName()) &&
        StringPool::IsEqual(Names[NumberOfIdentifiers-1], Result->PNamedRoot()->GetName()))
    {
        return NULL;
    }
    return Result;
}

Declaration *
Semantics::InterpretName
(
    _In_ ParseTree::Name *Name,      // Name to Interpret
    Scope *Lookup,              // Current hash scope
    GenericParameter *TypeParamToLookupIn,  // Only one of Lookup and TypeParamToLookupIn can be passed in
    NameFlags Flags,            // Flags for name interpretation
    Type *AccessingInstanceType,
    ErrorTable *Errors,         // Error table to put any errors in
    Compiler *TheCompiler,      // Current compiler
    CompilerHost *TheCompilerHost,      // Current compilerHost
    CompilationCaches *IDECompilationCaches,
    SourceFile *File,           // Current File
    _Out_ bool &NameIsBad,            // [out] Did we find the name?
    _Out_opt_ GenericBinding **GenericBindingContext,
    _In_opt_ NorlsAllocator *GenericBindingStorage,
    int GenericTypeArity

    #if IDE 
    , bool * pwasBoundThroughProjectLevelImport
    #endif

)
{
    VSASSERT(!GenericBindingContext || GenericBindingStorage,
                            "Allocator expected if generic bindings are expected!!!");

    NorlsAllocator Storage(NORLSLOC);

    Semantics Analyzer(
            GenericBindingStorage ?
                GenericBindingStorage :
                &Storage,
        Errors,
        TheCompiler,
        TheCompilerHost,
        File,
        NULL,
        false);

    Analyzer.InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,
        NULL,       // No call graph
        NULL,       // Not interested in transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,
        false,
        true);      // merge anonymous type templates

#if IDE 
    Analyzer.SetIDECompilationCaches(IDECompilationCaches);
#endif


    Declaration *Result =
        EnsureNamedRoot(
            Analyzer.InterpretName
            (
                Name,
                Lookup,
                TypeParamToLookupIn,
                Flags,
                AccessingInstanceType,
                NameIsBad,
                GenericBindingContext,
                GenericTypeArity
            )
        );

    // If no allocator provided by caller, then caller does not back a binding
    //
    if (Result && Result->IsGenericBinding() && !GenericBindingStorage)
    {
        #if IDE 

        if (pwasBoundThroughProjectLevelImport)
        {
            *pwasBoundThroughProjectLevelImport = Analyzer.WasNameFoundInProjectLevelImport();
        }

        #endif
        return Result->PGenericBinding()->GetGeneric();
    }

    #if IDE 

    if (pwasBoundThroughProjectLevelImport)
    {
        *pwasBoundThroughProjectLevelImport = Analyzer.WasNameFoundInProjectLevelImport();
    }

    #endif

    return Result;
}

Declaration *
Semantics::InterpretXmlPrefix
(
    BCSYM_Hash *Lookup,         // Hash table which maps prefixes to namespace aliases
    _In_z_ Identifier *Prefix,         // Prefix to interpret
    NameFlags Flags,            // Flags for name interpretation
    SourceFile *File            // Current File
)
{
    // First attempt to lookup xml prefixes defined on in-scope literals
    Declaration *Result;
    while (Lookup)
    {
        Result = Lookup->SimpleBind(Prefix, SIMPLEBIND_CaseSensitive);
        if (Result)
        {
            // Return symbol for alias or for namespace, depending on name search flag
            if (HasFlag(Flags, NameSearchDonotResolveImportsAlias))
            {
                return Result;
            }
            else
            {
                return Result->PAlias()->GetAliasedSymbol()->PNamedRoot();
            }
        }

        BCSYM_NamedRoot *Parent = Lookup->GetImmediateParent();
        if (!Parent || !Parent->IsHash())
            break;

        Lookup = Parent->PHash();
    }

    // Now look for imported xml prefixes
    XmlNamespaceImportsIterator Iterator(File);

    while (Iterator.MoveNext())
    {
        if (StringPool::IsEqualCaseSensitive(Prefix, Iterator.Current()->m_pstrAliasName))
        {
            // Return symbol for alias or for namespace, depending on name search flag
            if (HasFlag(Flags, NameSearchDonotResolveImportsAlias))
            {
                return Iterator.Current()->m_pAlias->PNamedRoot();
            }
            else
            {
                return Iterator.Current()->m_pTarget->PNamedRoot();
            }
        }
    }

    return NULL;
}

Declaration *
Semantics::LookupDefaultProperty
(
    Type *LookupClassInterfaceOrGenericParam,
    Compiler *TheCompiler,
    CompilerHost *TheCompilerHost,      // Current compilerHost
    CompilationCaches *IDECompilationCaches,
    _Out_opt_ GenericBinding **GenericBindingContext,
    NorlsAllocator *GenericBindingStorage,
    bool MergeAnonymousTypes
)
{
    Semantics Analyzer(GenericBindingStorage, NULL, TheCompiler, TheCompilerHost, NULL, NULL, NULL, false);

    InitializeNameLookupGenericBindingContext(GenericBindingContext);

    Analyzer.InitializeInterpretationState(
        NULL,
        NULL,
        NULL,
        NULL,       // No call graph.
        NULL,       // Not interested in transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,
        false,
        MergeAnonymousTypes);      // merge anonymous type templates

#if IDE 
    Analyzer.SetIDECompilationCaches(IDECompilationCaches);
#endif

    Declaration *Result = NULL;
    bool TypeIsBad = false;

    if (TypeHelpers::IsClassOrRecordType(LookupClassInterfaceOrGenericParam) ||
        TypeHelpers::IsInterfaceType(LookupClassInterfaceOrGenericParam) ||
        TypeHelpers::IsGenericParameter(LookupClassInterfaceOrGenericParam))
    {
        Result =
            Analyzer.LookupDefaultProperty(
                LookupClassInterfaceOrGenericParam,
                Location::GetHiddenLocation(),
                TypeIsBad,
                GenericBindingContext);
    }

    if (TypeIsBad)
    {
        return NULL;
    }

    return Result;
}

Declaration *
Semantics::LookupDefaultProperty
(
    Type *LookupClassInterfaceOrGenericParam,
    const Location &SourceLocation,
    _Inout_ bool &PropertyIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext
)
{
    if (TypeHelpers::IsClassOrRecordType(LookupClassInterfaceOrGenericParam))
    {
        return
            LookupDefaultPropertyInClass(
                LookupClassInterfaceOrGenericParam,
                LookupClassInterfaceOrGenericParam,
                SourceLocation,
                PropertyIsBad,
                GenericBindingContext);
    }
    else if (TypeHelpers::IsInterfaceType(LookupClassInterfaceOrGenericParam))
    {
        return
            LookupDefaultPropertyInInterface(
                LookupClassInterfaceOrGenericParam,
                LookupClassInterfaceOrGenericParam,
                SourceLocation,
                PropertyIsBad,
                GenericBindingContext);
    }
    else
    {
        VSASSERT(TypeHelpers::IsGenericParameter(LookupClassInterfaceOrGenericParam), "Non-Type parameter unexpected!!!");

        return
            LookupDefaultPropertyInGenericTypeParameter(
                LookupClassInterfaceOrGenericParam->PGenericParam(),
                LookupClassInterfaceOrGenericParam,
                SourceLocation,
                PropertyIsBad,
                GenericBindingContext);
    }

    VSFAIL("Unexpected code path!!!");

    return NULL;
}

Declaration *
Semantics::LookupDefaultPropertyInClass
(
    Type *LookupClass,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Out_ bool &PropertyIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext
)
{
    VSASSERT(LookupClass->IsClass(), "Non-Class unexpected!!!");

    Declaration *Result = NULL;
    PropertyIsBad = false;

    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

    for (Type* Lookup = LookupClass;
         Lookup;
         Lookup = GetBaseClass(Lookup))
    {
        ClassOrRecordType *Class = Lookup->PClass();

        Class->EnsureChildrenLoaded();
        Result = Class->GetDefaultProperty();

        if (Result)
        {
            *ResultGenericBindingContext =
                Lookup->IsGenericBinding() ?
                    Lookup->PGenericBinding() :
                    NULL;

            // Ignore inaccessible members if an accessible member is available
            // in a base class.
            //
            // If the method is overloaded, then we check for the most permissible
            // of the overloads. If all of them are inaccessible, we can safely ignore them
            // because the first thing overloads does is drop all inaccessible members from
            // the set.
            //
            if (!CanBeAccessible(Result, *ResultGenericBindingContext, AccessingInstanceType))
            {
                GenericBinding *BaseResultGenericBindingContext = NULL;

                Type *BaseClass = GetBaseClass(Lookup);

                Declaration *BaseResult =
                    BaseClass ?
                        LookupDefaultPropertyInClass(
                            BaseClass,
                            AccessingInstanceType,
                            SourceLocation,
                            PropertyIsBad,
                            &BaseResultGenericBindingContext) :
                        NULL;

                if (BaseResult && CanBeAccessible(BaseResult, BaseResultGenericBindingContext, AccessingInstanceType))
                {
                    *ResultGenericBindingContext = BaseResultGenericBindingContext;
                    return BaseResult;
                }
            }

            return
                CheckNamedRootAccessibility
                (
                        Result,
                        *ResultGenericBindingContext,
                        SourceLocation,
                        NameNoFlags,
                        AccessingInstanceType,
                        PropertyIsBad
                );
        }
    }

    *ResultGenericBindingContext = NULL;
    return NULL;
}

Declaration *
Semantics::LookupDefaultPropertyInInterface
(
    Type *LookupInterface,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Out_ bool &PropertyIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext
)
{
    VSASSERT(LookupInterface->IsInterface(), "Non-interface type unexpected!!!");

    InterfaceType *Interface = LookupInterface->PInterface();
    Declaration *Result = NULL;
    PropertyIsBad = false;

    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

    Interface->EnsureChildrenLoaded();
    Result = Interface->GetDefaultProperty();

    if (Result)
    {
        CreateNameLookupGenericBindingContext(Result, LookupInterface, ResultGenericBindingContext);

        return
            CheckNamedRootAccessibility
            (
                Result,
                *ResultGenericBindingContext,
                SourceLocation,
                NameNoFlags,
                AccessingInstanceType,
                PropertyIsBad
            );
    }

    InterfaceList *BaseInterfaceList = Interface->GetFirstImplements();

    for (InterfaceList *Base = BaseInterfaceList;
         Base;
         Base = Base->GetNext())
    {
        Symbol *ChasedBase = Base->GetInterfaceIfExists();

        if (ChasedBase == NULL)
        {
            // At some points during Bindable, the base interface may not be filled in.
            // (In particular, this occurs during the interpretation of an Inherits
            // statement within the interface.)
            continue;
        }

        if (ChasedBase->IsBad() || !ChasedBase->IsInterface())
        {
            PropertyIsBad = true;
            return NULL;
        }

        if (LookupInterface->IsGenericBinding())
        {
            ChasedBase =
                ReplaceGenericParametersWithArguments(
                    ChasedBase,
                    LookupInterface->PGenericBinding(),
                    m_SymbolCreator);
        }

        LookupDefaultPropertyInBaseInterface(
            ChasedBase,
            AccessingInstanceType,
            SourceLocation,
            PropertyIsBad,
            Result,
            *ResultGenericBindingContext);

        if (PropertyIsBad)
        {
            return Result;
        }
    }

    return Result;
}

void
Semantics::LookupDefaultPropertyInBaseInterface
(
    Type *LookupBaseInterface,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Out_ bool &PropertyIsBad,
    _Inout_ Declaration *&Result,
    _Inout_ GenericBinding *&ResultGenericBindingContext
)
{
    VSASSERT(&ResultGenericBindingContext, "NULL in/out buffer unexpected for generic binding context!!!");

    VSASSERT(LookupBaseInterface->IsInterface(), "Non-interface type unexpected!!!");

    GenericBinding *CandidateGenericBindingContext = NULL;

    Declaration *Candidate =
        LookupDefaultPropertyInInterface(
            LookupBaseInterface,
            AccessingInstanceType,
            SourceLocation,
            PropertyIsBad,
            &CandidateGenericBindingContext);

    //Removed 2006-01-17 - Microsoft VSWhidbey #569931

    //UpdateNameLookupGenericBindingContext(LookupBaseInterface, &CandidateGenericBindingContext);

    if (PropertyIsBad)
    {
        Result = Candidate;
        ResultGenericBindingContext = CandidateGenericBindingContext;

        return;
    }

    if (Candidate)
    {
        // Finding a declaration on two separate paths is an
        // ambiguity error unless it results from finding the same
        // declaration from two different inheritances from the
        // same interface.
        //
        // Interface members are always public, so no checking of
        // accessibility is required.

        if (Result &&
            (Result != Candidate ||
                !TypeHelpers::EquivalentTypeBindings(
                    ResultGenericBindingContext->PGenericTypeBinding(),
                    CandidateGenericBindingContext->PGenericTypeBinding())))
        {
            ReportSemanticError(
                ERRID_DefaultPropertyAmbiguousAcrossInterfaces4,
                SourceLocation,
                Result,
                ResultGenericBindingContext ?
                    ResultGenericBindingContext :
                    Result->GetParent(),
                Candidate,
                CandidateGenericBindingContext ?
                    CandidateGenericBindingContext :
                    Candidate->GetParent());

            PropertyIsBad = true;
            Result = NULL;
            ResultGenericBindingContext = NULL;

            return;
        }

        Result = Candidate;
        ResultGenericBindingContext = CandidateGenericBindingContext;
    }

    return;
}

Declaration *
Semantics::LookupDefaultPropertyInGenericTypeParameter
(
    GenericParameter *TypeParameter,
    Type *AccessingInstanceType,
    const Location &SourceLocation,
    _Inout_ bool &PropertyIsBad,
    _Out_opt_ GenericBinding **GenericBindingContext
)
{
    GenericBinding *TempGenericBindingContext;

    GenericBinding **ResultGenericBindingContext =
        GenericBindingContext ?
            GenericBindingContext :
            &TempGenericBindingContext;

    InitializeNameLookupGenericBindingContext(ResultGenericBindingContext);

    if (TypeParameter->IsBad() || !TypeParameter->GetConstraints())
    {
        return NULL;
    }

    Declaration *Result = NULL;

    // If a class constraint exists, look up in the class constraints
    //

    Type *ClassConstraint =
        GetClassConstraint(
            TypeParameter,
            m_CompilerHost,
            &m_TreeStorage,
            true,  // ReturnArraysAs"System.Array"
            true   // ReturnValuesAs"System.ValueType"or"System.Enum"
            );

    if (ClassConstraint)
    {
        Result =
            LookupDefaultPropertyInClass(
                ClassConstraint,
                AccessingInstanceType,
                SourceLocation,
                PropertyIsBad,
                ResultGenericBindingContext);
    }

    if (Result &&
        CanBeAccessible(
            Result,
            *ResultGenericBindingContext,
            AccessingInstanceType))
    {
        return Result;
    }

    // If no accessible member found in the class type constraining the generic parameter,
    // then lookup in the interface constraints.

    Declaration *InterfaceLookupResult = NULL;
    GenericBinding *InterfaceLookupResultGenericContext = NULL;

    BCITER_Constraints ConstraintsIter(TypeParameter, true, m_Compiler);

    while (GenericConstraint *Constraint = ConstraintsIter.Next())
    {
        VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

        if (!Constraint->IsGenericTypeConstraint())
        {
            continue;
        }

        Type *ChasedBase = Constraint->PGenericTypeConstraint()->GetType();

        if (ChasedBase == NULL || !ChasedBase->IsInterface())
        {
            // At some points during Bindable, the base interface may not be filled in.
            // (In particular, this occurs during the interpretation of an Inherits
            // statement within the interface.)
            continue;
        }

        if (ChasedBase->IsBad())
        {
            PropertyIsBad = true;
            return NULL;
        }

        LookupDefaultPropertyInBaseInterface(
            ChasedBase,
            AccessingInstanceType,
            SourceLocation,
            PropertyIsBad,
            InterfaceLookupResult,
            InterfaceLookupResultGenericContext);

        if (PropertyIsBad)
        {
            *ResultGenericBindingContext = InterfaceLookupResultGenericContext;
            return InterfaceLookupResult;
        }
    }

    if (InterfaceLookupResult)
    {
        *ResultGenericBindingContext = InterfaceLookupResultGenericContext;
        return InterfaceLookupResult;
    }

    return Result;
}

Type *
Semantics::InterpretTypeName
(
    ParseTree::Type *TypeName,
    _Out_ bool &TypeIsBad
)
{
    return
        InterpretTypeName(
            TypeName,
            TypeIsBad,
            TypeResolveNoFlags);
}

Type *
Semantics::InterpretTypeName
(
    ParseTree::Type *TypeName,
    _Out_ bool &TypeIsBad,
    TypeResolutionFlags Flags
)
{
    TypeIsBad = false;

    if (TypeName == NULL)
    {
        return GetFXSymbolProvider()->GetObjectType();
    }

    Declaration *TypeLookupContext = NULL;


    // Note that the setting of the TypeLookupContext is extremely sensitive in the
    // debugger scenarios. So ensure that any hash set to be the lookup context from
    // here is accessible in Bindable::ResolveNamedType through the named root when
    // looking for the unnamed namespace. This is required so that "Global" qualified
    // expressions work in the immediate window.

    if (m_Procedure)
    {
        TypeLookupContext = m_Procedure->GetGenericParamsHash();
    }

    if (TypeLookupContext == NULL)
    {
        if (IsAppliedAttributeContext())
        {
            // Yes, m_NamedContextForAppliedAttribute can be a non-container named root and
            // this is intentional for correct binding semantics for attributes.
            //
            TypeLookupContext = m_NamedContextForAppliedAttribute;
        }
        else
        {
            if (TypeLookupContext == NULL)
            {
                TypeLookupContext = ContainingContainer();
            }

            if (TypeLookupContext == NULL && m_Lookup != NULL)
            {
                TypeLookupContext = m_Lookup->GetParent();
            }
        }
    }

    if (TypeLookupContext == NULL)
    {
        ReportSemanticError(
            ERRID_NoTypeNamesAvailable,
            TypeName->TextSpan);

        TypeIsBad = true;
        return NULL;
    }

    Type *Result = NULL;

    Result =
        Declared::ResolveType(
            &m_TreeStorage,
            m_SourceFile,
            m_CompilationCaches,
            m_Errors,
            m_CompilerHost,
            TypeName,
            TypeLookupContext,
            m_Lookup,
            Flags | (PerformObsoleteChecks() ? TypeResolvePerformObsoleteChecks : TypeResolveNoFlags),
            IsAppliedAttributeContext());

    if (Result)
    {
        Result = ResolveTypeNameGenericBinding(Result->DigThroughAlias());
    }

    // ISSUE Microsoft 4/25/00: How do we get a NULL type here?
    if (!Result || TypeHelpers::IsBadType(Result))
    {
        TypeIsBad = true;
    }

    return Result;
}

//-------------------------------------------------------------------------------------------------
//
// This method provides a hook for the debugger to further resolve a named type which happens to 
// be generic.  Take for example Func(Of T).  The debugger can further resolve T to be the actual
// type involved in the current context since there are no generics at runtime.  
//
// In the core compiler there is no oppurutinity to do this since the named type is intentionally
// generic
//
//-------------------------------------------------------------------------------------------------
Type* 
Semantics::ResolveTypeNameGenericBinding( _In_ Type* pType )
{
    return pType;
}

GenericBinding* 
Semantics::CreateMeGenericBinding( _In_ ClassOrRecordType* pMeType )
{
	return SynthesizeOpenGenericBinding(pMeType, m_SymbolCreator);
}


void
Semantics::CreateNameLookupGenericBindingContext
(
    Declaration *Member,
    Symbol *TypeOrMethodFoundIn,
    _Out_opt_ GenericBinding **GenericBindingContext,
    _In_opt_ Type *AccessingInstanceType // only used by the EE overload
)
{
    if (!GenericBindingContext)
    {
        return;
    }

    *GenericBindingContext = NULL;

    if (m_SuppressNameLookupGenericBindingSynthesis)
    {
        return;
    }

    if (!Member ||
        !IsGenericOrHasGenericParent(Member->GetParent()) ||
        TypeHelpers::IsGenericParameter(Member) ||
        !TypeOrMethodFoundIn ||
        !TypeOrMethodFoundIn->IsNamedRoot())
    {
        return;
    }

    // 

    if (TypeHelpers::IsGenericTypeBinding(TypeOrMethodFoundIn) || !IsGenericOrHasGenericParent(TypeOrMethodFoundIn->PNamedRoot()))
    {
        *GenericBindingContext =
            DeriveGenericBindingForMemberReference(TypeOrMethodFoundIn, Member, m_SymbolCreator, m_CompilerHost);

        return;
    }

    Declaration *DeclOfTypeOrMethodFoundIn = TypeOrMethodFoundIn->PNamedRoot();

    if (IsGenericOrHasGenericParent(DeclOfTypeOrMethodFoundIn))
    {
        *GenericBindingContext =
            SynthesizeOpenGenericBinding(DeclOfTypeOrMethodFoundIn, m_SymbolCreator);

        if (IsType(DeclOfTypeOrMethodFoundIn))
        {
            *GenericBindingContext =
                DeriveGenericBindingForMemberReference(*GenericBindingContext, Member, m_SymbolCreator, m_CompilerHost);
        }

        return;
    }
}

void
Semantics::UpdateNameLookupGenericBindingContext
(
    Symbol * NameLookupResult,
    Type  * PossiblyGenericType,
    GenericBinding ** GenericBindingContext
)
{
    UpdateNameLookupGenericBindingContext(PossiblyGenericType, GenericBindingContext);

    if (NameLookupResult && NameLookupResult->IsExtensionCallLookupResult() && NameLookupResult->PExtensionCallLookupResult()->GetInstanceMethodLookupResult())
    {
        GenericBinding * context = NameLookupResult->PExtensionCallLookupResult()->GetInstanceMethodLookupGenericBinding();
        UpdateNameLookupGenericBindingContext(PossiblyGenericType, &context);
        NameLookupResult->PExtensionCallLookupResult()->SetInstanceMethodLookupGenericBinding(context);
    }
}

void
Semantics::UpdateNameLookupGenericBindingContext
(
    Type *PossiblyGenericType,
    _Inout_opt_ GenericBinding **GenericBindingContext
)
{
    if (!GenericBindingContext || !(*GenericBindingContext) || !PossiblyGenericType)
    {
        return;
    }

    // Applicable only for generic type bindings

    // Nothing to update for generic method contexts, because method contexts can
    // only occur within the body of the method and so are always open bindings.

    if (!TypeHelpers::IsGenericTypeBinding(*GenericBindingContext))
    {
        return;
    }

    if (!TypeHelpers::IsGenericTypeBinding(PossiblyGenericType))
    {
        return;
    }

    *GenericBindingContext =
        ReplaceGenericParametersWithArguments(
            *GenericBindingContext,
            PossiblyGenericType->PGenericTypeBinding(),
            m_SymbolCreator)->PGenericBinding();
}

BCSYM_NamedType *
Semantics::CreateNamedType
(
    _In_ Symbols &SymbolCreator,
    Declaration *Context,
    ULONG NameCount,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, NameCount);

    BCSYM_NamedType *NamedType =
        CreateNamedType(
            SymbolCreator,
            Context,
            NULL,
            true,
            false,
            true,
            NameCount,
            ArgumentList);

    va_end(ArgumentList);

    return NamedType;
}

BCSYM_NamedType *
Semantics::CreateNamedType
(
    _In_ Symbols &SymbolCreator,
    Declaration *Context,
    _Inout_opt_ BCSYM_NamedType **TypeList,
    bool IsGlobalNameSpaceBased,
    bool IsUsedInAppliedAttributeContext,
    bool IsTypeArgumentForGenericType,
    ULONG NameCount,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, NameCount);

    BCSYM_NamedType *NamedType =
        CreateNamedType(
            SymbolCreator,
            Context,
            TypeList,
            IsGlobalNameSpaceBased,
            IsUsedInAppliedAttributeContext,
            IsTypeArgumentForGenericType,
            NameCount,
            ArgumentList);

    va_end(ArgumentList);

    return NamedType;
}

BCSYM_NamedType *
Semantics::CreateNamedType
(
    _In_ Symbols &SymbolCreator,
    Declaration *Context,
    _Inout_opt_ BCSYM_NamedType **TypeList,
    bool IsGlobalNameSpaceBased,
    bool IsUsedInAppliedAttributeContext,
    bool IsTypeArgumentForGenericType,
    ULONG NameCount,
    va_list ArgumentList
)
{
    BCSYM_NamedType *NamedType = SymbolCreator.GetNamedType(NULL, Context, NameCount, TypeList);
    NamedType->SetSymbol(NULL);

    if (NameCount > 0)
    {
        NamedType->SetArgumentsArray((NameTypeArguments **)SymbolCreator.GetNorlsAllocator()->Alloc(
            VBMath::Multiply(
            sizeof(NameTypeArguments *), 
            NameCount)));
        for (ULONG i = 0; i < NameCount; ++i)
        {
            NamedType->GetNameArray()[i] = va_arg(ArgumentList, STRING *);
            Declaration *RawType = va_arg(ArgumentList, Declaration *);
            NamedType->GetArgumentsArray()[i] = va_arg(ArgumentList, NameTypeArguments *);

            if (RawType)
            {
                NamedType->SetSymbol(
                    IsTypeArgumentForGenericType && NamedType->GetArgumentsArray()[i] && NamedType->GetArgumentsArray()[i]->m_TypesForTypeArgumentsSpecified ?
                        SymbolCreator.GetGenericBinding(
                            false,
                            RawType,
                            NamedType->GetArgumentsArray()[i]->m_Arguments,
                            NamedType->GetArgumentsArray()[i]->m_ArgumentCount,
                            NamedType->GetSymbol() && NamedType->GetSymbol()->IsGenericTypeBinding() ?
                                NamedType->GetSymbol()->PGenericTypeBinding() :
                                NULL) :
                        RawType);
            }
        }
    }

    NamedType->SetIsGlobalNameSpaceBased(IsGlobalNameSpaceBased);
    NamedType->SetIsUsedInAppliedAttributeContext(IsUsedInAppliedAttributeContext);

    return NamedType;
}

NameTypeArguments *
Semantics::CreateNameTypeArguments
(
    Symbols &SymbolCreator,
    bool TypesForTypeArgumentsSpecified,
    ULONG ArgumentCount,
    ...
)
{
    va_list ArgumentList;
    va_start(ArgumentList, ArgumentCount);

    NameTypeArguments *Args = (NameTypeArguments *)SymbolCreator.GetNorlsAllocator()->Alloc(sizeof(NameTypeArguments));
    Args->m_Arguments = NULL;
    Args->m_ArgumentCount = ArgumentCount;
    Args->m_TypesForTypeArgumentsSpecified = false;

    if (TypesForTypeArgumentsSpecified && ArgumentCount > 0)
    {
        Args->m_Arguments = (BCSYM **)SymbolCreator.GetNorlsAllocator()->Alloc(VBMath::Multiply(
            sizeof(BCSYM *), 
            ArgumentCount));

        for (ULONG i = 0; i < ArgumentCount; ++i)
        {
            Args->m_Arguments[i] = va_arg(ArgumentList, BCSYM *);
        }

        Args->m_TypesForTypeArgumentsSpecified = true;
    }

    return Args;
}

NameTypeArguments *
Semantics::CreateNameTypeArguments
(
    Symbols &SymbolCreator,
    BCSYM **Arguments,
    ULONG ArgumentCount
)
{
    NameTypeArguments *Args = (NameTypeArguments *)SymbolCreator.GetNorlsAllocator()->Alloc(sizeof(NameTypeArguments));
    Args->m_Arguments = Arguments;
    Args->m_ArgumentCount = ArgumentCount;
    Args->m_TypesForTypeArgumentsSpecified = (ArgumentCount > 0 && Args->m_Arguments != NULL);

    return Args;
}

Declaration *
Semantics::GetExtensionAttributeClass
(
    Compiler * pCompiler,
    CompilerHost * pCompilerHost,
    SourceFile * pSourceFile,
    _In_ NorlsAllocator * pTreeStorage,
    Type * pAccessingInstanceType
)
{
    bool nameIsBad = false;

    ParserHelper parseHelper(pTreeStorage);

    Semantics
        analyzer
        (
            pTreeStorage,
            NULL, //don't care about errors
            pCompiler,
            pCompilerHost,
            pSourceFile,
            NULL,
            false
        );

    analyzer.InitializeInterpretationState
    (
        NULL,
        pSourceFile->GetUnnamedNamespace()->GetHash(),
        NULL,
        NULL,       // No call graph
        NULL,       // Not interested in transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,
        false,
        true
    );

    return
        EnsureNamedRoot
        (
            analyzer.InterpretName
            (
                parseHelper.CreateName
                (
                    4,
                    STRING_CONST(pCompiler, System),
                    STRING_CONST(pCompiler, Runtime),
                    STRING_CONST(pCompiler, CompilerServices),
                    STRING_CONST(pCompiler, Extension)
                ),
                pSourceFile->GetUnnamedNamespace()->GetHash(),
                NULL,
                NameSearchGlobalName | NameSearchIgnoreExtensionMethods | NameSearchAttributeReference,
                pAccessingInstanceType,
                nameIsBad,
                NULL,
                0
            )
        );
}

Declaration *
Semantics::GetExtensionAttributeCtor
(
    Compiler * pCompiler,
    CompilerHost * pCompilerHost,
    SourceFile * pSourceFile,
    _In_opt_ NorlsAllocator * pTreeStorage,
    Type * pAccessingInstanceType
)
{
    ThrowIfNull(pCompiler);
    ThrowIfNull(pCompilerHost);
    ThrowIfNull(pSourceFile);
    ThrowIfNull(pTreeStorage);
    //pAccessingInstanceType may be null!

    Declaration * pRet = NULL;
    bool nameIsBad = false;

    ParserHelper parseHelper(pTreeStorage);

    Semantics
        analyzer
        (
            pTreeStorage,
            NULL, //don't care about errors
            pCompiler,
            pCompilerHost,
            pSourceFile,
            NULL,
            false
        );

    analyzer.InitializeInterpretationState
    (
        NULL,
        pSourceFile->GetUnnamedNamespace()->GetHash(),
        NULL,
        NULL,       // No call graph
        NULL,       // Not interested in transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,
        false,
        true
    );

    Declaration * pAttribute =
        EnsureNamedRoot
        (
            analyzer.InterpretName
            (
                parseHelper.CreateName
                (
                    4,
                    STRING_CONST(pCompiler, System),
                    STRING_CONST(pCompiler, Runtime),
                    STRING_CONST(pCompiler, CompilerServices),
                    STRING_CONST(pCompiler, Extension)
                ),
                pSourceFile->GetUnnamedNamespace()->GetHash(),
                NULL,
                NameSearchGlobalName | NameSearchIgnoreExtensionMethods | NameSearchAttributeReference,
                pAccessingInstanceType,
                nameIsBad,
                NULL,
                0
            )
        );

    if (!nameIsBad &&
        pAttribute &&
        pAttribute->IsClass() &&
        pCompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::AttributeType) &&
        analyzer.IsOrInheritsFromOrImplements(pAttribute, pCompilerHost->GetFXSymbolProvider()->GetType(FX::AttributeType)))
    {
        CorAttributeTargets requiredTargets = (CorAttributeTargets)(catAssembly | catClass | catMethod);

        CorAttributeTargets attrValidOn = (CorAttributeTargets)0;
        bool allowMultiple = false;
        bool inherited = false;

        if
        (
            pAttribute->GetPWellKnownAttrVals() &&
            pAttribute->GetPWellKnownAttrVals()->GetAttributeUsageData(&attrValidOn, &allowMultiple, &inherited) &&
            (attrValidOn & requiredTargets) == requiredTargets
        )
        {
            nameIsBad = false;

            Declaration * pDecl =
                analyzer.ImmediateLookup
                (
                    pAttribute->PContainer()->GetHash(),
                    STRING_CONST(pCompiler, Constructor),
                    BINDSPACE_Normal
                );

            while
            (
                pDecl &&
                (
                    !analyzer.IsAccessible(pDecl, NULL, pAccessingInstanceType) ||
                    ! pDecl->IsProc() ||
                    ! pDecl->PProc()->IsInstanceConstructor() ||
                    pDecl->PProc()->GetFirstParam()
                )
            )
            {
                pDecl = pDecl->GetNextOfSameName();
            }

            pRet = pDecl;
        }
    }
    return pRet;
}
