//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Detect and report cycles among a set of symbols. For instance, detect
//  whether a class inheritance hierarchy forms a cycle: a inherits from b,
//  b inherits from c, c inherits from a, etc.
//
//  How it works:
//
//   - Call "AddSymbolToVerify" for each symbol you want to look for cycles in.
//   - Each symbol is put in the SymbolNode list as well as the SymbolNode hash table.
//   - FindCycles walks the symbol list, calling CheckCyclesOnNode on each list.
//
//   - CheckCyclesOnNode:
//
//      - If this node has been processed, stop.
//      - If this node is currently being processed (CurrentlyProcessing), remember that we have a cycle, stop.
//      - Mark this node as currently being processed.
//      - Add each child of this node to the SymbolNode list and call CheckCyclesOnNode on it.
//      - Mark this node as being done.
//
//   - If a cycle was found (m_CycleFound), rewalk the list of original nodes in the list
//     with the following slower algorithm to collect a list of all of the cycles.
//
//      - If this node is currently being processed (CurrentlyProcessing), add a
//        cycle to the cycle list (linked by NextListOfCycleLists) and return.
//      - Mark this node as currently being processed (CurrentlyProcessing).
//      - Add each child of this node to the SymbolNode list and call CheckCyclesOnNode on it.
//      - Check each cycle in the cycle list (linked by NextListOfCycleLists)
//        to see if this node is at the root.  If so, and this node
//        is the node in the list we're currently checking, add the cycle
//        to the list of cycles on the node.
//      - For each remaining cycle, add a cycle for this node as the
//        second element in the cycle list.
//      - Mark this node as being done (!CurrentlyProcessing).
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

// Constructor
Cycles::Cycles(
    _In_ Compiler *pCompiler, // [in] instance of the compiler
    _In_ NorlsAllocator *pnra, // [in] allocator to use
    _In_opt_ ErrorTable *perror, // [optional] errors go in here
    _In_ ERRID erridHeader, // [in] the error that precedes the list of cycles (e.g. Interface '|1' defines an inheritance cycle:)
    _In_ ERRID erridElement // [in] the error that is used to describe the elements in the cycle (e.g. "|0\n    '|1' calls '|2'.")
)
{
    memset(m_SymbolHash, 0, sizeof(m_SymbolHash));
    m_CompilerInstance = pCompiler;
    m_Allocator = pnra;
    m_ErrorLog = perror;
    m_erridHeader = erridHeader;
    m_erridElement = erridElement;
    m_CycleFound = false;
}

ErrorTable * Cycles::GetErrorTable(
    SymbolNode * pNode, 
    bool useSymbolErrorTable)
{
    if
    (
        useSymbolErrorTable &&
        pNode &&
        pNode->m_Symbol &&
        pNode->m_Symbol->GetSourceFile() &&
        pNode->m_Symbol->GetSourceFile()->GetCurrentErrorTable()
    )
    {
        return pNode->m_Symbol->GetSourceFile()->GetCurrentErrorTable();
    }
    else
    {
        return m_ErrorLog;

    }
}

/*****************************************************************************
;AddSymbolToVerify

The cycles class can batch up a group of symbols and then be called to do
cycle analysis on each one.  This adds a symbol to the list of those
we want to search for cycles
*****************************************************************************/
SymbolNode *Cycles::AddSymbolToVerify
(
    BCSYM_NamedRoot *SymbolToAdd, // [in] the symbol we want to add to the list of guys to check
    BCSYM_NamedRoot *CallTarget // [in] when SymbolToAdd is a Method, this is the optional target the method calls
)
{
    // Check to see if this symbol was already added - we don't want to do cycle checks on the same things over and over
    UINT_PTR HashIdx = ((UINT_PTR) SymbolToAdd / sizeof( BCSYM_NamedRoot )) % 256;
    SymbolNode *HashEntry = m_SymbolHash[ HashIdx ];

    while ( HashEntry && HashEntry->m_Symbol != SymbolToAdd )
    {
        HashEntry = HashEntry->NextHashEntry;
    }

    VSASSERT( !HashEntry || HashEntry->m_CallTarget == NULL, "Added a non-empty node twice.");

    // Create a new node.
    if ( !HashEntry )
    {
        HashEntry = (SymbolNode *)m_Allocator->Alloc( sizeof( SymbolNode ));

        SymbolToAdd = SymbolToAdd->DigThroughAlias()->PNamedRoot(); // get past aliasing

        HashEntry->StartOfCycleChain = false;
        HashEntry->m_Symbol = SymbolToAdd;

        SymbolsToCheck.InsertLast( HashEntry );

        // Add the node to the hash table.
        HashEntry->NextHashEntry = m_SymbolHash[ HashIdx ];
        m_SymbolHash[ HashIdx ] = HashEntry;
    }

    // Set the target the method calls (will only be !null if SymbolToAdd is a method)
    HashEntry->m_CallTarget = CallTarget;

    // Return it.
    return HashEntry;
}

/*****************************************************************************
;FindCycles

Trace this symbol and all its children to determine if there
are any cycles, e.g. makes sure class A doesn't inherit from class B
that inherits class A, etc.  Makes sure that a class definition doesn't
contain self referencing members, e.g. a struct s1 with a private x as S1 member

An important side effect of this function is that SymbolsToCheck will get
filled with a list of all children from most-derived, i.e. the
element we are checking ( first element ) to the most base class
( last element ) which is available afterwards via GetSymbolNodeList()
*****************************************************************************/
void Cycles::FindCycles
(
    bool CheckRecordCycles, // [in] whether to check whether fields in a struct imbed the structure itself
    bool useSymbolErrorTable // [in] indicates wether or not errors should be reported using the error table of the file
                              // containing the symbol where the error occurs, or if it should always use
                              // the error table provided in the object's constructor
)
{
    // Search all of the initial nodes for a cycle.  This will set m_CycleFound if we find a cycle anywhere in here.
    // This algorithm is fairly quick.

    SymbolNode *SymbolBeingChecked = SymbolsToCheck.GetFirst();
    unsigned NumSymbolsToCheck = SymbolsToCheck.NumberOfEntries(); // the count as of right now - we will be adding the children of SymbolBeingChecked to this list as we trawl so get the count now
    for ( unsigned i = 0; i < NumSymbolsToCheck; i++ ) // haul through all the symbols in the list we are supposed to check for cycles and do the fast check to see if a cycle exists
    {
        CheckCyclesOnNode( SymbolBeingChecked, CheckRecordCycles ); // and check each one for cycles
        SymbolBeingChecked = SymbolBeingChecked->Next();
        if ( m_CycleFound ) break; // as soon as we know any symbol has a cycle, we can do the heavy lifting to rescan the symbols and report errors on the ones that have cycles
    }

    if ( m_CycleFound ) // We need to do more work (as outlined in the file header) if we actually want to report errors for the cycles.
    {
        SymbolBeingChecked = SymbolsToCheck.GetFirst();
        // Run through all our symbols we were asked to check and for those that have cycles, build a cycle list for error reporting
        for ( unsigned i = 0; i < NumSymbolsToCheck; i++ )
        {
            CycleInfoList *ListOfCycleLists = NULL; // the list of lists of cycles for this symbol (why a list of lists? Cycles for each member of a struct that has cycles, or for instance)
            SymbolBeingChecked->StartOfCycleChain = true; // we want to find all the cycles that loop back to this particular symbol
            BuildCycleList( SymbolBeingChecked, CheckRecordCycles, SymbolBeingChecked->m_Symbol, &ListOfCycleLists );
            SymbolBeingChecked->StartOfCycleChain = false;

            // If this symbol did contain cycles, report the error(s)
            if ( ListOfCycleLists )
            {
                ReportErrorOnNode( SymbolBeingChecked, ListOfCycleLists, useSymbolErrorTable );
            }
            SymbolBeingChecked = SymbolBeingChecked->Next();
        }
    }
}

/*****************************************************************************
;CheckCyclesOnNode

Recursively detect whether the current node is part of a cycle.  This is just
a fast haul through the children of this symbol to look for cycles.  If found,
we then do the slower analysis required to get the cycle information needed
to report errors
*****************************************************************************/
void Cycles::CheckCyclesOnNode
(
    SymbolNode *SymbolToCheck, // [in] we want to see if we can find a path starting with this symbol that leads back to itself
    bool CheckRecordCycles // [in] whether we are specifically looking for structure cycles
)
{
    // If this node has already been processed, stop.
    if ( SymbolToCheck->BeenProcessed )
    {
        return;
    }

    // If this node is currently being processed then we've found a cycle - bail
    if ( SymbolToCheck->CurrentlyProcessing )
    {
        m_CycleFound = true;
        return;
    }

    SymbolToCheck->CurrentlyProcessing = true; // Mark this so that if we come across it again while processing its references, we will know we cycled

    // Check all of the children of this node to see if they might be part of the cycle.
    // 

    BCSYM_NamedRoot *Symbol = SymbolToCheck->m_Symbol->DigThroughAlias()->PNamedRoot();

    switch( Symbol->GetKind())
    {
        case SYM_Class:

            VSASSERT(!Symbol->PClass()->IsEnum(), "how did this get here?");
            // Structure field checks (just check fields - structures can't inherit)
            if ( Symbol->IsStruct())
            {
                if ( CheckRecordCycles )
                {
                    BCITER_CHILD StructIter;
                    StructIter.Init( Symbol->PContainer() );

                    while ( BCSYM_NamedRoot *StructMember = StructIter.GetNext())
                    {
                        if ( StructMember->IsVariable() && !StructMember->PVariable()->IsShared() && !StructMember->IsBad())
                        {
                            bool IsFromMetaData = false;
                            BCSYM *MemberType = StructMember->PVariable()->GetType()->DigThroughAlias();
                            if ( MemberType && !MemberType->IsBad() && MemberType->IsNamedRoot() && MemberType->PNamedRoot()->GetCompilerFile()->IsMetaDataFile())
                            {
                                // Bug #223813, #153794, #157083 // We only look at embedded value types - exclude intrinsics
                                // system.int32, etc. have this interesting property where the struct has an embedded member of itself as the first field.  We don't allow
                                // that in VB, at least VB shouldn't be able to produce them.  So skip all those and any others that could be built by another language
                                // with this 'interesting' property - which would be anybody brought in from metadata.  The assumption is that if the thing made it to
                                // metadata, it must be correct by the rules of whatever language produced it so just consume it...
                                IsFromMetaData = true;
                            }

                            // If the type of the field for this structure has a shot at embedding a non-shared copy of the struct itself then check it out
                            if ( MemberType && MemberType->IsStruct() && !IsFromMetaData && !MemberType->IsEnum())
                            {
                                // We've got a member of this structure that is a structure - check it out for a possible self referencing problem
                                // Make sure it's in the list of things to check if it isn't in there already
                                SymbolNode *EmbeddedMember = FindNode( MemberType->PClass());
                                if ( !EmbeddedMember )
                                {
                                    EmbeddedMember = AddSymbolToVerify( MemberType->PClass());
                                }
                                CheckCyclesOnNode( EmbeddedMember, CheckRecordCycles );
                            }
                        } // if variable
                    } // loop structure members
                } // CheckRecordCycles
            }
            else // Examine a Class for an inheritance cycle (but not built in classes like CurrencyRecord, etc.)
            {
                // If the base class is bad, but not a generic bad, then go ahead and add it in to the list of base classes.
                BCSYM_NamedRoot *BaseClass = (BCSYM_NamedRoot *)Symbol->PClass()->GetBaseClass()->DigThroughAlias();

                if ( BaseClass && !BaseClass->IsGenericBadNamedRoot())
                {
                    // Mark a cycle found to trigger error reporting during which time we will report that we aren't inheriting from a class
                    // Inheriting from a non-class will be reported later in bindable.
                    if ( BaseClass->IsClass() && BaseClass->PClass()->IsNotInheritable())
                    {
                        // VS 534262 & 525414:
                        // Make sure the error is associated with the file that contains the class declaration, since
                        // this error might be the only thing preventing us from going to the codegen phase and crashing
                        // because of bad symbols.
                        CompilerFile *RealSymbolFile = Symbol->GetContainingSourceFileIfBasic();
                        ErrorTable *RealErrorTable = NULL;

                        if (RealSymbolFile)
                        {
                            RealErrorTable = RealSymbolFile->GetErrorTable();
                        }

                        if (!RealErrorTable)
                        {
                            VSFAIL("We should only be getting errors on BASIC files");
                            RealErrorTable = m_ErrorLog;
                        }

                        RealErrorTable->CreateError(ERRID_InheritsFromCantInherit3,
                                                 Symbol->PClass()->GetInheritsLocation(),
                                                 Symbol->PClass()->GetName(),
                                                 BaseClass->GetName(),
                                                 StringOfSymbol( m_CompilerInstance, BaseClass ));

                        // VS 534262 & 525414:
                        // Note that the cyclic inheritance needs to be broken here because bindable assumes that
                        // there aren't any cycles by that point.
                        Symbols::SetBaseClass( Symbol->PClass(), Symbols::GetGenericBadNamedRoot());
                        break;
                    }
                    else if ( !BaseClass->IsClass() || BaseClass->IsEnum())
                    {
                        break; // no sense digging any further through this class.  Bindable will report the error about the bad inherits
                    }

                    SymbolNode *BaseClassFromNodeList = FindNode( BaseClass ); // Have we seen this base class before?
                    if ( !BaseClassFromNodeList )  // if not, add the base class to the list of symbols to check
                    {
                        BaseClassFromNodeList = AddSymbolToVerify( BaseClass );
                    }
                    CheckCyclesOnNode( BaseClassFromNodeList, CheckRecordCycles ); // look for cycles in the base class and its base class, etc. etc.
                }
            }
            break;

        case SYM_MethodImpl:
        {
            BCSYM_NamedRoot *CallTarget = SymbolToCheck->m_CallTarget->DigThroughAlias()->PNamedRoot();
            if ( CallTarget ) // CycleInfoList checks on methods involve looking at what the method calls
            {
                SymbolNode *CallInNodeList = FindNode( CallTarget );
                //VSWhidbey#57376,CallInNodeList will be NULL in EnC
                //VSASSERT( CallInNodeList, "Call target must already be in the list.");
                if (CallInNodeList)
                {
                    CheckCyclesOnNode( CallInNodeList, CheckRecordCycles );
                }
            }
            break;
        }

        case SYM_Interface: // Interface Inheritance cycle check
        {
            for ( BCSYM_Implements *ImplementsEntry = Symbol->PInterface()->GetFirstImplements();
                  ImplementsEntry;
                  ImplementsEntry = ImplementsEntry->GetNext())
            {
                if ( ImplementsEntry->IsBad() || ImplementsEntry->GetRawRoot() == NULL ||
                     ( ImplementsEntry->GetRawRoot()->IsNamedType() && ImplementsEntry->GetRawRoot()->PNamedType()->GetSymbol() == NULL )) continue;

                BCSYM *ImplementsType = ImplementsEntry->GetCompilerRoot();
                if ( ImplementsType->IsInterface()) // Bindable will report errors about inheriting from non-interfaces
                {
                    SymbolNode *ImplementsInNodeList = FindNode( ImplementsType->PInterface());
                    if ( !ImplementsInNodeList )
                    {
                        ImplementsInNodeList = AddSymbolToVerify( ImplementsType->PInterface());
                    }
                    CheckCyclesOnNode( ImplementsInNodeList, CheckRecordCycles ); // look for cycles in the thing the interface inherits from
                }
            }
            break;
        }
    } // switch

    SymbolToCheck->CurrentlyProcessing = false; // We've finished processing this symbol
    SymbolToCheck->BeenProcessed = true; // And we need to know we worked out the cycles on this symbol
}

/*****************************************************************************
;BuildCycleList

A recursive function that builds a list of the path of symbols that are part
of the cycle that leads from TestMeSymbol back to itself
*****************************************************************************/
void Cycles::BuildCycleList
(
    SymbolNode *PotentialSymbolContainingCycle, // [in] We are looking for a loop that brings us back to this guy
    bool CheckRecordCycles, // [in] whether we are checking STRUCTURE embedding cycles
    BCSYM *ReferringMember, // [in] the symbol that referred us here, e.g. for a structure this would be the member whose type is in PotentialSymbolContainingCycle
    CycleInfoList **ListOfCycleLists // [in/out] the head of the linked list of cycles from PotentialSymbolContainingCycle back to itself.
)
{
    // If this node is currently being processed, then it is the main loop of the cycle.  Keep track of it in the cycle list.
    if ( PotentialSymbolContainingCycle->CurrentlyProcessing )
    {
        // Only keep track of a cycles that loop back to the node we were initially called to check (we'll catch the other cycles that exist later)
        if ( PotentialSymbolContainingCycle->StartOfCycleChain )
        {
            CycleInfoList *NewCycle = (CycleInfoList*)m_Allocator->Alloc( sizeof( CycleInfoList ));
            NewCycle->Symbol = PotentialSymbolContainingCycle; // we found our way back to the initial symbol so we have a cycle.
            NewCycle->ReferringMember = ReferringMember;
            NewCycle->NextListOfCycleLists = *ListOfCycleLists; // We are creating a new list of cycles - attach it to the list of cycle lists already found for PotentialSymbolContainingCycle
            *ListOfCycleLists = NewCycle; // and make the head of the new list of cycles point to the start of the cycle chain we just discovered so we can tack the hops on for this particular cycle list as we return up the recursion stack
        }
        return;
    }

    PotentialSymbolContainingCycle->CurrentlyProcessing = true;

    // Check all of the children of this node to see if they might be part of the cycle.
    // 

    BCSYM_NamedRoot *TestMeSymbol = PotentialSymbolContainingCycle->m_Symbol->DigThroughAlias()->PNamedRoot();
    switch( TestMeSymbol->GetKind())
    {
        case SYM_Class:

            VSASSERT(!TestMeSymbol->PClass()->IsEnum(), "how did this get here?");
            // Check Structure fields (Structs don't inherit)
            if ( TestMeSymbol->IsStruct())
            {
                if ( CheckRecordCycles )
                {
                    BCITER_CHILD StructIter;
                    StructIter.Init( TestMeSymbol->PContainer() );

                    while ( BCSYM_NamedRoot *StructMember = StructIter.GetNext())
                    {
                        if ( StructMember->IsVariable() && !StructMember->PVariable()->IsShared() && !StructMember->IsBad())  // we are only interested in looking at non-shared embedded types
                        {
                            BCSYM *VarType = StructMember->PVariable()->GetType();
                            if ( VarType && VarType->IsStruct() &&
                                 !VarType->IsIntrinsicType() && !VarType->IsEnum())
                            {
                                VarType = VarType->DigThroughAlias();
                                if ( VarType->IsArrayType())
                                {
                                    VarType = VarType->PArrayType()->GetRoot(); // Dig through and get the type of the array
                                }

                                if ( VarType->IsClass() && !VarType->IsIntrinsicType() && !VarType->IsEnum())
                                {
                                    CycleInfoList *CyclesInChild = NULL; // the list of cycles for the member we are about to examine
                                    SymbolNode *EmbeddedStructMember = FindNode( VarType->PClass());
                                    VSASSERT( EmbeddedStructMember, "Should already exist because we examined all embedded guys during the quick portion of the FindCycles pass.");

                                    BuildCycleList( EmbeddedStructMember, CheckRecordCycles, StructMember, &CyclesInChild );
                                    MergeCycleLists( CyclesInChild, ListOfCycleLists );
                                }
                            }
                        }
                    }
                }
            }
            else // Handle Class inheritance cycles
            {
                BCSYM_NamedRoot *BaseClass = TestMeSymbol->PClass()->GetBaseClass()->PNamedRoot();
                if ( !BaseClass || !BaseClass->IsClass() || BaseClass->IsEnum() || BaseClass->PClass()->IsNotInheritable())
                {
                    break; // no sense digging any further through this class.  Bindable will report the error about the bad inherits
                }

                SymbolNode *SymbolForBaseClass = FindNode( BaseClass->DigThroughAlias()->PNamedRoot());
                VSASSERT( SymbolForBaseClass, "We should have already put this guy in the list when we found the cycle in the first place.");

                CycleInfoList *CyclesForSymbol = NULL;
                BuildCycleList( SymbolForBaseClass, CheckRecordCycles, TestMeSymbol, &CyclesForSymbol ); // generate a list of cycles for this symbol
                MergeCycleLists( CyclesForSymbol, ListOfCycleLists ); // and tack them to the head of the list of cycles lists found for the symbol
            }
            break;

        case SYM_MethodImpl:
        {
            BCSYM_NamedRoot *CallTarget = PotentialSymbolContainingCycle->m_CallTarget->DigThroughAlias()->PNamedRoot();

            if ( CallTarget )
            {
                SymbolNode *CallTargetFromNodeList = FindNode( CallTarget );
                VSASSERT( CallTargetFromNodeList, "Call target must already be in the node list." );

                CycleInfoList *MethodCycles = NULL;
                BuildCycleList( CallTargetFromNodeList, CheckRecordCycles, PotentialSymbolContainingCycle->m_Symbol, &MethodCycles );
                MergeCycleLists( MethodCycles, ListOfCycleLists );
            }
            break;
        }

        case SYM_Interface:
        {
            for ( BCSYM_Implements *Implements = TestMeSymbol->PInterface()->GetFirstImplements(); Implements; Implements = Implements->GetNext())
            {
                if ( Implements->IsBad() || Implements->GetRawRoot() == NULL ||
                     ( Implements->GetRawRoot()->IsNamedType() && Implements->GetRawRoot()->PNamedType()->GetSymbol() == NULL )) continue;

                BCSYM *ImplementsType = Implements->GetCompilerRoot();

                if ( ImplementsType->IsInterface())
                {
                    SymbolNode *ChildNode = FindNode( ImplementsType->PInterface());
                    VSASSERT( ChildNode, "Must already exist.");

                    CycleInfoList *InterfaceCycles = NULL;
                    // Break the cycle caused by this Implements statement
                    Implements->SetIsBad();

                    BuildCycleList( ChildNode, CheckRecordCycles, Implements, &InterfaceCycles );
                    MergeCycleLists( InterfaceCycles, ListOfCycleLists );
                }
            } // loop through Implements
        } // case SYM_Interface
        break;
    } // switch

    // As we wind back out of the call stack, this builds a list of the hops we took as we found the cycle to the initial node we started checking
    while ( *ListOfCycleLists )
    {
        CycleInfoList *CurrentCycle = *ListOfCycleLists;

        if ( CurrentCycle->Symbol != PotentialSymbolContainingCycle )
        {
            // Add ourselves to the list of hops taken on our way to the cycle that has been discovered
            CycleInfoList *NewCycle = (CycleInfoList *)m_Allocator->Alloc( sizeof( CycleInfoList ));
            NewCycle->Symbol = PotentialSymbolContainingCycle;

            // Insert ourselves between CurrentCycle and whatever CurrentCycle points to
            NewCycle->NextInCurrentCycleChain = CurrentCycle->NextInCurrentCycleChain; // include ourselves in the list of hops for the cycle
            CurrentCycle->NextInCurrentCycleChain = NewCycle; // and we insert ourselves after the cycles discovered so far
        }

        // Examine the next list of cycle lists
        ListOfCycleLists = &CurrentCycle->NextListOfCycleLists;
    }

    PotentialSymbolContainingCycle->CurrentlyProcessing = false;
}

/*****************************************************************************
;ReportErrorOnNode

Report an error on this node if it's part of a cycle.
*****************************************************************************/
void Cycles::ReportErrorOnNode
(
    SymbolNode *NodeToReport, // [in] we want to report an error on this node if it is in a cycle
    CycleInfoList *ListOfCycleLists, // [in] the list of cycle lists discovered for the symbol in NodeToReport
    bool useSymbolErrorTable // [in] indicates wether or not errors should be reported using the error table of the file
                             // containing the symbol where the error occurs, or if it should always use
                             // the error table provided in the object's constructor
)
{
    VSASSERT( ListOfCycleLists != NULL, "how did this happen?");

    StringBuffer NameOfSymbolWithCycles;
    StringBuffer NameOfSymbolInvolvedInCycle;

    //
    // Report the error.
    //

    CycleInfoList *HeadOfList = ListOfCycleLists;

    // Run through the list of cycles for each symbol we were called to find cycles on
    for ( ; ListOfCycleLists; ListOfCycleLists = ListOfCycleLists->NextListOfCycleLists )
    {
        if ( m_ErrorLog )
        {
            // Get the location for where the thing that caused the cycle is (inherits line, field member, etc.)
            Location *Loc;
            if ( NodeToReport->m_Symbol->IsClass() && !NodeToReport->m_Symbol->IsStruct() && !NodeToReport->m_Symbol->IsEnum())
            {
                Loc = NodeToReport->m_Symbol->PClass()->GetInheritsLocation();
            }
            else
            {
                Loc = NodeToReport->m_Symbol->GetLocation();
            }

            GetName( NodeToReport, &NameOfSymbolWithCycles );
            HRESULT hrErr = HrMakeRepl( m_erridHeader, NameOfSymbolWithCycles.GetString());

            // Build the list of hops we took so they understand how the cycle occurs (c1 inherits c2 which inherits i1, etc.)
            CycleInfoList *ThisCycle;
            for ( ThisCycle = ListOfCycleLists; ThisCycle->NextInCurrentCycleChain; ThisCycle = ThisCycle->NextInCurrentCycleChain )
            {
                GetName( ThisCycle->Symbol, &NameOfSymbolWithCycles );
                GetName( ThisCycle->NextInCurrentCycleChain->Symbol, &NameOfSymbolInvolvedInCycle );
                hrErr = HrMakeReplWithError( m_erridElement, hrErr, NameOfSymbolWithCycles.GetString(), NameOfSymbolInvolvedInCycle.GetString());
            }

            // Add the error to the error table.
            GetName( ThisCycle->Symbol, &NameOfSymbolWithCycles );
            GetName( NodeToReport, &NameOfSymbolInvolvedInCycle );

            ErrorTable * pErrorTable = GetErrorTable(NodeToReport, useSymbolErrorTable);
            pErrorTable->CreateErrorWithError( m_erridElement,
                                              Loc,
                                              hrErr,
                                              NameOfSymbolWithCycles.GetString(),
                                              NameOfSymbolInvolvedInCycle.GetString());

        } // if m_ErrorLog

        // Break the inheritance chain so that remaining code in the compiler that iterates over
        // this symbol won't go into an endless loop
        BreakCycle( NodeToReport->m_Symbol, ListOfCycleLists );
    } // loop list of cycles for this symbol
}

/*****************************************************************************
;GetName

Get the name of a symbol.  In the case of a method, get the rep of the
containing class
*****************************************************************************/
void Cycles::GetName
(
    SymbolNode *Symbol, // [in] symbol we want the name of
    StringBuffer *SymbolName // [in] name of Symbol put here
)
{
    SymbolName->Clear(); // make sure we don't append to old junk hanging out in here

    if ( Symbol->m_Symbol->IsMethodImpl()) // for methods, we want the name of the class that contains it
    {
        Symbol->m_Symbol->GetBasicRep( m_CompilerInstance, Symbol->m_Symbol->GetContainingClass(), SymbolName, NULL );
    }
    else
    {
        SymbolName->AppendSTRING( Symbol->m_Symbol->GetName());
    }
}

/*****************************************************************************
;FindNode

Determine if a symbol table element is in our node hash
*****************************************************************************/
SymbolNode * // the hash entry matching SymbolToMatch or NULL if not found
Cycles::FindNode
(
    BCSYM_NamedRoot *SymbolToMatch // [in] the symbol we want to find in the hash
)
{
    // Find the right bucket.
    UINT_PTR HashIdx = ((UINT_PTR)SymbolToMatch / sizeof( BCSYM_NamedRoot )) % 256;

    // Search for it in the hash
    for ( SymbolNode *HashEntry = m_SymbolHash[ HashIdx ]; HashEntry; HashEntry = HashEntry->NextHashEntry )
    {
        if ( HashEntry->m_Symbol == SymbolToMatch )
        {
            return HashEntry;
        }
    }
    return NULL; // not found
}

/*****************************************************************************
;BreakCycle

Breaks an inheritance hierarchy cycle by marking the class so that it knows
its base class leads to an inheritance cycle
*****************************************************************************/
void Cycles::BreakCycle(
    BCSYM_NamedRoot *Candidate, // [in] the symbol for which we want to change its base class to 'Object'
    CycleInfoList   *CycleList  // [in] the list of cycle lists discovered for the symbol in NodeToReport
)
{
    if ( !Candidate ) return;

    Candidate = Candidate->DigThroughAlias()->PNamedRoot();

    if ( Candidate->IsClass())
    {
        if ( Candidate->PClass()->IsStruct())
        {
            for ( CycleInfoList *List = CycleList; List != NULL; List = List->NextListOfCycleLists )
            {
                List->ReferringMember->PMember()->SetIsBad();
            }
        }
        else
        {
            VSASSERT( !Candidate->PClass()->IsEnum(), "How did an enum get in here?");
            // Classes are single inheritance so just whack the inheritance line
            Candidate->PClass()->SetBaseInvolvedInCycle(true);
            Candidate->PClass()->SetIsBad();
        }
    }
    else if ( Candidate->IsInterface())
    {
        for ( CycleInfoList *List = CycleList; List != NULL; List = List->NextListOfCycleLists )
        {
            List->ReferringMember->PImplements()->SetIsBad();
        }
        Candidate->PInterface()->SetIsBad();
    }

    // What happens to bad methods?
}

/*****************************************************************************
;MergeCycleLists

Adds the ChildCycleList to the head of the ParentCycleList
*****************************************************************************/
void Cycles::MergeCycleLists(
    CycleInfoList *ChildCycleList, // [in] we want to merge this cycle list into the ParentCycleList
    CycleInfoList **ParentCycleList // [in/out] The cycle list to merge with. On exit contains ChildCycleList
)
{
    CycleInfoList *NextListOfCycleLists;
    for (; ChildCycleList; ChildCycleList = NextListOfCycleLists ) // iterate the ChildCycleList
    {
        NextListOfCycleLists = ChildCycleList->NextListOfCycleLists;
        ChildCycleList->NextListOfCycleLists = *ParentCycleList; // link this ChildCycleList entry to the head of the ParentCycleList
        *ParentCycleList = ChildCycleList; // the head of the parent list now points to the child entry we linked on
    }
}
