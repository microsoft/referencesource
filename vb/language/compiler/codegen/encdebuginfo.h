//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Stores Information for generating/using EnC information.
//
//-------------------------------------------------------------------------------------------------

#pragma once

struct ConstantVariable : public CSingleLink<ConstantVariable>
{
    STRING        * VariableName;
    ConstantValue   Value;
    BYTE          * Signature;
    unsigned long   SignatureSize;
    mdSignature     SignatureToken;
};

struct LocalVariable : public CSingleLink<LocalVariable>
{
    STRING        * VariableName;
    BYTE          * Signature;
    unsigned long   SignatureSize;
    unsigned long   Flags;
    unsigned long   SlotNumber;
    // A value of False indicates that we're in EnC and we haven't reused this var
    bool            IsActive;
};

struct NewVariableLocation : public CSingleLink<NewVariableLocation>
{
    Location        Location;
};

class MethodSlots : public CSingleLink<MethodSlots>
{
public:
    void Init(
        Compiler * pCompiler,
        NorlsAllocator * pnra,
        mdToken MethodToken)
    {
        m_MethodToken = MethodToken;
        m_MethodIsOnStack = true;
        m_Compiler = pCompiler;
        m_SlotStorage = pnra;
    }

    HRESULT InitLocals(
        ENC_LOCALINFO *rgLocalInfo, 
        ULONG cLocals, 
        Location *plocNewLocals, 
        ULONG cNewLocals);

    void Reset();

    LocalVariable * AddVariable(
        BCSYM_Variable * Variable,
        unsigned long Flags,
        BYTE * Signature,
        unsigned long SignatureSize);
    
    LocalVariable * AddResumableLocalVariable(_In_ BCSYM_Variable* Variable);

    LocalVariable * FindVariable(
        _In_z_ STRING * VariableName,
        BYTE * Signature,
        unsigned long SignatureSize,
        Location *VariableLocation);

    ConstantVariable * AddConstant(
        BCSYM_Variable * Variable,
        BYTE * Signature,
        unsigned long SignatureSize);

    CSingleList<LocalVariable> * LocalVariableList()
    {
        return &m_LocalVariables;
    }

    CSingleList<ConstantVariable> * ConstantVariableList()
    {
        return &m_ConstantVariables;
    }

    mdToken m_MethodToken;
    bool m_MethodIsOnStack;

private:
    Compiler * m_Compiler;
    NorlsAllocator * m_SlotStorage;
    
    CSingleList<LocalVariable> m_LocalVariables;
    CSingleList<ConstantVariable> m_ConstantVariables;
    CSingleList<NewVariableLocation> m_NewLocalVariableLocations;
};
