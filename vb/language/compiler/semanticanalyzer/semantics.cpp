//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Implementation of VB semantic analysis infrastructure.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#if IDE 
#define TMH_FILENAME "semantics.tmh"
#include "..\..\Include\EnableLogging.def"
#endif


unsigned long ProcedureDescriptor::FreeTypeArgumentCount()
{
    return
        Binding.IsNull() || ! Binding.WasEverPartial() ?
            Proc->GetGenericParamCount() :
            Binding.FreeTypeArgumentCount();
}

ILTree::Expression * Semantics::InitializeConstructedVariable
(
    _Inout_ ILTree::Expression *VariableReference,
    _In_opt_ ParseTree::ArgumentList *Arguments,
    // Can be NULL. Location of the Type in "Dim x as New Type"
    const Location *TypeLoc
)
{
    // ISSUE: There is a regrettable amount of repeated code between this
    // method and the interpretation of New expressions.
    // It isn't practical to synthesize a New expression to interpret here,
    // because the type of the variable isn't available in parse tree form
    // and because instances of value types can be constructed here directly
    // (instead of introducing temporaries as is required for New expressions),
    // but it must be possible to share some of the logic.

    ILTree::Expression *ConstructorCall = NULL;
    ExpressionList *CopyOutArguments = NULL;
    Type *VariableType = VariableReference->ResultType;
    Type *ConstructedType = VariableType;

    if (TypeHelpers::IsGenericParameter(VariableType))
    {
        VSASSERT(VariableType->PGenericParam()->CanBeInstantiated(), "Non-new constrained type param unexpected!!!");

        if (Arguments)
        {
            // Arguments cannot be passed to a 'New' used on a type parameter.

            ReportSemanticError(
                ERRID_NewArgsDisallowedForTypeParam,
                Arguments->TextSpan);

            return AllocateBadExpression(VariableReference->Loc);
        }

        ILTree::NewExpression *New =
            &AllocateExpression(
                SX_NEW,
                VariableType,
                VariableReference->Loc)->AsNewExpression();
        New->Class = VariableType;

        return
            GenerateAssignment(
                VariableReference->Loc,
                VariableReference,
                New,
                false);
    }
    //
    // Creations of structures with no arguments use the built-in, nondeclarable,
    // uncallable parameterless constructor. Other creations call a constructor.
    //
    else if (TypeHelpers::IsReferenceType(VariableType) || Arguments)
    {
        // To facilitate COM2 interop, interfaces are decorated with a CoClass
        // attribute that points at the "real" type that a coclass is. So you can
        // say "New Recordset" where Recordset is an interface that has a CoClass
        // attribute that points at RecordsetClass class. So do the indirection here.
        if (TypeHelpers::IsInterfaceType(VariableType))
        {
            WCHAR *CoClassName;
            NorlsAllocator Scratch(NORLSLOC);

            if (!VariableType->GetPWellKnownAttrVals()->GetCoClassData(&CoClassName))
            {
                VSFAIL("How did this type get here?");
            }

            unsigned NameCount = m_Compiler->CountQualifiedNames(CoClassName);
            STRING **Names = (STRING **)Scratch.Alloc(VBMath::Multiply(
                sizeof(STRING *), 
                NameCount));
            bool IsBadCoClassName = false;

            m_Compiler->SplitQualifiedName(CoClassName, NameCount, Names);

            ConstructedType =
                Semantics::InterpretQualifiedName(
                    Names,
                    NameCount,
                    NULL,
                    NULL,
                    m_Compiler->GetUnnamedNamespace(m_Project)->GetHash(),
                    NameSearchIgnoreImports,
                    VariableReference->Loc,
                    NULL,
                    m_Compiler,
                    m_CompilerHost,
                    m_CompilationCaches,
                    NULL,
                    true,                              // perform obsolete checks
                    IsBadCoClassName);

            if (IsBadCoClassName && ConstructedType && !ConstructedType->IsGenericBadNamedRoot() &&
                !IsAccessible(ConstructedType->PNamedRoot(), NULL, m_Compiler->GetUnnamedNamespace(m_Project)))
            {
                ReportSemanticError(
                    ERRID_InAccessibleCoClass3,
                    VariableReference->Loc,
                    ConstructedType,
                    VariableType,
                    ConstructedType->PNamedRoot()->GetAccess());

                return AllocateBadExpression(VariableReference->Loc);
            }

            // Including IsBadCoClassName too here as a fall back for any other kind of badness
            // like obsoleteness etc., so that the user gets atleast the default error instead
            // of wrong compiling without any errors.
            //
            if (IsBadCoClassName || !ConstructedType || ConstructedType->IsBad() || !ConstructedType->IsClass())
            {
                ReportSemanticError(
                    ERRID_CoClassMissing2,
                    VariableReference->Loc,
                    CoClassName,
                    VariableType);

                return AllocateBadExpression(VariableReference->Loc);
            }
        }

        Declaration *Constructor = ConstructedType->PClass()->GetFirstInstanceConstructor(m_Compiler);

        if (Constructor == NULL)
        {
            ReportSemanticError(
                ERRID_ConstructorNotFound1,
                VariableReference->Loc,
                ConstructedType);

            return AllocateBadExpression(VariableReference->Loc);
        }

        bool InaccessibleConstructor = false;

        Constructor =
            CheckNamedRootAccessibility
            (
                Constructor,
                NULL,
                VariableReference->Loc,
                NameNoFlags,
                ContainingClass(),
                InaccessibleConstructor
            );

        if (InaccessibleConstructor)
        {
            return AllocateBadExpression(VariableReference->Loc);
        }

        ILTree::Expression *ConstructorReference =
            ReferToSymbol(
                TypeLoc != NULL ? *TypeLoc : VariableReference->Loc,
                Constructor,
                chType_NONE,
                NULL,
                DeriveGenericBindingForMemberReference(ConstructedType, Constructor, m_SymbolCreator, m_CompilerHost),
                ExprIsExplicitCallTarget | ExprIsConstructorCall);

        bool ReportErrors = m_ReportErrors;

        // Check for the special rule about supplying ADDRESSOF expressions
        // as arguments to delegate constructors.
        // If it applies, short-circuit the general processing.

        if (TypeHelpers::IsDelegateType(VariableType))
        {
            if (ArgumentsAllowedAsDelegateConstructorArguments(Arguments))
            {
                ILTree::Expression *DelegateValue =
                    InterpretExpressionWithTargetType(
                        Arguments->Element->Value,
                        ExprForceRValue,
                        VariableType);

                if (!IsBad(DelegateValue))
                {
                    return
                        GenerateAssignment(
                            VariableReference->Loc,
                            VariableReference,
                            DelegateValue,
                            false);
                }
                else
                {
                    return DelegateValue;
                }
            }
            else
            {
                ReportSemanticError(
                    ERRID_NoDirectDelegateConstruction1,
                    Arguments ?
                        Arguments->TextSpan :
                        (TypeLoc ? *TypeLoc : VariableReference->Loc),
                    VariableType);

                // Disable reporting other (bogus) errors for the construction.
                // (Semantic analysis of the construction must occur
                // so that Intellisense can provide tips.)
                m_ReportErrors = false;
            }
        }

        ConstructorCall =
            BindArgsAndInterpretCallExpression(
                VariableReference->Loc,
                ConstructorReference,
                chType_NONE,
                Arguments,
                CopyOutArguments,
                ExprResultNotNeeded | ExprIsConstructorCall,
                OvrldNoFlags,
                NULL);

        m_ReportErrors = ReportErrors;

#if 0
        // 




        // Because we have no language construct that can represent the value needed for
        // the function pointer param of a delegate constructor, we give an error.  The user
        // should be using AddressOf which is a compound representation of the object instance
        // and function pointer.

        if (!IsBad(ConstructorCall) &&
            TypeHelpers::IsDelegateType(VariableType) &&
            IsMagicDelegateConstructor(ViewAsProcedure(ConstructorReference->AsSymbolReferenceExpression().Symbol)))
        {
            ReportSemanticError(
                ERRID_UseAddressOf1,
                VariableReference->Loc,
                ConstructorReference->AsSymbolReferenceExpression().Symbol);

            MakeBad(ConstructorCall);
        }
#endif
        if (IsBad(ConstructorCall))
        {
            return ConstructorCall;
        }
    }

    ILTree::Expression *Result = NULL;

    if (TypeHelpers::IsValueType(VariableType) && !m_IsGeneratingXML)
    {
        // Construct the variable. If there is no constructor call to make,
        // issue a default initialization.

        if (ConstructorCall)
        {
            if (VariableReference->bilop == SX_SYM && VariableReference->AsSymbolReferenceExpression().pnamed->IsVariable())
            {
                //set this for definite assignment, it is an assignment not a call.
                SetFlag32(VariableReference,  SXF_SYM_ASGVIACALL);
            }
            ConstructorCall->AsCallExpression().MeArgument = MakeAddress(VariableReference, true);
            return AppendCopyOutArguments(ConstructorCall, CopyOutArguments, ExprResultNotNeeded);
        }
        else
        {
            ILTree::Expression *Init =
                AllocateExpression(
                    SX_INIT_STRUCTURE,
                    TypeHelpers::GetVoidType(),
                    Location::GetHiddenLocation());
            Init->AsInitStructureExpression().StructureReference = MakeAddress(VariableReference, true);
            Init->AsInitStructureExpression().StructureType = VariableType;

            VSASSERT(CopyOutArguments == NULL, "Astonishing presence of byref-by-copy arguments.");
            return Init;
        }
    }
    else
    {
        ILTree::Expression *New = NULL;
        if (!TypeHelpers::IsEmbeddableInteropType(VariableReference->ResultType))
        {
            New = AllocateExpression(SX_NEW, VariableReference->ResultType, VariableReference->Loc);
            New->AsNewExpression().Class = ConstructedType;
            New->AsNewExpression().ConstructorCall = ConstructorCall;
        }
        else
        {
            New = CreateInstanceComInteropNoPIA(VariableReference->ResultType, ConstructedType, VariableReference->Loc);
            // Fix for 499246: if the user is trying to instantiate a class from a No-PIA library,
            // instead of an interface, we must tell them to use the interface instead of its coclass.
            if (VariableReference->ResultType->IsClass() &&
                !VariableReference->ResultType->IsStruct() &&
                !VariableReference->ResultType->IsEnum())
            {
                ReportSemanticError(
                    ERRID_NewCoClassNoPIA, 
                    VariableReference->Loc, 
                    VariableReference->ResultType->PNamedRoot()->GetName());
            }
        }

        New = AppendCopyOutArguments(New, CopyOutArguments, ExprNoFlags);

        return
            GenerateAssignment(
                VariableReference->Loc,
                VariableReference,
                New,
                false);
    }
}

bool
Semantics::ArgumentsAllowedAsDelegateConstructorArguments
(
    _In_ ParseTree::ArgumentList *Arguments
)
{
    return Arguments != NULL &&
           Arguments->Element->Value &&
           (
            Arguments->Element->Value->Opcode == ParseTree::Expression::AddressOf ||
            Arguments->Element->Value->Opcode == ParseTree::Expression::Lambda
           ) &&
           Arguments->Next == NULL;
}

ILTree::Expression *
Semantics::InitializeConstructedVariable
(
    _Inout_ ILTree::Expression *VariableReference,
    _In_opt_ ParseTree::ArgumentList *Arguments,
    const Location *TypeLoc,          // Can be NULL. Location of the Type in "Dim x as New Type"
    _In_opt_ ParseTree::ObjectInitializerList *ObjectInitializer, // Can be NULL - Initializers for members of the constructed object.
    _In_opt_ ParseTree::BracedInitializerList * CollectionInitializer,
    const ParseTree::PunctuatorLocation * FromLocation,
    const Location * Loc
)
{
    ThrowIfFalse(!CollectionInitializer || Loc);
    ThrowIfFalse(!CollectionInitializer || FromLocation);
    
    AssertIfNull(VariableReference);

    bool InitializerListEmpty =
        (NULL == ObjectInitializer ||
            NULL == ObjectInitializer->BracedInitializerList ||
            NULL == ObjectInitializer->BracedInitializerList->InitialValues);

    if (InitializerListEmpty && ! CollectionInitializer)
    {
        return
            InitializeConstructedVariable(
                VariableReference,
                Arguments,
                TypeLoc);
    }
    else if (CollectionInitializer)
    {
        ParserHelper ph(&m_TreeStorage, *Loc);

        ParseTree::CollectionInitializerExpression * pColInit = 
            ph.CreateCollectionInitializer
            (
                ph.CreateNewObject
                (
                    ph.CreateBoundType
                    (
                        VariableReference->ResultType,
                        TypeLoc ? *TypeLoc : *Loc
                    ),
                    Arguments
                ),
                *FromLocation,
                CollectionInitializer
            );

        return 
            GenerateAssignment
            (
                *Loc, 
                VariableReference, 
                InterpretExpression
                (
                    pColInit,
                    ExprNoFlags
                ),
                false
            );
    }

    // If the variable being initialized with an "As new Type with " expression is a value
    // type, then the variable needs to be initialized in place so that the intermediate
    // changes to the variable are noticeable in user code. But if the variable is a
    // reference type, then the object needs to be initialized completely before it is
    // assigned to the variable so that the initialization is all or none. This is consistent
    // with how "As New Type" initialization already works.
    //
    // So for implementation,
    //
    // - for values types, the actual variable itself is initialized so that the intermediate
    //     state of initialization are noticed from the variable itself during the initialization
    //     process.
    //
    // - a temp is created for reference types when the "With" clause is used to specify
    //     additional initialization on the obect so that the temp can be used to complete
    //     the rest of the initialization on the constructed object successfully before
    //     the obect is assigned to the variable.
    //
    // - for reference types, if no initializer is specified with the "With" clause, then
    //     no temp is created and the result of the "New" expression is assigned to the
    //     variable as soon as the object is constructed successfully.

    ILTree::Expression *ReferenceToInit = VariableReference;
    Variable *Temp = NULL;

    if (!TypeHelpers::IsValueType(VariableReference->ResultType))
    {
        Temp = AllocateShortLivedTemporary(VariableReference->ResultType);

        ReferenceToInit =
            AllocateSymbolReference(
                Temp,
                VariableReference->ResultType,
                NULL,   // No base reference for local
                VariableReference->Loc);
    }

    ILTree::Expression *ConstructedInstance =
        InitializeConstructedVariable(
            ReferenceToInit,
            Arguments,
            TypeLoc);

    AssertIfNull(ConstructedInstance);

    if (IsBad(ConstructedInstance))
    {
        return ConstructedInstance;
    }

    Location WithClauseTextSpan = ObjectInitializer->With;
    if (ObjectInitializer->BracedInitializerList)
    {
        WithClauseTextSpan.m_lEndLine = ObjectInitializer->BracedInitializerList->TextSpan.m_lEndLine;
        WithClauseTextSpan.m_lEndColumn = ObjectInitializer->BracedInitializerList->TextSpan.m_lEndColumn;
    }

    ILTree::Expression *Initialization =
        InitializeObject(
            ObjectInitializer->BracedInitializerList,
            ReferenceToInit,
            WithClauseTextSpan,
            ExprNoFlags);

    AssertIfNull(Initialization);

    ILTree::Expression *Result =
             AllocateExpression(
                SX_SEQ_OP2,
                TypeHelpers::GetVoidType(),
                ConstructedInstance,
                Initialization,
                VariableReference->Loc);    // 

    if (Temp)
    {
        AssertIfFalse(ReferenceToInit != VariableReference);

        // If a temp was created and temp was initialized, then the temp needs to be copied
        // to the actual variable to be initialized.
        // We must call GenerateAssignment because VariableReference might be a property reference!

        ILTree::Expression *Copy =
            GenerateAssignment(
                VariableReference->Loc,
                VariableReference,
                AllocateSymbolReference(
                    Temp,
                    VariableReference->ResultType,
                    NULL,   // No base reference for local
                    VariableReference->Loc),
                false);

#if 0
        ILTree::Expression *Copy =
            AllocateExpression(
                SX_ASG,
                TypeHelpers::GetVoidType(),
                VariableReference,
                AllocateSymbolReference(
                    Temp,
                    VariableReference->ResultType,
                    NULL,   // No base reference for local
                    VariableReference->Loc),
                VariableReference->Loc);    // 
#endif

        Result =
             AllocateExpression(
                SX_SEQ_OP2,
                TypeHelpers::GetVoidType(),
                Result,
                Copy,
                VariableReference->Loc);    // 
    }

    return Result;
}

static bool
IsFieldToInitialize
(
    Declaration *Member,
    bool InitializeSharedFields
)
{
    // Because metadata cannot express decimal/date constants,
    // we have to emit them as shared readonly fields with initializers. So
    // we have to explicitly include them here.
    return
        !IsBad(Member) &&
        Member->IsVariable() &&
        ((!Member->PVariable()->IsConstant() &&
          Member->PVariable()->IsShared() == InitializeSharedFields &&
          (Member->IsVariableWithValue() ||
           Member->IsVariableWithArraySizes())) ||
         (Member->PVariable()->IsConstant() &&
          (Member->PVariable()->GetType()->GetVtype() == t_decimal ||
           Member->PVariable()->GetType()->GetVtype() == t_date) &&
          InitializeSharedFields));
}

bool
Semantics::IsDesignerControlClass
(
    ClassOrRecordType *PossibleDesignerControl,
    bool SearchThisSymbolOnly /* = false */
)
{
    AssertIfNull(PossibleDesignerControl);

    if (TypeHelpers::IsClassType(PossibleDesignerControl))
    {
        if (!SearchThisSymbolOnly)
        {
            return PossibleDesignerControl->GetPWellKnownAttrVals()->GetDesignerGeneratedData();
        }
        else
        {
            AttrVals * pAttrVals = PossibleDesignerControl->GetPAttrVals();
            if (pAttrVals)
            {
                // See if we have the DesignerGenerated attribute.
                for (BCSYM_ApplAttr * pApplAttr = pAttrVals->GetPsymApplAttrHead(); pApplAttr; pApplAttr = pApplAttr->GetNext())
                {
                    BCSYM_NamedType *pNamedType = pApplAttr->GetAttrClass();
                    if (pNamedType)
                    {
                        StringBuffer qualifiedName;
                        unsigned countOfNames = pNamedType->GetNameCount();
                        STRING **ppSplitName = pNamedType->GetNameArray();

                        for(unsigned i = 0; i < countOfNames; i++)
                        {
                            if (ppSplitName[i])
                            {
                                qualifiedName.AppendString(ppSplitName[i]);

                                if (i != countOfNames - 1)
                                {
                                    qualifiedName.AppendChar( L'.');
                                }
                            }
                        }

                        Compiler *pCompiler = PossibleDesignerControl->GetCompiler();
                        if (StringPool::IsEqual(pCompiler->AddString(&qualifiedName), pCompiler->AddString(DESIGNERGENERATEDATTRIBUTE)))
                        {
                            return true;
                        }

                        // Try name with "Attribute" suffix.
                        qualifiedName.AppendString(STRING_CONST(pCompiler, ComAttribute));
                        if (StringPool::IsEqual(pCompiler->AddString(&qualifiedName), pCompiler->AddString(DESIGNERGENERATEDATTRIBUTE)))
                        {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool
Semantics::IsInitializeComponent
(
    Compiler *CompilerToUse,
    Declaration *PossibleInitializeComponent
)
{
    AssertIfNull(PossibleInitializeComponent);

    if (PossibleInitializeComponent != NULL &&
        StringPool::IsEqual(PossibleInitializeComponent->GetName(), STRING_CONST(CompilerToUse, InitializeComponent)) &&
        PossibleInitializeComponent->IsMethodImpl() &&
        !PossibleInitializeComponent->IsBad() &&
        PossibleInitializeComponent->PProc()->GetFirstParam() == NULL &&
        PossibleInitializeComponent->PProc()->GetType() == NULL &&
        !PossibleInitializeComponent->PProc()->IsShared())
    {
        return true;
    }

    return false;
}

void
Semantics::InitializeFields
(
)
{
    bool InitializeSharedFields = WithinSharedProcedure();

    // Disable creating implicit declarations for interpreting initializers
    // of fields.

    bool CreateImplicitDeclarations = m_CreateImplicitDeclarations;
    m_CreateImplicitDeclarations = false;

    GenericBinding *GenericBindingContext = NULL;

    if (IsGenericOrHasGenericParent(ContainingClass()))
    {
        // Create a generic type binding that includes the parameters of the generic type as arguments.

        GenericBindingContext = SynthesizeOpenGenericBinding(ContainingClass(), m_SymbolCreator);
    }

    // All fields of a structure must be initialized. Accomplish this by
    // initializing the entire structure.

    if (TypeHelpers::IsRecordType(ContainingClass()) &&
        !InitializeSharedFields)
    {
        ILTree::Expression *StructureReference =
            AllocateSymbolReference(
                ContainingClass()->GetMe(),
                ContainingClass(),
                NULL,
                m_BlockContext->Loc);

        if (GenericBindingContext)
        {
            StructureReference->AsSymbolReferenceExpression().GenericBindingContext = GenericBindingContext;
            StructureReference->ResultType = GenericBindingContext;
        }

        Location Loc = Location::GetHiddenLocation();

        ILTree::Expression *Init =
            AllocateExpression(
                SX_INIT_STRUCTURE,
                TypeHelpers::GetVoidType(),
                Loc);
        Init->AsInitStructureExpression().StructureReference = MakeAddress(StructureReference, true);
        Init->AsInitStructureExpression().StructureType = StructureReference->ResultType;

        ILTree::Statement *InitStmt = AllocateStatement(SL_STMT, Loc, NoResume);
        InitStmt->AsStatementWithExpression().Operand1 = Init;

        // Indidate that this statement is an initialization statement in an
        // instance constructor so that it is generated outside the try block
        // generated for the on error statement.
        // Bug VSWhidbey 204996
        //
        SetFlag32(InitStmt, SLF_INIT_ME);
    }

    // Iterate over the class's members, and generate code to initialize
    // appropriate fields.

    // ISSUE: The fields should be initialized in declaration order. Were the
    // compiler symbol table well-designed, the fields would be available in
    // declaration order. However, this is not the case. Therefore, it is
    // necessary to sort the members by declaration order here.

    // First count the number of fields to initialize.

    unsigned InitializedFieldCount = 0;
    BCITER_CHILD MemberIterator(ContainingClass());
    for (Declaration *Member = MemberIterator.GetNext();
         Member;
         Member = MemberIterator.GetNext())
    {
        if (IsFieldToInitialize(Member, InitializeSharedFields))
        {
            InitializedFieldCount++;
        }
    }

    // Populate an array with the initialized fields.

    const unsigned InitializedFieldsScratchCount = 12;
    Variable *InitializedFieldsScratch[InitializedFieldsScratchCount];
    Variable **InitializedFields = InitializedFieldsScratch;

    if (InitializedFieldCount > InitializedFieldsScratchCount)
    {
        InitializedFields = new(m_TreeStorage) Variable*[InitializedFieldCount];
    }

    InitializedFieldCount = 0;
    MemberIterator.Init(ContainingClass());
    for (Declaration *Member = MemberIterator.GetNext();
         Member;
         Member = MemberIterator.GetNext())
    {
        if (IsFieldToInitialize(Member, InitializeSharedFields))
        {
#pragma prefast(suppress: 26010, "InitializedFields is correctly bounded")
            InitializedFields[InitializedFieldCount++] = Member->PVariable();
        }

        else if (IsProcedureDefinition(Member))
        {
            // Generate AddHandler calls for methods that handle events generated
            // by WithEvents fields in base classes.

            // If it's a partial method declaration, just abort.

            if( !ViewAsProcedure(Member)->IsPartialMethodDeclaration() )
            {
                BCITER_Handles iterHandles(ViewAsProcedureDefinition(Member));
                for (HandlesList *Handles = iterHandles.GetNext();
                     Handles;
                     Handles = iterHandles.GetNext())
                {
                    if (!Handles->IsBadHandlesList() && (Handles->IsMyBase() || Handles->IsEventFromMeOrMyClass()) &&
                        ((!InitializeSharedFields && !(ViewAsProcedureDefinition(Member)->IsShared() && Handles->GetEvent()->IsShared())) ||
                         (InitializeSharedFields && (ViewAsProcedureDefinition(Member)->IsShared() && Handles->GetEvent()->IsShared()))))    //Shared Method Handling Shared MyBase.Event should be added in the shared constructor
                    {
                        InterpretMyBaseHandler(ViewAsProcedure(Member), Handles);
                    }
                }
            }
        }
    }

#if IDE 
    if (m_Project->GenerateENCableCode())
    {
        // Also add object instances tracking list for EnC.
        // Add the following to the constructor of the class
        //
        //     __ENCAddToList(Me)
        //
        // Need to do this only for an instance constructor
        //
        if (!WithinSharedProcedure())
        {
            GenerateENCTrackingList(ContainingClass());
        }
    }
#endif IDE

    // Sort the initialized fields by declaration order.
    qsort(InitializedFields, InitializedFieldCount, sizeof(Declaration *), SortSymbolsByLocation);
    ErrorTable *SavedErrors = m_Errors;
    Namespace *SavedNamespaceForNameLookupInImports = m_UnnamedNamespaceForNameLookupInImports;
    SourceFile *SavedSourceFile = m_SourceFile;

    // Initialized the fields.
    for (unsigned FieldIndex = 0;
         FieldIndex < InitializedFieldCount;
         FieldIndex++, m_TemporaryManager->FreeShortLivedTemporaries())
    {
        Variable *Member = InitializedFields[FieldIndex];
        bool MungedDecimalConstant = false;
        SourceFile *FileContainingMember = Member->GetSourceFile();

        m_UnnamedNamespaceForNameLookupInImports = SavedNamespaceForNameLookupInImports;
        m_Errors = SavedErrors;
        m_SourceFile = SavedSourceFile;

        // For partial classes, members could come from different files and hence their initializers'
        // code could come from different files. So the file level imports used for name lookup should
        // be from the appropriate file. Also errors need to be reported into the appropriate error table.
        //
        if (FileContainingMember &&
            FileContainingMember != m_SourceFile)
        {
            // Bug VSWhidbey 564823.
            m_SourceFile = FileContainingMember;

            m_UnnamedNamespaceForNameLookupInImports = FileContainingMember->GetUnnamedNamespace();

            if (m_AltErrTablesForConstructor)
            {
                ErrorTable *AlternateErrorTable = NULL;

                if (m_ReportErrors && SavedErrors)
                {
                    AlternateErrorTable = m_AltErrTablesForConstructor(FileContainingMember);
                }

                if (AlternateErrorTable != NULL)
                {
                    m_Errors = AlternateErrorTable;
                }
            }
        }

        // Note (Microsoft): Since decimal/date constants cannot be expressed in metadata, we have to
        // emit them as shared readonly fields that have initializers. Rather than try to express
        // this duality throughout semantics, we're going to simulate it for a moment right here,
        // munging the symbol and then munging it back. Consider: a semantic implementation for this.
        if (IsConstant(Member) &&
            (TypeHelpers::IsDecimalType(GetDataType(Member)) || TypeHelpers::IsDateType(GetDataType(Member))))
        {
            // Make it non-constant, shared
            Member->SetVarkind(VAR_Member);
            Member->SetIsShared(true);
            MungedDecimalConstant = true;
        }

        ILTree::Expression *MemberReference = NULL;
        if (Member->IsWithEvents())
        {
            bool NameIsBad = false;

            Symbol * MemberProperty =
                LookupName(
                    ViewAsScope(ContainingClass()),
                    NULL,   // No Type parameter lookup
                    Member->GetName(),
                    NameSearchImmediateOnly,
                    ContainingClass(),
                    *Member->GetLocation(),
                    NameIsBad,
                    NULL,  // we already have the binding context, so no need to get it again
                    -1); // We are looking for a class member

            if (NameIsBad ||
                MemberProperty == NULL ||
                !IsProperty(MemberProperty))
            {
                // Something is wrong with the declaration. Trust that
                // declaration semantics has reported an appropriate error.

                continue;
            }

            MemberReference =
                AllocateExpression(
                    SX_PROPERTY_REFERENCE,
                    GetReturnType(ViewAsProcedure(MemberProperty)),
                    ReferToSymbol(
                        *Member->GetLocation(),
                        ViewAsProcedure(MemberProperty),
                        chType_NONE,
                        NULL,
                        GenericBindingContext,
                        ExprIsAssignmentTarget | ExprIsExplicitCallTarget),
                    NULL,
                    Member->HasLocation() ? *Member->GetLocation() : Location::GetHiddenLocation());
        }
        else if (Member->IsAutoPropertyBackingField())
        {
            // We initialize autoproperties through the setter, so we will initialize a temporary variable
            // and pass it to the property through its setter.
            Variable* pTemp = AllocateShortLivedTemporary(Member->GetType());

            // Create a SymbolReferenceExpression that has the location of the auto property backing field
            // but references the temporary variable.
            // We use the backing field's location because the location of the Initialization ILTree needs it.
            // pTemp does not have a location.
             MemberReference =
                ReferToSymbol(
                    Member->HasLocation() ? *Member->GetLocation() : Location::GetHiddenLocation(),
                    pTemp,
                    chType_NONE,
                    NULL,
                    GenericBindingContext,
                    ExprIsAssignmentTarget);
        }
        else
        {
            MemberReference =
                ReferToSymbol(
                    Member->HasLocation() ? *Member->GetLocation() : Location::GetHiddenLocation(),
                    Member,
                    chType_NONE,
                    NULL,
                    GenericBindingContext,
                    ExprIsAssignmentTarget);
        }

        // Now undo the decimal transformation above.
        if (MungedDecimalConstant)
        {
            // Make it constant, non-shared
            Member->SetVarkind(VAR_Const);
            Member->SetIsShared(false);
        }

        if (IsBad(MemberReference))
        {
            continue;
        }


        BackupValue<BCSYM_NamedRoot*>BackupFieldInitializerContext(&m_FieldInitializerContext);
        if (MemberReference->bilop == SX_SYM)
        {
            ThrowIfFalse(m_FieldInitializerContext == NULL);
            m_FieldInitializerContext = MemberReference->AsSymbolReferenceExpression().Symbol;
        }

        // There are 3 different kinds of initialization:
        //
        //  -- Explicit array sizes in declarators imply creating an array.
        //  -- Declaration "As New" implies creating an instance.
        //     (initialization of init flags for static local backing fields
        //      is a degenerate case of this.)
        //  -- Explicit initialization.

        ILTree::Expression *Initialization = NULL;

        // interpret the initializers in the context of the class rather the ctor context.
        // if the initializers are interpreted in the context of the ctor then the argument
        // names can hide the names of other fields in the class. The initializer will bind to
        // argument names instead. bug(b156175)
        Scope *savedLookup = m_Lookup;
        m_Lookup = ViewAsScope(ContainingClass());
        if (Member->IsNew())
        {
            // This is an "As New" initialization.

            ParseTree::ParenthesizedArgumentList *ParenthesizedArguments = NULL;
            ParseTree::ObjectInitializerList *ObjectInitializer = NULL;
            ParseTree::BracedInitializerList *CollectionInitializer = NULL;
            ParseTree::PunctuatorLocation FromLocation = {0};
            const Location * Loc = NULL;

            if (Member->PVariableWithValue()->GetExpression())
            {
                SymbolicValue *InitializerExpression = Member->PVariableWithValue()->GetExpression();

                VSASSERT(!InitializerExpression->IsEvaluated(), "Premature evaluation of initializer for non-constant field.");

                WCHAR *InitializerText = InitializerExpression->GetExpressionText();
                Parser InitializerParser
                (
                    &m_TreeStorage,
                    m_Compiler,
                    m_CompilerHost,
                    false,
                    m_Project->GetCompilingLanguageVersion()
                );

                Scanner
                    Scanner
                    (
                        m_Compiler,
                        InitializerText,
                        wcslen(InitializerText),
                        0,
                        InitializerExpression->GetLocation()->m_lBegLine,
                        InitializerExpression->GetLocation()->m_lBegColumn
                    );

                InitializerParser.ParseParenthesizedArgumentsAndObjectInitializer
                (
                    &Scanner,
                    m_Errors,
                    ParenthesizedArguments,
                    ObjectInitializer,
                    CollectionInitializer,
                    FromLocation,
                    m_Project ? m_Project->GetProjectLevelCondCompScope() : NULL, // Dev10 #789970 Use proper conditional complation scope.
                    m_SourceFile->GetConditionalCompilationConstants() // Dev10 #789970 Use proper conditional complation scope.
                );

                Loc = InitializerExpression->GetLocation();
            }

            Initialization =
                InitializeConstructedVariable
                (
                    MemberReference,
                    ParenthesizedArguments ? ParenthesizedArguments->Values : NULL,
                    (Member->GetRawType() && Member->GetRawType()->IsNamedType()) ? Member->GetRawType()->GetLocation() : NULL,
                    ObjectInitializer,
                    CollectionInitializer,
                    CollectionInitializer ? &FromLocation : NULL,
                    Loc
                );
        }
        else if (Member->IsVariableWithValue())
        {
            // This is an "explicit initial value" initialization.

            SymbolicValue *InitializerSymbol = Member->PVariableWithValue()->GetExpression();

            if (InitializerSymbol)
            {
                VSASSERT(MungedDecimalConstant || !InitializerSymbol->IsEvaluated(), "Premature evaluation of initializer for non-constant field.");

                WCHAR *InitializerText = InitializerSymbol->GetExpressionText();
                Parser InitializerParser(
                    &m_TreeStorage,
                    m_Compiler,
                    m_CompilerHost,
                    false,
                    m_Project->GetCompilingLanguageVersion(),
                    false,
                    false,
                    true);

                Scanner
                    Scanner(
                        m_Compiler,
                        InitializerText,
                        wcslen(InitializerText),
                        0,
                        InitializerSymbol->GetLocation()->m_lBegLine,
                        InitializerSymbol->GetLocation()->m_lBegColumn);

                ParseTree::Initializer *Initializer =
                    InitializerParser.ParseOneInitializer(
                        &Scanner,
                        m_Errors,
                        m_Project ? m_Project->GetProjectLevelCondCompScope() : NULL,
                        m_SourceFile->GetConditionalCompilationConstants()
                        );

                
                Initialization =
                        InterpretInitializer(
                            Initializer,
                            MemberReference);
            }
        }
        else if (Member->IsVariableWithArraySizes())
        {
            // This is an "explicit array sizes" initialization.

            ExpressionList *SizesList = NULL;
            ExpressionList **SizesListTarget = &SizesList;
            const Location &Hidden = Location::GetHiddenLocation();
            Type *TypeOfSizeExpr = GetFXSymbolProvider()->GetIntegerType();

            for (unsigned Index = 0;
                 Index < MemberReference->ResultType->PArrayType()->GetRank();
                 Index++)
            {
                SymbolicValue *InitializerSymbol = Member->PVariableWithArraySizes()->GetArraySize(Index);
                ILTree::Expression *Size = NULL;

                if (InitializerSymbol)
                {
                    VSASSERT(MungedDecimalConstant || !InitializerSymbol->IsEvaluated(), "Premature evaluation of initializer for non-constant field.");

                    WCHAR *InitializerText = InitializerSymbol->GetExpressionText();
                    Parser InitializerParser(
                        &m_TreeStorage,
                        m_Compiler,
                        m_CompilerHost,
                        false,
                        m_Project->GetCompilingLanguageVersion());

                    Scanner
                        Scanner(
                            m_Compiler,
                            InitializerText,
                            wcslen(InitializerText),
                            0,
                            InitializerSymbol->GetLocation()->m_lBegLine,
                            InitializerSymbol->GetLocation()->m_lBegColumn);

                    ParseTree::Expression *SizeInitializer;
                    InitializerParser.ParseOneExpression(
                        &Scanner,
                        m_Errors,
                        &SizeInitializer,
                        NULL, // ErrorInConstructRet
                        m_Project ? m_Project->GetProjectLevelCondCompScope() : NULL, // Dev10 #789970 Use proper conditional complation scope.
                        m_SourceFile->GetConditionalCompilationConstants() // Dev10 #789970 Use proper conditional complation scope.
                        );

                    if (!SizeInitializer)
                    {
                        continue;
                    }

                    Size =
                        InterpretExpressionWithTargetType(
                            SizeInitializer,
                            ExprForceRValue,
                            TypeOfSizeExpr);
                }

                if (!Size || IsBad(Size))
                {
                    continue;
                }

                if (IsConstant(Size))
                {
                    Size->AsIntegralConstantExpression().Value++;

                    if (Size->AsIntegralConstantExpression().Value < 0)
                    {
                        ReportSemanticError(
                            ERRID_NegativeArraySize,
                            Size->Loc);
                    }
                }
                else
                {
                    Size =
                        AllocateExpression(
                            SX_ADD,
                            GetFXSymbolProvider()->GetIntegerType(),
                            Size,
                            ProduceConstantExpression(
                                1,
                                Hidden,
                                GetFXSymbolProvider()->GetIntegerType()
                                IDE_ARG(0)),
                            Hidden);
                }

                ExpressionList *ListElement =
                    AllocateExpression(
                        SX_LIST,
                        TypeHelpers::GetVoidType(),
                        Size,
                        NULL,
                        Size->Loc);

                *SizesListTarget = ListElement;
                SizesListTarget = &ListElement->AsExpressionWithChildren().Right;
            }

            Initialization =
                GenerateAssignment(
                    MemberReference->Loc,
                    MemberReference,
                    AllocateExpression(
                        SX_NEW_ARRAY,
                        MemberReference->ResultType,
                        SizesList,
                        MemberReference->Loc),
                        false);

        }
        // restore context to the ctor
        m_Lookup = savedLookup;

        if (Initialization)
        {
            if (Member->IsBadVariableType())
            {
                // There is an error with this variable but we didn't want to mark it bad so it is normally usable.
                // Unfortunately this case (dd:112520) will fail the bound tree iterator for Closures because it
                // won't understand the SX_PROPERTY_REFERENCE that remains in the tree when structs are newed.
                continue;
            }
            ILTree::Statement *AssignmentStatement;
            Location Loc = Location::GetHiddenLocation();

            if (Member->HasLocation())
            {
                Loc = Initialization->Loc;
            }

            if (Member->IsAutoPropertyBackingField())
            {
                // Create an SX_SEQ_OP2 that has the Initialization expression and the
                // temporary variable as its children.
                ILTree::Expression *pInit = AllocateExpression( 
                    SX_SEQ_OP2,
                    Member->GetType(),
                    Initialization,
                    MemberReference,
                    Initialization->Loc);

                // Create a PropertyReferenceExpression to be used to generate the call expression.
                ILTree::Expression *pProperty = AllocateExpression(
                    SX_PROPERTY_REFERENCE,
                    GetReturnType(Member->GetAutoPropertyThatCreatedSymbol()),
                    ReferToSymbol(
                        *Member->GetLocation(),
                        ViewAsProcedure(Member->GetAutoPropertyThatCreatedSymbol()),
                        chType_NONE,
                        NULL,
                        GenericBindingContext,
                        ExprIsAssignmentTarget | ExprIsExplicitCallTarget),
                    NULL,
                    Member->HasLocation() ? *Member->GetLocation() : Location::GetHiddenLocation());

                // Creates the call expression that represents the call to the auto property's setter, in order
                // to initialize it.
                Initialization = InterpretPropertyAssignment(Initialization->Loc, pProperty, pInit, false);            
            }
            
            AssignmentStatement = AllocateStatement(SL_STMT, Loc, NoResume);
            AssignmentStatement->AsStatementWithExpression().Operand1 = Initialization;
            AssignmentStatement->AsStatementWithExpression().SetContainingFile(Member->GetSourceFile());
        }
    }

    m_Errors = SavedErrors;
    m_UnnamedNamespaceForNameLookupInImports = SavedNamespaceForNameLookupInImports;
    m_SourceFile = SavedSourceFile;

    // Initialize the init flags for static local backing fields.

    VSASSERT(ContainingClass(), "");
    if (ContainingClass())
    {
        // Expect ContainingClass() to be the main type because the first BackingFieldsHash is always stored on
        // the main type for partial classes.
        //
        VSASSERT(!ContainingClass()->IsPartialType(),
                 "Containing class should always be the main type since the logical container is always the main type!!!");

        for(BCSYM_Hash *BackingFieldsHash = ContainingClass()->GetBackingFieldsForStatics();
            BackingFieldsHash;
            BackingFieldsHash = BackingFieldsHash->GetNextHash())
        {
            BCITER_HASH_CHILD BackingFields(BackingFieldsHash);

            for (Declaration *BackingField = BackingFields.GetNext();
                 BackingField;
                 BackingField = BackingFields.GetNext())
            {
                if (BackingField->IsStaticLocalBackingField() &&
                    BackingField->PStaticLocalBackingField()->IsInitFlag() &&
                    BackingField->PStaticLocalBackingField()->IsShared() == InitializeSharedFields)
                {
                    // This is a degenerate form of As New without arguments.

                    ILTree::Expression *VariableReference =
                        ReferToSymbol(
                            Location::GetHiddenLocation(),
                            BackingField,
                            chType_NONE,
                            NULL,
                            GenericBindingContext,
                            ExprIsAssignmentTarget);

                    if (IsBad(VariableReference))
                    {
                        continue;
                    }

                    ILTree::Expression *Initialization = InitializeConstructedVariable(VariableReference, NULL, NULL, NULL, NULL, NULL, NULL);

                    ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, Location::GetHiddenLocation(), NoResume);
                    AssignmentStatement->AsStatementWithExpression().Operand1 = Initialization;
                }
            }
        }
    }


    // If the sub new is synthesized in a designer control, then add a call to InitializeComponent
    if (m_Procedure != NULL &&
        m_Procedure->IsInstanceConstructor() &&
        m_Procedure->IsSyntheticMethod() &&
        IsDesignerControlClass(ContainingClass()))
    {
        Declaration *InitComponent =
            ImmediateLookup(
                ViewAsScope(ContainingClass()),
                STRING_CONST(m_Compiler, InitializeComponent),
                BINDSPACE_Normal);

        while (InitComponent != NULL)
        {
            Declaration *InitComponentMethod = ChaseThroughAlias(InitComponent);

            if (IsInitializeComponent(m_Compiler, InitComponentMethod))
            {
                ILTree::Expression *CallExpression =
                    InterpretCallExpressionWithNoCopyout(
                        Location::GetHiddenLocation(),
                        AllocateSymbolReference(
                            InitComponentMethod,
                            TypeHelpers::GetVoidType(),
                            SynthesizeMeReference(Location::GetHiddenLocation(), ContainingClass(), false),
                            Location::GetHiddenLocation(),
                            ContainingClass()->IsGeneric() ?
                                SynthesizeOpenGenericBinding(ContainingClass(), m_SymbolCreator) :
                                NULL),
                        chType_NONE,
                        NULL,
                        false,
                        ExprNoFlags,
                        NULL);

                ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, Location::GetHiddenLocation(), NoResume);
                CallStatement->AsStatementWithExpression().Operand1 = CallExpression;
                break;
            }

            InitComponent = InitComponent->GetNextBound();
        }
    }

    m_CreateImplicitDeclarations = CreateImplicitDeclarations;
}

void
Semantics::InterpretMyBaseHandler
(
    Procedure *Method,
    HandlesList *Handles
)
{
    // For a method declared with
    //
    //      Handles MyBase.EventName
    //
    // Synthesize code equivalent to:
    //
    //      AddHandler MyBase.EventName, AddressOf MethodName
    //
    // The most straightforward and robust way to accomplish this is
    // actually to generate the equivalent parse tree and interpret it.

    // "AddHandler <MyBase/Me/MyClass/FullyQualifedTypeName>.<EventName>, AddressOf <MethodName>"
    //
    
    NorlsAllocator Allocator(NORLSLOC);
    ParserHelper PH(&Allocator, Location::GetHiddenLocation());

    Procedure *Event = Handles->GetEvent();

    ParseTree::Expression * EventQualifier = NULL;

    // Microsoft:
    // As part of fixing dev10 #426874, we should not generate instance.SharedEvent.
    // Rather, if the event is shared, always generate a type.

    //if (Method->IsShared() && Handles->GetEvent()->IsShared())
    if (Handles->GetEvent()->IsShared())
    {
        // Since we cannot use MyBase Or MyClass in the shared constructor,
        // generate parseTree for
        //      Global.FullyQualifiedMyBaseClassName.EventName
        //
        // Microsoft: In addition, if just the event is shared, we want to use a type to reference
        // it, rather then an instance.
        ClassOrRecordType *CurrentClassContext = ContainingClass();

        Type *SharedEventQualifier =
            Event->HasGenericParent() ?
                (Type *)DeriveGenericBindingForMemberReference(
                    IsGenericOrHasGenericParent(CurrentClassContext) ?
                        (BCSYM *)SynthesizeOpenGenericBinding(
                            CurrentClassContext,
                            m_SymbolCreator) :
                        CurrentClassContext,
                    Event,
                    m_SymbolCreator,
                    m_CompilerHost) :
                Event->GetContainer();

        EventQualifier = PH.CreateBoundSymbol(SharedEventQualifier);
    }
    else
    {
        VSASSERT(Handles->IsMyBase() || Handles->IsEventFromMeOrMyClass(),
                    "Unexpected Handles Context!!!");

        EventQualifier =
            Handles->IsMyBase() ?
                PH.CreateMyBaseReference() :
                PH.CreateMyClassReference();
    }

    // EventReference = <EventQualifier>.<EventName>
    ParseTree::Expression * EventReference = PH.CreateQualifiedExpression(
        EventQualifier,
        PH.CreateNameExpression(1, Event->GetName()));

    // AddressOf <Method>
    // Note that the symbol was already referenced in this file, so we do not need to add extra symbol <-> file dependencies
    ParseTree::Expression * AddressOfHandler = PH.CreateAddressOf(PH.CreateBoundSymbol(Method), true);
    
    ParseTree::HandlerStatement *ParsedAddHandlerStatement = NULL;
    // AddHandler <EventQualifier>.<EventName>, AddressOf<Method>
    ParsedAddHandlerStatement = PH.CreateAddHandler(EventReference, AddressOfHandler);

    // Set this statement to have hidden location so that users can not step to it
    //
    ParsedAddHandlerStatement->TextSpan = Location::GetHiddenLocation();

    // VSWhidbey[517456] Set resume index of this statement to NoResume.
    ParsedAddHandlerStatement->ResumeIndex = NoResume;

    // No obsolete checking required for the synthetic handler code
    // The errors/warnings would have already been generated on the
    // handles in the declarations itself.
    //
    bool OriginalStateValue = m_PerformObsoleteChecks;
    m_PerformObsoleteChecks = false;


    InterpretHandlerStatement(ParsedAddHandlerStatement);

    m_PerformObsoleteChecks = OriginalStateValue;
}

#if IDE

void
Semantics::RefreshNewENCMembers
(
    DynamicArray<BCSYM_Variable *> *ENCMembersToRefresh
)
{
    BCSYM_Variable *ObjectTemporary = NULL;

    for(unsigned i = 0; i < ENCMembersToRefresh->Count(); i++)
    {
        BCSYM_Variable *MemberVar = ENCMembersToRefresh->Element(i);

        // ENC sends in all the new members irrespective of the types in which
        // they are added. But in a ENC refresh function, we will only refresh the
        // new members of its containing type.
        //
        if (MemberVar->GetContainer() != ContainingClass())
        {
            continue;
        }

        if (MemberVar->IsConstant())
        {
            VSFAIL("New constants unexpected during ENC!!!");
            continue;
        }

        if (m_Procedure != NULL && (MemberVar->IsShared() != m_Procedure->IsShared()))
        {
            continue;
        }

        RefreshNewENCMember(MemberVar, ObjectTemporary);
    }
}

void
Semantics::RefreshNewENCMember
(
    BCSYM_Variable *MemberVar,
    BCSYM_Variable *&ObjectTemporary
)
{
    const Location &Hidden = Location::GetHiddenLocation();

    // The refresh is achieved by assigning a member to itself.
    //
    // The first reference to a new member triggers the clr to create a real
    // entity for the new member.

    // Me.x
    //
    ILTree::Expression *Source =
        AllocateSymbolReference(
            MemberVar,
            MemberVar->GetType(),
            MemberVar->IsShared() ?
                NULL :
                SynthesizeMeReference(Hidden, ContainingClass(), false),
            Hidden,
            IsGenericOrHasGenericParent(ContainingClass()) ?
                SynthesizeOpenGenericBinding(ContainingClass(), m_SymbolCreator) :
                NULL);

    BCSYM_Variable *TargetVar = MemberVar;

    if (MemberVar->IsReadOnly())
    {
        // Dim Temporary As Object
        // Temporary = CObj(Me.x)

        // Convert to object so that only one temporary of type System.Object is created
        // and reused across the various readonly member access statements.
        //
        Source = Convert(Source, GetFXSymbolProvider()->GetObjectType(), ExprHasDirectCastSemantics, ConversionWidening);

        if (ObjectTemporary == NULL)
        {
            ObjectTemporary = AllocateShortLivedTemporary(GetFXSymbolProvider()->GetObjectType());
        }

        TargetVar = ObjectTemporary;
    }
    // else
    //{
        // Generate "Me.x = Me.x"
    //}

    ILTree::Expression *Target =
        AllocateSymbolReference(
            TargetVar,
            TargetVar->GetType(),
            (TargetVar == MemberVar && !MemberVar->IsShared()) ?
                SynthesizeMeReference(Hidden, ContainingClass(), false) :
                NULL,
            Hidden,
            IsGenericOrHasGenericParent(ContainingClass()) ?
                SynthesizeOpenGenericBinding(ContainingClass(), m_SymbolCreator) :
                NULL);

    ILTree::ExpressionWithChildren *MemberVarReadAndAssign =
        (ILTree::ExpressionWithChildren *)AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            Target,
            Source,
            Hidden);

    // Suppress any boxed value type cloning etc. since this is an assignment
    // of a variable to itself.
    //
    SetFlag32(MemberVarReadAndAssign, SXF_ASG_SUPPRESS_CLONE);

    ILTree::Statement *AssignmentStatement = AllocateStatement(SL_STMT, Hidden, NoResume);
    AssignmentStatement->AsStatementWithExpression().Operand1 = MemberVarReadAndAssign;
}

void
Semantics::GenerateENCTrackingList(ClassOrRecordType *Class)
{
    VSASSERT(wcscmp(STRING_CONST(m_Compiler, ENCTrackingList), L"__ENCList") == 0,
                "ENCTrackingList lost!!!");

    Declaration *PossibleENCTrackingList =
        ImmediateLookup(ViewAsScope(Class), STRING_CONST(m_Compiler, ENCTrackingList), BINDSPACE_Normal);

    if (PossibleENCTrackingList == NULL ||
        !PossibleENCTrackingList->IsVariable() ||
        !PossibleENCTrackingList->PVariable()->IsENCTrackingList())
    {
        return;
    }

    const WCHAR *AddElementCode = L"__ENCAddToList(Me)";

    Scanner
        Scanner(
            m_Compiler,
            AddElementCode,
            (size_t)wcslen(AddElementCode),
            0,
            0,
            0);

    NorlsAllocator Allocator(NORLSLOC);
    Parser Parser(&Allocator, m_Compiler, m_CompilerHost, false, LANGUAGE_CURRENT);

    ParseTree::Expression *AddElementExpression;
    Parser.ParseOneExpression(
        &Scanner,
        m_Errors,
        &AddElementExpression);

    if (AddElementExpression == NULL)
    {
        return;
    }

    ILTree::Expression *AddElementBoundExpression =
        InterpretExpression(
            AddElementExpression,
            ExprNoFlags);

    if (!AddElementBoundExpression || IsBad(AddElementBoundExpression))
    {
        return;
    }

    const Location &Hidden = Location::GetHiddenLocation();

    AddElementBoundExpression->ResultType = TypeHelpers::GetVoidType();
    AddElementBoundExpression->vtype = t_void;
    AddElementBoundExpression->Loc = Hidden;

    ILTree::Statement *CallStatement = AllocateStatement(SL_STMT, *Class->GetLocation(), NoResume);
    CallStatement->AsStatementWithExpression().Operand1 = AddElementBoundExpression;
    CallStatement->Loc = Hidden;
}

#endif IDE

int
Semantics::BaseClassConstructorAcceptingNoArgumentsCount
(
    ClassOrRecordType *DerivedClass,
    _Out_ Procedure *&MatchingConstructor,
    _Out_ bool &HasAccessibleConstructor
)
{
    ClassOrRecordType *BaseClass = DerivedClass->GetCompilerBaseClass();

    MatchingConstructor = NULL;

    if (BaseClass == NULL || TypeHelpers::IsBadType(BaseClass))
    {
        return -1;
    }


    unsigned NoArgumentsConstructorCount = 0;

    for (Procedure *Constructor = BaseClass->GetFirstInstanceConstructor(m_Compiler);
         Constructor;
         Constructor = Constructor->GetNextInstanceConstructor())
    {
        // - Generic constructors are disallowed, but in case they show up because of bad metadata, ignore them.
        // - Ignore Shared constructors

        if (Constructor->IsGeneric() ||
            !Constructor->IsInstanceConstructor())
        {
            continue;
        }

        if (Semantics::IsAccessible(
                Constructor,
                NULL,   // Accessing through base, don't need a binding context
                DerivedClass,
                DerivedClass,
                m_SymbolCreator,
                m_CompilerHost))
        {
            HasAccessibleConstructor = true;

            if (Constructor->GetRequiredParameterCount() == 0)
            {
                NoArgumentsConstructorCount++;

                if (NoArgumentsConstructorCount == 1)
                {
                    MatchingConstructor = Constructor;
                }
            }
        }
    }

    return NoArgumentsConstructorCount;
}

unsigned ErrorCountInAssociatedErrorTables
(
    BCSYM_Container *Container,
    ErrorTable *Errors,
    AlternateErrorTableGetter AltErrTables
)
{
    AssertIfNull(Container);
    AssertIfNull(Errors);
    AssertIfNull(AltErrTables);

    unsigned ErrorCount = Errors->GetErrorCount();

    if (Container->GetMainType())
    {
        Container = Container->GetMainType();
    }

    for (BCSYM_Container *Current = Container;
         Current;
         Current = Current->GetNextPartialType())
    {
        SourceFile *CurrentFile = Current->GetSourceFile();
        VSASSERT(CurrentFile != NULL, "NULL sourcefile unexpected!!!");

        ErrorTable *CurrentErrors = AltErrTables(CurrentFile);
        VSASSERT(CurrentErrors != NULL, "NULL error table unexpected!!!");

        if (CurrentErrors != Errors)
        {
            ErrorCount += CurrentErrors->GetErrorCount();
        }
    }

    return ErrorCount;
}

ILTree::ProcedureBlock *
Semantics::InterpretMethodBody
(
    Procedure *Method,
    _In_ ParseTree::MethodBodyStatement *MethodBody,
    Cycles *CycleDetection,
    _Inout_opt_ CallGraph *CallTracking,
    Scope *LocalsAndParameters,
    AlternateErrorTableGetter AltErrTablesForConstructor,  // should only be passed in by the background compiler during the main compilation
    DynamicArray<BCSYM_Variable *> *ENCMembersToRefresh,
    bool MergeAnonymousTypes
)
{
    // Interpret method body is not recursive - hence the assert.
    ThrowIfTrue(m_InterpretingMethodBody);

    // Use the back up mechanism to ensure that the value is restored
    // irrespective of the return path used.
    BackupValue<bool> BackupInterpretMethodBody(&m_InterpretingMethodBody);

    // Indicate that a method body is being analyzed.
    m_InterpretingMethodBody = true;

    // Ensure XmlNameVar is empty.  XmlNameVar caches temporary variables for the duration of a method so
    // that XmlName and XmlNamespace lookup at runtime is fast.
    m_XmlNameVars = NULL;

    m_methodDeferredTempCount = 0;

    Variable *ReturnVariable = NULL;
    unsigned OriginalErrorCount = 0;

    VSASSERT(AltErrTablesForConstructor == NULL || Method->IsAnyConstructor(),
                "Why are alternate error tables passed in for non-constructor methods ?");

#if IDE 
    // should only be passed in by the background compiler during the main compilation
    //
    VSASSERT(AltErrTablesForConstructor == NULL || GetCompilerSharedState()->IsInBackgroundThread(),
                "Alternate error table expected only during the main compilation task!!!");
#endif IDE

    if (Method->IsAnyConstructor() && AltErrTablesForConstructor && Method->GetContainer())
    {
        OriginalErrorCount =
            ErrorCountInAssociatedErrorTables(
                Method->GetContainer(),
                m_Errors,
                AltErrTablesForConstructor);
    }
    else
    {
        AltErrTablesForConstructor = NULL;

        if (m_Errors==NULL)
        {
            VSFAIL("oops, see bug 729104");
        }
        else
        {
            OriginalErrorCount = m_Errors->GetErrorCount();
        }
    }

    m_AltErrTablesForConstructor = AltErrTablesForConstructor;

    // Create the SB_PROC node.
    ILTree::ProcedureBlock *BoundBody = &AllocateStatement(SB_PROC, MethodBody->TextSpan, 0, false)->AsProcedureBlock();
    BoundBody->fEmptyBody = MethodBody->IsEmpty;
    BoundBody->pproc = Method;

    // Update m_statementGroupId here.  m_statementGroupId is meant to reflect the mapping of
    // parse tree statements to BILTREE statements by putting BILTREE statements into groups.
    // The ProcedureBody is not in the same group as the first Statement in the body so
    // update the group after allocating the statement
    ++m_statementGroupId;

    // If the client of the interpretation is the debugger evaluation
    // engine, the locals scope is supplied. For normal interpretation,
    // the locals scope is synthesized within the interpreter and LocalsAndParameters
    // should be NULL.

    if (LocalsAndParameters == NULL)
    {
        Declared::MakeLocalsFromParams(
            m_Compiler,
            &m_TreeStorage,
            Method,
            m_Errors,
            true,
            &ReturnVariable,
            &LocalsAndParameters);
    }

    BoundBody->Locals = LocalsAndParameters;

    if (Method->IsSyntheticMethod() && Method->IsBad())
    {
        return BoundBody;
    }

    // If we have a partial method, we must have an empty body. Validate
    // this in semantics, because if the user makes a method body edit, we
    // will decompile only to bound. Doing this check in parser, declared,
    // or bindable will not suffice, unless we decompile to declared the entire
    // file, which is a performance penalty since designer-generated files
    // tend to be large.

    if (Method->IsPartialMethodDeclaration())
    {
        if (MethodBody->Children != NULL)
        {
            ReportSemanticError(
                ERRID_PartialMethodMustBeEmpty,
                *(Method->GetLocation())
                );
            MarkContainingLambdaOrMethodBodyBad();
            return BoundBody;
        }
    }

    if (CallTracking)
    {
        CallTracking->SetSource(Method);
    }

    InitializeInterpretationState(
        BoundBody,
        BoundBody->Locals,
        CycleDetection,
        CallTracking,
        m_SymbolsCreatedDuringInterpretation,
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        true,       // can interpret statements
        true,       // can interpret expressions
        MergeAnonymousTypes); // merge anonymous type templates


    BoundBody->ptempmgr = m_TemporaryManager;

    // If the method returns a value, connect the symbol representing
    // the return value with the method body. Code generation needs this so it
    // knows which variable should be returned.

    if (Method->GetType())
    {
        BoundBody->ReturnVariable = ReturnVariable;
    }

    BoundBody->fSeenOnErr = MethodBody->ProcedureContainsOnError || MethodBody->ProcedureContainsResume;
    BoundBody->fSeenResume = MethodBody->ProcedureContainsResume;

    BoundBody->OnErrorHandlerCount = MethodBody->OnErrorHandlerCount;
    BoundBody->OnErrorResumeCount = MethodBody->OnErrorResumeCount;

    m_ProcedureContainsOnError = MethodBody->ProcedureContainsOnError;  // A state variable is useful for brevity.
    m_ProcedureContainsTry = MethodBody->ProcedureContainsTry;  // A state variable is useful for brevity.

    EstablishBlockContext(BoundBody);

    IndicateLocalResumable();
    // Procedures [here] indicate their resumable stuff in Semantics::InterpretMethodBody, before
    // evaluating fields in synthetic constructors, and before doing TryInterpretBlock
    // Lambdas indicate their resumable stuff in LambdaBodyInterpretStatement::DoInterpretBody, immediately before doing TryInterpretBlock

    if (Method->IsAsyncKeywordUsed() || Method->IsIteratorKeywordUsed())
    {
        CheckSecurityCriticalAttributeOnParentOfResumable(Method);
    }

    if ( !BypassConstructorLogic() )
    {
        // Check for valid constructor logic, except if this method is runtime managed like the delegate classes.
        if (Method->IsInstanceConstructor() && !Method->IsMethodCodeTypeRuntime())
        {
            m_CompilingConstructorDefinition = true;

            // Find the first statement with an effect.

            ParseTree::StatementList *First;
            for (First = MethodBody->Children;
                 First && First->Element->IsPlacebo();
                 First = First->NextInBlock)
            {
            }

            ParseTree::Statement *FirstExecutableStatement = First ? First->Element : NULL;

            // Make sure the first statement is a constructor call.

            bool FirstStatementIsValidConstructorCall = false;

            if (FirstExecutableStatement &&
                FirstExecutableStatement->Opcode == ParseTree::Statement::Call)
            {
                ParseTree::Expression *CallTarget =
                    FirstExecutableStatement->AsCall()->Target;

                if (CallTarget->Opcode == ParseTree::Expression::DotQualified)
                {
                    VSASSERT(CallTarget->AsQualified()->Name != NULL, "QualifiedExpression::Name must be set!");

                    if(CallTarget->AsQualified()->Name &&
                       CallTarget->AsQualified()->Name->Opcode == ParseTree::Expression::Name)
                    {
                        ParseTree::IdentifierDescriptor * Name = &CallTarget->AsQualified()->Name->AsName()->Name;

                        if (StringPool::IsEqual(Name->Name, m_Compiler->TokenToString(tkNEW)) &&
                            !Name->IsBracketed &&
                            CallTarget->AsQualified()->Base &&
                            (CallTarget->AsQualified()->Base->Opcode == ParseTree::Expression::MyBase ||
                             CallTarget->AsQualified()->Base->Opcode == ParseTree::Expression::MyClass ||
                             CallTarget->AsQualified()->Base->Opcode == ParseTree::Expression::Me ||
                             CallTarget->AsQualified()->Base->Opcode == ParseTree::Expression::SyntaxError))
                        {
                            FirstStatementIsValidConstructorCall = true;
                        }
                    }
                }
            }

            if (FirstStatementIsValidConstructorCall)
            {
                m_ExpectingConstructorCall = true;

                // The non-shared fields will be initialized after processing
                // the first statement, which is a call to another constuctor.
            }
            else if (TypeHelpers::IsRecordType(Method->GetParent()))
            {
                // The non-shared fields of a structure must be initialized here
                // if there is no later point to initialize them.

                InitializeFields();
            }
            else
            {
                Procedure *BaseConstructorWithNoRquiredArguments = NULL;
                bool HasAccessibleConstructor = false;
                unsigned NumberOfConstructorsAcceptingNoArguments =
                                BaseClassConstructorAcceptingNoArgumentsCount(
                                    Method->GetParent()->PClass(),
                                    BaseConstructorWithNoRquiredArguments,
                                    HasAccessibleConstructor);

                if (NumberOfConstructorsAcceptingNoArguments == 1 ||
                    NumberOfConstructorsAcceptingNoArguments == -1)     // -1 is returned when the base class is bad
                {
                    // Synthesize "MyBase.New()".

                    m_ExpectingConstructorCall = true;

                    BackupValue<bool>SynthesizingBaseCtorCall(&m_SynthesizingBaseCtorCall);
                    m_SynthesizingBaseCtorCall = true;

                    ParseTree::Expression MyBase;
                    MyBase.Opcode = ParseTree::Expression::MyBase;
                    MyBase.TextSpan = Location::GetHiddenLocation();

                    ParseTree::NameExpression NewName;
                    NewName.Opcode = ParseTree::Expression::Name;
                    NewName.Name.Name = STRING_CONST(m_Compiler, New);
                    NewName.Name.TypeCharacter = chType_NONE;
                    NewName.Name.IsBracketed = false;
                    NewName.Name.IsNullable = false;
                    NewName.Name.IsBad = false;

                    ParseTree::QualifiedExpression MyBaseNew;
                    MyBaseNew.Opcode = ParseTree::Expression::DotQualified;
                    MyBaseNew.TextSpan = MyBase.TextSpan;
                    MyBaseNew.Base = &MyBase;
                    MyBaseNew.Name = &NewName;

                    ParseTree::CallStatement Call;
                    memset(&Call,0, sizeof(Call));
                    Call.Opcode = ParseTree::Statement::Call;
                    Call.TextSpan = MyBaseNew.TextSpan;
                    Call.ResumeIndex = NoResume;
                    Call.SetParent(NULL);
                    Call.Comments = NULL;
                    Call.Target = &MyBaseNew;
                    Call.Arguments.Values = NULL;

                    // No normal obsolete checking done for the synthetic Mybase.New
                    // call because specialized error reporting is required. This
                    // specialized obsolete checking and error reporting is done
                    // later in this procedure.
                    //
                    bool OriginalStateValue = m_PerformObsoleteChecks;
                    m_PerformObsoleteChecks = false;

                    InterpretCall(&Call);

                    m_PerformObsoleteChecks = OriginalStateValue;

                    // Specialized obsolete checking and reporting to report the
                    // appropriate warnings/errors at the appropriate locations.

                    bool IsUseOfObsoleteSymbolError = false;
                    WCHAR *ObsoleteMessage = NULL;

                    if (BaseConstructorWithNoRquiredArguments &&
                        PerformObsoleteChecks() &&
                        ObsoleteChecker::IsSymbolObsolete(
                            BaseConstructorWithNoRquiredArguments,
                            IsUseOfObsoleteSymbolError,
                            &ObsoleteMessage))
                    {
                        // If Synthetic constructor, then generate a slightly different
                        // error on the class itself
                        //
                        if (Method->IsSyntheticMethod())
                        {
                            ReportSemanticError(
                                IsUseOfObsoleteSymbolError ?
                                    (ObsoleteMessage ?
                                        ERRID_NoNonObsoleteConstructorOnBase4 :
                                        ERRID_NoNonObsoleteConstructorOnBase3) :
                                    (ObsoleteMessage ?
                                        WRNID_NoNonObsoleteConstructorOnBase4 :
                                        WRNID_NoNonObsoleteConstructorOnBase3),
                                *Method->GetParent()->GetLocation(),
                                Method->GetParent(),
                                BaseConstructorWithNoRquiredArguments,
                                Method->GetParent()->PClass()->GetCompilerBaseClass(),
                                ObsoleteMessage);
                        }
                        else
                        {
                            ReportSemanticError(
                                IsUseOfObsoleteSymbolError ?
                                    (ObsoleteMessage ?
                                        ERRID_RequiredNonObsoleteNewCall4 :
                                        ERRID_RequiredNonObsoleteNewCall3) :
                                    (ObsoleteMessage ?
                                        WRNID_RequiredNonObsoleteNewCall4 :
                                        WRNID_RequiredNonObsoleteNewCall3),
                                FirstExecutableStatement ?
                                    FirstExecutableStatement->TextSpan :
                                    (Method->HasLocation() ?
                                        *Method->GetLocation() :
                                        MethodBody->TextSpan),
                                BaseConstructorWithNoRquiredArguments,
                                Method->GetParent()->PClass()->GetCompilerBaseClass(),
                                Method->GetParent(),
                                ObsoleteMessage);
                        }
                    }
                }
                else
                {
                    // If Synthetic constructor, then generate a slightly different
                    // error on the class itself
                    //
                    if (HasAccessibleConstructor)
                    {
                        if (Method->IsSyntheticMethod())
                        {
                            ReportSemanticError(
                                NumberOfConstructorsAcceptingNoArguments == 0 ?
                                    ERRID_NoConstructorOnBase2 :
                                    ERRID_NoUniqueConstructorOnBase2,
                                *Method->GetParent()->GetLocation(),
                                Method->GetParent(),
                                Method->GetParent()->PClass()->GetCompilerBaseClass());
                        }
                        else
                        {
                            ReportSemanticError(
                                NumberOfConstructorsAcceptingNoArguments == 0 ?
                                    ERRID_RequiredNewCall2 :
                                    ERRID_RequiredNewCallTooMany2,
                                FirstExecutableStatement ?
                                    FirstExecutableStatement->TextSpan :
                                    (Method->HasLocation() ?
                                        *Method->GetLocation() :
                                        MethodBody->TextSpan),
                                Method->GetParent()->PClass()->GetCompilerBaseClass(),
                                Method->GetParent());
                        }
                    }
                    else
                    {
                        ReportSemanticError(
                            ERRID_NoAccessibleConstructorOnBase,
                            *Method->GetParent()->PClass()->GetInheritsLocation(),
                            Method->GetParent()->PClass()->GetCompilerBaseClass());
                    }


                    // Constructor call cycles detection apparently needs to know
                    // that this constructor doesn't call another constructor
                    // of the same class.

                    m_ConstructorCycles->AddSymbolToVerify(Method);
                }
            }
        }
        else if (Method->IsSharedConstructor())
        {
            InitializeFields();
        }
    }

    // Create symbols for all defined labels. (Do this here, rather than when a label
    // statement is encountered, to avoid issues with gotos that preceded their targets.
    CreateLabelDefinitions(MethodBody);

#if IDE 
    if (ENCMembersToRefresh &&
        m_Procedure != NULL &&
        m_Procedure->IsSyntheticMethod() &&
        (m_Procedure->PSyntheticMethod()->GetSyntheticKind() == SYNTH_ENCHiddenRefresh ||
         m_Procedure->PSyntheticMethod()->GetSyntheticKind() == SYNTH_ENCSharedHiddenRefresh))
    {
        RefreshNewENCMembers(ENCMembersToRefresh);
    }
#else
    VSASSERT(ENCMembersToRefresh == NULL, "ENC members unexpected during command line compilation!!!");
#endif


    if (!TryInterpretBlock(MethodBody))
    {
        MakeBad(BoundBody);
        ReportSemanticError(ERRID_EvaluationAborted, MethodBody->TextSpan);
        return BoundBody;  // if the foreground thread wants to stop the bkd, then let's not recur so IDE is more responsive
    }


    VSASSERT(m_BlockContext == BoundBody, "Generated block nesting confused.");

    // check for async methods without awaits
    if (Method->IsAsyncKeywordUsed() && !BoundBody->fSeenAwait)
    {
        // "Warning: this method lacks awaits and so will run synchronously. Consider awaiting an async method in it."
        //
        // (actually, in the case of "<DllImport()> Async Sub f()", let's suppress the warning...)
        if (Method->GetPWellKnownAttrVals()->GetDllImportData() == NULL)
        {
            ReportSemanticError(WRNID_AsyncLacksAwaits, *Method->GetLocation());
        }
    }

    // check for restricted types the locals defined at the method body level(skip arguments)
    // for locals defined in the nested blocks the following block level shadowing check would do the work
    CheckRestrictedArraysInLocals(BoundBody->Locals);

    // Check for block-level declarations that shadow locals in enclosing blocks.
    CheckChildrenForBlockLevelShadowing(BoundBody);
    
    unsigned CurrentErrorCount = 0;

    if (m_Errors==NULL)
    {
        VSFAIL("oops, see bug 729104");
    }
    else
    {
        CurrentErrorCount =
            AltErrTablesForConstructor ?
                ErrorCountInAssociatedErrorTables(
                    Method->GetContainer(),
                    m_Errors,
                    AltErrTablesForConstructor) :
                m_Errors->GetErrorCount();
    }

    if (CurrentErrorCount > OriginalErrorCount)
    {
        // It is a requirement that all method bodies containing any
        // syntactic or semantic invalidity be marked bad. This is accomplished
        // by checks at these points:
        //
        //  InterpretStatementSequence catches all statements with syntax errors.
        //  InterpretStatementOperand catches all semantically invalid
        //    expressions that occur as operands of statements.
        //  InterpretCall catches semantically invalid statement-level calls.
        //  InterpretMethodBody (here) catches the generation of errors
        //    during the interpretation of a method body.
        //  A few places catch bad name interpretations in an ad-hoc fashion.

        MarkContainingLambdaOrMethodBodyBad();
    }

    if ((m_Procedure != NULL && !m_Procedure->IsSyntheticMethod()) ||
        (m_Procedure->IsAnyConstructor() && !m_Procedure->GetContainer()->IsAnonymousDelegate()))
    {
        // Flow analysis is normally only done on user-generated code, not synthetic code.
        // But if there were field initializers, then they'll have been injected into all class constructors,
        // and we need to do flow-analysis on them. (although not on constructors of anonymous delegates)
        // Note: if there were multiple constructors then each field initializer will end up
        // being analyzed multiple times; see bug Dev10#498116
        CheckFlow(BoundBody,false);
    }

    // place a marker for the transient sybols to be created in this BoundBody.
    // Assumes, only TransformMethod can create transient procedures, and no other transient
    // proc is created up to this point. Otherwise, move StartMethod() up.
    // 
    if(!m_IsGeneratingXML && m_SymbolsCreatedDuringInterpretation)
    {
        m_SymbolsCreatedDuringInterpretation->StartMethod();
    }

    // Dev10 #735384 Check if we are improperly using embedded types
    if (m_Project != NULL && m_Errors != NULL && m_ReportErrors &&
        m_Project->NeedToCheckUseOfEmbeddedTypes())
    {
        CheckUsageOfEmbeddedTypesVisitor checker(m_Project, m_Errors);

        checker.Visit(BoundBody);
    }

    BoundBody = TransformMethod(BoundBody);

    if(!m_IsGeneratingXML && m_SymbolsCreatedDuringInterpretation)
    {
        LowerBoundTreesForMethod(BoundBody, m_SymbolsCreatedDuringInterpretation);

        // This is where we rewrite resumable methods. As for resumable lambdas, they're
        // rewritten in LowerBoundTreesForMethod.
        if (!BoundBody->pproc->IsSyntheticMethod()) BoundBody = LowerResumableMethod(BoundBody);

        m_SymbolsCreatedDuringInterpretation->EndMethod();
    }

    m_AltErrTablesForConstructor = NULL;
    return BoundBody;
}

void
Semantics::CreateLabelDefinitions(_In_ ParseTree::ExecutableBlockStatement *pBlock)
{
    VSASSERT( pBlock->Opcode == ParseTree::Statement::ProcedureBody ||
              pBlock->Opcode == ParseTree::Statement::FunctionBody ||
              pBlock->Opcode == ParseTree::Statement::OperatorBody ||
              pBlock->Opcode == ParseTree::Statement::PropertyGetBody ||
              pBlock->Opcode == ParseTree::Statement::PropertySetBody ||
              pBlock->Opcode == ParseTree::Statement::AddHandlerBody ||
              pBlock->Opcode == ParseTree::Statement::RemoveHandlerBody ||
              pBlock->Opcode == ParseTree::Statement::RaiseEventBody ||
              pBlock->Opcode == ParseTree::Statement::LambdaBody ||
              pBlock->Opcode == ParseTree::Statement::SyntaxError,
              "Expected either a MethodBodyStatement or a LambdaBodyStatement or a Syntax Error");

    ParseTree::StatementList *pDefinedLabels = (pBlock->Opcode == ParseTree::Statement::LambdaBody) ?
                                                    pBlock->AsLambdaBody()->pDefinedLabels :
                                                    pBlock->AsMethodBody()->DefinedLabels;
    
    for (ParseTree::StatementList *Labels = pDefinedLabels;
         Labels;
         Labels = Labels->NextInBlock)
    {
        ParseTree::LabelReferenceStatement *Label = Labels->Element->AsLabelReference();

        if (!Label->Label.IsBad)
        {
            GetLabelForDefinition(Label)->DefiningStatement = Label;
        }
    }
}


ILTree::Expression *
Semantics::InterpretExpression
(
    ParseTree::Expression *Input,
    Scope *Lookup,
    ILTree::Expression *WithStatementContext,
    ExpressionFlags flags,
    Type *TargetType
)
{
    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,
        NULL,      // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        true,      // preserve extra semantic information
        true,      // perform obsolete checks
        false,     // cannot interpret statements
        true,
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        NULL,
        false       // reset closures information
    );

    m_CreateImplicitDeclarations = flags & ExprCreateImplicitDeclarations;

    if (WithStatementContext)
    {
        m_EnclosingWith = &AllocateStatement(SB_WITH, WithStatementContext->Loc, 0, false)->AsWithBlock();
        m_EnclosingWith->Loc = WithStatementContext->Loc;

        InterpretWithStatement(WithStatementContext, m_EnclosingWith);
    }

    if (TargetType)
    {
        return InterpretExpressionWithTargetType(Input, flags | ExprForceRValue, TargetType);
    }
    else
    {
        return InterpretExpression(Input, flags);
    }
}

ILTree::Expression *
Semantics::InterpretExpressionAndLower
(
    ParseTree::Expression *Input,
    Scope *Lookup,
    ILTree::Expression *WithStatementContext,
    ExpressionFlags flags,
    Type *TargetType
)
{
    // Note: it's quite possible that we get called here with a bad expression.
    // That happens e.g. in the debugger where the "autos" window just throws at
    // us random snippets of expressions. e.g. given y.Save("fred.txt"), the debugger
    // will ask us to interpret the expression y.Save, even though VB language
    // rules say that y.Save is equivalent to y.Save() which likely has no valid overloads
    // and so is a bad expression.

    ILTree::Expression* Result = InterpretExpression(
        Input,
        Lookup,
        WithStatementContext,
        flags,
        TargetType
        );

    // Lower any created deferred temporaries
    if (m_methodDeferredTempCount > 0)
    {
        m_methodDeferredTempCount = 0;
        DeferredTempIterator deferred(this);
        deferred.Iterate(&Result);

        // During processing in the iterator, all SX_DEFERRED_TEMPs should be removed.
        // If the method still has deferred temps then they were created during iterate.
        // This is a bug in the iterate code. 
        ThrowIfTrue(m_methodDeferredTempCount != 0);
    }

    LowerBoundTreeVisitor lit(this);
    lit.Visit(&Result);

    return( Result );
}

#if HOSTED
ILTree::ExecutableBlock *
Semantics::InterpretStatementsForHostedCompiler
(
    ParseTree::ExecutableBlockStatement *Input,
    Scope *Lookup
)
{
    InitializeInterpretationState(
        NULL,       // No method body
        Lookup,     // Scope
        NULL,       // No cycle detection
        NULL,       // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        true,       // can interpret statements
        true,       // can interpret expressions
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false);     // reset closures information

    ILTree::ExecutableBlock * pBlockContext = AllocateBlock(SB_PROC, Input, false);
    EstablishBlockContext(pBlockContext);
    InterpretAndPopBlock(Input);
    return pBlockContext;
}

ILTree::Expression *
Semantics::InterpretExpressionForHostedCompiler
(
    ParseTree::Expression *Input,
    Scope *Lookup,
    Type *TargetType
)
{
    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,
        NULL,      // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        false,      // don't preserve extra semantic information
        true,       // perform obsolete checks
        false,      // cannot interpret statements
        true,       // can interpret expressions
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false);     // don't reset closures information

    // This option (option explicit) is not supported in the hosted compiler
    // and is in fact not exposed to consumers of the hosted compiler.
    // 
    // Note this this has never been set to true for hosted compiler.
    // If ever this needs to be true for hosted compiler, then there might
    // be issues like QuerySemantics - assert in 1571 (throws in ret) where
    // block context is assumed when this is set to true.
    //
    m_CreateImplicitDeclarations = false;

    // ExprForceRValue is required so that only valid RHS expressions are
    // permitted and the expression is completely converted to an RValue.
    //
    // Examples include 
    // - call to Subs (i.e. any methods returning void type) are disallowed
    //  as expressions
    //
    // - array inference is completed and inference errors shown if any, and
    //  if no errors, array element conversions to the inferred type completed.
    //
    ExpressionFlags Flags = ExprForceRValue;

    ILTree::Expression *Result = NULL;

    // ISSUE: The below code should be re-factored and re-used by Interpret----igment
    // InterpretInitializer and this function. Not re-factoring now in order to avoid
    // code churn in the core compiler this close to Beta2.
    //
    if (Input->Opcode == ParseTree::Expression::ArrayInitializer)
    {
        // For this case too, ideally a call to InterpretExpressionWithTargetType
        // should suffice just like the "else" case below. But the implementation
        // of InterpretExpressionWithTargetType does not pass the target type to
        // InterpretExpression (and eventually MakeRValue) which results in the
        // target type not being considered when determining the type for an array
        // initializer. Hence the special handling for this case.

        // ISSUE: The ExprDontInferResultType flag should not be needed in either
        // of the calls below since it does affect the processing of array literals.
        // But since this is present in the implementation for the assignment
        // statement (InterpretAssignment), and given the lateness of this change,
        // going ahead and including this flag in order to avoid any risk for Beta2.
        
        Result = InterpretExpression(Input, Flags | ExprDontInferResultType, 0, NULL, TargetType);

        if (TargetType != NULL && !IsBad(Result))
        {
            Result = ConvertWithErrorChecking(Result, TargetType, Flags | ExprDontInferResultType);
        }
    }
    else
    {
        Result = InterpretExpressionWithTargetType(Input, Flags, TargetType);
    }

    if (Result && !IsBad(Result))
    {
        // Void expressions are not permitted in expression contexts.

        // If the expression is not already bad, then need to check for expressions
        // returning void (like addressof) that are valid expressions only in
        // certain contexts, but can never be a free-standing expression with
        // their own type.
        //
        // This check needs to be done here since the compiler permits the 
        // addressof expr even when forcing to RValue with the idea that when
        // completing the compilation of the context where the addressof is used,
        // the addressof will then either be transformed to a non-void expression
        // or result in compile errors.

        if (TypeHelpers::IsVoidType(Result->ResultType))
        {
            ReportSemanticError(
                ERRID_VoidValue,
                Result->Loc);

            Result = MakeBad(Result);
        }
    }

    return Result;
}
#endif HOSTED

ConstantValue
Semantics::InterpretConstantExpression
(
    ParseTree::Expression *Input,
    Scope *Lookup,
    Type *TargetType,
    bool ConditionalCompilationExpression,
    _Out_ bool *ResultIsBad,
    BCSYM_NamedRoot *ContextOfSymbolUsage,
    bool IsSyntheticExpression,
    _Out_opt_ ILTree::Expression **pExpressionResult
)
{
    ExpressionFlags Flags = ExprForceRValue | ExprMustBeConstant;

    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,
        NULL,       // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,      // cannot interpret statements
        true,      // can interpret expressions
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false,       // reset closures information
        ContextOfSymbolUsage);

    m_CreateImplicitDeclarations = false;

    // Backup old value indicating whether a synthetic constant
    // is being evaluated
    //
    bool OriginalStateValue = m_IsEvaluatingSyntheticConstantExpression;

    m_IsEvaluatingSyntheticConstantExpression = IsSyntheticExpression;

    if (ConditionalCompilationExpression)
    {
        m_EvaluatingConditionalCompilationConstants = true;
        m_UsingOptionTypeStrict = false;
    }

    // Question: is "Nothing" a constant?
    // VB spec $11.2: "A constant expression is an expression whose value can be fully evaluated at compile time"
    // The spec goes on to say that expressions involving dynamic allocation are not constants, and that
    // constants only include the intrinsic types, and that constants can only include the basic operators and
    // conversions, and that every operand along the way has to be a constant.
    // So the answer is, it depends: "Nothing" is a constant if and only if default(TargetType) is a constant,
    // i.e. has intrinsic type.
    //    const a as integer = Nothing      -- is a constant, since default(integer)=0 which is a constant
    //    const b = ctype(nothing, integer) -- is a constant, for the same reason
    //    const c as integer? = Nothing     -- not a constant, since default(integer?) is a newly allocated structure, not a constant
    //    const d = ctype(nothing, integer?)-- not a constant either
    //    const e as object = nothing       -- is a constant, since "nullref" is a valid constant
    //    const f = nothing                 -- is a constant, since when TargetType is absent we interpret Nothing to have type Object
    //    const g = {1,2,3}                 -- not a constant, since it involves dynamic allocation.
    //    const h = CType(nothing,Integer?).Value -- not a constant, since types of some operands along the way are not intrinsict.
    //
    // Question: what about attribute arguments?
    // The CLR uses an entirely different mechanism to store attribute arguments: ECMA $II.23.3.
    // It allows primitive value types, strings, System.Types, or arrays of any of these things.
    // So in the VB compiler, an attribute argument must either be a valid constant, or a System.Type,
    // or an array of one of these things. Note that e.g. GetType(String) or {1,2,3} are NOT constants
    // even though they're allowed in attribute arguments.
    //
    // Question: what about optional parameter defaults?
    // The CLR uses an entirely different mechanism to store parameter defaults: ECMA $II.16.2,
    // Also, through VB convention, two other types can be stored as defaults through attributes,
    // DecimalConstantAttribute and DateTimeConstantAttribute.
    // It so happens that $II.16.2 + decimal + datetime = the complete list of VB primitive types
    // expressible as constants. That's handy.
    // One additional part of $II.16.2 is the ability to store "nullref". We use this to encode "Nothing".
    // Thus, even though values like IntPtr and Nullable<Integer> are non-primitive structures,
    // and so default(IntPtr) and default(Nullable<Integer>) are NOT constants, they're
    // still allowed in optional parameter arguments. This is handled by Bindable::EvaluateDeclaredExpression,
    // before it even calls InterpretConstantExpression.
    //
    // Question: what about constant declarations, e.g. "Const x = 1" or "Const x as Integer = 1"
    // This function we're in coincides exactly with what the spec is allowed in constant declarations.
    //
    // Question: what about conditional compilation constants?
    // Again, they coincide exactly with this function we're in.
    //
    // Please note that this function, InterpretConstantExpression, only interprets constants
    // It emits an error-message for optional-parameter-defaults like "Nothing" where target-type is IntPtr,
    // and it emits an error-message for attribute-arguments like "{1,2,3}".
    // It is up to the caller to handle these special cases elsewhere.

    VSASSERT(TargetType==NULL || !TypeHelpers::IsNullableType(TargetType),
             "Internal error: a nullable type is not an intrinsic, hence not a constant, so you should never call InterpretConstantExpression with a nullable target type");
    // Note: early in Dev10 we used to handle nullable types here. But now their special-case handling has been
    // moved to Bindable::EvaluateDeclaredExpression since it only ever applied to parameter defaults.
    // e.g. a field "Const x as Integer? = 1" is not allowed, since constant fields must have intrinsic type,
    // and Integer? isn't intrinsic.

    ILTree::Expression *ExpressionResult = InterpretExpressionWithTargetType(Input, Flags | ExprForceRValue, TargetType);
    
    ConstantValue Result;
    if (IsBad(ExpressionResult) || !IsConstant(ExpressionResult) ||
        (m_EvaluatingConditionalCompilationConstants &&
        TypeHelpers::IsArrayType(ExpressionResult->ResultType)))
    {
        // DevDiv Bug #162847: things like #Date1#-#Date2# will yield a TimeSpan, which isn't a constant, so we report an error

        if (!IsBad(ExpressionResult))
        {
            ReportSemanticError(
                ERRID_ConstAsNonConstant,
                Input->TextSpan);
        }

        Result.TypeCode = t_bad;
        *ResultIsBad = true;
    }
    else
    {
        Result = ExtractConstantValue(ExpressionResult);
        *ResultIsBad = false;
    }

    // Restore old value indicating whether a synthetic constant
    // is being evaluated
    //
    m_IsEvaluatingSyntheticConstantExpression = OriginalStateValue;

    if (pExpressionResult!=NULL)
    {
        *pExpressionResult = (*ResultIsBad) ? NULL : ExpressionResult;
    }
    return Result;
}


ConstantValue
Semantics::ConvertConstantExpressionToConstantWithErrorChecking
(
    ILTree::Expression *Input,
    Type *TargetType,
    _Out_ bool &ResultIsBad
)
{
    ConstantValue Result;

    if (Input==NULL || IsBad(Input) || !IsConstant(Input))
    {
        VSFAIL("Please supply a constant input");
        if (Input!=NULL)
        {
            ReportSemanticError(ERRID_InternalCompilerError, Input->Loc);
        }
        ResultIsBad = true;
        Result.TypeCode = t_bad;
        return Result;
    }
 
    ILTree::Expression *expr = ConvertWithErrorChecking(
                                    Input,
                                    TargetType,
                                    ExprForceRValue | ExprMustBeConstant);
    
    ResultIsBad = IsBad(expr);
    
    if (ResultIsBad)
    {
        Result.TypeCode = t_bad;
    }
    else
    {
        Result = ExtractConstantValue(expr);
    }
    return Result;
}



ILTree::Expression *
Semantics::InterpretInitializer
(
    _In_ ParseTree::Initializer *Input,
    Scope *Lookup,
    ILTree::Expression *WithStatementContext,
    Type *TargetType,
    bool MergeAnonymousTypes,
    ExpressionFlags flags
)
{
    ParseTree::Expression *pInitializer = ParserHelper::DigToInitializerValue(Input);
    bool fInterpretStatements = false;      // cannot interpret statements
    if (ParserHelper::TryGetLambdaBodyFromParseTreeNode(pInitializer, false))
    {
        fInterpretStatements = true;
    }
        
    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,       // No cycle detection
        NULL,       // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        true,       // preserve extra semantic information
        true,       // perform obsolete checks
        fInterpretStatements,
        true,       // can interpret expressions
        MergeAnonymousTypes,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false);     // reset closures information

    m_CreateImplicitDeclarations = flags & ExprCreateImplicitDeclarations;

    if (WithStatementContext)
    {
        m_EnclosingWith = &AllocateStatement(SB_WITH, WithStatementContext->Loc, 0, false)->AsWithBlock();
        InterpretWithStatement(WithStatementContext, m_EnclosingWith);
    }

    if (Input->Opcode == ParseTree::Initializer::Deferred)
    {
        Input = Input->AsDeferred()->Value;
    }

    return InterpretInitializer(Input, TargetType, ExprForceRValue | flags);
}

Type *
Semantics::InterpretType
(
    ParseTree::Type *Type,
    Scope *Lookup,
    _Out_ bool &TypeIsBad,
    TypeResolutionFlags Flags
)
{
    InitializeInterpretationState(
        NULL,
        Lookup,
        NULL,       // No cycle detection
        NULL,       // No call graph
        m_SymbolsCreatedDuringInterpretation,       // transient symbols
        false,      // preserve extra semantic information
        true,       // perform obsolete checks
        false,      // cannot interpret statements
        false,      // cannot interpret expressions
        m_MergeAnonymousTypeTemplates,  // can merge anonymous type templates
        NULL,       // Applied attribute context
        false);     // reset closures information

    return InterpretTypeName(Type, TypeIsBad, Flags);
}

void
Semantics::InitializeInterpretationState
(
    _In_ ILTree::ProcedureBlock *MethodBody,
    Scope *Lookup,
    Cycles *CycleDetection,
    CallGraph *CallTracking,
    _In_ TransientSymbolStore *CreatedDuringInterpretation,
    bool PreserveExtraSemanticInformation,
    bool PerformObsoleteChecks,
    bool CanInterpretStatements,
    bool CanInterpretExpressions,
    bool MergeAnonymousTypeTemplates,
    Declaration *AppliedAttributeContext,
    bool ResetClosuresInfo,
    Declaration *NamedContextForConstantUsage
#if IDE 
    , FromItemsHashTable *FromItemsMap
#endif
)
{
    // Initialize the per-interpretation state.

    m_Lookup = Lookup;
    m_ContainingClass = NULL;
    m_ContainingContainer = NULL;
    m_NamedContextForAppliedAttribute = AppliedAttributeContext;
    m_SuppressNameLookupGenericBindingSynthesis = false;
    m_NamedContextForConstantUsage = NamedContextForConstantUsage;

    if (m_SourceFile)
    {
        m_Project = m_SourceFile->GetProject();
        m_UnnamedNamespaceForNameLookupInImports = m_SourceFile->GetUnnamedNamespace();
    }
    else
    {
        m_Project = NULL;
        if (Lookup)
        {
            CompilerFile *LookupFile = Lookup->GetCompilerFile();
            if (LookupFile)
            {
                m_Project = LookupFile->GetProject();
                m_UnnamedNamespaceForNameLookupInImports = LookupFile->GetUnnamedNamespace();
            }
        }
    }

    // Set the caches in case they haven't been set before.
#if IDE 
    if (GetCompilerSharedState()->IsInBackgroundThread())
#endif
    {
        if (m_Project)
        {
            if (!m_LookupCache)
            {
                m_LookupCache = m_Project->GetLookupCache();
            }

            if (!m_ExtensionMethodLookupCache)
            {
                m_ExtensionMethodLookupCache = m_Project->GetExtensionMethodLookupCache();
            }

            if (!m_LiftedOperatorCache)
            {
                m_LiftedOperatorCache = m_Project->GetLiftedOperatorCache();
            }
        }

        if (GetCompilerHost() && !m_MergedNamespaceCache)
        {
            m_MergedNamespaceCache = GetCompilerHost()->GetMergedNamespaceCache();
        }
    }

    m_XmlSymbols.SetContext(Lookup, m_Compiler, GetCompilerHost());
    m_ProcedureTree = MethodBody;
    if (MethodBody)
    {
        m_ProcedureTree->Locals = Lookup;
    }
    Declaration *LookupParent = Lookup ? Lookup->GetParent() : NULL;
    m_Procedure = (LookupParent && LookupParent->IsProc()) ? LookupParent->PProc() : NULL;
    m_NoIntChecks = m_Project && m_Project->RemoveIntChecks() || m_Procedure && m_Procedure->IsSyntheticMethod() && m_Procedure->PSyntheticMethod()->GetSyntheticKind() == SYNTH_TransientNoIntCheckSymbol;

#if IDE 
    m_FromItemsMap = FromItemsMap;
#endif

    m_EnclosingWith = NULL;
    m_ConstructorCycles = CycleDetection;
    m_CallGraph = CallTracking;

    m_SymbolsCreatedDuringInterpretation = CreatedDuringInterpretation;
    m_MergeAnonymousTypeTemplates = MergeAnonymousTypeTemplates;

    if (m_SymbolsCreatedDuringInterpretation)
    {
        m_TransientSymbolCreator.SetNorlsAllocator(m_SymbolsCreatedDuringInterpretation->SymbolStorage());
    }
    else
    {
        m_TransientSymbolCreator.SetNorlsAllocator(&m_TreeStorage);
    }

    if (ResetClosuresInfo)
    {
        InitClosures(
                m_Procedure,
                m_Procedure ? m_Procedure->GetBoundTree() : NULL);
    }

    m_LabelHashBucketCount = MethodBody ? max(MethodBody->DefinedLabelCount * 2, 7) : 1;
    typedef LabelDeclarationList *LabelDeclarationPointer;
    m_LabelHashBuckets = CanInterpretStatements ? new(m_TreeAllocator) LabelDeclarationPointer[m_LabelHashBucketCount] : NULL;

    m_CompilingConstructorDefinition = false;
    m_ExpectingConstructorCall = false;
    m_SynthesizingBaseCtorCall = false;
    m_DisallowMeReferenceInConstructorCall = false;
    m_ProcedureContainsTry = false;
    m_ProcedureContainsOnError = false;
    m_EvaluatingConditionalCompilationConstants = false;

    m_EvaluatingConditionalCompilationConstants = false;
    m_UsingOptionTypeStrict = m_SourceFileOptions & OPTION_OptionStrict;

    {
        m_CreateImplicitDeclarations = !(m_SourceFileOptions & OPTION_OptionExplicit);

        // Always do obsolete checking unless told otherwise.
        // Never perform obsolete checks when analyzing synthetic code because all useful checks are performed
        // on the declaration.  To clarify, synthetic methods are always side-effects of certain kinds of
        // declarations and obsolete checking is performed on declarations during step to CS_SymbolsChecked.
        m_PerformObsoleteChecks = PerformObsoleteChecks && !WithinSyntheticCode();

        //=========Start=============
        // The following is added for Bug482094. If the current symbol is a container or a procedure, we need to 
        // decide whether to check obsolete in this container or procedure. There is no any senario that change m_PerformObsoleteChecks
        // from false to true, then the folowing check is only needed if m_PerformObsoleteChecks is true.
        // 
        // In general we need to check whether the current procedure or container or indirect container is obsolete, 
        // if any of them is obsolete then no obsolete check is needed.
        //
        // ObsoleteCheckTargetContainer is the closest container(m_procedure does not consider as a container here), it is used
        // to get information of compilation state, this information is needed to determine whether it is ready to check attributes.
        // The method is similar to the method used in ObsoleteChecker::CheckObsolete. The reason why we copy the method from
        // ObsoleteChecker::CheckObsolete is that we need to ensure that if performing obsolete check can not be determined now, then 
        // "m_PerformObsoleteChecks == true" actually means that we don't know obsolete check is needed or not, so the symbols should
        // be added to DelayedObsoleteCheckInfo later in ObsoleteCheck.
        if (m_PerformObsoleteChecks)
        {
            BCSYM_NamedRoot * ObsoleteCheckTarget = GetContextOfSymbolUsage();
            BCSYM_Container * ObsoleteCheckTargetContainer = ContainingContainer();

            if ( ObsoleteCheckTarget!= NULL && 
                 ObsoleteCheckTargetContainer != NULL &&
                 m_SourceFile != NULL &&
                 m_SourceFile->GetCompState() >= CS_Bound &&
                 ObsoleteCheckTargetContainer->IsBindingDone())
            {
                m_PerformObsoleteChecks = !ObsoleteChecker::IsObsoleteOrHasObsoleteContainer(ObsoleteCheckTarget);
            }
        }
        //=========End==============
    }

    m_PreserveExtraSemanticInformation = PreserveExtraSemanticInformation | m_IsGeneratingXML;

    m_BlockContext = NULL;
    m_StatementTarget = NULL;
    m_LastStatementInContext = NULL;

    m_TemporaryManager =
        CanInterpretExpressions ?
            new(m_TreeAllocator)
                TemporaryManager(
                    m_Compiler,
                    m_CompilerHost,
                    MethodBody ? MethodBody->pproc : NULL,
                    &m_TreeStorage) :
            NULL;
}

const bool TypeAllowsCompileTimeConversions[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  allowconversion,
    #include "..\Symbols\TypeTables.h"
    #undef DEF_TYPE
};

bool
AllowsCompileTimeConversions
(
    Type *TargetType
)
{
#pragma prefast(suppress: 26010 26011, "Can only be outside of bounds if symbol is bad")
    return TypeAllowsCompileTimeConversions[TargetType->GetVtype()];
}

const bool TypeAllowsCompileTimeOperations[] =
{
    #define DEF_TYPE(type, clrscope, clrname, vbname, size, token, isnumeric, isintegral, isunsigned, isreference, allowoperation, allowconversion)  allowoperation,
    #include "..\Symbols\TypeTables.h"
    #undef DEF_TYPE
};

bool
AllowsCompileTimeOperations
(
    Type *TargetType
)
{
#pragma prefast(suppress: 26010 26011, "Can only be outside of bounds if symbol is bad")
    return TypeAllowsCompileTimeOperations[TargetType->GetVtype()];
}

bool
Semantics::IsOrInheritsFrom
(
    Type *Derived,
    Type *Base
)
{
    return TypeHelpers::IsOrInheritsFrom(Derived, Base, m_SymbolCreator, false, NULL);
}



// =================================================================================================
// DoesReceiverMatchInstance
// This function is used to check whether an extension method matches an instance (both in
// the compiler and for intellisense), and also for method overloading.
// For a receiver to match an instance, more or less, the type of that instance has to be convertible
// to the type of the receiver with the same bit-representation (i.e. identity on value-types
// and reference-convertibility on reference types).
// Actually, we don't include the reference-convertibilities that seem nonsensical, e.g. enum() to underlyingtype()
// We do include inheritance, implements and variance conversions amongst others.
//
// Note: this function used to be called "IsSubTypeOf".
// Beware of breaking the function up into smaller functions with similar general-sounding names.
// The function itself is tied to the question of whether the receiver matches an instance,
// and all the subtests it does which recursively call DoesReceiverMatchInstance are also
// tied to this purpose and are not general-purpose. That's really why they're all inlined here
// rather than split out. (Also, we believe that this function body can be done away with
// entirely, so there's no point factoring it out.)
//
bool Semantics::DoesReceiverMatchInstance(Type * InstanceType, Type * ReceiverType)
{
    VSASSERT(InstanceType!=NULL && ReceiverType!=NULL, "Please don't call DoesReceiverMatchInstance with NULL arguments");

    bool Matches = false;

    // An "object" receiver can match any interface:
    if (ReceiverType->IsObject() && InstanceType->IsInterface())
    {
        Matches = true;
    }

    // Inheritance:
    else if (IsOrInheritsFromOrImplements(InstanceType, ReceiverType))
    {
        Matches = true;
    }

    // IsUnconstrainedTypeParameterInheritingFromObject ?
    // Microsoft - Dev Div Bugs # 26379
    // For some reason IsOrInheritsFromOrImplements does not return true
    // when InstanceType is an unconstrained type parameter and ReceiverType is object.
    // To compensate for this, I introduce a check for this exact case here:
    else if (   InstanceType->IsGenericParam() && ReceiverType->IsObject()
             && !InstanceType->PGenericParam()->GetConstraints())
    {
        Matches = true;
    }

    // IsCovariantArray ?
    else if (  InstanceType->IsArrayType() && ReceiverType->IsArrayType()
            && InstanceType->PArrayType()->GetRank() == ReceiverType->PArrayType()->GetRank()
            && DoesReceiverMatchInstance(InstanceType->PArrayType()->GetRoot(), ReceiverType->PArrayType()->GetRoot()))
    {
        Matches = true;
    }


    // Variance from a non-array to an interface? (we deal with the array case later)
    else if (  ReceiverType->IsInterface() && ReceiverType->IsGenericTypeBinding() && TypeHelpers::HasVariance(ReceiverType)
            && !InstanceType->IsArrayType()) // this check !IsArrayType is a shortcut; could be ommitted
    {
        ConversionClass result =
            ClassifyAnyVarianceConversionToInterface(
                ReceiverType,
                InstanceType,
                m_SymbolCreator,
                m_CompilerHost,
                ConversionSemantics::ForExtensionMethods,
                0, // RecursionCount=0 because we're calling it from outside
                false,
                NULL,
                NULL
                );

        if (result==ConversionIdentity || result==ConversionWidening)
        {
            Matches = true;
            // this also covers the case where SourceType is a generic param with appropriate constraints
        }
    }

    // Variance from something to a delegate?
    else if (  ReceiverType->IsDelegate() && ReceiverType->IsGenericTypeBinding() && TypeHelpers::HasVariance(ReceiverType)
             && !InstanceType->IsGenericParam())
    {
        // Question: why don't we also need to check for variance from a generic-param to a delegate?
        // Answer: well, generic-params aren't allowed to have class-constraints to delegate types.
        // And delegate types are non-inheritable, so no class constraint can ever let us infer that
        // a generic type is a delegate. Also, if the generic-param has interface constraints, well,
        // no interface constraints can ever let us infer that a generic type is a delegate type.

        // Question: why do we only look at InstanceType and not walk up its inheritance-chain?
        // Answer: because delegates are not-inheritable.

        if (InstanceType->IsDelegate() && InstanceType->IsGenericTypeBinding() && TypeHelpers::HasVariance(InstanceType))
        {
            if (HasVarianceConversionToDelegate(ReceiverType, InstanceType, ConversionSemantics::Default, 0, m_SymbolCreator, m_CompilerHost))
            {
                Matches = true;
            }
        }
    }

    // IsArrayInheritingFromSystemArrayOrItsImplementedInterfaces ?
    else if (  InstanceType->IsArrayType()
            && IsOrInheritsFromOrImplements(GetFXSymbolProvider()->GetType(FX::ArrayType), ReceiverType))
    {
        Matches = true;
    }

    // IsArrayInheritingFromOrVarianceConvertibleToGenericCollectionInterface ?
    else if (InstanceType->IsArrayType() && InstanceType->PArrayType()->GetRank()==1
          && ReceiverType->IsGenericTypeBinding() && ReceiverType->PGenericTypeBinding()->GetGeneric()->IsInterface()
          && ( (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIListType)
                && BCSYM::AreTypesEqual(ReceiverType->PGenericTypeBinding()->GetGeneric(), GetFXSymbolProvider()->GetType(FX::GenericIListType)))
              ||
              (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIReadOnlyListType)
                && BCSYM::AreTypesEqual(ReceiverType->PGenericTypeBinding()->GetGeneric(), GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyListType)))
              ||
              (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIReadOnlyCollectionType)
                && BCSYM::AreTypesEqual(ReceiverType->PGenericTypeBinding()->GetGeneric(), GetFXSymbolProvider()->GetType(FX::GenericIReadOnlyCollectionType)))
              ||
               (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericICollectionType)
                && BCSYM::AreTypesEqual(ReceiverType->PGenericTypeBinding()->GetGeneric(), GetFXSymbolProvider()->GetType(FX::GenericICollectionType)))
              ||
               (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEnumerableType)
               && BCSYM::AreTypesEqual(ReceiverType->PGenericTypeBinding()->GetGeneric(), GetFXSymbolProvider()->GetType(FX::GenericIEnumerableType)))
             )
          && ( (TypeHelpers::IsReferenceType(ReceiverType->PGenericBinding()->GetArgument(0))
                && TypeHelpers::IsReferenceType(InstanceType->PArrayType()->GetRoot()))
              ||
               (TypeHelpers::IsValueType(ReceiverType->PGenericBinding()->GetArgument(0))
                && TypeHelpers::IsValueType(InstanceType->PArrayType()->GetRoot()))
              ||
               (TypeHelpers::IsGenericParameter(ReceiverType->PGenericBinding()->GetArgument(0))
                && TypeHelpers::IsGenericParameter(InstanceType->PArrayType()->GetRoot()))
             )
          && DoesReceiverMatchInstance(InstanceType->PArrayType()->GetRoot(), ReceiverType->PGenericTypeBinding()->GetArgument(0)))
    {
        Matches = true;
    }

    else
    {
        Matches = false;
    }


    // 

    VPASSERT(Matches == 
        (   ::ClassifyPredefinedCLRConversion(ReceiverType,InstanceType,m_SymbolCreator,m_CompilerHost,ConversionSemantics::ForExtensionMethods,0,false,NULL,NULL,NULL)==ConversionIdentity
         || ::ClassifyPredefinedCLRConversion(ReceiverType,InstanceType,m_SymbolCreator,m_CompilerHost,ConversionSemantics::ForExtensionMethods,0,false,NULL,NULL,NULL)==ConversionWidening),
             "DoesReceiverMatchInstance/ClassifyPredefinedCLRConversion mismatch; please tell Microsoft");

    return Matches;
}


bool
Semantics::IsOrInheritsFromOrImplements
(
    Type *Derived,
    Type *Base
)
{
    return TypeHelpers::IsOrInheritsFromOrImplements(Derived, Base, m_SymbolCreator, false, NULL, m_CompilerHost);
}

Type *
Semantics::GetBaseClass
(
    Type *Derived
)
{
    return TypeHelpers::GetBaseClass(Derived, m_SymbolCreator);
}

bool
AreAssembliesEqualByName
(
    CompilerProject *Project1,
    CompilerProject *Project2
)
{
    return StringPool::IsEqual(GetAssemblyName(Project1), GetAssemblyName(Project2));
}

// Return the name of the output assembly for source projects and the name of the
// internal assembly name for metadata projects. Internal assembly name == file
// name almost all the time for metadata projects too.
//
STRING *
GetAssemblyName
(
    CompilerProject *Project
)
{
    STRING *AssemblyName = Project->GetAssemblyName();

    if (!AssemblyName)
    {
        // For .Net modules in VB source.
        Project->GetRootOfPEFileName(&AssemblyName);
    }

    return AssemblyName;
}

// Return the name of the project for source projects and the name of the "file.dll" for
// metadata projects.
//
STRING *
GetErrorProjectName
(
    CompilerProject *Project
)
{
    return
        Project->IsMetaData() ? Project->GetFileNameWithNoPath() : Project->GetFileName();
}

void
ReportSmartReferenceError
(
    unsigned ErrorId,
    _In_ CompilerProject *ReferencingProject,
    CompilerProject *RequiredReferenceProject,
    Compiler *Compiler,
    ErrorTable *ErrorTable,
    _In_z_ WCHAR *Extra,
    const Location *ErrorLocation,
    ...
)
{
    // Find if the current referencing project references an entity that may
    // be similar to RequiredReferenceProject.

    ReferenceIterator References(ReferencingProject);
    CompilerProject *CurrentReference = NULL;

    while (CurrentReference = References.Next())
    {
        if (CurrentReference == RequiredReferenceProject)
        {
            continue;
        }

        if (AreAssembliesEqualByName(CurrentReference, RequiredReferenceProject))
        {
            break;
        }
    }


    bool ErrorReported = false;

    if (CurrentReference)
    {
        // Get the default error string after substituting the replacements.

        StringBuffer DefaultErrorString;

        va_list ap;
        va_start(ap, ErrorLocation);

        ResLoadStringReplArgs(
            ErrorId,
            &DefaultErrorString,
            NULL,
            ap);

        va_end(ap);


        if (CurrentReference->IsMetaData() && RequiredReferenceProject->IsMetaData())
        {
            // Multiple copies of the same DLL.

            CompilerProject *Project1 = RequiredReferenceProject->GetFirstReferencingProject();

            if (Project1)
            {
                ErrorTable->CreateError(
                    ERRID_GeneralErrorMixedDLLs5,
                    (Location *)ErrorLocation,
                    DefaultErrorString.GetString(),
                    RequiredReferenceProject->GetFileName(),         // Path to the DLL
                    GetErrorProjectName(Project1),
                    CurrentReference->GetFileName(),                 // Path to the DLL
                    GetErrorProjectName(ReferencingProject));

                ErrorReported = true;
            }
        }
        else if (!CurrentReference->IsMetaData() && !RequiredReferenceProject->IsMetaData())
        {
            // Multiple copies of the same DLL.

            // Extremely uncommon scenario. No specialized error message in this case.
        }
        else
        {
            // Project and DLL mixed.

            CompilerProject *MetadataProject =
                CurrentReference->IsMetaData() ? CurrentReference : RequiredReferenceProject;

            CompilerProject *NonMetadataProject =
                CurrentReference->IsMetaData() ? RequiredReferenceProject : CurrentReference;

            CompilerProject *Project1 =
                (ReferencingProject && ReferencingProject->IsProjectReferenced(MetadataProject)) ?
                    ReferencingProject :
                    MetadataProject->GetFirstReferencingProject();

            if (Project1)
            {
                ErrorTable->CreateError(
                    ERRID_GeneralErrorDLLProjectMix5,
                    (Location *)ErrorLocation,
                    DefaultErrorString.GetString(),
                    GetAssemblyName(MetadataProject),
                    GetErrorProjectName(MetadataProject),
                    GetErrorProjectName(Project1),
                    GetErrorProjectName(NonMetadataProject));

                ErrorReported = true;
            }
        }
    }

    if (!ErrorReported)
    {
        // Report the default error

        va_list ap;
        va_start(ap, ErrorLocation);

        ErrorTable->CreateErrorArgsBase(
            ErrorId,
            NULL,
            Extra,
            (Location *)ErrorLocation,
            NULL,
            ap);

        va_end(ap);
    }
}

unsigned
CheckConstraintsOnNew
(
    _Inout_ Type *&TypeOfInstance
)
{
    WCHAR *DummyPointer;

    // Allow creation of instances of intrinsic types.
    if (TypeHelpers::IsIntrinsicType(TypeOfInstance))
    {
        TypeOfInstance = TypeOfInstance->DigThroughAlias();
    }

    // Note that the GenericParameter check is done before the other check
    // because otherwise some unnecessary checks on the constraints of generic
    // parameters are done, sometimes even before the constraint types have
    // been resolved causing possible crashes.
    //
    if (TypeHelpers::IsGenericParameter(TypeOfInstance))
    {
        if (!TypeOfInstance->PGenericParam()->CanBeInstantiated())
        {
            return ERRID_NewIfNullOnGenericParam;
        }
    }

    else if (!(TypeHelpers::IsClassType(TypeOfInstance) &&
               !TypeOfInstance->PClass()->IsCantNew()) &&
             !TypeHelpers::IsValueType(TypeOfInstance) &&
             !(TypeHelpers::IsInterfaceType(TypeOfInstance) &&
               TypeOfInstance->GetPWellKnownAttrVals()->GetCoClassData(&DummyPointer) && DummyPointer != NULL))
    {
        unsigned ErrorId = 0;

        if (!TypeHelpers::IsClassType(TypeOfInstance))
        {
            ErrorId = ERRID_NewIfNullOnNonClass;
        }
        else if (TypeOfInstance->PClass()->IsStdModule())
        {
            ErrorId = ERRID_ExpectedNewableClass1;
        }
        else if (!TypeOfInstance->PClass()->AreMustOverridesSatisfied())
        {
            ErrorId = ERRID_NewIfNullOnAbstractClass1;
        }
        else if (TypeOfInstance->PClass()->IsMustInherit())
        {
            ErrorId = ERRID_NewOnAbstractClass;
        }
        else
        {
            // No assert because a class could be not creatable because of a TypeLibType
            // attribute applied on it.
            //
            // VSFAIL("Surprising reason why class can't be instantiated.");

            ErrorId = ERRID_ExpectedNewableClass1;
        }

        return ErrorId;
    }

    return 0;
}

Procedure *
Semantics::FindHelperMethod
(
    _In_z_ Identifier *MethodName,
    ClassOrRecordType *ContainingType,
    const Location &TextSpan,
    bool SuppressErrorReporting
)
{
    VSASSERT(!ContainingType->IsBad(), "didn't expect bad container");

    Declaration *Method =
        ImmediateLookup(
            ViewAsScope(ChaseThroughAlias(ContainingType)->PContainer()),
            MethodName,
            BINDSPACE_Normal);

    if (Method == NULL ||
        !ChaseThroughAlias(Method)->IsProc())
    {
        if (!SuppressErrorReporting)
        {
            ReportuntimeHelperNotFoundError(
                TextSpan,
                ConcatNameSpaceAndName(
                    m_Compiler,
                    ContainingType->GetQualifiedName(false),
                    MethodName));
        }
        return NULL;
    }

    return ChaseThroughAlias(Method)->PProc();
}

ClassOrRecordType *
Semantics::FindHelperClass
(
    _In_z_ Identifier *ClassName,
    HelperNamespace ContainingNamespace,
    const Location &TextSpan
)
{
    STRING *NamespaceName;
    Namespace *Helpers = NULL;

    switch (ContainingNamespace)
    {
        case SystemNamespace:
            NamespaceName = STRING_CONST(m_Compiler, ComDomain);
            break;

        case MicrosoftVisualBasicNamespace:
            NamespaceName = STRING_CONST(m_Compiler, MicrosoftVisualBasic);
            break;

        case MicrosoftVisualBasicCompilerServicesNamespace:
            NamespaceName = STRING_CONST(m_Compiler, VBCompilerServices);
            break;

        default:
            VSFAIL("unknown value");
            ReportSemanticError(ERRID_InternalCompilerError, TextSpan);
            return NULL;
    }

    Helpers = m_Compiler->FindNamespace(NamespaceName);

    if (Helpers == NULL)
    {
        ReportuntimeHelperNotFoundError(
            TextSpan,
            ConcatNameSpaceAndName(
                m_Compiler,
                NamespaceName,
                ClassName));
        return NULL;
    }

    bool NameIsBad = false;

    Declaration *HelperClass =
        LookupInNamespace(
            Helpers,
            ClassName,
            NameNoFlags,
            BINDSPACE_Type,
            true,
            false,
            TextSpan,
            NameIsBad,
            NULL,   // No generic helpers yet
            -1);

    if (NameIsBad)
    {
        return NULL;
    }

    if (HelperClass == NULL ||
        !TypeHelpers::IsClassType(ChaseThroughAlias(HelperClass)))
    {
        ReportuntimeHelperNotFoundError(
            TextSpan,
            ConcatNameSpaceAndName(
                m_Compiler,
                NamespaceName,
                ClassName));
        return NULL;
    }

    return ChaseThroughAlias(HelperClass)->PClass();
}

Semantics::Semantics
(   _In_ NorlsAllocator *TreeStorage,
    ErrorTable *Errors,
    Compiler *TheCompiler,
    CompilerHost *CompilerHost,
    SourceFile *File,
    _In_ TransientSymbolStore *pCreatedDuringInterpretation,
    bool IsGeneratingXML,
    bool fIncludeBadExpressions,
    bool createdInParseTreeService
) :
    m_CompilerHost(CompilerHost),
    m_TreeStorage(*TreeStorage),
    m_FXSymbolProvider(CompilerHost->GetFXSymbolProvider()),
    m_EmbeddedTypeIdentity(NULL),
    m_Errors(Errors),
    m_SymbolCreator(TheCompiler, TreeStorage, NULL),
    m_CreateImplicitDeclarations(false),
    m_EvaluatingConditionalCompilationConstants(false),
    m_CompilingConstructorDefinition(false),
    m_ExpectingConstructorCall(false),
    m_SynthesizingBaseCtorCall(false),
    m_DisallowMeReferenceInConstructorCall(false),
    m_ProcedureContainsTry(false),
    m_ProcedureContainsOnError(false),
    m_UsingOptionTypeStrict(false),
    m_IsGeneratingXML(IsGeneratingXML),
    m_Lookup(NULL),
    m_LabelHashBucketCount(0),
    m_LabelHashBuckets(NULL),
    m_Compiler(TheCompiler),
    m_ProcedureTree(NULL),
    m_TransientSymbolOwnerProc(NULL),
    m_EnclosingWith(NULL),
    m_ConstructorCycles(NULL),
    m_CallGraph(NULL),
    m_LookupCache(NULL),
    m_MergedNamespaceCache(NULL),
    m_DoNotMergeNamespaceCaches(false),
    m_CompilationCaches(NULL),
    m_SourceFile(File),
    m_Project(NULL),
    m_ContainingClass(NULL),
    m_ContainingContainer(NULL),
    m_UnnamedNamespaceForNameLookupInImports(NULL),
    m_SuppressNameLookupGenericBindingSynthesis(false),
    m_IsEvaluatingSyntheticConstantExpression(false),
    m_NamedContextForAppliedAttribute(NULL),
    m_DefAsgCount(0),
    m_DefAsgAllocator(*TreeStorage),
    m_AltErrTablesForConstructor(NULL),
    m_fIncludeBadExpressions(fIncludeBadExpressions),
    m_pReceiverType(NULL),
    m_pReceiverLocation(NULL),
    m_ExplicitLoopVariableCreated(false),
    m_CreateExplicitScopeForLoop(0),
    m_InitializerInferStack(NULL),
    m_InitializerTargetIsArray(false),
    m_SymbolsCreatedDuringInterpretation(pCreatedDuringInterpretation),
    m_TransientSymbolCreator(TheCompiler, TreeStorage, NULL),
    m_MergeAnonymousTypeTemplates(true),
    m_closureRoot(NULL),
    m_InLambda(NotALambda),
    m_InQuery(false),
    m_InConstantExpressionContext(false),
    m_methodHasLambda(false),
    m_methodDeferredTempCount(0),
    m_statementGroupId(1),
    m_ExtensionMethodLookupCache(NULL),
    m_LiftedOperatorCache(NULL),
    m_InterpretingMethodBody(false),
    m_XmlNameVars(NULL),
    m_AnonymousTypeBindingTable(NULL),
    m_StatementLambdaInterpreter(NULL),
    m_ExpressionLambdaInterpreter(NULL),
    m_OuterStatementLambdaTree(NULL),
    m_ReportMultilineLambdaReturnTypeInferenceErrors(),
    m_NamedContextForConstantUsage(NULL),
    m_FieldInitializerContext(NULL),
    m_IsCreatingRelaxedDelegateLambda(false)
#if IDE 
    , m_FromItemsMap(NULL)
#endif
    ,m_XmlSemantics(NULL)
    ,m_UseQueryNameLookup(0)
    ,m_JoinKeyBuilderList(NULL)
#if IDE 
    ,m_nameFoundInProjectImports(false)
#endif
{
    m_CompilingLanguageVersion = File ? File->GetProject()->GetCompilingLanguageVersion() : LANGUAGE_CURRENT;
    m_ReportErrors = m_Errors ? true : false;
    m_TreeAllocator.Init(TheCompiler);
    m_TreeAllocator.SetAllocator(TreeStorage);
    m_SourceFileOptions = m_SourceFile ? m_SourceFile->GetOptionFlags() : 0;

    m_CreatedInParseTreeService = createdInParseTreeService;

    // Note (9/7/2001):
    //
    // The basic problem is that the IDE foreground thread should never be permitted to cache any
    // declarations for name lookup--only real compilation (the IDE background thread or the
    // command-line compiler) should be able to do so. The background compiler successfully invalidates
    // cached entries during demotion, but only for cached entries it has created. If the foreground
    // thread creates one, say because IntelliSense, XMLGen or pretty listing interprets an expression
    // in a method body, bad things can happen if a demotion happens at exactly the wrong time.
    //
    // Let execution on the background thread be the decision which determines whether caching is allowed.

    m_PermitDeclarationCaching = true;

#if IDE 
    if (GetCompilerSharedState()->IsInMainThread())  // Could be in Dbg thread too
    {
        if (m_SourceFile && m_SourceFile->GetProject())
        {
            CompilationCaches *pCompilationCaches;
            VSASSERT(!m_SourceFile->IsMetaDataFile(),"How can metadatafile in main thread want compilation cache?");
            pCompilationCaches = GetCompilerCompilationCaches();
            SetIDECompilationCaches(pCompilationCaches);
        }
        
    }
    else
#endif
    {
        if(m_SourceFile && m_SourceFile->GetProject())
        {
            m_LookupCache = m_SourceFile->GetProject()->GetLookupCache();
            m_ExtensionMethodLookupCache = m_SourceFile->GetProject()->GetExtensionMethodLookupCache();
            m_LiftedOperatorCache = m_SourceFile->GetProject()->GetLiftedOperatorCache();
        }

        if (GetCompilerHost())
        {
            m_MergedNamespaceCache = GetCompilerHost()->GetMergedNamespaceCache();
        }
    }

    // In the IDE case, since semantics objects and their allocators are not used long enough, caching bindings
    // might actually be overhead most of the time and may not be worthwhile.
    //
    // 




    if (File && File->SymbolStorage() == TreeStorage)
    {
        m_SymbolCreator.SetGenericBindingCache(File->GetCurrentGenericBindingCache());
    }
}

Type *
Semantics::GetPointerType
(
    Type *PointedToType
)
{
    return m_SymbolCreator.MakePtrType(PointedToType);
}

void
Semantics::LogDependency
(
    Declaration *DependedOn
)
{
#if IDE 
    if (m_SourceFile && DependedOn != Symbols::GetGenericBadNamedRoot())
    {
        // Do not set usage for namespaces. Since they cannot be used independently
        // (statements, expressions, &c), just wait for the acutal symbol used.
        if (!DependedOn->IsNamespace())
        {
            // Record reference usage
            VSASSERT(m_SourceFile->GetCompilerProject(), "Sourcefile without project?");

            m_SourceFile->GetCompilerProject()->SetReferenceUsageForSymbol(DependedOn);
        }
    }
#endif
}

#if IDE 
void
Semantics::SetIDECompilationCaches(CompilationCaches *pCompilationCaches)
{
    if (GetCompilerSharedState()->IsInMainThread() && pCompilationCaches)
    {
        m_LookupCache = pCompilationCaches->GetLookupCache();
        m_ExtensionMethodLookupCache = pCompilationCaches->GetExtensionMethodLookupCache();
        m_LiftedOperatorCache = pCompilationCaches->GetLiftedOperatorCache();
        m_MergedNamespaceCache = pCompilationCaches->GetMergedNamespaceCache();

        m_CompilationCaches = pCompilationCaches;
    }
}
#endif IDE

bool
Semantics::WithinModule
(
)
{
    ClassOrRecordType *Module = ContainingClass();
    return Module && Module->IsStdModule();
}


bool
RefersToGenericParameter
(
    Type *PossiblyGenericType,
    _Inout_ NorlsAllocator *allocator,
    _Out_ BCSYM_GenericParam **&ListOfGenericParamsFound,
    _Out_ unsigned *NumberOfGenericParamsFound
)
{
    unsigned NumberOfGenericParams = 0;

    if (!RefersToGenericParameter(PossiblyGenericType, NULL, NULL, NULL, 0, &NumberOfGenericParams))
    {
        return false;
    }

    if (!VBMath::TryMultiply(sizeof(BCSYM_GenericParam *), NumberOfGenericParams))
    {
        VSASSERT(false, "Adding assert to catch this case, original code failed silently, would like to know why this is here");
        return false; // Overflow, definitely something weird going on -- just return gracefully.
    }

    ListOfGenericParamsFound = (BCSYM_GenericParam **)allocator->Alloc(sizeof(BCSYM_GenericParam *) * NumberOfGenericParams);
    unsigned BufferSize = NumberOfGenericParams;
    NumberOfGenericParams = 0;

    RefersToGenericParameter(PossiblyGenericType, NULL, NULL, ListOfGenericParamsFound, BufferSize, &NumberOfGenericParams);

    *NumberOfGenericParamsFound = NumberOfGenericParams;

    return true;
}

bool
RefersToGenericParameter
(
    Type *PossiblyGenericType,
    Declaration *Generic,
    GenericParameter *GenericParam,
    _Out_opt_cap_(ListSize) BCSYM_GenericParam **ListOfGenericParamsFound,
    unsigned ListSize,
    _Inout_ unsigned *NumberOfGenericParamsFound,
    bool IgnoreMethodTypeParams,
    IReadonlyBitVector * pFixedParameterBitVector
)
{
    VSASSERT(!ListOfGenericParamsFound || NumberOfGenericParamsFound, "Inconsistency in generic params buffer args!!!");

    if (PossiblyGenericType == NULL)
    {
        return false;
    }

    if (TypeHelpers::IsGenericParameter(PossiblyGenericType))
    {
        if (IgnoreMethodTypeParams && PossiblyGenericType->PGenericParam()->IsGenericMethodParam())
        {
            return false;
        }

        bool MatchingGenericParamFound =
            GenericParam ?
                (PossiblyGenericType->PGenericParam() == GenericParam) :
                (
                    (
                        Generic == NULL ||
                        PossiblyGenericType->PGenericParam()->GetParent() == Generic
                    ) &&
                    (
                        !pFixedParameterBitVector ||
                        !pFixedParameterBitVector->BitValue(PossiblyGenericType->PGenericParam()->GetPosition())
                    )
                );

        if (MatchingGenericParamFound && NumberOfGenericParamsFound)
        {
            if (ListOfGenericParamsFound && *NumberOfGenericParamsFound < ListSize)
            {
                ListOfGenericParamsFound[*NumberOfGenericParamsFound] = PossiblyGenericType->PGenericParam();
            }

            *NumberOfGenericParamsFound = *NumberOfGenericParamsFound + 1;
        }

        return MatchingGenericParamFound;
    }

    if (TypeHelpers::IsGenericTypeBinding(PossiblyGenericType))
    {
        bool MatchingGenericParamFound = false;

        GenericTypeBinding *Binding = PossiblyGenericType->PGenericTypeBinding();
        unsigned ArgumentCount = Binding->GetArgumentCount();

        for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
        {
            if
            (
                RefersToGenericParameter
                (
                    Binding->GetArgument(ArgumentIndex),
                    Generic,
                    GenericParam,
                    ListOfGenericParamsFound,
                    ListSize,
                    NumberOfGenericParamsFound,
                    IgnoreMethodTypeParams,
                    pFixedParameterBitVector
                )
            )
            {
                // Continue on if a count of generic params found is required
                //
                if (NumberOfGenericParamsFound)
                {
                    MatchingGenericParamFound = true;
                }
                else
                {
                    return true;
                }
            }
        }

        if
        (
            Binding->GetParentBinding() &&
            RefersToGenericParameter
            (
                Binding->GetParentBinding(),
                Generic,
                GenericParam,
                ListOfGenericParamsFound,
                ListSize,
                NumberOfGenericParamsFound,
                IgnoreMethodTypeParams,
                NULL
            )
        )
        {
            MatchingGenericParamFound = true;
        }

        return MatchingGenericParamFound;
    }

    if (TypeHelpers::IsPointerType(PossiblyGenericType))
    {
        return
            RefersToGenericParameter
            (
                TypeHelpers::GetReferencedType(PossiblyGenericType->PPointerType()),
                Generic,
                GenericParam,
                ListOfGenericParamsFound,
                ListSize,
                NumberOfGenericParamsFound,
                IgnoreMethodTypeParams,
                pFixedParameterBitVector
            );
    }

    if (TypeHelpers::IsArrayType(PossiblyGenericType))
    {
        return
            RefersToGenericParameter
            (
                TypeHelpers::GetElementType(PossiblyGenericType->PArrayType()),
                Generic,
                GenericParam,
                ListOfGenericParamsFound,
                ListSize,
                NumberOfGenericParamsFound,
                IgnoreMethodTypeParams,
                pFixedParameterBitVector
            );
    }

    return false;
}

bool
RefersToGenericParameter
(
    Type *PossiblyGenericType,
    Declaration *Generic,
    IReadonlyBitVector * pFixedParameterBitVector
)
{
    return
        RefersToGenericParameter
        (
            PossiblyGenericType,
            Generic,
            NULL,
            NULL,
            0,
            NULL,
            false,
            pFixedParameterBitVector
        );

}


bool
IsGenericOrHasGenericParent
(
    Declaration *Decl
)
{
    if (Decl == NULL)
    {
        return false;
    }

    if (Decl->IsGeneric())
    {
        return true;
    }

    return Decl->HasGenericParent();
}

Type *
ReplaceGenericParametersWithArguments
(
    Type *PossiblyGenericType,
    GenericBindingInfo GenericBindingContext,
    _In_ Symbols &SymbolCreator
)
{
    if (GenericBindingContext.IsNull() || !RefersToGenericParameter(PossiblyGenericType))
    {
        return PossiblyGenericType;
    }

    if (TypeHelpers::IsGenericParameter(PossiblyGenericType))
    {
        Type *Argument = GenericBindingContext.GetCorrespondingArgument(PossiblyGenericType->PGenericParam());

        return Argument ? Argument : PossiblyGenericType;
    }

    if (TypeHelpers::IsGenericTypeBinding(PossiblyGenericType))
    {
        GenericTypeBinding *Binding = PossiblyGenericType->PGenericTypeBinding();
        unsigned ArgumentCount = Binding->GetArgumentCount();

        Type **NewArguments;
        StackAllocTypeArgumentsIfPossible(NewArguments, ArgumentCount, SymbolCreator.GetNorlsAllocator(), SymbolCreator);

        if (ArgumentCount > 0)
        {
            for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                NewArguments[ArgumentIndex] = ReplaceGenericParametersWithArguments(Binding->GetArgument(ArgumentIndex), GenericBindingContext, SymbolCreator);
            }
        }
        if (PossiblyGenericType->HasLocation())
        {
                SymbolCreator.GetGenericBindingWithLocation(
                    Binding->IsBadGenericBinding(),
                    Binding->GetGenericType(),
                    NewArguments,
                    ArgumentCount,
                    Binding->GetParentBinding() ?
                        ReplaceGenericParametersWithArguments(Binding->GetParentBinding(), GenericBindingContext, SymbolCreator)->PGenericTypeBinding():
                        NULL,
                    PossiblyGenericType->GetLocation(),
                    NewArgumentsStackAllocated);  // if stack allocated type arguments, need to copy over when creating the binding
        }
        else
        {
            return
                SymbolCreator.GetGenericBinding(
                    Binding->IsBadGenericBinding(),
                    Binding->GetGenericType(),
                    NewArguments,
                    ArgumentCount,
                    Binding->GetParentBinding() ?
                        ReplaceGenericParametersWithArguments(Binding->GetParentBinding(), GenericBindingContext, SymbolCreator)->PGenericTypeBinding():
                        NULL,
                    NewArgumentsStackAllocated);  // if stack allocated type arguments, need to copy over when creating the binding
        }
    }

    if (TypeHelpers::IsPointerType(PossiblyGenericType))
    {
        return SymbolCreator.MakePtrType(ReplaceGenericParametersWithArguments(TypeHelpers::GetReferencedType(PossiblyGenericType->PPointerType()), GenericBindingContext, SymbolCreator));
    }
    if (TypeHelpers::IsArrayType(PossiblyGenericType))
    {
        ArrayType *Array = PossiblyGenericType->PArrayType();
            return
                SymbolCreator.GetArrayType(
                    Array->GetRank(),
                ReplaceGenericParametersWithArguments(TypeHelpers::GetElementType(Array), GenericBindingContext, SymbolCreator));
        }

    return PossiblyGenericType;
}

bool
InferTypeArgumentsFromArgument
(
    const Location *ArgumentLocation,
    Type *ArgumentType,
    bool ArgumentTypeByAssumption,
    Type *ParameterType,
    Parameter *Param,
    _In_opt_ Semantics::TypeInference* TypeInference,
    _Inout_ Type **TypeInferenceArguments,
    _Out_opt_ bool *TypeInferenceArgumentsByAssumption,   // can be NULL
    _Out_opt_ Location *InferredTypeArgumentLocations,    // can be NULL
    _Out_opt_ Parameter **ParamsInferredFrom,                 // can be NULL
    _Out_opt_ GenericParameter **InferredTypeParameter,     // can be NULL
    _Out_opt_ Type **InferredTypeArgument,                // can be NULL
    Procedure *TargetProcedure,
    Compiler *CompilerInstance,
    CompilerHost *CompilerHostInstance,
    Symbols &SymbolCreator,
    MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
    ConversionRequiredEnum InferenceRestrictions,
    IReadonlyBitVector * pFixedParameterBitVector
)
{
    // This routine is given an argument e.g. "List(Of IEnumerable(Of Int))",
    // and a parameter e.g. "IEnumerable(Of IEnumerable(Of T))".
    // The task is to infer hints for T, e.g. "T=int".
    // This function takes care of allowing (in this example) List(Of _) to match IEnumerable(Of _).
    // As for the real work, i.e. matching the contents, we invoke "InferTypeArgumentsFromArgumentDirectly"
    // to do that.

    // Note: this function returns "false" if it failed to pattern-match between argument and parameter type,
    // and "true" if it succeeded.
    // Success in pattern-matching may or may not produce type-hints for generic parameters.
    // If it happened not to produce any type-hints, then maybe other argument/parameter pairs will have produced
    // their own type hints that allow inference to succeed, or maybe no one else will have produced type hints,
    // or maybe other people will have produced conflicting type hints. In those cases, we'd return True from
    // here (to show success at pattern-matching) and leave the downstream code to produce an error message about
    // failing to infer T.
    

    // First try to the things directly. Only if this fails will we bother searching for things like List->IEnumerable.

    bool Inferred =
        InferTypeArgumentsFromArgumentDirectly
        (
            ArgumentLocation,
            ArgumentType,
            ArgumentTypeByAssumption,
            ParameterType,
            Param,
            TypeInference,
            TypeInferenceArguments,
            TypeInferenceArgumentsByAssumption,
            InferredTypeArgumentLocations,
            ParamsInferredFrom,
            InferredTypeParameter,
            InferredTypeArgument,
            TargetProcedure,
            CompilerInstance,
            CompilerHostInstance,
            SymbolCreator,
            DigThroughToBasesAndImplements,
            InferenceRestrictions,
            pFixedParameterBitVector
        );

    if (Inferred)
    {
        return Inferred;
    }

    if (ParameterType->IsGenericParam())
    {
        // If we failed to match an argument against a generic parameter T, it means that the
        // argument was something unmatchable, e.g. an AddressOf.
        return false;
    }

    
    // If we didn't find a direct match, we will have to look in base clases for a match.
    // We'll either fix ParameterType and look amongst the bases of ArgumentType,
    // or we'll fix ArgumentType and look amongst the bases of ParameterType,
    // depending on the "DigThroughToBasesAndImplements" flag. This flag is affected by
    // covariance and contravariance...

    if (DigThroughToBasesAndImplements == MatchGenericArgumentToParameterExactly)
    {
        return false;
    }


    if (ArgumentType->IsAnonymousDelegate() && ParameterType->IsDelegate() && !ParameterType->IsAnonymousDelegate() &&
        DigThroughToBasesAndImplements == MatchBaseOfGenericArgumentToParameter &&
        !TypeHelpers::EquivalentTypes(ParameterType, CompilerHostInstance->GetFXSymbolProvider()->GetDelegateType()) &&
        !TypeHelpers::EquivalentTypes(ParameterType, CompilerHostInstance->GetFXSymbolProvider()->GetMultiCastDelegateType()) &&
        (InferenceRestrictions == ConversionRequired::Any || InferenceRestrictions == ConversionRequired::AnyReverse ||
         InferenceRestrictions == ConversionRequired::AnyAndReverse))
    {
        // Some trickery relating to the fact that anonymous delegates can be converted to any delegate type.
        // We are trying to match the anonymous delegate "BaseSearchType" onto the delegate "FixedType". e.g.
        // Dim f = function(i as integer) i   // ArgumentType = VB$AnonymousDelegate`2(Of Integer,Integer)
        // inf(f)                             // ParameterType might be e.g. D(Of T) for some function inf(Of T)(f as D(Of T))
        //                                    // maybe defined as Delegate Function D(Of T)(x as T) as T.
        // We're looking to achieve the same functionality in pattern-matching these types as we already
        // have for calling "inf(function(i as integer) i)" directly. 
        // It allows any VB conversion from param-of-fixed-type to param-of-base-type (not just reference conversions).
        // But it does allow a zero-argument BaseSearchType to be used for a FixedType with more.
        // And it does allow a function BaseSearchType to be used for a sub FixedType.
        //
        // Anyway, the plan is to match each of the parameters in the ArgumentType delegate
        // to the equivalent parameters in the ParameterType delegate, and also match the return types.
        //
        // This only works for "ConversionRequired::Any", i.e. using VB conversion semantics. It doesn't work for
        // reference conversions. As for the AnyReverse/AnyAndReverse, well, in Orcas that was guaranteed
        // to fail type inference (i.e. return a false from this function). In Dev10 we will let it succeed
        // with some inferred types, for the sake of better error messages, even though we know that ultimately
        // it will fail (because no non-anonymous delegate type can be converted to a delegate type).

        BCSYM_GenericBinding *BindingForArgumentInvoke = TypeHelpers::IsGenericTypeBinding(ArgumentType) ? ArgumentType->PGenericTypeBinding() : NULL;
        BCSYM_GenericBinding *BindingForParameterInvoke= TypeHelpers::IsGenericTypeBinding(ParameterType)? ParameterType->PGenericTypeBinding(): NULL;
        //
        Declaration *ArgumentInvokeDeclaration  = GetInvokeFromDelegate(ArgumentType, CompilerInstance);
        Declaration *ParameterInvokeDeclaration = GetInvokeFromDelegate(ParameterType, CompilerInstance);
        if (ArgumentInvokeDeclaration == NULL || IsBad(ArgumentInvokeDeclaration) || !ArgumentInvokeDeclaration->IsProc() ||
            ParameterInvokeDeclaration== NULL || IsBad(ParameterInvokeDeclaration)|| !ParameterInvokeDeclaration->IsProc())
        {
            return false;
        }
        BCSYM_Proc *ArgumentInvokeProc = ArgumentInvokeDeclaration->PProc();
        BCSYM_Proc *ParameterInvokeProc = ParameterInvokeDeclaration->PProc();

        // First we'll check that the argument types all match.
        BCSYM_Param *ArgumentParam  = ArgumentInvokeProc->GetFirstParam();
        BCSYM_Param *ParameterParam = ParameterInvokeProc->GetFirstParam();
        while (ArgumentParam != NULL && ParameterParam != NULL)
        {
            Type *ArgumentParamType = GetDataType(ArgumentParam);
            ArgumentParamType = ReplaceGenericParametersWithArguments(ArgumentParamType, BindingForArgumentInvoke, SymbolCreator);
            bool ArgumentParamIsByReference = false;
            if (ArgumentParamType!=NULL && TypeHelpers::IsPointerType(ArgumentParamType))
            {
                ArgumentParamIsByReference = true;
                ArgumentParamType = TypeHelpers::GetReferencedType(ArgumentParamType->PPointerType());
            }
            //
            Type *ParameterParamType = GetDataType(ParameterParam);
            ParameterParamType = ReplaceGenericParametersWithArguments(ParameterParamType, BindingForParameterInvoke, SymbolCreator);
            bool ParameterParamIsByReference = false;
            if (ParameterParamType!=NULL && TypeHelpers::IsPointerType(ParameterParamType))
            {
                ParameterParamIsByReference = true;
                ParameterParamType = TypeHelpers::GetReferencedType(ParameterParamType->PPointerType());
            }

            if (ParameterParamIsByReference != ArgumentParamIsByReference)
            {
                // Require an exact match between ByRef/ByVal, since that's how type inference of lambda expressions works.
                return false;
            }

            bool MatchedParameter = InferTypeArgumentsFromArgument(
                                        ArgumentLocation,
                                        ArgumentParamType,
                                        ArgumentTypeByAssumption,
                                        ParameterParamType,
                                        Param,
                                        TypeInference,
                                        TypeInferenceArguments,
                                        TypeInferenceArgumentsByAssumption,
                                        InferredTypeArgumentLocations,
                                        ParamsInferredFrom,
                                        InferredTypeParameter,
                                        InferredTypeArgument,
                                        TargetProcedure,
                                        CompilerInstance,
                                        CompilerHostInstance,
                                        SymbolCreator,
                                        MatchArgumentToBaseOfGenericParameter,
                                        ConversionRequired::AnyReverse, // AnyReverse: contravariance in delegate arguments
                                        pFixedParameterBitVector);
            if (!MatchedParameter)
            {
                return false;
            }
            //
            if (ArgumentParam) ArgumentParam = ArgumentParam->GetNext();
            if (ParameterParam) ParameterParam = ParameterParam->GetNext();
        }
        //
        if ((ParameterParam!=NULL && ArgumentInvokeProc->GetFirstParam()!=NULL) || ArgumentParam!=NULL)
        {
            // If parameter-counts are mismatched then it's a failure.
            // Exception: Zero-argument relaxation: we allow a parameterless VB$AnonymousDelegate argument
            // to be supplied to a function which expects a parameterfull delegate.
            return false;
        }

        // Now check that the return type matches.
        // Note: we allow a *function* VB$AnonymousDelegate to be supplied to a function which expects a *sub* delegate.
        Type *ArgumentReturnType = ArgumentInvokeProc->GetType();
        ArgumentReturnType = (ArgumentReturnType == NULL) ? NULL : ReplaceGenericParametersWithArguments(ArgumentReturnType, BindingForArgumentInvoke, SymbolCreator);
        if (ArgumentReturnType!=NULL && TypeHelpers::IsPointerType(ArgumentReturnType))
        {
            ArgumentReturnType = TypeHelpers::GetReferencedType(ArgumentReturnType->PPointerType());
        }
        Type *ParameterReturnType = ParameterInvokeProc->GetType();
        ParameterReturnType = (ParameterReturnType == NULL) ? NULL : ReplaceGenericParametersWithArguments(ParameterReturnType, BindingForParameterInvoke, SymbolCreator);
        if (ParameterReturnType!=NULL && TypeHelpers::IsPointerType(ParameterReturnType))
        {
            ParameterReturnType = TypeHelpers::GetReferencedType(ParameterReturnType->PPointerType());
        }

        if (ParameterReturnType == NULL)
        {
            // A *sub* delegate parameter can accept either a *function* or a *sub* argument:
            return true;
        }
        else if (ArgumentReturnType == NULL)
        {
            // A *function* delegate parameter cannot accept a *sub* argument.
            return false;
        }
        else
        {
            // Otherwise, a function argument VB$AnonymousDelegate was supplied to a function parameter:
            bool MatchedReturn = InferTypeArgumentsFromArgument(
                                        ArgumentLocation,
                                        ArgumentReturnType,
                                        ArgumentTypeByAssumption,
                                        ParameterReturnType,
                                        Param,
                                        TypeInference,
                                        TypeInferenceArguments,
                                        TypeInferenceArgumentsByAssumption,
                                        InferredTypeArgumentLocations,
                                        ParamsInferredFrom,
                                        InferredTypeParameter,
                                        InferredTypeArgument,
                                        TargetProcedure,
                                        CompilerInstance,
                                        CompilerHostInstance,
                                        SymbolCreator,
                                        MatchBaseOfGenericArgumentToParameter,
                                        ConversionRequired::Any, // Any: covariance in delegate returns
                                        pFixedParameterBitVector);
            return MatchedReturn;
        }
    }



    Type* &BaseSearchType = (DigThroughToBasesAndImplements == MatchBaseOfGenericArgumentToParameter) ? ArgumentType : ParameterType;
    Type* &FixedType = (DigThroughToBasesAndImplements == MatchBaseOfGenericArgumentToParameter) ? ParameterType : ArgumentType;
    // MatchBaseOfGenericArgumentToParameter: used for covariant situations,
    // e.g. matching argument "List(Of _)" to parameter "ByVal x as IEnumerable(Of _)".
    //
    // Otherwise, MatchArgumentToBaseOfGenericParameter, used for contravariant situations,
    // e.g. when matching argument "Action(Of IEnumerable(Of _))" to parameter "ByVal x as Action(Of List(Of _))".

    if (!FixedType->IsGenericTypeBinding())
    {
        // If the fixed candidate isn't a generic (e.g. matching argument IList(Of String) to non-generic parameter IList),
        // then we won't learn anything about generic type parameters here:
        return false;
    }

    if (!IsClassOnly(FixedType->PNamedRoot()) && !FixedType->IsInterface())
    {
        // Whatever "BaseSearchType" is, it can only inherit from "FixedType" if FixedType is a class/interface.
        // (it's impossible to inherit from anything else, apart from the delegate case dealt with above).
        return false;
    }

    if (!BaseSearchType->IsClass() && !BaseSearchType->IsInterface() && !BaseSearchType->IsGenericParam() &&
        !(TypeHelpers::IsArrayType(BaseSearchType) && BaseSearchType->PArrayType()->GetRank()==1))
    {
        // The things listed above are the only ones that have bases that could ever lead anywhere useful.
        // Confusingly, "IsClass" is satisfied by structures, enums, delegates and modules as well as just classes.
        return false;
    }

    if (BaseSearchType->IsNamedRoot() && FixedType->IsNamedRoot() &&
        BaseSearchType->PNamedRoot() == FixedType->PNamedRoot())
    {
        // If the types checked were already identical, then exit
        return false;
    }



    // Otherwise, if we got through all the above tests, then it really is worth searching through the base
    // types to see if that helps us find a match. But first, some trickery, due to the fact that arrays
    // implement IList(Of T) but this fact isn't stored explicitly in the compiler's data-structures
    // and isn't picked up by the routine "IsOrInheritsFromOrImplements"...
    DynamicArray<GenericTypeBinding *> MatchingGenericBindings;

    if (TypeHelpers::IsArrayType(BaseSearchType) &&
        CompilerHostInstance->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIListType))
    {
        // Pretend that the array implements IList and try to infer the type.
        if (!InferBaseSearchTypeFromArray(BaseSearchType, FixedType, CompilerHostInstance, MatchingGenericBindings, SymbolCreator, FX::GenericIListType))
        {
            // If that didn't work pretend that the array implements IReadOnlyList (which is true since .NET 4.5)
            if (CompilerHostInstance->GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIReadOnlyListType))
            {
                InferBaseSearchTypeFromArray(BaseSearchType, FixedType, CompilerHostInstance, MatchingGenericBindings, SymbolCreator, FX::GenericIReadOnlyListType);
            }
        }
    }
    else
    {
        // Here's the code to look through base types looking for a match:

        TypeHelpers::IsOrInheritsFromOrImplements(
            BaseSearchType,
            FixedType->PGenericTypeBinding()->GetGeneric(),
            SymbolCreator,
            false,
            &MatchingGenericBindings,
            CompilerHostInstance);
    }

    if (MatchingGenericBindings.Count() == 0 || !TypeHelpers::EquivalentTypeBindings(MatchingGenericBindings))
    {
        return false;
    }

    // And this is what we found for BaseSearchType:
    BaseSearchType = MatchingGenericBindings.Element(0);

    // NOTE: BaseSearchType was a REFERENCE, to either ArgumentType or ParameterType.
    // Therefore the above statement has altered either ArgumentType or ParameterType.

    return
        InferTypeArgumentsFromArgumentDirectly
        (
            ArgumentLocation,
            ArgumentType,
            ArgumentTypeByAssumption,
            ParameterType,
            Param,
            TypeInference,
            TypeInferenceArguments,
            TypeInferenceArgumentsByAssumption,
            InferredTypeArgumentLocations,
            ParamsInferredFrom,
            InferredTypeParameter,
            InferredTypeArgument,
            TargetProcedure,
            CompilerInstance,
            CompilerHostInstance,
            SymbolCreator,
            DigThroughToBasesAndImplements,
            InferenceRestrictions,
            pFixedParameterBitVector
        );
}

bool
InferBaseSearchTypeFromArray
(
    _In_    Type* BaseSearchType,
    _In_    Type* FixedType,
    _In_    CompilerHost *CompilerHostInstance,
    _Inout_ DynamicArray<GenericTypeBinding *> &MatchingGenericBindings,
    Symbols &SymbolCreator,
    FX::TypeName InterfaceName
)
{
    // A rank-1 array T() implements the interfaces IList(Of T), ICollection(Of T), IEnumerable(Of T), IReadOnlyList(Of T), IReadOnlyCollection(Of T)
    // IList, ICollection, IEnumerable, and also the classes System.Array and System.Object.
    // For purposes of type-inference, only the first five are generic, so we need only consider them.
    // And IList(Of T) inherits from ICollection(Of T) and IEnumerable(Of T), and IReadOnlyList(Of T) inherits from IReadOnlyCollection(Of T), so actually it's
    // enough just to pretend BaseSearchType was an IList(Of T) or IReadOnlyList(Of T) instead of T(), and then proceed with
    // normal type inference.
    VSASSERT(BaseSearchType->PArrayType()->GetRank()==1, "internal logic error: we already ensured that rank==1 above");

    Type *ArrayElementType = TypeHelpers::GetElementType(BaseSearchType->PArrayType());
    BaseSearchType =
                SymbolCreator.GetGenericBinding(
                    false,
                    CompilerHostInstance->GetFXSymbolProvider()->GetType(InterfaceName),
                    &ArrayElementType,
                    1,
                    NULL,
                    true);

    TypeHelpers::IsOrInheritsFromOrImplements(
            BaseSearchType,
            FixedType->PGenericTypeBinding()->GetGeneric(),
            SymbolCreator,
            false,
            &MatchingGenericBindings,
            CompilerHostInstance);

    if (MatchingGenericBindings.Count() == 0 || !TypeHelpers::EquivalentTypeBindings(MatchingGenericBindings))
    {
        return false;
    }

    return true;
}

bool
InferTypeArgumentsFromArgumentDirectly
(
    const Location *ArgumentLocation,
    Type *ArgumentType,
    bool ArgumentTypeByAssumption,
    Type *ParameterType,
    Parameter *Param,
    _In_opt_ Semantics::TypeInference* TypeInference,
    _Inout_ Type **TypeInferenceArguments,
    _Out_opt_ bool *TypeInferenceArgumentsByAssumption,   // can be NULL
    _Out_opt_ Location *InferredTypeArgumentLocations,    // can be NULL
    _Out_ Parameter **ParamsInferredFrom,                 // can be NULL
    _Out_opt_ GenericParameter **InferredTypeParameter,     // can be NULL
    _Out_opt_ Type **InferredTypeArgument,                // can be NULL
    Procedure *TargetProcedure,
    Compiler *CompilerInstance,
    CompilerHost *CompilerHostInstance,
    Symbols &SymbolCreator,
    MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements,
    ConversionRequiredEnum InferenceRestrictions,
    IReadonlyBitVector * pFixedParameterBitVector
)
{
    GenericParameter *GenericParamFound = NULL;
    unsigned NumberOfGenericParamsFound = 0;


    // Port SP1 CL 2941446 to VS10
    if (ArgumentType && (ArgumentType->IsVoidType() || TypeHelpers::IsVoidArrayLiteralType(ArgumentType)))
    {
        // We should never be able to infer a value from something that doesn't provide a value, e.g:
        // Foo(Of T) can't be passed Sub bar(), as in Foo(Bar())  
        return false;
    }

    if
    (
        !RefersToGenericParameter
        (
            ParameterType,
            TargetProcedure,
            NULL,
            InferredTypeParameter,
            InferredTypeParameter ? 1 : 0,
            InferredTypeParameter ? &NumberOfGenericParamsFound : NULL,
            false,
            pFixedParameterBitVector
        )
    )
    {
        return true;
    }

    // Port SP1 CL 2922610 to VS10
    // unwrap argument type if it is a pointer type. 
    // This could happen when lambdas are passed to generic functions
    if (ArgumentType && TypeHelpers::IsPointerType(ArgumentType))
    {
        ArgumentType = TypeHelpers::GetReferencedType(ArgumentType->PPointerType());
    }

    // If a generic method is parameterized by T, an argument of type A matching a parameter of type
    // P can be used to infer a type for T by these patterns:
    //
    //   -- If P is T, then infer A for T
    //   -- If P is G(Of T) and A is G(Of X), then infer X for T
    //   -- If P is Array Of T, and A is Array Of X, then infer X for T
    //   -- If P is ByRef T, then infer A for T

    if (TypeHelpers::IsGenericParameter(ParameterType))
    {
        if (ParameterType->PGenericParam()->GetParent() == TargetProcedure)
        {
            if (TypeInference)
            {
                VSASSERT(!TypeInference->GetInferenceGraph()->IsVerifyingAssertions(),
                    "We are asserting our type inference. How can we have resolved a new typevalue? Only lambda's and delegates can create backedges and upon verification they should have been replaced with proper types and either failed to interpret or succeed, not find new generic type hints");

                TypeInference->GetInferenceGraph()->RegisterTypeParameterHint(
                    ParameterType->PGenericParam(),
                    ArgumentType,
                    ArgumentTypeByAssumption,
                    ArgumentLocation,
                    Param,
                    false,
                    InferenceRestrictions);
            }
            else
            {
                if (!Semantics::TypeInference::RegisterInferredType(
                    ParameterType->PGenericParam(),
                    ArgumentType,
                    ArgumentTypeByAssumption,
                    ArgumentLocation,
                    Param,

                    InferredTypeArgument,
                    InferredTypeParameter,

                    TypeInferenceArguments,
                    TypeInferenceArgumentsByAssumption,
                    InferredTypeArgumentLocations,
                    ParamsInferredFrom,
                    &SymbolCreator))
                {
                    return false;
                }
            }
        }
    }
    else if (TypeHelpers::IsGenericTypeBinding(ParameterType))  // e.g. handle foo(of T)(x as Bar(Of T)) We need to dig into Bar(Of T)
    {
        if (TypeHelpers::IsGenericTypeBinding(ArgumentType))
        {
            GenericTypeBinding *ParameterBinding = ParameterType->PGenericTypeBinding();
            GenericTypeBinding *ArgumentBinding = ArgumentType->PGenericTypeBinding();

            if (BCSYM::AreTypesEqual(ArgumentBinding->GetGeneric(),ParameterBinding->GetGeneric()))
            {
                do
                {
                    VSASSERT
                    (
                        BCSYM::AreTypesEqual
                        (
                            ArgumentBinding->GetGeneric(),
                            ParameterBinding->GetGeneric()
                        ),
                        "Inconsistent parent bindings for given generic bindings!!!"
                    );

                    VSASSERT(
                        ArgumentBinding->GetArgumentCount() == ParameterBinding->GetArgumentCount(),
                        "Inconsistent argument counts for bindings of a given generic.");

                    GenericParameter *gParam = ArgumentBinding->GetGeneric()->GetFirstGenericParam();
                    for (unsigned ArgumentIndex = 0;
                         ArgumentIndex < ArgumentBinding->GetArgumentCount();
                         ArgumentIndex++, gParam=gParam->GetNextParam())
                    {
                        VSASSERT(gParam!=NULL, "we had more arguments than generic parameters!");

                        // The following code is subtle. Let's recap what's going on...
                        // We've so far encountered some context, e.g. "_" or "ICovariant(_)"
                        // or "ByRef _" or the like. This context will have given us some TypeInferenceRestrictions.
                        // Now, inside the context, we've discovered a generic binding "G(Of _,_,_)"
                        // and we have to apply extra restrictions to each of those subcontexts.
                        // For non-variant parameters it's easy: the subcontexts just acquire the Identity constraint.
                        // For variant parameters it's more subtle. First, we have to strengthen the
                        // restrictions to require reference conversion (rather than just VB conversion or
                        // whatever it was). Second, if it was an In parameter, then we have to invert
                        // the sense.
                        //

                        // Processing of generics is tricky in the case that we've already encountered
                        // a "ByRef _". From that outer "ByRef _" we will have inferred the restriction
                        // "AnyConversionAndReverse", so that the argument could be copied into the parameter
                        // and back again. But now consider if we find a generic inside that ByRef, e.g.
                        // if it had been "ByRef x as G(Of T)" then what should we do? More specifically, consider a case
                        //    "Sub f(Of T)(ByRef x as G(Of T))"  invoked with some   "dim arg as G(Of Hint)".
                        // What's needed for any candidate for T is that G(Of Hint) be convertible to
                        // G(Of Candidate), and vice versa for the copyback.
                        // 
                        // But then what should we write down for the hints? The problem is that hints inhere
                        // to the generic parameter T, not to the function parameter G(Of T). So we opt for a
                        // safe approximation: we just require CLR identity between a candidate and the hint.
                        // This is safe but is a little overly-strict. For example:
                        //    Class G(Of T)
                        //       Public Shared Widening Operator CType(ByVal x As G(Of T)) As G(Of Animal)
                        //       Public Shared Widening Operator CType(ByVal x As G(Of Animal)) As G(Of T)
                        //    Sub inf(Of T)(ByRef x as G(Of T), ByVal y as T)
                        //    ...
                        //    inf(New G(Of Car), New Animal)
                        //    inf(Of Animal)(New G(Of Car), New Animal)
                        // Then the hints will be "T:{Car=, Animal+}" and they'll result in inference-failure,
                        // even though the explicitly-provided T=Animal ends up working.
                        // 
                        // Well, it's the best we can do without some major re-architecting of the way
                        // hints and type-inference works. That's because all our hints inhere to the
                        // type parameter T; in an ideal world, the ByRef hint would inhere to the parameter.
                        // But I don't think we'll ever do better than this, just because trying to do
                        // type inference inhering to arguments/parameters becomes exponential.
                        // Variance generic parameters will work the same.

                        // Dev10#595234: each Param'sInferenceRestriction is found as a modification of the surrounding InferenceRestriction:
                        ConversionRequiredEnum ParamInferenceRestrictions;
                        switch (gParam->GetVariance())
                        {
                            case Variance_None:
                                ParamInferenceRestrictions = ConversionRequired::Identity;
                                break;
                            case Variance_In:
                                ParamInferenceRestrictions = InvertConversionRequirement(
                                                            StrengthenConversionRequirementToReference(
                                                                InferenceRestrictions));
                                break;
                            case Variance_Out:
                                ParamInferenceRestrictions = StrengthenConversionRequirementToReference(InferenceRestrictions);
                                break;
                            default:
                                VSFAIL("Unexpected variance");
                                ParamInferenceRestrictions = ConversionRequired::Identity;
                        }

                        MatchGenericArgumentToParameterEnum DigThroughToBasesAndImplements;
                        //
                        if (ParamInferenceRestrictions == ConversionRequired::Reference)
                        {
                            DigThroughToBasesAndImplements = MatchBaseOfGenericArgumentToParameter;
                        }
                        else if (ParamInferenceRestrictions == ConversionRequired::ReverseReference)
                        {
                            DigThroughToBasesAndImplements = MatchArgumentToBaseOfGenericParameter;
                        }
                        else
                        {
                            DigThroughToBasesAndImplements = MatchGenericArgumentToParameterExactly;
                        }
                                                               

                        if
                        (
                            !InferTypeArgumentsFromArgument
                            (
                                ArgumentLocation,
                                ArgumentBinding->GetArgument(ArgumentIndex),
                                ArgumentTypeByAssumption,
                                ParameterBinding->GetArgument(ArgumentIndex),
                                Param,
                                TypeInference,
                                TypeInferenceArguments,
                                TypeInferenceArgumentsByAssumption,
                                InferredTypeArgumentLocations,
                                ParamsInferredFrom,
                                InferredTypeParameter,
                                InferredTypeArgument,
                                TargetProcedure,
                                CompilerInstance,
                                CompilerHostInstance,
                                SymbolCreator,
                                DigThroughToBasesAndImplements,
                                ParamInferenceRestrictions,
                                pFixedParameterBitVector
                          )
                        )
                        {
                            return false;
                        }
                    }
                }
                while ((ParameterBinding = ParameterBinding->GetParentBinding()) &&
                       (ArgumentBinding = ArgumentBinding->GetParentBinding()));

                VSASSERT(ParameterBinding == NULL &&
                         ArgumentBinding &&
                         ArgumentBinding->GetParentBinding() == NULL,
                            "Inconsistent parent bindings for given generic bindings!!!");

                return true;
            }
        }
        else if (TypeHelpers::IsNullableType(ParameterType, CompilerHostInstance))
        {
            // we reach here when the ParameterType is a generic type binding of Nullable,
            // and the argument type is NOT a generic type binding.

            return
                InferTypeArgumentsFromArgument
                (
                    ArgumentLocation,
                    ArgumentType,
                    ArgumentTypeByAssumption,
                    TypeHelpers::GetElementTypeOfNullable(ParameterType, CompilerHostInstance),
                    Param,
                    TypeInference,
                    TypeInferenceArguments,
                    TypeInferenceArgumentsByAssumption,
                    InferredTypeArgumentLocations,
                    ParamsInferredFrom,
                    InferredTypeParameter,
                    InferredTypeArgument,
                    TargetProcedure,
                    CompilerInstance,
                    CompilerHostInstance,
                    SymbolCreator,
                    DigThroughToBasesAndImplements,
                    CombineConversionRequirements(
                        InferenceRestrictions,
                        ConversionRequired::ArrayElement), // Microsoft: ??? what do array elements have to do with nullables?
                    pFixedParameterBitVector
                );
        }

        return false;
    }

    else if (TypeHelpers::IsArrayType(ParameterType))
    {
        if (TypeHelpers::IsArrayType(ArgumentType))
        {
            ArrayType *ParameterArray = ParameterType->PArrayType();
            ArrayType *ArgumentArray = ArgumentType->PArrayType();

            if (ParameterArray->GetRank() == ArgumentArray->GetRank())
            {
                return
                    InferTypeArgumentsFromArgument
                    (
                        ArgumentLocation,
                        TypeHelpers::GetElementType(ArgumentArray),
                        ArgumentTypeByAssumption,
                        TypeHelpers::GetElementType(ParameterArray),
                        Param,
                        TypeInference,
                        TypeInferenceArguments,
                        TypeInferenceArgumentsByAssumption,
                        InferredTypeArgumentLocations,
                        ParamsInferredFrom,
                        InferredTypeParameter,
                        InferredTypeArgument,
                        TargetProcedure,
                        CompilerInstance,
                        CompilerHostInstance,
                        SymbolCreator,
                        DigThroughToBasesAndImplements,
                        CombineConversionRequirements(
                            InferenceRestrictions,
                            ArgumentArray->IsArrayLiteralType() ? ConversionRequired::Any : ConversionRequired::ArrayElement),
                        pFixedParameterBitVector
                    );
            }
        }
        return false;
    }
    else if (TypeHelpers::IsPointerType(ParameterType))
    {
        // This occurs if the parameter is ByRef. Just chase through the pointer type.
        return
            InferTypeArgumentsFromArgument
            (
                ArgumentLocation,
                ArgumentType,
                ArgumentTypeByAssumption,
                TypeHelpers::GetReferencedType(ParameterType->PPointerType()),
                Param,
                TypeInference,
                TypeInferenceArguments,
                TypeInferenceArgumentsByAssumption,
                InferredTypeArgumentLocations,
                ParamsInferredFrom,
                InferredTypeParameter,
                InferredTypeArgument,
                TargetProcedure,
                CompilerInstance,
                CompilerHostInstance,
                SymbolCreator,
                DigThroughToBasesAndImplements,
                InferenceRestrictions,
                pFixedParameterBitVector
            );
    }
    return true;
}


ConversionRequiredEnum
CombineConversionRequirements
(
    ConversionRequiredEnum restriction1,
    ConversionRequiredEnum restriction2
)
{
    // See semantics.h / ConversionRequiredEnum for what the restrictions are,
    // and what their partial lattice looks like. This function returns the least upper bound
    // of two restrictions in that lattice. The code is messy, but it implements a straightforward thing.
    VSASSERT(ConversionRequired::Count==8, "If you've updated the type argument inference restrictions, then please also update CombineInferenceRestrictions()");
    //
    //    [reverse chain] [None] < AnyReverse < ReverseReference < Identity
    //    [middle  chain] None < [Any,AnyReverse] < AnyConversionAndReverse < Identity
    //    [forward chain] [None] < Any < ArrayElement < Reference < Identity

    // identical?
    if (restriction1 == restriction2)
    {
        return restriction1;
    }

    // none?
    if (restriction1 == ConversionRequired::None)
    {
        return restriction2;
    }
    else if (restriction2 == ConversionRequired::None)
    {
        return restriction1;
    }

    // forced to the top of the lattice?
    else if (restriction1 == ConversionRequired::Identity || restriction2 == ConversionRequired::Identity)
    {
        return ConversionRequired::Identity;
    }

    // within the reverse chain?
    else if ((restriction1 == ConversionRequired::AnyReverse || restriction1 == ConversionRequired::ReverseReference) &&
             (restriction2 == ConversionRequired::AnyReverse || restriction2 == ConversionRequired::ReverseReference))
    {
        return ConversionRequired::ReverseReference;
    }

    // within the middle chain?
    else if ((restriction1 == ConversionRequired::Any || restriction1 == ConversionRequired::AnyReverse || restriction1 == ConversionRequired::AnyAndReverse) &&
             (restriction2 == ConversionRequired::Any || restriction2 == ConversionRequired::AnyReverse || restriction2 == ConversionRequired::AnyAndReverse))
    {
        return ConversionRequired::AnyAndReverse;
    }

    // within the forward chain?
    else if ((restriction1 == ConversionRequired::Any || restriction1 == ConversionRequired::ArrayElement) &&
             (restriction2 == ConversionRequired::Any || restriction2 == ConversionRequired::ArrayElement))
    {
        return ConversionRequired::ArrayElement;
    }
    else if ((restriction1 == ConversionRequired::Any || restriction1 == ConversionRequired::ArrayElement || restriction1 == ConversionRequired::Reference) &&
             (restriction2 == ConversionRequired::Any || restriction2 == ConversionRequired::ArrayElement || restriction2 == ConversionRequired::Reference))
    {
        return  ConversionRequired::Reference;
    }

    // otherwise we've crossed chains
    else
    {
        return ConversionRequired::Identity;
    }
}


ConversionRequiredEnum
StrengthenConversionRequirementToReference
(
    ConversionRequiredEnum restriction
)
{
    // See semantics.h / ConversionRequiredEnum for what the restrictions are,
    // and what their partial lattice looks like. This function goes up the lattice to a reference
    // restriction.
    VSASSERT(ConversionRequired::Count==8, "If you've updated the type argument inference restrictions, then please also update StrengthenConversionRequirementToReference()");
    //
    //    [reverse chain] [None] < AnyReverse < ReverseReference < Identity
    //    [middle  chain] None < [Any,AnyReverse] < AnyConversionAndReverse < Identity
    //    [forward chain] [None] < Any < ArrayElement < Reference < Identity

    if (restriction == ConversionRequired::AnyReverse)
    {
        return ConversionRequired::ReverseReference;
    }
    else if (restriction == ConversionRequired::Any ||
             restriction == ConversionRequired::ArrayElement)
    {
        return ConversionRequired::Reference;
    }
    else if (restriction == ConversionRequired::AnyAndReverse)
    {
        return ConversionRequired::Identity;
    }
    else
    {
        return restriction;
    }
}


ConversionRequiredEnum
InvertConversionRequirement
(
    ConversionRequiredEnum restriction
)
{
    // See semantics.h / ConversionRequiredEnum for what the restrictions are,
    // and what their partial lattice looks like. This function switches to the inverse chain.
    VSASSERT(ConversionRequired::Count==8, "If you've updated the type argument inference restrictions, then please also update InvertConversionRequirement()");
    //
    //    [reverse chain] [None] < AnyReverse < ReverseReference < Identity
    //    [middle  chain] None < [Any,AnyReverse] < AnyConversionAndReverse < Identity
    //    [forward chain] [None] < Any < ArrayElement < Reference < Identity

    // from reverse chain to forward chain:
    if (restriction == ConversionRequired::AnyReverse)
    {
        return ConversionRequired::Any;
    }
    else if (restriction == ConversionRequired::ReverseReference)
    {
        return ConversionRequired::Reference;
    }

    // from forward chain to reverse chain:
    else if (restriction == ConversionRequired::Any)
    {
        return ConversionRequired::AnyReverse;
    }
    else if (restriction == ConversionRequired::ArrayElement)
    {
        VSFAIL("unexpected: ArrayElementConversion restriction has no reverse");
        return ConversionRequired::ReverseReference;
    }
    else if (restriction == ConversionRequired::Reference)
    {
        return ConversionRequired::ReverseReference;
    }
    
    // otherwise we're either in the middle chain, or identity
    else
    {
        return restriction;
    }

}



bool
Semantics::InferTypeArgumentsFromLambdaArgument
(
    ILTree::Expression *Argument,
    Type *ParameterType,
    Parameter *Param,_In_opt_
    TypeInference* TypeInference,
    _Inout_cap_(TypeArgumentCount) Type **TypeInferenceArguments,
    _Out_opt_cap_(TypeArgumentCount) bool *TypeInferenceArgumentsByAssumption,
    _Out_opt_cap_(TypeArgumentCount) Location *InferredTypeArgumentLocations,
    _Out_opt_ GenericParameter**  InferredTypeParameter, // Needed for error reporting
    unsigned TypeArgumentCount,
    Procedure *TargetProcedure,
    GenericTypeBinding *GenericBindingContext,
    _Out_opt_ AsyncSubAmbiguityFlags *pAsyncSubArgumentAmbiguity
)
{
    AssertIfFalse(Argument->bilop == SX_UNBOUND_LAMBDA || Argument->bilop == SX_LAMBDA);
    if(!(Argument->bilop == SX_UNBOUND_LAMBDA || Argument->bilop == SX_LAMBDA))
    {
        return true; // Should never get here
    }

    GenericParameter *GenericParamFound = NULL;
    Type * GenericExpressionType = NULL;

    if (TypeHelpers::IsGenericTypeBinding(ParameterType) &&
        (GenericExpressionType = GetFXSymbolProvider()->GetGenericExpressionType()) &&
        TypeHelpers::EquivalentTypes(ParameterType->PGenericTypeBinding()->GetGenericType(),
                        GenericExpressionType))
    {
        // If we've got an Expression(Of T), skip through to T
        return
            InferTypeArgumentsFromLambdaArgument(
                Argument,
                ParameterType->PGenericTypeBinding()->GetArgument(0),
                Param,
                TypeInference,
                TypeInferenceArguments,
                TypeInferenceArgumentsByAssumption,
                InferredTypeArgumentLocations,
                InferredTypeParameter,
                TypeArgumentCount,
                TargetProcedure,
                GenericBindingContext,
                pAsyncSubArgumentAmbiguity);
    }
    else if (TypeHelpers::IsGenericParameter(ParameterType))
    {
        Type* AnonymousLambdaType;

        if (Argument->bilop == SX_LAMBDA)
        {
            AnonymousLambdaType = GetInstantiatedAnonymousDelegate(
                &Argument->AsLambdaExpression()
                );
        }
        else if (Argument->bilop == SX_UNBOUND_LAMBDA)
        {
            AnonymousLambdaType = InferLambdaType(
                &Argument->AsUnboundLambdaExpression(),
                Argument->Loc);
        }

        if (AnonymousLambdaType != NULL)
        {
            // report the type of the lambda parameter to the delegate parameter.
            return InferTypeArgumentsFromArgument(
                &Argument->Loc,
                AnonymousLambdaType,
                false, // not ArgumentTypeByAssumption
                ParameterType,
                Param,
                TypeInference,
                TypeInferenceArguments,
                TypeInferenceArgumentsByAssumption,
                InferredTypeArgumentLocations,
                NULL,
                InferredTypeParameter,
                NULL,
                TargetProcedure,
                m_Compiler,
                m_CompilerHost,
                m_SymbolCreator,
                MatchBaseOfGenericArgumentToParameter,
                ConversionRequired::Any
                );
        }
        else
        {
            return false;
        }

    }
    else if (TypeHelpers::IsDelegateType(ParameterType))
    {
        // First, we need to build a partial binding context using the type
        // arguments as they stand right now, with some of them NULL.
        GenericBinding *PartialGenericBindingContext =
            m_SymbolCreator.GetGenericBinding(
                false,
                TargetProcedure,
                TypeInferenceArguments,
                TypeArgumentCount,
                GenericBindingContext,
                false);  // Don't worry about making copies off the stack -- this is a transient thing

        // Now we apply the partial binding to the delegate type, leaving uninferred type parameters
        // as unbound type parameters.
        Type *DelegateType =
            ReplaceGenericParametersWithArguments(ParameterType, PartialGenericBindingContext, m_SymbolCreator);

        // Now find the invoke method of the delegate
        Declaration *InvokeMethod = GetInvokeFromDelegate(DelegateType, m_Compiler);

        if (InvokeMethod == NULL || IsBad(InvokeMethod) || !InvokeMethod->IsProc())
        {
            // If we don't have an Invoke method, just bail.
            return true;
        }

        Type *ReturnType =
            ReplaceGenericParametersWithArguments(
                ViewAsProcedure(InvokeMethod)->GetType(),
                TypeHelpers::IsGenericTypeBinding(DelegateType) ? DelegateType->PGenericTypeBinding() : NULL,
                m_SymbolCreator);

        // If the return type doesn't refer to parameters, no inference required.
        if (!RefersToGenericParameter(
                ReturnType,
                TargetProcedure,
                NULL,
                NULL,
                0,
                NULL))
        {
            return true;
        }

        Parameter* firstLambdaParameter;
        if (Argument->bilop == SX_LAMBDA)
        {
            firstLambdaParameter = Argument->AsLambdaExpression().FirstParameter;
        }
        else if (Argument->bilop == SX_UNBOUND_LAMBDA)
        {
            firstLambdaParameter = Argument->AsUnboundLambdaExpression().FirstParameter;
        }

        Parameter* delegateParam;
        Parameter* lambdaParameter;
        for (delegateParam = ViewAsProcedure(InvokeMethod)->GetFirstParam(),
             lambdaParameter = firstLambdaParameter;

             delegateParam && lambdaParameter;

             delegateParam = delegateParam->GetNext(),
             lambdaParameter = lambdaParameter->GetNext())
        {
            Type *ParamType =
                ReplaceGenericParametersWithArguments(
                    delegateParam->GetType(),
                    DelegateType->PGenericBinding(),
                    m_SymbolCreator);

            if (!lambdaParameter->GetType())
            {
                // If a lambda parameter has no type and the delegate parameter refers
                // to an unbound generic parameter, we can't infer yet.
                if (RefersToGenericParameter(
                        ParamType,
                        TargetProcedure,
                        NULL,
                        NULL,
                        0,
                        NULL))
                {
                    // Skip this type argument and hope other parameters will infer it.
                    // if that doesn't happen it will report an error.
                    continue;
                }
            }
            else
            {
                // report the type of the lambda parameter to the delegate parameter.
                InferTypeArgumentsFromArgument(
                    &Argument->Loc,
                    lambdaParameter->GetType(),
                    false, // not ArgumentTypeByAssumption
                    ParamType,
                    Param,
                    TypeInference,
                    TypeInferenceArguments,
                    TypeInferenceArgumentsByAssumption,
                    InferredTypeArgumentLocations,
                    NULL,
                    InferredTypeParameter,
                    NULL,
                    TargetProcedure,
                    m_Compiler,
                    m_CompilerHost,
                    m_SymbolCreator,
                    MatchBaseOfGenericArgumentToParameter,
                    ConversionRequired::Any
                    );
            }
        }

        // OK, now we know that we can apply the lambda binding, so do it.
        ILTree::Expression *Result = NULL;

        if(Argument->bilop == SX_UNBOUND_LAMBDA)
        {
            // Port SP1 CL 2943055 to VS10
            // Bug #165844 - DevDiv Bugs        
            // It turns out that InterpretUnboundLambdaBinding() is destructive to the UnboundLambdaExpression node.
            // In particular parameter's type may be modified. Also, below we modify some fields in this structure.
            // So, let's clone the node here and operate on the copy instead. The xCopyBilTreeForScratch will clone 
            // parameters too. Instead, we could do a shallow copy here and clone parameters inside 
            // InterpretUnboundLambdaBinding. However, it seems like an overkill to clone parameters every time 
            // InterpretUnboundLambdaBinding is called because, usually, when we don't want the tree to be mutated,
            // InterpretUnboundLambdaBinding is operating on a copy already created by Overload Resolution. 
            // The interesting thing about type inference is that it may be called twice from MatchArguments (the second
            // time is to report errors) and the second call is going to operate on mutated Lambdas, unless we prevent mutation.
            ILTree::UnboundLambdaExpression* UnboundLambda = &m_TreeAllocator.xCopyBilTreeForScratch(Argument)->AsUnboundLambdaExpression();

            // Don't allow relaxation semantics, we want the pure type of the body.
            UnboundLambda->AllowRelaxationSemantics = false;

            // Don't try to infer the resulttype
            UnboundLambda->InterpretBodyFlags |= ExprInferResultTypeExplicit;

            DelegateRelaxationLevel RelaxationLevel = DelegateRelaxationLevelNone;
            Result = InterpretUnboundLambdaBinding(
                UnboundLambda,
                DelegateType,
                false,  // ConvertToDelegateReturnType
                RelaxationLevel,
                true, // Inferring Result Type
                NULL,
                NULL,
                false,
                false,
                pAsyncSubArgumentAmbiguity,
                NULL // pDroppedAsyncTaskReturn
                ); 
        }
        else
        {
            DelegateRelaxationLevel RelaxationLevel = DelegateRelaxationLevelNone;

            Result =
                ConvertToDelegateType
                (
                    &Argument->AsLambdaExpression(),
                    DelegateType,
                    false,
                    RelaxationLevel
                );
        }

        if (!Result || IsBad(Result))
        {
            // Intepretation failed, give up.
            return false;
        }

        ILTree::LambdaExpression *LambdaResult = &Result->AsLambdaExpression();

        if ((LambdaResult->IsStatementLambda &&
                (!LambdaResult->GetStatementLambdaBody() ||
                    !LambdaResult->GetStatementLambdaBody()->pReturnType ||
                    LambdaResult->GetStatementLambdaBody()->pReturnType->IsBad())
             ) ||
             (!LambdaResult->IsStatementLambda &&
                 (!LambdaResult->GetExpressionLambdaBody() ||
                    !LambdaResult->GetExpressionLambdaBody()->ResultType ||
                    LambdaResult->GetExpressionLambdaBody()->ResultType->IsBad())))
        {
            return true;
        }


        // Normally when we pass a lambda "Function() 1" to a parameter "Func(Of T)", then
        // the return type of the lambda "1" is matched against the return type of the parameter "T"...
        Type *pInferenceLambdaReturnType = LambdaResult->IsStatementLambda
                                           ? LambdaResult->GetStatementLambdaBody()->pReturnType
                                           : LambdaResult->GetExpressionLambdaReturnType();
        Type *pInferenceParameterReturnType = ReturnType;

        // But in the case of async/iterator lambdas, e.g. pass "Async Function() 1" 
        // to a parameter of type "Func(Of Task(Of T))" then we have to dig in further
        // and match 1 to T...
        if (LambdaResult->IsAsyncKeywordUsed || LambdaResult->IsIteratorKeywordUsed)
        {
            // By this stage we know that
            // * we have an async/iterator lambda argument
            // * the parameter-to-match is a delegate type whose result type refers to generic parameters
            // The parameter might be a delegate with result type e.g. "Task(Of T)" in which case we have
            // to dig in. Or it might be a delegate with result type "T" in which case we
            // don't dig in.
            if (pInferenceLambdaReturnType->IsGenericTypeBinding() &&
                pInferenceParameterReturnType->IsGenericTypeBinding() &&
                TypeHelpers::EquivalentTypes(pInferenceLambdaReturnType->PGenericTypeBinding()->GetGeneric(),
                                             pInferenceParameterReturnType->PGenericTypeBinding()->GetGeneric()) &&
                pInferenceLambdaReturnType->PGenericTypeBinding()->GetArgumentCount()==1)
            {
                // We can assume that the lambda will have return type Task(Of T) or IEnumerable(Of T)
                // or IEnumerator(Of T) as appropriate. That's already been established by the lambda-interpretation.

                pInferenceLambdaReturnType = pInferenceLambdaReturnType->PGenericTypeBinding()->GetArgument(0);
                pInferenceParameterReturnType = pInferenceParameterReturnType->PGenericTypeBinding()->GetArgument(0);
            }

        }

        // Now infer from the result type
        return
            InferTypeArgumentsFromArgument(
                &Argument->Loc,
                pInferenceLambdaReturnType,
                false, // not ArgumentTypeByAssumption ??? Microsoft: but maybe it should...
                pInferenceParameterReturnType,
                Param,
                TypeInference,
                TypeInferenceArguments,
                TypeInferenceArgumentsByAssumption,
                InferredTypeArgumentLocations,
                NULL,
                InferredTypeParameter,
                NULL,
                TargetProcedure,
                m_Compiler,
                m_CompilerHost,
                m_SymbolCreator,
                MatchBaseOfGenericArgumentToParameter,
                ConversionRequired::Any
                );
    }
    else if (TypeHelpers::IsPointerType(ParameterType))
    {
        // This occurs if the parameter is ByRef. Just chase through the pointer type.
        return
            InferTypeArgumentsFromLambdaArgument(
                Argument,
                TypeHelpers::GetReferencedType(ParameterType->PPointerType()),
                Param,
                TypeInference,
                TypeInferenceArguments,
                TypeInferenceArgumentsByAssumption,
                InferredTypeArgumentLocations,
                InferredTypeParameter,
                TypeArgumentCount,
                TargetProcedure,
                GenericBindingContext,
                pAsyncSubArgumentAmbiguity);
    }

    return true;
}

bool
Semantics::InferTypeArguments_Unused
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    ILTree::Expression **BoundArguments,
    ILTree::Expression *FirstParamArrayArgument,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags,
    _Inout_ GenericBinding *& GenericBindingContext,
    _Out_ Location *&InferredTypeArgumentLocations,_Out_
    TypeInferenceLevel &TypeInferenceLevel,
    _Out_ bool &AllFailedInferenceIsDueToObject,
    bool ignoreFirstParameter,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethod
)
{
    VSFAIL("Unexpected: I thought that this function was not invoked by anyone!");

    GenericBindingInfo binding = GenericBindingContext;

    bool ret =
        InferTypeArguments
        (
            CallLocation,
            TargetProcedure,
            BoundArguments,
            FirstParamArrayArgument,
            DelegateReturnType,
            OvrldFlags,
            binding,
            InferredTypeArgumentLocations,
            TypeInferenceLevel,
            AllFailedInferenceIsDueToObject,
            ignoreFirstParameter,
            SuppressMethodNameInErrorMessages,
            CandidateIsExtensionMethod
        );

    GenericBindingContext = binding.PGenericBinding();

    return ret;
}

bool
Semantics::InferTypeArguments
(
    const Location &CallLocation,
    Procedure *TargetProcedure,
    ILTree::Expression **BoundArguments,
    ILTree::Expression *FirstParamArrayArgument,
    Type *DelegateReturnType,
    OverloadResolutionFlags OvrldFlags,
    GenericBindingInfo &GenericBindingContext,
    _Out_ Location *&InferredTypeArgumentLocations,
    _Out_ TypeInferenceLevel &TypeInferenceLevel,
    _Out_ bool &AllFailedInferenceIsDueToObject,
    bool ignoreFirstParameter,
    bool SuppressMethodNameInErrorMessages,
    bool CandidateIsExtensionMethod,
    bool reportInferenceAssumptions, // Report error/warning for lambda-param-assumed-Object and array-literal-assumed-Object
    _Inout_opt_ AsyncSubAmbiguityFlagCollection **ppAsyncSubArgumentListAmbiguity
)
{
    ThrowIfFalse(!ignoreFirstParameter || CandidateIsExtensionMethod);

    IReadonlyBitVector * pFixedParameterBitVector = NULL;

    if
    (
        IsGeneric(TargetProcedure) &&
        (
            GenericBindingContext.IsNull() ||
            GenericBindingContext.IsGenericTypeBinding() ||
            GenericBindingContext.FreeTypeArgumentCount()
        )
    )
    {
        GenericBindingInfo save_GenericBindingContext = GenericBindingContext;

        // No binding has been supplied for the method. Allocate a binding with NULL type
        // arguments--they will be filled in below.

        bool InferenceOK = true;
        bool SomeInferenceFailed = false;

        unsigned TypeArgumentCount = TargetProcedure->GetGenericParamCount();;


        Type **ConflictingTypeArguments = new(m_TreeStorage) Type * [TypeArgumentCount];
        bool *TypeArgumentsByAssumption = NULL;
        Location *TypeArgumentLocations  = NULL;
        Type **TypeArguments;
        Type *_TypeArgumentsBuffer[12];
        bool TypeArgumentsStackAllocated = false;


        Parameter *BufferForParamsInferredFrom[8];
        Parameter **ParamsInferredFrom = BufferForParamsInferredFrom;

        if (TypeArgumentCount > DIM(BufferForParamsInferredFrom))
        {
            ParamsInferredFrom = (Parameter **)m_TreeStorage.Alloc(TypeArgumentCount * sizeof(Parameter *));
        }
        else
        {
            memset(BufferForParamsInferredFrom, 0, sizeof(BufferForParamsInferredFrom));
        }

        PartialGenericBinding * pPartialBinding = NULL;


        TypeArgumentsByAssumption = (bool*)m_TreeStorage.Alloc(sizeof(bool) * TypeArgumentCount);
        TypeArgumentLocations = (Location *)m_TreeStorage.Alloc(sizeof(Location) * TypeArgumentCount);
        StackAllocTypeArgumentsIfPossible_EX(TypeArguments, TypeArgumentCount, &m_TreeStorage, m_SymbolCreator, _TypeArgumentsBuffer, TypeArgumentsStackAllocated);


        if
        (
            (pPartialBinding = GenericBindingContext.PPartialGenericBinding(false))
        )
        {
            memcpy(TypeArgumentLocations, pPartialBinding->m_pTypeArgumentLocations, sizeof(Location) * TypeArgumentCount);
            pFixedParameterBitVector = pPartialBinding->m_pFixedTypeArgumentBitVector;
            memcpy(TypeArguments, pPartialBinding->m_pGenericBinding->GetArguments(), sizeof(BCSYM *) * TypeArgumentCount);
        }

        Parameter * FirstParameter = TargetProcedure->GetFirstParam();
        InferenceErrorReasons InferenceErrorReasons = InferenceErrorReasonsOther;
        if (ignoreFirstParameter && FirstParameter)
        {
            FirstParameter = FirstParameter->GetNext();
        }

        {
            TypeInference typeInference(
                this,
                m_Compiler,
                m_CompilerHost,
                &m_SymbolCreator,

                TargetProcedure,
                TypeArgumentCount,
                TypeArguments,
                TypeArgumentsByAssumption,
                TypeArgumentLocations,
                pFixedParameterBitVector,
                ParamsInferredFrom,
                &GenericBindingContext);

            typeInference.SetErrorReportingOptions(
                SuppressMethodNameInErrorMessages,
                m_ReportErrors,
                CandidateIsExtensionMethod);

            // For delegate, we need to type inference based on the return type of the delegate too
            if (DelegateReturnType && !TypeHelpers::IsVoidType(DelegateReturnType) &&
                TargetProcedure->GetType() && !TypeHelpers::IsVoidType(TargetProcedure->GetType()))
            {
                DelegateReturnType = ReplaceGenericParametersWithArguments(DelegateReturnType, GenericBindingContext, m_SymbolCreator);
            }
            else
            {
                DelegateReturnType = NULL;
            }

            typeInference.InferTypeParameters(
                FirstParameter,
                BoundArguments,
                FirstParamArrayArgument,
                DelegateReturnType,
                CallLocation,
                reportInferenceAssumptions,
                ppAsyncSubArgumentListAmbiguity);

            SomeInferenceFailed = typeInference.SomeInferenceHasFailed();
            TypeInferenceLevel = typeInference.GetTypeInferenceLevel();
            InferenceOK = !SomeInferenceFailed;
            AllFailedInferenceIsDueToObject = typeInference.AllFailedInferenceIsDueToObject();
            InferenceErrorReasons = typeInference.GetErrorReasons();
#if DEBUG
            if (SomeInferenceFailed && AllFailedInferenceIsDueToObject)
            {
                DBG_SWITCH_PRINTF(fDumpInference, "  !! All inference failures are due to object, generating late bound call.\n");
            }
#endif
        }


        // Make sure that AllFailedInferenceIsDueToObject only stays set,
        // if there was an actual inference failure.
        if (!SomeInferenceFailed || DelegateReturnType)
        {
            AllFailedInferenceIsDueToObject = false;
        }

        // Verify that all type arguments have been inferred.
        //
        if (!SomeInferenceFailed)
        {
            for (unsigned TypeArgumentIndex = 0; TypeArgumentIndex < TypeArgumentCount; TypeArgumentIndex++)
            {
                if (TypeArguments[TypeArgumentIndex] == NULL)
                {
                    InferenceOK = false;
                    // Bug 122092: AddressOf doesn't want detailed info on which parameters could not be
                    // inferred, just report the general type inference failed message in this case.
                    if (HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf))
                    {
                        SomeInferenceFailed = true;
                    }
                    else if (m_ReportErrors)
                    {
                        GenericParameter *UnboundParameter = TargetProcedure->GetFirstGenericParam();
                        for (unsigned TypeParameterIndex = 0; TypeParameterIndex < TypeArgumentIndex; TypeParameterIndex++)
                        {
                            UnboundParameter = UnboundParameter->GetNextParam();
                        }

                        StringBuffer textBuffer;

                        ReportMethodCallError
                        (
                            SuppressMethodNameInErrorMessages,
                            CandidateIsExtensionMethod,
                            false, // Special handling above for addressof error reporting
                            ERRID_UnboundTypeParam1,
                            ERRID_UnboundTypeParam2,
                            ERRID_UnboundTypeParam3,
                            ERRID_None,  // Special handling above for addressof error reporting
                            CallLocation,
                            ExtractErrorName
                            (
                                UnboundParameter,
                                textBuffer
                            ),
                            TargetProcedure,
                            pFixedParameterBitVector,
                            GenericBindingContext
                        );
                    }
                }
            }
        }

        if (SomeInferenceFailed)
        {            
            ERRID Error1;
            ERRID Error2;
            ERRID Error3;

            ComputeErrorsForInference(
                InferenceErrorReasons,
                OvrldFlags & OvrldDontReportAddTypeParameterInTypeInferErrors,
                &Error1,
                &Error2,
                &Error3);
            
            ReportMethodCallError
            (
                SuppressMethodNameInErrorMessages,
                CandidateIsExtensionMethod,
                HasFlag(OvrldFlags, OvrldReportErrorsForAddressOf),
                Error1,
                Error2,
                Error3,
                ERRID_DelegateBindingTypeInferenceFails,
                CallLocation,
                NULL, // No First Substitution.
                TargetProcedure,
                pFixedParameterBitVector,
                GenericBindingContext
            );
        }

        if (!InferenceOK)
        {
            //if(save_GenericBindingContext.IsGenericTypeBinding())
            //{
            //    GenericBindingContext = save_GenericBindingContext;  // shouldn't clear the binding here because eroor reporting may not do the right thing, simply restore the original value
            //}
            //else
            //{
            //    GenericBindingContext = (GenericBinding *)NULL;
            //}

            return false;
        }

        // Maybe report warnings for the type arguments we assumed?
        // See comment in InferTypeAndPropagateHints for what 'reportInferenceAssumptions' is about.
        // Basically, it tells us to report type-inference warnings even despite m_ReportErrors being turned off.
        if (reportInferenceAssumptions && OptionStrictOn())
        {
            BCSYM_GenericParam *reportParam = TargetProcedure->GetFirstGenericParam();
            for (unsigned int i=0; i<TypeArgumentCount; i++, reportParam=reportParam->GetNextParam())
            {
                if (!TypeArgumentsByAssumption[i])
                {
                    continue;
                }
                StringBuffer buf1, buf2, buf3;
                BackupValue<bool> backup_report_errors(&m_ReportErrors);
                m_ReportErrors = true;
                // "Data type of '|1' in '|2' could not be inferred. '|3' assumed."
                ReportSemanticError(WRNID_TypeInferenceAssumed3,
                                    TypeArgumentLocations[i],
                                    ExtractErrorName(reportParam, buf1),
                                    ExtractErrorName(TargetProcedure, buf2),
                                    ExtractErrorName(TypeArguments[i], buf3));
                backup_report_errors.Restore();
            }
        }


        //If we don't have a full method binding here, it means
        //that we either have a null binding, a generic type binding, or a partial binding.
        //In these cases we need to generate a binding and if the current binding is not a partial binding, then set it
        //up as our parent.
        if (! GenericBindingContext.IsFullMethodBinding())
        {
            GenericBindingContext =
                m_SymbolCreator.GetGenericBinding
                (
                    false,
                    TargetProcedure,
                    TypeArguments,
                    TypeArgumentCount,
                    GenericBindingContext.PGenericTypeBinding(false),
                    TypeArgumentsStackAllocated
                );

            if (OvrldFlags & (OvrldSomeCandidatesAreExtensionMethods))
            {
                GenericBindingContext.SetTypeArgumentLocationsAndOldPartialBinding(TypeArgumentLocations, pPartialBinding);
            }
        }

        InferredTypeArgumentLocations = TypeArgumentLocations;

        return InferenceOK;
    }

    GenericBindingContext.ConvertToFullBindingIfNecessary(this, TargetProcedure);
    AllFailedInferenceIsDueToObject = false;
    return true;
}


void
Semantics::ComputeErrorsForInference
(
    InferenceErrorReasons InferenceErrorReasons,
    bool NoExplicitTrySpecify,
    _Out_opt_ ERRID *ResultError1,
    _Out_opt_ ERRID *ResultError2,
    _Out_opt_ ERRID *ResultError3
)
{
    ThrowIfNull(ResultError1);
    ThrowIfNull(ResultError2);
    ThrowIfNull(ResultError3);

    ERRID Error1;
    ERRID Error2;
    ERRID Error3;

    if (InferenceErrorReasons & InferenceErrorReasonsAmbiguous)
    {
        if (NoExplicitTrySpecify)
        {
            Error1 = ERRID_TypeInferenceFailureNoExplicitAmbiguous1;
            Error2 = ERRID_TypeInferenceFailureNoExplicitAmbiguous2;
            Error3 = ERRID_TypeInferenceFailureNoExplicitAmbiguous3;
        }
        else
        {
            Error1 = ERRID_TypeInferenceFailureAmbiguous1;
            Error2 = ERRID_TypeInferenceFailureAmbiguous2;
            Error3 = ERRID_TypeInferenceFailureAmbiguous3;
        }
    }
    else if (InferenceErrorReasons & InferenceErrorReasonsNoBest)
    {
        if (NoExplicitTrySpecify)
        {
            Error1 = ERRID_TypeInferenceFailureNoExplicitNoBest1;
            Error2 = ERRID_TypeInferenceFailureNoExplicitNoBest2;
            Error3 = ERRID_TypeInferenceFailureNoExplicitNoBest3;
        }
        else
        {
            Error1 = ERRID_TypeInferenceFailureNoBest1;
            Error2 = ERRID_TypeInferenceFailureNoBest2;
            Error3 = ERRID_TypeInferenceFailureNoBest3;
        }
    }
    else
    {
        if (NoExplicitTrySpecify)
        {
            Error1 = ERRID_TypeInferenceFailureNoExplicit1;
            Error2 = ERRID_TypeInferenceFailureNoExplicit2;
            Error3 = ERRID_TypeInferenceFailureNoExplicit3;
        }
        else
        {
            Error1 = ERRID_TypeInferenceFailure1;
            Error2 = ERRID_TypeInferenceFailure2;
            Error3 = ERRID_TypeInferenceFailure3;
        }
    }

    *ResultError1 = Error1;
    *ResultError2 = Error2;
    *ResultError3 = Error3;
}

bool
Semantics::ArgumentTypePossiblyMatchesParamarrayArrayShape
(
    Type *ArgumentType,
    Type *ParamarrayParameterType
)
{
    VSASSERT(ParamarrayParameterType->IsArrayType(), "Non-array paramarray parameter type unexpected!!!");

    for(;
        ParamarrayParameterType->IsArrayType();
        ParamarrayParameterType = TypeHelpers::GetElementType(ParamarrayParameterType->PArrayType()))
    {
        if (!ArgumentType->IsArrayType() ||
            ArgumentType->PArrayType()->GetRank() != ParamarrayParameterType->PArrayType()->GetRank())
        {
            return false;
        }

        ArgumentType = TypeHelpers::GetElementType(ArgumentType->PArrayType());
    }

    return true;
}

GenericBinding *
SynthesizeOpenGenericBinding
(
    Declaration *Generic,
    _In_ Symbols &SymbolCreator
)
{
    // Count the number of parameters.

    unsigned ParameterCount = 0;
    GenericParameter *Parameter;

    for (Parameter = Generic->GetFirstGenericParam(); Parameter; Parameter = Parameter->GetNextParam())
    {
        ParameterCount++;
    }

    // Create the argument array.

    Type **Arguments;
    StackAllocTypeArgumentsIfPossible(Arguments, ParameterCount, SymbolCreator.GetNorlsAllocator(), SymbolCreator);

    if (ParameterCount > 0)
    {
        unsigned ArgumentIndex = 0;
        for (Parameter = Generic->GetFirstGenericParam(); Parameter && ParameterCount; Parameter = Parameter->GetNextParam())
        {
#pragma prefast(suppress: 26010, "Arguments buffer is correctly sized")
            Arguments[ArgumentIndex] = Parameter;
            ArgumentIndex++;
        }
    }

    BCSYM_GenericTypeBinding *pParentBinding;

    {
        pParentBinding = IsGenericOrHasGenericParent(Generic->GetParent()) ?
                SynthesizeOpenGenericBinding(Generic->GetParent(), SymbolCreator)->PGenericTypeBinding() :
                NULL;
    }

    return
        SymbolCreator.GetGenericBinding(
            false,
            Generic,
            Arguments,
            ParameterCount,
            pParentBinding,
            ArgumentsStackAllocated);  // if stack allocated type arguments, need to copy over when creating the binding
}

struct DerivedClassBinding
{
    GenericTypeBinding *Binding;
    DerivedClassBinding *Derived;
};

static GenericTypeBinding *
BindBaseType
(
    _In_ DerivedClassBinding *Base,
    _In_ Symbols &SymbolCreator
)
{
    if (Base->Binding == NULL)
    {
        return NULL;
    }

    return
        ReplaceGenericParametersWithArguments(
            Base->Binding,
            Base->Derived ? BindBaseType(Base->Derived, SymbolCreator) : NULL,
            SymbolCreator)->PGenericTypeBinding();
}

static bool
DeriveGenericBinding
(
    Type *BaseType,
    Declaration *Member,
    DerivedClassBinding *Derived,
    _In_ Symbols &SymbolCreator,
    _Out_ GenericTypeBinding *&Result
)
{
    if (BaseType == NULL || BaseType->IsGenericBadNamedRoot())
    {
        Result = NULL;
        return false;
    }

    DerivedClassBinding BaseBinding =
        {
            TypeHelpers::IsGenericTypeBinding(BaseType) ? BaseType->PGenericTypeBinding() : NULL,
            Derived
        };

    if (Member->GetContainer() == BaseType->PContainer())
    {
        Result = BindBaseType(&BaseBinding, SymbolCreator);
        return true;
    }

    if (TypeHelpers::IsClassOrRecordType(BaseType))
    {
        return
            DeriveGenericBinding(
                BaseType->PClass()->GetBaseClass(),
                Member,
                &BaseBinding,
                SymbolCreator,
                Result);
    }

    for (BCSYM_Implements *InterfaceBase = BaseType->PInterface()->GetFirstImplements();
         InterfaceBase;
         InterfaceBase = InterfaceBase->GetNext())
    {
        if (!InterfaceBase->IsBadImplements() &&
            !InterfaceBase->GetCompilerRoot()->IsBad())
        {
            if (DeriveGenericBinding(
                    InterfaceBase->GetCompilerRoot(),
                    Member,
                    &BaseBinding,
                    SymbolCreator,
                    Result))
            {
                return true;
            }
        }
    }

    return false;
}

// Derive the generic type binding applicable to referring to a declaration that is a member of
// a possibly generic type (or a base type of that type).
// A non-null result occurs if the type owning the declaration is a binding of a generic type or
// is declared within a generic type and is referred to through a binding of that type.
//
// The implementation strategy is to first determine the chain of base types from the original
// (most derived) type to the type containing the member, walk this chain starting from
// the type containing the member and ending with the first type that is not generic,
// and then construct the binding in the reverse order of the walk.

GenericTypeBinding *
DeriveGenericBindingForMemberReference
(
    Type *PossiblyGenericType,
    Declaration *Member,
    _In_ Symbols &SymbolCreator,
    CompilerHost *CompilerHost
)
{
    if (!Member->HasGenericParent())
    {
        return NULL;
    }

    if (PossiblyGenericType == NULL)
    {
        return NULL;
    }

    PossiblyGenericType = PossiblyGenericType->ChaseToType();
    GenericTypeBinding *Result = NULL;

    if (TypeHelpers::IsClassOrRecordType(PossiblyGenericType) || TypeHelpers::IsInterfaceType(PossiblyGenericType))
    {
        DeriveGenericBinding(
            PossiblyGenericType,
            Member,
            NULL,
            SymbolCreator,
            Result);

        return Result;
    }
    else if (TypeHelpers::IsGenericParameter(PossiblyGenericType))
    {
        GenericParameter *TypeParameter = PossiblyGenericType->PGenericParam();

        Type *ClassConstraint =
            GetClassConstraint(
                TypeParameter,
                CompilerHost,
                SymbolCreator.GetNorlsAllocator(),
                true,  // ReturnArraysAs"System.Array"
                false  // don't ReturnValuesAs"System.ValueType"or"System.Enum"
                );

        if (ClassConstraint != NULL)
        {
            DeriveGenericBinding(
                ClassConstraint,
                Member,
                NULL,
                SymbolCreator,
                Result);

            return Result;
        }

        for (GenericTypeConstraint *Constraint = TypeParameter->GetTypeConstraints();
             Constraint;
             Constraint = Constraint->Next())
        {
            if (Constraint->IsBadConstraint())
            {
                continue;
            }

            Result = NULL;

            if (DeriveGenericBinding(
                    Constraint->GetType(),
                    Member,
                    NULL,
                    SymbolCreator,
                    Result))
            {
                return Result;
            }
        }
    }

    if (TypeHelpers::IsVoidType(PossiblyGenericType))
    {
        return NULL;
    }

    // ??? Anything else to try?

    return NULL;
}

GenericBinding *
Semantics::ValidateGenericArguments
(
    const Location &TextSpan,
    Declaration *Generic,
    _In_count_(ArgumentCount) Type **BoundArguments,
    Location TypeArgumentLocations[],
    unsigned ArgumentCount,
    GenericTypeBinding *ParentBinding,
    _Out_ bool &ResultIsBad
)
{
    // At this point the number of arguments is known to be correct.

    GenericBinding *Result =
        m_SymbolCreator.GetGenericBinding(
            false,
            Generic,
            BoundArguments,
            ArgumentCount,
            ParentBinding);

    // Check the the argument types against the parameter constraints.

    if (m_ReportErrors)
    {

        if (! Bindable::CheckGenericConstraints(
            Result,
            TypeArgumentLocations,
            NULL,
            m_Errors,
            m_CompilerHost,
            m_Compiler,
            &m_SymbolCreator,
            m_CompilationCaches))
        {
            ResultIsBad = true;
        }
    }

    return Result;
}

void
Semantics::ReportBadType
(
    Type *BadType,
    const Location &ErrorLocation
)
{
    if (m_ReportErrors)
    {
        ReportBadType(BadType, ErrorLocation, m_Compiler, m_Errors);
    }
}


// static
void
Semantics::ReportBadType
(
    Type *BadType,
    const Location &ErrorLocation,
    Compiler * pCompiler, 
    ErrorTable * pErrorTable
)
{
    VSASSERT(TypeHelpers::IsBadType(BadType), "Advertised bad type isn't.");

    if (TypeHelpers::IsPointerType(BadType))
    {
        BadType = TypeHelpers::GetReferencedType(BadType->PPointerType());
    }

    while (TypeHelpers::IsArrayType(BadType))
    {
        BadType = TypeHelpers::GetElementType(BadType->PArrayType());
    }

    VSASSERT(TypeHelpers::IsBadType(BadType), "Advertised bad type isn't.");

    // Remarkably, there are cases where a type is bad but IsBad returns
    // false. In these cases, no error is necessary here because one
    // will have been reported for the type already.

    if (BadType->IsBad())
    {
        if (BadType->IsGenericTypeBinding())
        {
            GenericTypeBinding *Binding = BadType->PGenericTypeBinding();

            if (TypeHelpers::IsBadType(Binding->GetGenericType()))
            {
                ReportBadType(Binding->GetGenericType(), ErrorLocation, pCompiler, pErrorTable);
            }

            unsigned ArgumentCount = Binding->GetArgumentCount();
            for (unsigned ArgumentIndex = 0; ArgumentIndex < ArgumentCount; ArgumentIndex++)
            {
                if (TypeHelpers::IsBadType(Binding->GetArgument(ArgumentIndex)))
                {
                    ReportBadType(Binding->GetArgument(ArgumentIndex), ErrorLocation, pCompiler, pErrorTable);
                }
            }
        }
        else
        {
            BadType->PNamedRoot()->ReportError(
                pCompiler,
                pErrorTable,
                (Location *)&ErrorLocation);
        }
    }
}

void
Semantics::ReportBadDeclaration
(
    Declaration *Bad,
    const Location &ErrorLocation
)
{
    if (m_ReportErrors)
    {
        ReportBadDeclaration(Bad, ErrorLocation, m_Compiler, m_Errors);
    }
}

// static
void
Semantics::ReportBadDeclaration
(
    Declaration *Bad,
    const Location &ErrorLocation,
    Compiler * pCompiler, 
    ErrorTable * pErrorTable
)
{
    VSASSERT(IsBad(Bad), "Advertised bad declaration isn't.");

    // Remarkably, there are cases where a symbol is bad but IsBad returns
    // false. In these cases, no error is necessary here because one
    // will have been reported for the type already.
    // Which cases??

    if (Bad->IsBad())
    {
        Bad->ReportError(
            pCompiler,
            pErrorTable,
            (Location *)&ErrorLocation);
    }
}

WCHAR *
Semantics::ExtractErrorName
(
    Symbol *Substitution,
    StringBuffer &TextBuffer,
    bool FormatAsExtensionMethod,
    IReadonlyBitVector * FixedTypeArgumentBitVector,
    GenericBinding * GenericBindingContext
)
{
    if (m_Errors==NULL)
    {
        VSFAIL("ExtractErrorName has been called when m_Errors was NULL. This should never happen. The caller should avoid calling ExtractErrorName in such circumstances.");
        return L""; // fallback so we don't crash at runtime
    }

    return
        m_Errors->ExtractErrorName
        (
            Substitution,
            m_Procedure == NULL ? NULL : m_Procedure->GetContainingClass(),
            TextBuffer,
            FormatAsExtensionMethod ? GenericBindingContext : NULL,
            FormatAsExtensionMethod,
            FixedTypeArgumentBitVector,
            m_CompilerHost
        );
}

BCSYM_NamedRoot* 
Semantics::GetContextOfSymbolUsage()
{
    BCSYM_NamedRoot *ContextOfSymbolUsage = NULL;
    if (m_NamedContextForAppliedAttribute) 
    {
    
       // In the following case, assuming CC1 is obsolete. No obsolete warning should
       // report for CC1.val, since m_NamedContextForAppliedAttribute refers to Foo1
       // and Foo1 is obsolete
       // Example:
       //     <Obsolete> _
       //     <MyAttribute1(CC1.val)> Sub Foo1()  'not expect warning
       //     End Sub
    
       return m_NamedContextForAppliedAttribute;
    }
    else if (m_NamedContextForConstantUsage)
    {
        // if m_NamedContextForConstantUsage is defined, then it means 
        // the symbol is a constant and used in an optional parameter  OR,
        // in a contant field
        return m_NamedContextForConstantUsage;
    }
    else if (m_Procedure)
    {
        // ObsoleteCheckTarget is the current procedure or container. if both m_Procedure and m_ContainingContainer is
        // defined, we should assign m_Procedure to ObsoleteCheckTarget since m_Procedure must be not farther to the 
        // symbol than m_ContainingContainer. ("Farther" is in terms of container hierarchy.)

        return m_Procedure;
    }
    else
    {
        return ContainingContainer();
    }

}

void
Semantics::CheckObsolete
(
    Symbol *SymbolToCheck,
    const Location &ErrorLocation
)
{

    if (PerformObsoleteChecks()) 
    {
        ObsoleteChecker::CheckObsolete(
            SymbolToCheck,
            (Location *)&ErrorLocation,
            // The container context in which the Obsolete symbol was bound to
            ContainingContainer(),
            // The type context in which the Obsolete symbol was used.
            // This should be the same as the ContainingContainer except for
            // attributes applied to non-namespace containers in which
            // ContainingContainer will be the parent of the non-namespace
            // container, but ContainerContextForObsoleteSymbolUsage would
            // be the non-namespace container.
            ContainerContextForObsoleteSymbolUsage(),
            m_Errors,
            m_SourceFile,
            GetContextOfSymbolUsage());
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(ErrorId, (Location *)&ErrorLocation);
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    ParseTree::Statement::Opcodes Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    ParseTree::Expression::Opcodes Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    BILOP Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ILTree::ILNode *Substitution1
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location *ErrorLocation,
    Symbol *Substitution1
)
{
    ReportSemanticError(ErrorId, *ErrorLocation, Substitution1);
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    StringBuffer &Substitution1
)
{
    ReportSemanticError(ErrorId, ErrorLocation, Substitution1.GetString());
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError
        (
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2)
        );
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    ACCESS Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            MessageSubstitution(Substitution2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ILTree::ILNode *Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    _In_opt_z_ const WCHAR *Substitution2
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            Substitution2 ? Substitution2 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    _In_opt_z_ const WCHAR *Extra,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateErrorWithExtra(
            ErrorId,
            Extra,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"");
    }
}


void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    const Quadword Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        WCHAR TextBuffer1[65];
        StringBuffer TextBuffer2;

        _i64tow_s(Substitution1, TextBuffer1, _countof(TextBuffer1), 10);

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            TextBuffer1,
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    _In_opt_z_ const WCHAR *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            Substitution2 ? Substitution2 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1,
    _In_ ParseTree::Expression *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer),
            MessageSubstitution(Substitution2, TextBuffer));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    StringBuffer &Substitution1,
    _In_ CompileError *Substitution2
)
{
    ReportSemanticError(
        ErrorId,
        ErrorLocation,
        Substitution1.GetString(),
        Substitution2->m_wszMessage);
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    StringBuffer &Substitution2
)
{
    ReportSemanticError(
        ErrorId,
        ErrorLocation,
        Substitution1,
        Substitution2.GetString());
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1,
    StringBuffer &Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            Substitution2.GetString());
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1,
    _In_opt_z_ const WCHAR *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            Substitution2 ? Substitution2 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ILTree::ILNode *Substitution1,
    StringBuffer &Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            Substitution2.GetString());
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    BILOP Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1),
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_ ParseTree::Expression *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorID,
    const Location & ErrorLocation,
    _In_opt_z_ STRING * Substitution1,
    _In_opt_z_ STRING * Substitution2,
    _In_opt_z_ STRING * Substitution3,
    _In_opt_z_ STRING * Substitution4,
    _In_opt_z_ STRING * Substitution5,
    _In_opt_z_ STRING * Substitution6,
    _In_opt_z_ STRING * Substitution7
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError
        (
            ErrorID,
            (Location *)&ErrorLocation,
            Substitution1,
            Substitution2,
            Substitution3,
            Substitution4,
            Substitution5,
            Substitution6,
            Substitution7
        );
    }

}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    _In_ ILTree::Expression *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            MessageSubstitution(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    ParseTree::Expression::Opcodes Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    BILOP Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    BILOP Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3,
    Symbol *Substitution4
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;
        StringBuffer TextBuffer4;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3),
            ExtractErrorName(Substitution4, TextBuffer4));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3,
    Symbol *Substitution4
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;
        StringBuffer TextBuffer4;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3),
            ExtractErrorName(Substitution4, TextBuffer4));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    ACCESS Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            MessageSubstitution(Substitution3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    ACCESS Substitution1
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError
        (
            ErrorId,
            (Location *)&ErrorLocation,
            MessageSubstitution(Substitution1)
        );
    }
}


void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    Symbol *Substitution2,
    StringBuffer &Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            ExtractErrorName(Substitution2, TextBuffer2),
            Substitution3.GetString());
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    _In_z_ WCHAR *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            Substitution3);
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    _In_z_ WCHAR *Substitution3,
    _In_z_ WCHAR *Substitution4,
    _In_z_ WCHAR *Substitution5
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            Substitution3,
            Substitution4,
            Substitution5);
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    _In_z_ WCHAR *Substitution3,
    _In_z_ WCHAR *Substitution4,
    _In_z_ WCHAR *Substitution5,
    _In_z_ WCHAR *Substitution6
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            Substitution3,
            Substitution4,
            Substitution5,
            Substitution6);
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    _In_z_ WCHAR *Substitution2,
    _In_z_ WCHAR *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            Substitution2,
            Substitution3);
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    CompilerProject *Substitution2,
    CompilerProject *Substitution3
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateErrorWithExtra(
            ErrorId,
            Substitution2->GetErrorName(),
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            Substitution2->IsMetaData() ? Substitution2->GetScopeName() : Substitution2->GetFileName(),
            Substitution3->IsMetaData() ? Substitution3->GetScopeName() : Substitution3->GetFileName());
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    _In_opt_z_ const WCHAR *wszExtra,
    const Location &ErrorLocation,
    Symbol *Substitution1
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;

        m_Errors->CreateErrorWithExtra(
            ErrorId,
            wszExtra,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    _In_opt_z_ const WCHAR *wszExtra,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateErrorWithExtra(
            ErrorId,
            wszExtra,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3,
    Symbol *Substitution4
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;
        StringBuffer TextBuffer4;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3),
            ExtractErrorName(Substitution4, TextBuffer4));
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    Symbol *Substitution3,
    _In_opt_z_ const WCHAR *Substitution4
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;
        StringBuffer TextBuffer3;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            ExtractErrorName(Substitution3, TextBuffer3),
            Substitution4 ? Substitution4 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    Symbol *Substitution1,
    Symbol *Substitution2,
    _In_opt_z_ const WCHAR *Substitution3,
    _In_opt_z_ const WCHAR *Substitution4
)
{
    if (m_ReportErrors)
    {
        StringBuffer TextBuffer1;
        StringBuffer TextBuffer2;

        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, TextBuffer1),
            ExtractErrorName(Substitution2, TextBuffer2),
            Substitution3 ? Substitution3 : L"",
            Substitution4 ? Substitution4 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorId,
    const Location &ErrorLocation,
    _In_opt_z_ const WCHAR *Substitution1,
    _In_opt_z_ const WCHAR *Substitution2,
    _In_opt_z_ const WCHAR *Substitution3
)
{
    if (m_ReportErrors)
    {
        m_Errors->CreateError(
            ErrorId,
            (Location *)&ErrorLocation,
            Substitution1 ? Substitution1 : L"",
            Substitution2 ? Substitution2 : L"",
            Substitution3 ? Substitution3 : L"");
    }
}

void
Semantics::ReportSemanticError
(
    unsigned ErrorID,
    const Location & ErrorLocation,
    Symbol * Substitution1,
    _In_opt_z_ const WCHAR * Substitution2,
    _In_opt_z_ const WCHAR * Substitution3,
    _In_opt_z_ const WCHAR * Substitution4
)
{
    StringBuffer buffer1;

    if (m_ReportErrors)
    {
        m_Errors->CreateError
        (
            ErrorID,
            (Location *)&ErrorLocation,
            ExtractErrorName(Substitution1, buffer1),
            Substitution2,
            Substitution3,
            Substitution4
        );
    }
}

void
Semantics::ReportMissingType
(
    FX::TypeName Type,
    const Location & ErrorLocation
)
{
    if (m_ReportErrors)
    {
        GetFXSymbolProvider()->ReportTypeMissingErrors(Type, m_Errors, ErrorLocation);
    }
}

void
Semantics::ReportuntimeHelperNotFoundError
(
    const Location & ErrorLocation,
    _In_opt_z_ const WCHAR *NameOfMissingHelper
)
{
    ReportSemanticError(
        ERRID_MissingRuntimeHelper,
        ErrorLocation,
        NameOfMissingHelper);
}

const WCHAR *
Semantics::MessageSubstitution
(
    ParseTree::Expression::Opcodes Opcode
)
{
    const WCHAR *Substitution = L"...";

    switch (Opcode)
    {
        case ParseTree::Expression::Plus:
        case ParseTree::Expression::UnaryPlus:
            Substitution = L"+";
            break;
        case ParseTree::Expression::Minus:
        case ParseTree::Expression::Negate:
            Substitution = L"-";
            break;
        case ParseTree::Expression::Multiply:
            Substitution = L"*";
            break;
        case ParseTree::Expression::Power:
            Substitution = L"^";
            break;
        case ParseTree::Expression::Divide:
            Substitution = L"/";
            break;
        case ParseTree::Expression::Modulus:
            Substitution = L"Mod";
            break;
        case ParseTree::Expression::IntegralDivide:
            Substitution = L"\\";
            break;
        case ParseTree::Expression::ShiftLeft:
            Substitution = L"<<";
            break;
        case ParseTree::Expression::ShiftRight:
            Substitution = L">>";
            break;
        case ParseTree::Expression::Xor:
            Substitution = L"Xor";
            break;
        case ParseTree::Expression::Concatenate:
            Substitution = L"&";
            break;
        case ParseTree::Expression::Or:
            Substitution = L"Or";
            break;
        case ParseTree::Expression::OrElse:
            Substitution = L"OrElse";
            break;
        case ParseTree::Expression::And:
            Substitution = L"And";
            break;
        case ParseTree::Expression::AndAlso:
            Substitution = L"AndAlso";
            break;
        case ParseTree::Expression::Not:
            Substitution = L"Not";
            break;
        case ParseTree::Expression::Like:
            Substitution = L"Like";
            break;
        case ParseTree::Expression::Equal:
            Substitution = L"=";
            break;
        case ParseTree::Expression::NotEqual:
            Substitution = L"<>";
            break;
        case ParseTree::Expression::LessEqual:
            Substitution = L"<=";
            break;
        case ParseTree::Expression::GreaterEqual:
            Substitution = L">=";
            break;
        case ParseTree::Expression::Less:
            Substitution = L"<";
            break;
        case ParseTree::Expression::Greater:
            Substitution = L">";
            break;
        case ParseTree::Expression::MyClass:
            Substitution = L"MyClass";
            break;
        case ParseTree::Expression::MyBase:
            Substitution = L"MyBase";
            break;
        case ParseTree::Expression::Me:
            Substitution = L"Me";
            break;
        case ParseTree::Expression::Nothing:
            Substitution = L"Nothing";
            break;
        default:
            break;
    }

    return Substitution;
}

const WCHAR *
Semantics::MessageSubstitution
(
    ParseTree::Statement::Opcodes Opcode
)
{
    const WCHAR *Substitution = L"...";

    switch (Opcode)
    {
        case ParseTree::Statement::Redim:
            Substitution = L"Redim";
            break;
        case ParseTree::Statement::Erase:
            Substitution = L"Erase";
            break;
        default:
            break;
    }

    return Substitution;
}

const WCHAR *
Semantics::MessageSubstitution
(
    _In_ ParseTree::Expression *TreeToRepresent,
    StringBuffer &TextBuffer
)
{
    switch (TreeToRepresent->Opcode)
    {
        case ParseTree::Expression::Name:
            return TreeToRepresent->AsName()->Name.Name;
        case ParseTree::Expression::BangQualified:
        case ParseTree::Expression::DotQualified:
            return MessageSubstitution(TreeToRepresent->AsQualified()->Name, TextBuffer);
        case ParseTree::Expression::CallOrIndex:
            return MessageSubstitution(TreeToRepresent->AsCallOrIndex()->Target, TextBuffer);
        case ParseTree::Expression::Parenthesized:
            return MessageSubstitution(TreeToRepresent->AsUnary()->Operand, TextBuffer);
    }

    return MessageSubstitution(TreeToRepresent->Opcode);
}

const WCHAR *
Semantics::MessageSubstitution
(
    BILOP Opcode
)
{
    const WCHAR *Substitution = L"...";

    switch (Opcode)
    {
        case SX_ADD:
            Substitution = L"+";
            break;
        case SX_SUB:
            Substitution = L"-";
            break;
        case SX_MUL:
            Substitution = L"*";
            break;
        case SX_POW:
            Substitution = L"^";
            break;
        case SX_DIV:
            Substitution = L"/";
            break;
        case SX_MOD:
            Substitution = L"Mod";
            break;
        case SX_IDIV:
            Substitution = L"\\";
            break;
        case SX_XOR:
            Substitution = L"Xor";
            break;
        case SX_CONC:
            Substitution = L"&";
            break;
        case SX_ORELSE:
            Substitution = L"OrElse";
            break;
        case SX_OR:
            Substitution = L"Or";
            break;
        case SX_ANDALSO:
            Substitution = L"AndAlso";
            break;
        case SX_AND:
            Substitution = L"And";
            break;
        case SX_NOT:
            Substitution = L"Not";
            break;
        case SX_PLUS:
            Substitution = L"+";
            break;
        case SX_NEG:
            Substitution = L"-";
            break;
        case SX_EQ:
            Substitution = L"=";
            break;
        case SX_NE:
            Substitution = L"<>";
            break;
        case SX_LE:
            Substitution = L"<=";
            break;
        case SX_GE:
            Substitution = L">=";
            break;
        case SX_LT:
            Substitution = L"<";
            break;
        case SX_GT:
            Substitution = L">";
            break;
        case SX_NOTHING:
            Substitution = L"Nothing";
            break;
        case SL_REDIM:
            Substitution = L"Redim";
            break;
        case SX_SHIFT_LEFT:
            Substitution = L"<<";
            break;
        case SX_SHIFT_RIGHT:
            Substitution = L">>";
            break;

        default:
            break;
    }

    return Substitution;
}

const WCHAR *
Semantics::MessageSubstitution
(
    ACCESS Access
)
{
    switch (Access)
    {
        case ACCESS_Private:
            return L"Private";
        case ACCESS_Friend:
            return L"Friend";
        case ACCESS_Protected:
            return L"Protected";
        case ACCESS_ProtectedFriend:
            return L"Protected Friend";
        case ACCESS_Public:
            return L"Public";
        default:
            VSFAIL("Surprising accessibility.");
            return L"";
    }
}

const WCHAR *
Semantics::MessageSubstitution
(
    _In_ ILTree::ILNode *TreeToRepresent,
    StringBuffer &TextBuffer
)
{
    switch (TreeToRepresent->bilop)
    {
        case SX_NAME:
            return TreeToRepresent->AsArgumentNameExpression().Name;
        case SX_SYM:
            return ExtractErrorName(TreeToRepresent->AsSymbolReferenceExpression().Symbol, TextBuffer);
        case SX_PROPERTY_REFERENCE:
            return MessageSubstitution(TreeToRepresent->AsPropertyReferenceExpression().Left, TextBuffer);
        case SX_CALL:
            return MessageSubstitution(TreeToRepresent->AsCallExpression().Left, TextBuffer);
        case SX_COLINITELEMENT:
            return MessageSubstitution(TreeToRepresent->AsColInitElementExpression().CallExpression, TextBuffer);
    }

    return MessageSubstitution(TreeToRepresent->bilop);
}

GenericBinding*
Semantics::GetGenericTypeBindingContextForContainingClass()
{
    return NULL;
}

void
Semantics::GetProcedureState
(
    _Out_ ProcedureState *state
)
{
    ThrowIfNull(state);

    state->Proc= this->m_Procedure;
    state->Body= this->m_ProcedureTree;
    state->TemporaryMgr = this->m_TemporaryManager;
}

void
Semantics::SetProcedureState
(
    _In_ ProcedureState *state
)
{
    ThrowIfNull(state);

    m_Procedure = state->Proc;
    m_ProcedureTree = state->Body;
    m_TemporaryManager = state->TemporaryMgr;
}

unsigned
ExpressionListLength
(
    ExpressionList *List
)
{
    unsigned Length = 0;

    while (List)
    {
        VSASSERT(List->bilop == SX_LIST, "Advertised list isn't.");

        Length++;
        List = List->AsExpressionWithChildren().Right;
    }

    return Length;
}

void
CheckRestrictedType
(
    ERRID errid,
    BCSYM *Type,
    Location *ErrorLocation,
    CompilerHost *CompilerHost,
    ErrorTable *Errors

)
{
    if (Type && Errors)
    {
        if (IsRestrictedType(Type, CompilerHost))
        {
            StringBuffer TextBuffer;

            Errors->CreateError(
                errid,
                ErrorLocation,
                Errors->ExtractErrorName(Type, NULL, TextBuffer));
        }
    }
}

bool
IsRestrictedType
(
    BCSYM *Type,
    CompilerHost *CompilerHost
)
{
    return
        (CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::TypedReferenceType) &&
             TypeHelpers::EquivalentTypes(Type, CompilerHost->GetFXSymbolProvider()->GetType(FX::TypedReferenceType))) ||
        (CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::ArgIteratorType) &&
             TypeHelpers::EquivalentTypes(Type, CompilerHost->GetFXSymbolProvider()->GetType(FX::ArgIteratorType))) ||
        (CompilerHost->GetFXSymbolProvider()->IsTypeAvailable(FX::RuntimeArgumentHandleType) &&
             TypeHelpers::EquivalentTypes(Type, CompilerHost->GetFXSymbolProvider()->GetType(FX::RuntimeArgumentHandleType)));
}

void
CheckRestrictedArrayType
(
    BCSYM *Type,
    Location *ErrorLocation,
    CompilerHost *CompilerHost,
    ErrorTable *Errors

)
{
    if (Type && Type->IsArrayType())
    {
        BCSYM *ElementType = Type->ChaseToType();
        CheckRestrictedType(ERRID_RestrictedType1, ElementType, ErrorLocation, CompilerHost, Errors);
    }
}

bool
IsRestrictedArrayType
(
    BCSYM *Type,
    CompilerHost *CompilerHost
)
{
    if (Type && Type->IsArrayType())
    {
        BCSYM *ElementType = Type->ChaseToType();
        return IsRestrictedType(ElementType, CompilerHost);
    }

    return false;
}


Location&
Semantics::GetSpan
(
    _Out_ Location &Target,
    Location &StartLocation,
    Location &EndLocation
)
{
    Target.SetLocation(
        StartLocation.m_lBegLine,
        StartLocation.m_lBegColumn,
        EndLocation.m_lEndLine,
        EndLocation.m_lEndColumn);
    return Target;
}

void
Semantics::RegisterTransientSymbol
(
    Declaration *Symbol
)
{
    if (m_SymbolsCreatedDuringInterpretation)
    {
        Procedure* pProc = m_TransientSymbolOwnerProc ? m_TransientSymbolOwnerProc : m_Procedure;
        m_SymbolsCreatedDuringInterpretation->Insert(Symbol, pProc);
    }
}

void
Semantics::RegisterAnonymousDelegateTransientSymbolIfNotAlreadyRegistered
(
    _In_ BCSYM_Class *pSymbol
)
{
    if (m_SymbolsCreatedDuringInterpretation)
    {
        m_SymbolsCreatedDuringInterpretation->InsertAnonymousDelegateIfNotAlreadyInserted(pSymbol, m_Procedure);
    }
}

void
Semantics::TransformDeferredTemps()
{
    if (m_methodDeferredTempCount > 0)
    {
        m_methodDeferredTempCount = 0;

        DeferredTempIterator deferred(this);

        deferred.Iterate();

        // During processing in the iterator, all SX_DEFERRED_TEMPs should be removed.
        // If the method still has deferred temps then they were created during iterate.
        // This is a bug in the iterate code. 

        ThrowIfTrue(m_methodDeferredTempCount != 0);
    }
}

ILTree::ProcedureBlock*
Semantics::TransformMethod
(
    ILTree::ProcedureBlock *OuterMostProcedureBoundTree
)
{
    if (!OuterMostProcedureBoundTree ||
        IsBad(OuterMostProcedureBoundTree) ||                           // errors in the bound tree
        m_IsGeneratingXML ||                                            // uncomplete bound tree for XML generaton
        !m_Procedure ||                                                 // guard for the next checks, m_Procedure can be null.
        m_Procedure->GetPWellKnownAttrVals()->GetDllImportData() ||     //DLL imports
        m_Procedure->IsMustOverrideKeywordUsed())                       // must override (abstract) methods
    {
        return OuterMostProcedureBoundTree;
    }

    // Xml literals leaves special nodes in the tree for locals used to cache
    // XNames, XNamespaces.  These nodes must be removed and the locals must be 
    // allocates the the procedure level and before the closure code is run.  Do
    // it now.

    TransformDeferredTemps();

    InitClosures(OuterMostProcedureBoundTree->pproc, OuterMostProcedureBoundTree);

#if DEBUG

    // This is a trade off between performance and testing coverage.  We want to
    // gaurantee that closures can run accross any BILTREE we produce yet we
    // don't want it to always run because it only needs to be run when there is
    // an SX_LAMBDA.  Running it on every tree will just degrade performance
    // needlessly
    //
    // Running closures all the time in debug opens up a new set of problems where
    // closures could be hiding real bugs in the code and they will only show up
    // in retail builds.
    //
    // A compromise is to run the BiltreeIterator on every bound tree in debug.  It
    // will forcefully iterater the tree and do nothing except assert if the tree
    // is not as expected.  This will gaurantee we can iterate every produced
    // tree without additionally adding any extra issues.
    if ( OuterMostProcedureBoundTree )
    {
        BoundTreeDebugVisitor it(m_Compiler);
        it.Visit(OuterMostProcedureBoundTree);
    }

#endif

    if ( m_closureRoot )
    {
        // Increase the statement group id here to gaurantee that closures will generate
        // statements with a completely different group ID than those generated for the
        // normal biltree
        ++m_statementGroupId;

        m_closureRoot->Iterate();
        m_closureRoot->CreateClosureCode();
        CleanupClosures();
    }

    return OuterMostProcedureBoundTree;

}

void
Semantics::InitClosures(Procedure *proc, ILTree::ProcedureBlock *body)
{
    if ( m_closureRoot )
    {
        CleanupClosures();
    }

    // Don't create the closure code if there were no lambdas in the method body.  It is
    // pointless to run closures when there are no lambdas and will detract from
    // the overall performance of the compiler.
    if ( proc && body && m_methodHasLambda)
    {
        m_closureRoot = new ClosureRoot(proc, body, this, this->m_TransientSymbolCreator);
    }
}

void
Semantics::CleanupClosures()
{
    if ( m_closureRoot )
    {
        delete m_closureRoot;
        m_closureRoot = NULL;
    }
}

Procedure*
Semantics::FindRealIsTrueOperator
(
    ILTree::Expression* Input
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ISTRUE );

    if( Input->AsExpressionWithChildren().Left->bilop == SX_CALL &&
        Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_SYM &&
        Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol->IsLiftedOperatorMethod()
      )
    {
        LiftedOperatorMethod* lom =
            Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->AsSymbolReferenceExpression().Symbol->PLiftedOperatorMethod();

        if( lom->GetActualProc()->GetAssociatedOperatorDef()->GetOperator() == OperatorIsTrue )
        {
            // Only return the proc if the lifted operator is IsTrue.
            return( lom->GetActualProc()->PMethodImpl() );
        }
    }

    return( NULL );
}

ILTree::Expression*
Semantics::FindOperandForIsTrueOperator
(
    ILTree::Expression* Input
)
{
    ThrowIfNull( Input );
    ThrowIfFalse( Input->bilop == SX_ISTRUE );

    // If this is not a lifted IsTrue, then we just use the standard operand.

    if( FindRealIsTrueOperator( Input ) == NULL )
    {
        return( Input->AsExpressionWithChildren().Left );
    }

    // Otherwise, dig through and find the operand from the IsTrue call.

    // This would be the case where IsTrue is applied on a T? where T != bool,
    // and there is an operator. Thus, find the argument for this operator.

    VSASSERT( Input->AsExpressionWithChildren().Left->bilop == SX_CALL, "Need a call here." );
    VSASSERT( Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Left->bilop == SX_SYM, "Need a symbol here." );
    VSASSERT( Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left->bilop == SX_SYM, "Need a symbol for the arg" );

    return(
        Input->AsExpressionWithChildren().Left->AsExpressionWithChildren().Right->AsExpressionWithChildren().Left
        );
}

//==============================================================================
// Bound Tree Lowering semantics
//==============================================================================
// The lowering step in bound tree semantics is intended for those tree rewritings and optimizations that are
// necessary only for the code generation. The resulting trees are operational equivalent but they may incur
// some loss on the original user code semantics.
// Such rewritings should not be done early while building the tree because they confuse the subsequent various
// language analysis steps on the tree.
// For example a Ctype between two Nullable types is translated here in an SX_IFF structure that contains some
// conversions, temp assignments and possibly a call to a defined implicit ops for conversions. Had this
// transformation hapen while building the tree, the expression tree generator would have a hard
// time to recognize the SX_IIF structure and to decide whether it is generated by an user IIF or by
// a an implicit CType() .
//
// The lowerring semantics is done for the procedure 'BoundBody' and any transient method produced during
// previous'BoundBody' interpretation up to this point.
// Make sure the (TransientSymbolStore::) StartMethod() and EndMethod() are used correctly to set
// the TransientSymbolStore::InMethodSymbolIterator.
#define VBLoweringTemporaryPrefix L"LW"

void
Semantics::LowerBoundTreesForMethod(ILTree::ProcedureBlock *BoundBody, TransientSymbolStore *CreatedDuringInterpretation)
{
    if (BoundBody &&
        !IsBad(BoundBody) &&
        !m_IsGeneratingXML)
    {
        LowerBoundTree(BoundBody);

        // This is post interpretation step in InterpretMethodBody. This method walks the
        // bound tree and checks to see if any anonymous delegates are used and if so
        // registers them with the transient symbol store. This registering is done so
        // late during interpretation and not as soon as the anonymous delegate is created
        // in order to ensure that only the anonymous delegates that are used in the final
        // bound tree are emitted into the assembly.
        // For an exmaple, see bug Devdiv 78381.
        //
        RegisterRequiredAnonymousDelegates(BoundBody);

        CLinkRangeIter<TransientSymbol> iter(m_SymbolsCreatedDuringInterpretation->MethodSymbolIterator());
        TransientSymbol *pTransientSymbol = NULL;

        // lower all the transient methods created during the interpretation of this method.
        while (pTransientSymbol = iter.Next())
        {
            AssertIfNull(pTransientSymbol->m_pTransient);

            if (pTransientSymbol->m_pTransient->IsProc() )
            {
                BackupValue<Procedure*> backupTransientSymbolOwnerProc(&m_TransientSymbolOwnerProc);
                m_TransientSymbolOwnerProc = m_Procedure;
                ILTree::ProcedureBlock *tBoundBody = pTransientSymbol->m_pTransient->PProc()->GetBoundTree();

                // Save and update the ProcedureState
                ProcedureState savedState;
                ProcedureState state(tBoundBody);
                this->GetProcedureState(&savedState);
                this->SetProcedureState(&state);
                LowerBoundTree(tBoundBody);

                tBoundBody = LowerResumableMethod(tBoundBody); // This is where we lower any resumable lambdas.
                pTransientSymbol->m_pTransient->PProc()->SetBoundTree(tBoundBody);
                // (as for resumable procedures, they are lowered in TransformMethod).

                RegisterRequiredAnonymousDelegates(tBoundBody);
                this->SetProcedureState(&savedState);

            }
        }

    }
}

void
Semantics::LowerBoundTree(_In_opt_ ILTree::ProcedureBlock *BoundBody)
{

    const WCHAR *OldPrefix = NULL;
    m_TemporaryManager->LockExistingShortLivedTempsForNewPass(VBLoweringTemporaryPrefix, OldPrefix);

    LowerBoundTreeVisitor lit(this);
    lit.Visit(BoundBody);

    m_TemporaryManager->UnlockExistingShortLivedTemps(OldPrefix);


}

// Builds and returns Operand.HasValue expression tree.
ILTree::Expression* Semantics::CreateNullableHasValueCall(Type *OperandType, ILTree::Expression *Operand)
{
    AssertIfNull(OperandType);
    AssertIfFalse(TypeHelpers::IsNullableType(OperandType, m_CompilerHost));
    AssertIfNull(Operand);

    Scope* Scope = ViewAsScope(OperandType->PClass());

    Declaration *HasValueProc = Scope->SimpleBind(STRING_CONST(m_Compiler, HasValue));
    AssertIfFalse(HasValueProc && HasValueProc->IsProperty());
    AssertIfFalse(TypeHelpers::IsBooleanType(HasValueProc->PProperty()->GetType()));

    ILTree::Expression *HasValueProcRef = ReferToSymbol
    (
        Operand->Loc,
        HasValueProc->PProperty()->GetProperty(),
        chType_NONE,
        NULL, // explicit call target (see following SX_CALL)
        OperandType->PGenericTypeBinding(),
        ExprIsExplicitCallTarget
    );

    SetFlag32(HasValueProcRef, SXF_SYM_NONVIRT); // HasValue is sealed

    ILTree::Expression *Result = AllocateExpression
    (
        SX_CALL,
        HasValueProc->PProperty()->GetType(),
        HasValueProcRef,
        Operand->Loc
    );

    Result->AsCallExpression().MeArgument = MakeAddress(Operand, true);

    return Result;
}

// Builds and returns Operand.op_Explicit expression tree.
ILTree::Expression* Semantics::CreateNullableExplicitOperatorCall(Type *OperandType, ILTree::Expression *Operand)
{
    AssertIfNull(OperandType);
    AssertIfFalse(TypeHelpers::IsNullableType(OperandType, m_CompilerHost));
    AssertIfNull(Operand);

    Declaration* member1 = OperandType->PClass()->GetUnBindableChildrenHash()->SimpleBind(
        STRING_CONST(m_Compiler, CType));
    Declaration* member2 = member1->GetNextOfSameName();

    // Use op_Explicit.

    Declaration* op_explicit = member1->PUserDefinedOperator()->GetOperator() == OperatorNarrow ?
        member1 : member2;

    AssertIfFalse( op_explicit->PUserDefinedOperator()->GetOperator() == OperatorNarrow );

    ILTree::Expression *HasValueProcRef = ReferToSymbol
    (
        Operand->Loc,
        op_explicit->PUserDefinedOperator()->GetOperatorMethod(),
        chType_NONE,
        NULL, // explicit call target (see following SX_CALL)
        OperandType->PGenericTypeBinding(),
        ExprIsExplicitCallTarget
    );

    SetFlag32(HasValueProcRef, SXF_SYM_NONVIRT); // HasValue is sealed

    ILTree::Expression *Result = AllocateExpression
    (
        SX_CALL,
        TypeHelpers::GetElementTypeOfNullable(OperandType, m_CompilerHost),
        HasValueProcRef,
        AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
            Operand,
            Operand->Loc),
        Operand->Loc
    );

    SetFlag32(Result, SXF_CALL_WAS_OPERATOR);
    Result->AsCallExpression().OperatorOpcode = SX_CTYPE;

    Result->AsCallExpression().MeArgument = MakeAddress(Operand, true);

    return Result;
}

// Builds and returns Operand.Value expression tree.
ILTree::Expression* Semantics::CreateNullableValueCall(Type *OperandType, ILTree::Expression *Operand)
{
    AssertIfNull(OperandType);
    AssertIfFalse(TypeHelpers::IsNullableType(OperandType, m_CompilerHost));
    AssertIfNull(Operand);

    Scope* Scope = ViewAsScope(OperandType->PClass());

    Declaration *ValueProc = Scope->SimpleBind(STRING_CONST(m_Compiler, GetValueOrDefault));
    AssertIfFalse(ValueProc && ValueProc->IsMethodDecl());

    ILTree::Expression *ValueProcRef = ReferToSymbol
    (
        Operand->Loc,
        ValueProc,
        chType_NONE,
        NULL, // explicit call target (see following SX_CALL)
        OperandType->PGenericTypeBinding(),
        ExprIsExplicitCallTarget
    );

    SetFlag32(ValueProcRef, SXF_SYM_NONVIRT); // GetValueOrDefault is sealed

    ILTree::Expression *Result = AllocateExpression
    (
        SX_CALL,
        TypeHelpers::GetElementTypeOfNullable(OperandType, m_CompilerHost),
        ValueProcRef,
        Operand->Loc
    );

    Result->AsCallExpression().MeArgument = MakeAddress(Operand, true);

    return Result;
}

// Builds and returns expression representing NULL value of specified Nullable type.
ILTree::Expression *Semantics::CreateNullValue(Type *ResultType, Location &Location)
{
    AssertIfNull(ResultType);
    AssertIfFalse(TypeHelpers::IsNullableType(ResultType, m_CompilerHost));

    Variable* Temporary = AllocateLifetimeNoneTemporary(ResultType, &Location);

    ILTree::Expression *TemporaryReference = AllocateSymbolReference
    (
        Temporary,
        ResultType,
        NULL, // no base for varible reference
        Location,
        ResultType->PGenericTypeBinding()
    );

    ILTree::Expression *Init = AllocateExpression
    (
        SX_INIT_STRUCTURE,
        TypeHelpers::GetVoidType(),
        Location
    );
    Init->AsInitStructureExpression().StructureType = ResultType;
    Init->AsInitStructureExpression().StructureReference = MakeAddress(TemporaryReference, true);

    return AllocateExpression
    (
        SX_SEQ_OP2,
        ResultType,
        Init,
        AllocateSymbolReference
        (
            Temporary,
            ResultType,
            NULL, // no base for varible reference
            Location,
            ResultType->PGenericTypeBinding()
        ),
        Location
    );
}

// Generates constructor call "New ResultType?(Value)".
ILTree::Expression *Semantics::WrapInNullable(Type *ResultType, ILTree::Expression *Value)
{
    AssertIfNull(ResultType);
    AssertIfFalse(TypeHelpers::IsNullableType(ResultType, m_CompilerHost));
    AssertIfNull(Value);

    return CreateConstructedInstance
    (
        ResultType,
        Value->Loc,
        Value->Loc,
        AllocateExpression
        (
            SX_LIST,
            TypeHelpers::GetVoidType(),
            AllocateExpression
            (
                SX_ARG,
                TypeHelpers::GetVoidType(),
                Value,
                Value->Loc
            ),
            Value->Loc
        ),
        false,
        ExprNoFlags
    );
}

// Creates and returns expression of form "New Nullable(Of Boolean)(Value)".
ILTree::Expression* Semantics::WrapInNullable(bool Value, Location &Location)
{
    if (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericNullableType))
    {
        return WrapInNullable
        (
            GetFXSymbolProvider()->GetNullableIntrinsicSymbol(t_bool),
            ProduceConstantExpression
            (
                Value ? COMPLUS_TRUE : COMPLUS_FALSE,
                Location,
                GetFXSymbolProvider()->GetBooleanType()
                IDE_ARG(0)
            )
        );
    }
    else
    {
        ReportMissingType(FX::GenericNullableType, Location);
        return AllocateBadExpression(Location);
    }
}

// Captures expression in an addressed temporary if it's of nullable type and in
// regular variable if it's not nullable. Returns capturing expression,
// a reference to the capturing variable and the variable itself.
void Semantics::CaptureNullableOperand
(
    _In_ ILTree::Expression *Operand,
    _Out_ ILTree::Expression *&Capture,
    _Out_ ILTree::Expression *&VariableReference,
    _Deref_out_opt_ Variable **CapturingVariable
)
{
    AssertIfNull(Operand);

    Type *ResultType = Operand->ResultType;
    bool IsNullable = TypeHelpers::IsNullableType(ResultType, m_CompilerHost);
    Location &Location = Operand->Loc;
    Variable *Variable = NULL;

    if (Operand->bilop == SX_SYM && Operand->AsSymbolReferenceExpression().Symbol->IsLocal())
    {
        Variable = Operand->AsSymbolReferenceExpression().Symbol->PVariable();
        Capture = IsNullable ?
            MakeAddress(Operand, true) :
            Operand;
    }
    else
    {
        Capture = IsNullable ?
            CaptureInAddressedTemporary(Operand, ResultType, Variable):
            CaptureInShortLivedTemporary(Operand, Variable);
    }

    if (Variable && CapturingVariable)
    {
        *CapturingVariable = Variable;
    }

    VariableReference = AllocateSymbolReference
    (
        Variable,
        ResultType,
        NULL,
        Location
    );
    SetFlag32(VariableReference, SXF_LVALUE);
}

// Given an operand of a nullable type (System.Nullable(Of T)) builds condition and value expressions
// which can be used for IIF code-generation. Resulting expressions share same temp local storing the operand.
// Generated pseudo-code:
//   Dim temp As OperandType? = Operand
//   Dim Condition As Boolean = temp.HasValue
//   Dim Value As OperandType = temp.GetValueOrDefault()
//
// NOTE: The actual variable declaration is hidden inside Condition in the form of assignment, so the
// tree produced for Condition out paramter looks like this CALL(ASG(temp, Operand),HasValue).
// Code-generation places temp declaration just before the Condition expression is evaluated.
// If you have 2 conditions returned by this function, connecting them with SX_AND will produce
// explicit operand evaluation before the SX_AND and connecting with SX_ANDALSO will delay
// evaluation of the second argument until the result of the first one is processed.
void Semantics::ProcessNullableOperand(_In_ ILTree::Expression *Operand, _Inout_ ILTree::Expression *&Condition, _Out_ ILTree::Expression *&Value, bool returnOperandAsValue)
{
    AssertIfNull(Operand);
    Location &Location = Operand->Loc;

    Type* OperandType = Operand->ResultType;
    AssertIfFalse(TypeHelpers::IsNullableType(OperandType, m_CompilerHost));
    AssertIfNull(TypeHelpers::GetElementTypeOfNullable(OperandType, m_CompilerHost));

    Scope* Scope = ViewAsScope(OperandType->PClass());

    // Lift the operand into a temporary (Dim temp As OperandType? = Operand).
    ILTree::Expression *OperandAddress = MakeAddress(Operand, true);
    ILTree::Expression *OperandAddressRef = NULL;
    if (OperandAddress->bilop == SX_ASG_RESADR)
    {
        // If MakeAddress captured Operand in an addressed local, we need to get that local and reuse it,
        ILTree::Expression *Local = OperandAddress->AsBinaryExpression().Left;
        if (Local->bilop == SX_SYM &&
            Local->AsSymbolReferenceExpression().Symbol->IsVariable() &&
            Local->AsSymbolReferenceExpression().Symbol->PVariable()->IsTemporary())
        {
            if (returnOperandAsValue)
            {
                OperandAddressRef = Local;
            }
            else
            {
                OperandAddressRef = MakeAddress(Local, true);
            }

        }
        else
        {
            VSFAIL("Unexpected result of MakeAddress in OperandAddress, reusing OperandAddress (bad IL image may occur).");
            OperandAddressRef = OperandAddress;
        }
    }
    else
    {
        AssertIfFalse(OperandAddress->bilop == SX_ADR);
        if (returnOperandAsValue)
        {
            OperandAddressRef = OperandAddress->AsBinaryExpression().Left;
        }
        else
        {
            OperandAddressRef = OperandAddress;
        }

    }
    AssertIfNull(OperandAddressRef);

    // Generate condition expression (temp.HasValue).
    ILTree::Expression *HasValueCall = CreateNullableHasValueCall(OperandType, OperandAddress);

    // If Condition was supplied, AND it with the HasValueCall.
    Condition = Condition ?
        AllocateExpression(
            SX_AND,
            GetFXSymbolProvider()->GetBooleanType(),
            Condition,
            HasValueCall,
            Location) :
        HasValueCall;

    ILTree::Expression *ValueExpr = NULL;

    // the value can be the 'Operand' itself or 'Operand.GetValueOrDefault()'
    if (returnOperandAsValue)
    {
        ValueExpr = OperandAddressRef;
    }
    else
    {
        // Generate value expression (temp.GetValueOrDefault()).
        ValueExpr = CreateNullableValueCall(OperandType, OperandAddressRef);
    }

    // Do this last since it might change the Operand.
    Value = ValueExpr;
}


// Given a SX_IIFCoalesce node, build the elements for the equivalent IIF. IIF(Condition, TrueExpr, FalseExpr)
void Semantics::LoadIIfElementsFromIIfCoalesce
(
    ILTree::Expression *expr,       // in: SX_IIFCoalesce node IF(X,Y) .
    ILTree::Expression *&Condition, // out: Condition - based on first argument. Can be be temp(=X), 'temp IsNot Nothing', 'temp.hasValue'
    _Out_ ILTree::Expression *&TrueExpr,  // out: TrueExpr - can be Conv(temp), Conv(temp->GetValueOrDefault)
    _Out_ ILTree::Expression *&FalseExpr, // out: FalseExpr - is Conv(Y)
    bool fPreserveCondition  // in:
                            // false - the first operand in expr is evaluated once possibly via a temp. The temp is used in Condition and TrueExpr.
                            //          Used in lowering for preparing the tree for code gen.
                            // true  - the first operand in expr is returned as the Condition. Also, it is shared in TrueExpr. Necesary conversions
                            //          are built on top of X and Y. Used for Expression Trees.
)
{
    // coalesce IIF(x,y) is equivalent to IIF(x ISNOT Nothing, x, y)
    // what we get at this stage is: IIF(x1, y1). The result type is the final correct type, yet the conversion
    // logic must be redone.
    // The translation is as follows:
    //   Notations:
    //      TX  - type of X
    //      TY - type of Y
    //      TXE - the type of X.Value() when X is nullable
    //      TYE - the type of Y.Value() when Y is nullable
    //
    // !!! WARNING: The following table is no longer up-to-date. There is another possibility,
    // that the target type (expr->ResultType) is Object, while neither of the expressions
    // is Object. The code has been updated to reflect this. But the table and the code
    // are gnarly. I didn't understand the table comprehensively enough to know whether
    // it should be deleted entirely. This entire function would benefit from a rewrite,
    // plus an explanation of what "fPreserveCondition" is for, plus a specification of
    // what's required of it.
    //
    //
    // 1. if X is reference type
    //      a. conversion on Y (if TY widens to TX)
    //              temp = X
    //              = IIF(temp ISNOT Nothing, temp, convert(Y, TX))
    //
    //      b. conversion on X(if TX widens to TY)
    //              temp  = X
    //              = IIF(temp ISNOT Nothing, convert(temp, TY), Y)
    //
    //      c. X and Y have same type
    //              temp = X
    //              = IIF(temp ISNOT Nothing, X, Y)
    //
    // 2. if X is a nullable type
    //  2.1 - conversion on Y
    //      a. if (Y is nullable) and (TY widens to TX)
    //              temp = X
    //              = IIF(temp.HasValue, temp, convert(Y, TX))
    //
    //      b. if (Y is not nullable) and (TY widens to TXE)        // erase ?
    //              temp = X
    //              = IIF(temp.HasValue, temp.Value, convert(Y, TXE))
    //
    //  2.2 - conversion on X
    //      a. if (Y is nullable) and (TX widens to TY).
    //         The conversion of X to TY can introduce an extra null check. Case a1 is to avoid this
    //
    //          a1. TX widens to TY via a lifted user defined conversion or predefined conversion.
    //              temp = X
    //              = IIF(temp.HasValue, convert(convert(temp.Value, TYE), TY), Y)
    //
    //          a2. TX widens to TY via a specific nullable user defined conversion.
    //              temp = X
    //              = IIF(temp.HasValue, convert(temp, TY), Y)
    //
    //      b. if (Y is not nullable) and (TXE widens to TY)
    //              temp = X
    //              = IIF(temp.HasValue, convert(temp.Value, TY), Y)
    //
    //  2.3 - X and Y have same type
    //              temp = X
    //              = IIF(temp.HasValue, temp, Y)
    //
    //  2.4 - TXE = TY , X is nullable, X.Value has the same type as Y ( i.e  IIF(S?, S)
    //              temp = X
    //              = IIF(temp.HasValue, temp.value, Y)

    TrueExpr = NULL;
    FalseExpr = expr->AsCoalesceExpression().Right;

    ILTree::Expression  *ConditionAndTrueExpr = expr->AsCoalesceExpression().Left;
    Type    *ConditionType = ConditionAndTrueExpr->ResultType;
    Type    *ElementConditionType = ConditionType;
    bool fNullableCondition = TypeHelpers::IsNullableType(ConditionType, GetCompilerHost());
    ExpressionFlags Flags = ExprForceRValue;

    Procedure *OperatorMethod = NULL;
    GenericBinding *OperatorMethodGenericContext = NULL;
    bool OperatorMethodIsLifted = false;
    bool fConversionOnFirstArg = false;
    bool fConversionOnSecondArg = false;

    bool fElementTypeUsedInCondition = false;
    if ( fNullableCondition && !TypeHelpers::IsNullableType(FalseExpr->ResultType, GetCompilerHost()))
    {
        // in IIF(x,y) if  x is nullable and y is not nullable, then drop '?' from x during conversion semantics
        fElementTypeUsedInCondition = true;
        ElementConditionType = TypeHelpers::GetElementTypeOfNullable(ConditionType, GetCompilerHost());
    }

    ConversionClass TrueConversion =
        ClassifyConversion(expr->ResultType, ElementConditionType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

    if (TrueConversion != ConversionIdentity)
    {
        // IIF(X,Y).  Conversion on X, X widens to type of Y.
        // Or, speciall semantic of ForcedLiftedCatenationIIFCoalesce which allows a narrowing here for nullable operands in catenation
        if (expr->ForcedLiftedCatenationIIFCoalesce)
        {
            Flags |= ExprHasExplicitCastSemantics;
        }
        fConversionOnFirstArg = true;
    }

    ConversionClass FalseConversion =
        ClassifyConversion(expr->ResultType, FalseExpr->ResultType, OperatorMethod, OperatorMethodGenericContext, OperatorMethodIsLifted);

    if (FalseConversion != ConversionIdentity )
    {
        // IIF(X,Y).  Conversion on Y, Y widens to type of X. Or, type of X.Value() when X is nullable and Y is not nullable.
        fConversionOnSecondArg = true;
    }

    // narrowing conversions are already thrown out while creating IIFCoalesce node.(except the catenaton case)
    VSASSERT ( (TrueConversion==ConversionIdentity && FalseConversion==ConversionIdentity) || fConversionOnFirstArg == true || fConversionOnSecondArg == true, "Bad conversion in lowering IIF coalesce");

    if(!fNullableCondition)
    {
        // x is of reference type


        AssertIfFalse(TypeHelpers::IsReferenceType(ConditionType));

        if (fPreserveCondition)
        {
            Condition = ConditionAndTrueExpr;
            TrueExpr = ConditionAndTrueExpr;
        }
        else
        {

            UseTwiceShortLived(ConditionAndTrueExpr, Condition, TrueExpr);
            Condition = ConvertWithErrorChecking(Condition, GetFXSymbolProvider()->GetObjectType(), ExprForceRValue);
            Condition = AllocateExpression(
                SX_ISNOT,
                GetFXSymbolProvider()->GetBooleanType(),
                Condition,
                AllocateExpression(
                    SX_NOTHING,
                    Condition->ResultType,
                    ConditionAndTrueExpr->Loc),
                ConditionAndTrueExpr->Loc);
        }

        if (fConversionOnSecondArg)
        {
            // 1. a. X reference type,  conversion on Y (TY widens to TX)
            FalseExpr = ConvertWithErrorChecking(FalseExpr, expr->ResultType, Flags);
        }
        if(fConversionOnFirstArg)
        {
            // 1. b. X reference type,  conversion on X (TX widens to TY)
            TrueExpr = ConvertWithErrorChecking(TrueExpr, expr->ResultType, Flags);
        }

    }
    else
    {
        // X is of nullable type

        // When building the 'TrueExpr' part of final IIF:
        //      Cases: 2.1.a, 2.2.a2, 2.3 use (temp=)X
        //      Cases(others): 2.1.b, 2.2.a.a1, 2.2.b, 2.4 use (temp=)X.Value
        bool fUseOperandAsValue =   (fConversionOnSecondArg && !fElementTypeUsedInCondition) ||
            (fConversionOnFirstArg && !fElementTypeUsedInCondition && OperatorMethod && !OperatorMethodIsLifted) ||
            (!fConversionOnSecondArg && !fConversionOnFirstArg && !fElementTypeUsedInCondition);

        // consistency check with the cases above. The assert bellow shoud fail if new cases are introduced without being
        // addressed properly.
        VSASSERT( fUseOperandAsValue ||
            (
            (fConversionOnSecondArg && fElementTypeUsedInCondition ) ||
            (fConversionOnFirstArg && !fElementTypeUsedInCondition && (NULL == OperatorMethod || OperatorMethodIsLifted )) ||
            (fConversionOnFirstArg && fElementTypeUsedInCondition ) ||
            (!fConversionOnSecondArg && !fConversionOnFirstArg && fElementTypeUsedInCondition)
            ),
            "Flags not consistent in lowering Coalesce IIF"
            );

        // prepare the condition and (partially) the TrueExpr
        if (fPreserveCondition)
        {
            Condition = ConditionAndTrueExpr;
            TrueExpr = ConditionAndTrueExpr;
        }
        else
        {
            ProcessNullableOperand(ConditionAndTrueExpr, Condition, TrueExpr, fUseOperandAsValue);
        }

        ILTree::Expression * originalFalseExpr = FalseExpr; //Preserve original expression before we convert it.
        
        if (fConversionOnSecondArg)
        {
            //2.1 - conversion on Y
            //(2.1.a. conversion on Y, Y is nullable) and
            //(2.1.b. conversion on Y, Y is not nullable)
            // TrueExpr is (temp=)X or (temp=)X.Value, accordingly.

            AssertIfFalse(
                (!fElementTypeUsedInCondition && BCSYM::AreTypesEqual(TrueExpr->ResultType, ConditionType) ) ||
                ( fElementTypeUsedInCondition &&
                BCSYM::AreTypesEqual(TrueExpr->ResultType, (fPreserveCondition ? ConditionType : TypeHelpers::GetElementTypeOfNullable(ConditionType, GetCompilerHost())))
                ));


            AssertIfFalse(
                fConversionOnFirstArg ||
                BCSYM::AreTypesEqual(
                    !fPreserveCondition ? expr->ResultType : TypeHelpers::GetElementTypeOfNullable(expr->ResultType, GetCompilerHost()),
                    !fPreserveCondition ? TrueExpr->ResultType : TypeHelpers::GetElementTypeOfNullable(TrueExpr->ResultType, GetCompilerHost())));
            FalseExpr = ConvertWithErrorChecking(FalseExpr, expr->ResultType, Flags);
        }
        
        if (fConversionOnFirstArg)
        {
            //  2.2 - conversion on X , all cases
            // TrueExpr is (temp=)X or (temp=)X.Value, accordingly.
            // case 2.2.a.a1 case has an initial conversion of TrueExpr to type of Y.Value  (TYE)
            // on top of 2.2.a.a1 and for all the other cases, a final conversion is necessary from TrueExpr to type of Y (TY)
            if ( !fElementTypeUsedInCondition && (NULL == OperatorMethod || OperatorMethodIsLifted ))
            {
                // case 2.2.a.a1. Make first TrueExpr = convert(temp.Value, TYE)
                AssertIfFalse(TypeHelpers::IsNullableType(originalFalseExpr->ResultType, GetCompilerHost()));
                AssertIfFalse(
                    BCSYM::AreTypesEqual(
                        fPreserveCondition ?
                            TypeHelpers::GetElementTypeOfNullable(TrueExpr->ResultType, GetCompilerHost()) :
                            TrueExpr->ResultType,
                        TypeHelpers::GetElementTypeOfNullable(ConditionType, GetCompilerHost())) ||
                    FalseExpr->ResultType->IsObject()); // #531876 We may be converting both sides to Object, the types don't have to match in this case.

                // In expression trees case TrueExpr is the first op., X. (Otherwise TrueExpr is  X.Value)
                // The original conversion classification was based on TX widdens to TY. In the case of a lifted user defined
                // conversion the conversion TX to TYE would fail. Skip the inner converssion for the expression trees case
                // n explicit case of failure is S? widens to Long? via a lifted user defined conv. IIF(S?, double?)
                if ( !fPreserveCondition)
                {
                    TrueExpr = ConvertWithErrorChecking(
                    TrueExpr,
                    TypeHelpers::GetElementTypeOfNullable(FalseExpr->ResultType, GetCompilerHost()),
                    Flags);
                }
            }
            else if (fElementTypeUsedInCondition && fPreserveCondition)
            {
                // case 2.2.b special for Expression Trees
                // In expression trees case TrueExpr is the first op., X. (Otherwise TrueExpr is  X.Value)
                // X is nullable type and the conversion TX -> TY might fail because TY is not nullable.
                // An explicit case of failure is S? widens to Long? via a lifted user defined conv. IIF(S?, double)
                // The result of the whole expression is TY - not nullable, ET wont lift anything in this case, so
                // insert an inner conversion to the element type of first arg.

                // Do this only if we need,

                bool EmitEraseCast = true;

                if( OperatorMethod != NULL )
                {
                    Parameter* CurrentParameter = OperatorMethod->GetFirstParam();

                    if( TypeHelpers::IsNullableType( CurrentParameter->GetType(), GetCompilerHost() ) &&
                        TypeHelpers::IsNullableType( TrueExpr->ResultType, GetCompilerHost() ) )
                    {
                        EmitEraseCast = false;
                    }
                }

                if( EmitEraseCast )
                {
                    TrueExpr = CreateNullableExplicitOperatorCall(TrueExpr->ResultType, TrueExpr);
                }
            }
            AssertIfFalse(BCSYM::AreTypesEqual(expr->ResultType, FalseExpr->ResultType));
            TrueExpr = ConvertWithErrorChecking(TrueExpr, expr->ResultType, Flags);
        }

    }

    VSASSERT( (fPreserveCondition || Condition->vtype == t_bool) &&
        (IsBad(TrueExpr) || IsBad(FalseExpr) ||
        BCSYM::AreTypesEqual(
            fPreserveCondition ? TypeHelpers::GetElementTypeOfNullable(TrueExpr->ResultType, GetCompilerHost()) : TrueExpr->ResultType,
            fPreserveCondition ? TypeHelpers::GetElementTypeOfNullable(FalseExpr->ResultType, GetCompilerHost()) : FalseExpr->ResultType) ) &&
        (IsBad(FalseExpr) || BCSYM::AreTypesEqual(FalseExpr->ResultType, expr->ResultType)),
        "Bad types in while building IIF");

}

ILTree::Expression*
Semantics::LowerCType(
    ILTree::Expression* expr
    )
{
    ILTree::Expression *ptreeSource = expr->AsBinaryExpression().Left;
    Type *TargetType = expr->ResultType;
    Type *SourceType = ptreeSource->ResultType;

    bool TargetIsNullable = TypeHelpers::IsNullableType(TargetType, m_CompilerHost); //Nullable( Of T) or Null reference T
    bool SourceIsNullable = TypeHelpers::IsNullableType(SourceType, m_CompilerHost);

    //aply nullable conversion only if no other predefined conversions exist.
    if((TargetIsNullable || SourceIsNullable) &&
        (ConversionError == ClassifyPredefinedConversion(TargetType, SourceType, false, NULL, NULL, true)))
    {

        Type    *ElementTargetType = TypeHelpers::GetElementTypeOfNullable(TargetType, m_CompilerHost);
        Type    *ElementSourceType = TypeHelpers::GetElementTypeOfNullable(SourceType, m_CompilerHost);

        // get the op_Implicit() and op_Explicit() for TargetType if target is nullable. Otherwise pick the ops from source
        // which must be nullable
        BCSYM_MethodDecl *ConvOperatorWiden;
        BCSYM_MethodDecl *ConvOperatorNarrow;
        Type *NullableType = TargetIsNullable ? TargetType : SourceType;
        GenericTypeBinding *GenericBindingContext = (NullableType->IsGenericTypeBinding() ? NullableType->PGenericTypeBinding() : NULL);

        Declaration *Member1 = NullableType->PClass()->GetUnBindableChildrenHash()->SimpleBind(STRING_CONST(m_Compiler, CType));
        Declaration *Member2 = Member1->GetNextOfSameName();
        VSASSERT( IsUserDefinedOperator(Member1) && IsUserDefinedOperator(Member2) &&
            (Member1->PUserDefinedOperator()->GetOperator() == OperatorNarrow || Member1->PUserDefinedOperator()->GetOperator() == OperatorWiden) &&
            (Member2->PUserDefinedOperator()->GetOperator() == OperatorNarrow || Member2->PUserDefinedOperator()->GetOperator() == OperatorWiden),
            "Bad read of CType operator from Nullable");
        if (Member1->PUserDefinedOperator()->GetOperator() == OperatorNarrow)
        {
            ConvOperatorNarrow = Member1->PUserDefinedOperator()->GetOperatorMethod();
            ConvOperatorWiden = Member2->PUserDefinedOperator()->GetOperatorMethod();
        }
        else
        {
            ConvOperatorNarrow = Member2->PUserDefinedOperator()->GetOperatorMethod();
            ConvOperatorWiden = Member1->PUserDefinedOperator()->GetOperatorMethod();

        }
        BCSYM_MethodDecl *ConvOperator;

        //get S->T conversion classification. It is going to be used for all cases.
        ConversionClass ElementConversionClassification =
            ClassifyPredefinedConversion(
                    ElementTargetType,
                    ElementSourceType);
        VSASSERT(ConversionError != ElementConversionClassification, "Bad Nullable conversion semantics");


        if (ElementTargetType == ElementSourceType)
        {
            // conversion (T to T?) or (T? to T).
            // the identity cases (T to T) and (T? to T?) do not get a Ctype, do not show up here

            if ( TargetIsNullable )
            {
                // widening conversion from T to T?
                // translate into call to T?::op_Implicit(T) -> T?
                ConvOperator = ConvOperatorWiden;
            }
            else
            {
                //narrowing conversion from T? to T
                //translates into call to T?::op_Explicit(T?)  -> T
                ConvOperator =  ConvOperatorNarrow;
            }
            ILTree::Expression *MethodReference =
                ReferToSymbol(
                    ptreeSource->Loc,
                    ConvOperator,
                    chType_NONE,
                    NULL,
                    GenericBindingContext,
                    ExprIsExplicitCallTarget);
            SetFlag32(MethodReference, SXF_SYM_NONVIRT);  // All operators are Shared.

            ILTree::Expression *CallResult =
                AllocateExpression(
                        SX_CALL,
                        TargetType,
                        MethodReference,
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            ptreeSource,
                            ptreeSource->Loc),
                        ptreeSource->Loc);

            SetFlag32(CallResult, SXF_CALL_WAS_OPERATOR);
            CallResult->AsCallExpression().OperatorOpcode = SX_CTYPE;
            return( CallResult );

        }

        // ElementTargetType != ElementSourceType
        // (S? to T?) or (S to T?) or (S? to T). Obviously (S to T) does not show up here

        if ( TargetIsNullable ) // case (S? to T?) or (S to T?)
        {
            // if (S converts(widening/narrowing) to T)  then (S? converts(widening/narrowing) to T?)
            // also, (S converts(widdening/narrowing) to T?)

            if ( !SourceIsNullable)
            {
                // case ( S to T?). Translate into T?::op_Implicit(Convert_to_T(S)) -> T?

                //get S->T conversion:
                ILTree::Expression* ElementConversion = Convert(ptreeSource, ElementTargetType, ExprNoFlags, ElementConversionClassification);

                 //get the reference to T?::op_Implicit() - involved in both cases
                ILTree::Expression *MethodConversionReference =
                    ReferToSymbol(
                        ptreeSource->Loc,
                        ConvOperatorWiden,  // the op_Implicit from T?
                        chType_NONE,
                        NULL,
                        GenericBindingContext,
                        ExprIsExplicitCallTarget);
                SetFlag32(MethodConversionReference, SXF_SYM_NONVIRT);  // All operators are Shared.

                ILTree::Expression *CallResult =
                    AllocateExpression(
                            SX_CALL,
                            TargetType,
                            MethodConversionReference,
                            AllocateExpression(
                                SX_LIST,
                                TypeHelpers::GetVoidType(),
                                ElementConversion,
                                ptreeSource->Loc),
                            ptreeSource->Loc);

                SetFlag32(CallResult, SXF_CALL_WAS_OPERATOR);
                CallResult->AsCallExpression().OperatorOpcode = SX_CTYPE;
                return( CallResult );
            }
            else
            {
                // case (S? to T?)
                // translate into:
                //( if S?.HasValue() then T?::op_Implicit(Convert_to_T(S?.Value()) else New T?() ) -> T?
                // take care to evaluate S only once (due to side efect; use temps)

                ILTree::Expression *Condition = NULL, *Value = NULL;
                ProcessNullableOperand(ptreeSource, Condition, Value);

                ILTree::Expression* ElementConversion = Convert(
                    Value,
                    ElementTargetType,
                    ExprNoFlags,
                    ElementConversionClassification);

                return( AllocateIIfExpression(
                    TargetType,
                    Condition,
                    WrapInNullable(TargetType, ElementConversion),
                    CreateNullValue(TargetType, expr->Loc),
                    expr->Loc
                    ));
            }
        }
        else // case(S?, T)
        {
            // S? converts narrowing to T. If there is a predefined conversion T to S
            // 




            // since !TargetIsNullable ConvOperatorNarrow(op_Explicit) is the one from S?. Same with GenericBindingContext
            // ConvOperator =  ConvOperatorNarrow;

            ILTree::Expression *MethodConversionReference =
                ReferToSymbol(
                    ptreeSource->Loc,
                    ConvOperatorNarrow,
                    chType_NONE,
                    NULL,
                    GenericBindingContext,
                    ExprIsExplicitCallTarget);
            SetFlag32(MethodConversionReference, SXF_SYM_NONVIRT);  // All operators are Shared.

            ILTree::Expression *CallResult =
                AllocateExpression(
                        SX_CALL,
                        ElementSourceType,
                        MethodConversionReference,
                        AllocateExpression(
                            SX_LIST,
                            TypeHelpers::GetVoidType(),
                            ptreeSource,
                            ptreeSource->Loc),
                        ptreeSource->Loc);

            SetFlag32(CallResult, SXF_CALL_WAS_OPERATOR);
            CallResult->AsCallExpression().OperatorOpcode = SX_CTYPE;
            ILTree::Expression* Conversion = Convert(CallResult, TargetType, ExprNoFlags, ElementConversionClassification);
            return( Conversion );
        }
    }

    return( expr );
}

// This is post interpretation step in InterpretMethodBody. This method walks the
// bound tree and checks to see if any anonymous delegates are used and if so
// registers them with the transient symbol store. This registering is done so
// late during interpretation and not as soon as the anonymous delegate is created
// in order to ensure that only the anonymous delegates that are used in the final
// bound tree are emitted into the assembly.
// For an exmaple, see bug Devdiv 78381.
//
void Semantics::RegisterRequiredAnonymousDelegates
(
    ILTree::ProcedureBlock *OuterMostProcedureBoundTree
)
{
    if (!m_SymbolsCreatedDuringInterpretation ||
        !OuterMostProcedureBoundTree ||
        IsBad(OuterMostProcedureBoundTree) ||                           // errors in the bound tree
        m_IsGeneratingXML ||                                            // uncomplete bound tree for XML generaton
        !m_Procedure ||                                                 // guard for the next checks, m_Procedure can be null.
        m_Procedure->GetPWellKnownAttrVals()->GetDllImportData() ||     //DLL imports
        m_Procedure->IsMustOverrideKeywordUsed())                       // must override (abstract) methods
    {
        return;
    }

    AnonymousDelegatesRegisterVisitor TypeIter(m_Procedure, m_SymbolsCreatedDuringInterpretation);
    TypeIter.Visit(OuterMostProcedureBoundTree);
}

Scope *
Semantics::GetMergedHashTable
(
    BCSYM_NamespaceRing *NamespaceRing
)
{
#if HOSTED
    if (NamespaceRing->GetContainsHostedDynamicHash())
    {
        return NULL;
    }
#endif

    if (m_MergedNamespaceCache && !m_DoNotMergeNamespaceCaches)
    {
        NamespaceRingNode *pNamespaceRingNode = NULL;
        NamespaceRingKey key;
        key.InitKey(GetCompilerHost(), NamespaceRing);

        m_MergedNamespaceCache->Insert(&key, &pNamespaceRingNode);

        if (!pNamespaceRingNode->HasBeenMerged)
        {
            pNamespaceRingNode->MergedHash = CreateMergedHashTable(NamespaceRing, m_MergedNamespaceCache->GetAllocator());
            pNamespaceRingNode->HasBeenMerged = true;
        }

        return pNamespaceRingNode->MergedHash;
    }

#if IDE
    VSASSERT(m_DoNotMergeNamespaceCaches ||  !GetCompilerSharedState()->IsInMainThread(),"Main thread calling semantics with no compilation cache: opportunity for optimization");
#endif IDE
    return NULL;
}

Scope *
Semantics::CreateMergedHashTable
(
    BCSYM_NamespaceRing *NamespaceRing,
    NorlsAllocator *pNorlsAllocator
)
{
    BCSYM_Namespace* pFirstNamespace = NamespaceRing->GetFirstNamespace();
    BCSYM_Namespace* pNamespace = pFirstNamespace;

    // calculate the total number of symbols already existed under BCSYM_Namespace's.
    unsigned int TotalNoOfSymbols = 0;

    do
    {
        // DDB 124514.
        // If ever a project that we need is not in declared, just skip over them.

        if( pNamespace->GetCompilerFile()->GetProject() != NULL &&
            pNamespace->GetCompilerFile()->GetProject()->GetCompState() >= CS_Declared )
        {
            VSASSERT( pNamespace->GetCompilerFile()->GetCompState() >= CS_Declared, "woah, adding namespace in nostate" );

            if (pNamespace->GetCompilerFile()->GetCompilerHost() == GetCompilerHost())
            {
                TotalNoOfSymbols += pNamespace->GetHash()->CSymbols();
            }
        }

        pNamespace = pNamespace->GetNextNamespace();

    } while (pNamespace != pFirstNamespace);

    // select a suitable prime number or simply TotalNoOfSymbols for TotalNoOfBuckets
    unsigned int TotalNoOfBucketsNeeded = 0;
    if (TotalNoOfSymbols *2 <= 17)
    {
        TotalNoOfBucketsNeeded = 17;
    }
    else if (TotalNoOfSymbols *2 <= 31)
    {
        TotalNoOfBucketsNeeded = 31;
    }
    else if (TotalNoOfSymbols *2 <= 67)
    {
        TotalNoOfBucketsNeeded = 67;
    }
    else if (TotalNoOfSymbols *2 <= 127)
    {
        TotalNoOfBucketsNeeded = 127;
    }
    else if (TotalNoOfSymbols *2 <= 257)
    {
        TotalNoOfBucketsNeeded = 257;
    }
    else if (TotalNoOfSymbols *2 < 601)
    {
        TotalNoOfBucketsNeeded = 601;
    }
    else
    {
        // Microsoft note: memory footprint consideration...
        //
        // Should we cap the number of 'TotalNoOfBucketsNeeded' to some bound? It depends on
        // how total number of symbols as seen by the compiler is related to those that are visible under some namespaces.
        // If the percentage of total symbols visible under some namespaces is "large", then we probably should cap it
        // to some predefined prime number and live with long bucket chaining.
        //

        TotalNoOfBucketsNeeded = TotalNoOfSymbols;

        // comment out the following line if we want to cap the bucket size to a max prime, say 601.
        //TotalNoOfBucketsNeeded = 601;
    }

    Symbols SymbolCreator(
        GetCompiler(),
        pNorlsAllocator,
        NULL);

    // Allocate and initialize BCSYM_Hash object that represents a merged hash table.
    Scope *MergedHash = SymbolCreator.GetHashTable(NULL, NULL, false, TotalNoOfBucketsNeeded, NULL);

    if (MergedHash)
    {
#if DEBUG
#if IDE 
        LOG_INFO(
            VB_NAMESPACE_HASH,
            L"Start logging hash files..."
            );
#endif
#endif
        // Fill up the merged hash table by iterating through BCSYM_Namespace's.
        pNamespace = pFirstNamespace;
        do
        {
            // DDB 124514.
            // If ever a project that we need is not in declared, just skip over them.

            if( pNamespace->GetCompilerFile()->GetProject() != NULL &&
                pNamespace->GetCompilerFile()->GetProject()->GetCompState() >= CS_Declared )
            {
                if (pNamespace->GetCompilerFile()->GetCompilerHost() == GetCompilerHost())
                {

#if DEBUG
#if IDE 
                    LOG_INFO(
                        VB_NAMESPACE_HASH,
                        L"Hashing file: %S, project: %S.",
                        pNamespace->GetCompilerFile()->GetFileName(),
                        pNamespace->GetCompilerFile()->GetProject() != NULL ?
                            pNamespace->GetCompilerFile()->GetProject()->GetFileName() :
                            L"<No Project>"
                        );
#endif
#endif

                    if (!pNamespace->GetHash()->MergeHashTable(MergedHash, &SymbolCreator))
                    {
                        MergedHash = NULL;
                        break;
                    }
                }
            }

            pNamespace = pNamespace->GetNextNamespace();
        } while (pNamespace != pFirstNamespace);
    }

    return MergedHash;
}

bool
Semantics::IterateNamespaceRing
(
    BCSYM_NamespaceRing *NamespaceRing,
    RingIteratorType rit
)
{
    if (m_MergedNamespaceCache)
    {
        NamespaceRingNode *pNamespaceRingNode = NULL;
        NamespaceRingKey key;
        key.InitKey(GetCompilerHost(), NamespaceRing);

        m_MergedNamespaceCache->Insert(&key, &pNamespaceRingNode);

        switch (rit)
        {
            case ritExtensionMethods:
                if (!pNamespaceRingNode->IteratedForExtensionMethods)
                {
                    bool skippedUnavailableNamespaces = false;
                    pNamespaceRingNode->ContainsExtensionMethods = IterateNamespaceRingForEM(NamespaceRing, skippedUnavailableNamespaces);

                    if (skippedUnavailableNamespaces)
                    {
                        // Dev10 #570673 Since we skipped some namespaces and the caching is done on
                        // CompilerHost level, we can mark the ring as iterated only if found extension methods
                        // (ContainsExtensionMethods == true).
                        pNamespaceRingNode->IteratedForExtensionMethods = (pNamespaceRingNode->ContainsExtensionMethods == true);
                    }
                    else
                    {
                        pNamespaceRingNode->IteratedForExtensionMethods = true;
                    }
                        
                }

                return pNamespaceRingNode->ContainsExtensionMethods;

            case ritModules:
                if (!pNamespaceRingNode->IteratedForModules)
                {
                    pNamespaceRingNode->ContainsModules = IterateNamespaceRingForModules(NamespaceRing);
                    pNamespaceRingNode->IteratedForModules = true;
                }

                return pNamespaceRingNode->ContainsModules;
        }
    }

    // Safe Default
    return true;
}

bool
Semantics::IterateNamespaceRingForEM
(
    BCSYM_NamespaceRing *NamespaceRing,
    bool & skippedUnavailableNamespaces
)
{
    BCSYM_Namespace* pFirstNamespace = NamespaceRing->GetFirstNamespace();
    BCSYM_Namespace* pNamespace = pFirstNamespace;

    skippedUnavailableNamespaces =  false;
    
    do
    {
        // Only consider namespaces in the same compilerhost.
        if (GetCompilerHost() == pNamespace->GetCompilerFile()->GetCompilerHost())
        {
            // Dev10 #570673 Below is a comment copied from Semantics::LookupInModulesInNamespace(), 
            // we are hitting the same issue here.
            // Bug 437737. With the new compilation order for diamond references,
            // modules in metadata files that are not referenced by this project
            // either directly or indirectly cannot be loaded when binding this
            // project.
            //
            if (!(pNamespace->GetCompilerFile() &&
                  pNamespace->GetCompilerFile()->IsMetaDataFile() &&
                  pNamespace->GetCompilerFile()->GetCompState() < CS_Bound))
            {
                if (pNamespace->GetCompilerFile()->ContainsExtensionMethods())
                {
                    TypeExtensionIterator typeExtensionIterator(pNamespace);

                    if (typeExtensionIterator.MoveNext())
                    {
                        return true;
                    }
                }
            }
            else
            {
                AssertIfTrue(NamespaceIsAvailableToCurrentProject(pNamespace));
                skippedUnavailableNamespaces = true;
            }
        }

        pNamespace = pNamespace->GetNextNamespace();
    } while (pNamespace != pFirstNamespace);

    return false;
}

bool
Semantics::IterateNamespaceRingForModules
(
    BCSYM_NamespaceRing *NamespaceRing
)
{
    BCSYM_Namespace* pFirstNamespace = NamespaceRing->GetFirstNamespace();
    BCSYM_Namespace* pNamespace = pFirstNamespace;

    do
    {
        // Only consider namespaces in the same compilerhost.
        if (GetCompilerHost() == pNamespace->GetCompilerFile()->GetCompilerHost())
        {
            if (pNamespace->GetFirstModule())
            {
                return true;
            }
        }

        pNamespace = pNamespace->GetNextNamespace();
    } while (pNamespace != pFirstNamespace);

    return false;
}

bool
Semantics::DoesRingContainExtensionMethods
(
    BCSYM_NamespaceRing *NamespaceRing
)
{
    return IterateNamespaceRing(NamespaceRing, ritExtensionMethods);
}

bool
Semantics::DoesRingContainModules
(
    BCSYM_NamespaceRing *NamespaceRing
)
{
    return IterateNamespaceRing(NamespaceRing, ritModules);
}

Semantics::InitializerInferInfo::InitializerInferInfo(Semantics::InitializerInferInfo** parent) :
    BackupValue<Semantics::InitializerInferInfo*>(parent),
    Parent(*parent),
    CircularReferenceDetected(false),
    Variable(NULL)
{
    *parent = this;
}

    





void StatementListBuilder::Add(_In_ ILTree::Statement *statement)
{
    VSASSERT(statement!=NULL, "Why add a NULL statement?");
    if (statement == NULL) return;
    VSASSERT(statement->Next == NULL, "Use Add to add only a single statement (with stmt->Next==NULL). Use AddList to add several.");
    *m_ppLastStatement = statement;
    m_ppLastStatement = &statement->Next;
}

void StatementListBuilder::AddList(_In_opt_ ILTree::Statement *statements)
{
    if (statements == NULL) return;
    *m_ppLastStatement = statements;
    while ((*m_ppLastStatement) != NULL) m_ppLastStatement = &((*m_ppLastStatement)->Next);
}




//-------------------------------------------------------------------------------------------------
//
// Is this a symbol reference to Me where me has th MyBase or MyClass context
//
bool IsMyBaseMyClassSymbol(ILTree::Expression *expr)
{
    ThrowIfNull(expr);

    if ( SX_SYM != expr->bilop )
    {
        return false;
    }

    ILTree::SymbolReferenceExpression &symRef = expr->AsSymbolReferenceExpression();
    if ( symRef.Symbol
        && symRef.Symbol->IsVariable()
        && symRef.Symbol->PVariable()->IsMe()
        && symRef.uFlags & (SXF_SYM_MYBASE | SXF_SYM_MYCLASS) )
    {
        return true;
    }

    return false;
}



//-------------------------------------------------------------------------------------------------
//
// Some handy functions to make debug messages easier to write...
//
#if DEBUG
wchar_t *Semantics::DelegateRelaxationLevelToDebugString(DelegateRelaxationLevel level)
{
    switch (level)
    {
        case DelegateRelaxationLevelNone: return L"DelegateRelaxationLevelNone";
        case DelegateRelaxationLevelWidening: return L"DelegateRelaxationLevelWidening";
        case DelegateRelaxationLevelWideningDropReturnOrArgs: return L"DelegateRelaxationLevelWideningDropReturnOrArgs";
        case DelegateRelaxationLevelWideningToNonLambda: return L"DelegateRelaxationLevelWideningDropReturnOrArgs";
        case DelegateRelaxationLevelNarrowing: return L"DelegateRelaxationLevelNarrowing";
        case DelegateRelaxationLevelInvalid: return L"DelegateRelaxationLevelNarrowing";
        default: return L"DelegateRelaxationLevel???";
    }
}

wchar_t *Semantics::TypeInferenceLevelToDebugString(TypeInferenceLevel level)
{
    switch (level)
    {
        case TypeInferenceLevelNone: return L"TypeInferenceLevelNone";
        case TypeInferenceLevelOrcas: return L"TypeInferenceLevelOrcas";
        case TypeInferenceLevelInvalid: return L"TypeInferenceLevelInvalid";
        default: return L"TypeInferenceLevel???";
    }
}

wchar_t *Semantics::CandidateToDebugString(_In_ Declaration *Candidate, bool showDetail)
{
    if (Candidate == NULL)
    {
        return L"L<NULL>";
    }
    if (Candidate->GetLocation() == NULL)
    {
        return L"Lsynth";
    }
    StringBuffer buf;
    if (showDetail)
    {
        StringBuffer temp;
        wchar_t *pDetail = ExtractErrorName(Candidate, temp);
        buf.AppendPrintf(L"L%li:%s", Candidate->GetLocation()->m_lBegLine, pDetail);
    }
    else
    {
        buf.AppendPrintf(L"L%li", Candidate->GetLocation()->m_lBegLine);
    }
    STRING *Result = GetCompiler()->AddString(&buf);
    return Result;
}

wchar_t *Semantics::CandidateToDebugString(_In_ OverloadList *Candidate, bool showDetail)
{
    if (Candidate == NULL)
    {
        return L"L<NULL>";
    }
    return CandidateToDebugString(Candidate->Candidate, showDetail);
}

wchar_t *Semantics::TypeToDebugString(_In_ Type *Type)
{
    if (Type == NULL)
    {
        return L"<TNull>";
    }
    StringBuffer buf1, buf2;
    wchar_t *pTypeName = ExtractErrorName(Type, buf1);
    buf2.AppendPrintf(L"%s", pTypeName);
    STRING *Result = GetCompiler()->AddString(&buf2);
    return Result;
}
#endif

void
Semantics::CheckSecurityCriticalAttributeOnParentOfResumable
(
    BCSYM_Proc* pResumableProc
)
{
       VSASSERT(pResumableProc->IsAsyncKeywordUsed() || pResumableProc->IsIteratorKeywordUsed(), "Expect the proc to be async or iterator");
    
       // Check parent
       BCSYM_NamedRoot *pParent = pResumableProc->GetParent();
       bool foundError = false;
      
       while (!foundError && pParent != NULL && (pParent->IsClass() || pParent->IsInterface() || pParent->IsStruct()))
       {       

           if (pParent->GetPWellKnownAttrVals()->GetSecurityCriticalData())
           {           
               ReportSemanticError(
                    ERRID_SecurityCriticalAsyncInClassOrStruct,
                    *(pResumableProc->GetLocation()),
                    L"SecurityCritical");

               foundError = true;
           }
       
           if (pParent->GetPWellKnownAttrVals()->GetSecuritySafeCriticalData() )
           {               
               ReportSemanticError(
                    ERRID_SecurityCriticalAsyncInClassOrStruct,
                    *(pResumableProc->GetLocation()),
                    L"SecuritySafeCritical");   
               foundError = true;
           }
           
           pParent = pParent->GetParent();
       }
}
