//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Does the work need to bind implementing members to implemented members.
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

int _cdecl SortImplementsHelper( const void *arg1, const void *arg2);

/*****************************************************************************
;ResolveImplements

Hooks the implementing methods to the interface members they implement.
Also checks custom attributes
*****************************************************************************/
void
Bindable::ResolveImplementsInContainer()
{

    VSASSERT( GetStatusOfResolveImplementsInContainer() != InProgress,
                    "How can we ever get into this scenario ?");

    if (GetStatusOfResolveImplementsInContainer() == Done)
    {
        return;
    }

    SetStatusOfResolveImplementsInContainer(InProgress);

    BCSYM_Container *Container = CurrentContainer();

    if (Container->IsBad() ||
        (!IsClass(Container) &&
            !IsStructure(Container)))
    {
        SetStatusOfResolveImplementsInContainer(Done);
        return;
    }

    // Resolve implements for bases first
    if (BCSYM_Class *Base = Container->PClass()->GetCompilerBaseClass())
    {
        ResolveImplementsInContainer(Base, m_CompilationCaches);
    }

    // Note: Shadowing Semantics for the Implemented Interfaces are ensured to be done in "AddInterfacesClassImplements"

    // do implements semantics for this class
    //
    // Verify that all interfaces that a class claim to implement are actually fully implemented.
    CheckImplementsForClass(Container->PClass());

    SetStatusOfResolveImplementsInContainer(Done);
}


/*****************************************************************************
;CheckImplementsForClass

Verifies that the events and implemented interfaces on this class are correctly defined.

1. Build a list of all of the interfaces implemented by this class,
    including base interfaces.
2. Walk all of the members of the class and "bind" their implements
    clauses to the member they're implementing.  Add that member to
    the existance tree.
3. Walk all of the interfaces implemented by this class and make sure
    that each member of that interface is defined in the existance
    tree
*****************************************************************************/
void
Bindable::CheckImplementsForClass(BCSYM_Class *Class)
{
    NorlsAllocator  TreeAllocator(NORLSLOC);

     // All the interfaces (and their bases) that need to be implemented by this class
    DynamicArray<ImplementsInfo>  ListOfInterfacesToBeImplemented;

    // the list of interfaces that actually get implemented
    MemberBindingHashTable ImplementedMembers(&TreeAllocator);

    // The symbol allocator to use for creating generic bindings
    Symbols
        SymbolFactory(
            CurrentCompilerInstance(),
            CurrentAllocator(),
            NULL,
            CurrentGenericBindingCache());

    // We only want to build this list up if this class claims to implement an interface.
    // What the base class claims to implement was dealt with when the base class came
    // through CheckImplementsForClass()
    //
    if (BCITER_ImplInterfaces::HasImplInterfaces(Class))
    {
        // Build the list of interfaces and base interfaces that must be implemented by this class.
        BuildListOfImplementedInterfaces(Class, &ListOfInterfacesToBeImplemented, &SymbolFactory);
    }

    // Wander over the members of this class and check off the interface members they actually implement
    BCITER_CHILD_SORTED ClassIter(Class);
    while (BCSYM_NamedRoot *ImplementingMember = ClassIter.GetNext())
    {
        // Bug fix for whidbey 32593
        if (ImplementingMember->IsBad())
        {
            continue;
        }

        if (ImplementingMember->IsMethodImpl() ||
            ImplementingMember->IsProperty() ||
            ImplementingMember->IsEventDecl())
        {
            BindImplements(ImplementingMember->PProc(), &ListOfInterfacesToBeImplemented, &ImplementedMembers, &SymbolFactory);
        }
    }

    // Make sure everything is implemented.
    CheckForCompletelyImplementedInterfaces(Class, &ListOfInterfacesToBeImplemented, &ImplementedMembers);
}

/*****************************************************************************
;CheckForCompletelyImplementedInterfaces

Make sure that all of the interfaces implemented by this class, both
directly and indirectly, are fully implemented.
*****************************************************************************/
void Bindable::CheckForCompletelyImplementedInterfaces
(
    BCSYM_Class *Class, // [in] the class doing the implementing
    DynamicArray<ImplementsInfo> *ListOfInterfacesToImplement,  // [in] the list of interfaces we are supposed to implement for Class
    MemberBindingHashTable *ImplementedInterfaceMembers // [in] the list of interface members that were implemented
)
{
    unsigned NumInterfacesToImplement     = ListOfInterfacesToImplement->Count();
    ImplementsInfo *InterfacesToImplement = ListOfInterfacesToImplement->Array();

    for ( unsigned InterfaceIdx = 0; InterfaceIdx < NumInterfacesToImplement; InterfaceIdx++ )
    {
        bool OnlyCheckForCompleteness = false;

        // Skip over the interfaces that a base class claimed to implement - we will catch the situation where a base class didn't implement
        // everyone it was supposed to when that class comes through ResolveImplements()
        if ( InterfacesToImplement[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::ImplementedOnBaseClass ||
                InterfacesToImplement[ InterfaceIdx ].m_IsBad ||
                InterfacesToImplement[ InterfaceIdx ].IsReimplementation())    // Reimplementations don't have to reimplement all
                                                                               // Members.
        {
            if(InterfacesToImplement[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::DirectlyImplemented &&
                InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->IsReimplementingInterface())
            {
                // Bug #104767 - DevDiv Bugs: need to keep track of whether all members of this interface are reimplemented by this class
                OnlyCheckForCompleteness = true;
            }
            else
            {
                continue;
            }
        }

        BCITER_CHILD InterfaceIter;
        InterfaceIter.Init( InterfacesToImplement[ InterfaceIdx ].m_Interface->PInterface() );

        // 



        // Haul through the members on the interfaces we were supposed to implement and make sure that they were actually implemented
        bool allMembersAreImplemented = true;

        while ( BCSYM_NamedRoot *InterfaceMember = InterfaceIter.GetNext())
        {
            if (!InterfaceMember->IsContainer() &&
                InterfaceMember->GetBindingSpace() != BINDSPACE_IgnoreSymbol &&
                !ImplementedInterfaceMembers->Exists(
                    InterfaceMember,
                    InterfacesToImplement[InterfaceIdx].m_Interface->IsGenericTypeBinding() ?
                        InterfacesToImplement[InterfaceIdx].m_Interface->PGenericTypeBinding() :
                        NULL))
            {
                if ( InterfaceMember->IsBad())
                {
                    allMembersAreImplemented = false;

                    if(OnlyCheckForCompleteness)
                    {
                        AssertIfTrue(allMembersAreImplemented);
                        break;
                    }

                    VSASSERT( InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->GetRawRoot()->IsNamedType(),
                                    "How can an valid class level implements clause not have a named type ?");

                    InterfaceMember->ReportError(
                        CurrentCompilerInstance(),
                        CurrentErrorLog(InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->GetRawRoot()->PNamedType()),
                        InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->GetLocation());
                }
                else
                {
                    // #282112 - some things (like _VtblGap8()) are hidden and shouldn't be implemented.
                    if ( !InterfaceMember->IsProc() || InterfaceMember->PProc()->DoesntRequireImplementation())
                    {
                        continue;
                    }

                    allMembersAreImplemented = false;

                    if(OnlyCheckForCompleteness)
                    {
                        AssertIfTrue(allMembersAreImplemented);
                        break;
                    }

                    VSASSERT( InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->GetRawRoot()->IsNamedType(),
                                    "How can an valid class level implements clause not have a named type ?");

                    ERRID errid = InterfaceMember->IsProperty() ?
                                    ERRID_UnimplementedProperty3 :
                                    ((InterfaceMember->IsEventDecl() && InterfaceMember->PProc()->GetType()) ?
                                        ERRID_UnimplementedBadMemberEvent :
                                        ERRID_UnimplementedMember3);
                    StringBuffer Signature;
                    InterfaceMember->GetBasicRep(
                        CurrentCompilerInstance(),
                        InterfacesToImplement[InterfaceIdx].m_Interface->PInterface(),
                        &Signature,
                        InterfacesToImplement[InterfaceIdx].m_Interface->IsGenericBinding() ?
                            InterfacesToImplement[InterfaceIdx].m_Interface->PGenericBinding() :
                            NULL);

                    // "|1 '|2' must implement '|3' for interface '|4'." or
                    // "|1 '|2' cannot implement interface '|4' because it declares '|4' which has a return type."(VSW357546)
                    ReportErrorOnSymbol(
                        errid,
                        CurrentErrorLog(InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->GetRawRoot()->PNamedType()),
                        InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause, // specifies the interface that wasn't fully implemented
                        Class->IsStruct() ?
                            L"Structure" :
                            L"Class",
                        Class->GetName(),       // the class doing the implementing
                        Signature.GetString(),  // member that wasn't implemented
                        GetQualifiedErrorName(InterfacesToImplement[ InterfaceIdx ].m_Interface));
                }
            }
        }

        if(OnlyCheckForCompleteness && allMembersAreImplemented)
        {
            InterfacesToImplement[ InterfaceIdx ].m_ImplementsClause->SetAreAllMembersReimplemented(true);
        }
    }
}

/*****************************************************************************
;BuildListOfImplementedInterfaces

Builds a list of implemented interfaces and generates errors if we have
any bad implements situations ( i.e. implementing an interface twice or
implementing an interface already implemented by a base class).
*****************************************************************************/
void Bindable::BuildListOfImplementedInterfaces
(
    BCSYM_Class *Class, // [in] the class implementing the interfaces
    DynamicArray<ImplementsInfo> *ListOfInterfacesToBeImplemented, // [in] we put all the interfaces implemented by this class in here
    Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
)
{
    unsigned ImplementedInterfacesIdx;

    // Add the list of interfaces for this Class's IMPLEMENTS list
    //
    AddInterfacesClassImplements(
        Class,
        ListOfInterfacesToBeImplemented,
        ImplementsInfo::DirectlyImplemented,
        false,
        SymbolFactory);

    // Walk the list of interfaces the class directly implements and add the base interfaces of those interfaces
    // (and their bases, recursively)
    //
    for (ImplementedInterfacesIdx = 0;
        ImplementedInterfacesIdx < ListOfInterfacesToBeImplemented->Count();
        ImplementedInterfacesIdx++ ) // TRICK: this also runs through the base interfaces of the base interfaces
                                     // entries added during the loop since they are added to the end of the array
                                     // as we go
    {
        ImplementsInfo *ImplementsInfo = &ListOfInterfacesToBeImplemented->Array()[ ImplementedInterfacesIdx ];

        AddBaseInterfaces(
            ImplementsInfo->m_Interface,
            ListOfInterfacesToBeImplemented,
            ImplementsInfo::BaseInterface,
            ImplementsInfo->m_ImplementsClause,
            ImplementsInfo->m_Class,
            SymbolFactory);
    }

    // Add the interfaces that all base classes of Class claim to implement to the array.
    //
    for (BCSYM *BaseClass = TypeHelpers::GetBaseClass(Class, *SymbolFactory);
         BaseClass;
         BaseClass = TypeHelpers::GetBaseClass(BaseClass, *SymbolFactory))
    {
        AddInterfacesClassImplements(
            BaseClass,
            ListOfInterfacesToBeImplemented,
            ImplementsInfo::ImplementedOnBaseClass,
            true /* is base class */,
            SymbolFactory);
    }

    // Walk the base interfaces of those interfaces added while adding the implemented interfaces implemented
    // by each base class of Class, above and add their base interfaces.  TRICK: The base interfaces were added
    // to the array starting at the location we left off in the previous walk - so start there.
    //
    for ( ; ImplementedInterfacesIdx < ListOfInterfacesToBeImplemented->Count(); ImplementedInterfacesIdx++)
    {
        ImplementsInfo *Helper = &ListOfInterfacesToBeImplemented->Array()[ ImplementedInterfacesIdx ];

        AddBaseInterfaces(
            Helper->m_Interface,
            ListOfInterfacesToBeImplemented,
            ImplementsInfo::ImplementedOnBaseClass,
            NULL,
            Helper->m_Class,
            SymbolFactory);
    }

    // ** At this point, the array contains ALL the interfaces and base interfaces that must be implemented in order
    // for this class (and its bases), to successfully implement the specified interface(s)

    // Sort the array of interfaces to implement - this will aid the detection of duplicates later
    //
    if (ListOfInterfacesToBeImplemented->Count() != 0) // check for this or qsort has a tendency to do memory overwrites
    {
        qsort(
            ListOfInterfacesToBeImplemented->Array(),
            ListOfInterfacesToBeImplemented->Count(),
            sizeof(ImplementsInfo),
            SortImplementsHelper);
    }

    // Walk the array squeezing out duplicates.  Give an error for the following situations:
    //   1. The class claims to directly implement the same interface more than once
    //   2. An interface is directly implemented by the class and by its base class

    unsigned ReadIdx, WriteIdx, Count = ListOfInterfacesToBeImplemented->Count();
    ImplementsInfo *ImplementsInfo    = ListOfInterfacesToBeImplemented->Array();

    for (ReadIdx = WriteIdx = 0; ReadIdx < Count; ReadIdx++)
    {
        // Figure out if we should copy this one/give an error.  This will 'continue' if we should skip this one.
        // Always copy the first one.
        //
        if (ReadIdx > 0)
        {
            unsigned CompareFlags =
                BCSYM::CompareReferencedTypes(
                    ImplementsInfo[WriteIdx - 1].m_Interface,
                    (GenericBinding *)NULL,
                    ImplementsInfo[ReadIdx].m_Interface,
                    (GenericBinding *)NULL,
                    SymbolFactory);

            // Is the current interface in the list the same as the last interface we looked at?
            if ((CompareFlags & EQ_Shape) == 0)
            {
                // Is the current interface in the list the same as the last interface we looked at?
                if ((CompareFlags & EQ_GenericTypeParams) == 0)
                {
                    // If both are directly implemented then give an error.
                    if (ImplementsInfo[WriteIdx - 1].m_WhereFrom == ImplementsInfo::DirectlyImplemented &&
                        ImplementsInfo[ReadIdx].m_WhereFrom == ImplementsInfo::DirectlyImplemented)
                    {
                        VSASSERT( ImplementsInfo[ReadIdx].m_ImplementsClause->GetRawRoot()->IsNamedType(),
                                        "How can an valid class level implements clause not have a named type ?");

                        BCSYM_Container *ContextForFirstImplements =
                            ImplementsInfo[WriteIdx - 1].m_ImplementsClause->GetRawRoot()->PNamedType()->GetContext()->PContainer();

                        BCSYM_Container *ContextForSecondImplements =
                            ImplementsInfo[ReadIdx].m_ImplementsClause->GetRawRoot()->PNamedType()->GetContext()->PContainer();

                        // Report dup error only if both the implements clauses are specified in the same partial container,
                        // else implements are suppposed to be unioned across the various different partial types.
                        //
                        if (ContextForFirstImplements == ContextForSecondImplements)
                        {
                            ReportErrorOnSymbol(
                                ERRID_InterfaceImplementedTwice1,
                                CurrentErrorLog(ImplementsInfo[ ReadIdx ].m_ImplementsClause->GetRawRoot()->PNamedType()),
                                ImplementsInfo[ ReadIdx ].m_ImplementsClause,
                                GetQualifiedErrorName(ImplementsInfo[ ReadIdx ].m_Interface));
                        }
                        else
                        {
                            VSASSERT(ContextForFirstImplements->IsSameContainer(ContextForSecondImplements),
                                        "Duplicate implements error reporting across different types unexpected!!!");
                        }

                        ImplementsInfo[ ReadIdx ].m_ImplementsClause->SetIsRedundantImplements(true);
                    }
                    // If the class directly implements an interface that its base class directly implements, then give an error.
                    //
                    else if (ImplementsInfo[WriteIdx - 1].m_WhereFrom == ImplementsInfo::ImplementedOnBaseClass &&
                            ImplementsInfo[ReadIdx].m_WhereFrom == ImplementsInfo::DirectlyImplemented)
                    {
                        VSASSERT( ImplementsInfo[ ReadIdx ].m_ImplementsClause->GetRawRoot()->IsNamedType(),
                                        "How can an valid class level implements clause not have a named type ?");

                        ImplementsInfo[ReadIdx].SetOriginalImplementer(
                            ImplementsInfo[WriteIdx - 1].m_Class,
                            (CompareFlags & EQ_GenericTypeParams));

                        ImplementsInfo[ReadIdx].m_ImplementsClause->SetIsReimplementingInterface(true);

                        ImplementsInfo[WriteIdx - 1] = ImplementsInfo[ReadIdx];
                    }
                    // If the interface is indirectly implemented and inherited from the base class then propagate the m_Implements
                    // so we can give  a more meaningful error if the user tries to 'implement' one of the members of this interface.
                    //
                    else if (ImplementsInfo[WriteIdx - 1].m_WhereFrom == ImplementsInfo::ImplementedOnBaseClass &&
                            ImplementsInfo[ReadIdx].m_WhereFrom == ImplementsInfo::BaseInterface)
                    {
                        ImplementsInfo[ReadIdx].SetOriginalImplementer(
                            ImplementsInfo[WriteIdx - 1].m_Class,
                            (CompareFlags & EQ_GenericTypeParams));

                        ImplementsInfo[WriteIdx - 1] = ImplementsInfo[ReadIdx];
                    }
                    // Do not copy.
                    continue;
                }
                else
                {
                    VSASSERT(CompareFlags & EQ_GenericTypeParams, "Interface implements check confused!!!");

                    // If both are directly implemented then give an error.
                    if (ImplementsInfo[WriteIdx - 1].m_WhereFrom == ImplementsInfo::DirectlyImplemented &&
                        ImplementsInfo[ReadIdx].m_WhereFrom == ImplementsInfo::DirectlyImplemented)
                    {
                        VSASSERT( ImplementsInfo[ReadIdx].m_ImplementsClause->GetRawRoot()->IsNamedType(),
                                        "How can an valid class level implements clause not have a named type ?");

                            // Cannot implement interface '|1' because it could possibly clash with another implemented interface '|2' for some type arguments.
                            // or
                            // Cannot implement interface '|1' because its implementation could possibly clash with the implementation of another implemented interface '|2' for some type arguments.

                        ReportErrorOnSymbol(
                            ERRID_InterfacePossiblyImplTwice2,
                            CurrentErrorLog(ImplementsInfo[ReadIdx].m_ImplementsClause->GetRawRoot()->PNamedType()),
                            ImplementsInfo[ReadIdx].m_ImplementsClause,
                            GetQualifiedErrorName(ImplementsInfo[ReadIdx].m_Interface),
                            GetQualifiedErrorName(ImplementsInfo[WriteIdx - 1].m_Interface));

                        // Do not copy.
                        continue;
                    }
                }
            }
        }

        // Copy the helper.
        if (ReadIdx != WriteIdx)
        {
            ImplementsInfo[WriteIdx] = ImplementsInfo[ReadIdx];
        }
        WriteIdx++;
    }

    // Microsoft 9/13/2004:  Since both Count and WriteIdx are unsigned, any operation on them can only produce
    // a positive number.  Thus, the expression as originally written will always be true, and we could in theory
    // try to shrink the list by some insanely large number.  I've rewritten the code to avoid this.
    //if (Count - WriteIdx > 0)
    if (Count > WriteIdx)
    {
        // Squeeze the array which gets rid of all the dupes filtered out above
        //
        ListOfInterfacesToBeImplemented->Shrink(Count - WriteIdx);
    }
}

/*****************************************************************************
;AddInterfacesClassImplements

Add the interfaces the class claims to implement to the array
*****************************************************************************/
void Bindable::AddInterfacesClassImplements
(
    BCSYM *Class, // [in] the class claiming to implement the interface
    DynamicArray<ImplementsInfo> *TheImplementedInterfaces, // [in] we put the interfaces that are implemented by Class in here
    ImplementsInfo::WhereImplementedFrom WhereFrom, // [in] how the class implements this interface
    bool IsBaseClass, // [in] whether Class is a base class
    Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
)
{
    // Add all of the interfaces implemented by this class to TheImplementedInterfaces.
    BCITER_ImplInterfaces ImplInterfacesIter(Class->PClass());
    while (BCSYM_Implements *Implements = ImplInterfacesIter.GetNext())
    {
        if ( Implements->IsBad() || Implements->GetRoot()->IsBad()) continue; // skip over horked implements statements

        BCSYM *Interface = Implements->GetRoot();

        // Verify that the thing we are implementing is a valid interface.
        if ( !Interface->IsInterface()) // if the symbol listed in the implements list isn't an interface then error out
        {
            // Invalid type for use with implements
            if ( !IsBaseClass )
            {
                VSASSERT( Implements->GetRawRoot()->IsNamedType(),
                                "Non-named types (primitive types) at the class level implements should have been made bad by declared!!!");

                ReportErrorOnSymbol( Interface->IsGenericParam() ? ERRID_ImplementsGenericParam : ERRID_BadImplementsType,
                                     CurrentErrorLog(Implements->GetRawRoot()->PNamedType()),
                                     Implements );
            }
            Implements->SetIsBad();
            continue;
        }

        if (Class->IsGenericTypeBinding() && Interface->IsGenericTypeBinding())
        {
            Interface =
                ReplaceGenericParametersWithArguments(
                    Interface,
                    Class->PGenericTypeBinding(),
                    *SymbolFactory);
        }

        ImplementsInfo Info;

        Info.m_Interface = Interface;
        Info.m_WhereFrom = WhereFrom;
        Info.m_ImplementsClause = IsBaseClass ? NULL : Implements; // if we are adding interfaces implemented by a base class, the base class will have claimed to implement it
        Info.m_Class = Class;
        Info.m_IsBad = false;
        Info.m_OriginalImplementer = NULL;
        //Info.m_IsReimplOnlyForSomeTypeArgs = false;

        TheImplementedInterfaces->Add() = Info;

        // Need shadowing for interface done
        ResolveShadowingOverloadingOverridingAndCheckGenericsForContainer(Info.m_Interface->PInterface(), m_CompilationCaches);

        // Need attributes ----ed because we need to need to know if the coclass attribute is present
        ----AttributesOnAllSymbolsInContainer(Info.m_Interface->PInterface(), m_CompilationCaches);

    } // loop through implements
}

/*****************************************************************************
;AddBaseInterfaces

Adds the interfaces inherited by this one to the array.  Does not dig
further, the caller takes care of that.
*****************************************************************************/
void Bindable::AddBaseInterfaces
(
    BCSYM *Interface, // [in] the interface whose base interfaces we are adding
    DynamicArray<ImplementsInfo> *InfoOnImplements, // [in] we put the base interfaces of Interface into here
    ImplementsInfo::WhereImplementedFrom WhereFrom, // [in] where the base interfaces are coming from
    BCSYM_Implements *OriginalImplements, // [in] the implements statement that started us out
    BCSYM *ImplementingClass, // [in] class that implements this chain of implemented interfaces
    Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
)
{
    // Add all of the base interfaces of this interface to the array.
    for ( BCSYM_Implements *Implements = Interface->GetFirstImplements(); Implements; Implements = Implements->GetNext())
    {
        if ( !Implements->IsBad() && !Implements->GetRoot()->IsBad())
        {
            BCSYM *BaseInterface = Implements->GetRoot()->DigThroughAlias();

            if (Interface->IsGenericTypeBinding())
            {
                BaseInterface =
                    ReplaceGenericParametersWithArguments(
                        BaseInterface,
                        Interface->PGenericTypeBinding(),
                        *SymbolFactory);
            }

            ImplementsInfo Info;
            Info.m_Interface  = BaseInterface;
            Info.m_WhereFrom  = WhereFrom;
            Info.m_ImplementsClause = OriginalImplements;
            Info.m_Class      = ImplementingClass;
            Info.m_IsBad = false;
            Info.m_OriginalImplementer = NULL;
            //Info.m_IsReimplOnlyForSomeTypeArgs = false;

            InfoOnImplements->Add() = Info;
        }
    }
}

/*****************************************************************************
;SortImplementsHelpers

Sort the implements helper array for use in qsort.
*****************************************************************************/
int _cdecl SortImplementsHelper( const void *arg1, const void *arg2)
{
    ImplementsInfo *Helper1 = ( ImplementsInfo *)arg1;
    ImplementsInfo *Helper2 = ( ImplementsInfo *)arg2;

    // Sort first by fully qualified name, then by how it's implemented then
    // by the location of the implements symbol.  This sort must always
    // be deterministic or this stuff will be impossible to debug.

    STRING *pstrName1 =
        Helper1->m_Interface->PInterface()->GetQualifiedName(
            false,
            NULL,
            true,
            Helper1->m_Interface->IsGenericBinding() ?
                Helper1->m_Interface->PGenericBinding() :
                NULL);

    STRING *pstrName2 =
        Helper2->m_Interface->PInterface()->GetQualifiedName(
            false,
            NULL,
            true,
            Helper2->m_Interface->IsGenericBinding() ?
                Helper2->m_Interface->PGenericBinding() :
                NULL);

    int Compare = wcscmp( pstrName1, pstrName2);

    if ( Compare != 0)
    {
        return Compare;
    }

    Compare = Helper1->m_WhereFrom - Helper2->m_WhereFrom;

    if ( Compare != 0)
    {
        return Compare;
    }

    if ( Helper1->m_ImplementsClause == Helper2->m_ImplementsClause)
    {
        return 0;
    }

    if ( !Helper1->m_ImplementsClause)
    {
        return -1;
    }
    else if ( !Helper2->m_ImplementsClause)
    {
        return 1;
    }

    Compare = Helper1->m_ImplementsClause->GetLocation()->m_lBegLine - Helper2->m_ImplementsClause->GetLocation()->m_lBegLine;

    if ( Compare != 0)
    {
        return Compare;
    }

    Compare = Helper1->m_ImplementsClause->GetLocation()->m_lBegColumn - Helper2->m_ImplementsClause->GetLocation()->m_lBegColumn;

    return Compare;
}

/*****************************************************************************
;FindImplementedInterfaceMember

Helper to find the member of an implemented interface.

This method handles all errors.
*****************************************************************************/
BCSYM_NamedRoot * // the interface member to implement
Bindable::FindImplementedInterfaceMember
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
)
{
    *HasError = false;
    *HasMatch = false;

    // Search the given interface first.
    bool CurrentlyHaveMatch = false;
    DynamicArray<BCSYM_Proc *> AmbiguousImplementedMembers;

    BCSYM_NamedRoot *InterfaceMemberToImplement = Interface->SimpleBind( NULL, ImplementedMemberName );

    // Ignore the candidate if we are binding in a CoClass context and it doesn't match the kind of member
    // we are looking for (see below).
    if (InterfaceMemberToImplement && HasFlag(Flags, NameSearchCoClassContext))
    {
        if (IsEvent(SignatureToMatch) && !IsEvent(InterfaceMemberToImplement))
        {
            InterfaceMemberToImplement = NULL;
            goto SkipBases;
        }
        else if (!IsEvent(SignatureToMatch) && IsEvent(InterfaceMemberToImplement))
        {
            InterfaceMemberToImplement = NULL;
            goto SkipBases;
        }
    }

    // Are we trying to implement a horked interface member?
    if ( InterfaceMemberToImplement && InterfaceMemberToImplement->IsBad())
    {
        InterfaceMemberToImplement->ReportError( CurrentCompilerInstance(), CurrentErrorLog(SignatureToMatch), ErrorSymbol->GetLocation()); // Reports the error in GetErrId()
        *HasError = true;

        if ( !ProcessingBase )
        {
            SignatureToMatch->SetIsBad(); // It couldn't implement what it wanted so it's bad
        }
        return NULL;
    }

    // Make sure we aren't horked right off the bat in that they are trying to implement something that is overloaded
    if ( !ProcessingBase )
    {
        bool IsBadName = false;
        bool IsAmbigousName = false;

        Semantics::InterpretName(
            ImplementedMemberName,
            *(ErrorSymbol->GetLocation()),
            Interface->GetHash(),
            NULL,   // No Type parameter lookup
            Flags | NameSearchIgnoreParent,
            NULL, /* NULL accessing instance type */
            CurrentContainer()->GetHash(),
            NULL, /* no error log - we want to log our own error */
            CurrentCompilerInstance(),
            CurrentCompilerHost(),
            m_CompilationCaches,
            NULL, /* no source file needed */
            false,  /* No obsolete checking */
            IsBadName,
            NULL, /* No binding context required */
            NULL,
            -1,
            &IsAmbigousName);

        if (IsAmbigousName)
        {
            *HasError = true;
            ReportErrorOnSymbol( ERRID_AmbiguousImplementsMember3, CurrentErrorLog(SignatureToMatch), ErrorSymbol, ImplementedMemberName );
            return NULL;
        }
    }


    // We found a guy by the right name - make sure the signature of the implementing method and the method being implemented on the interface match
    for (BCSYM_NamedRoot *CurrentInterfaceMemberToImplement = InterfaceMemberToImplement;
         CurrentInterfaceMemberToImplement != NULL;
         CurrentInterfaceMemberToImplement = CurrentInterfaceMemberToImplement->GetNextBound())
    {
        DWORD CompareFlags =
            CurrentInterfaceMemberToImplement->IsProc() ?
                BCSYM::CompareProcs(
                    CurrentInterfaceMemberToImplement->PProc(),
                    InterfaceBinding,
                    SignatureToMatch,
                    (GenericBinding *)NULL,
                    SymbolFactory) :
                EQ_Shape;

        if (ONLY_THESE_FLAGS_ARE_SET( EQ_Name | EQ_ParamName | EQ_Flags | EQ_Default | EQ_GenericMethodTypeParams | EQ_GenericMethodTypeParamsForReturn, CompareFlags))
        {
            // Signatures match, but are we implementing like kinds? i.e. an event must implement an event, not a sub of the same signature, etc.
            if (GetSimilarKind( SignatureToMatch->GetKind()) == GetSimilarKind( CurrentInterfaceMemberToImplement->GetKind()))
            {
                if (CurrentlyHaveMatch)
                {
                    VSASSERT(InterfaceMemberToImplement != NULL,
                                    "Inconsistency during Interface member matching!!!");

                    if (AmbiguousImplementedMembers.Count() == 0)
                    {
                        AmbiguousImplementedMembers.Add() = InterfaceMemberToImplement->PProc();
                    }

                    AmbiguousImplementedMembers.AddElement(SignatureToMatch);
                }
                else
                {
                    if (!CompareConstraints(
                            CurrentInterfaceMemberToImplement,
                            InterfaceBinding,
                            SignatureToMatch,
                            NULL,
                            SymbolFactory))
                    {
                        // "'|1' cannot implement '|2.|3' because they differ by their type parameter constraints."

                        ReportErrorOnSymbolWithBindingContexts(
                            ERRID_ImplementsWithConstraintMismatch3,
                            CurrentErrorLog(SignatureToMatch),
                            ErrorSymbol,
                            SignatureToMatch,
                            NULL,                   // Generic binding context
                            Interface,
                            InterfaceBinding,
                            CurrentInterfaceMemberToImplement,
                            DeriveGenericBindingForMemberReference(
                                InterfaceBinding ? (BCSYM *)InterfaceBinding : Interface,
                                CurrentInterfaceMemberToImplement,
                                *SymbolFactory,
                                CurrentCompilerHost()));
                    }

                    CurrentlyHaveMatch = true;
                    InterfaceMemberToImplement = CurrentInterfaceMemberToImplement;

                    VSASSERT(!InterfaceMemberToImplement->HasGenericParent() ||
                             InterfaceBinding != NULL,
                                "Generic binding context expected for member of generic interface!!!");

                    // If member of a generic type, then continue looking to detect ambiguous overrides
                    // that could potentially occur due to method signature unification, else break
                    // and return this matching member.
                    //
                    if (!InterfaceMemberToImplement->HasGenericParent())
                    {
                        break; // we've found a match
                    }
                }
            }
        }
    }

    VSASSERT(AmbiguousImplementedMembers.Count() == 0 ||
             (AmbiguousImplementedMembers.Count() >= 2 && InterfaceBinding),
                "Ambiguity unexpected when only one implemented member found!!!");

    if (AmbiguousImplementedMembers.Count() > 0)
    {
        // "Member '|1.|2' that matches this signature cannot be implemented because '|1' contains multiple members with
        // this same name and signature: |3"

        StringBuffer AmbiguousSignatures;

        ReportErrorOnSymbol(
            ERRID_AmbiguousImplements3,
            CurrentErrorLog(SignatureToMatch),
            ErrorSymbol,
            AmbiguousImplementedMembers.Element(0)->GetName(),
            GetQualifiedErrorName(InterfaceBinding),
            GetSigsForAmbiguousOverridingAndImplErrors(AmbiguousImplementedMembers, AmbiguousSignatures));

        *HasError = true;
        return NULL;
    }

    if ( CurrentlyHaveMatch )
    {
        goto SkipBases;
    }

    // We use IsShadowing here rather than IsShadowsKeywordUsed because we'll have caught a missing Shadows and those
    // are warnings. Since we've made sure the signatures are the same, we don't have to bother with whether this shadows
    // by name or by name and signature.
    if ( InterfaceMemberToImplement && InterfaceMemberToImplement->IsShadowing())
    {
        goto SkipBases; // We're shadowing -- don't look any further.
    }

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
    if (Interface->GetPAttrVals()->GetPWellKnownAttrVals()->GetCoClassData(&CoClassName))
    {
        SetFlag(Flags, NameSearchCoClassContext);
    }

    // Search all of the base interfaces.
    for ( BCSYM_Implements *Implements = Interface->GetFirstImplements();
            Implements;
            Implements = Implements->GetNext())
    {
        if ( Implements->IsBad() || Implements->GetRoot()->IsBad())
        {
            continue;
        }

        BCSYM *ImplementedInterface = Implements->GetRoot()->DigThroughAlias();
        bool BaseHasMatch;
        BCSYM_NamedRoot *MemberFromBase =
            FindImplementedInterfaceMember(
                ImplementedMemberName,
                ImplementedInterface->PInterface(),
                ImplementedInterface->IsGenericTypeBinding() ?
                    ReplaceGenericParametersWithArguments(
                        ImplementedInterface->PGenericTypeBinding(),
                        InterfaceBinding,
                        *SymbolFactory)->PGenericTypeBinding() :
                    NULL,
                SignatureToMatch,
                SymbolFactory,
                Flags,
                true /* processing base interface now */,
                ErrorSymbol,
                HasError,
                &BaseHasMatch);

        if ( *HasError )
        {
            return NULL;
        }

        if ( MemberFromBase ) // No ambiguity between members of the same name on the specified interface and its base interfaces (we check
                                // that first off when entering this function), so use the one we found on the base as the guy we'll implement
        {
            CurrentlyHaveMatch = BaseHasMatch ;
            InterfaceMemberToImplement = MemberFromBase;
        }
    }

SkipBases:

    // No member?  An error if we've recursed through all the bases and still not found it
    if ( !ProcessingBase && !CurrentlyHaveMatch )
    {
        ReportErrorOnSymbol(
            ERRID_IdentNotMemberOfInterface4,
            CurrentErrorLog(SignatureToMatch),
            ErrorSymbol,
            SignatureToMatch->GetName(),
            ImplementedMemberName,
            StringOfSymbol(CurrentCompilerInstance(), SignatureToMatch),
            GetQualifiedErrorName(
                InterfaceBinding ?
                    InterfaceBinding :
                    Interface->PNamedRoot()));

        // Don't mark SignatureToMatch as bad yet because it will circumvent looking for additional implements
        // specified for this symbol.  We'll mark the symbol as bad in the the caller after we run the implements list
    }

    if ( HasMatch )
    {
        *HasMatch = CurrentlyHaveMatch;
    }

    if ( !ProcessingBase && !CurrentlyHaveMatch )
    {
        return NULL;
    }
    else
    {
        return InterfaceMemberToImplement;
    }
}

/*****************************************************************************
;BindImplements

Bind and verify the "implements" list for the method.  It will check the following:
    1. that the named type actually is an interface
    2. that the interface is actually implemented by this class
    3. that the interface ( or a base interface )  actually contains the procedure with the same type/signature as this member
    4. that this member has not already been implemented
*****************************************************************************/
void Bindable::BindImplements
(
    BCSYM_Proc * ImplementingMember, // [in] the class member that is implementing an interface member
    DynamicArray<ImplementsInfo> * TheImplementedInterfaces, // [in] contains a list of interfaces we have implemented so far
    MemberBindingHashTable * ImplementedMembers, // [in] the list of members that are implemented
    Symbols *SymbolFactory // [in] Symbol factory for creating generic bindings
)
{
    BCSYM_ImplementsList *ImplementsList = ImplementingMember->GetImplementsList();

    bool MarkImplementingThingBad = false;
    BCSYM_EventDecl *PreviousImplementedWindowsRuntimeEvent = NULL;
    BCSYM_EventDecl *PreviousImplementedDotNETEvent = NULL;
    // Haul through the things this member claims to implement - mark which interface method is implemented and check for errors.
    for ( ; ImplementsList; ImplementsList = ImplementsList->GetNext() )
    {
        if ( ImplementsList->GetType()->IsBad()) continue; // Ignore bad things.

        if ( !ImplementsList->GetType()->IsInterface())
        {
            ReportErrorOnSymbol( ERRID_BadImplementsType, CurrentErrorLog(ImplementingMember), ImplementsList );
            continue;
        }

        BCSYM *Interface = ImplementsList->GetType()->DigThroughAlias();

        // Make sure that the interface is on the list of ones the class implements
        unsigned InterfaceIdx, NumImplementedInterfaces = TheImplementedInterfaces->Count();
        ImplementsInfo *ImplementedInterfacesList = TheImplementedInterfaces->Array(); // All the interfaces implemented by the class
        for ( InterfaceIdx = 0; InterfaceIdx < NumImplementedInterfaces; InterfaceIdx++ )
        {
            if ( TypeHelpers::EquivalentTypes( ImplementedInterfacesList[ InterfaceIdx ].m_Interface, Interface ) &&
                ( ImplementedInterfacesList[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::DirectlyImplemented ||
                  ImplementedInterfacesList[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::BaseInterface ))
            {
                break;
            }
        }
        // If the interface isn't in the list then give an error and move on to the next entry in the Implements list for the member
        if ( InterfaceIdx == NumImplementedInterfaces )
        {
            ReportErrorOnSymbol(
                ERRID_InterfaceNotImplemented1,
                CurrentErrorLog(ImplementingMember),
                ImplementsList,
                GetQualifiedErrorName(Interface));

            continue;
        }

        // Find the member on the interface that we claim to implement
        bool HasError, HasMatch;
        BCSYM_NamedRoot *InterfaceMemberToImplement =
            FindImplementedInterfaceMember(
                ImplementsList->GetName(),
                Interface->PInterface(),
                Interface->IsGenericTypeBinding() ?
                    Interface->PGenericTypeBinding() :
                    NULL,
                ImplementingMember,
                SymbolFactory,
                ImplementingMember->IsEventDecl() ?
                    NameSearchEventReference :
                    NameNoFlags,
                false, /* this is the specified interface, not a base */
                ImplementsList,
                &HasError,
                &HasMatch);

        if ( !InterfaceMemberToImplement )
        {
            MarkImplementingThingBad = true;
            continue; // appropriate errors logged during FindImplementedInterfaceMember()
        }
        else if ( InterfaceMemberToImplement->IsProc() && InterfaceMemberToImplement->PProc()->DoesntRequireImplementation())
        {
            // "'|1 |2' cannot implement '|3' because '|3' is not virtual.")
            ReportErrorOnSymbol( ERRID_CantImplementNonVirtual3,
                                        CurrentErrorLog(ImplementingMember),
                                        ImplementsList,
                                        StringOfSymbol( CurrentCompilerInstance(), ImplementingMember ), // |1
                                        ImplementingMember->GetName(), // |2
                                        ImplementsList->GetName()); // |3
            ImplementingMember->SetIsBad();
            continue;
        }

        // Switch to the interface the member actually lives on (in case we found the implemented member on a base interface)
        //
        BCSYM_GenericBinding *ActualInterfaceBinding =
            DeriveGenericBindingForMemberReference(
                Interface,
                InterfaceMemberToImplement,
                *SymbolFactory,
                CurrentCompilerHost());

        BCSYM *ActualInterface =
            ActualInterfaceBinding ?
                ActualInterfaceBinding :
                (BCSYM *)InterfaceMemberToImplement->GetContainer();

        bool FoundOnBase = false;
        if ( !TypeHelpers::EquivalentTypes( ActualInterface, Interface ) ) // get InterfaceIdx to match the base Interface we are implementing if we ended up finding a member on a base interface
        {
            FoundOnBase = true; // keep track of the fact that the member was found on a base interface
            for ( InterfaceIdx = 0; InterfaceIdx < NumImplementedInterfaces; InterfaceIdx++ )
            {
                if ( TypeHelpers::EquivalentTypes( ImplementedInterfacesList[ InterfaceIdx ].m_Interface, ActualInterface ) )
                {
                    break;
                }
            }
        }

        VSASSERT( InterfaceIdx < NumImplementedInterfaces &&
                    (ImplementedInterfacesList[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::BaseInterface ||
                     ImplementedInterfacesList[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::DirectlyImplemented),
                        "Reimplemented interfaces should have overridden base interfaces. So this should never occur!!!");

        // Are we trying to implement something that the base class may have implemented?
        if ( ImplementedInterfacesList[ InterfaceIdx ].IsReimplementation() )
        {
            if ( ImplementedInterfacesList[ InterfaceIdx ].m_WhereFrom == ImplementsInfo::BaseInterface )
            {
#if 0
                    // 


                if (ImplementedInterfacesList[InterfaceIdx ].IsReimplOnlyForSomeTypeArgs())
                {
                    // "'|1.|2' from 'implements |3' re-implements '|3.|4' already implemented by the base class '|5' for some type arguments."
                }
                else
#endif
                {
                    // WinRT, as it was designed, doesn't support composability properly and has no concept of re-implementation of 
                    // interfaces. If a composable WinRT type implements an interface without virutal methods, they expect that those
                    // methods cannot be overridden by any derived types. Therefore, if we find that the current interface method implements an 
                    // interface member and that this interface is also implemented by a base type which is also a WinRT type, then we disallow 
                    // this re-implementation.

                    if (ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()->GetPWellKnownAttrVals()->GetWindowsRuntimeImportData())
                    {
                        // "'|1.|2' from 'implements |3' is already implemented by the base class '|4'. Re-implementation of Windows Runtime Interface |4 is not allowed
    
                        ReportErrorOnSymbol(
                            ERRID_ReImplementatingWinRTInterface5,
                            CurrentErrorLog(ImplementingMember),
                            ImplementsList,
                            GetQualifiedErrorName(ActualInterface),
                            InterfaceMemberToImplement->GetName(),
                            GetQualifiedErrorName(Interface),
                            GetQualifiedErrorName(ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()),
                            GetQualifiedErrorName(ActualInterface));
                    }
                    else
                    {
                        // "'|1.|2' from 'implements |3' is already implemented by the base class '|4'. Re-implementation of |5 assumed."

                        ReportErrorOnSymbol(
                            WRNID_IndirectlyImplementedBaseMember5,
                            CurrentErrorLog(ImplementingMember),
                            ImplementsList,
                            GetQualifiedErrorName(ActualInterface),
                            InterfaceMemberToImplement->GetName(),
                            GetQualifiedErrorName(Interface),
                            GetQualifiedErrorName(ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()),
                            StringOfSymbol(CurrentCompilerInstance(), InterfaceMemberToImplement));
                    }
                }
            }
            else
            {
#if 0
                    // 


                if (ImplementedInterfacesList[InterfaceIdx ].IsReimplOnlyForSomeTypeArgs())
                {
                    // "'|1.|2' re-implements '|3.|4' already implemented by the base class '|5' for some type arguments."

                }
                else
#endif
                {
                    if (ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()->GetPWellKnownAttrVals()->GetWindowsRuntimeImportData())
                    {
                        
                        // "'|1.|2' is already implemented by the base class '|3'. Re-implementation of Windows Runtime Interface |4 is not allowed."

                        ReportErrorOnSymbol(
                            ERRID_ReImplementatingWinRTInterface4,
                            CurrentErrorLog(ImplementingMember),
                            ImplementsList,
                            GetQualifiedErrorName(ActualInterface),
                            InterfaceMemberToImplement->GetName(),
                            GetQualifiedErrorName(ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()),
                            GetQualifiedErrorName(ActualInterface));
                    }
                    else
                    {
                        // "'|1.|2' is already implemented by the base class '|3'. Re-implementation of |4 assumed."

                        ReportErrorOnSymbol(
                            WRNID_ImplementedBaseMember4,
                            CurrentErrorLog(ImplementingMember),
                            ImplementsList,
                            GetQualifiedErrorName(ActualInterface),
                            InterfaceMemberToImplement->GetName(),
                            GetQualifiedErrorName(ImplementedInterfacesList[ InterfaceIdx ].GetOriginalImplementer()),
                            StringOfSymbol(CurrentCompilerInstance(), InterfaceMemberToImplement));
                    }
                }
            }
        }

        BCSYM_GenericTypeBinding *UnifyingInterfaceBinding =
            ImplementedMembers->GetUnifyingMemberBinding(
                InterfaceMemberToImplement,
                ActualInterfaceBinding->PGenericTypeBinding(),
                SymbolFactory,
                CurrentCompilerInstance());

        if (UnifyingInterfaceBinding)
        {
            // "Cannot implement '|1.|2' because its implementation could clash with the implementation for '|3.|2' for some type arguments."

            ReportErrorOnSymbol(
                ERRID_InterfaceMethodImplsUnify3,
                CurrentErrorLog(ImplementingMember),
                ImplementsList,
                GetQualifiedErrorName(ActualInterface),
                InterfaceMemberToImplement->GetName(),
                GetQualifiedErrorName(UnifyingInterfaceBinding));

            continue;
        }

        // Keep track of the interface members that have been implemented and give an error if this one has already been implemented
        if (ImplementedMembers->Add(InterfaceMemberToImplement, ActualInterfaceBinding->PGenericTypeBinding()))
        {
            ReportErrorOnSymbol(
                ERRID_MethodAlreadyImplemented2,
                CurrentErrorLog(ImplementingMember),
                ImplementsList,
                GetQualifiedErrorName(ActualInterface),
                InterfaceMemberToImplement->GetName());

            continue;
        }

        ImplementsList->SetImplementedMember( InterfaceMemberToImplement, ActualInterfaceBinding );

        // Dev10 #735384 Check if we are improperly using embedded types
        if (InterfaceMemberToImplement->IsProc())
        {
            BCSYM_Proc * pImplementedProc = InterfaceMemberToImplement->PProc();
            ErrorTable * pErrorTable = CurrentErrorLog(ImplementingMember);

            if (pErrorTable != NULL && ImplementingMember->HasLocation())
            {
                CompilerProject * pUseProject = ImplementingMember->GetContainingProject();
                CompilerProject * pImportedFromProject = pImplementedProc->GetContainingProject();

                if (pUseProject != NULL && pImportedFromProject != NULL && pUseProject != pImportedFromProject &&
                    pUseProject->NeedToCheckUseOfEmbeddedTypes())
                {
                    if (!ImplementingMember->IsBad())
                    {
                        TypeHelpers::CheckProcForUseOfEmbeddedTypes(
                            ImplementingMember,            
                            pUseProject,
                            pUseProject, 
                            pErrorTable,
                            ImplementingMember->GetLocation());
                    }

                    if (!pImplementedProc->IsBad())
                    {
                        TypeHelpers::CheckProcForUseOfEmbeddedTypes(
                            pImplementedProc,            
                            pUseProject,
                            pImportedFromProject, 
                            pErrorTable,
                            ImplementingMember->GetLocation());
                    }
                }
            }
        }

        if ( ImplementingMember->IsEventDecl())
        {
            BCSYM_EventDecl *ImplementingEvent = ImplementingMember->PEventDecl();
            BCSYM *ImplementingEventDelegate = ImplementingEvent->GetDelegate();
            BCSYM_EventDecl *ImplementedEvent = InterfaceMemberToImplement->PEventDecl();
            BCSYM *ImplementedEventDelegate = ImplementedEvent->GetDelegate();

            // If an event implements more than interface event, all the delegate types must be the same (#227424)
            if ( ImplementingEventDelegate )
            {
                unsigned DelegateCompare =
                    BCSYM::CompareReferencedTypes(
                        ImplementingEventDelegate,
                        (GenericBinding *)NULL,
                        ImplementedEventDelegate,
                        ActualInterfaceBinding,
                        SymbolFactory);

                if ( DelegateCompare != EQ_Match )
                {
                    VSASSERT(ImplementedEventDelegate, "Implement event should have a delegate!!!");

                    // "Event '|1' can't implement event '|2' on interface '|3' because their delegates types '|4' and '|5' do not match.")
                    // or
                    // "Event '|1' can't implement Event '|2' on interface '|3' because its delegate type is different than another Event's delegate type being implemented by Event '|1'.")

                    ReportErrorAtLocation(
                        ImplementingEvent->IsDelegateFromImplements() || !ImplementedEventDelegate ?
                            ERRID_MultipleEventImplMismatch3 :
                            ERRID_EventImplMismatch5,                       // Bug VSWhidbey 224442
                        CurrentErrorLog(ImplementingMember),
                        ImplementsList->GetLocation(),
                        ImplementingMember->GetName(),                      // |1
                        ImplementedEvent->GetName(),                        // |2
                        GetQualifiedErrorName(ActualInterface),             // |3
                        GetQualifiedErrorName(ImplementingEventDelegate),   // |4
                        ImplementedEventDelegate ?                          // |5
                            GetQualifiedErrorName(ImplementedEventDelegate) :
                            NULL);
                }

                // A WindowsRuntime Event's RemoveHandler param type != EventRegistrationToken or 
                // a .NET event's RemoveHandler param type = EventRegistrationToken is an error.
                if (ImplementingEvent->IsBlockEvent() &&
                    CurrentCompilerHost()->GetFXSymbolProvider()->IsTypeAvailable(FX::EventRegistrationTokenType) &&
                    ((ImplementedEvent->IsWindowsRuntimeEvent() &&
                      !BCSYM::AreTypesEqual(
                        ImplementingEvent->GetProcRemove()->GetFirstParam()->GetType(),
                        CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::EventRegistrationTokenType))) ||
                     (!ImplementedEvent->IsWindowsRuntimeEvent() &&
                       BCSYM::AreTypesEqual(
                        ImplementingEvent->GetProcRemove()->GetFirstParam()->GetType(),
                        CurrentCompilerHost()->GetFXSymbolProvider()->GetType(FX::EventRegistrationTokenType)))))
                {
                    // "Event '|1' cannot implement event '|2' on interface '|3' because the parameters of their 'RemoveHandler' methods do not match."

                    BCSYM *RawParamType = ImplementingEvent->GetProcRemove()->GetFirstParam()->GetRawType();

                    ReportErrorAtLocation(
                        ERRID_EventImplRemoveHandlerParamWrong,
                        CurrentErrorLog(ImplementingEvent),
                        RawParamType->IsNamedType() ?
                            RawParamType->GetLocation() :
                            ImplementingEvent->GetProcRemove()->GetFirstParam()->GetLocation(),
                        ImplementingEvent->GetName(),
                        ImplementedEvent->GetName(),
                        GetQualifiedErrorName(ActualInterface));

                    ImplementingEvent->SetIsBad();
                }
            }
            else
            {
                if ( ImplementedEventDelegate )
                {
                    ImplementedEventDelegate =
                        ReplaceGenericParametersWithArguments(
                            ImplementedEventDelegate,
                            ActualInterfaceBinding,
                            *SymbolFactory );

                    // Fill in the missing information.  We didn't know any of this until we actually resolved the event.
                    ImplementingEvent->SetDelegate( ImplementedEventDelegate );
                    ImplementingEvent->SetIsDelegateFromImplements(true);

                    // Fill in the delegate types for the Add/Remove procs.
                    // We would have done this earlier but we didn't know any of this until we actually resolved the event.
                    if ( ImplementingEvent->GetDelegateVariable())
                    {
#if IDE
                        VSASSERT(ImplementingEvent->GetDelegateVariable()->GetType(), 
                            "should not be null since other locations in the compiler depend on this being set.  See Dev11 375566");

                        ImplementingEvent->SetDeclaredDelegateVariableType(ImplementingEvent->GetDelegateVariable()->GetType());
#endif

                        Symbols::SetType( ImplementingEvent->GetDelegateVariable(), ImplementedEventDelegate );
                    }

                    VSASSERT( ImplementingEvent->GetProcAdd() &&
                              ImplementingEvent->GetProcAdd()->IsSyntheticMethod() &&
                              ImplementingEvent->GetProcRemove() &&
                              ImplementingEvent->GetProcRemove()->IsSyntheticMethod(),
                                    "Block events without an explicit delegate type unexpected !!!");

                    Symbols::SetParamType( ImplementingEvent->GetProcAdd()->GetFirstParam(), ImplementedEventDelegate );
                    Symbols::SetParamType( ImplementingEvent->GetProcRemove()->GetFirstParam(), ImplementedEventDelegate );
                }
            }

            if ((PreviousImplementedWindowsRuntimeEvent && !ImplementedEvent->IsWindowsRuntimeEvent()) ||
                (PreviousImplementedDotNETEvent && ImplementedEvent->IsWindowsRuntimeEvent()))
            {
                // Event '|1' cannot implement a Windows Runtime event '|2' and a regular .NET event '|3'.
                ReportErrorAtLocation(
                        ERRID_MixingWinRTAndNETEvents,
                        CurrentErrorLog(ImplementingEvent),
                        ImplementsList->GetLocation(),
                        ImplementingEvent->GetName(),
                        GetQualifiedErrorName(PreviousImplementedWindowsRuntimeEvent? PreviousImplementedWindowsRuntimeEvent: ImplementedEvent),
                        GetQualifiedErrorName(PreviousImplementedDotNETEvent? PreviousImplementedDotNETEvent: ImplementedEvent));

                ImplementingEvent->SetIsBad();
            }

            if (ImplementedEvent->IsWindowsRuntimeEvent())
            {
                ImplementingEvent->ConvertToWindowsRuntimeEvent(CurrentCompilerHost(), SymbolFactory);
                PreviousImplementedWindowsRuntimeEvent = ImplementedEvent;
            }
            else
            {
                PreviousImplementedDotNETEvent = ImplementedEvent;
            }
        }
    } // haul through implements in implements list

    // Mark the ImplementingMember bad now since we've run the list of implements and it won't hinder resolving any of the entries in the implements list.
    if ( MarkImplementingThingBad )
    {
        ImplementingMember->SetIsBad();
    }
}
