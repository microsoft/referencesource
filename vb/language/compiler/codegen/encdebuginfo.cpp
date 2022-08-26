//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Manage information needed for EditAndContinue
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

//-------------------------------------------------------------------------------------------------
//
// During an EnC session, this function builds the list of variable slots using the debug information.
// We do this to preserve the original slot configuration.
//
HRESULT MethodSlots::InitLocals(
    ENC_LOCALINFO * rgLocalInfo,
    ULONG cLocals, 
    Location *plocNewLocals, 
    ULONG cNewLocals)
{
#if DEBUG
    unsigned long MaxSlot = 0;
#endif
    VB_ENTRY();
    for(ULONG i = 0; i < cLocals; i++)
    {
        LocalVariable * NewVariable = (LocalVariable *)m_SlotStorage->Alloc(sizeof(LocalVariable));
        // Add the name to the string pool
        NewVariable->VariableName = m_Compiler->AddString(rgLocalInfo[i].bstrLocalName);
        ::SysFreeString(rgLocalInfo[i].bstrLocalName);
        rgLocalInfo[i].bstrLocalName = NULL;
        if (rgLocalInfo[i].ulAttribute & VAR_IS_COMP_GEN)
        {
            NewVariable->Flags = VAR_IS_COMP_GEN;
        }
        else
        {
            VSASSERT(rgLocalInfo[i].ulAttribute == 0, "What other flags are there?");
        }
        NewVariable->SignatureSize = rgLocalInfo[i].cbSize;
        NewVariable->Signature = (BYTE *)m_SlotStorage->Alloc(NewVariable->SignatureSize);
        memcpy(NewVariable->Signature,rgLocalInfo[i].pSignature,NewVariable->SignatureSize);
        ::CoTaskMemFree(rgLocalInfo[i].pSignature);
        rgLocalInfo[i].pSignature = NULL;
        rgLocalInfo[i].cbSize = 0;
        NewVariable->SlotNumber = rgLocalInfo[i].slot;
#if DEBUG
        // want to make sure the slot numbers given to us are valid
        if (NewVariable->SlotNumber > MaxSlot)
        {
            MaxSlot = NewVariable->SlotNumber;
        }
#endif
        m_LocalVariables.InsertLast(NewVariable);
    }

    if (cNewLocals && plocNewLocals)
    {
        for (ULONG i = 0; i < cNewLocals; ++i)
        {
            NewVariableLocation *NewVarLoc = (NewVariableLocation *)m_SlotStorage->Alloc(sizeof(NewVariableLocation));
            NewVarLoc->Location = plocNewLocals[i];
            m_NewLocalVariableLocations.InsertLast(NewVarLoc);
        }
    }

    VB_EXIT();
}

//-------------------------------------------------------------------------------------------------
//
// During an EnC session, this function resets the data on the method slot for the current delta-generation session.
// This is necessary if delta-generation fails and user must edit their code and then relaunch the delta-generation.
//
void MethodSlots::Reset()
{
    m_LocalVariables.Reset();
    m_ConstantVariables.Reset();
    m_NewLocalVariableLocations.Reset();
}

//-------------------------------------------------------------------------------------------------
//
//  Insert into the pool of constant variables for the method
//
ConstantVariable * MethodSlots::AddConstant(
    BCSYM_Variable * Variable,
    BYTE * Signature,
    unsigned long SignatureSize)
{
    VSASSERT(Variable && Variable->IsVariable() && Variable->IsConstant(), "AddConstant: Symbol is invalid");

    ConstantVariable * Constant = NULL;

    Constant = (ConstantVariable *)m_SlotStorage->Alloc(sizeof(ConstantVariable));

    Constant->VariableName = Variable->PNamedRoot()->GetName();
    VSASSERT(Variable->PVariableWithValue()->GetExpression()->IsEvaluated(), "Invalid state");
    Variable->PVariableWithValue()->GetExpression()->GetValue().CopyConstant(&Constant->Value, m_SlotStorage);

    Constant->Signature = Signature;
    Constant->SignatureSize = SignatureSize;

    m_ConstantVariables.InsertLast(Constant);

    return Constant;
}

//-------------------------------------------------------------------------------------------------
//
// Assigns a slot number to a local variable and inserts it into a pool of local variables for the method
//
LocalVariable * MethodSlots::AddVariable(
    BCSYM_Variable * Variable,
    unsigned long Flags,
    BYTE * Signature,
    unsigned long SignatureSize)
{
    VSASSERT(Variable && Variable->IsVariable(), "AddVariable: Symbol is invalid");
    VSASSERT(Signature && SignatureSize, "AddVariable:  A signature is needed!");
    VSASSERT(!Variable->GetRewrittenName(), "AddVariable: Shouldn't be emitting async locals");

    STRING * VariableName = Variable->PNamedRoot()->GetName();

    LocalVariable * CurrentVariable = NULL;

    // If this method is on the stack in an EnC session, we need to preserve the original slot
    // configuration.  To do this, we'll search for the variable's original slot (if there was one)
    // using the variable name and signature as the key
    //
    if (m_MethodIsOnStack)
    {
        CurrentVariable = FindVariable(VariableName, Signature, SignatureSize, Variable->GetLocation());
    }

    // If no slot exists for this variable, make a new one
    if (CurrentVariable == NULL)
    {
        CurrentVariable = (LocalVariable *)m_SlotStorage->Alloc(sizeof(LocalVariable));

        CurrentVariable->VariableName = VariableName;
        CurrentVariable->Flags = Flags;
        CurrentVariable->SlotNumber = m_LocalVariables.NumberOfEntries();  // List size before insertion represents the next highest slot number
        CurrentVariable->Signature = Signature;
        CurrentVariable->SignatureSize = SignatureSize;
        CurrentVariable->IsActive = true; // If we're not in EnC then always set this to true

        m_LocalVariables.InsertLast(CurrentVariable);
    }
    else
    {
        // Make sure to set the Flags
        CurrentVariable->Flags = Flags;
    }

    return CurrentVariable;
}

//-------------------------------------------------------------------------------------------------
//
// Creates a LocalVariable to insert into the current scope for the PDB.  This local will not be emitted in IL
// since it is not inserted in m_LocalVariables.
//
LocalVariable * MethodSlots::AddResumableLocalVariable(_In_ BCSYM_Variable* Variable)
{
    VSASSERT(Variable && Variable->IsVariable() && Variable->GetRewrittenName(), "AddResumableLocalVariable: Symbol is invalid");
   
    LocalVariable * CurrentVariable = (LocalVariable *)m_SlotStorage->Alloc(sizeof(LocalVariable));

    CurrentVariable->VariableName = Variable->GetRewrittenName();
    CurrentVariable->Flags = 0;
    // Note that this slot is used by another local variable.  We can't give a real slot since this local isn't actually
    // present in the IL, but the VB EE invokes the C# EE visualizer service which requires all locals to have a valid slot #.
    CurrentVariable->SlotNumber = 0;    
    CurrentVariable->Signature = NULL;
    CurrentVariable->SignatureSize = 0;
    CurrentVariable->IsActive = true; // If we're not in EnC then always set this to true

    return CurrentVariable;
}

//-------------------------------------------------------------------------------------------------
//
// Finds and existing slot based on VariableName and Signature during an EnC session
//
// 

LocalVariable * MethodSlots::FindVariable(
    _In_z_ STRING * VariableName,
    BYTE * Signature,
    unsigned long SignatureSize,
    Location *VariableLocation)
{
    if (VariableLocation)
    {
        CSingleListIter<NewVariableLocation> IterNewVariableLocation(&m_NewLocalVariableLocations);
        NewVariableLocation *NewVarLoc;
        while (NewVarLoc = IterNewVariableLocation.Next())
        {
            if (NewVarLoc->Location.ContainsInclusive(VariableLocation))
            {
                return NULL;    // This is a new ENC variable.
            }
        }
    }

    LocalVariable * CurrentVariable;

    CSingleListIter<LocalVariable> IterLocals(&m_LocalVariables);

    // Iterate over all local variables currently known.  If the variable we are trying to add
    // matches one which already exists, then we "reuse" this slot.
    //
    while (CurrentVariable = IterLocals.Next())
    {
        if (StringPool::IsEqual(CurrentVariable->VariableName, VariableName) &&
            CurrentVariable->SignatureSize == SignatureSize &&
            memcmp(CurrentVariable->Signature, Signature, sizeof(BYTE) * SignatureSize) == 0 &&
            !CurrentVariable->IsActive)
        {
            // Mark this slot as "reused" so we don't find it again in case another variable
            // with the same name and signature exists (which can happen if both variables are
            // contained within different scopes)
            //
            CurrentVariable->IsActive = true;
            return CurrentVariable;
        }
    }

    return NULL;
}
