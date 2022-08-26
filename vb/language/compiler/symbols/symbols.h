//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Helpers used to create symbols.
//
//-------------------------------------------------------------------------------------------------

#pragma once

enum Vtypes;
struct Text;
class Compiler;
class CompilerFile;
struct ConstantValue;
class BCSYM_NamedRoot;
class GenericBindingCache;


#define DEFAULT_HASH_SIZE 16


class TransientSymbolStore;

/*****************************************************************************
;Symbols

Factory methods for building symbol table symbols
*****************************************************************************/
struct Symbols
{
    Symbols(
        Compiler *pCompiler,
        _In_ NorlsAllocator* pnra,_In_
        LineMarkerTable* plmt,_In_
        GenericBindingCache *GenericBindingCache = NULL);

    // Memory allocation.
    NorlsAllocator *GetNorlsAllocator()
    {
        return m_Allocator;
    }

    void SetNorlsAllocator(NorlsAllocator *Allocator)
    {
        m_Allocator = Allocator;
    }

    void SetCompiler(Compiler *CompilerInstance)
    {
        m_CompilerInstance = CompilerInstance;
    }

    // Accessors to static symbols.
    static BCSYM_SimpleType *GetVoidType();
    static BCSYM_GenericBadNamedRoot *GetGenericBadNamedRoot();

    // Wrap a type symbol.
    BCSYM_PointerType *MakePtrType(BCSYM *ptypRoot);

    // Symbol allocators and constructors.
    BCSYM_NamedRoot *GetBadNamedRoot(_In_opt_z_ STRING *pstrName,
                                     _In_opt_z_ STRING *pstrNameSpace,
                                     DECLFLAGS uFlags,
                                     BindspaceType bs,
                                     ERRID errid,            // May be zero
                                     _In_opt_z_ STRING *pstrExtra,      // May be NULL
                                     _Inout_ SymbolList *psymlist,  // May be NULL
                                     bool HasLocation = false); // BadNamedRoots typically don't have location, unless we are importing types for No-PIA, for example.

    void InitBadNamedRoot
    (
        _Out_ BCSYM_NamedRoot *NewBadNamedRoot,
        _In_opt_z_ STRING *Name,
        _In_opt_z_ STRING *Namespace,
        DECLFLAGS DeclarationFlags,
        BindspaceType BindingSpace,
        ERRID ErrorId,              // may be zero
        _In_opt_z_ STRING *ErrorString         // may be NULL
    );

    BCSYM_NamedType *GetNamedType(Location *ploc,
                                  BCSYM_NamedRoot *psymContext,
                                  unsigned cNames,
                                  _Inout_ _Deref_prepost_opt_valid_ BCSYM_NamedType **ppnamedtypeList);

    BCSYM_ArrayType *GetArrayType(unsigned Rank,
                                  BCSYM *ElementType);

    BCSYM_ArrayLiteralType *
    GetArrayLiteralType
    (
        unsigned rank,
        BCSYM * pElementType,
        ConstIterator<ILTree::Expression *> &elements,
        const Location & literalLoc
    );   

    BCSYM_Expression *GetConstantExpression(Location *ploc,
                                            BCSYM_NamedRoot *psymContext,
                                            _Inout_opt_ _Deref_prepost_opt_valid_ BCSYM_Expression **ppconstexprListHead,
                                            _Inout_opt_ _Deref_prepost_opt_valid_ BCSYM_Expression **ppconstexprListTail,
                                            _In_opt_z_ STRING *pstrFake,
                                            _In_opt_count_(ExpressionStringLength+1) _Pre_z_ const WCHAR *wszFake2,
                                            size_t ExpressionStringLength);

    BCSYM_Expression *GetFixedExpression(_In_ ConstantValue *Value);

    BCSYM_Alias *GetAlias(_In_z_ STRING *pstrName,
                          DECLFLAGS uFlags,
                          BCSYM *psymAliased,
                          _Inout_ SymbolList *psymbollist,
                          bool DigThroughAliases = true);

    BCSYM_Alias *GetAlias(Location *pLoc,
                          _In_z_ STRING *pstrName,
                          DECLFLAGS uFlags,
                          BCSYM *psymAliased,
                          _Inout_ SymbolList *psymbollist,
                          bool DigThroughAliases = true);

    BCSYM_Alias *GetAliasOfSymbol(BCSYM_NamedRoot *pnamedAliased,
                                  ACCESS access,
                                  _Inout_ SymbolList *psymbollist,
                                  bool DigThroughAliases = true);

    void GetImplements(Location *ploc,
                       _In_opt_z_ STRING *pstrName,
                       BCSYM *psym,
                       _Inout_ _Deref_prepost_valid_ BCSYM_Implements **p----lList);

    void GetVariable(const Location *ploc,
                     _In_z_ STRING *pstrName,
                     _In_z_ STRING *pstrEmittedName,
                     DECLFLAGS uFlags,
                     VARIABLEKIND varkind,
                     BCSYM *ptyp,
                     BCSYM_Expression *pconstexpr,
                     _Inout_ SymbolList *psymbollist,
                     _Inout_ BCSYM_Variable *pvariable);

    void GetProc(Location *ploc,
                 _In_z_ STRING *pstrName,
                 _In_z_ STRING *pstrEmittedName,
                 CodeBlockLocation* pCodeBlock,
                 CodeBlockLocation* pProcBlock,
                 DECLFLAGS uFlags,
                 BCSYM *ptyp,
                 BCSYM_Param *pparamList,
                 BCSYM_Param *pparamReturn,
                 _In_opt_z_ STRING *pstrLibName,
                 _In_opt_z_ STRING *pstrAliasName,
                 SyntheticKind sk,
                 _Inout_ BCSYM_GenericParam *pGenericParameters,
                 _Inout_ SymbolList *psymbollist,
                 _Inout_ BCSYM_Proc *pproc);

    BCSYM_Param *GetParam(Location *ploc,
                          _In_z_ STRING *pstrName,
                          BCSYM *ptyp,
                          PARAMFLAGS uFlags,
                          BCSYM_Expression *pconstexpr,
                          _Inout_opt_ BCSYM_Param **ppparamFirst,
                          _Inout_opt_ BCSYM_Param **ppparamLast,
                          bool isReturnType);

    static void SetClassAsStandardModule(_Out_ BCSYM_Class *pclass);

    void GetClass(Location *ploc,
                  _In_z_ STRING *pstrName,
                  _In_z_ STRING *pstrEmittedName,
                  _In_z_ STRING *pstrNameSpace,
                  CompilerFile *pfile,
                  BCSYM *ptypBase,
                  BCSYM_Implements *----lList,
                  DECLFLAGS DeclFlags,
                  Vtypes UnderlyingType,
                  _Out_opt_ BCSYM_Variable *pvarMe,
                  SymbolList *psymlistChildren,
                  SymbolList *psymlistUnBindableChildren,
                  _Inout_ BCSYM_GenericParam *pGenericParameters,
                  _Inout_ SymbolList *psymlist,
                  _Inout_ BCSYM_Class *pclass);

    void GetConditionalCompilationScope(_Out_ BCSYM_Container *pcont);

    BCSYM_Interface *GetInterface(Location *ploc,
                                  _In_z_ STRING *pstrName,
                                  _In_z_ STRING *pstrEmittedName,
                                  _In_z_ STRING *pstrNameSpace,
                                  CompilerFile *pfile,
                                  DECLFLAGS uFlags,
                                  BCSYM_Implements *----lList,
                                  SymbolList *psymlistChildren,
                                  SymbolList *psymlistUnBindableChildren,
                                  _Inout_ BCSYM_GenericParam *pGenericParameters,
                                  _Inout_opt_ SymbolList *psymlist,
                                  _Out_opt_ BCSYM_Interface *pAllocatedMemory = NULL );

    void GetProperty(Location *Location,
                     _In_z_ STRING *PropertyName,
                     DECLFLAGS PropertyFlags,
                     BCSYM_Proc *GetProperty,
                     BCSYM_Proc *SetProperty,
                     _Out_ BCSYM_Property *PropertySymbol,
                     _Inout_opt_ SymbolList *TheSymbolList );

    BCSYM_Hash *GetHashTable(_In_opt_z_ STRING *pstrName,
                             BCSYM_NamedRoot *pnamedParent,
                             bool SetParent,
                             unsigned cBuckets,
                             SymbolList *psymlistChildren);

    BCSYM_Namespace *GetNamespace(_In_z_ STRING *UnqualifiedName,
                                  _In_z_ STRING *ParentQualifiedName,
                                  BCSYM_NamespaceRing *NamespaceRing,
                                  CompilerFile *OwningFile,
                                  unsigned NumChildren,
                                  unsigned NumUnBindableChildren,
                                  Location *NamespaceLocation);

    BCSYM_NamespaceRing *GetNamespaceRing(_In_z_ STRING *QualifiedName,
                                          BCSYM_Hash *RingPool);

#if IDE 
    BCSYM_XmlName *GetXmlName( BCSYM_XmlNamespace *XmlNamespace,
                                    _In_z_ STRING *Name );
#endif

    BCSYM_XmlNamespaceDeclaration *GetXmlNamespaceDeclaration(_In_z_ STRING *Name,
                                            CompilerFile *OwningFile,
                                            CompilerProject *Project,
                                            Location *NamespaceLocation,
                                            bool IsImported );

    BCSYM_XmlNamespace *GetXmlNamespace(_In_z_ STRING *Name,
                                            CompilerFile *OwningFile,
                                            CompilerProject *Project);


    BCSYM_HandlesList *GetHandlesList( Location *FullyQualifiedNameLocation,
                                       _In_ Location *WithEventVarLocation,
                                       _In_ Location *EventNameLocation,
                                       _In_opt_z_ STRING   *WithEventsVarName,
                                       _In_opt_z_ STRING   *EventSourcePropertyName,
                                       _In_z_ STRING   *EventName,
                                       bool     HandlesMyBaseEvent,
                                       bool     HandlesMyClassEvent,
                                       bool     HandlesMeEvent,
                                       BCSYM_MethodImpl *HandlingMethod );

    BCSYM_ImplementsList *GetImplementsList(Location * ploc,
                                            BCSYM * ptyp,
                                            _In_z_ STRING * pstrProcName);

    BCSYM_ApplAttr *GetApplAttr(BCSYM_NamedType * pAttributeName, Location * ploc, bool IsAssembly, bool IsModule);

    BCSYM_GenericNonTypeConstraint *GetGenericNonTypeConstraint( Location *Location,
                                                                 BCSYM_GenericNonTypeConstraint::ConstraintKind ConstraintKind );

    BCSYM_GenericTypeConstraint *GetGenericTypeConstraint( Location *Location,
                                                           BCSYM *ConstraintType );

    BCSYM_GenericParam *GetGenericParam( Location *Location,
                                         _In_z_ STRING *Name,
                                         unsigned Position,
                                         bool IsGenericMethodParam,
                                         Variance_Kind Variance);

    // Assign a list of generic parameters to an owner.
    void SetGenericParams( _Out_opt_ BCSYM_GenericParam *ListOfGenericParameters,
                           BCSYM_NamedRoot *Owner );

    BCSYM_GenericBinding *GetGenericBinding
    (
        bool IsBad,
        BCSYM_NamedRoot *Generic,
        _In_count_(ArgumentCount) BCSYM *Arguments[],
        unsigned ArgumentCount,
        BCSYM_GenericTypeBinding *ParentBinding,
        bool AllocAndCopyArgumentsToNewList = false
    );

    // This only insantiate a generic type with one argument,
    // if need to take more than one argument, you can overload 
    // this function.
    BCSYM_GenericTypeBinding *GetGenericTypeInsantiation
    (
        BCSYM_NamedRoot *TypeConstructor,
        Type* ActualTypeArgument
    );

    // Dev10 #618745:
    // Return new or cached GenericBinding equivalent to the one passed in.
    // Used to deal with lifetime management for GenericBindings.
    BCSYM_GenericBinding *GetEquivalentGenericBinding
    (
        BCSYM_GenericBinding * pBinding
    );

    BCSYM_GenericBinding *GetGenericBindingWithLocation
    (
        bool IsBad,
        BCSYM_NamedRoot *Generic,
        _In_count_(ArgumentCount) BCSYM *Arguments[],
        unsigned ArgumentCount,
        BCSYM_GenericTypeBinding *ParentBinding,
        const Location *Loc,
        bool AllocAndCopyArgumentsToNewList = false
    );

    BCSYM_TypeForwarder *GetTypeForwarder( _In_z_ STRING *Name,
                                           _In_z_ STRING *Namespace,
                                           mdAssemblyRef tkDestAssemblyRef,
                                           _Inout_opt_ SymbolList *OwningSymbolList );

    BCSYM_ExtensionCallLookupResult  * GetExtensionCallLookupResult();

    //
    // Allocators for symbols that are constructed later.
    //

    BCSYM_CCContainer *AllocCCContainer(bool hasLocation, CompilerFile *CompilerFile )
    {
        BCSYM_CCContainer * Container = (BCSYM_CCContainer *)AllocSymbol(SYM_CCContainer, hasLocation, 0);
        Container->SetCompilerFile(CompilerFile);
        return Container;
    }

    BCSYM_Hash *AllocHash(unsigned NumOfBuckets)
    {
        if (VBMath::TryMultiply(NumOfBuckets, sizeof(BCSYM_NamedRoot *)))
        {
            return (BCSYM_Hash *)AllocSymbol(SYM_Hash, false, NumOfBuckets * sizeof(BCSYM_NamedRoot *));
        }

        return NULL;
    }

    BCSYM_Class *AllocClass(bool hasLocation)
    {
        return (BCSYM_Class *)AllocSymbol(SYM_Class, hasLocation, 0);
    }

    BCSYM_Interface *AllocInterface(bool hasLocation)
    {
        return (BCSYM_Interface *)AllocSymbol(SYM_Interface, hasLocation, 0);
    }

    BCSYM_MethodImpl *AllocMethodImpl(bool hasLocation)
    {
        return (BCSYM_MethodImpl *)AllocSymbol(SYM_MethodImpl, hasLocation, 0);
    }

    BCSYM_SyntheticMethod *AllocSyntheticMethod(bool hasLocation = false)
    {
        return (BCSYM_SyntheticMethod *)AllocSymbol(SYM_SyntheticMethod, hasLocation, 0);
    }

    BCSYM_MethodDecl *AllocMethodDecl(bool hasLocation)
    {
        return (BCSYM_MethodDecl *)AllocSymbol(SYM_MethodDecl, hasLocation, 0);
    }

    BCSYM_UserDefinedOperator *AllocUserDefinedOperator(bool hasLocation)
    {
        return (BCSYM_UserDefinedOperator *)AllocSymbol(SYM_UserDefinedOperator, hasLocation, 0);
    }

    Operator *
    LiftUserDefinedOperator
    (
        _In_ BCSYM_UserDefinedOperator * pSource,
        GenericTypeBinding * pGenericBindingContext,
        Semantics * pSemantics,
        CompilerHost * pCompilerHost
    );


    BCSYM_EventDecl *AllocEventDecl(bool hasLocation)
    {
        return (BCSYM_EventDecl *)AllocSymbol(SYM_EventDecl, hasLocation, 0);
    }

    BCSYM_DllDeclare *AllocDllDeclare(bool hasLocation)
    {
        return (BCSYM_DllDeclare *)AllocSymbol(SYM_DllDeclare, hasLocation, 0);
    }

    BCSYM_Variable *AllocVariable(bool hasLocation, bool hasValue)
    {
        VSASSERT(sizeof(BCSYM_Variable) == sizeof(BCSYM_VariableWithValue), "have to be equivalent!");
        BilKind bkind = hasValue ? SYM_VariableWithValue : SYM_Variable;
        return (BCSYM_Variable *)AllocSymbol(bkind, hasLocation, 0);
    }

    BCSYM_Param *AllocParameter(bool hasLocation, bool hasValue)
    {
        VSASSERT(sizeof(BCSYM_Param) == sizeof(BCSYM_ParamWithValue), "have to be equivalent!");
        BilKind bkind = hasValue ? SYM_ParamWithValue : SYM_Param;
        return (BCSYM_Param *)AllocSymbol(bkind, hasLocation, 0);
    }

    BCSYM_Variable *AllocVariableWithArraySizes(unsigned Rank)
    {
        if (VBMath::TryMultiply(Rank, sizeof(BCSYM_Expression *)))
        {
            return (BCSYM_Variable *)AllocSymbol(SYM_VariableWithArraySizes, true, Rank * sizeof(BCSYM_Expression *));
        }

        return NULL;
    }

    BCSYM_StaticLocalBackingField *AllocStaticLocalBackingField(bool hasLocation)
    {
        return (BCSYM_StaticLocalBackingField *)AllocSymbol(SYM_StaticLocalBackingField, hasLocation, 0);
    }

    BCSYM_CCConstant *AllocCCConstant()
    {
        return (BCSYM_CCConstant *)AllocSymbol(SYM_CCConstant, true, 0);
    }

    BCSYM_Property *AllocProperty(bool hasLocation)
    {
        return (BCSYM_Property *)AllocSymbol( SYM_Property, hasLocation, 0);
    }

    //
    // Allocates an ImportsTarget Symbol.
    //
    ImportedTarget *AllocImportedTarget(unsigned NumDotDelimitedNames);

    //
    // Setters used from outside of Symbols.
    //

    // Add a symbol to a hash table.
    static void AddSymbolListToHash(
            _Inout_ BCSYM_Hash *phash,
            _In_opt_ SymbolList *psymlist,
            bool SetParent);
    static void AddSymbolToHash(
            _Inout_ BCSYM_Hash *phash,
            _Inout_ BCSYM_NamedRoot *pnamed,
            bool SetParent,
            bool ContainerIsModule,
            bool ContainerIsNamespace);

    // Remove a symbol from a hash table
    static BCSYM_NamedRoot * RemoveSymbolFromHash( _Inout_ BCSYM_Hash *Hash, BCSYM_NamedRoot *RemoveMe );

    // Set the information into a container that allows us to delay load it.
    static void SetContainerDelayLoadInformation(_Inout_ BCSYM_Container *pcontainer,
                                                 mdTypeDef td,
                                                 mdTypeRef tkExtends,
                                                 MetaDataFile *pMetaDataFile);

    static void SetDelayBindNamespace
    (
        BCSYM_Namespace *Namespace
    );

    static void MorphClassToDelegate(BCSYM_Class *pclass);
    static void MorphVariableToConst(_Out_ BCSYM_Variable *Variable, BCSYM_Expression *Value);
    static void MorphVariableToVariableWithValue(_Out_ BCSYM_Variable *Variable, BCSYM_Expression *Value);
    static void MorphParamToOptional(_Out_ BCSYM_Param *Param, BCSYM_Expression *Value);

    // If you need to change a symbol's parent, call ClearParent first.  This is a clear
    // sign that you are going to overwrite the symbol's parent.  (The assert in SetParent
    // is too useful to remove.  It catches weird cases where symbols are accidentally
    // re-parented.)
    static void ClearParent(BCSYM_NamedRoot *pnamed)
    {
        pnamed->SetImmediateParent(NULL);
    }

    static void SetParent(BCSYM_NamedRoot *pnamed, BCSYM_NamedRoot *pnamedParent)
    {
        VSASSERT(pnamed->GetImmediateParent() == NULL || pnamed->GetImmediateParent() == pnamedParent, "Illegal attempt to change symbol's parent.");
        pnamed->SetImmediateParent(pnamedParent);
    }

    static void SetBaseClass(BCSYM_Class* DerivedClass, BCSYM *NewBaseClass)
    {
        // This assert is a little weird.  We can in fact legally change a symbol's base class to Object or GenericBadNamedRoot.
        VSASSERT(DerivedClass->GetRawBase() == NULL || DerivedClass->GetRawBase() == NewBaseClass || (NewBaseClass != NULL && ( NewBaseClass->IsObject() || NewBaseClass->PNamedRoot()->IsBadNamedRoot())),
                 "Illegal attempt to change symbol's base class.");

        if (DerivedClass->GetRawBase() && DerivedClass->GetRawBase()->IsNamedType())
        {
            DerivedClass->GetRawBase()->PNamedType()->SetSymbol(NewBaseClass);
        }
        else
        {
            DerivedClass->SetRawBase(NewBaseClass);
        }

        DerivedClass->SetBaseInvolvedInCycle(false); // we figure out if this isn't the case in bindable
    }

    static void SetNext(BCSYM_NamedRoot *pnamed, BCSYM_NamedRoot *pnamedParent)
    {
        pnamed->SetNextInSymbolList(pnamedParent);
    }

    static void SetChildrenLoaded(BCSYM_Container *pcontainer)
    {
        // If the children are loaded, then this symbol must have a hash table.
        // BCSYM_Containers have a union with two members:
        //
        //  union
        //  {
        //      BCSYM_Hash *m_phash;
        //      BCSYM_Container *m_pNestedTypeList;
        //  };
        //
        // Before the children are done loading, m_pNestedTypeList is the
        // valid part of the union.  Once the container is delay loaded,
        // though, the container transforms the list into a hash table and
        // m_phash becomes the valid part of the union.
        //
        // This means we can assert that we have a hash table here.

        VSASSERT(pcontainer->GetHashRaw()->IsHash(), "m_ChildrenNotLoaded and the container's union are out of sync!");
        pcontainer->SetChildrenNotLoaded(false);
    }

    static void SetChildrenLoading(BCSYM_Container *pcontainer, bool isLoading)
    {
        pcontainer->SetChildrenLoading(isLoading);
    }

    static void SetBaseAndImplementsLoaded(BCSYM_Container *pcontainer)
    {
        pcontainer->SetBaseAndImplementsNotLoaded(false);
    }

    static void SetBaseAndImplementsLoading(BCSYM_Container *pcontainer, bool isLoading)
    {
        pcontainer->SetBaseAndImplementsLoading(isLoading);
    }

    static void SetToken(BCSYM_NamedRoot *pnamed, mdToken tk)
    {
        pnamed->SetToken(tk);
        pnamed->SetAttributesEmitted(false);
    }

    static void SetToken(BCSYM_Param *pparam, mdToken tk)
    {
        pparam->SetToken(tk);
        pparam->SetAreAttributesEmitted(false);
    }

    static void SetComClassToken(BCSYM_NamedRoot *pnamed, mdToken tk)
    {
        pnamed->SetComClassToken(tk);
    }

    static void SetComClassToken(BCSYM_Param *pparam, mdToken tk)
    {
        pparam->SetComClassToken(tk);
    }

    static void SetComClassEventsToken(BCSYM_Class *pclass, mdToken tk)
    {
        pclass->SetComClassEventsToken(tk);
    }

    static void SetAttributesEmitted(BCSYM_NamedRoot *pnamed)
    {
        pnamed->SetAttributesEmitted(true);
    }

    static void SetAttributesEmitted(BCSYM_Param *pparam)
    {
        pparam->SetAreAttributesEmitted(true);
    }

    static void SetNextParam(BCSYM_Param *pParam, BCSYM_Param *pNext)
    {
        pParam->SetNext(pNext);
    }

    static void SetNextExpression(BCSYM_Expression *pexpr, BCSYM_Expression *pexprNext)
    {
        pexpr->SetNext(pexprNext);
    }

    static void SetName(BCSYM_NamedRoot *pnamed, _In_z_ STRING *pstrName)
    {
        // It's legal to change the casing of the name, so we need to use IsEqual
        VSASSERT(pnamed->GetName() == NULL || StringPool::IsEqual(pnamed->GetName(), pstrName),
            "Illegal attempt to change symbol's name.");

        pnamed->SetName(pstrName);
    }

    static void SetEmittedName(BCSYM_NamedRoot *pnamed, _In_z_ STRING *pstrEmittedName)
    {
        // It's legal to change the casing of the emitted name, so we need to use IsEqual
        VSASSERT(pnamed->GetEmittedName() == NULL || StringPool::IsEqual(pnamed->GetEmittedName(), pstrEmittedName),
            "Illegal attempt to change symbol's emitted name.");

        pnamed->SetEmittedName(pstrEmittedName);
    }

    static void SetDefaultProperty(BCSYM_Container *pcontainer, BCSYM_Property *pprop)
    {
        pcontainer->SetDefaultProperty(pprop);
    }

    static void SetOverloads(BCSYM_Proc *pproc)
    {
        pproc->SetIsOverloads(true);
    }

    static void SetIsDefault(BCSYM_Property *pprop)
    {
        pprop->SetIsDefault(true);
    }

    static void SetHandlesAndImplLists(BCSYM_MethodImpl * pmeth, BCSYM_HandlesList * phandles, BCSYM_ImplementsList * ----lList)
    {
        pmeth->SetHandlesList(phandles);
        pmeth->SetImplementsList(----lList);
    }

    static void SetIsPropertyGet(BCSYM_Proc *pproc)
    {
        pproc->SetIsPropertyGet(true);
    }

    static void SetIsPropertySet(BCSYM_Proc *pproc)
    {
        pproc->SetIsPropertySet(true);
    }

    static void SetIsImplementingComClassInterface(BCSYM_Proc *pproc)
    {
        pproc->SetIsImplementingComClassInterface(true);
    }

    static void ResetOverloadsAndOverrides(BCSYM_Proc *pproc)
    {
        pproc->SetIsOverloads(false);
        pproc->SetOverriddenProc(NULL);
        pproc->SetOverridesRequiresMethodImpl(false);
    }

    static void ClearOverloadsOverridesKeyword( BCSYM_Proc *Proc );
    static void ResetFlags(BCSYM_NamedRoot *pnamed, CompilationState State);
    static void ClearShadowsBit( BCSYM_NamedRoot *NamedRoot );

    static void ProliferateBindingSpace(BCSYM_Alias *palias)
    {
        palias->SetBindingSpace(palias->GetSymbol()->PNamedRoot()->GetBindingSpace());
    }

    static void MakeUnbindable(BCSYM_NamedRoot *pRoot)
    {
        if (pRoot)
        {
            pRoot->SetBindingSpace(BINDSPACE_IgnoreSymbol);
        }
    }

    static void SetCode(BCSYM_SyntheticMethod *pmeth, _In_opt_count_(cchCode) const WCHAR *wszCode, unsigned cchCode)
    {
        pmeth->SetCode(wszCode);
        pmeth->SetCodeSize(cchCode);
    }

    static void SetParsedCode(BCSYM_SyntheticMethod *pmeth, ParseTree::StatementList* pParsedCode, unsigned LocalsCount)
    {
        pmeth->SetParsedCode(pParsedCode);
        pmeth->SetLocalsCount(LocalsCount);
    }

    static void SetType(BCSYM_Member *pMember, BCSYM *pType)
    {
        pMember->SetType(pType);
    }

    static void SetParamType(BCSYM_Param *pParam, BCSYM *pType)
    {
        pParam->SetType(pType);
    }

    static PARAMFLAGS CalculateInitialParamFlags(BCSYM_Param *pParam)
    {
        PARAMFLAGS flags = 0;

        if ( pParam->IsParamArray() )
        {
            flags |= PARAMF_ParamArray;
        }
        if ( pParam->IsOptional() )
        {
            flags |= PARAMF_Optional;
        }
        if ( pParam->IsMarshaledAsObject() )
        {
            flags |= PARAMF_MarshalAsObject;
        }
        if ( pParam->IsByRefKeywordUsed() )
        {
            flags |= PARAMF_ByRef;
        }
        else
        {
            flags |= PARAMF_ByVal;
        }

        return flags;
    }

    static void SetParamList(BCSYM_Proc *pProc, BCSYM_Param *pParam)
    {
        pProc->SetFirstParam(pParam);
    }

    static void SetImplements(BCSYM_Container *Type, BCSYM_Implements *Implements)
    {
        if (Type->IsClass())
        {
            Type->PClass()->SetImpList(Implements);
        }
        else
        {
            Type->PInterface()->SetImpList(Implements);
        }
    }

// ******** IDE Only ********

#if IDE 

    static void SetImage(BCSYM_Proc *Proc, BYTE *Image, unsigned ImageSize)
    {
        if (Proc->IsMethodImpl())
        {
            BCSYM_MethodImpl *Method = Proc->PMethodImpl();
            Method->SetImage(Image,ImageSize);
        }
        else
        {
            BCSYM_SyntheticMethod *Method = Proc->PSyntheticMethod();
            Method->SetImage(Image,ImageSize);
        }
    }
#endif IDE

    //
    // Helpers.
    //

    // Convert DECLFLAGS to ACCESS
    static ACCESS AccessOfFlags(DECLFLAGS uFlags);

    // Convert ACCESS to DECLFLAGS
    static DECLFLAGS MapAccessToDeclFlags( ACCESS Access );

#if DEBUG
    static void DebShowSymbols(BCSYM *psym, Compiler *pCompiler, unsigned cchIndent = 0);
    static void DebShowBasicRep(Compiler *pCompiler, BCSYM *psym, unsigned cchIndent);
#endif

    GenericBindingCache * GetGenericBindingCache()
    {
        return m_GenericBindingCache;
    }

    void SetGenericBindingCache(GenericBindingCache *Cache)
    {
        m_GenericBindingCache = Cache;
    }

    bool AreBindingsCached()
    {
        return m_GenericBindingCache != NULL;
    }

#if 0
    // Avoid modifying the symbol table for the following reasons:
    //  - Mainly in order to avoid threading issue when the foreground thread is
    //      accessing the symbol table.
    //  - An additional benefit from not modifying the symbol table, there are
    //      no complex decompilation issues to deal with.
    //
    void InsertTransientClassInSymbolTable(BCSYM_Class *pClass);
    static void RemoveTransientClassFromSymbolTable(BCSYM_Class *pClass);
    static void InsertTransientMemberInSymbolTable(BCSYM_NamedRoot *pMember);
    static void RemoveTransientMemberFromSymbolTable(BCSYM_NamedRoot *pMember);
    void InsertTransientSymbolInSymbolTable(BCSYM_NamedRoot *pSymbol);
    static void RemoveTransientSymbolFromSymbolTable(BCSYM_NamedRoot *pSymbol);
#endif 0

    BCSYM *
    LiftType
    (
        BCSYM * pType,
        CompilerHost * pCompilerHost
    );

private:


    BCSYM_Param *
    LiftParameter
    (
        _In_opt_ BCSYM_Param * pParam,
        CompilerHost * pCompilerHost
    );

    BCSYM_Param *
    CloneParameter
    (
        _In_opt_ BCSYM_Param * pParam,
        CompilerHost * pCompilerHost
    );

    BCSYM_Param *
    LiftOperatorParameters
    (
        BCSYM_Param * pFirstParam,
        GenericTypeBinding * pGenericBindingContext,
        Semantics * pSemantics,        
        CompilerHost * pCompilerHost
    );

    BCSYM_LiftedOperatorMethod * CreateLiftedOperatorImplementation
    (
        BCSYM_UserDefinedOperator * liftedOperator,
        _In_ BCSYM_MethodDecl * associatedMethodOfSource,
        CompilerHost * pCompilerHost
    );

    BCSYM *AllocSymbol(BilKind bkind, bool hasLocation, size_t cbExtra);

    BCSYM_GenericBinding *BuildGenericBinding
    (
        bool IsBad,
        BCSYM_NamedRoot *Generic,
        _In_count_(ArgumentCount) BCSYM *Arguments[],
        unsigned ArgumentCount,
        BCSYM_GenericTypeBinding *ParentBinding,
        const Location *Loc,
        bool AllocAndCopyArgumentsToNewList
    );

    //
    // Data members.
    //

    // Context.
    Compiler *m_CompilerInstance;

    // Allocator to put the symbols into.
    NorlsAllocator *m_Allocator;

    // Line marker table for symbols.
    LineMarkerTable *m_LineMarkerTable;

    // The generic binding caches in which the cache bindings.
    GenericBindingCache *m_GenericBindingCache;

#if DEBUG
    static void DebShowSymbolReference(Compiler *pCompiler, BCSYM *psym, unsigned cchIndent = 0);
    static void DebShowNamedRootFlags(BCSYM_NamedRoot *psym);
    static void DebShowProcFlags(BCSYM_Proc *psym);
    static void DebShowProc(Compiler *pCompiler, BCSYM_Proc *psym, unsigned cchIndent);
    static void DebShowVarFlags(BCSYM_Variable *psym);
#endif  // DEBUG

}; // Symbols

class GenericBindingCache
{
    // 


#define CACHE_HASH_TABLE_SIZE 256

  public:

    GenericBindingCache(NorlsAllocator *Allocator)
    {
        m_Allocator = Allocator;
        m_Hash = NULL;
    }

    void Add(unsigned long Key, BCSYM_GenericBinding *Binding);

    BCSYM_GenericBinding* Find
    (
        unsigned long Key,
        BCSYM_NamedRoot *Generic,
        _In_count_(ArgumentCount) BCSYM *Arguments[],
        unsigned ArgumentCount,
        BCSYM_GenericTypeBinding *ParentBinding
    );

    static bool ComputeKeyForGenericBinding
    (
        BCSYM_NamedRoot *Generic,
        _In_count_(ArgumentCount) BCSYM *Arguments[],
        unsigned ArgumentCount,
        BCSYM_GenericTypeBinding *ParentBinding,
        _Out_ unsigned long &BindingSignature
    );

    void ClearCache()
    {
        m_Hash = NULL;
    }

  private:
#if _WIN64
    typedef FixedSizeHashTable<CACHE_HASH_TABLE_SIZE, size_t, BCSYM_GenericBinding *> HashForGenericBindingCache;
#else
    typedef FixedSizeHashTable<CACHE_HASH_TABLE_SIZE, unsigned long, BCSYM_GenericBinding *> HashForGenericBindingCache;
#endif

    NorlsAllocator *m_Allocator;
    HashForGenericBindingCache *m_Hash;


#undef CACHE_HASH_TABLE_SIZE
};

class BackupGenericBindingCache :
    public BackupState<Symbols *, GenericBindingCache *>
{
public:
    BackupGenericBindingCache(Symbols * pSymbols) :
        BackupState(pSymbols, ExtractSavedValue(pSymbols))
    {
    }

    ~BackupGenericBindingCache()
    {
        Restore();
    }
protected:
    void DoRestore()
    {
        if (m_constructorArg)
        {
            m_constructorArg->SetGenericBindingCache(m_savedValue);
        }
    }
private:
    GenericBindingCache * ExtractSavedValue(Symbols * pSymbols)
    {
        return pSymbols ? pSymbols->GetGenericBindingCache() : NULL;
    }
};


BCSYM *DigThroughNamedTypeIfPossible(BCSYM *PossibleNamedType);
