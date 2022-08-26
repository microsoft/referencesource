//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

#define CTOR_CODE_BACK_OFFSET 0  // Anonymous Type .ctor code injection offset (from the end of function)
#define EQUALS_CODE_BACK_OFFSET 10  // Anonymous Type ToString code injection offset (from the end of function)
#define GETHASHCODE_CODE_BACK_OFFSET 0  // Anonymous Type GetHashCode code injection offset (from the end of function)
#define TOSTRING_CODE_BACK_OFFSET 40 // Anonymous Type ToString code injection offset (from the end of function)

#define CTOR_LENGTH_PER_FIELD 71 // Estimate length of per field synthetic code for Anonymous Type .ctor
#define EQUALS_LENGTH_PER_FIELD 322 // Estimate length of per field synthetic code for Anonymous Type Equals
#define GETHASHCODE_LENGTH_PER_FIELD 71 // Estimate length of per field synthetic code for Anonymous Type GetHashCode
#define TOSTRING_LENGTH_PER_FIELD 74    // Estimate length of per field synthetic code for Anonymous Type ToString

ILTree::Expression *
Semantics::InitializeAnonymousType
(
    ParseTree::BracedInitializerList *BracedInitializerList,
    bool NoWithScope,
    bool QueryErrorMode,
    BCSYM_Namespace *ContainingNamespace,
    BCSYM_Class *ContainingClass,
    Location &CreatingLocation,
    Location &TextSpan,
    ExpressionFlags Flags
)
{
    // Bugs: Devdiv - 39975, 39950, 42444.
    // The following assert is wrong. It is possible for m_Procedure to be NULL when processing
    // anonymous type expressions. The cases where this is possible are when the expression is
    // outside a method body eg: field initializers, {optional parameter expressions,  attribute
    // arguments (although only constants are allowed in these cases, in UI scenarios, the expressions
    // might be bound in this context to provide certain IDE services)}.
    //
    // VSASSERT(m_Procedure != NULL, "How can we initialize anonymous type without a procedure context?");

    // We create a binding table here that will be used for binding fields and mapping them to temporaries.

    AnonymousTypeBindingTable BindingTable( m_TemporaryManager );

    if (Flags & ExprMustBeConstant)
    {
        ReportSemanticError(ERRID_RequiredConstExpr, TextSpan);
        return AllocateBadExpression(TextSpan);
    }

    bool IsInitializerValid = false;

    NorlsAllocator Scratch(NORLSLOC);
    ULONG FieldCount = 0;
    MemberInfoList *MemberInfos = InitializeMemberInfoList(Scratch, BracedInitializerList, QueryErrorMode,&FieldCount);

    if (FieldCount > 0 && MemberInfos == NULL)
    {
        return AllocateBadExpression(TextSpan);
    }

    // Initialize the binding table with the number of fields that we have.

    BindingTable.SetInitialSize( FieldCount );

    TransientSymbolStore LocalTransients(m_Compiler, &Scratch);
    BackupValue<TransientSymbolStore *> SymbolsCreatedDuringInterpretatio_Backup(&m_SymbolsCreatedDuringInterpretation);
    if (!m_SymbolsCreatedDuringInterpretation)
    {
        m_SymbolsCreatedDuringInterpretation = &LocalTransients;    // Bug [30776] [31373] use a local transient symbols store for merging (binding)
    }

    ClassOrRecordType *AnonymousTypeTemplate =
        GetAnonymousTypeTemplate(
            ContainingNamespace,
            ContainingClass,
            MemberInfos,
            &BindingTable,
            FieldCount,
            QueryErrorMode);

    if (AnonymousTypeTemplate == NULL)
    {
        return AllocateBadExpression(TextSpan);
    }

    if (FieldCount == 0)
    {
        ReportSemanticError(ERRID_AnonymousTypeNeedField, TextSpan);
        return AllocateBadExpression(TextSpan);
    }

    GenericBinding *AnonymousTypeBinding =
        GetAnonymousTypeBinding(
            AnonymousTypeTemplate,
            TextSpan,
            MemberInfos,
            &BindingTable,
            FieldCount,
            NoWithScope, QueryErrorMode);

    if (AnonymousTypeBinding == NULL && FieldCount > 0)
    {
        return AllocateBadExpression(TextSpan);
    }

    ILTree::Expression *Result = NULL;
    if (Flags & ExprTypeInferenceOnly)
    {
        Result =
            AllocateExpression(
                SX_BOGUS,
                AnonymousTypeBinding,
                CreatingLocation);
    }
    else
    {
        Result =
            InitializeAnonymousType(
                AnonymousTypeBinding,
                MemberInfos,
                &BindingTable,
                FieldCount,
                NoWithScope,
                CreatingLocation,
                TextSpan,
                Flags);
    }

    return Result;
}

ILTree::Expression *
Semantics::InitializeAnonymousType
(
    GenericBinding *AnonymousTypeBinding,
    MemberInfoList *MemberInfos,
    AnonymousTypeBindingTable* BindingTable,
    ULONG FieldCount,
    bool NoWithScope,
    Location &CreatingLocation,
    Location &TextSpan,
    ExpressionFlags Flags
)
{
    if (!AnonymousTypeBinding || !MemberInfos || !AnonymousTypeBinding->IsGenericTypeBinding())
    {
        return AllocateBadExpression(TextSpan);
    }

#if DEBUG

    // Validate all initializers are resolved correctly before we reach this function

    for( MemberInfoList *Info = MemberInfos; Info != NULL ; Info = Info->Next )
    {
        VSASSERT(
            Info->InferredInitializer && !IsBad( Info->InferredInitializer ),
            "How did a bad initializer get here?"
            );
    }

#endif

    // Build the AsAnonymousTypeExpression node which will store all of our data.

    ILTree::AnonymousTypeExpression* Result = ( ILTree::AnonymousTypeExpression* )AllocateExpression(
        SX_ANONYMOUSTYPE,
        AnonymousTypeBinding,
        NULL,
        NULL,
        TextSpan
        );

    Result->BoundMembersCount = FieldCount;
    Result->BoundMembers = new( m_TreeStorage ) ILTree::BoundMemberInfoList[ FieldCount ];

    // We first need to be able to evaluate each expression, asking semantics to replace
    // any property references with the corresponding temporaries. To do this, we need to
    // setup a with context so that binding will succeed, but we should validate that
    // we do not refer to any symbol from the with context.

    BackupValue< ILTree::WithBlock* > backupWith( &m_EnclosingWith );

    // Create the temp for our with block.

    Variable *Temporary = AllocateShortLivedTemporary( AnonymousTypeBinding, &CreatingLocation );

    SetupAnonymousTypeWithEnclosure(
        Temporary,
        AllocateSymbolReference(
            Temporary,
            AnonymousTypeBinding,
            NULL,
            TextSpan
            ),
        NoWithScope
        );

    // Now, let's evaluate the expression, and mark that we want to replace properties
    // of this anonymous type with their temps.

    BackupValue< AnonymousTypeBindingTable* > backupTable( &m_AnonymousTypeBindingTable );
    m_AnonymousTypeBindingTable = BindingTable;

    BackupValue< AnonymousTypeBindingTable::BindingMode > backupMode( &( BindingTable->m_Mode ) );
    BindingTable->m_Mode = AnonymousTypeBindingTable::Replacing;

    BackupValue< Variable* > backupBase( &( BindingTable->m_BaseReference ) );
    BindingTable->m_BaseReference = Temporary;

    ULONG i = 0;

    for( MemberInfoList *Info = MemberInfos; Info != NULL ; i += 1, Info = Info->Next )
    {

        ParseTree::Expression* ParseTreeValue = NULL;

        if( Info->Initializer )
        {
            switch( Info->Initializer->Opcode )
            {

            case ParseTree::Initializer::Expression:
                ParseTreeValue = Info->Initializer->AsExpression()->Value;
                break;

            case ParseTree::Initializer::Assignment:
                ParseTreeValue = Info->Initializer->AsAssignment()->Initializer->AsExpression()->Value;
                break;

            default:
                VSFAIL("Surprising initializer");

            }
        }

        ILTree::Expression* Value = InterpretExpression( ParseTreeValue, ExprForceRValue );
        VSASSERT( !Value->IsScratch, "Why do we get back a scratch expression?" );

        if( Value == NULL || IsBad( Value ) || Value->bilop == SX_BOGUS )
        {
            ReportSemanticError(
                ERRID_ObjectInitializerBadValue,
                Value ? Value->Loc : ParseTreeValue->TextSpan
                );

            /* Dev10 694326 - Normally we don't get a bad initializer because we evaluate them earlier and if
               they are bad, we punt before getting to this function.  See: bool Semantics::TryInterpretBlock()
               and look for:    if (!Input->HasSyntaxError)
                                {
                                    InterpretVariableDeclarationStatement(Input->AsVariableDeclaration());
                                }

              But we can still get a bad initializer here. For instance, one situation that gets us a bad initializer is: if
              the IDE asks the background thread to abort while we are processing XML we return a bad expression to indicate
              that we didn't finish processing the XML.  See the if (CheckStop(this)) in Semantics::InterpretXmlElement()
              Now that's hard to hit but testing ran into it sporadically and it results in a crash because downstream
              the code expects that the BoundMembers[] array be filled out.  Better to mark the result as bad so we don't even
              try to process it later */
            MakeBad( Result ); 
        }

        // Store the data that we care about in the AnonymousTypeExpression.

        Result->BoundMembers[ i ].Property = BindingTable->GetRealProperty( i );
        Result->BoundMembers[ i ].Temp = BindingTable->GetTemp( i );
        // One way or another, make sure BoundMembers[i] is filled out.
        if ( Value )
        {
            Result->BoundMembers[ i ].BoundExpression = Value;
        }
        else // we hit an error processing the initializer
        {
            Result->BoundMembers[ i ].BoundExpression = AllocateBadExpression(ParseTreeValue->TextSpan);
        }

        VSASSERT( Result->BoundMembers[ i ].Property != NULL, "Why is the property NULL?" );
        VSASSERT( Result->BoundMembers[ i ].BoundExpression != NULL, "Why is the expression NULL?" );
    }

    // Restore the binding tables.

    backupTable.Restore();
    backupMode.Restore();
    backupBase.Restore();

    // Restore the with context.

    backupWith.Restore();

    // Now build the constructor call.

    BCSYM_Proc* Constructor = AnonymousTypeBinding->PClass()->GetFirstInstanceConstructor(
        m_Compiler
        );

    VSASSERT( Constructor != NULL, "Why can't we bind to our constructor?" );
    Result->Constructor = Constructor;

    // Save the binding.

    Result->AnonymousTypeBinding = AnonymousTypeBinding;

    return( Result );
}

MemberInfoList *
Semantics::InitializeMemberInfoList
(
    NorlsAllocator &rnra,
    ParseTree::BracedInitializerList *BracedInitializerList,
    bool QueryErrorMode,
    ULONG *MemberInfoCount
)
{
    MemberInfoList *MemberInfos = NULL;
    MemberInfoList **CurrentMemberInfo = &MemberInfos;

    bool AllInitializersValid = false;
    ULONG FieldCount = 0;
    if (BracedInitializerList)
    {
        AllInitializersValid = true;

        for (ParseTree::InitializerList *Initializers = BracedInitializerList->InitialValues;
             Initializers;
             Initializers = Initializers->Next)
        {
            ParseTree::Initializer *Operand = Initializers->Element;
            ParseTree::IdentifierDescriptor *PropertyName = NULL;
            Location OperandTextSpan = {0};
            bool IsNameBangQualified = false;
            bool IsXMLNameRejectedAsBadVBIdentifier = false;
            ParseTree::XmlNameExpression *XMLNameInferredFrom = NULL;
            bool NeedToReportError = true;

            if (Operand)
            {
                switch (Operand->Opcode)
                {
                    case ParseTree::Initializer::Expression:
                        PropertyName =
                            ExtractAnonTypeMemberName(
                                Operand->AsExpression()->Value,
                                IsNameBangQualified,
                                IsXMLNameRejectedAsBadVBIdentifier,
                                XMLNameInferredFrom);

                        OperandTextSpan = Operand->AsExpression()->Value->TextSpan;
                        NeedToReportError = (Operand->AsExpression()->Value->Opcode != ParseTree::Expression::SyntaxError);
                        break;

                    case ParseTree::Initializer::Assignment:
                        PropertyName = &Operand->AsAssignment()->Name;
                        OperandTextSpan = Operand->AsAssignment()->TextSpan;
                        break;

                    default:
                        VSFAIL("Surprising initializer");
                }
            }

            if (PropertyName && !PropertyName->IsBad && PropertyName->Name)
            {
                if (PropertyName->TypeCharacter == chType_NONE)
                {
                    *CurrentMemberInfo = new(rnra) MemberInfoList(
                        PropertyName,
                        FieldCount,
                        Operand,
                        IsNameBangQualified,
                        Operand->FieldIsKey,
                        OperandTextSpan
                        );

                    CurrentMemberInfo = &(*CurrentMemberInfo)->Next;
                }
                else
                {
                    AllInitializersValid = false;

                    // Do not report extra error if parser reported syntax error
                    if (NeedToReportError)
                    {
                        ReportSemanticError(
                            QueryErrorMode ? ERRID_QueryAnonymousTypeDisallowsTypeChar : ERRID_AnonymousTypeDisallowsTypeChar,
                            OperandTextSpan);
                    }
                }
            }
            else
            {
                AllInitializersValid = false;

                // Do not report extra error if parser reported syntax error
                if (NeedToReportError)
                {
                    if (IsXMLNameRejectedAsBadVBIdentifier && XMLNameInferredFrom)
                    {
                        ReportSemanticError(
                            QueryErrorMode ? ERRID_QueryAnonTypeFieldXMLNameInference : ERRID_AnonTypeFieldXMLNameInference,
                            XMLNameInferredFrom->TextSpan);
                    }
                    else
                    {
                        ReportSemanticError(
                            QueryErrorMode ? ERRID_QueryAnonymousTypeFieldNameInference : ERRID_AnonymousTypeFieldNameInference,
                            OperandTextSpan);
                    }
                }
            }

            ++FieldCount;
        }
    }

    if (MemberInfoCount)
    {
        *MemberInfoCount = FieldCount;
    }

    return AllInitializersValid ? MemberInfos : NULL;
}

// ----------------------------------------------------------------------------
// If we reuse an anonymous type class (because of merging) we will need to
// build the binding table based on a class template.
// ----------------------------------------------------------------------------

void Semantics::BuildBindingTableFromClass(
    MemberInfoList *MemberInfos,
    AnonymousTypeBindingTable* BindingTable,
    ULONG FieldCount,
    BCSYM_Class* Class
    )
{
    ThrowIfNull( MemberInfos );
    ThrowIfNull( BindingTable );
    ThrowIfFalse( FieldCount > 0 );
    ThrowIfNull( Class );

    ULONG i = 0;

    for( MemberInfoList *Info = MemberInfos; Info != NULL ; Info = Info->Next, i += 1 )
    {
        Declaration* PropDecl = Class->GetHash()->SimpleBind( Info->Name->Name );

        VSASSERT( PropDecl->IsProperty(), "Woah, why is this member not a property?" );
        BindingTable->AddRealProperty( i, PropDecl->PProperty() );
    }
}

// ----------------------------------------------------------------------------
// Get the anonymous type template. This method must fill out the
// "Real Property" entry in the Binding Table.
// ----------------------------------------------------------------------------

ClassOrRecordType * Semantics::GetAnonymousTypeTemplate
(
    BCSYM_Namespace *ContainingNamespace,
    BCSYM_Class *ContainingClass,
    MemberInfoList *MemberInfos,
    AnonymousTypeBindingTable* BindingTable,
    unsigned FieldCount,
    bool QueryErrorMode
)
{
    DWORD AnonymousTypeHash = GetAnonymousTypeHashCode(
        MemberInfos,
        FieldCount,
        !m_MergeAnonymousTypeTemplates
        );

    TransientSymbolStore *Transients = m_SymbolsCreatedDuringInterpretation;

    if( m_SymbolsCreatedDuringInterpretation )
    {
        BCSYM_Class **PossibleMatch = Transients->GetAnonymousTypeHash()->HashFind( AnonymousTypeHash );

        while( PossibleMatch && *PossibleMatch )
        {
            if( MatchAnonymousType(
                    *PossibleMatch,
                    MemberInfos,
                    m_SourceFile,
                    FieldCount,
                    !m_MergeAnonymousTypeTemplates
                    ) &&
                ( m_MergeAnonymousTypeTemplates ||
                  Transients->GetAnonymousTypeHash()->IsUncommited( AnonymousTypeHash, PossibleMatch ) ) )
            {
                // If we find a match, we have to build the binding table from this.

                if( FieldCount > 0 )
                {
                    BuildBindingTableFromClass( MemberInfos, BindingTable, FieldCount, *PossibleMatch );
                }
                return *PossibleMatch;
            }

            PossibleMatch = Transients->GetAnonymousTypeHash()->FindNextWithKey( AnonymousTypeHash );
        }
    }

    ClassOrRecordType *AnonymousType =
        GenerateAnonymousType(
            ContainingNamespace,
            ContainingClass,
            MemberInfos,
            BindingTable,
            AnonymousTypeHash,
            FieldCount,
            QueryErrorMode);

#if IDE 
    if (AnonymousType && !m_MergeAnonymousTypeTemplates)
    {
        AnonymousType->SetIsNonMergedAnonymousType(true);
    }
#endif

    if (AnonymousType && Transients)
    {
        Transients->GetAnonymousTypeHash()->TransactHashAdd(AnonymousTypeHash, AnonymousType);
    }

    return AnonymousType;
}

GenericBinding *
Semantics::GetAnonymousTypeBinding
(
    Declaration *AnonymousTypeTemplate,
    Location &TextSpan,
    MemberInfoList *MemberInfos,
    AnonymousTypeBindingTable* BindingTable,
    ULONG FieldCount,
    bool NoWithScope,
    bool QueryErrorMode
)
{
    if (!MemberInfos || FieldCount == 0)
    {
        return NULL;
    }

    ErrorTable *BackupErrors = m_Errors;
    ErrorTable Errors(m_Compiler, m_Project, NULL);
    m_Errors = &Errors;

    // Allocate a dummy temporary variable with cloned anonymous type class but without the public
    // properties as interpretation with context. This is to allow auto-reference fully resolved
    // members of anonymous type, such as "New With {.a = 1, .b = .a}" where member "b"'s type
    // instantiation depends on member "a"'s type instantiation.
    // The cloned anonymous type class is intentionally with public properties that are neither
    // readable nor writable.  However as we resolve each field we'll add those properties to the
    // bindable hash of the Clone and allow other members to refer to them.
    // We repeat interpretation passes until all fields are resolved or until we can no longer
    // make any more progress.

    bool HasAtleastOneKeyField = false;

    for (MemberInfoList *Info = MemberInfos; Info; Info = Info->Next)
    {
        if( Info->FieldIsKey )
        {
            HasAtleastOneKeyField = true;
            break;
        }
    }

    ClassOrRecordType *AnonymousTypeClone = GenerateUnregisteredAnonymousType(
        AnonymousTypeTemplate->GetName(),
        AnonymousTypeTemplate->GetContainingNamespace()->GetName(),
        0,
        MemberInfos,
        FieldCount,
        HasAtleastOneKeyField
        );

    // We will create anonymous type clone with properties that will be bound against.
    // These properties will have no setters/getters yet. The reason for setting these up
    // is to detect errors and determine the type, without necessarily binding the
    // set and get methods. We will do the synthesis of the actual methods when we
    // resolve each field.

    GenerateDummyProperties( AnonymousTypeClone, MemberInfos, FieldCount );

    Variable *Temporary = m_TransientSymbolCreator.AllocVariable(true, false);
    m_TransientSymbolCreator.GetVariable(
        &TextSpan,
        AnonymousTypeTemplate->GetName(),
        AnonymousTypeTemplate->GetName(),
        DECLF_Public,
        VAR_Local,
        AnonymousTypeClone,
        NULL,
        NULL,
        Temporary);
    Temporary->SetIsTemporary();
    Symbols::SetParent(Temporary, m_Procedure);

    // Setup temporary with enclosing using the temporary variable (with dummy value).
    // We need this in order to allow the properties to bind successfully. For example,
    // in: new with { .a = 10, .b = .a }, to bind the RHS .a, we must set up the with
    // context so that there is context for the "." operator.

    ILTree::WithBlock *OldEnclosingWith = m_EnclosingWith;

    GenericBinding *OpenBinding = SynthesizeOpenGenericBinding(
        AnonymousTypeClone,
        m_TransientSymbolCreator
        );

    GenericBinding *Result = SynthesizeOpenGenericBinding(
        AnonymousTypeClone,
        m_TransientSymbolCreator
        );

    ExpressionList *BadValues = NULL;
    ExpressionList **BadValue = &BadValues;

    // Run multiple passes through initializers until all generic arguments are resolved.

    ULONG ResolvedCount = 0;
    ULONG LastResolvedCount = 0;

    // Here, we are going to do the initial resolving of the anonymous type fields. We want to
    // instruct semantics to create temporaries for any properties that are referenced by the
    // same anonymous type.

    BackupValue< AnonymousTypeBindingTable* > backup( &m_AnonymousTypeBindingTable );
    m_AnonymousTypeBindingTable = BindingTable;

    BackupValue< AnonymousTypeBindingTable::BindingMode > backup2( &( BindingTable->m_Mode ) );
    BindingTable->m_Mode = AnonymousTypeBindingTable::Resolving;

    BackupValue< Variable* > backup3( &( BindingTable->m_BaseReference ) );
    BindingTable->m_BaseReference = Temporary;

    while (ResolvedCount < FieldCount)
    {
        LastResolvedCount = ResolvedCount;

        Errors.DeleteAll();

        // Set up m_EnclosedWith context to generictypebinding that is resolved at this point.
        Temporary->SetType(Result);

        SetupAnonymousTypeWithEnclosure(
            Temporary,
            AllocateSymbolReference(
                Temporary,
                Result,
                NULL,
                TextSpan),
                NoWithScope);

        ULONG i = 0;
        MemberInfoList *Info = MemberInfos;
        for (; i < FieldCount && Info; Info = Info->Next, ++i)
        {
            UINT CurrentErrorCount = Errors.GetErrorCount();

            if (!Info->InferredInitializer || Info->IsBadInitializer)
            {
                bool IsExplicitlyNamed = Info->Initializer->Opcode != ParseTree::Initializer::Expression;

                Info->IsBadInitializer = false;

                // Try resolve the type by interpreting the value.
                // If interpretation fails the return value will be bad.

                ILTree::Expression *Value = NULL;

                switch (Info->Initializer->Opcode)
                {
                    case ParseTree::Initializer::Expression:
                        Value = InterpretExpression(
                            Info->Initializer->AsExpression()->Value,
                            ExprForceRValue | ExprTypeInferenceOnly
                            );
                        break;

                    case ParseTree::Initializer::Assignment:
                        Value = InterpretInitializer(
                            Info->Initializer->AsAssignment()->Initializer,
                            NULL,
                            ExprForceRValue | ExprTypeInferenceOnly
                            );
                        break;

                    default:
                        VSFAIL("Surprising initializer");
                }

                if (m_Errors->GetErrorCount() < CurrentErrorCount)
                {
                    continue;
                }

                Info->InferredInitializer = Value;

                if (Value && !IsBad(Value) && Value->ResultType)
                {
                    if (IsCircularTypeReference(AnonymousTypeClone, Value->ResultType))
                    {
                        Info->IsBadInitializer = true;
                    }
                    else if (TypeHelpers::IsVoidType(Value->ResultType) ||
                        IsRestrictedType(Value->ResultType, m_CompilerHost) ||
                        IsRestrictedArrayType(Value->ResultType, m_CompilerHost))
                    {
                        Info->IsBadInitializer = true;
                    }
                    else if( TypeHelpers::IsGenericParameter( Value->ResultType ) &&
                        Value->ResultType->PGenericParam()->GetPosition() < OpenBinding->GetArgumentCount() &&
                        Value->ResultType->PGenericParam() ==
                            OpenBinding->GetArgument( Value->ResultType->PGenericParam()->GetPosition() ) )
                    {
                        Info->IsBadInitializer = true;
                    }
                    else
                    {
                        Result->GetArguments()[i] = Value->ResultType;
                        ++ResolvedCount;    // Consider the parameter as resolved.

                        if (AnonymousTypeClone->GetHash())
                        {
                            Declaration *PropDecl = AnonymousTypeClone->GetHash()->SimpleBind(Info->Name->Name);
                            if (PropDecl)
                            {
                                Symbols::RemoveSymbolFromHash(AnonymousTypeClone->GetHash(), PropDecl);
                            }
                        }

                        // We were able to resolve the type and verify the semantics of this particular property.
                        // Generate the necessary methods for this anonymous type, and record it in the binding
                        // table. This will cause subsequent initializers which bind to us to mark us as specially
                        // and cause us to generate a temporary.

                        BCSYM_Property* prop = GenerateAnonymousTypeField(
                            AnonymousTypeClone,
                            Info->Name->Name,
                            Info->TextSpan,
                            Info->Name->TextSpan,
                            IsExplicitlyNamed,
                            Info->IsNameBangQualified,
                            QueryErrorMode,
                            Info->FieldIsKey,
                            OpenBinding->GetArguments()[i]);

                        if( prop != NULL )
                        {
                            // Record this property in the binding table.
                            BindingTable->AddDummyProperty( i, prop );
                        }
                    }
                }
                else if (Info->InferredInitializer)
                {
                    Info->IsBadInitializer = true;
                }
            }
        }

        // Devdiv Bugs[22605] Orcas decision is not to run multiple passes to support out of order
        // initializer interpretation.
        // if (ResolvedCount <= LastResolvedCount)  // Support out of order initializer interpretation
        if (ResolvedCount < FieldCount)
        {
            m_Errors = BackupErrors;

            if (m_ReportErrors)
            {
                m_Errors->MoveNoStepErrors(&Errors);

                // Interpret bad initializers again, this time with error reporting on
                Info = MemberInfos;
                for (i = 0; i < FieldCount && Info; Info = Info->Next, ++i)
                {
                    bool IsExplicitlyNamed = Info->Initializer->Opcode != ParseTree::Initializer::Expression;
                    ILTree::Expression *Value = Info->InferredInitializer;

                    if (Info->IsBadInitializer && Value && !IsBad(Value) && Value->ResultType)
                    {
                        if (IsCircularTypeReference(AnonymousTypeClone, Value->ResultType))
                        {
                            ReportSemanticError(
                                ERRID_CircularInference2,
                                Value->Loc,
                                AnonymousTypeClone,
                                AnonymousTypeClone);
                        }
                        else if (TypeHelpers::IsVoidType(Value->ResultType))
                        {
                            ReportSemanticError(
                                ERRID_VoidValue,
                                Value->Loc);
                        }
                        else if (IsRestrictedType(Value->ResultType, m_CompilerHost) ||
                            IsRestrictedArrayType(Value->ResultType, m_CompilerHost))
                        {
                            StringBuffer TextBuffer;
                            m_Errors->CreateError(
                                ERRID_RestrictedType1,
                                (Location *)&Value->Loc,
                                m_Errors->ExtractErrorName(Value->ResultType, NULL, TextBuffer));
                        }
                        else
                        {
                            // Cannot resolve the type of this initializer due to circular reference.
                            ReportSemanticError(
                                ERRID_BadOrCircularInitializerReference,
                                Value->Loc);
                        }
                    }
                }
            }

            m_EnclosingWith = OldEnclosingWith; // Restore With Enclosure
            return NULL;
        }
    }

    m_EnclosingWith = OldEnclosingWith; // Restore With Enclosure
    m_Errors = BackupErrors;

    return ResolvedCount == FieldCount ?
        m_TransientSymbolCreator.GetGenericBindingWithLocation(
            false,
            AnonymousTypeTemplate,
            Result->GetArguments(),
            Result->GetArgumentCount(),
            NULL,
            &TextSpan) :
        NULL;
}

ClassOrRecordType *
Semantics::GenerateAnonymousType
(
    BCSYM_Namespace *ContainingNamespace,
    BCSYM_Class *ContainingClass,
    MemberInfoList *MemberInfos,
    AnonymousTypeBindingTable* BindingTable,
    DWORD AnonymousTypeHash,
    unsigned FieldCount,
    bool QueryErrorMode
)
{
    bool IsBad = false;

    BCSYM_Container *Container = ContainingClass ? ContainingClass->PContainer() : ContainingNamespace->PContainer();

    bool IsDummyAnonymousType = !m_MergeAnonymousTypeTemplates;

    // First, check if this anonymous type has any key fields. If it does not, we will not even emit the
    // Equals and GetHashCode methods.

    MemberInfoList *Info = MemberInfos;
    bool HasAtleastOneKeyField = false;

    for (Info = MemberInfos; Info; Info = Info->Next)
    {
        if( Info->FieldIsKey )
        {
            HasAtleastOneKeyField = true;
            break;
        }
    }

    // Build a stub with the name VB$AnonymousType_<number>
    STRING *ClassName = NULL;
    if (m_SymbolsCreatedDuringInterpretation)
    {
        ClassName = m_SymbolsCreatedDuringInterpretation->GetAnonymousTypeNameGenerator()->GetNextName();
    }
    else
    {
        ClassName = m_Compiler->AddString(VB_ANONYMOUSTYPE_PREFIX);
        IsDummyAnonymousType = true;
    }

    ClassOrRecordType *AnonymousClass = GenerateUnregisteredAnonymousType(
        ClassName,
        ContainingNamespace->GetName(),
        AnonymousTypeHash,
        MemberInfos,
        FieldCount,
        HasAtleastOneKeyField
        );

    // Report the new symbol we are bringing into existence, so that it can be correctly managed during decompilation
    Symbols::SetParent(AnonymousClass, Container->GetHash());
    RegisterTransientSymbol(AnonymousClass);

    // Generate anonymous type fields
    if (MemberInfos)
    {
        GenericParameter *GenericParam = AnonymousClass->GetFirstGenericParam();

        Info = MemberInfos;
        for (ULONG i = 0; Info && GenericParam && i < FieldCount; ++i, GenericParam = GenericParam->GetNextParam())
        {
            bool IsExplicitlyNamed = Info->Initializer->Opcode != ParseTree::Initializer::Expression;

            BCSYM_Property* prop = GenerateAnonymousTypeField(
                AnonymousClass,
                Info->Name->Name,
                Info->TextSpan,
                Info->Name->TextSpan,
                IsExplicitlyNamed,
                Info->IsNameBangQualified,
                QueryErrorMode,
                Info->FieldIsKey,
                GenericParam
                );

            if( prop == NULL )
            {
                IsBad = true;    // Bad anonymous type
            }
            else
            {
                // Record this property in the binding table.
                BindingTable->AddRealProperty( i, prop );
            }

            Info = Info->Next;
        }

        // Optimization to avoid some work - this assumes non-merged anonymous types will not get into final code.
        if (!IsDummyAnonymousType)
        {
            UpdateAnonymousTypeChildren(AnonymousClass->GetHash(), MemberInfos, FieldCount);
        }

        // Generate anonymous type attributes.  Do this after we've generated all of the fields because
        // the Debugger Display code generates a legal property.  We need to make sure it doesn't conflict
        // with the name of an existing property explicitly declared by the user.  Therefore it has to
        // go after the normal properties are generated
        GenerateAnonymousTypeAttributes(AnonymousClass, MemberInfos);
    }


    VSASSERT(Container->GetUnBindableChildrenHash(), "How can a container not have an unbindable members hash ?");
    return IsBad ? NULL : AnonymousClass;
}

ClassOrRecordType *
Semantics::GenerateUnregisteredAnonymousType
(
    _In_z_ STRING *ClassName,
    _In_z_ STRING *ContainingNamespaceName,
    DWORD AnonymousTypeHash,
    MemberInfoList *MemberInfos,
    unsigned FieldCount,
    bool HasAtleastOneKeyField
)
{
    ClassOrRecordType *AnonymousClass = m_TransientSymbolCreator.AllocClass(false);

    // Anonymous Type generic parameters goes from T0 ... Tm where m = number of initializer fields
    BCSYM_GenericParam *GenericParams = NULL;
    BCSYM_GenericParam **GenericParam = &GenericParams;
    BCSYM **GenericArguments = NULL;

    if (FieldCount > 0)
    {
        GenericArguments = (BCSYM **)m_TransientSymbolCreator.GetNorlsAllocator()->Alloc(
            VBMath::Multiply(
            sizeof(BCSYM *), 
            FieldCount));
    }

    for (unsigned i = 0; i < FieldCount; ++i)
    {
        WCHAR GenericParamName[MaxStringLengthForParamSize + 1];
        IfFailThrow(StringCchPrintfW(GenericParamName, DIM(GenericParamName), WIDE("T%d"), i));

        *GenericParam =
            m_TransientSymbolCreator.GetGenericParam(
                NULL,
                m_Compiler->AddString(GenericParamName),
                i,
                false,
                Variance_None); // we're generating a class, and classes must be invariant

        GenericArguments[i] =
            CreateNamedType(
                m_TransientSymbolCreator,
                AnonymousClass, // Context
                NULL,   // TypeList
                false,
                false,
                false,
                1,
                (*GenericParam)->GetName(),   // Name
                (*GenericParam),  // ptyp
                NULL);  // Generic type arguments

        GenericParam = (*GenericParam)->GetNextParamTarget();
    }

    BCSYM_Implements *ImplementsSymbolList = NULL;

    // We implement IEquatable only if we have at least one key field.

    if( HasAtleastOneKeyField )
    {
        GenerateAnonymousTypeImplements(ClassName, GenericArguments, FieldCount, AnonymousClass, &ImplementsSymbolList);
    }

    const DECLFLAGS DECLF_AnonymousClassFlags = DECLF_Friend | DECLF_Anonymous | DECLF_NotInheritable;

    m_TransientSymbolCreator.GetClass(
        NULL,
        ClassName,
        GetEmittedTypeNameFromActualTypeName(ClassName, GenericParams, m_Compiler), // emitted name for class,
        ContainingNamespaceName,
        m_SourceFile,
        GetFXSymbolProvider()->GetType(FX::ObjectType),
        ImplementsSymbolList,
        DECLF_AnonymousClassFlags,
        t_bad,
        m_TransientSymbolCreator.AllocVariable(false, false),
        NULL,
        NULL,
        GenericParams,
        NULL,
        AnonymousClass);

    SymbolList ChildrenList;

    BuildAnonymousTypeConstructorTemplate(
        GenericArguments,
        MemberInfos,
        FieldCount,
        &ChildrenList
        );

    GenerateAnonymousTypeChildren(
        ClassName,
        GenericArguments,
        AnonymousTypeHash,
        FieldCount,
        HasAtleastOneKeyField,
        AnonymousClass,
        &ChildrenList
        );

    AnonymousClass->SetTypeAccessExplictlySpecified(false);
    AnonymousClass->SetBaseExplicitlySpecified(false);

#if IDE
    AnonymousClass->SetAnonymousTypeProc(m_Procedure);
#endif

    Scope *Hash = m_TransientSymbolCreator.GetHashTable(ClassName, AnonymousClass, true, FieldCount + 17, &ChildrenList);
    Scope *UnbindableHash = m_TransientSymbolCreator.GetHashTable(ClassName, AnonymousClass, true, 0, NULL);

    AnonymousClass->SetHashes(Hash, UnbindableHash);

    AnonymousClass->SetBindingDone(true);

    return AnonymousClass;
}

// ----------------------------------------------------------------------------
// We build the template for the anonymous type constructor here, which has
// as arguments the initializers in sequence order.
// ----------------------------------------------------------------------------

void
Semantics::BuildAnonymousTypeConstructorTemplate
(
    BCSYM **GenericArguments,
    MemberInfoList *MemberInfos,
    ULONG FieldCount,
    SymbolList *OwningSymbolList // [in] symbol list to add to
)
{
    ThrowIfNull( OwningSymbolList );

    BCSYM_Param* Param = NULL;
    BCSYM_Param* ParamLast = NULL;

    // Build the parameter list for the constructor.
    // Note that we never put [] around arguments even if it is a keyword. We also
    // never change the constructor name.

    ULONG i = 0;
    for( MemberInfoList* Info = MemberInfos; Info; Info = Info->Next, i += 1 )
    {
        m_TransientSymbolCreator.GetParam(
            NULL,
            m_Compiler->AddString( Info->Name->Name ),
            GenericArguments[ i ],
            PARAMF_ByVal,
            NULL,
            &Param,
            &ParamLast,
            false
            );
    }

    // Build the constructor.

    Procedure* Constructor = Declared::BuildSyntheticConstructor(
        &m_TransientSymbolCreator,
        m_Compiler,
        Param,
        OwningSymbolList,
		Declared::SyntheticConstructorType::Instance
        );

    // For now, put in an empty method body. We'll synthesize the assignment
    // to the fields later.

    Symbols::SetCode(Constructor->PSyntheticMethod(), USZ_CRLF, CCH_CRLF);
}

void
Semantics::GenerateAnonymousTypeImplements
(
    _In_z_ STRING *ClassName,
    BCSYM **GenericArguments,
    ULONG FieldCount,
    ClassOrRecordType *AnonymousClass,
    BCSYM_Implements **ImplementsSymbolList
)
{
    if (!GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEquatableType))
    {
        return;
    }

    // Create a named type for VB$AnonymousType_n(Of T0, T1, ..., Tm)
    BCSYM_NamedType *AnonymousClassNamedType =
        CreateNamedType(
            m_TransientSymbolCreator,
            AnonymousClass, // Context
            1,
            ClassName,  // Name
            AnonymousClass, // ptyp
            CreateNameTypeArguments(
                m_TransientSymbolCreator,
                GenericArguments,
                FieldCount));   // GenericArguments
    if (!AnonymousClassNamedType)
    {
        return;
    }

    // Generate Implements Global.System.IEquatable(Of VB$AnonymousType_n(Of T0, T1, ..., Tm))
    BCSYM_NamedType *NamedType =
        CreateNamedType(
            m_TransientSymbolCreator,
            AnonymousClass, // Context
            2,
            m_Compiler->AddString(L"System"),   // Name
            NULL,   // ptyp
            NULL,   // Generic type arguments
            m_Compiler->AddString(L"IEquatable"),   // Name
            GetFXSymbolProvider()->GetType(FX::GenericIEquatableType),   // ptyp
            CreateNameTypeArguments(
                m_TransientSymbolCreator,
                true,   // TypesForTypeArgumentSpecified
                1,
                AnonymousClassNamedType)); // Generic type arguments
    if (!NamedType)
    {
        return;
    }

    m_TransientSymbolCreator.GetImplements(
        NULL,
        NamedType->GetHashingName(),
        NamedType,
        ImplementsSymbolList);
}

void
Semantics::GenerateAnonymousTypeChildren
(
    _In_z_ STRING *ClassName,
    BCSYM **GenericArguments,
    DWORD AnonymousTypeHash,
    ULONG FieldCount,
    bool HasAtleastOneKeyField,
    ClassOrRecordType *AnonymousClass,
    SymbolList *ClassChildren
)
{
    NorlsAllocator *pTransientAllocator = m_TransientSymbolCreator.GetNorlsAllocator();

    if( HasAtleastOneKeyField )
    {
        STRING *MethodEquals = m_Compiler->AddString(L"Equals");

        // Public Overloads Overrides Function Equals(ByVal obj As Object) As Boolean
        BCSYM_SyntheticMethod *EqualsOverrides = m_TransientSymbolCreator.AllocSyntheticMethod(false);

        BCSYM_Param *EqualsOverridesParam_First = m_TransientSymbolCreator.GetParam(
            NULL,
            m_Compiler->AddString(L"obj"),
            GetFXSymbolProvider()->GetType(FX::ObjectType),
            PARAMF_ByVal,
            NULL,
            NULL, // tack the parameter onto this list
            NULL,
            false);

        m_TransientSymbolCreator.GetProc(
            NULL,
            MethodEquals,
            MethodEquals,
            (CodeBlockLocation*) NULL,
            (CodeBlockLocation*) NULL,
            DECLF_Public | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_OverridesKeywordUsed | DECLF_Function | DECLF_HasRetval | DECLF_AllowOptional,
            GetFXSymbolProvider()->GetType(t_bool), // return type of the proc
            EqualsOverridesParam_First,
            NULL,
            NULL,
            NULL,
            SYNTH_TransientSymbol, // synthetic kind
            NULL,
            ClassChildren,
            EqualsOverrides);

        Symbols::SetOverloads(EqualsOverrides);
        // Defer Setting the Body of EqualsOverrides until EqualsOverloads is defined next.

        // Public Overloads Function Equals(ByVal val As VB$AnonymousType_n(Of T0, T1, ..., Tm)) As Boolean Implements Global.System.IEquatable(Of AnonymousType).Equals
        BCSYM_SyntheticMethod *EqualsOverloads = m_TransientSymbolCreator.AllocSyntheticMethod(false);

        // Create a named type for VB$AnonymousType_n(Of T0, T1, ..., Tm) under the Equals Context
        BCSYM_NamedType *AnonymousClassNamedType =
            CreateNamedType(
                m_TransientSymbolCreator,
                EqualsOverloads, // Context
                1,
                ClassName,  // Name
                AnonymousClass, // ptyp
                CreateNameTypeArguments(
                    m_TransientSymbolCreator,
                    GenericArguments,
                    FieldCount));   // GenericArguments

        if (GetFXSymbolProvider()->IsTypeAvailable(FX::GenericIEquatableType))
        {
            // Generate Implements Global.System.IEquatable(Of VB$AnonymousType_n(Of T0, T1, ..., Tm))
            BCSYM_NamedType *NamedType =
                CreateNamedType(
                    m_TransientSymbolCreator,
                    EqualsOverloads, // Context
                    2,
                    m_Compiler->AddString(L"System"),   // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    m_Compiler->AddString(L"IEquatable"),   // Name
                    GetFXSymbolProvider()->GetType(FX::GenericIEquatableType),   // ptyp
                    CreateNameTypeArguments(
                        m_TransientSymbolCreator,
                        true,   // TypesForTypeArgumentSpecified
                        1,
                        AnonymousClassNamedType)); // Generic type arguments

            BCSYM_ImplementsList *ImplementsList = m_TransientSymbolCreator.GetImplementsList(
                NULL,
                NamedType,
                MethodEquals);

            BCSYM_Interface *IEquatableInterface =
                GetFXSymbolProvider()->GetType(FX::GenericIEquatableType)->PInterface();

            // There's only 1 Equals in IEquatable
            BCSYM_NamedRoot *IEquatableEquals = IEquatableInterface->SimpleBind(NULL, MethodEquals);    

            ImplementsList->SetImplementedMember(
                IEquatableEquals,
                m_TransientSymbolCreator.GetGenericBinding(
                    false,
                    GetFXSymbolProvider()->GetType(FX::GenericIEquatableType),
                    NamedType->GetArgumentsArray()[1]->m_Arguments,
                    NamedType->GetArgumentsArray()[1]->m_ArgumentCount,
                    NULL));

            EqualsOverloads->SetImplementsList(ImplementsList);
        }

        // Parameter ByVal val As VB$AnonymousType_n(Of T0, T1, ..., Tm)
        BCSYM_Param *Param_First = m_TransientSymbolCreator.GetParam(
            NULL,
            m_Compiler->AddString(L"val"),
            AnonymousClassNamedType,
            PARAMF_ByVal,
            NULL,
            NULL, // tack the parameter onto this list
            NULL,
            false);

        m_TransientSymbolCreator.GetProc(
            NULL,
            MethodEquals,
            MethodEquals,
            (CodeBlockLocation*) NULL,
            (CodeBlockLocation*) NULL,
            DECLF_Public | DECLF_Hidden | DECLF_OverloadsKeywordUsed | DECLF_Implementing | DECLF_Function | DECLF_HasRetval | DECLF_AllowOptional,
            GetFXSymbolProvider()->GetType(t_bool), // return type of the proc
            Param_First,
            NULL,
            NULL,
            NULL,
            SYNTH_TransientSymbol, // synthetic kind
            NULL,
            ClassChildren,
            EqualsOverloads);

        Symbols::SetOverloads(EqualsOverloads);

        // Default Method Body
        // Equals = True
        // If val Is Nothing Then Return False
        // With val
        // End With
        StringBuffer EqualsOverloadsCodeBuffer;
        EqualsOverloadsCodeBuffer.AppendString(
            WIDE("Equals = True\r\n")
            WIDE("If val Is Nothing Then Return False\r\n")
            WIDE("With val\r\n")
            WIDE("End With\r\n"));

        Symbols::SetCode(
            EqualsOverloads,
            EqualsOverloadsCodeBuffer.AllocateBufferAndCopy(pTransientAllocator),
            EqualsOverloadsCodeBuffer.GetStringLength());

        // Now set the EqualsOverrides Method Body
        EqualsOverrides->SetBoundTree(
            CreateEqualsOverridesBody(
                EqualsOverrides,
                EqualsOverloads,
                AnonymousClass,
                FieldCount));

        // Public Overrides Function GetHashCode() As Integer
        BCSYM_SyntheticMethod *GetHashCodeOverrides = m_TransientSymbolCreator.AllocSyntheticMethod(false);
        m_TransientSymbolCreator.GetProc(
            NULL,
            m_Compiler->AddString(L"GetHashCode"),
            m_Compiler->AddString(L"GetHashCode"),
            (CodeBlockLocation*) NULL,
            (CodeBlockLocation*) NULL,
            DECLF_Public | DECLF_Hidden | DECLF_OverridesKeywordUsed | DECLF_Function | DECLF_HasRetval | DECLF_AllowOptional,
            GetFXSymbolProvider()->GetType(t_i4), // return type of the proc
            NULL,
            NULL,
            NULL,
            NULL,
            SYNTH_TransientNoIntCheckSymbol, // synthetic kind
            NULL,
            ClassChildren,
            GetHashCodeOverrides);

        // Default Method Body
        // GetHashCode = AnonymousTypeHash
        StringBuffer GetHashCodeOverridesCodeBuffer;

        WCHAR Digits[MaxStringLengthForIntToStringConversion]; // e.g. long + trailing NULL
        IfFailThrow(StringCchPrintfW(Digits, DIM(Digits), L"%lu", AnonymousTypeHash));

        const WCHAR *GetHashCodeTemplate =
            WIDE("GetHashCode = |1\r\n");
        ResStringRepl(GetHashCodeTemplate, &GetHashCodeOverridesCodeBuffer, Digits);

        Symbols::SetCode(
            GetHashCodeOverrides,
            GetHashCodeOverridesCodeBuffer.AllocateBufferAndCopy(pTransientAllocator),
            GetHashCodeOverridesCodeBuffer.GetStringLength());
    }

    // Public Overrides Function ToString() As Integer
    BCSYM_SyntheticMethod *ToStringOverrides = m_TransientSymbolCreator.AllocSyntheticMethod(false);
    m_TransientSymbolCreator.GetProc(
        NULL,
        m_Compiler->AddString(L"ToString"),
        m_Compiler->AddString(L"ToString"),
        (CodeBlockLocation*) NULL,
        (CodeBlockLocation*) NULL,
        DECLF_Public | DECLF_Hidden | DECLF_OverridesKeywordUsed | DECLF_Function | DECLF_HasRetval | DECLF_AllowOptional,
        GetFXSymbolProvider()->GetType(t_string), // return type of the proc
        NULL,
        NULL,
        NULL,
        NULL,
        SYNTH_TransientSymbol, // synthetic kind
        NULL,
        ClassChildren,
        ToStringOverrides);

    // Default Method Body
    StringBuffer ToStringOverridesCodeBuffer;
    ToStringOverridesCodeBuffer.AppendString(L"Dim buf As New System.Text.StringBuilder\r\nbuf.Append(\"{ \")\r\nbuf.Append(\"}\")\r\nReturn buf.ToString()\r\n");
    Symbols::SetCode(
        ToStringOverrides,
        ToStringOverridesCodeBuffer.AllocateBufferAndCopy(pTransientAllocator),
        ToStringOverridesCodeBuffer.GetStringLength());
}

//==============================================================================
// Generate the attributes for an anonymous type.
//
// DebuggerDisplayAttribute
//==============================================================================
void
Semantics::GenerateAnonymousTypeAttributes
(
    ClassOrRecordType *AnonymousClass,
    MemberInfoList *MemberInfos
)
{
    STRING* DebuggerDisplayString = GenerateAnonymousTypeDebuggerDisplayProperty(AnonymousClass, MemberInfos);
    GenerateDebuggerDisplayAttribute(AnonymousClass, DebuggerDisplayString);
}

//==============================================================================
//
// This code generates up the string which will be inside the DebuggerDisplay attribute
// on the generated anonymous type.  The style of the string is 
//
//   Prop1={Prop1}, Prop2={Prop2}
//
// We will calculate up to a maximum of 3 properties.  Horizontal space in the 
// debugger is limited and having more than 3 properties is simply not readable 
// in the default display.  
//
// Properties are not free so adding more will be both unreadable and have a 
// performance penalty attached to it.  
//
// Users can access the other properties by simply expanding the element
//
//==============================================================================
STRING*
Semantics::GenerateAnonymousTypeDebuggerDisplayProperty
(
    ClassOrRecordType *AnonymousClass,
    MemberInfoList *MemberInfos
)
{
    ThrowIfNull(AnonymousClass);
    ThrowIfNull(MemberInfos);

    StringBuffer buffer;
    buffer.AppendString(L"Global.System.Diagnostics.DebuggerDisplay(\"");
    unsigned int count = 0;
    for (MemberInfoList *Info = MemberInfos; Info; Info = Info->Next)
    {
        if ( count > 3 )
        {
            buffer.AppendString(L", ...");
            break;
        }
        else if ( count > 0 )
        {
            buffer.AppendString(L", ");
        }
        buffer.AppendPrintf(L"%s={%s}", 
                Info->Name->Name,
                Info->Name->Name);
        count++;
    }

    buffer.AppendString(L"\")");
    return m_Compiler->AddString(&buffer);
}

void
Semantics::GenerateDebuggerDisplayAttribute
(
    ClassOrRecordType *AnonymousClass,
    _In_z_ STRING *DebuggerDisplayString
)
{
    if (GetFXSymbolProvider()->IsTypeAvailable(FX::DebuggerDisplayAttributeType))
    {
        Type* debuggerDisplayType = GetFXSymbolProvider()->GetType(FX::DebuggerDisplayAttributeType);

        // Passing voidType to see if this sybmol is globally avialable. Don't pass in null, or it will assume it is in the current class
        // and not perform certain checks if the symbol is marked protected. Otherwise the voidType is used to get a generic bindings of which
        // are none, so that is good too.
        // Empty generic binding context is good because debuggerdisplayattribute doesn't have
        if (IsAccessible(debuggerDisplayType, NULL, TypeHelpers::GetVoidType()))
        {
            // Set the debugger display attribute on Anonymous Type symbol
            BCSYM_NamedType *AttributeName =
                CreateNamedType(
                    m_TransientSymbolCreator,
                    AnonymousClass, // Context
                    3,
                    m_Compiler->AddString(L"System"),   // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    m_Compiler->AddString(L"Diagnostics"),   // Name
                    NULL,   // ptyp
                    NULL,   // Generic type arguments
                    m_Compiler->AddString(L"DebuggerDisplay"),   // Name
                    debuggerDisplayType,   // ptyp
                    NULL);  // Generic type arguments
            AttributeName->SetIsAttributeName(true);

            BCSYM_ApplAttr *AttributeSymbol =
                m_TransientSymbolCreator.GetApplAttr(
                    AttributeName,
                    NULL,
                    false,
                    false);

            Location loc = {0};
            loc.Invalidate();

            AttributeSymbol->SetExpression(
                        m_TransientSymbolCreator.GetConstantExpression( 
                        &loc,
                        AnonymousClass,
                        NULL, // no const expr list
                        NULL, // no const expr list
                        NULL, // no fake string
                        DebuggerDisplayString,
                        -1 ) // figure the size out based on what we pass in for the expression
                                            );

            if (!AnonymousClass->GetPAttrVals())
            {
                AnonymousClass->SetPAttrVals((AttrVals *)m_TransientSymbolCreator.GetNorlsAllocator()->Alloc(sizeof(AttrVals)));
            }
            else
            {
                AttributeSymbol->SetNext(AnonymousClass->GetPAttrVals()->GetPNon----edData()->m_psymApplAttrHead);
            }
            AnonymousClass->GetPAttrVals()->GetPNon----edData()->m_psymApplAttrHead = AttributeSymbol;

            ILTree::Expression *ptreeExprBound;
            ptreeExprBound = Attribute::GetBoundTreeFromApplAttr(
                AttributeSymbol,
                AnonymousClass,
                m_Errors,
                m_TransientSymbolCreator.GetNorlsAllocator(),
                m_CompilerHost);

            VSASSERT(!ptreeExprBound || ptreeExprBound->bilop == SX_APPL_ATTR,
                "How can bound tree for an applied attribute be anything else ?");

            if (ptreeExprBound)
            {
                AttributeSymbol->SetBoundTree(&ptreeExprBound->AsAttributeApplicationExpression());
            }
        }
    }
}

STRING *
Semantics::AnonymousTypeCreateFieldNameForDeclaration(_In_z_ STRING *PropertyName)
{
    STRING *FieldName = m_Compiler->ConcatStrings(WIDE("$"), PropertyName);
    return FieldName;
}

void
Semantics::AnonymousTypeAppendFieldNameIntoCode(_In_z_ STRING *PropertyName, StringBuffer & Code)
{
    if(PropertyName[0] == WIDE('$'))
    {
        if(STRING_CONST(m_Compiler, It1) == PropertyName)
        {
            Code.AppendSTRING(STRING_CONST(m_Compiler, FieldIt1));
            return;
        }
        else if(STRING_CONST(m_Compiler, It2) == PropertyName)
        {
            Code.AppendSTRING(STRING_CONST(m_Compiler, FieldIt2));
            return;
        }
        else if(STRING_CONST(m_Compiler, Group2) == PropertyName)
        {
            Code.AppendSTRING(STRING_CONST(m_Compiler, FieldGroup2));
            return;
        }

        AssertIfFalse(false);
    }

    Code.AppendString(WIDE("_"));
    Code.AppendSTRING(PropertyName);
}

STRING *
Semantics::AnonymousTypeCreateFieldNameForCode(_In_z_ STRING *PropertyName)
{
    if(PropertyName[0] == WIDE('$'))
    {
        if(STRING_CONST(m_Compiler, It1) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldIt1);
        }
        else if(STRING_CONST(m_Compiler, It2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldIt2);
        }
        else if(STRING_CONST(m_Compiler, Group2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldGroup2);
        }

        AssertIfFalse(false);
    }

    STRING *FieldName = m_Compiler->ConcatStrings(WIDE("_"), PropertyName);
    return FieldName;
}

STRING *
Semantics::AnonymousTypeCreateFieldNameFromCode(_In_z_ STRING *NameInCode)
{
    if(NameInCode[0] == WIDE('_'))
    {
        STRING *ActualName = m_Compiler->ConcatStrings(WIDE("$"), &NameInCode[1]);
        return ActualName;
    }
    else
    {
        if(STRING_CONST(m_Compiler, FieldIt1) == NameInCode)
        {
            return m_Compiler->ConcatStrings(WIDE("$"), STRING_CONST(m_Compiler, It1));
        }
        else if(STRING_CONST(m_Compiler, FieldIt2) == NameInCode)
        {
            return m_Compiler->ConcatStrings(WIDE("$"), STRING_CONST(m_Compiler, It2));
        }
        else if(STRING_CONST(m_Compiler, FieldGroup2) == NameInCode)
        {
            return m_Compiler->ConcatStrings(WIDE("$"), STRING_CONST(m_Compiler, Group2));
        }
    }

    return NULL;
}

// ----------------------------------------------------------------------------
// The strategy here is that the constructor must match the argument names,
// but in the synthetic body, we must use a different scheme to prevent name
// clashes.
//
// Sub New( vbit1 as integer, $VB$IT1 as integer)
//     Me.$vbit1 = _vbit1 ' map
//     Me.$vb$it1 = vbit1 ' map
//
// becomes
//
// Sub New( vbit1 as integer, $VB$IT1 as integer)
//     Me.$vbit1 = vbit1 ' ummap
//     Me.$vb$it1 = $VB$IT1 ' ummap
// ----------------------------------------------------------------------------

STRING *
Semantics::MapAnonymousTypeConstructorNameForBody(_In_z_ STRING *PropertyName)
{
    if(PropertyName[0] == WIDE('$'))
    {
        if(STRING_CONST(m_Compiler, It1) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldIt1);
        }
        else if(STRING_CONST(m_Compiler, It2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldIt2);
        }
        else if(STRING_CONST(m_Compiler, Group2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, FieldGroup2);
        }

        AssertIfFalse(false);
    }

    STRING *FieldName = m_Compiler->ConcatStrings(WIDE("_"), PropertyName);
    return FieldName;
}

STRING *
Semantics::UnMapAnonymousTypeConstructorNameForBody(_In_z_ STRING *PropertyName)
{
    if(PropertyName[0] == WIDE('_'))
    {
        STRING *ActualName = m_Compiler->AddString(&PropertyName[1]);
        return ActualName;
    }
    else
    {
        if(STRING_CONST(m_Compiler, FieldIt1) == PropertyName)
        {
            return STRING_CONST(m_Compiler, It1);
        }
        else if(STRING_CONST(m_Compiler, FieldIt2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, It2);
        }
        else if(STRING_CONST(m_Compiler, FieldGroup2) == PropertyName)
        {
            return STRING_CONST(m_Compiler, Group2);
        }
    }

    return NULL;
}

BCSYM_Property*
Semantics::GenerateAnonymousTypeField
(
    Type *Target,
    _In_z_ STRING *PropertyName,
    Location &TextSpan,
    Location &NameTextSpan,
    bool IsExplicitlyNamed,
    bool IsBangNamed,
    bool QueryErrorMode,
    bool FieldIsKey,
    BCSYM *FieldType
)
{
    NorlsAllocator *pTransientAllocator = m_TransientSymbolCreator.GetNorlsAllocator();
    VSASSERT(Target->IsContainer(), "Unexpected!");

    Declaration *Member = GetFXSymbolProvider()->GetType(FX::ObjectType)->PContainer()->GetHash()->SimpleBind(
        PropertyName);
    if (Member)
    {
        if(QueryErrorMode)
        {
            ReportSemanticError(
                ERRID_QueryInvalidControlVariableName1,
                TextSpan);
        }
        else
        {
            ReportSemanticError(
                ERRID_DuplicateAnonTypeMemberName1,
                TextSpan,
                PropertyName);
        }

         return NULL;
    }

    Member = Target->PContainer()->GetHash()->SimpleBind(PropertyName);
    if (Member)
    {
        ReportSemanticError(
            QueryErrorMode ? ERRID_QueryDuplicateAnonTypeMemberName1 : ERRID_DuplicateAnonTypeMemberName1,
            TextSpan,
            PropertyName);
        return NULL;
    }

    SymbolList BindableSymbols;
    SymbolList UnBindableSymbols;
    STRING *FieldName = AnonymousTypeCreateFieldNameForDeclaration(PropertyName);
    BCSYM_Variable *Field = m_TransientSymbolCreator.AllocVariable( true, false );

    m_TransientSymbolCreator.GetVariable(
        &TextSpan, // No location
        FieldName,
        FieldName,
        DECLF_Private | DECLF_Hidden | (FieldIsKey ? DECLF_ReadOnly : 0),
        VAR_Member,
        FieldType,
        NULL,
        &BindableSymbols,
        Field);

    STRING *GetName = m_Compiler->ConcatStrings( CLS_PROPERTY_GET_PREFIX, PropertyName);
    BCSYM_SyntheticMethod *GetMethod = m_TransientSymbolCreator.AllocSyntheticMethod(true);
    m_TransientSymbolCreator.GetProc(
        &TextSpan,
        GetName,
        GetName,
        ( CodeBlockLocation* ) NULL,
        ( CodeBlockLocation* ) NULL,
        DECLF_PropGet | DECLF_Public | DECLF_Hidden | DECLF_SpecialName,
        FieldType,
        NULL,
        NULL,
        NULL,
        NULL,
        SYNTH_TransientSymbol,
        NULL,
        &UnBindableSymbols,
        GetMethod );

    StringBuffer GetCode;

    GetCode.AppendString(WIDE("Return "));
    AnonymousTypeAppendFieldNameIntoCode(PropertyName, GetCode);
    GetCode.AppendWithLength(USZ_CRLF, CCH_CRLF);

    Symbols::SetCode(
        GetMethod,
        GetCode.AllocateBufferAndCopy(pTransientAllocator),
        GetCode.GetStringLength());

    // Emit a setter only if the field is not a key field.

    BCSYM_SyntheticMethod *SetMethod = NULL;

    if( !FieldIsKey )
    {
        SetMethod = m_TransientSymbolCreator.AllocSyntheticMethod(true);

        STRING *SetName = NULL;
        if (m_Compiler && 
            m_Compiler->GetProjectBeingCompiled() &&
            m_Compiler->GetProjectBeingCompiled()->GetOutputType() == OUTPUT_WinMDObj)
        {
            SetName = m_Compiler->ConcatStrings( CLS_PROPERTY_PUT_PREFIX, PropertyName );
        }
        else
        {
            SetName = m_Compiler->ConcatStrings( CLS_PROPERTY_SET_PREFIX, PropertyName );
        }

        BCSYM_Param *ParamFirst = NULL, *LastParam = NULL;

        m_TransientSymbolCreator.GetParam(
           NULL,
           m_Compiler->AddString( WIDE( "Value" )),
           FieldType,
           NULL,
           NULL,
           &ParamFirst,
           &LastParam,
           false);

        m_TransientSymbolCreator.GetProc(
            &TextSpan,
            SetName,
            SetName,
            (CodeBlockLocation*) NULL,
            (CodeBlockLocation*) NULL,
            DECLF_PropSet | DECLF_Public | DECLF_Hidden | DECLF_SpecialName,
            NULL,
            ParamFirst,
            NULL,
            NULL,
            NULL,
            SYNTH_TransientSymbol,
            NULL,
            &UnBindableSymbols,
            SetMethod );

        StringBuffer SetCode;

        AnonymousTypeAppendFieldNameIntoCode(PropertyName, SetCode);
        SetCode.AppendString(WIDE(" = Value"));

        Symbols::SetCode(
            SetMethod,
            SetCode.AllocateBufferAndCopy(pTransientAllocator),
            SetCode.GetStringLength());
    }

    Property *PropertySymbol = m_TransientSymbolCreator.AllocProperty(true);

    m_TransientSymbolCreator.GetProc(
        NULL,
        PropertyName,
        PropertyName,
        ( CodeBlockLocation* ) NULL,
        ( CodeBlockLocation* ) NULL,
        DECLF_Public | DECLF_Function | DECLF_HasRetval,
        FieldType,
        NULL,
        NULL,
        NULL,
        NULL,
        SYNTH_TransientSymbol,
        NULL,
        NULL,
        PropertySymbol->GetPropertySignature()); // Fills out the base class of the BCSYM_Property

    PropertySymbol->SetIsExplicitlyNamed(IsExplicitlyNamed);
    PropertySymbol->SetIsBangNamed(IsBangNamed);
    PropertySymbol->SetIsFromAnonymousType(true);

    m_TransientSymbolCreator.GetProperty(
        &NameTextSpan,
        PropertyName,
        DECLF_Public | DECLF_ShadowsKeywordUsed | (FieldIsKey ? DECLF_ReadOnly : 0),
        GetMethod,
        SetMethod,
        PropertySymbol,
        &BindableSymbols );

    Symbols::AddSymbolListToHash(Target->PContainer()->GetHash(), &BindableSymbols, true);
    Symbols::AddSymbolListToHash(Target->PContainer()->GetUnBindableChildrenHash(), &UnBindableSymbols, true);

    return PropertySymbol;
}

// ----------------------------------------------------------------------------
// There is a terrible hack here.
// Anonymous type fields are named $field. In this method, we emit synthetic
// code, and we name the fields _field. In name lookup, we check whether we
// are a anonymous type method, and *replace* any leading _ to a $! This is
// terrible.
//
// Right now, what I have done is I have special cased that code so that for
// the constructor, we only replace member fields. This is because the
// constructor accepts arguments that must match the field names for
// System.Core's sake.
//
// If you EVER modify this code, please check with Microsoft before you do. It's
// quite likely that it is possible to break this.
//
// Ideally, this would be done using parse helpers.
// ----------------------------------------------------------------------------

void Semantics::UpdateAnonymousTypeChildren
(
    BCSYM_Hash *Members,
    MemberInfoList *MemberInfos,
    ULONG FieldCount
)
{
    VSASSERT(Members, "Unexpected NULL member");
    if (!Members || !MemberInfos)
    {
        return;
    }

    NorlsAllocator *pTransientAllocator = m_TransientSymbolCreator.GetNorlsAllocator();

    // .ctor
    Declaration *Constructor = Members->SimpleBind( m_Compiler->AddString( L".ctor" ) );
    if( Constructor != NULL && Constructor->IsProc() )
    {
        BCSYM_SyntheticMethod* SyntheticConstructor = Constructor->PSyntheticMethod();
        StringBuffer CodeText;
        SafeInt<ULONG> SafeFieldCount(FieldCount);

        CodeText.AllocateSize( (SafeFieldCount * CTOR_LENGTH_PER_FIELD).Value() );

        CodeText.AppendString( SyntheticConstructor->GetCode() );

        if( CodeText.GetStringLength() > CTOR_CODE_BACK_OFFSET )
        {
            ULONG FieldNum = 0;

            // For each of the fields, generate assignment from the arguments.

            for( MemberInfoList* Info = MemberInfos; Info; Info = Info->Next, FieldNum += 1 )
            {
                STRING* FieldName = AnonymousTypeCreateFieldNameForCode( Info->Name->Name );

                // For the body, we have to map the RHS. In InterpretName, we will unmap the RHS.

                StringBuffer FieldCodeText;
                const WCHAR *FieldCodeTemplate = WIDE( "Me.|1 = |2\r\n" );
                ResStringRepl(
                    FieldCodeTemplate,
                    &FieldCodeText,
                    FieldName,
                    MakeSafeName( m_Compiler, MapAnonymousTypeConstructorNameForBody( Info->Name->Name ), false )
                    );

                CodeText.InsertString(CodeText.GetStringLength() - CTOR_CODE_BACK_OFFSET, FieldCodeText.GetString());
            }
        }
        else
        {
            VSFAIL( "Invalid .ctor Method Body ");
        }

        Symbols::SetCode(
            SyntheticConstructor,
            CodeText.AllocateBufferAndCopy( pTransientAllocator ),
            CodeText.GetStringLength()
            );
    }

    // Equals
    Declaration *Equals = Members->SimpleBind(m_Compiler->AddString(L"Equals"));
    while (Equals && Equals->IsSyntheticMethod() && !Equals->PSyntheticMethod()->IsImplementing())
    {
        Equals = Equals->GetNextOfSameName();
    }
    if (Equals && Equals->IsSyntheticMethod() && Equals->PSyntheticMethod()->IsImplementing())
    {
        BCSYM_SyntheticMethod *EqualsProcedure = Equals->PSyntheticMethod();

        StringBuffer CodeText;
        SafeInt<ULONG> codeSize( FieldCount );
        codeSize *= EQUALS_LENGTH_PER_FIELD;
        CodeText.AllocateSize(codeSize.Value());

        CodeText.AppendString(EqualsProcedure->GetCode());
        if (CodeText.GetStringLength() > EQUALS_CODE_BACK_OFFSET)
        {
            for (MemberInfoList *Info = MemberInfos; Info; Info = Info->Next)
            {
                // Only emit if this is a key field.

                if( !Info->FieldIsKey )
                {
                    continue;
                }

                STRING *FieldName = AnonymousTypeCreateFieldNameForCode(Info->Name->Name);

                // |1 - FieldName
                //
                // If |1 Is Nothing Then
                //     If .|1 IsNot Nothing Then Return False
                // Else
                //     If .|1 Is Nothing Then Return False
                // End If
                // If |1 IsNot Nothing AndAlso .|1 IsNot Nothing AndAlso Not CObj(|1).Equals(CObj(.|1)) Then Return False
                const WCHAR *FieldCodeTemplate =
                    WIDE("If |1 Is Nothing Then\r\n")
                    WIDE("    If .|1 IsNot Nothing Then Return False\r\n")
                    WIDE("Else\r\n")
                    WIDE("    If .|1 Is Nothing Then Return False\r\n")
                    WIDE("End If\r\n")
                    WIDE("If |1 IsNot Nothing AndAlso .|1 IsNot Nothing AndAlso Not CObj(|1).Equals(CObj(.|1)) Then Return False\r\n");
                StringBuffer FieldCodeText;
                ResStringRepl(FieldCodeTemplate, &FieldCodeText, FieldName);

                // Insert before "End With\r\n"
                CodeText.InsertString(CodeText.GetStringLength() - EQUALS_CODE_BACK_OFFSET, FieldCodeText.GetString());
            }

            Symbols::SetCode(
                EqualsProcedure,
                CodeText.AllocateBufferAndCopy(pTransientAllocator),
                CodeText.GetStringLength());
        }
        else
        {
            VSFAIL("Invalid Equals Method Body");
        }
    }

    // GetHashCode
    Declaration *GetHashCode = Members->SimpleBind(m_Compiler->AddString(L"GetHashCode"));
    if (GetHashCode && GetHashCode->IsProc())
    {
        BCSYM_SyntheticMethod *GetHashCodeProcedure = GetHashCode->PSyntheticMethod();
        StringBuffer CodeText;
        SafeInt<ULONG> codeSize( FieldCount );
        codeSize *= GETHASHCODE_LENGTH_PER_FIELD;
        CodeText.AllocateSize(codeSize.Value());

        CodeText.AppendString(GetHashCodeProcedure->GetCode());
        if (CodeText.GetStringLength() > GETHASHCODE_CODE_BACK_OFFSET)
        {
            ULONG FieldNum = 0;
            for (MemberInfoList *Info = MemberInfos; Info; Info = Info->Next, ++FieldNum)
            {
                // Only emit if this is a key field.

                if( !Info->FieldIsKey )
                {
                    continue;
                }

                STRING *FieldName = AnonymousTypeCreateFieldNameForCode(Info->Name->Name);

                StringBuffer FieldCodeText;
                // GetHashCode *= &HA5555529
                // If Not |1 Is Nothing Then GetHashCode += |1.GetHashCode()
                const WCHAR *FieldCodeTemplate =
                    WIDE("GetHashCode *= &HA5555529\r\n")
                    WIDE("If Not |1 Is Nothing Then GetHashCode += |1.GetHashCode()\r\n");
                ResStringRepl(FieldCodeTemplate, &FieldCodeText, FieldName);

                CodeText.InsertString(CodeText.GetStringLength() - GETHASHCODE_CODE_BACK_OFFSET, FieldCodeText.GetString());
            }
        }
        else
        {
            VSFAIL("Invalid GetHashCode Method Body");
        }

        Symbols::SetCode(
            GetHashCodeProcedure,
            CodeText.AllocateBufferAndCopy(pTransientAllocator),
            CodeText.GetStringLength());
    }

    // ToString
    Declaration *ToString = Members->SimpleBind(m_Compiler->AddString(L"ToString"));
    if (ToString && ToString->IsProc())
    {
        BCSYM_SyntheticMethod *ToStringProcedure = ToString->PSyntheticMethod();
        StringBuffer CodeText;
        SafeInt<ULONG> codeSize( FieldCount );
        codeSize *= TOSTRING_LENGTH_PER_FIELD;
        CodeText.AllocateSize(codeSize.Value());

        CodeText.AppendString(ToStringProcedure->GetCode());
        if (CodeText.GetStringLength() > TOSTRING_CODE_BACK_OFFSET)
        {
            for (MemberInfoList *Info = MemberInfos; Info; Info = Info->Next)
            {
                STRING *FieldName = AnonymousTypeCreateFieldNameForCode(Info->Name->Name);

                StringBuffer FieldCodeText;
                // |1 - FieldName
                //
                // buf.AppendFormat("{0} = {1}|3", "|1", |2)
                const WCHAR *FieldCodeTemplate = WIDE("buf.AppendFormat(\"{0} = {1}|3\", \"|1\", |2)\r\n");
                ResStringRepl(FieldCodeTemplate, &FieldCodeText, Info->Name->Name, FieldName, Info->Next ? WIDE(", ") : WIDE(" "));

                // Insert.
                CodeText.InsertString(CodeText.GetStringLength() - TOSTRING_CODE_BACK_OFFSET, FieldCodeText.GetString());
            }

            Symbols::SetCode(
                ToStringProcedure,
                CodeText.AllocateBufferAndCopy(pTransientAllocator),
                CodeText.GetStringLength());
        }
        else
        {
            VSFAIL("Invalid ToString Method Body");
        }
    }
}

ILTree::Expression *
Semantics::SetupAnonymousTypeWithEnclosure
(
    Variable *Temporary,
    ILTree::Expression *Value,
    bool NoWithScope
)
{
    ILTree::Expression *TemporaryReference =
        AllocateSymbolReference(
            Temporary,
            Temporary->GetType(),
            NULL,
            Value->Loc);
    SetFlag32(TemporaryReference, SXF_LVALUE);

    ILTree::ExpressionWithChildren *TemporaryAssignment =
        (ILTree::ExpressionWithChildren *)AllocateExpression(
            SX_ASG,
            TypeHelpers::GetVoidType(),
            TemporaryReference,
            Value,
            Value->Loc);
    SetFlag32(TemporaryAssignment, SXF_ASG_SUPPRESS_CLONE);

    if(!NoWithScope)
    {
        m_EnclosingWith = &AllocateStatement(SB_WITH, Value->Loc, 0, false)->AsWithBlock();
        m_EnclosingWith->TemporaryBindAssignment = TemporaryAssignment;
        m_EnclosingWith->TemporaryClearAssignment = NULL;   // Not needed
        SetFlag32(m_EnclosingWith, SBF_WITH_LVALUE);
    }

    return TemporaryAssignment;
}

ILTree::ProcedureBlock *
Semantics::CreateEqualsOverridesBody
(
    Procedure *EqualsOverrides,
    Procedure *EqualsOverloads,
    ClassOrRecordType *AnonymousClass,
    ULONG FieldCount
)
{
    Location HiddenLocation = { 0 };
    HiddenLocation.SetLocationToHidden();

    GenericBinding *GenericBindingContext = FieldCount > 0 ?
        SynthesizeOpenGenericBinding(AnonymousClass, m_TransientSymbolCreator) :
        NULL;

    // Default Method Body
    // Return Me.Equals(TryCast(obj, AnonymousType))
    ILTree::ProcedureBlock *EqualsBody = &AllocateStatement(SB_PROC, HiddenLocation, 0, false)->AsProcedureBlock();
    EqualsBody->pproc = EqualsOverrides;

    EqualsBody->ptempmgr =
        new(m_TreeStorage)
            TemporaryManager(
                m_Compiler,
                m_CompilerHost,
                EqualsOverrides,
                &m_TreeStorage);

    Scope *LocalsAndParameters = NULL;
    Variable *FirstParamVar = NULL;
    Declared::MakeLocalsFromParams(
        m_Compiler,
        &m_TreeStorage,
        EqualsOverrides,
        m_Errors,
        false,
        &FirstParamVar,
        &LocalsAndParameters);

    Variable *ReturnVariable = Declared::MakeReturnLocal(m_Compiler, &m_TreeStorage, EqualsOverrides, NULL);
    Symbols::AddSymbolToHash(LocalsAndParameters, ReturnVariable, true, false, false);

    EqualsBody->Locals = LocalsAndParameters;
    EqualsBody->ReturnVariable = ReturnVariable;

    ILTree::Expression *BoundArgument =
        ConvertWithErrorChecking(
            AllocateSymbolReference(
                FirstParamVar,
                FirstParamVar->GetType(),
                NULL,
                HiddenLocation,
                GenericBindingContext),
            FieldCount > 0 ? (Type *)GenericBindingContext : (Type *)AnonymousClass,
            ExprIsExplicitCast | ExprHasExplicitCastSemantics | ExprHasTryCastSemantics);

    ExpressionList *BoundArguments =
        AllocateExpression(
            SX_LIST,
            TypeHelpers::GetVoidType(),
            BoundArgument,
            NULL,
            HiddenLocation);

    ILTree::ReturnStatement *Return = &AllocateStatement(SL_RETURN, HiddenLocation, 0, false)->AsReturnStatement();
    Return->ReturnExpression =
        AllocateExpression(
            SX_CALL,
            GetReturnType(EqualsOverloads),
            AllocateSymbolReference(
                EqualsOverloads,
                TypeHelpers::GetVoidType(),
                NULL,
                HiddenLocation,
                GenericBindingContext),
            BoundArguments,
            HiddenLocation);

    Return->ReturnExpression->AsCallExpression().MeArgument =
        AllocateSymbolReference(
            AnonymousClass->GetMe(),
            FieldCount > 0 ? (Type *)GenericBindingContext : (Type *)AnonymousClass,
            NULL,
            HiddenLocation,
            GenericBindingContext);

    // Create the SB_PROC node.
    EqualsBody->Child = Return;

    return EqualsBody;
}

void
Semantics::GenerateDummyProperties
(
    ClassOrRecordType *AnonymousType,
    MemberInfoList *MemberInfos,
    ULONG FieldCount
)
{
    GenericParameter *GenericParam = AnonymousType->GetFirstGenericParam();
    MemberInfoList *Info = MemberInfos;
    for (ULONG i = 0; Info && GenericParam && i < FieldCount; ++i, Info = Info->Next, GenericParam = GenericParam->GetNextParam())
    {
        if (!GetFXSymbolProvider()->GetType(FX::ObjectType)->PContainer()->GetHash()->SimpleBind(Info->Name->Name) &&
            !AnonymousType->GetHash()->SimpleBind(Info->Name->Name))
        {
            SymbolList BindableSymbols;
            Property *PropertySymbol = m_TransientSymbolCreator.AllocProperty(NULL);

            m_TransientSymbolCreator.GetProc(
                NULL,
                Info->Name->Name,
                Info->Name->Name,
                ( CodeBlockLocation* ) NULL,
                ( CodeBlockLocation* ) NULL,
                DECLF_Public | DECLF_Function | DECLF_HasRetval,
                GenericParam,
                NULL,
                NULL,
                NULL,
                NULL,
                SYNTH_TransientSymbol,
                NULL,
                NULL,
                PropertySymbol->GetPropertySignature()); // Fills out the base class of the BCSYM_Property

            m_TransientSymbolCreator.GetProperty(
                NULL,
                Info->Name->Name,
                DECLF_Public | DECLF_ShadowsKeywordUsed | (Info->FieldIsKey ? DECLF_ReadOnly : 0),
                NULL,   // Neither Get or Set is defined for dummy property.
                NULL,   // Neither Get or Set is defined for dummy property.
                PropertySymbol,
                &BindableSymbols);

            PropertySymbol->SetIsExplicitlyNamed(Info->Initializer->Opcode != ParseTree::Initializer::Expression);
            PropertySymbol->SetIsBangNamed(Info->IsNameBangQualified);
            Symbols::AddSymbolListToHash(AnonymousType->GetHash(), &BindableSymbols, true);
        }
    }
}

bool
MatchAnonymousType
(
    ClassOrRecordType *AnonymousClass,
    MemberInfoList *MemberInfos,
    SourceFile *SourceFile,
    unsigned FieldCount,
    bool CheckLocation
)
{
    VSASSERT(AnonymousClass->IsContainer(), "How come Anonymous Type is not a container?");
    Scope *Hash = AnonymousClass->PContainer()->GetHash();
    if (Hash == NULL)
    {
        return false;   // Bad anonymous type
    }

    // Devdiv Bug [26424] Anonymous Type is to support assembly level merging.
#if 0
    // Allow method level merging only
    if (AnonymousClass->GetAnonymousTypeProc() != CurrentProcedure)
    {
        return false;   // Not generated from the same method.
    }
#endif

    // Anonymous Types merging algorithm -
    // Anonymous types need to be merged by name of the field in order.
    // This is achieved here by comparing corresponding name and the type name.
    // Because VB generates anonymous types using generic arguments as T0 ... Tn and assign them to anonymous
    // type fields in order, comparing corresponding name and type name guarantees us
    // match criteria of both name and order would have been satisfied.
    // We also consider "key"-ness when matching. We use the fact that key fields only have getters.
    // We don't have to worry about C# naming scheme since we will never merge across assembly boundaries.

    unsigned MatchCount = 0;
    GenericParameter *GenericParam = AnonymousClass->GetFirstGenericParam();
    MemberInfoList *Info = MemberInfos;

    for( ; Info && GenericParam; Info = Info->Next, GenericParam = GenericParam->GetNextParam() )
    {
        Declaration *Member = Hash->SimpleBind( Info->Name->Name );

        if( Member != NULL &&
            Member->GetAccess() == ACCESS_Public &&
            Member->IsProperty()
            )
        {
            Type* MemberType = Member->PProperty()->GetType();

            if( MemberType != NULL &&
                MemberType->IsGenericParam() &&
                MemberType->PGenericParam()->GetName() == GenericParam->GetName() &&

                // Check key-ness; if field is key, we must not have set property. Or if
                // field is not key, we must have set property. If either constraint is
                // matched, these felds match.

                ( ( Info->FieldIsKey && Member->PProperty()->SetProperty() == NULL ) ||
                  ( !Info->FieldIsKey && Member->PProperty()->SetProperty() != NULL ) ) &&

                // Consider location as well.

                ( !CheckLocation ||
                  CheckLocation &&    // Devdiv Bugs[30776] [31373]
                  AnonymousClass->GetSourceFile() == SourceFile &&
                  Member->PProperty()->HasLocation() &&
                  Location::Compare(Member->PProperty()->GetLocation(), &Info->Name->TextSpan ) == 0
                )
              )
            {
                ++MatchCount;
            }
        }
    }
    return (Info == NULL) && (GenericParam == NULL) &&   // No more generic parameters left
        MatchCount == FieldCount;   // Exact match.
}

#if IDE 
bool
MatchAnonymousType
(
    Container *AnonymousType1,
    Container *AnonymousType2
)
{
    if (AnonymousType1 && AnonymousType2)
    {
        if (AnonymousType1 == AnonymousType2)
        {
            return true;
        }

#if 0   // Devdiv Bug [26424] Anonymous Type is merged at assembly level.
        // Allow method level merging only
        if (AnonymousType1->GetAnonymousTypeProc() != AnonymousType2->GetAnonymousTypeProc())
        {
            return false;   // Not generated from the same method.
        }
#endif

        ULONG PublicPropertyCount = 0;
        ULONG MatchCount = 0;
        BCITER_CHILD ChildIterator(AnonymousType1);
        for (Declaration *Child = ChildIterator.GetNext();
             Child != NULL;
             Child = ChildIterator.GetNext())
        {
            if (Child &&
                Child->GetAccess() == ACCESS_Public &&
                Child->IsProperty())
            {
                ++PublicPropertyCount;
                Declaration *OtherChild = AnonymousType2->GetHash()->SimpleBind(Child->GetName());
                if (OtherChild &&
                    OtherChild->GetAccess() == ACCESS_Public &&
                    OtherChild->IsProperty())
                {
                    // NOTE: We don't want to compare locations here, since the whole purpose of this
                    // method is to allow AT to match regardless of location. It is used in IDE scenarios
                    // where AT-s are not merged and if we don't allow them to match, many compiler
                    // semantics would be broken. See also BCSYM::AreTypesEqual for conditions on
                    // when this method is invoked.
                    Type *InputChildType = Child->PProperty()->GetType();
                    Type *OtherChildType = OtherChild->PProperty()->GetType();
                    if (InputChildType && OtherChildType &&
                        InputChildType->IsGenericParam() && OtherChildType->IsGenericParam() &&
                        InputChildType->PGenericParam()->GetName() == OtherChildType->PGenericParam()->GetName() &&
                        Child->PProperty()->IsReadOnly() == OtherChild->PProperty()->IsReadOnly())
                    {
                        ++MatchCount;
                    }
                }
            }
        }
        return PublicPropertyCount == MatchCount;
    }

    return AnonymousType1 == AnonymousType2;
}
#endif

DWORD
GetAnonymousTypeHashCode
(
    MemberInfoList *MemberInfos,
    ULONG FieldCount,
    bool CheckLocation
)
{
    // Devdiv Bug [26424] Anonymous Type is merged at assembly level.
    // NOTE method level anonymous type merging would require hashing anonymous type name to account for anonymous
    // types of same shape but at different method levels.
    // Since the final design is perform assembly level merging we are ok to return -1 instead.
    if (!MemberInfos || FieldCount == 0)
    {
        return -1;
    }

    NorlsAllocator nra(NORLSLOC);

    SafeInt<ULONG> cElements(FieldCount);
    if( CheckLocation )
    {
        cElements *= 2;
    }

    SafeInt<ULONG> ElementsAsPtr(cElements);
    ElementsAsPtr *= sizeof(BYTE*);
    const BYTE **rgpbToBeHashed = (const BYTE **)nra.Alloc(ElementsAsPtr.Value());

    SafeInt<ULONG> ElementsAsSizeT(cElements);
    ElementsAsSizeT *= sizeof(size_t);
    size_t *rgcbToBeHashed = (size_t *)nra.Alloc(ElementsAsSizeT.Value());

    memset(rgpbToBeHashed, 0, ElementsAsPtr.Value());
    memset(rgcbToBeHashed, 0, ElementsAsSizeT.Value());

    ULONG i = 0;
    for (MemberInfoList *Info = MemberInfos; i < FieldCount && Info; ++i, Info = Info->Next)
    {
        SafeInt<size_t> MemberNameLength(StringPool::StringLength(Info->Name->Name));
        SafeInt<size_t> MemberNameLengthPlusOne(MemberNameLength);
        MemberNameLengthPlusOne += 1;
        SafeInt<size_t> MemberNameRawLength(MemberNameLengthPlusOne);
        MemberNameRawLength *= sizeof(WCHAR);
        WCHAR *MemberName = (WCHAR *)nra.Alloc(MemberNameRawLength.Value());
        ToLowerCase(Info->Name->Name, MemberName, MemberNameLengthPlusOne.Value());    // VB Names are case-insensitive.
        rgpbToBeHashed[i] = (BYTE *)MemberName;
        rgcbToBeHashed[i] = MemberNameLength.Value() * sizeof(WCHAR);
    }

    if (CheckLocation)
    {
        i = 0;
        for (MemberInfoList *Info = MemberInfos; i < FieldCount && Info; ++i, Info = Info->Next)
        {
            rgpbToBeHashed[FieldCount + i] = (BYTE *)&Info->Name->TextSpan;
            rgcbToBeHashed[FieldCount + i] = sizeof(Location);
        }
    }

    return ComputeCRC32(rgpbToBeHashed, rgcbToBeHashed, cElements.Value());
}

bool
IsCircularTypeReference
(
    Type *AnonymousType,
    Type *MemberType
)
{
    if (!AnonymousType || !MemberType)
    {
        return false;
    }

    if (TypeHelpers::EquivalentTypes(AnonymousType, MemberType))
    {
        return true;
    }

    if (MemberType->IsGenericBinding())
    {
        // Check each generic argument
        for (ULONG i = 0; i < MemberType->PGenericBinding()->GetArgumentCount(); ++i)
        {
            if (IsCircularTypeReference(AnonymousType, MemberType->PGenericBinding()->GetArgument(i)))
            {
                return true;
            }
        }

        return TypeHelpers::EquivalentTypes(AnonymousType, MemberType->PGenericBinding()->GetGeneric());
    }

    return false;
}

ParseTree::IdentifierDescriptor *
Semantics::ExtractAnonTypeMemberName
(
    ParseTree::Expression *Input,
    bool &IsNameBangQualified,
    bool &IsXMLNameRejectedAsBadVBIdentifier,
    ParseTree::XmlNameExpression *&XMLNameInferredFrom
)
{
    switch (Input->Opcode)
    {
        case ParseTree::Expression::Name:
            return &Input->AsName()->Name;

        case ParseTree::Expression::XmlName:
            XMLNameInferredFrom = Input->AsXmlName();

            if (Input->AsXmlName()->LocalName.Name &&
                !Scanner::IsIdentifier(Input->AsXmlName()->LocalName.Name))
            {
                IsXMLNameRejectedAsBadVBIdentifier = true;
                return NULL;
            }

            return &Input->AsXmlName()->LocalName;

        case ParseTree::Expression::DotQualified:

            // See if this is an Identifier qualified with XmlElementsQualified or XmlDescendantsQualified
            {
                ParseTree::Expression * Name = Input->AsQualified()->Name;

                if(Name && Name->Opcode == ParseTree::Expression::Name)
                {
                    ParseTree::Expression * Base = Input->AsQualified()->Base;

                    if(Base &&
                        (
                         Base->Opcode == ParseTree::Expression::XmlElementsQualified ||
                         Base->Opcode == ParseTree::Expression::XmlDescendantsQualified
                        )
                       )
                    {
                        // infer the name from the XmlElementsQualified or XmlDescendantsQualified
                        return ExtractAnonTypeMemberName(
                                    Base,
                                    IsNameBangQualified,
                                    IsXMLNameRejectedAsBadVBIdentifier,
                                    XMLNameInferredFrom);
                    }
                }
            }
            __fallthrough;
        case ParseTree::Expression::BangQualified:
        case ParseTree::Expression::XmlElementsQualified:
        case ParseTree::Expression::XmlAttributeQualified:
        case ParseTree::Expression::XmlDescendantsQualified:

            IsNameBangQualified = Input->Opcode == ParseTree::Expression::BangQualified;

            return ExtractAnonTypeMemberName(
                        Input->AsQualified()->Name,
                        IsNameBangQualified,
                        IsXMLNameRejectedAsBadVBIdentifier,
                        XMLNameInferredFrom);

        case ParseTree::Expression::CallOrIndex:

            {
                ParseTree::CallOrIndexExpression * CallOrIndex = Input->AsCallOrIndex();

                if(!CallOrIndex->Arguments.Values)
                {
                    return
                                ExtractAnonTypeMemberName(
                                    CallOrIndex->Target,
                                    IsNameBangQualified,
                                    IsXMLNameRejectedAsBadVBIdentifier,
                                    XMLNameInferredFrom);
                }

                AssertIfNull(CallOrIndex->Arguments.Values);

                // See if this indexed XmlElementsQualified or XmlDescendantsQualified
                if(!Input->AsCallOrIndex()->Arguments.Values->Next)
                {
                    if(CallOrIndex->Target &&
                        (
                         CallOrIndex->Target->Opcode == ParseTree::Expression::XmlElementsQualified ||
                         CallOrIndex->Target->Opcode == ParseTree::Expression::XmlDescendantsQualified
                        )
                       )
                    {
                        // infer the name from the XmlElementsQualified or XmlDescendantsQualified
                        return ExtractAnonTypeMemberName(
                                    CallOrIndex->Target,
                                    IsNameBangQualified,
                                    IsXMLNameRejectedAsBadVBIdentifier,
                                    XMLNameInferredFrom);
                    }
                }

                break;
            }
    }

    return NULL;
}
