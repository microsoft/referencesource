//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work to bind handles clauses and verify withevents variables.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

void Bindable::ValidateWithEventsVarsAndHookUpHandlersInContainer()
{

    VSASSERT( GetStatusOfValidateWithEventsVarsAndHookUpHandlers() != InProgress,
                    "How can we have have event validation cycles ?");

    if (GetStatusOfValidateWithEventsVarsAndHookUpHandlers() == Done)
    {
        return;
    }

    BCSYM_Container *Container = CurrentContainer();
    if (!Container->IsClass() ||
        TypeHelpers::IsEnumType(Container->PClass()))
    {
        SetStatusOfValidateWithEventsVarsAndHookUpHandlers(Done);
        return;
    }

    SetStatusOfValidateWithEventsVarsAndHookUpHandlers(InProgress);

    // Makes sure validation of withevents and shadowing, etc. of the types
    // of the withevents variables in bases are done.
    //
    BCSYM_Class *Base = Container->PClass()->GetCompilerBaseClass();
    if (Base)
    {
        ValidateWithEventsVarsAndHookUpHandlersInContainer(Base, m_CompilationCaches);
    }

    // Need to use the SAFE iterator because more members might
    // be added to the Container during hookuping of Handles.
    // This adding of member will happen while the iterator is
    // still in use.
    //
    BCITER_CHILD_SAFE Members(Container);

    // Validate all the withevent vars in this container
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        if (IsWithEventsVariable(Member))
        {
            ValidateWithEventsVar(Member->PVariable());
        }
    }

    // Validate and hookup all the handles clauses specified in this container
    Members.Reset();
    while (BCSYM_NamedRoot *Member = Members.GetNext())
    {
        // Only methods with implementation can have handles clauses
        //
        if (!Member->IsMethodImpl())
        {
            continue;
        }

        BCITER_Handles iterHandles( Member->PMethodImpl() );

        for(BCSYM_HandlesList *HandlesEntry = iterHandles.GetNext();
            HandlesEntry;
            HandlesEntry = iterHandles.GetNext())
        {
            if (HandlesEntry->IsBad())
            {
                continue;
            }

            ValidateAndHookUpHandles(
                HandlesEntry,
                Member->PMethodImpl());
        }
    }

    SetStatusOfValidateWithEventsVarsAndHookUpHandlers(Done);
}

bool
Bindable::IsWithEventsVariable
(
    BCSYM_NamedRoot *Member
)
{
    if (Member->IsVariable() &&
        Member->PVariable()->IsWithEvents() && // Is this a WithEvents variable?
        !Member->CreatedByWithEventsDecl())    // don't consider the synthetic WithEvents variable we build
    {
        return true;
    }

    return false;
}

/**************************************************************************************************
;ValidateWithEventsVar

Makes sure the WithEvents var sources at least one visible non-shared event.
Also checks to make sure the type behind the WithEvents var is a class/interface
***************************************************************************************************/
bool // true - WithEvents var is good
Bindable::ValidateWithEventsVar
(
    BCSYM_Variable *WithEventsVar
)
{
    // We don't want to mark withevents variables in references assemblies as bad
    // because we want users to be able to use these withevents variables at least
    // in non-withevents context

    if (WithEventsVar->IsBad())
    {
        return false;
    }

    VSASSERT( WithEventsVar->GetType(),
                    "How can a withevents variable not have atleast a bad type ?");

    if (WithEventsVar->GetType()->IsBad())
    {
        WithEventsVar->SetIsBad();

        // Also, indicate that the problem here is with a type of the with events variable.
        //
        WithEventsVar->PVariable()->SetIsBadVariableType(true);
        return false;
    }

    BCSYM *TypeOfWithEventsVar = WithEventsVar->GetType();

    // The type of a WITHEVENTS variable must be either a
    // - a type parameter with a class constraint or
    // - class or an interface,
    //
    if (!IsClassOrInterface(TypeOfWithEventsVar) &&
        !(TypeOfWithEventsVar->IsGenericParam() &&
            TypeOfWithEventsVar->PGenericParam()->IsReferenceType()))
    {
        ReportErrorOnSymbol(
            ERRID_WithEventsAsStruct,
            CurrentErrorLog(WithEventsVar),
            // Intrinsic types don't have locations, so report error on the withevents var itself
            //
            WithEventsVar->GetRawType()->IsNamedType()?
                WithEventsVar->GetRawType() :
                WithEventsVar);

        // Indicate that the problem here is with the type of the withevents variable, but don't
        // mark the variable itself as completely bad because we still want the user to be able
        // to use it like a normal variable.
        //
        WithEventsVar->PVariable()->SetIsBadVariableType(true);
        return false;
    }

    // The WithEvents variable must source at least one non-shared event that is visible
    // to the container in which it is defined.
    //
    // Errors are no longer reported for this scenario. - see bug VSWhidbey 542456.
    // Because of this CheckForAccessibleEvents is no longer required, but this indirectly/subtly
    // results in calls to ResolveShadowingOverloadingAndOverridingForContainer,
    // ----AttributesOnAllSymbolsInContainer and other such tasks in bindable which later consumers
    // of these withevent variable might be dependent on. So to minimize code churn and risk for
    // whidbey, not removing this call. ----AttributesOnAllSymbolsInContainer is infact known to
    // be required for withevents of interfaces with the CoClass attributes.
    //
    // 




    bool AccessibilityIssue = false;
    CheckForAccessibleEvents(
        TypeOfWithEventsVar,
        // Consider EventSource properties too as being accessible events
        true,
        AccessibilityIssue);

    return true;
}


bool // true - there are events defined on the WithEvents variable AND they are visible to ContainerOfWithEventsVar
Bindable::CheckForAccessibleEvents
(
    BCSYM *TypeOfWithEventsVar,
    bool ConsiderEventSourcePropertiesToo,
    bool &AnyEventDefined
)
{
    // Note: the calling function is expected to initialize AnyEventDefine to false.
    // We don't do this here because the function is recursive.

    if (TypeOfWithEventsVar->IsBad())
    {
        // This has the effect of sometimes missing an event def we could have found had we not punted.
        // But does it make sense to dig into an interface that is junk?

        return false;
    }

    BCSYM *TypePossiblyContainingEvents;

    if (TypeOfWithEventsVar->IsGenericParam())
    {
        TypePossiblyContainingEvents =
            GetClassConstraint(
                TypeOfWithEventsVar->PGenericParam(),
                CurrentCompilerHost(),
                CurrentAllocator(),
                true,   // ReturnArraysAs"System.Array"
                true    // ReturnValuesAs"System.ValueType"or"System.Enum"
                );

        // See if there are any events in the class constraint
        //
        // Note that the class constraint needs to be checked separately to
        // preserve the shadowing semantics for the events found.
        //
        if (TypePossiblyContainingEvents &&
            !TypePossiblyContainingEvents->IsBad() &&
            CheckForAccessibleEventsInContainer(
                TypePossiblyContainingEvents,
                TypeOfWithEventsVar,
                ConsiderEventSourcePropertiesToo,
                AnyEventDefined))
        {
            return true;
        }

        VSASSERT(TypePossiblyContainingEvents ||
                 TypeOfWithEventsVar->PGenericParam()->IsReferenceType(), "Unexpected withevents type!!!");

        // See if there are any events in the Interface constraints

        BCITER_Constraints ConstraintsIter(TypeOfWithEventsVar->PGenericParam(), true, CurrentCompilerInstance());

        while (BCSYM_GenericConstraint *Constraint = ConstraintsIter.Next())
        {
            VSASSERT(!Constraint->IsBadConstraint(), "Bad constraint unexpected!!!");

            if (!Constraint->IsGenericTypeConstraint())
            {
                continue;
            }

            TypePossiblyContainingEvents = Constraint->PGenericTypeConstraint()->GetType();

            if (!TypePossiblyContainingEvents ||
                TypePossiblyContainingEvents->IsBad() ||
                !TypePossiblyContainingEvents->IsInterface())
            {
                continue;
            }

            if (CheckForAccessibleEventsInContainer(
                    TypePossiblyContainingEvents,
                    TypeOfWithEventsVar,
                    ConsiderEventSourcePropertiesToo,
                    AnyEventDefined))
            {
                return true;
            }
        }

        return false;
    }
    else
    {
        TypePossiblyContainingEvents = TypeOfWithEventsVar;

        return
            CheckForAccessibleEventsInContainer(
                TypePossiblyContainingEvents,
                TypeOfWithEventsVar,
                ConsiderEventSourcePropertiesToo,
                AnyEventDefined);
    }

    return false;
}

bool
Bindable::CheckForAccessibleEventsInContainer
(
    BCSYM *TypePossiblyContainingEvents,
    BCSYM *TypeOfWithEventsVar,
    bool ConsiderEventSourcePropertiesToo,
    bool &AnyEventDefined
)
{
    // Note: the calling function is expected to initialize AnyEventDefine to false.
    // We don't do this here because the function is recursive.

    VSASSERT(TypePossiblyContainingEvents &&
             TypePossiblyContainingEvents->IsContainer(),
                "Non-container unexpected!!!");

    // will hold a list of members marked as Shadowing in here
    //
    MembersHashTable ShadowingMembers;

    // Resolve shadowing etc. for the type of the withevents variable before looking for accessible events in it
    ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(TypePossiblyContainingEvents->PContainer(), m_CompilationCaches);

    return CheckForAccessibleEventsWorker(
                TypePossiblyContainingEvents,
                TypeOfWithEventsVar,
                &ShadowingMembers,
                ConsiderEventSourcePropertiesToo,
                AnyEventDefined);
}

bool
Bindable::CheckForAccessibleEventsWorker
(
    BCSYM *TypePossiblyContainingEvents,
    BCSYM *TypeOfWithEventsVar,
    MembersHashTable *ShadowingMembers,
    bool ConsiderEventSourcePropertiesToo,
    bool &AnyEventDefined
)
{
    VSASSERT( TypePossiblyContainingEvents->IsContainer(),
                    "Looking for events in non-container unexpected!!!");

    // 


    // ---- attributes on all symbols in the EventDecl container. This is needed
    // in order to verify if a property is an EventSource which is determined by
    // an attribute on the property.
    //
    if (ConsiderEventSourcePropertiesToo)
    {
        ----AttributesOnAllSymbolsInContainer(TypePossiblyContainingEvents->PContainer(), m_CompilationCaches);
    }

    BCITER_CHILD ContainerIterator;
    ContainerIterator.Init(TypePossiblyContainingEvents->PContainer());

    for(BCSYM_NamedRoot *EventDecl = ContainerIterator.GetNext();
        EventDecl != NULL;
        EventDecl = ContainerIterator.GetNext())
    {
        // Was the event decl we just found shadowed out by somebody on a nearer base class/interface?
        //
        if (IsMemberShadowed(EventDecl, ShadowingMembers))
            continue;

        bool IsMemberAccessible =
            IsAccessible(
                EventDecl,
                TypePossiblyContainingEvents->IsGenericBinding() ?
                    TypePossiblyContainingEvents->PGenericBinding() :
                    NULL,
                CurrentContainer(),
                TypeOfWithEventsVar);

        // Keep track of who is shadowing in this container - we'll need the info if the event we find is on a derived container
        if (IsMemberAccessible && EventDecl->IsShadowing())
        {
            ShadowingMembers->Add(EventDecl);
        }

        if (ConsiderEventSourcePropertiesToo &&
            EventDecl->IsProperty() &&
            ValidEventSourceProperty(EventDecl->PProperty()))
        {
            IsMemberAccessible =
                IsMemberAccessible &&
                IsAccessible(
                    EventDecl->PProperty()->GetProperty(),
                    TypePossiblyContainingEvents->IsGenericBinding() ?
                        TypePossiblyContainingEvents->PGenericBinding() :
                        NULL,
                    CurrentContainer(),
                    TypeOfWithEventsVar);

            BCSYM *PropertyReturnType = EventDecl->PProperty()->GetType();

            // 


            // We only care about properties returning classes or interfaces
            // because only classes and interfaces can be specified in a
            // handles clause.
            //

            VSASSERT(IsClassOrInterface(PropertyReturnType),
                        "Invalid event source property found. Should have been disallowed earlier!!!");

            // We've come across a Property that returns an Event source.  Dig through it.
            //
            if (CheckForAccessibleEvents(
                    PropertyReturnType,                // Property return type
                    false,                             // i.e. don't consider EventSource properties, because
                                                       //    we only allow at most 3 names i.e. x.y.z in the
                                                       //    handles clause and and only y is allowed to be the
                                                       //    referring event source property.
                    AnyEventDefined))
            {
                // there are events on this WithEvents variable - and they are accessible
            }
            else
            {
                continue;
            }
        }
        else if (EventDecl->IsEventDecl())
        {
            // there are events on this WithEvents variable - whether they are accessible is another matter
            //
            AnyEventDefined = true;
        }
        else
        {
            // Screen out everthing that isn't an Event declaration or possible Event Source
            //
            continue;
        }

        // This assert will guard against any bad changes to the conditions checks above
        VSASSERT( AnyEventDefined,
                        "How can we get here when we have not found any event or event source ?");

        // Determine if the event is accessible
        // It must be visible to the WithEvents container that references the event
        //
        if (IsMemberAccessible)
        {
            return true;
        }

    } // trawl container members looking for events/properties that may return events


    // Look recursively in bases too
    BasesIter Bases(TypePossiblyContainingEvents->PContainer());

    BCSYM_GenericTypeBinding *BaseGenericBinding;
    while (BCSYM_Container *Base = Bases.GetNextBase(&BaseGenericBinding))
    {
        if (Base->IsBad()) continue;

        if (CheckForAccessibleEventsWorker(
                BaseGenericBinding ?
                    (BCSYM *)BaseGenericBinding :
                    Base,
                TypeOfWithEventsVar,
                ShadowingMembers,
                false,  // Don't consider event source properties in based as available
                        // because we don't allow binding to them in the handles clause
                        // (for no other better reason than leaving this implementation
                        //  as it was without complicating things, since this is not a
                        //  user feature anyway.)
                AnyEventDefined))
        {
            return true;
        }
    }

    return false; // no visible events found
}


void
Bindable::ValidateWithEventsVarsInHandlesListsAndSynthesizePropertiesIfRequired()
{
    BCSYM_Container *ContainerOfHandlingMethods = CurrentContainer();

    if (!IsClass(ContainerOfHandlingMethods) &&
        !IsStructure(ContainerOfHandlingMethods) &&
        !IsStdModule(ContainerOfHandlingMethods))
    {
        return;
    }

    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCITER_CHILD Members(ContainerOfHandlingMethods);
    while(BCSYM_NamedRoot *Member = Members.GetNext())
    {
        // only method implementations can have handles clauses
        if (!Member->IsMethodImpl())
        {
            continue;
        }

        BCSYM_Proc *Proc = Member->PProc();
        BCITER_Handles iterHandles(Member->PMethodImpl());
        BCSYM_HandlesList *Handles = iterHandles.GetNext();

        if (!Handles)
        {
            continue;
        }

        ErrorTable *ErrorLog = CurrentErrorLog(Proc);

        for(;
            Handles;
            Handles = iterHandles.GetNext())
        {
            if (Handles->IsMyBase() ||
                Handles->IsEventFromMeOrMyClass())
            {
                continue;
            }

            bool FoundInBase;
            BCSYM_Variable *WithEventsVar =
                GetWithEventsVarReferredToInHandlesClause(
                    Handles,
                    FoundInBase);

            if (!WithEventsVar)
            {
                // "Handles clause requires a WithEvents variable."
                ReportErrorAtLocation(
                    ERRID_NoWithEventsVarOnHandlesList,
                    ErrorLog,
                    Handles->GetLocationOfWithEventsVar());

                Handles->SetIsBad();
            }
            else if (WithEventsVar->IsBad() ||
                     WithEventsVar->IsBadVariableType() ||   // the type of the variable is good, but is not a class or interface
                     WithEventsVar->GetType()->IsBad())      // the type of the variable is bad
            {
                // Any metadata errors on a symbol should be reported at the location
                // the symbol is used in source code
                //
                if (DefinedInMetaData(WithEventsVar->GetContainer()))
                {
                    VSASSERT( !DefinedInMetaData(CurrentContainer()),
                                    "How can Current Context for handles clauses not be in VB Source Code ?!");

                    WithEventsVar->ReportError(
                        CurrentCompilerInstance(),
                        ErrorLog,
                        Handles->GetLocationOfWithEventsVar());
                }

                Handles->SetIsBad();
            }
            else
            {
                // get the withevents property if possible
                BCSYM_Property *WithEventsProperty =
                    GetWithEventsPropertyForWithEventsVariable(WithEventsVar->PVariable());

                // Create it if it doesn't exist (for handling events defined on
                // WithEvent vars that exist on a base class).
                //
                if (!WithEventsProperty)
                {
                    VSASSERT(FoundInBase,
                                "Why do we have to synthesize a property for a withevents variable in the current class ? It should already have been synthesized in declared!!");

                    WithEventsProperty =
                        SynthesizeWithEventsProperty(WithEventsVar->PVariable(), SymbolFactory);

                    WithEventsProperty->SetCreatedByHandlesClause(Handles);
                }

                Handles->SetWithEventsProperty(WithEventsProperty);
            }
        }
    }
}

BCSYM_Variable*
Bindable::GetWithEventsVarReferredToInHandlesClause
(
    BCSYM_HandlesList *HandlesEntry,
    bool &WithEventsVarFoundInBase
)
{
    WithEventsVarFoundInBase = false;

    // 


    bool InAccessibleMember = false;

    // Bind to the WithEvents variable
    //
    BCSYM_Variable *WithEventsVar =
        BindWithEventsVariable(
            HandlesEntry->GetWithEventsVarName(),
            InAccessibleMember,
            WithEventsVarFoundInBase);

    return WithEventsVar;
}


BCSYM_Property*
Bindable::GetWithEventsPropertyForWithEventsVariable
(
    BCSYM_Variable *WithEventsVar
)
{
    BCSYM_NamedRoot *PossibleWithEventsProperty =
        CurrentContainer()->SimpleBind( NULL, WithEventsVar->GetName());

    while (PossibleWithEventsProperty &&
           !(PossibleWithEventsProperty->IsProperty() &&
                PossibleWithEventsProperty->CreatedByWithEventsDecl() == WithEventsVar))
    {
        PossibleWithEventsProperty = PossibleWithEventsProperty->GetNextOfSameName();
    }

    return PossibleWithEventsProperty->PProperty();
}


BCSYM_Variable*
Bindable::BindWithEventsVariable
(
    _In_z_ STRING *WithEventVarName,
    bool &InAccessible,
    bool &WithEventsVarFoundInBase
)
{
    InAccessible = false;
    WithEventsVarFoundInBase = false;

    VSASSERT( CurrentContainer()->IsClass(), "How can a non-class have handles clauses ?");

    BCSYM_Class *Class = CurrentContainer()->PClass();
    bool IsShadowed = false;

    BCSYM_NamedRoot *Member;
    for(Member = Class->SimpleBind(NULL, WithEventVarName);
        Member;
        Member = Member->GetNextOfSameName())
    {
        if (Member->IsVariable() &&
            Member->PVariable()->GetVarkind() == VAR_WithEvents)
        {
            return Member->PVariable();
        }

        // Ignore synthesized properties in checking for shadowing.

        if (!Member->CreatedByWithEventsDecl() || !Member->IsProperty())
        {
            IsShadowed = true;
        }
    }


    for(BCSYM_Class *Base = Class->GetCompilerBaseClass();
        !IsShadowed && Base;
        Base = Base->GetCompilerBaseClass())
    {
        for(Member = Base->SimpleBind(NULL, WithEventVarName);
            Member;
            Member = Member->GetNextOfSameName())
        {
            if (Member->IsVariable() &&
                Member->PVariable()->GetVarkind() == VAR_WithEvents)
            {
                if (IsAccessible(Member))
                {
                    WithEventsVarFoundInBase = true;
                    return Member->PVariable();
                }

                InAccessible = true;
            }
            else
            {
                if (IsAccessible(Member) &&
                    // Ignore synthesized properties in checking for shadowing.
                    (!Member->CreatedByWithEventsDecl() || !Member->IsProperty()))
                {
                    IsShadowed = true;
                }
            }
        }
    }

    return NULL;
}

BCSYM_Property*
Bindable::SynthesizeWithEventsProperty
(
    BCSYM_Variable *WithEventsVar,
    Symbols &SymbolFactory
)
{
    VSASSERT(CurrentContainer()->IsClass(),
                    "why would a non-class need to synthesize withevents properties ?");

    BCSYM_Class *Class = CurrentContainer()->PClass();
    BCSYM_Property *WithEventsProperty;

    WithEventsProperty =
        Declared::BuildWithEventsProperty(
            WithEventsVar->PVariable(),
            DeriveGenericBindingForMemberReference(
                IsGenericOrHasGenericParent(CurrentContainer()) ?
                    (BCSYM *)SynthesizeOpenGenericBinding(Class, SymbolFactory) :
                    CurrentContainer(),
                WithEventsVar,
                SymbolFactory,
                CurrentCompilerHost()),
            &SymbolFactory,
            // note to GenSyntheticCode() that generated code should call the base class.
            // Always safe because we only get here if we tried to handle an event that
            // was defined in a base class of the current one
            //
            true,
            Class->IsStdModule(),
            CurrentCompilerInstance(),
            NULL,   // No symbols list to put the property in, we put it in the hash manually below
            NULL);  // No symbols list to put the get, set in, we put it in the hash manually below

    WithEventsProperty->SetMemberThatCreatedSymbol(WithEventsVar);

    WithEventsProperty->GetProperty()->SetMemberThatCreatedSymbol(WithEventsVar);
    WithEventsProperty->SetProperty()->SetMemberThatCreatedSymbol(WithEventsVar);

    // Add the Property to the container's hash
    //
    Symbols::AddSymbolToHash(
        Class->GetHash(),
        WithEventsProperty,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace


    VSASSERT( Class->GetUnBindableChildrenHash(),
                "How can a class not have an unbindable members hash ?");

    // Add the get accessor of the Property to the container's unbindable members hash
    //
    Symbols::AddSymbolToHash(
        Class->GetUnBindableChildrenHash(),
        WithEventsProperty->GetProperty(),
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

    // Add the set accessor of the Property to the container's unbindable members hash
    //
    Symbols::AddSymbolToHash(
        Class->GetUnBindableChildrenHash(),
        WithEventsProperty->SetProperty(),
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

#if IDE

    // Add these to the list of symbols created in bindable.
    // This list is used to delete these from their respective hashes during decompilation
    // from bindable.
    //
    SymbolFactory.GetAliasOfSymbol(
        WithEventsProperty,
        ACCESS_Private,
        CurrentContainer()->GetBindableSymbolList());

    SymbolFactory.GetAliasOfSymbol(
        WithEventsProperty->GetProperty(),
        ACCESS_Private,
        CurrentContainer()->GetBindableSymbolList());

    SymbolFactory.GetAliasOfSymbol(
        WithEventsProperty->SetProperty(),
        ACCESS_Private,
        CurrentContainer()->GetBindableSymbolList());

#endif

    return WithEventsProperty;
}


/*****************************************************************************
;ValidateHandlesLists

Given Method() Handles X1.SomeEvent

    1. verify that X1 is a valid WithEvents variable
    2. verify that X1 sources SomeEvent()
*****************************************************************************/
void
Bindable::ValidateAndHookUpHandles
(
    BCSYM_HandlesList *HandlesEntry,
    BCSYM_MethodImpl *HandlingMethod
)
{
    VSASSERT( !HandlesEntry->IsBad(), "Bad handles unexpected here!!");
    VSASSERT( CurrentContainer()->IsClass(),
                    "Non-classes/non-structures/non-modules cannot have handles clauses!!");

    // Dev10#489103: if the handling method is bad, then we can't really tell whether it handles the event:
    if (HandlingMethod->IsBadParamTypeOrReturnType())
    {
        HandlesEntry->SetIsBad();
        return;
    }

    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    BCSYM_Class *ContainerOfHandlingMethod = CurrentContainer()->PClass();

    BCSYM *TypeToFindEventIn;
    BCSYM_Property *EventSourceProperty = NULL;
    BCSYM_Property *WithEventsProperty;
    BCSYM *InstanceTypeThroughWhichEventIsAccessed;

    if (HandlesEntry->IsMyBase())
    {
        TypeToFindEventIn = TypeHelpers::GetBaseClass(ContainerOfHandlingMethod, SymbolFactory);

        // A bad base is no base
        if (!TypeToFindEventIn)
        {
            HandlesEntry->SetIsBad();
            return;
        }

        InstanceTypeThroughWhichEventIsAccessed = ContainerOfHandlingMethod;
    }
    else if (HandlesEntry->IsEventFromMeOrMyClass())
    {
        if (ContainerOfHandlingMethod->IsGeneric())
        {
            TypeToFindEventIn =
                SynthesizeOpenGenericBinding(
                    ContainerOfHandlingMethod,
                    SymbolFactory);
        }
        else
        {
            TypeToFindEventIn = ContainerOfHandlingMethod;
        }

        InstanceTypeThroughWhichEventIsAccessed = ContainerOfHandlingMethod;
    }
    else
    {
        WithEventsProperty = HandlesEntry->GetWithEventsProperty();

        VSASSERT( WithEventsProperty,
                        "How can a non-bad handles entry not have a withevents property ?");

        BCSYM_Variable *WithEventsVar = WithEventsProperty->CreatedByWithEventsDecl();

        VSASSERT( WithEventsVar,
                        "How can a non-bad handles entry not refer to a withevents var ?");

        if (WithEventsVar->IsBad() ||
            WithEventsVar->IsBadVariableType())
        {
            HandlesEntry->SetIsBad();
            return;
        }

        BCSYM *TypeOfWithEventsVar = WithEventsProperty->GetType();

        VSASSERT( TypeOfWithEventsVar->IsContainer() ||
                  (TypeOfWithEventsVar->IsGenericParam() &&
                        TypeOfWithEventsVar->PGenericParam()->IsReferenceType()),
                            "What else can the type of a withevents variable be ?");

        if (HandlesEntry->GetEventSourcePropertyName())
        {
            // If withevents variable type is type parameter, then search in its class constraint,
            // else search in the withevents variable type itself.
            //
            EventSourceProperty =
                GetEventSourceProperty(
                    TypeOfWithEventsVar,
                    HandlesEntry->GetEventSourcePropertyName());

            if (!EventSourceProperty)
            {
                // "'Handles' must specify a 'WithEvents' variable or 'MyBase' qualified with a single identifier."

                ReportErrorOnSymbol(
                    IsStdModule(CurrentContainer()) ?
                        ERRID_HandlesSyntaxInModule :
                        ERRID_HandlesSyntaxInClass,
                    CurrentErrorLog(HandlingMethod),
                    HandlesEntry);

                HandlesEntry->SetIsBad();
                return;
            }

            // 






            HandlesEntry->SetEventSourceProperty(EventSourceProperty);

            // 


            VSASSERT( EventSourceProperty->GetType()->IsContainer(),
                            "Referring Property - What else can a type that is not a named type be ?");

            TypeToFindEventIn = EventSourceProperty->GetType();
        }
        else
        {
            TypeToFindEventIn = TypeOfWithEventsVar;
        }

        InstanceTypeThroughWhichEventIsAccessed = TypeToFindEventIn;
    }

    VSASSERT( TypeToFindEventIn, "No type to find the event in ?");

    bool IsBadName;
    BCSYM_NamedRoot *EventDecl =
        Semantics::EnsureNamedRoot
        (
            Semantics::InterpretName
            (
                HandlesEntry->GetEventName(),
                *(HandlesEntry->GetLocationOfEvent()),
                TypeToFindEventIn->IsContainer() ?
                    TypeToFindEventIn->PContainer()->GetHash() :
                    NULL,
                TypeToFindEventIn->IsGenericParam() ?
                    TypeToFindEventIn->PGenericParam() :
                    NULL,
                NameSearchIgnoreParent | NameSearchEventReference | NameSearchIgnoreExtensionMethods,
                InstanceTypeThroughWhichEventIsAccessed,
                ContainerOfHandlingMethod->GetHash(),     // Current Context
                CurrentErrorLog(HandlingMethod),
                CurrentCompilerInstance(),
                CurrentCompilerHost(),
                m_CompilationCaches,
                CurrentSourceFile(HandlingMethod),
                false,                                    // don't perform obsolete checks
                IsBadName,
                NULL,   // the binding context is not required here since the synthesized
                        // code has the withevents variable from which the type binding
                        // information is obtained
                NULL,
                -1
            )
        );


    if (IsBadName)
    {
        HandlesEntry->SetIsBad();
        return;
    }

    if (!EventDecl || !EventDecl->IsEventDecl())
    {
        ReportErrorAtLocation(
            ERRID_EventNotFound1,
            CurrentErrorLog(HandlingMethod),
            HandlesEntry->GetLocationOfEvent(),
            HandlesEntry->GetEventName());

        HandlesEntry->SetIsBad();
        return;
    }

    if (EventDecl->IsBad())
    {
        HandlesEntry->SetIsBad();
        return;
    }


    BCSYM_EventDecl *Event = EventDecl->PEventDecl();
    HandlesEntry->SetEvent(Event);

    // Microsoft - 11/17/2005
    // See Bug # 544269
    // The call to ResolveAllNamedTypesForContainer needed to be replaced with a call
    // to ResolveShadowingOverloadingAndOverridingForContainer because event signatures
    // are not necessarily setup untill after ResolveShadowingOverloadingAndOverriding
    // has been called.

    // Go and bind the event we are comparing against up to the state we need for
    // validating handles.
    ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Event->GetContainer(), m_CompilationCaches);


    // Check for bad param types here (VSW#172753).
    if (Event->GetDelegate() &&
        Event->GetDelegate()->IsContainer() &&
        DefinedInMetaData(Event->GetDelegate()->PContainer()))
    {
        for (BCSYM_Param *Param = Event->GetFirstParam();
             Param;
             Param = Param->GetNext())
        {
            if (Param->GetType()->IsBad())
            {
                Param->GetType()->PNamedRoot()->ReportError(
                    CurrentCompilerInstance(),
                    CurrentErrorLog(HandlingMethod),
                    HandlesEntry->GetLocationOfEvent());

                HandlesEntry->SetIsBad();
                return;
            }
        }
    }

    BCSYM_GenericBinding *EventGenericBindingContext =
        DeriveGenericBindingForMemberReference(TypeToFindEventIn, Event, SymbolFactory, CurrentCompilerHost());

   MethodConversionClass MethodConversion = Semantics::ClassifyMethodConversion(
            HandlingMethod,
            GenericBindingInfo(),
            Event,
            EventGenericBindingContext,
            false, //IgnoreReturnValueErrorsForInference
            &SymbolFactory,
            CurrentCompilerHost());

    SourceFile * SourceFile = HandlingMethod->GetSourceFile();

    if (!Semantics::IsSupportedMethodConversion(
            SourceFile->GetOptionFlags() & OPTION_OptionStrict,
            MethodConversion))
    {
        // "Method '|1' cannot handle Event '|2' because they do not have the same signature."

        StringBuffer HandlingMethodRepresentation;
        HandlingMethod->GetBasicRep(CurrentCompilerInstance(), NULL, &HandlingMethodRepresentation);

        StringBuffer EventRepresentation;
        Event->GetBasicRep(CurrentCompilerInstance(), NULL, &EventRepresentation, EventGenericBindingContext);

        ReportErrorAtLocation(
            ERRID_EventHandlerSignatureIncompatible2,
            CurrentErrorLog(HandlingMethod),
            HandlesEntry->GetLocationOfEvent(),
            HandlingMethodRepresentation.GetString(),
            EventRepresentation.GetString());

        HandlesEntry->SetIsBad();
        return;
    }

    // Hook up handlers to the events they specify in their handles clauses

    if (HandlesEntry->IsMyBase() ||
        HandlesEntry->IsEventFromMeOrMyClass())
    {
        // For these cases, we search for valid MyBase, Me, MyClass handles clauses and set up the
        // addhandlers in all constructors that directly call a base constructor. This is done in
        // semantics in initializefields because that is when the information whether it directly
        // calls the base constructor is known.
        //

        const bool BuildSharedConstructor = true;

        if (HandlesEntry->GetEvent()->IsShared() &&
            HandlingMethod->IsShared())
        {
            // Make sure a shared constructor is present, else synthesize one

            if (!ContainerOfHandlingMethod->GetSharedConstructor(CurrentCompilerInstance()))
            {
                // Synthesize one
                SynthesizeConstructor(BuildSharedConstructor);
            }
        }
        else
        {
            // Make sure an Instance constructor is present, else synthesize one

            if (!ContainerOfHandlingMethod->GetFirstInstanceConstructor(CurrentCompilerInstance()))
            {
                // Synthesize one
                SynthesizeConstructor(!BuildSharedConstructor);
            }
        }
    }
    else
    {
        VSASSERT( WithEventsProperty->SetProperty(),
                        "How can there be no set accessor for a withevents property ?");


        if (WithEventsProperty->IsShared() &&
            !HandlingMethod->IsShared())
        {
            // "Events of shared WithEvents variables cannot be handled by non-shared methods."

            ReportErrorAtLocation(
                ERRID_SharedEventNeedsSharedHandler,
                CurrentErrorLog(HandlingMethod),
                HandlesEntry->GetLocationOfWithEventsVar());

            HandlesEntry->SetIsBad();
            return;
        }

        // 

        // hookup the handler list info for the above handles entry on to the WithEvents_Set.
        // GenWithEventsSetCode() will write synthetic code for these later
        //
        WithEventsProperty->SetProperty()->AddToHandlerList(
            HandlingMethod,
            Event,
            EventGenericBindingContext,
            EventSourceProperty,
            CurrentAllocator());

    }
}


BCSYM_Property*
Bindable::GetEventSourceProperty
(
    BCSYM *TypeOfWithEventsVar,
    _In_z_ STRING *PropertyName
)
{
    // Need to find a property that sources an event

    // To handle events on subobjects, (internal use only), you must supply a Class
    // type for the WithEvents variable. But for completeness with respect to generics,
    // adding support type parameters with class constraints too.

    BCSYM *TypeToFindEventSourcePropertyIn;

    if (TypeOfWithEventsVar->IsGenericParam())
    {
        // 
        TypeToFindEventSourcePropertyIn =
            GetClassConstraint(
                TypeOfWithEventsVar->PGenericParam(),
                CurrentCompilerHost(),
                CurrentAllocator(),
                false, // don't ReturnArraysAs"System.Array"
                true   // ReturnValuesAs"System.ValueType"or"System.Enum"
                );
    }
    else
    {
        TypeToFindEventSourcePropertyIn = TypeOfWithEventsVar;
    }

    if (!TypeToFindEventSourcePropertyIn ||
        !TypeToFindEventSourcePropertyIn->IsClass())
    {
        return NULL;
    }

    // ---- attributes on all symbols in the TypeOfWithEventsVar container. This is needed
    // in order to verify if a property is an EventSource which is determined by
    // an attribute on the property.
    //
    ----AttributesOnAllSymbolsInContainer(TypeToFindEventSourcePropertyIn->PClass(), m_CompilationCaches);

    BCSYM_NamedRoot *Property =
        TypeToFindEventSourcePropertyIn->PClass()->SimpleBind(NULL, PropertyName);

    while (Property && Property->IsProperty())
    {
        if (ValidEventSourceProperty(Property->PProperty()) &&
            IsAccessible(
                Property->PProperty()->GetProperty(),
                TypeToFindEventSourcePropertyIn->IsGenericBinding() ?
                    TypeToFindEventSourcePropertyIn->PGenericBinding() :
                    NULL,
                CurrentContainer(),
                TypeOfWithEventsVar))
        {
            return Property->PProperty(); // found it
        }

        Property = Property->GetNextOfSameName();

        // Note:
        // - we don't support finding the Property if it is overloaded
        // across classes.
        // - Also note that we don't support digging into bases. This was
        // how the previous impl. was.
        // Given that this is not a user feature, we don't bother doing
        // this extra work.

    }

    return NULL;
}


bool
Bindable::ValidEventSourceProperty
(
    BCSYM_Property *Property
)
{
    return
        Property->ReturnsEventSource() &&
        Property->GetParameterCount() == 0 &&
        Property->GetProperty() &&
        // We only care about properties returning classes or interfaces
        // because only classes and interfaces can be specified in a
        // handles clause.
        //
        IsClassOrInterface(Property->GetType());
}

BCSYM_Proc*
Bindable::SynthesizeConstructor
(
    bool SynthesizeSharedConstructor
)
{
    VSASSERT(CurrentContainer()->IsClass(),
                    "why would a non-class need to synthesize constructors ?");

    BCSYM_Class *Class = CurrentContainer()->PClass();

    Symbols
        SymbolAllocator(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

	Declared::SyntheticConstructorTypeEnum makeWhat = Declared::SyntheticConstructorType::Instance;
	if (SynthesizeSharedConstructor) {
		makeWhat = Declared::SyntheticConstructorType::Shared;
	} else if (Class->IsMustInherit()) {
		makeWhat = Declared::SyntheticConstructorType::MustInherit;
	}
    BCSYM_Proc *Constructor =
        Declared::BuildSyntheticConstructor(
            &SymbolAllocator,
            CurrentCompilerInstance(),
            NULL,
			makeWhat);  // No Symbol list to add this symbol to
                    // We will add it manually to the hash

    VSASSERT( Constructor,
                    "Unexpected failure in synthesizing constructor");

    // Add the Consatructor to the container's hash
    //
    Symbols::AddSymbolToHash(
        Class->GetHash(),
        Constructor,
        true,       // set the parent of this member
        Class->IsStdModule(),
        false);     // container is not namespace

#if IDE

    // Add this to the list of symbols created in bindable.
    // This list is used to delete these from their respective
    // hashes during decompilation from bindable.
    //
    SymbolAllocator.GetAliasOfSymbol(
        Constructor,
        ACCESS_Private,
        CurrentContainer()->GetBindableSymbolList());
#endif

    return Constructor;
}

