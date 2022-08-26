//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to move a module from Declared state to Bindable state.
//
//-------------------------------------------------------------------------------------------------

#pragma once

class Compiler;
class LineMarkerTable;
struct SymbolList;
struct BINDER_REFINFO;
struct ImportedTarget;

typedef unsigned __int64 TypeResolutionFlags;
typedef HashTable<32> SHADOWS_HASH_TABLE;
typedef HashTable<256> PROC_HASH_TABLE;

// TRUE if all the flags specified by the mask are set in testMe - no others may be set
#define ONLY_THESE_FLAGS_ARE_SET( mask, testMe ) (!(( testMe ) & ~( mask )))

#define CLS_PROPERTY_SET_PREFIX  WIDE( "set_" )
#define CLS_PROPERTY_SET_PREFIX_LEN 4 // character len of CLS_PROPERTY_SET_PREFIX
#define CLS_PROPERTY_GET_PREFIX  WIDE( "get_" )
#define CLS_PROPERTY_GET_PREFIX_LEN 4 // character len of CLS_PROPERTY_GET_PREFIX
#define CLS_PROPERTY_PUT_PREFIX  WIDE( "put_" )
#define CLS_PROPERTY_PUT_PREFIX_LEN 4 // character len of CLS_PROPERTY_PUT_PREFIX

#define CLS_MYGROUPCOLLECTION_FIELD_PREFIX  WIDE( "m_" )
#define CLS_MYGROUPCOLLECTION_FIELD_PREFIX_LEN 2 // character len of CLS_MYGROUPCOLLECTION_FIELD_PREFIX

//
// You must use these defines - otherwise you risk breaking the string offsets used when generating synthetic code
// These defines assure that the correct indexes can be used to find event names, etc. for synth code.
//

#define ATTACH_LISTENER_PREFIX WIDE( "add_" )
#define ATTACH_LISTENER_PREFIX_LEN 4 // the # of chars in ATTACH_LISTENER_PREFIX

#define REMOVE_LISTENER_PREFIX WIDE( "remove_" )
#define REMOVE_LISTENER_PREFIX_LEN 7 // the # of chars in REMOVE_LISTENER_PREFIX

#define FIRE_LISTENER_PREFIX WIDE( "raise_" )
#define FIRE_LISTENER_PREFIX_LEN 6 // the # of chars in FIRE_LISTENER_PREFIX

#define EVENT_DELEGATE_SUFFIX WIDE( "EventHandler" )
#define EVENT_DELEGATE_SUFFIX_LEN 12 // the # of chars in EVENT_DELEGATE_SUFFIX

#define EVENT_VARIABLE_SUFFIX WIDE( "Event" )
#define EVENT_VARIABLE_SUFFIX_LEN 5 // the # of chars in EVENT_VARIABLE_SUFFIX

#define MAXIMUM_EVENT_PREFIX_LEN         12  // The maximum number of characters we tack onto an event name ("EventHandler")
#define MAXIMUM_WITHEVENTS_PREFIX_LEN    4   // The maximum number of characters we tack onto a withevents name ("set_" and "get_")

// Macro to iterate over all the symbols with attributes in a container (including the partial
// containers)
#define Begin_ForAllSymbolsInContainerWithAttrs(SymbolHavingAttrs, Container, FunctionToGetHeadOfList) \
   { \
        BCSYM_Container *SymbolsContainer = Container; \
        do \
        { \
            for(BCSYM *SymbolHavingAttrs = SymbolsContainer->FunctionToGetHeadOfList(); \
                SymbolHavingAttrs; \
                SymbolHavingAttrs = SymbolHavingAttrs->GetPAttrVals()->GetPsymNextSymbolWithApplAttr()) \
            {

// end of Macro to get symbols with attributes in a container
#define End_ForAllSymbolsInContainerWithAttrs \
            } \
        } while(SymbolsContainer = SymbolsContainer->GetNextPartialType()); \
   }


typedef unsigned __int64 NameFlags;

// compare function which helps to sort mustoverride members when reporting errors
int __cdecl SortSymbolsByBaseClassesAndLocation(const void *arg1, const void *arg2);


//-------------------------------------------------------------------------------------------------
//
// A simple structure to assist in verifying the implemented interfaces.
// We create an array of these things to try to figure out exactly what needs
// to be implemented on this class and what's been taken care of by our
// base classes.
//
struct ImplementsInfo
{
    BCSYM *m_Interface; // The interfaces being implemented.
    enum WhereImplementedFrom // How is this interface implemented?
    {
        ImplementedOnBaseClass, // The interface is implemented by one of our base classes (directly or indirectly)
        DirectlyImplemented,    // The class directly implements the interface
        BaseInterface          // The interface is a base interface of an interface a class implements
    } m_WhereFrom;

    // What "implements" clause did this come from?  This will be NULL if we got this from a base class unless it is
    // also indirectly implemented then this will be set so we can give a good error.
    BCSYM_Implements *m_ImplementsClause;
    BCSYM *m_Class; // The class that implements this interface
    BCSYM *m_OriginalImplementer; // The base class that originally implemented this interface
    bool m_IsBad : 1; // Do we have an error?

#if 0
    // 


    bool m_IsReimplOnlyForSomeTypeArgs : 1; // Is the current reimplementation only a reimplementation for some type arguments ?
#endif

    void SetOriginalImplementer(BCSYM *BaseClass, bool fIsReimplOnlyForSomeTypeArgs)
    {
        m_OriginalImplementer = BaseClass;
        //m_IsReimplOnlyForSomeTypeArgs = fIsReimplOnlyForSomeTypeArgs;
    }

    BCSYM* GetOriginalImplementer()
    {
        return m_OriginalImplementer;
    }

    bool IsReimplementation()
    {
        return m_OriginalImplementer;
    }

#if 0
    // 


    bool IsReimplOnlyForSomeTypeArgs()
    {
        return m_IsReimplOnlyForSomeTypeArgs;
    }
#endif

};

// This is used to store information during structure member cycle detection
//
struct StructCycleNode
{
    BCSYM *m_Struct;                        // Struct or struct generic binding
    BCSYM_Variable *m_Field;                // Field involved in the cycle

    StructCycleNode *m_Next;               // link to next node involved in the cycle
};

/*****************************************************************************
;Bindable

Steps a file from Declared state to Bindable state by binding named types, resolving
constant expressions, check for proper inheritance semantics ( overloading, overriding,
not exposing restricted types, etc ), hook up implements, hook up handles, verifying that classes,
records, and enumerations are well formed, etc.
*****************************************************************************/
class Bindable
{
    friend class BCSYM_Container;
    friend class BCSYM;
    friend class Semantics;

    friend Vtypes BCSYM_Class::GetVtype();
    friend BCSYM * BCSYM::DigThroughNamedType();

    // forward declarations:
    enum SyntheticContext;
    struct BindableMemberInfo;
    struct MemberInfoIter;
    struct MembersHashTable;
    enum ShadowingOverloadingOverridingInfo;
    struct DelayedCheckManager;
    class RelationalOperatorPool;
    struct MemberBindingHashTable;

    public:

    enum TaskTriState;

    //========================================================================
    // These entrypoints are common code shared by the compilation code,
    // the metadata importer and the locals code.
    //========================================================================

    // Resolves all of the imports clauses.  Used for binding.
    static void
    ResolveImportsList
    (
        ImportedTarget *pImportsList,
        SourceFile *psrcfile,
        CompilerProject *pProject,
        NorlsAllocator *pnraSymbols,
        LineMarkerTable* plmt,
        ErrorTable *perrors,
        Compiler *CompilerInstance,
        CompilationCaches *IDECompilationCaches
    );

    static void
    ResolveImports
    (
        BCSYM_Namespace *Namespace,
        SourceFile *SourceFile,
        CompilerProject *Project,
        NorlsAllocator *Allocator,
        LineMarkerTable* LineMarkerTbl,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance,
        CompilationCaches *IDECompilationCaches
    );

    // Resolves each NamedType symbol in the specified list - This is a helper for MakeBindable and for building up the local variables of a function.
    static void
    ResolveAllNamedTypes
    (
        BCSYM_NamedType *pnamtypList,
        NorlsAllocator *pnraSymbols,
        ErrorTable *perrors,
        Compiler *CompilerInstance,
        CompilerHost *CompilerHost,
        SourceFile *pSourceFile,
        TypeResolutionFlags Flags,
        CompilationCaches *IDECompilationCaches
    );

    // Processes each constant declaration and makes sure that each has a value.
    static void
    EvaluateConstantDeclarations
    (
        BCSYM_Expression *pexprList,
        NorlsAllocator *pSymbolStorage,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance,
        CompilerHost *CompilerHost,
        CompilationCaches *IDECompilationCaches
    );

    // Will step a declared expression to Evaluated
    static void
    EvaluateDeclaredExpression
    (
        BCSYM_Expression * pexpr,
        NorlsAllocator * pTreeStorage,
        NorlsAllocator * pSymbolStorage,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance,
        CompilerHost *CompilerHost,
        CompilationCaches *IDECompilationCaches
    );

    static void
    CheckAllGenericConstraints
    (
        BCSYM_NamedType *NamedTypeList,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches
    );

    // Method to call to completely take all containers in a SourceFile through Bindable
    static void
    BindSourceFile
    (
        SourceFile *SourceFile,
        CompilationCaches *IDECompilationCaches
    );

    // Method to call to completely take a Container through Bindable
    static void
    BindContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    // this is an entry point for BCSYM_Class And BCSYM_Interface to Resolve bases on demand
    static void
    ResolveBasesIfNotStarted
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static TaskTriState
    GetStatusOfResolveBases
    (
        BCSYM_Container *Container
    );

    void
    SetCurrentBase(BCSYM_NamedType *BaseType)
    {
        m_CurrentBase = BaseType;
    }

    BCSYM_NamedType *
    GetCurrentBase()
    {
        return m_CurrentBase;
    }

    // Public to allow obsolete checker to ---- attributes on symbols on demand
    static void
    ----AttributesOnAllSymbolsInContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    // Entry point for Obsoletechecker
    // IsContainerReadyForObsoleteChecking here will only return true
    // if the current Container Context's Attributes have already been ----ed.
    // This is a helper for obsolete checker to avoid circular dependencies when
    // doing obsolete checking and attribute cracking might be required on other
    // containers on demand.when doing name binding for earlier stages
#if DEBUG
    static bool
    IsContainerReadyForObsoleteChecking
    (
        BCSYM_Container *Container
    );
#endif DEBUG

    void ValidateExtensionAttributeOnMetaDataClass(BCSYM_Class * pClass);

    static void
    BindMetaDataContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

#if IDE 
    static void
    UnBindSourceFile
    (
        SourceFile *SourceFile,
        bool UnBindContainers
    );

    static void
    ClearProjectAssemblyAttrInfoDueToFile
    (
        SourceFile *SourceFile
    );

#endif IDE

    static void
    ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    // Useful helper outside bindable too
    static bool
    IsSynthetic
    (
        BCSYM_NamedRoot *Member,
        BCSYM_NamedRoot **SourceOfMember = NULL,
        BCSYM_NamedRoot **NamedContextOfMember = NULL,
        SyntheticContext *SyntheticContext = NULL
    );

    static BCSYM_Proc *
    GetPublicParameterlessConstructor
    (
        BCSYM_Class *Class,
        Compiler    *CompilerInstance
    );

    // Used by semantics too
    static bool
    IsTypeNestedIn
    (
        BCSYM_NamedRoot *ProbableNestedType,
        BCSYM_Container *ProbableEnclosingContainer
    );

    // static void
    //GenSyntheticMyGroupCollectionProperties
    //(
    //    BCSYM_Class * Class,
    //    CompilerProject* project
    //);

    static void
    GenSyntheticMyGroupCollectionProperties
    (
        MyGroupCollectionInfo* pGroupInfo,
        CompilerProject* project
    );

    static bool
    IsMyGroupCollection
    (
        BCSYM_Container *Container
    );

    static bool
    ValidateGenericConstraintsOnPartiallyBoundExtensionMethod
    (
        BCSYM_Proc * pProc,
        PartialGenericBinding * pPartialGenericBinding,
        Symbols * pSymbols,
        CompilerHost * pCompilerHost
    );

    // Work horse for resolving a named type
    static void
    ResolveNamedType
    (
        BCSYM_NamedType * pNamedType,
        NorlsAllocator *,
        ErrorTable *,
        Compiler*,
        CompilerHost*,
        SourceFile*,
        TypeResolutionFlags,
        CompilationCaches*,
        NameFlags NameInterpretFlags,
        bool DisableNameLookupCaching
    );

private:

    struct InterfaceMemberInfo
    {
        BCSYM_NamedRoot *Method; // the method itself
        STRING *SpecifiedInterface; // the method the user specified
    };

    static BCSYM_GenericBinding *
    ResolveGenericType
    (
        BCSYM_NamedRoot *GenericType,
        NameTypeArguments *Arguments,
        BCSYM_GenericTypeBinding *ParentBinding,
        ErrorTable *,
        Compiler*,
        Symbols *SymbolFactory
    );

    static void
    Bindable::ValidateArity
    (
        _In_z_ STRING *Name,
        BCSYM_NamedRoot *Generic,
        BCSYM_GenericTypeBinding *ParentBinding,
        unsigned ArgumentCount,
        Location *Loc,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance,
        bool &IsBad
    );

    static bool
    ResolveGenericArguments
    (
        BCSYM *UnboundGeneric,
        BCSYM_GenericParam *Parameters,
        NameTypeArguments *ArgumentsInfo,
        ErrorTable *ErrorLog,          // [in] the error log to put our errors into - NULL if we are importing metadata
        Compiler *CompilerInstance     // [in] the current context
    );

    static void
    CheckGenericConstraints
    (
        BCSYM_NamedType *NamedType,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches,
        bool CheckForArgumentBindingsToo = false
    );

    static void
    CheckGenericConstraintsForBindingsInTypeArguments
    (
        NameTypeArguments *TypeArguments,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches
    );

    static bool
    CheckGenericConstraints
    (
        BCSYM_GenericBinding *Binding,
        Location TypeArgumentLocations[],               // Only one of TypeArgumentLocations and TypeArgumentsLocationHolder can be passed in
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches
    );

    static void
    CheckGenericConstraints
    (
        BCSYM *Type,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches,
        bool CheckForArgumentBindingsToo = false
    );

    static bool
    ValidateConstraintOnPartiallyBoundExtensionMethod
    (
        BCSYM_Proc * pProc,
        PartialGenericBinding * pPartialGenericBinding,
        GenericConstraint * pConstraint,
        GenericParameter * pParameter,
        Type * pTypeParameterValue,
        Symbols * pSymbols,
        CompilerHost * pCompilerHost
    );

    static bool
    ValidateTypeConstraintOnPartiallyBoundExtensionMethod
    (
        Procedure * pProc,
        PartialGenericBinding * pPartialGenericBinding,
        GenericTypeConstraint * pConstraint,
        Type * pTypeParameterValue,
        Symbols * pSymbols,
        CompilerHost * pCompilerHost
    );


    static bool
    ValidateGenericConstraintsForType
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        BCSYM_GenericBinding *Binding,
        Location TypeArgumentLocations[],
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        bool IgnoreProjectEquivalence = false,              // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForType = NULL,            // The project containing a component of type "Type" that was considered
                                                            // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForGenericParam = NULL     // The project containing a component of type "Parameter" that was considered
                                                            // for a successful match ignoring project equivalence.
    );

    static bool
    ValidateTypeConstraintForType
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        BCSYM_GenericTypeConstraint *Constraint,
        BCSYM_GenericBinding *Binding,
        Location TypeArgumentLocations[],
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        bool IgnoreProjectEquivalence = false,              // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForType = NULL,            // The project containing a component of type "Type" that was considered
                                                            // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForConstraint = NULL       // The project containing a component of "Constraint"s type that was considered
                                                            // for a successful match ignoring project equivalence.
    );

    static bool
    ValidateTypeConstraintForType
    (
        BCSYM *Type,
        BCSYM *ConstraintType,
        CompilerHost *CompilerHostInstance,
        Symbols *SymbolFactory,
        bool IgnoreProjectEquivalence = false,              // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForType = NULL,            // The project containing a component of type "Type" that was considered
                                                            // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForConstraintType = NULL   // The project containing a component of type "ConstraintType" that was considered
                                                            // for a successful match ignoring project equivalence.
    );

    static bool
    ValidateNewConstraintForType
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        Location TypeArgumentLocations[],
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance
    );

    static bool
    ValidateReferenceConstraintForType
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        Location TypeArgumentLocations[],
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog
    );

    static bool
    ValidateValueConstraintForType
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        Location TypeArgumentLocations[],
        NameTypeArguments *TypeArgumentsLocationHolder,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        bool isNullableValue
    );

    static Location*
    GetTypeArgumentLocation
    (
        BCSYM_GenericParam *Parameter,
        NameTypeArguments *TypeArgumentsLocationHolder,
        Location TypeArgumentLocations[]
    );

  public:
    static void
    CheckGenericConstraintsForImportsTarget
    (
        ImportedTarget *ImportsTarget,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        CompilationCaches *IDECompilationCaches
    );

    static bool
    DoesTypeSatisfyGenericConstraints
    (
        BCSYM *Type,
        BCSYM_GenericParam *Parameter,
        BCSYM_GenericBinding *Binding,
        CompilerHost *CompilerHostInstance,
        Compiler *CompilerInstance,
        Symbols *SymbolFactory,
        bool IgnoreProjectEquivalence = false,              // Consider types as equivalent ignoring their projects' equivalence.
        CompilerProject **ProjectForType = NULL,            // The project containing a component of type "Type" that was considered
                                                            // for a successful match ignoring project equivalence.
        CompilerProject **ProjectForGenericParam = NULL     // The project containing a component of type "Parameter" that was considered
                                                            // for a successful match ignoring project equivalence.
    )
    {
        return
            ValidateGenericConstraintsForType(
                Type,
                Parameter,
                Binding,
                NULL,
                NULL,
                NULL,
                CompilerHostInstance,
                CompilerInstance,
                SymbolFactory,
                IgnoreProjectEquivalence,
                ProjectForType,
                ProjectForGenericParam);
    }

  private:

    static void
    VerifyOperatorsInContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    VerifyOperatorsInContainer();

    void
    VerifyOperator
    (
        BCSYM_UserDefinedOperator *Member,
        BCSYM *InstanceTypeForMemberContainer,
        RelationalOperatorPool *Pool,
        Symbols *SymbolFactory
    );

    static void
    ScanContainerForObsoleteUsage
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches,
        bool NeedToCheckObsolete = true
    );

    void
    ScanContainerForObsoleteUsage(bool NeedToCheckObsolete);

    static void
    ScanContainerForResultTypeChecks
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    ScanContainerForResultTypeChecks();

    void
    EvaluateAllConstantDeclarationsInContainer();

    // This should only be used when Binding SourceFiles i.e. VB source code
    // This should not be used when importing metadata because we want to load
    // metadata symbols on demand.
    static void
    BindContainerAndNestedTypes
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static void
    GenSyntheticCodeForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );


    // Resolves the constant expressions and does the final processing on properties.
    void
    EvaluateConstantExpressionsInContainer();


    // Access Exposure Semantics helpers -------------------------

    /*
        Return true if no error,
        else false if error due to access exposure
    */
    bool
    VerifyAccessExposureOfBaseClassOrInterface
    (
        BCSYM_Container *ClassOrInterface,
        BCSYM *RawBase
    );

    void
    VerifyAccessExposureForMember
    (
        BCSYM_Member    *Member // [in] the member we are checking to see if it exposes a more restrictive type
    );

    void
    VerifyAccessExposureForGenericParameters
    (
        BCSYM_Container *PossibleGenericContainer
    );

    void
    VerifyAccessExposureForGenericParameters
    (
        BCSYM_Proc *PossibleGenericProc
    );

    void
    VerifyAccessExposureForGenericParametersHelper
    (
        BCSYM_NamedRoot *ProcOrContainer,
        BCSYM_GenericParam *ListOfGenericParams
    );

    void
    VerifyAccessExposureForMemberType
    (
        BCSYM_NamedRoot    *Member,   // [in] the member we are checking to see if it exposes a more restrictive type
        BCSYM    *TypeBehindMember // [in] the type we are checking to see if it exposes a more restrictive type
    );

    void
    VerifyAccessExposureForParameterType
    (
        BCSYM_Member    *Member,   // [in] the member we are checking to see if it exposes a more restrictive type
        BCSYM_Param *Param,        // [in] the param we are checking
        BCSYM *TypeBehindParam     // [in] the type we are checking
    );

    // End of Access Exposure Semantics helpers -------------------------

    // Checks on class inheritance - only does the checks that do not involve walking inheritance chains.
    void
    DoClassInheritanceValidationNotRequiringOtherBoundBasesInfo
    (
        BCSYM_Class * MainClass
    );

    // Checks on class inheritance - only does the checks that do involve walking inheritance chains.
    void
    DoClassInheritanceValidationRequiringOtherBoundBasesInfo
    (
        BCSYM_Class * MainClass
    );

    // Checks to make sure we don't have public interfaces inheriting private ones, etc.
    void
    ValidateInterfaceInheritance
    (
        BCSYM_Interface *Interface
    );

    // Return true if Base is valid, else false
    //
    bool
    VerifyBaseNotNestedInDerived
    (
        BCSYM_NamedType *Base
    );

    // Return true if Base is valid, else false
    //
    bool
    VerifyBaseNotGenericParam
    (
        BCSYM_NamedType *Base
    );

    static void
    MarkBaseClassBad
    (
        BCSYM_Class *DerivedClass
    );

    // Synthetic code generation helpers --------------------------

    void
    GenSyntheticCodeForContainer();

    void
    GenSyntheticCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod
    );

    void
    GenSynthFormMainCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod,
        StringBuffer *CodeBlock
    );

    void
    GenSyntheticGetCode
    (
        _In_ BCSYM_SyntheticMethod *pSyntheticGet,
        _Out_ StringBuffer *pCodeBlockText
    );

    void
    GenSyntheticSetCode
    (
        _In_ BCSYM_SyntheticMethod *pSyntheticSet,
        _Out_ ParseTree::StatementList** pSetParsedCode,
        _Out_ unsigned * pLocalsCountForParsedCode
    );

    void
    GenMyGroupCollectionGetCode
    (
        BCSYM_SyntheticMethod *SyntheticGet,
        StringBuffer *CodeBlockText
    );

    void

    GenMyGroupCollectionSetCode
    (
        BCSYM_SyntheticMethod *SyntheticSet,
        StringBuffer *CodeBlockText
    );

    void
    GenWithEventsSetAddRemoveBlocks
    (
        HandlerList *Handlers,
        ParseTree::Expression * WithEventsVariable,
        ParseTree::StatementList** AssignmentBlock,
        ParseTree::StatementList** RemoveHandlerCode,
        ParseTree::StatementList** AddHandlerCode,
        unsigned * LocalsCountForParsedCode
    );

public:

    static BCSYM *
    ExtractDelegateFromHandlesEntry
    (
        void * Parameter
    );

private:

    struct DelayedGetDelegateParameter
    {
        DelayedGetDelegateParameter
        (
            HandlerList * HandlesEntry,
            Compiler * CompilerInstance,
            _In_ NorlsAllocator * Allocator,_In_
            GenericBindingCache * GenericBindingCache
        ) :
            m_HandlesEntry(HandlesEntry),
            m_Symbols(
                CompilerInstance,
                Allocator,
                NULL,
                GenericBindingCache)
        {
        }

        Symbols m_Symbols;
        HandlerList * m_HandlesEntry;
    };

    void
    GenAddEventCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod,
        StringBuffer *CodeBlockText
    );

    void
    GenRemoveEventCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod,
        StringBuffer * CodeBlockText
    );

#if IDE 
    void
    GenIterateENCTrackingListCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod,
        StringBuffer * CodeBlockText
    );

    void
    GenAddToENCTrackingListCode
    (
        BCSYM_SyntheticMethod *SyntheticMethod,
        StringBuffer * CodeBlockText
    );
#endif IDE

    static STRING *
    GetMyGroupScrambledName
    (
        BCSYM_Class * GroupMemberClass,
        NorlsAllocator * Scratch,
        Compiler * Compiler
    );

    void
    GenSyntheticMyGroupCollectionProperty
    (
        BCSYM_Class * ContainerClass,
        BCSYM_Class * GroupMemberClass,
        NorlsAllocator * Scratch,
        bool mangleNames
    );

    bool
    ConflictsWithOtherProperties   // not a real synthetic helper, but keep together MyCollection stuff
    (
        BCSYM_Class * Class,
        BCSYM_Class * GroupMemberClass,
        _In_z_ STRING * MyCollectionCandidateName,
        bool hasSet
    );

    // End of Synthetic code generation helpers --------------------------

    // Start of Event Semantic helpers

    BCSYM_Proc*
    SynthesizeConstructor
    (
        bool SynthesizeSharedConstructor
    );

    // End of Event semantics helpers

    bool
    SourceFilesOfContainerHaveErrorsInCurrentCompilerStep();

    // Processes implements, hooking the implementing methods to the interface members they implement.
    void
    ResolveImplementsInContainer();

    static void
    ResolveImplementsInContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    CheckImplementsForClass
    (
        BCSYM_Class *Class
    );

    void
    CheckForCompletelyImplementedInterfaces
    (
        BCSYM_Class *Class, // [in] the class doing the implementing
        DynamicArray<ImplementsInfo> *ListOfInterfacesToImplement,  // [in] the list of interfaces we are supposed to implement for Class
        MemberBindingHashTable *ImplementedInterfaceMembers // [in] the list of interface members that were implemented
    );

    void
    BuildListOfImplementedInterfaces
    (
        BCSYM_Class *Class, // [in] the class implementing the interfaces
        DynamicArray<ImplementsInfo> *ListOfInterfacesToBeImplemented, // [in] we put all the interfaces implemented by this class in here
        Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
    );


    void
    AddInterfacesClassImplements
    (
        BCSYM *Class, // [in] the class claiming to implement the interface
        DynamicArray<ImplementsInfo> *TheImplementedInterfaces, // [in] we put the interfaces that are implemented by Class in here
        ImplementsInfo::WhereImplementedFrom WhereFrom, // [in] how the class implements this interface
        bool IsBaseClass, // [in] whether Class is a base class
        Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
    );

    void AddBaseInterfaces
    (
        BCSYM *Interface, // [in] the interface whose base interfaces we are adding
        DynamicArray<ImplementsInfo> *InfoOnImplements, // [in] we put the base interfaces of Interface into here
        ImplementsInfo::WhereImplementedFrom WhereFrom, // [in] where the base interfaces are coming from
        BCSYM_Implements *OriginalImplements, // [in] the implements statement that started us out
        BCSYM *ImplementingClass, // [in] class that implements this chain of implemented interfaces
        Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
    );

    BCSYM_NamedRoot * // the interface member to implement
    FindImplementedInterfaceMember
    (
        _In_z_ STRING *ImplementedMemberName,  // [in] the member to find
        BCSYM_Interface *Interface, // [in] the interface to search
        BCSYM_GenericTypeBinding *InterfaceBinding,
        BCSYM_Proc *SignatureToMatch, // [in] the signature we're looking for
        Symbols *SymbolFactory, // [in] Symbol factory for creating bindings during signature matches
        NameFlags Flags, // [in] flags for controlling the name lookup
        bool ProcessingBase, // [in] are we processing the directly specified interface (when false, we are working on a base interface)
        BCSYM *ErrorSymbol, // [in] symbol to put errors on
        bool *HasError, // [out] indicates if we've already reported an error on this search
        bool *HasMatch // [out] indicates if the returned member is an actual match
    );

    void
    BindImplements
    (
        BCSYM_Proc * ImplementingMember, // [in] the class member that is implementing an interface member
        DynamicArray<ImplementsInfo> * TheImplementedInterfaces, // [in] contains a list of interfaces we have implemented so far
        MemberBindingHashTable * ImplementedMembers, // [in] the list of members that are implemented
        Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
    );


    static bool
    IsWithEventsVariable
    (
        BCSYM_NamedRoot *Member
    );

    void
    ValidateWithEventsVarsAndHookUpHandlersInContainer();

    static void
    ValidateWithEventsVarsAndHookUpHandlersInContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static BCSYM * // the type of pType
    ChaseThroughToType
    (
        BCSYM * Type // [in] the type to chase through
    );

    static ACCESS // The access you get by restricting MemberAccess by ContainingAccess
    ConstrainAccess(
        ACCESS MemberAccess, // [in] the access of the thing defined in the container
        ACCESS ContainerAccess // [in] the access of the container
    );

    static bool // true - can be considered to be protected members of the same class, false - should be considered as members of different classes
    CanBeConsideredProtectedMembersOfSameClass
    (
        BCSYM_Container *ContainerOfExposed, // [in] we want to see if this container is a 'peer' of ContainerOfExposer
        BCSYM_Container *ContainerOfExposer // [in] we are going to wind this guy out to see if it ever matches ContainerOfExposed
    );

    void CheckForDuplicateInherits
    (
        BCSYM_Interface *InterfaceBeingChecked  // [in] the interface to check
    );

    bool
    CanInterfacesOrBasesUnify
    (
        BCSYM *Interface1,
        BCSYM *Interface2,
        Symbols *SymbolFactory,
        BCSYM **UnifyingInterface1,
        BCSYM **UnifyingInterface2,
        bool SkipDirectComparison
    );

    bool
    CanInterface1UnifyWithInterface2OrItsBases
    (
        BCSYM_GenericTypeBinding *Interface1,
        BCSYM *Interface2,
        Symbols *SymbolFactory,
        BCSYM **UnifyingInterface1,
        BCSYM **UnifyingInterface2,
        bool SkipDirectComparison
    );

    static bool
    CanInterfacesUnify
    (
        BCSYM *Interface1,
        BCSYM *Interface2,
        Symbols *SymbolFactory,
        Compiler *CompilerInstance
    );

    static void
    DimAsNewSemantics
    (
        BCSYM *SymbolToCheck,
        ERRID &Error
    );

    static bool
    IsTypeValidForConstantVariable
    (
        BCSYM *Type
    );

    static BilKind // the 'normalized' symbol
    GetSimilarKind
    (
        BilKind SymbolKind // [in] the symbol to normalize
    );

public:
    static void
    ResolveFileLevelImports
    (
        CompilerFile *File,
        CompilationCaches *IDECompilationCaches
    );

private:
    // Resolve base for current container
    void
    ResolveBasesForContainer();

    // Resolve base for a Class
    void
    ResolveBaseForClass
    (
        BCSYM_Class *Class
    );

    // Resolve bases for an Interface
    void
    ResolveBasesForInterface
    (
        BCSYM_Interface *Interface
    );

    bool
    DoesBaseCauseInheritanceCycle
    (
        BCSYM_Container *Container,
        BCSYM_Container *Base,
        Location *InheritsLocation
    );

    bool
    DetectInheritanceCycle
    (
        BCSYM_Container *ContainerToVerify,
        BCSYM_Container *BaseContainer,
        DynamicArray<BCSYM_Container *> *ContainersInCycle
    );


    static bool
    IsClassOrInterface
    (
        BCSYM *Container
    );

    static bool
    IsClass
    (
        BCSYM *Container
    );

    static bool
    IsInterface
    (
        BCSYM *Container
    );

    static bool
    IsStdModule
    (
        BCSYM *Container
    );

    static bool
    IsUnnamedNamespace
    (
        BCSYM *PossibleNamespace,
        Compiler *CompilerInstance
    );

    void
    ReportInheritanceCyclesError
    (
        DynamicArray<BCSYM_Container *> *ClassesInCycle,
        Location *Location
    );

    void
    ResolveNamedType
    (
        BCSYM_NamedType *NamedType, // [in] the named type to resolve
        TypeResolutionFlags Flags,
        NameFlags NameInterpretFlags = NameNoFlags,
        bool DisableNameLookupCaching = false
    );

    void
    ResolveNamedTypeAndTypeArguments
    (
        BCSYM_NamedType *NamedType, // [in] the named type to resolve
        TypeResolutionFlags Flags,
        NameFlags NameInterpretFlags,
        bool DisableNameLookupCaching
    );

    void
    ResolveTypeArguments
    (
        NameTypeArguments **ArgumentsArray,
        unsigned NumberOfArguments,
        ErrorTable *ErrorLog,
        SourceFile *CurrentSourceFile,
        TypeResolutionFlags Flags,
        NameFlags NameInterpretFlags,
        bool DisableNameLookupCaching
    );

    static void
    ResolveTypeArguments
    (
        NameTypeArguments **ArgumentsArray,
        unsigned NumberOfArguments,
        NorlsAllocator *Allocator,
        ErrorTable *ErrorLog,
        Compiler *CompilerInstance,
        CompilerHost *CompilerHost,
        SourceFile *CurrentSourceFile,
        TypeResolutionFlags Flags,
        CompilationCaches *IDECompilationCaches,
        NameFlags NameInterpretFlags,
        bool DisableNameLookupCaching,
        bool &HasGenericArgument
    );

    // Resolve Bases for the given Container
    static void
    ResolveBasesForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static void
    ResolveAllNamedTypesForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    ResolveAllNamedTypesForContainer
    (
        TypeResolutionFlags Flags      // [in] indicates how to resolve the type
    );

#if IDE 
    void
    GenerateENCTrackingCode();

    Type*
    DetermineTypeOfENCTrackingList
    (
        Symbols* SymbolAllocator
    );

    ClassOrRecordType*
    GetAndVerifyCOMClassSymbol
    (
        FX::TypeName Symbol
    );

    void
    GenerateENCHiddenRefreshCode();
#endif IDE

    void
    ResolveBaseType
    (
        BCSYM_NamedType *Type
    );


    // checks structure containership cycles
    void
    CheckStructureMemberCycles();

    static BCSYM_Class *
    IsTypeStructAndIsTypeOfAValidInstanceMemberVariable
    (
        BCSYM_NamedType *NamedType,
        BCSYM_GenericTypeBinding *&BindingContextForMemberVariableType
    );

    static void
    DetectStructMemberCyclesForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    DetectStructMemberCyclesForContainer();

    static void
    DetectStructureMemberCycle
    (
        BCSYM_Class *Structure,
        BCSYM_GenericTypeBinding *StructureBindingContext,
        BCSYM_GenericTypeBinding *OpenBindingForStructure,
        StructCycleNode *PrevCycleNode,
        StructCycleNode **CycleCausingNode,
        Symbols *SymbolAllocator
    );

    static void
    ReportStructureMemberCycleError
    (
        StructCycleNode *CycleInitiatingNode,
        Symbols *SymbolAllocator
    );

    void
    ReTypeEnumMembersIfNecessary();

    static bool
    IsStructure
    (
        BCSYM_Container *Container
    );

    static bool
    IsStructure
    (
        BCSYM *PossibleStructure
    );

    void
    ResolveAllNamedTypes
    (
        BCSYM_NamedType *NamedTypesList, // [in] the named type to resolve
        TypeResolutionFlags Flags      // [in] indicates how to resolve the type
    );

    void BuildEvent
    (
        BCSYM_EventDecl *Event // [in] the event declaration
    );

  public:

    // This is public so that metaimport also can use it
    //
    static void
    CopyInvokeParamsToEvent
    (
        BCSYM_EventDecl *Event,
        BCSYM *Delegate,
        BCSYM_Proc *DelegateInvoke,
        NorlsAllocator *Allocator,
        Symbols *SymbolFactory
    );

  private:
    void
    ReportSyntheticMemberClashInSameClass
    (
        BindableMemberInfo *MemberInfo,
        BindableMemberInfo *OtherMemberInfo
    );

    bool
    IsShadowing(unsigned ShadowingInfo)
    {
        return ShadowingInfo & Shadows;
    }

    bool
    IsShadowing(const BindableMemberInfo &MemberInfo)
    {
        return IsShadowing(MemberInfo.ShadowingInfo);
    }

    bool
    IsShadowingExplicitly(unsigned ShadowingInfo)
    {
        return (ShadowingInfo & Shadows) &&
                    !(ShadowingInfo & ImplicitShadows);
    }

    bool
    IsShadowingExplicitly(const BindableMemberInfo &MemberInfo)
    {
        return IsShadowingExplicitly(MemberInfo.ShadowingInfo);
    }

    bool
    IsShadowingImplicitly(unsigned ShadowingInfo)
    {
        return (ShadowingInfo & Shadows) &&
                    (ShadowingInfo & ImplicitShadows);
    }

    bool
    IsShadowingImplicitly(const BindableMemberInfo &MemberInfo)
    {
        return IsShadowingImplicitly(MemberInfo.ShadowingInfo);
    }

    bool
    IsOverloading(unsigned ShadowingInfo)
    {
        return ShadowingInfo & Overloads;
    }

    bool
    IsOverloading(const BindableMemberInfo &MemberInfo)
    {
        return IsOverloading(MemberInfo.ShadowingInfo);
    }

    bool
    IsOverriding(unsigned ShadowingInfo)
    {
        return ShadowingInfo & Overrides;
    }

    bool
    IsOverriding(const BindableMemberInfo &MemberInfo)
    {
        return IsOverriding(MemberInfo.ShadowingInfo);
    }

    //-------------- Start Of Name Binding Helpers --------------------

    static bool
    NamesCanOverload
    (
        BCSYM_NamedRoot *Member1,
        BCSYM_NamedRoot *Member2
    );

    static BCSYM_NamedRoot *
    SimpleBindIncludingUnBindableMembers_Helper
    (
        BCSYM_Container *Container,
        _In_z_ STRING *Name
    );

    static BCSYM_NamedRoot *
    SimpleBindIncludingUnBindableMembers
    (
        BCSYM_Container *Container,
        BCSYM_NamedRoot *SourceMember
    );

    static BCSYM_NamedRoot *
    GetNextOfSameNameIncludingUnBindableMembers_Helper
    (
        BCSYM_NamedRoot *Member
    );

    static BCSYM_NamedRoot *GetNextOfSameNameIncludingUnBindableMembers
    (
        BCSYM_NamedRoot *Member
    );

    //-------------- End Of Name Binding Helpers ----------------------


    void
    ResolveShadowingOverloadingAndOverridingForContainer(); // implemented in ShadowingSemantics.cpp

    void
    ResolveShadowingOverloadingAndOverridingForBases(); // implemented in ShadowingSemantics.cpp

    void
    CheckVarianceValidityOfContainer(CompilationCaches *IDECompilationCaches);

    void
    CheckVarianceValidityOfInterface(BCSYM_Interface *Interface);

    void
    CheckVarianceValidityOfDelegate(BCSYM_Class *Delegate);

    void
    CheckVarianceValidityOfClassOrStructOrEnum(BCSYM_Container *Container);

    void
    CheckVarianceValidityOfSignature(BCSYM_Proc *Signature);

    void
    CheckVarianceAmbiguity(BCSYM_Container *Container, CompilationCaches *IDECompilationCaches);

    // Following enum is used just to help give more specific error messages.
    enum VarianceContextEnum
    {
        // We'll give specific error messages in these simple contexts:
        VarianceContextByVal,
        VarianceContextByRef,
        VarianceContextReturn,
        VarianceContextConstraint,
        VarianceContextNullable,
        VarianceContextReadOnlyProperty,
        VarianceContextWriteOnlyProperty,
        VarianceContextProperty,
        // Otherwise (e.g. nested inside a generic) we use the following as a catch-all:
        VarianceContextComplex,
    };

    void
    CheckThatTypeSatisfiesVariance(
        BCSYM *Type,
        Variance_Kind Variance,
        BCSYM_Container *Container,
        VarianceContextEnum Context,
        BCSYM *ErrorSymbol,
        ErrorTable *ErrorTable);

    void
    CheckThatTypeSatisfiesVariance_Helper(
        BCSYM *Type,                  // We will check this type...
        Variance_Kind Variance,       // ... to make sure it can deliver this kind of variance
        BCSYM_Container *Container,   // The container where we're trying to refer to the type
        VarianceContextEnum Context,  // And where specifically in that container we're trying to refer to the type
        BCSYM *ErrorSymbol,           // If it can't, we'll report an error squiggly under this symbol
        ErrorTable *ErrorTable,       // in this error-table
        _In_opt_ BCSYM_GenericBinding *ErrorBinding, // This is the binding that immediately contains Type (if Type is contained by a binding)
        _In_opt_ BCSYM_GenericParam *ErrorParam,     // and this is the generic parameter that it was bound to
        int ErrorBindingNesting                      // Was that generic at the very top level of ErrorSymbol? (if so, it will simplify error messages)
        );

    Variance_Kind
    InvertVariance(Variance_Kind v);

    void
    CheckGenericConstraintsForContainer();

    void
    ValidateGenericParameters
    (
        BCSYM_Container *Container
    );

    void
    ValidateGenericParameters
    (
        BCSYM_Proc *Generic
    );

    void
    ValidateGenericParamsAndDirectConstraints
    (
        BCSYM_GenericParam *ParameterList,
        bool &ConstraintsFound
    );

    void
    ValidateNameShadowingForGenericParams
    (
        BCSYM_GenericParam *ParameterList
    );

    void
    ValidateDirectConstraintsForGenericParams
    (
        BCSYM_GenericParam *ParameterList,
        bool &ConstraintsFound
    );

    void
    ValidateDirectConstraint
    (
        BCSYM_GenericConstraint *Constraint,
        BCSYM_GenericParam *Parameter,
        CompilerHost *CompilerHost,
        bool &HasReferenceConstraint,
        bool &HasValueConstraint,
        BCSYM *&ClassConstraintType,
        bool DisallowRedundancy
    );

    static bool
    ValidateConstraintType
    (
        BCSYM *ConstraintType,
        Location *ConstraintLocation,
        _In_opt_z_ STRING *GenericParameterName,
        ErrorTable *ErrorLog,
        CompilerHost *CompilerHost,
        Compiler *CompilerInstance,
        NorlsAllocator *Allocator,
        bool &HasReferenceConstraint,
        bool &HasValueConstraint,
        BCSYM *&ClassConstraintType,
        bool DisallowRedundancy
    );

  public:
    static bool
    IsValidConstraintType
    (
        BCSYM *ConstraintType,
        CompilerHost *CompilerHost,
        Compiler *CompilerInstance,
        NorlsAllocator *Allocator,
        bool DisallowRedundancy
    );

  private:
    void
    ValidateIndirectConstraintsForGenericParams
    (
        BCSYM_GenericParam *ParameterList,
        ErrorTable *ErrorLog
    );

    void
    CheckAllGenericParamsForConstraintCycles
    (
        BCSYM_GenericParam *ParameterList,
        ErrorTable *ErrorLog
    );

    void
    CheckAllGenericParamsForConstraintCycles
    (
        BCSYM_GenericParam *ParameterList,
        BCSYM_GenericTypeConstraint *CurrentConstraints[],
        bool ParametersChecked[],
        ErrorTable *ErrorLog
    );

    void
    CheckGenericParamForConstraintCycles
    (
        BCSYM_GenericParam *Parameter,
        BCSYM_GenericTypeConstraint *CurrentConstraints[],
        bool ParametersChecked[],
        ErrorTable *ErrorLog
    );

    void
    ReportConstraintCycleError
    (
        ErrorTable *ErrorLog,
        BCSYM_GenericParam *Parameter,
        BCSYM_GenericTypeConstraint *ConstraintsInCycle[],
        unsigned int CycleInitiatingConstraintIndex
    );

    void
    CheckAllGenericParamsForIndirectConstraintClashes
    (
        BCSYM_GenericParam *ParameterList,
        bool ParametersChecked[],
        ErrorTable *ErrorLog
    );

    void
    CheckGenericParamForIndirectConstraintClash
    (
        BCSYM_GenericParam *Parameter,
        bool ParametersChecked[],
        ErrorTable *ErrorLog
    );

    void
    ValidateIndirectConstraints
    (
        BCSYM_GenericParam *Parameter,
        bool ParametersChecked[],
        ErrorTable *ErrorLog
    );

    bool
    ConstraintsConflict
    (
        BCSYM_GenericConstraint *Constraint1,
        BCSYM_GenericConstraint *Constraint2
    );

    WCHAR *
    GetErrorNameForConstraint
    (
        BCSYM_GenericConstraint *Constraint,
        StringBuffer *Buffer
    );

    static ShadowingOverloadingOverridingInfo
    IsOverloadingOverridingOrShadowing
    (
        BCSYM_NamedRoot *Member
    );

    void
    CheckForOverloadOverridesShadowsClashesInSameContainer
    (
        MemberInfoIter *MemberInfos
    );

    void
    ValidateDefaultProperties
    (
        MemberInfoIter *MemberInfos
    );

    BCSYM_Property *
    GetShadowedDefaultProperty
    (
        BCSYM_Container *Container
    );

    void
    GetListOfMustOverrideMembersInBasesYetToBeOverridden
    (
        BCSYM_Class *Class,
        MembersHashTable *MustOverrideMembersList
    );

    void
    ValidateProperty
    (
        BCSYM_Property *Property
    );

    void
    ResolveShadowingForMember // implemented in ShadowingSemantics.cpp
    (
        BindableMemberInfo *ShadowingMemberInfo,
        MembersHashTable *MustOverrideMembersList
    );

    void
    ResolveShadowingForMember // implemented in ShadowingSemantics.cpp
    (
        BindableMemberInfo *ShadowingMemberInfo
    );

    void
    DetermineShadowingAndOverloadingForMemberAcrossContainers // implemented in ShadowingSemantics.cpp
    (
        BindableMemberInfo *OverloadingMemberInfo,
        BCSYM_Container *Container,
        BCSYM_NamedRoot *&FirstOverloadedOrShadowedMemberFound,
        BCSYM_NamedRoot *&OverloadedMemberFoundInDifferentBaseThanFirst,
        bool &IsShadowing
    );

    void
    GenerateWarningIfShadowingImplicitly
    (
        BindableMemberInfo *ShadowingMemberInfo,
        BindableMemberInfo *ShadowedMemberInfo,
        bool IndicateShadowingAcrossMultipleBases
    );

    void
    GenerateImplicitShadowingWarning
    (
        BindableMemberInfo *ShadowingMemberInfo,
        BindableMemberInfo *ShadowedMemberInfo,
        bool IndicateShadowingAcrossMultipleBases
    );

    void
    GenerateErrorIfShadowingAnyMustOverrideMember
    (
        BindableMemberInfo *ShadowingMemberInfo,
        MembersHashTable *MustOverrideMembersList,
        bool fHideBySig = false
    );

    void
    ResolveOverloadingForMemberAcrossContainers
    (
        BindableMemberInfo *OverloadingMemberInfo,
        MembersHashTable *MustOverrideMembersList
    );

    void
    ResolveOverloadingForMemberAcrossContainers
    (
        BindableMemberInfo *OverloadingMemberInfo
    );

    bool
    CanShadowByNameAndSig
    (
        BCSYM_Proc *Member,
        BCSYM_GenericBinding *MemberBinding,
        BCSYM_Proc *OtherMember,
        BCSYM_GenericBinding *OtherMemberBinding,
        unsigned &CompareFlags,
        Symbols *SymbolFactory
    );

    bool
    CanOverride
    (
        BCSYM_Proc *Member,
        BCSYM_GenericBinding *MemberBinding,
        BCSYM_Proc *OtherMember,
        BCSYM_GenericBinding *OtherMemberBinding,
        unsigned &CompareFlags,
        Symbols *SymbolFactory
    );

    bool
    CompareConstraints
    (
        BCSYM_NamedRoot *OverriddenMember,
        BCSYM_GenericBinding *OverriddenMemberBinding,
        BCSYM_NamedRoot *OverridingMember,
        BCSYM_GenericBinding *OverridingMemberBinding,
        Symbols *SymbolFactory
    );

    bool
    CompareConstraints
    (
        BCSYM_GenericParam *GenericParam1,
        BCSYM_GenericBinding *MemberBinding1,
        BCSYM_GenericParam *GenericParam2,
        BCSYM_GenericBinding *MemberBinding2,
        Symbols *SymbolFactory
    );

    bool
    CompareConstraintsHelper
    (
        BCSYM_GenericParam *GenericParam1,
        BCSYM_GenericBinding *MemberBinding1,
        BCSYM_GenericParam *GenericParam2,
        BCSYM_GenericBinding *MemberBinding2,
        Symbols *SymbolFactory
    );

    void
    PropagateShadowingInfoBetweenSourceSymbolsAndTheirSyntheticSymbols
    (
        MemberInfoIter *MemberInfos
    );

    void
    ResolveOverridingForContainer
    (
        MemberInfoIter *MemberInfos,
        MembersHashTable *MustOverrideMembersList
    );

    void
    ResolveOverridingForMember
    (
        BindableMemberInfo *OverridingMemberInfo,
        BCSYM_GenericBinding *BindingForContainer,
        MembersHashTable *MustOverrideMembersList,
        Symbols *SymbolFactory
    );

    static bool
    HaveIntermediateClassDefinedInDifferentAssemblyOrModule
    (
        BCSYM_Class *DerivedClass,
        BCSYM_Class *BaseClass
    );

    unsigned
    FindMemberOverriddenByOverridingMember
    (
        BindableMemberInfo *OverridingMemberInfo,
        BCSYM_GenericBinding *OverridingMemberBinding,
        BCSYM_Container *Container,
        BCSYM_Proc *&OverriddenMember,
        BCSYM_Proc *&InAccessibleVirtualMemberInIntermediateClass,
        DynamicArray<BCSYM_Proc *> &InAccessibleNotOverridableFriendMethods,
        bool &OverriddenMemberConstraintsMatch,
        DynamicArray<BCSYM_Proc *> &AmbiguousOverriddenMembers,
        Symbols *SymbolFactory
    );

    void
    VerifyOverridingSemantics
    (
        BindableMemberInfo *OverridingMemberInfo,
        BindableMemberInfo *OverriddenMemberInfo,
        DynamicArray<BCSYM_Proc *> &InAccessibleNotOverridableFriendMethods,
        unsigned CompareFlags,
        bool ConstraintsMatch,
        DynamicArray<BCSYM_Proc *> &AmbiguousOverriddenMembers,
        Symbols *SymbolFactory
    );

    bool
    IsMustOverrideMethodOrProperty
    (
        BCSYM_NamedRoot *Member
    );

    bool
    IsOverridingMethodOrProperty
    (
        BCSYM_NamedRoot *Member
    );

    bool
    IsAccessible
    (
        BCSYM_NamedRoot *Member,
        BCSYM_GenericBinding *MemberBindingContext = NULL,
        BCSYM_Container *Context = NULL,
        BCSYM *TypeOfAccessingInstance = NULL
    );

public:
    static bool
    IsAccessibleOutsideAssembly
    (
        BCSYM_NamedRoot *Member,
        bool ConsiderFriend = false
    );

private:

    bool
    IsVirtual
    (
        BCSYM_Proc *Proc
    );

    bool
    IsOverridable
    (
        BCSYM_Proc *Proc
    );

    static bool
    BothAreMethodsOrBothAreProperties
    (
        BCSYM_NamedRoot *Member1,
        BCSYM_NamedRoot *Member2
    );

    static bool
    EmittedNamesAreCompatible
    (
        BCSYM_NamedRoot *Member1,
        BCSYM_NamedRoot *Member2
    );

    static bool
    ResolveOverloadingShouldSkipBadMember
    (
        int errid
    );

    void
    ResolveOverloadingInSameContainer
    (
        MemberInfoIter *MemberInfos
    );

    static void
    MarkOverloadedMemberBad
    (
        BCSYM_NamedRoot *Member
    );

    static void
    MarkFakeWindowsRuntimeMemberBad
    (
        _In_ BCSYM_NamedRoot *Member,
        Compiler *pCompiler
    );

    void
    ReportOverloadsError
    (
        BCSYM_NamedRoot *Member,
        BCSYM_NamedRoot *OtherMember,
        unsigned CompareFlags
    );

    void
    ReportEnumMemberClashError
    (
        BCSYM_NamedRoot *SymbolThatOccursFirst,
        BCSYM_NamedRoot *SymbolThatOccursSecond
    );

    static bool
    IsEnumMemberReservedMember
    (
        BCSYM_NamedRoot *Member
    );

    void
    VerifyClassIsMarkedAbstractIfNeeded
    (
        MemberInfoIter *MemberInfos,
        MembersHashTable *MustOverrideMembersInBases
    );

    // This is a helper for and only for BCSYM_Class::GetVType()
    // to resolve an enum's underlying type if possible when
    // trying to determine the VType.
    //
    // This is needed because during bindable, some constant
    // expressions in declarations might need to be evaluated
    // for which we might need to know the VType of enums.
    //
    // Returns a bool if the underlying type was resolved, else
    // returns false.
    //
    static bool
    DetermineEnumUnderlyingTypeIfPossible
    (
        BCSYM_Class *Enum,
        CompilationCaches *IDECompilationCaches
    );

    // This is a helper for and only for BCSYM::DigThroughNamedType
    // to resolve named types on demand when digging through
    // a named type.
    //
    // This is need because during bindable, some constant
    // expressions in declarations might need to be evaluated
    // for which we might need to dig through the named types
    // in containers which have not yet been through named type
    // resolution.
    //
    // Returns true if the NamedType was resolved, else returns false.
    //
    static void
    ResolveNamedTypeIfPossible
    (
        BCSYM_NamedType *NamedType,
        CompilationCaches *IDECompilationCaches
    );

    // Returns true if resolved, else returns false.
    //
    static bool
    ResolveAllNamedTypesInContainerIfPossible
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches,
        bool EnsureNoPreviousStepsAreInProgress
    );

    WCHAR *
    GetBasicRep
    (
        BCSYM *Symbol,
        StringBuffer *BasicRep,
        BCSYM_Container *ContainerContext = NULL,
        BCSYM_GenericBinding *GenericBindingContext = NULL
    );

    STRING *
    GetQualifiedErrorName
    (
        BCSYM *NamedRoot
    );

    WCHAR *
    GetErrorNameAndSig
    (
        BCSYM *Symbol,
        BCSYM_GenericTypeBinding *GenericBindingContext,
        ErrorTable *ErrorLog,
        StringBuffer *TextBuffer
    );

    WCHAR *
    GetSigsForAmbiguousOverridingAndImplErrors
    (
        DynamicArray<BCSYM_Proc *> &AmbiguousMembers,
        StringBuffer &Buffer
    );

    static void
    ReportErrorOnSymbol
    (
        ERRID ErrID,
        ErrorTable *ErrorLog,
        BCSYM *SymbolToReportErrorOn,
        _In_opt_z_ const STRING *ErrorReplString1 = NULL,
        _In_opt_z_ const STRING *ErrorReplString2 = NULL,
        _In_opt_z_ const STRING *ErrorReplString3 = NULL,
        _In_opt_z_ const STRING *ErrorReplString4 = NULL,
        _In_opt_z_ const STRING *ErrorReplString5 = NULL,
        _In_opt_z_ const STRING *ErrorReplString6 = NULL,
        _In_opt_z_ const STRING *ErrorReplString7 = NULL,
        _In_opt_z_ const STRING *ErrorReplString8 = NULL,
        _In_opt_z_ const STRING *ErrorReplString9 = NULL
    );

    void
    ReportErrorOnSymbol
    (
        ERRID ErrID,
        ErrorTable *ErrorLog,
        BCSYM *SymbolToReportErrorOn,
        BCSYM *Substitution1,
        BCSYM *Substitution2 = NULL,
        BCSYM *Substitution3 = NULL,
        BCSYM *Substitution4 = NULL,
        BCSYM *Substitution5 = NULL
    );

    void
    ReportErrorOnSymbolWithBindingContexts
    (
        ERRID ErrID,
        ErrorTable *ErrorLog,
        BCSYM *SymbolToReportErrorOn,
        BCSYM *Substitution1,
        BCSYM_GenericTypeBinding *GenericBindingContext1,
        BCSYM *Substitution2 = NULL,
        BCSYM_GenericTypeBinding *GenericBindingContext2 = NULL,
        BCSYM *Substitution3 = NULL,
        BCSYM_GenericTypeBinding *GenericBindingContext3 = NULL,
        BCSYM *Substitution4 = NULL,
        BCSYM_GenericTypeBinding *GenericBindingContext4 = NULL,
        BCSYM *Substitution5 = NULL,
        BCSYM_GenericTypeBinding *GenericBindingContext5 = NULL
    );

    static void
    ReportErrorAtLocation
    (
        ERRID ErrID,
        ErrorTable *ErrorLog,
        Location *Location,
        _In_opt_z_ STRING *ErrorReplString1 = NULL,
        _In_opt_z_ STRING *ErrorReplString2 = NULL,
        _In_opt_z_ STRING *ErrorReplString3 = NULL,
        _In_opt_z_ STRING *ErrorReplString4 = NULL,
        _In_opt_z_ STRING *ErrorReplString5 = NULL,
        _In_opt_z_ STRING *ErrorReplString6 = NULL,
        _In_opt_z_ STRING *ErrorReplString7 = NULL,
        _In_opt_z_ STRING *ErrorReplString8 = NULL,
        _In_opt_z_ STRING *ErrorReplString9 = NULL
    );

    void
    ReportErrorAtLocation
    (
        ERRID ErrID,
        ErrorTable *ErrorLog,
        Location *Location,
        BCSYM *Substitution
    );

    static Location *
    GetTypeUseErrorLocation
    (
        BCSYM *RawType,
        Location *DefaultLocation = NULL
    );

    bool
    CanOverload
    (
        BCSYM_NamedRoot *Member,
        BCSYM_NamedRoot *OtherMember,
        unsigned &CompareFlags
    );

    bool
    DoSignaturesMatchAsPartialMethods
    (
        BCSYM_NamedRoot *Member,
        BCSYM_NamedRoot *OtherMember,
        unsigned &CompareFlags
    );

    bool
    ValidatePartialMethodsMatchGenericConstraints
    (
        BCSYM_Proc *Partial,
        BCSYM_Proc *Implementing
    );

    bool
    ValidateParameterNamesOnPartialMethods
    (
        BCSYM_Proc* Partial,
        BCSYM_Proc* Implementing
    );

    void
    CreateAttrValsForPartialMethodSymbols
    (
        BCSYM_Proc* PartialProc,
        BCSYM_Proc* ImplementingProc,
        BCSYM *Partial,
        BCSYM *Implementing,
        WellKnownAttrVals* Common
    );

    void
    FixUpAttributesForPartialMethods
    (
        BCSYM_Proc *Partial,
        BCSYM_Proc *Implementing
    );

    void
    LinkParametersForPartialMethods
    (
        BCSYM_Proc *Partial,
        BCSYM_Proc *Implementing
    );

    bool
    AttemptToLinkPartialMethods
    (
        BCSYM_Proc *Partial,
        BCSYM_Proc *Implementing,
        unsigned PartialCompareFlags
    );

  public:
    static bool
    IsMethod
    (
        BCSYM *Member
    );

    static bool
    IsProperty
    (
        BCSYM *Member
    );

  private:
    bool
    IgnoreClashingErrorsOnSyntheticMembers
    (
        BindableMemberInfo *MemberInfo1,
        BindableMemberInfo *MemberInfo2
    );

    void
    ValidateWithEventsVarsInHandlesListsAndSynthesizePropertiesIfRequired();


    // The handles clause that specifies the WithEvents variable we want to find is sent in.
    //
    BCSYM_Variable *
    GetWithEventsVarReferredToInHandlesClause
    (
        BCSYM_HandlesList *HandlesEntry,
        bool &WithEventsVarFoundInBase
    );

    // Get the withevents property associated with a given withevents variable.
    //
    BCSYM_Property*
    GetWithEventsPropertyForWithEventsVariable
    (
        BCSYM_Variable *WithEventsVar
    );

    BCSYM_Variable*
    BindWithEventsVariable
    (
        _In_z_ STRING *WithEventVarName,
        bool &InAccessible,
        bool &WithEventsVarFoundInBase
    );

    BCSYM_Property*
    SynthesizeWithEventsProperty
    (
        BCSYM_Variable *WithEventsVar,
        Symbols &SymbolFactory
    );

    bool
    ValidateWithEventsVar
    (
        BCSYM_Variable *WithEventsVar
    );

    // AnyEventDefined - [out] whether any events were defined on TypeBehindWithEventsVar.
    //                   NOTE: CALLER SHOULD INITIALIZE TO FALSE!  We don't because this function is recursive
    //
    bool
    CheckForAccessibleEvents
    (
        BCSYM *TypeOfWithEventsVar,
        bool ConsiderEventSourcePropertiesToo,
        bool &AnyEventDefined
    );

    // true - there are events defined on the WithEvents variable AND they are visible to ContainerOfWithEventsVar
    // EventDeclContainer - [in] the container to search for an accessible event
    // TypeOfWithEventsVar - [in] the type of the withevents variable
    // AnyEventDefined - [out] whether any events were defined on TypeBehindWithEventsVar.
    //                   NOTE: CALLER SHOULD INITIALIZE TO FALSE! We don't because this function is recursive
    //
    bool
    CheckForAccessibleEventsInContainer
    (
        BCSYM *TypePossiblyContainingEvents,
        BCSYM *TypeOfWithEventsVar,
        bool ConsiderEventSourcePropertiesToo,
        bool &AnyEventDefined
    );

    // true - there are events defined on the WithEvents variable AND they are visible to ContainerOfWithEventsVar
    // EventDeclContainer - [in] the container to search for an accessible event
    // TypeOfWithEventsVar - [in] the type of the withevents variable
    // MembersHashTable *ShadowingMembers - [in/out] Any members that are marked Shadows get put in here
    // AnyEventDefined - [out] whether any events were defined on TypeBehindWithEventsVar.
    //                   NOTE: CALLER SHOULD INITIALIZE TO FALSE! We don't because this function is recursive
    //
    bool
    CheckForAccessibleEventsWorker
    (
        BCSYM *TypePossiblyContainingEvents,
        BCSYM *TypeOfWithEventsVar,
        MembersHashTable *ShadowingMembers,
        bool ConsiderEventSourcePropertiesToo,
        bool &AnyEventDefined
    );

    // Is give member shadowed by any member in the given HashTable ?
    //
    bool
    IsMemberShadowed
    (
        BCSYM_NamedRoot *Member,
        MembersHashTable *ShadowingMembers
    );

    void
    ValidateAndHookUpHandles
    (
        BCSYM_HandlesList *HandlesEntry,
        BCSYM_MethodImpl *HandlingMethod
    );

    // Get the event source Property with the given Name - Useful helper when processing handles
    // of the form "handles X.Property.e" where Property has the EventSource attribute on it.
    //
    BCSYM_Property*
    GetEventSourceProperty
    (
        BCSYM *TypeOfWithEventsVar,
        _In_z_ STRING *PropertyName
    );

    static bool
    ValidEventSourceProperty
    (
        BCSYM_Property *Property
    );

    void
    ----AttributesOnAllSymbolsInContainer();

  public:
    static BCSYM_NamedRoot *
    GetNamedContextOfSymbolWithApplAttr
    (
        BCSYM *SymbolWithApplAttr
    );

    static BCSYM_Class *
    HasBaseForMyGroupCollection
    (
        BCSYM_Container *Container
    );

    // compare all the bases of the class container against the base specified by each
    // My* group class and return the found index the list of My groups (if any)
    bool
    MatchBaseForMyGroupCollection
    (
        BCSYM_Container *Container,
        ULONG* index
    );

    static void
    ScanForMyGroupCollectionMembers
    (
        BCSYM_Container *pContainer,
        CompilerProject *pProject
    );

    static bool
    FindBaseInMyGroupCollection
    (
        BCSYM_Class *Class,
        WellKnownAttrVals::MyGroupCollectionData* groupCollectionData,
        unsigned* foundIndex
    );

    static void
    ScanAndLoadMyGroupCollectionMembers
    (
        BCSYM_Container *pContainer,
        CompilerProject *pProject
    );

    static bool
    CanBeMyGroupCollectionMember
    (
        BCSYM_Class *pClass,
        Compiler *pCompiler
    );

#if DEBUG
    static void
    DebugPrintfCanBeMyGroupCollectionMember
    (
        BCSYM_Class *pClass,
        BCSYM_Class *pGroupClass,
        Compiler *pCompiler
    );
#endif //DEBUG

  private:
    void
    ----AttributesForAllBases();

    void
    ResolveShadowingForAllAttributeClassesUsedInContainer();

    void
    InheritWellKnowAttributesFromBases();

    void
    InheritWellKnownAttributes(BCSYM_Container *Base);

  public:
    static void
    DetermineIfAttribute
    (
        BCSYM_Class *Class,
        CompilerHost *CompilerHost
    );

    static bool
    DefinedInMetaData
    (
        BCSYM_Container *Container
    )
    {
        CompilerFile *File = Container->GetCompilerFile();

#if HOSTED
        return !File || File->IsMetaDataFile() || Container->GetExternalSymbol();
#else
        return !File || File->IsMetaDataFile();
#endif
    }

  private:
    static void
    VerifyAttributesOnAllSymbolsInContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    VerifyAttributesOnAllSymbolsInContainer();

    void
    VerifyCustomAttributesOnAllSymbolsInContainer();

    void
    VerifyCustomAttributesOnSymbol
    (
        BCSYM * pSymbol,
        ErrorTable *pErrorTable,
        SourceFile *pSourceFile
    );

    // Note: AppliedAttributeSymbol refers to the symbol that encapsulates the information
    // about the current use of the particular attribute in question
    //
    void
    CheckForMultipleUsesOfAttribute
    (
        BCSYM *SymbolHavingAttrs,
        BCSYM_ApplAttr *AppliedAttributeSymbol,
        BCSYM_Class *AttributeClassToCheckFor
    );

    bool
    DerivesFrom
    (
        BCSYM_Container *PossiblyDerived,
        BCSYM_Container *Base
    );

    bool
    DefinedInMetaData()
    {
        return DefinedInMetaData(CurrentContainer());
    }

    //--------- Decompilation Helpers ------------------

#if IDE 

    static void
    ResetImports
    (
        BCSYM_Namespace *pNamespace
    );

    static void
    DeleteBindableInstancesForContainerAndNestedTypes
    (
        BCSYM_Container *Container
    );

    static void
    UnBindContainerAndNestedTypes
    (
        BCSYM_Container *Container
    );

    static void
    UnBindContainerIncludingPartialContainers
    (
        BCSYM_Container *Container
    );

    static void
    UnBindContainer
    (
        BCSYM_Container *Container,
        bool ClearBases,
        bool ClearNamedTypes,
        bool ClearShadowsOverloadsOverrides,
        bool ClearResolvedImplements
    );

    static void
    UnDeleteSymbolsDeletedInBindable
    (
        BCSYM_Container *Container
    );

    static void
    ClearGenericsStateInHashes
    (
        BCSYM_Container *Container
    );

    static void
    UnBindPartialTypes
    (
        BCSYM_Container *Container
    );

    static void
    SplitPartialTypes
    (
        BCSYM_Container *PartialContainer
    );

    //note - the function may return a new hash
    static BCSYM_Hash *
    ResetHashParent
    (
        BCSYM_Hash *Hash,
        BCSYM_Container *CurrentParent
    );

    static void
    RestoreMainType
    (
        BCSYM_Container *Container
    );

    static void
    ResetPartialTypeFlag
    (
        BCSYM_Container *Container
    )
    {
        Container->SetIsPartialType(Container->IsPartialKeywordUsed());
    }

    static void
    UnBindBasesOfContainer
    (
        BCSYM_Container *Container
    );

    static void
    UnResolveNamedType
    (
        BCSYM_NamedType *Type
    );

    static void
    UnResolveNamedTypeAndTypeArguments
    (
        BCSYM_NamedType *Type
    );

    static void
    UnResolveNameTypeArguments
    (
        NameTypeArguments **TypeArgumentLists,
        unsigned NumberOfLists
    );

    static void
    UnBindAllNamedTypesInContainer
    (
        BCSYM_Container *Container
    );

    static void
    UndoPartialTypeMemberValidation
    (
        BCSYM_NamedRoot *Member
    );

    static void
    UndoPartialMethodChainingAndHashing
    (
        BCSYM_NamedRoot *Member
    );

    static void
    UnbindContainerLevelImplementsClauses
    (
        BCSYM_Container *Container
    );

    static void
    DeleteSymbolsCreatedInBindable
    (
        BCSYM_Container *Container
    );

    static void
    RemoveFromMyCollectionLists
    (
        BCSYM_Container *Container,
        bool fCleanGroupSymbolAttribute  //deep clean, while removing a group collection class, remove also the MyColl. attr from the class
    );

    static void
    ResetGeneralBadness
    (
        BCSYM_NamedRoot *Member
    );

    static void
    ResetBadnessDueToShadowingSemanticsOrAfter
    (
        BCSYM_NamedRoot *Member
    );

    static void
    UndoOverloadsOverridesAndHandlesSemantics
    (
        BCSYM_Proc *Proc
    );

    static void
    UnBindAttributesOnSymbolsInContainer
    (
        BCSYM_Container *Container
    );

    static void
    ReleaseAttrValsAllocatedInBindable
    (
        BCSYM_Container *Container
    );

    static void
    ResetAttributes
    (
        BCSYM *Member
    );

    static void
    UndoImplementsSemantics
    (
        BCSYM_Proc *Proc
    );

    // Public helper for SourceFile to call during its destruction
    //
  public:
    static void
    DeleteAllBindableInstancesInFile
    (
        SourceFile *File
    );

  private:
    static void
    DeleteAllBindableInstancesInFile
    (
        BCSYM_Container *Container
    );

#endif IDE

    //--------- End Of Decompilation helpers --------------

    //------ Access Exposure validation helpers -----------------------

    bool
    VerifyAccessExposure
    (
        BCSYM_NamedRoot *Member,
        BCSYM *RawType,
        DynamicArray<BCSYM_Container *> *&ContainersWithAccessError,  // [In] expect NULL initialized, [out] Non-NULL if any access exposure errors, caller needs to delete it
        DynamicArray<BCSYM *> *&TypesExposed,                         // [In] expect NULL, [out] Non-NULL if any access exposure errors, caller needs to delete it
        bool TypeIsParentBinding = false
    );

    // Returns true if no error, false if error
    bool
    VerifyAccessExposure
    (
        BCSYM_NamedRoot *Member,
        BCSYM_Container *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext,
        BCSYM_Container *&ContainerWithAccessError
    );

    static bool
    MemberIsOrNestedInType
    (
        BCSYM_NamedRoot *Member,
        BCSYM_Container *Type
    );

    // Returns true if no error, false if error
    bool
    VerifyAccessExposureWithinAssembly
    (
        BCSYM_NamedRoot *Member,
        BCSYM_Container *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext,
        BCSYM_Container *&ContainerWithAccessError
    );

    // Returns true if no error, false if error
    bool
    VerifyAccessExposureOutsideAssembly
    (
        BCSYM_NamedRoot *Member,
        BCSYM_Container *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext
    );


    // Returns true if no error, false if error
    bool
    VerifyAccessExposureHelper
    (
        BCSYM_NamedRoot *Member,
        BCSYM_Container *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext,
        BCSYM_Container *&ContainerWithAccessError,
        bool &SeenThroughInheritance,
        bool IsOutsideAssembly
    );

    static BCSYM_Container *
    FindEnclosingContainerWithGivenAccess
    (
        BCSYM_NamedRoot *Member,
        ACCESS StopAtAccess,
        bool IsOutsideAssembly
    );

    bool
    CanBeAccessedThroughInheritance
    (
        BCSYM_NamedRoot *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext,
        BCSYM_Container *Container,
        bool IsOutsideAssembly
    );

    static ACCESS
    GetAccessInAssemblyContext
    (
        BCSYM_NamedRoot *Member,
        bool IsOutsideAssemblyContext
    );

    static  ACCESS
    GetEffectiveAccessOutsideAssembly
    (
        BCSYM_NamedRoot *Member
    );

    bool
    IsTypeAccessible
    (
        BCSYM_NamedRoot *Type,
        BCSYM_GenericTypeBinding *TypeGenericBindingContext,
        BCSYM_Container *ContainerContext
    );

    //------ End of Access Exposure Validation Helpers -----------------

    //------ Partial Type helpers --------------------------------------
    static void
    ResolvePartialTypesForContainer
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static void
    ResolvePartialTypesForNestedContainers
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    static void
    EnsurePartialTypesAreResolved
    (
        BCSYM_Container *Container,
        CompilationCaches *IDECompilationCaches
    );

    void
    ResolvePartialTypesForContainer();

    void
    FindAllPossibleTypeComponents
    (
        BCSYM_Container *EnclosingContainer,
        _In_z_ STRING *NameOfPartialType,
        DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void
    FindAllPossibleTypeComponentsInContainer
    (
        BCSYM_Container *EnclosingContainer,
        _In_z_ STRING *NameOfPartialType,
        DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void
    FindAllPossibleTypeComponentsInHash
    (
        BCSYM_Hash *Hash,
        _In_z_ STRING *NameOfPartialType,
        DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void
    FilterAllDuplicatesAndCombineTypes
    (
        BCSYM_Container *EnclosingContainer,
        DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingTypes,
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void VerifyXMLDocCommentsOnPartialType
    (
        BCSYM_Class *PartialClass
    );

    void ElectPartialTypeAsMainType
    (
        BCSYM_Class *PartialTypeToMorphToMainType
    );

    void ValidateModifiersAcrossPartialTypes
    (
        BCSYM_Class *MainType
    );

    static void
    SortListOfContainersByLocation
    (
        DynamicArray<BCSYM_Container *> &ListOfContainers
    );

    static bool
    BothAreClassesOrBothAreStructures
    (
        BCSYM_Container *Container1,
        BCSYM_Container *Container2
    );

    static bool
    BothAreClasses
    (
        BCSYM_Container *Container1,
        BCSYM_Container *Container2
    );

    static bool
    BothAreStructures
    (
        BCSYM_Container *Container1,
        BCSYM_Container *Container2
    );

    static bool
    BothAreInterfaces
    (
        BCSYM_Container *Container1,
        BCSYM_Container *Container2
    );

    void
    ReportDuplicateErrorsOnMainTypes
    (
        BCSYM_Container *EnclosingContainer,
        DynamicArray<BCSYM_Container *> &ListOfPreviouslyExistingMainTypes,
        DynamicArray<BCSYM_Container *> &ListOfMainTypesInCurrentProject
    );

    void
    ReportDuplicateErrorOnMainType
    (
        BCSYM_Container *EnclosingContainer,
        BCSYM_Container *FirstMainType,
        unsigned IndexOfFirstPossibleDuplicate,
        DynamicArray<BCSYM_Container *> &ListOfMainTypesInCurrentProject
    );

    void
    ReportTypeClashError
    (
        BCSYM_Container *Type,
        BCSYM_Container *ClashingType,
        bool AreTypesMerged
    );

    static void
    MakeDuplicateContainerBad
    (
        BCSYM_Container *Container
    );

    void
    CombineTypes
    (
        DynamicArray<BCSYM_Container *> &ListOfTypes
    );

    void
    CombineMainAndPartialTypes
    (
        BCSYM_Container *MainType
    );

    void
    CombineHashes
    (
        BCSYM_Class *NewParent,
        BCSYM_Hash *&FirstHash,
        BCSYM_Hash *&PrevHash,
        BCSYM_Hash *NextHash
    );

    void
    RemoveDuplicateSynthesizedSymbols
    (
        BCSYM_Container *MainType
    );

    void
    RemoveDuplicateSyntheticMethods
    (
        _In_z_ STRING *NameOfSyntheticMethod,
        BCSYM_Container *MainType
    );

    void
    DeleteSymbolInBindable
    (
        BCSYM_NamedRoot *Member
    );

    void
    MarkStatusOfResolvePartialTypesForAllTypeComponents
    (
        TaskTriState Status,
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void
    ResolvePartialTypesForNestedContainers();

    void static
    MarkStatusOfResolvePartialTypesForNestedContainers
    (
        TaskTriState Status,
        BCSYM_Container *Container
    );

    void
    CreateDependenciesBetweenFilesContainerDifferentPartialTypes
    (
        DynamicArray<BCSYM_Container *> &ListOfTypesInCurrentProject
    );

    void
    CreatePartialTypeDependency
    (
        BCSYM_Container *Dependent,
        BCSYM_Container *DependsOn
    );

  public:

    static bool
    HavePartialTypesBeenResolvedForGivenType
    (
        BCSYM_Container *Container
    );

  private:


    //------ End of Partial Type Helpers -------------------------------


    Compiler *
    CurrentCompilerInstance()
    {
        return m_CompilerHost->GetCompiler();
    }

    CompilerHost *
    CurrentCompilerHost()
    {
        return m_CompilerHost;
    }

    SourceFile *
    CurrentSourceFile(BCSYM_NamedRoot *NamedContext)
    {
        VSASSERT( NamedContext,
                        "NULL context unexpected");

        // Optimization for non-partial types
        //
#if !DEBUG
        if (!ContainerHasPartialTypeComponents())
        {
            return m_SourceFile;
        }
#endif

        SourceFile *CurrentFile = NamedContext->GetSourceFile();

        VSASSERT( ContainerHasPartialTypeComponents() ||
                  CurrentFile == m_SourceFile,
                        "non-partial type Optimization failure unexpected !!!");

        return CurrentFile;
    }

    inline ErrorTable *
    CurrentErrorLog(BCSYM_NamedType *NamedTypeContext)
    {
        VSASSERT( NamedTypeContext,
                        "NULL context unexpected");

        // Optimization for non-partial types
        //
#if !DEBUG
        if (!ContainerHasPartialTypeComponents())
        {
            return m_ErrorTable;
        }
#endif

        VSASSERT( NamedTypeContext->GetContext(),
                        "NULL Context unexpected for named type!!!");

        ErrorTable *CurrentErrorTable = NamedTypeContext->GetContext()->GetErrorTableForContext();

        VSASSERT( ContainerHasPartialTypeComponents() ||
                  CurrentErrorTable == m_ErrorTable,
                        "non-partial type Optimization failure unexpected !!!");

        return CurrentErrorTable;

    }

    inline ErrorTable *
    CurrentErrorLog(BCSYM_NamedRoot *NamedRootContext)
    {
        VSASSERT( NamedRootContext,
                        "NULL context unexpected");

        // Optimization for non-partial types
        //
#if !DEBUG
        if (!ContainerHasPartialTypeComponents())
        {
            return m_ErrorTable;
        }
#endif

        ErrorTable *CurrentErrorTable = NamedRootContext->GetErrorTableForContext();

        VSASSERT( ContainerHasPartialTypeComponents() ||
                  CurrentErrorTable == m_ErrorTable,
                        "non-partial type Optimization failure unexpected !!!");

        return CurrentErrorTable;
    }

    ErrorTable *
    CurrentErrorLog
    (
        BCSYM *Symbol,
        BCSYM_NamedRoot *NamedRootContext
    );

    inline BCSYM_Container *
    CurrentContainer()
    {
        return m_CurrentContainerContext;
    }

    inline NorlsAllocator *
    CurrentAllocator()
    {
        return m_Allocator;
    }

    inline GenericBindingCache*
    CurrentGenericBindingCache();

    inline TaskTriState
    GetStatusOfResolvePartialTypes()
    {
        return (TaskTriState)m_StatusOfResolvePartialTypes;
    }

    inline void
    SetStatusOfResolvePartialTypes
    (
        TaskTriState State
    )
    {
        m_StatusOfResolvePartialTypes = State;
    }

    inline TaskTriState
    GetStatusOfResolvePartialTypesForNestedContainers()
    {
        return (TaskTriState)m_StatusOfResolvePartialTypesForNestedContainers;
    }

    inline void
    SetStatusOfResolvePartialTypesForNestedContainers
    (
        TaskTriState State
    )
    {
        m_StatusOfResolvePartialTypesForNestedContainers = State;
    }

    inline TaskTriState
    GetStatusOfResolveBases()
    {
        return (TaskTriState)m_StatusOfResolveBases;
    }

    inline void
    SetStatusOfResolveBases
    (
        TaskTriState State
    )
    {
        m_StatusOfResolveBases = State;
    }

    inline TaskTriState
    GetStatusOfResolveNameTypes()
    {
        return (TaskTriState)m_StatusOfResolveNamedTypes;
    }

    inline void
    SetStatusOfResolveNamedTypes
    (
        TaskTriState State
    )
    {
        m_StatusOfResolveNamedTypes = State;
    }

    inline TaskTriState
    GetStatusOfDetectStructMemberCycles()
    {
        return (TaskTriState)m_StatusOfDetectStructMemberCycles;
    }

    inline void
    SetStatusOfDetectStructMemberCycles
    (
        TaskTriState State
    )
    {
        m_StatusOfDetectStructMemberCycles = State;
    }

    inline TaskTriState
    GetStatusOfResolveShadowingOverloadingAndOverriding()
    {
        return (TaskTriState)m_StatusOfResolveShadowingOverloadingAndOverriding;
    }

    inline void
    SetStatusOfResolveShadowingOverloadingAndOverriding
    (
        TaskTriState State
    )
    {
        m_StatusOfResolveShadowingOverloadingAndOverriding = State;
    }

    inline TaskTriState
    GetStatusOfValidateWithEventsVarsAndHookUpHandlers()
    {
        return (TaskTriState)m_StatusOfValidateWithEventsVarsAndHookUpHandlers;
    }

    inline void
    SetStatusOfValidateWithEventsVarsAndHookUpHandlers
    (
        TaskTriState State
    )
    {
        m_StatusOfValidateWithEventsVarsAndHookUpHandlers = State;
    }

    inline TaskTriState
    GetStatusOf----AttributesOnAllSymbolsInContainer()
    {
        return (TaskTriState)m_StatusOf----AttributesOnAllSymbolsInContainer;
    }

    inline void
    SetStatusOf----AttributesOnAllSymbolsInContainer
    (
        TaskTriState State
    )
    {
        m_StatusOf----AttributesOnAllSymbolsInContainer = State;
    }

    inline TaskTriState
    GetStatusOfVerifyAttributesOnAllSymbolsInContainer()
    {
        return (TaskTriState)m_StatusOfVerifyAttributesOnAllSymbolsInContainer;
    }

    inline void
    SetStatusOfVerifyAttributesOnAllSymbolsInContainer
    (
        TaskTriState State
    )
    {
        m_StatusOfVerifyAttributesOnAllSymbolsInContainer = State;
    }

    inline TaskTriState
    GetStatusOfVerifyOperatorsInContainer()
    {
        return (TaskTriState)m_StatusOfVerifyOperatorsInContainer;
    }

    inline void
    SetStatusOfVerifyOperatorsInContainer
    (
        TaskTriState State
    )
    {
        m_StatusOfVerifyOperatorsInContainer = State;
    }

    inline TaskTriState
    GetStatusOfResolveImplementsInContainer()
    {
        return (TaskTriState)m_StatusOfResolveImplementsInContainer;
    }

    inline void
    SetStatusOfResolveImplementsInContainer
    (
        TaskTriState State
    )
    {
        m_StatusOfResolveImplementsInContainer = State;
    }

    inline TaskTriState
    GetStatusOfGenSyntheticCodeForContainer()
    {
        return (TaskTriState)m_StatusOfGenSyntheticCodeForContainer;
    }

    inline void
    SetStatusOfGenSyntheticCodeForContainer(TaskTriState State)
    {
        m_StatusOfGenSyntheticCodeForContainer = State;
    }

    inline TaskTriState
    GetStatusOfScanContainerForObsoleteUsage()
    {
        return (TaskTriState)m_StatusOfScanContainerForObsoleteUsage;
    }

    inline void
    SetStatusOfScanContainerForObsoleteUsage
    (
        TaskTriState State
    )
    {
        m_StatusOfScanContainerForObsoleteUsage = State;
    }


    inline TaskTriState
    GetStatusOfScanContainerForReturnTypeCheck()
    {
        return (TaskTriState)m_StatusOfScanContainerForReturnTypeCheck;
    }

    inline void
    SetStatusOfScanContainerForReturnTypeCheck
    (
        TaskTriState State
    )
    {
        m_StatusOfScanContainerForReturnTypeCheck = State;
    }


    inline bool
    PossibleInheritanceCycleDetected()
    {
        return m_PossibleInheritanceCycleDetected;
    }

    inline void
    SetPossibleInheritanceCycleDetected
    (
        bool fPossibleInheritanceCycle
    )
    {
        m_PossibleInheritanceCycleDetected = fPossibleInheritanceCycle;
    }

    bool
    StructureMemberCycleDetected()
    {
        return m_StructureMemberCycleDetected;
    }

    void
    SetStructureMemberCycleDetected
    (
        bool StructureMemberCycleDetected
    )
    {
        m_StructureMemberCycleDetected = StructureMemberCycleDetected;
    }

    void SetContainerHasPartialTypeComponents
    (
        bool ContainerHasPartialTypeComponents
    )
    {
        m_ContainerHasPartialTypeComponents = ContainerHasPartialTypeComponents;
    }

    bool ContainerHasPartialTypeComponents()
    {
        return m_ContainerHasPartialTypeComponents;
    }
public:

    NEW_MUST_ZERO()

    Bindable
    (
        BCSYM_Container *ContainerContext,
        SourceFile *SourceFileOfContainer,
        ErrorTable *ErrorLog,
        NorlsAllocator *Allocator,
        CompilerHost *CompilerHost
    )
    {
        m_CurrentContainerContext = ContainerContext;
        m_SourceFile = SourceFileOfContainer;
        m_ErrorTable = ErrorLog;
        m_Allocator = Allocator;
        m_CompilerHost = CompilerHost;
        m_CompilationCaches = NULL;

        // This is needed because the MyClass stuff re-creates
        // bindable instance after they have been destroyed and
        // the container is bound.
        //
        if (ContainerContext->IsBindingDone())
        {
            CurrentBindingState = 0xFFFFFFFF;
        }

    }

    ~Bindable()
    {
        delete m_DelayedCheckManager;
    }

    void SetIDECompilationCaches(CompilationCaches *pCompilationCaches)
    {
#if IDE 
        m_CompilationCaches = pCompilationCaches;
#endif
    }

private:

    // Data Members

    CompilerHost *m_CompilerHost;
    SourceFile *m_SourceFile;
    ErrorTable *m_ErrorTable;
    NorlsAllocator *m_Allocator;
    BCSYM_Container *m_CurrentContainerContext;
    CompilationCaches *m_CompilationCaches;

    union
    {
        // !!!Note the number of bits used for m_Status... fields and the size of
        // CurrentBindingState need to be kept in sync.
        //
        struct
        {
            unsigned m_StatusOfResolvePartialTypes: 2;
            unsigned m_StatusOfResolvePartialTypesForNestedContainers: 2;
            unsigned m_StatusOfResolveBases: 2;
            unsigned m_StatusOfResolveNamedTypes: 2;
            unsigned m_StatusOfDetectStructMemberCycles: 2;
            unsigned m_StatusOfResolveShadowingOverloadingAndOverriding: 2;
            unsigned m_StatusOfValidateWithEventsVarsAndHookUpHandlers: 2;
            unsigned m_StatusOf----AttributesOnAllSymbolsInContainer: 2;
            unsigned m_StatusOfVerifyAttributesOnAllSymbolsInContainer : 2;
            unsigned m_StatusOfVerifyOperatorsInContainer : 2;
            unsigned m_StatusOfResolveImplementsInContainer: 2;
            unsigned m_StatusOfGenSyntheticCodeForContainer: 2;
            unsigned m_StatusOfScanContainerForObsoleteUsage: 2;
            unsigned m_StatusOfScanContainerForReturnTypeCheck: 2;
        };

        unsigned int CurrentBindingState : 28;
    };

    // Used to detect inheritance cycles
    //
    bool m_PossibleInheritanceCycleDetected: 1;

    // Used to detect embedded member cycles in structures.
    //
    bool m_StructureMemberCycleDetected: 1;

    // Is set if the container associated with this bindable
    // instance is a partial component of a type.
    //
    // Useful for optimizing some cases for non-partial types
    //
    bool m_ContainerHasPartialTypeComponents: 1;

    // Used to detect inheritance cycles
    //
    BCSYM_NamedType *m_CurrentBase;

    // Used to manage any obsolete checks that need to be delayed for
    // lack of attribute information till the time the container
    // context is ready for obsolete usage scanning.
    //
    DelayedCheckManager *m_DelayedCheckManager;

    // Helper data structures specific to Bindable

    struct BindableMemberInfo
    {
        BindableMemberInfo
        (
            BCSYM_NamedRoot *Member
        );

        STRING*
        GetName();

        bool
        IsSynthetic();

        void
        SetIsBad();

        Location*
        GetLocationToReportError();

        // Data Members

        BCSYM_NamedRoot *Member;

        // Actual type is "ShadowingOverloadingOverridingInfo", but C++ does not allow some bit operations like
        // &=, |=, etc. on enums, so...
        //
        unsigned ShadowingInfo;

        // If member is synthetic, then this is the user source symbol based on which the synthetic symbol was
        // synthesize. eg:
        //    - Property foo for synthetic method get_foo,
        //    - Base withevents variable for the property generated when handling an event of the base withevents variable.
        //
        BCSYM_NamedRoot *SourceOfSyntheticMember;

        // if member is synthetic, then this is the user source symbol that caused the synthesis of the synthetic
        // symbol. eg:
        //    - Property foo for synthetic method get_foo,
        //    - The Handling method for the property generated when handling an event of the base withevents variable.
        //
        BCSYM_NamedRoot *NamedContextOfSyntheticMember;

        SyntheticContext syntheticContext;
    };

    struct MemberInfoIter
    {
        MemberInfoIter
        (
            BCSYM_Container *Container,
            Compiler *CompilerInstance
        );

        BindableMemberInfo*
        GetNext
        (
            bool fSkipBad = true
        );

        BindableMemberInfo*
        GetNextOfSameName
        (
            bool fSkipBad = true
        );

        void
        StartPeeking();

        BindableMemberInfo*
        PeekAtNextOfSameName
        (
            bool fSkipBad = true
        );

        void
        Reset();

        ~MemberInfoIter();

      private:

        BindableMemberInfo*
        CreateMemberInfosForMembers
        (
            unsigned NumberOfMembers
        );

        void
        FreeMemberInfos();

        BindableMemberInfo*
        GetNextMemberInfo
        (
            bool fSkipBad
        );

        BindableMemberInfo*
        GetNextMemberInfoOfSameName
        (
            unsigned &CurrentMemberIndex,
            bool fSkipBad
        );

        // Data Members

        Compiler *m_CompilerInstance;

        BindableMemberInfo *m_MemberInfos;
        unsigned m_NumberOfMembers;

        unsigned m_NextMemberIndex;
        unsigned m_NextPeekIndex;
        STRING *m_CurrentMemberName;
        BindableMemberInfo *m_CurrentMemberInfo;
    };

    struct BasesIter
    {
        BasesIter
        (
            BCSYM_Container *Container
        );

        BCSYM_Container*
        GetNextBase
        (
            BCSYM_GenericTypeBinding **BaseGenericBinding = NULL
        );

        inline BCSYM_Implements*
        GetCurrentImplements()
        {
            return m_LinkToCurrentBase;
        }

      private:

        BCSYM*
        GetFirst();

        BCSYM*
        GetNext();

        static BCSYM*
        GetBaseInterface
        (
            BCSYM_Implements *&LinkToNextBaseOnSameLevel
        );

        BCSYM_Implements *m_LinkToCurrentBase;    // for interfaces only, classes have only single inheritance
                                                // for interfaces, this link helps to find the next Base at the same level


        BCSYM_Container *m_ClassOrInterface;

        bool m_IteratorStarted;

    };


    // Case insensitive hash table used to temporarily hash Members
    struct MembersHashTable
    {
      public:
        void Add(BCSYM_NamedRoot *Member)
        {
            STRING_INFO *StringInfo = StringPool::Pstrinfo(Member->GetName());
            m_Hash->Add(StringInfo, Member);
        }

        void Remove(BCSYM_NamedRoot *Member)
        {
            STRING_INFO *StringInfo = StringPool::Pstrinfo(Member->GetName());
            m_Hash->Remove(StringInfo, Member);
        }

        BCSYM_NamedRoot* Find(_In_z_ STRING *Key)
        {
            STRING_INFO *StringInfo = StringPool::Pstrinfo(Key);

            BCSYM_NamedRoot **pFoundMember = (BCSYM_NamedRoot **)m_Hash->Find(StringInfo);
            if (pFoundMember)
            {
                m_FindKey = Key;
                return *pFoundMember;
            }
            else
            {
                return NULL;
            }
        }

        BCSYM_NamedRoot* FindNextOfSameName()
        {
            BCSYM_NamedRoot **pFoundMember;

            while ((pFoundMember = (BCSYM_NamedRoot **)m_Hash->FindNext()) &&
                   (!StringPool::IsEqual((*pFoundMember)->GetName(), m_FindKey)));


            if (pFoundMember)
            {
                return *pFoundMember;
            }
            else
            {
                return NULL;
            }
        }

        int GetNumberOfEntries()
        {
            return m_Hash->NumEntries();
        }

        void ResetHashIterator()
        {
            m_Hash->ResetHashIterator();
        }

        // Iterate over all elements of the hash
        BCSYM_NamedRoot* GetNext()
        {
            BCSYM_NamedRoot **pMember = (BCSYM_NamedRoot **)m_Hash->IterateHash();

            if (pMember)
            {
                return *pMember;
            }

            return NULL;
        }

        MembersHashTable()
        {
            m_Allocator = new NorlsAllocator(NORLSLOC);
            m_Hash = new HashTable<256>(m_Allocator);
            m_FindKey = NULL;
        }

        ~MembersHashTable()
        {
            delete m_Hash;
            delete m_Allocator;
        }

      private:
        STRING* m_FindKey;
        NorlsAllocator *m_Allocator;
        HashTable<256> *m_Hash;
    };

    struct DelayedCheckManager
    {
        DelayedCheckManager
        (
            Compiler *CompilerInstance,
            CompilerHost *CompilerHost,
            BCSYM_Container *ContainerContext
        ) :
            m_Compiler(CompilerInstance),
            m_CompilerHost(CompilerHost),
            m_Allocator(NORLSLOC),
            m_ListOfDelayedObsoleteChecks(false),
            m_ListOfDelayedReturnTypeChecks(false),
            m_ContainerContext(ContainerContext)
        {
        };

        ~DelayedCheckManager()
        {
            ClearListOfDelayChecks<DelayedObsoleteCheckInfo>(&m_ListOfDelayedObsoleteChecks);
            ClearListOfDelayChecks<DelayedReturnTypeCheckInfo>(&m_ListOfDelayedReturnTypeChecks);
        }

        template <typename CheckInfoType>
        void
        ClearListOfDelayChecks
        (
            CDoubleList<CheckInfoType>* listOfDelayedChecks
        )
        {
            CDoubleListForwardIter<CheckInfoType> iterator(listOfDelayedChecks);
            CheckInfoType * pCur = NULL;
            while ((pCur = iterator.Next()))
            {
                if (pCur->ShouldBeRemovedFromErrorTable() && pCur->m_ErrorLogToReportObsoleteError)
                {
                    pCur->m_ErrorLogToReportObsoleteError->RemoveDependency(pCur);
                }
            }
        }


        void
        AddDelayedObsoleteCheck
        (
            BCSYM *PossibleObsoleteSymbol,
            _In_ Location *LocationOfSymbolUsage,
            ErrorTable *ErrorLogToReportObsoleteError,
            Declaration *ContextOfSymbolUsage
        )
        {
            DelayedObsoleteCheckInfo *NewDelayedObsoleteCheck = new (m_Allocator) DelayedObsoleteCheckInfo
                (
                    PossibleObsoleteSymbol,
                    *LocationOfSymbolUsage,
                    ErrorLogToReportObsoleteError,
                    &m_ListOfDelayedObsoleteChecks,
                    ContextOfSymbolUsage
                );

            if (ErrorLogToReportObsoleteError && ErrorLogToReportObsoleteError->IsTransient())
            {
                ErrorLogToReportObsoleteError->RecordDependency(NewDelayedObsoleteCheck);
            }

            m_ListOfDelayedObsoleteChecks.InsertFirst(NewDelayedObsoleteCheck);
        }

        void
        AddDelayedReturnTypeCheck
        (
            Member *MemberToCheck,
            const Location *LocationOfMemberToCheck,
            ErrorTable *ErrorLogToReportObsoleteError
        )
        {
            DelayedReturnTypeCheckInfo *NewDelayedCheck = new (m_Allocator) DelayedReturnTypeCheckInfo
                (
                    MemberToCheck,
                    *LocationOfMemberToCheck,
                    ErrorLogToReportObsoleteError,
                    &m_ListOfDelayedReturnTypeChecks
                );

            if (ErrorLogToReportObsoleteError && ErrorLogToReportObsoleteError->IsTransient())
            {
                ErrorLogToReportObsoleteError->RecordDependency(NewDelayedCheck);
            }

            m_ListOfDelayedReturnTypeChecks.InsertFirst(NewDelayedCheck);

        }

        void
        CompleteDelayedObsoleteChecks();

        void
        CompleteDelayedResultTypeChecks();

    private:

        template <typename CheckInfoType>
        struct DelayedCheckInfo :
            // Provides a wrapper so that an instance of this type can be
            // inserted into a single linked list
            public CDoubleLink<CheckInfoType>,
            public ErrorTableDependency
        {
            DelayedCheckInfo
            (
                _In_ const Location & LocationOfSymbolUsage,
                ErrorTable *ErrorLogToReportObsoleteError,
                _In_ CDoubleList<CheckInfoType> * pList
            ) :
                m_LocationOfSymbolUsage(LocationOfSymbolUsage),
                m_ErrorLogToReportObsoleteError(ErrorLogToReportObsoleteError),
                m_pListBackPointer(pList)
            {

            }

            Location m_LocationOfSymbolUsage;
            ErrorTable *m_ErrorLogToReportObsoleteError;
            CDoubleList<CheckInfoType> * m_pListBackPointer;

            void NotifyTableReplacement
            (
                ErrorTable * pOldErrorTable,
                ErrorTable * pNewErrorTable
            )
            {
                m_ErrorLogToReportObsoleteError = pNewErrorTable;
            }

            void NotifyTableDestruction
            (
                ErrorTable * pOldErrorTable
            )
            {
                if (m_pListBackPointer)
                {
                    m_ErrorLogToReportObsoleteError = NULL;
                    m_pListBackPointer->Remove((CheckInfoType*)this);
                    m_pListBackPointer = NULL;
                }
            }

        };

        struct DelayedObsoleteCheckInfo :
            DelayedCheckInfo<DelayedObsoleteCheckInfo>
        {
            DelayedObsoleteCheckInfo
            (
                BCSYM *PossibleObsoleteSymbol,
                const Location & LocationOfSymbolUsage,
                ErrorTable *ErrorLogToReportObsoleteError,
                CDoubleList<DelayedObsoleteCheckInfo> * pList,
                Declaration *ContextOfSymbolUsage
            ) :
                m_PossibleObsoleteSymbol(PossibleObsoleteSymbol),
                DelayedCheckInfo(LocationOfSymbolUsage, ErrorLogToReportObsoleteError, pList),
                m_ContextOfSymbolUsage(ContextOfSymbolUsage)
            {
            };

            BCSYM *m_PossibleObsoleteSymbol;
            // The context in which the symbol was used.
            Declaration *m_ContextOfSymbolUsage;
        };

        struct DelayedReturnTypeCheckInfo :
        DelayedCheckInfo<DelayedReturnTypeCheckInfo>
        {
            DelayedReturnTypeCheckInfo
            (
                Member *MemberToCheck,
                const Location & LocationOfMemberToCheck,
                ErrorTable *ErrorLogToReportObsoleteError,
                CDoubleList<DelayedReturnTypeCheckInfo> * pList
            ) :
                m_MemberToCheck(MemberToCheck),
                DelayedCheckInfo(LocationOfMemberToCheck, ErrorLogToReportObsoleteError, pList)
            {
            };

            Member *m_MemberToCheck;
           };

        NorlsAllocator  m_Allocator;
        Compiler*       m_Compiler;
        CompilerHost*   m_CompilerHost;
        CDoubleList<DelayedObsoleteCheckInfo> m_ListOfDelayedObsoleteChecks;
        CDoubleList<DelayedReturnTypeCheckInfo> m_ListOfDelayedReturnTypeChecks;
        BCSYM_Container *m_ContainerContext;
    };


    struct OperatorInfo : CSingleLink<OperatorInfo>
    {
        BCSYM_UserDefinedOperator *Symbol;
    };

    class RelationalOperatorPool
    {
        // This class tracks pair-wise declarations of operators.
        //
        // As we iterate over each member during verifcation, we consider adding it to the pool.
        // Only operators which require pair-wise declarations (-, <> or IsTrue, IsFalse) are
        // considered.  At every point during the process, only unmatched operators exist in the
        // pool.  When the verfication process is complete, we walk all declarations in the pool
        // and report an error on each.
        //
        // Algorithm description:
        //
        //     Given pool P and container C
        //
        //     1. For each operator declaration D in C:
        //            - Determine E, the pair-wise match of D.
        //            - If E does not exist in P, D is added to P.
        //            - If E exists in P, E is removed from P.
        //
        //     2. For each operator declaration U in P:
        //            - Report an error on U (because it is unmatched).
        //
        //     E is the pair-wise match of D if the name of E equals MapToMatching(name of D) and
        //     the signatures and return types of D and E are identical.
        //
        //     Matching Example:
        //         Operator >=(x As Integer, y As Integer) As Double
        //         Operator <=(x As Integer, y As Integer) As Double
        //
        //     MapToMatching(name) looks up the opposite name for the pair:
        //              >   <
        //             >=   <=
        //              =   <>
        //         IsTrue   IsFalse
        //

    public:

        RelationalOperatorPool
        (
            NorlsAllocator &Allocator
        ) :
            m_Allocator(Allocator)
        {
#if DEBUG
            IsCollated = false;
#endif
        };

        void
        Add
        (
            UserDefinedOperators Operator,
            BCSYM_UserDefinedOperator *Symbol
        );

        CSingleList<OperatorInfo> *
        CollateUnmatchedOperators();

    private:

        OperatorInfo *
        Search
        (
            CSingleList<OperatorInfo> &SearchList,
            BCSYM_UserDefinedOperator *SymbolToMatch
        );

        void
        AddMatchingOperator
        (
            CSingleList<OperatorInfo> &AddList,
            CSingleList<OperatorInfo> &SearchList,
            BCSYM_UserDefinedOperator *Symbol
        );

        NorlsAllocator &m_Allocator;

        CSingleList<OperatorInfo> m_IsTrue;
        CSingleList<OperatorInfo> m_IsFalse;
        CSingleList<OperatorInfo> m_Equal;
        CSingleList<OperatorInfo> m_NotEqual;
        CSingleList<OperatorInfo> m_Less;
        CSingleList<OperatorInfo> m_LessEqual;
        CSingleList<OperatorInfo> m_GreaterEqual;
        CSingleList<OperatorInfo> m_Greater;

#if DEBUG
        bool IsCollated;
#endif
    };

    struct MemberBindingHashTable
    {
        MemberBindingHashTable
        (
            _In_ NorlsAllocator *Allocator
        ) : m_Hash(Allocator)
        {
        }

        bool Exists
        (
            BCSYM_NamedRoot *Member,
            BCSYM_GenericTypeBinding *Binding
        )
        {
            for(BCSYM_GenericTypeBinding **BindingInHash = m_Hash.HashFind(Member);
                BindingInHash;
                BindingInHash = m_Hash.FindNext())
            {
                if (Binding == *BindingInHash ||
                    (Binding &&
                        *BindingInHash &&
                        BCSYM_GenericTypeBinding::AreTypeBindingsEqual(
                            Binding,
                            *BindingInHash)))
                {
                    return true;
                }
            }

            return false;
        }

        // Returns true if already exists without adding again,
        // else adds to hash and returns false
        //
        bool Add
        (
            BCSYM_NamedRoot *Member,
            BCSYM_GenericTypeBinding *Binding
        )
        {
            if (Exists(Member, Binding))
            {
                return true;
            }

            m_Hash.HashAdd(Member, Binding);

            return false;
        }

        BCSYM_GenericTypeBinding *GetUnifyingMemberBinding
        (
            BCSYM_NamedRoot *Member,
            BCSYM_GenericTypeBinding *Binding,
            Symbols *SymbolFactory,
            Compiler *CompilerInstance
        );

      private:
        FixedSizeHashTable<256, BCSYM_NamedRoot *, BCSYM_GenericTypeBinding *> m_Hash;
    };

public:

    void
    AddToListOfDelayedObsoleteChecks
    (
        BCSYM *PossibleObsoleteSymbol,
        _In_ Location *LocationOfSymbolUsage,
        ErrorTable *ErrorLogToReportObsoleteError,
        Declaration *ContextOfSymbolUsage
    )
    {
        GetAndEnsureDelayedCheckManager()->AddDelayedObsoleteCheck(
            PossibleObsoleteSymbol,
            LocationOfSymbolUsage,
            ErrorLogToReportObsoleteError,
            ContextOfSymbolUsage);
    }

    void
    AddToListOfDelayedReturnTypeChecks
    (
        Member *MemberToCheck,
        const Location *LocationOfMemberToCheck,
        ErrorTable *ErrorLogToReportObsoleteError
    )
    {
        GetAndEnsureDelayedCheckManager()->AddDelayedReturnTypeCheck(
            MemberToCheck,
            LocationOfMemberToCheck,
            ErrorLogToReportObsoleteError);
    }

private:

    DelayedCheckManager*
    GetAndEnsureDelayedCheckManager()
    {

        if (!m_DelayedCheckManager)
        {
            m_DelayedCheckManager =
                new (zeromemory) DelayedCheckManager(
                        CurrentCompilerInstance(),
                        CurrentCompilerHost(),
                        CurrentContainer());
        }

        return m_DelayedCheckManager;
    }

public:

    // General enum to use to track progress of tasks
    enum TaskTriState
    {
        NotStarted = 0,
        InProgress = 1,
        // the reason this is 3 is because the filling up pattern (all 1's) is simpler
        // currently it is used by bindable to indicate all tasks as done.
        Done = 3
    };

private:

    enum SyntheticContext
    {
        UnKnown,
        PropertyGetOrSet,   // Property Get or Property Set
        EventDecl,          // Event Add, Remove, Delegate or Variable
        WithEventsDecl,     // Withevents Property, Variable
        HandlesOfBaseWithEvents,
        OperatorMethod,     // Synthetic method created for each Operator
        AutoPropertyBackingField    // Auto property backing field
    };

    enum ShadowingOverloadingOverridingInfo
    {
        Overloads = 1,
        Overrides = 2,
        Shadows = 4,
        ImplicitShadows = 8,     // if a member is shadowing implicitly, then both the Shadows and ImplicitShadows should be specified
        Bad = 16
    };
};
