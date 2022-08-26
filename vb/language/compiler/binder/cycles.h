//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Detect and report cycles among a set of symbols.
//
//-------------------------------------------------------------------------------------------------

#pragma once

//-------------------------------------------------------------------------------------------------
//
// Helper classes to keep track of state for symbols as we find cycles
//
struct SymbolNode : CDoubleLink<SymbolNode>
{
    bool StartOfCycleChain; // is this the symbol for which we would like to find a loop back to?
    bool CurrentlyProcessing;  // whether we are currently processing the children for this node
    bool BeenProcessed; // whether this symbol and its children have been checked for cycles
    BCSYM_NamedRoot *m_Symbol; // the symbol itself
    BCSYM_NamedRoot *m_CallTarget; // if the symbol is a method, the thing the method calls
    SymbolNode *NextHashEntry;
};

struct CycleInfoList
{
    SymbolNode *Symbol; // The symbol involved in a cycle
    BCSYM *ReferringMember; // points to the member that referred us to Symbol (e.g. for a structure this is the member that provided the type for Symbol, above)
    CycleInfoList *NextInCurrentCycleChain; // points to the next cycle considered part of this cycle chain
    CycleInfoList *NextListOfCycleLists; // A symbol can have several lists of cycle lists; one for each individual member in a structure, for example
};

//-------------------------------------------------------------------------------------------------
//
// Detect and report cycles among a set of symbols. For instance, detect
// whether a class inheritance heirarchy forms a cycle: a inherits from b,
// b inherits from c, c inherits from a, etc.
//
class Cycles
{
public:

    Cycles(
        _In_ Compiler *pCompiler, 
        _In_ NorlsAllocator *pnra, 
        _In_opt_ ErrorTable *perror = NULL, 
        _In_ ERRID erridHeader = 0, 
        _In_ ERRID erridElement = 0);
    SymbolNode *AddSymbolToVerify( BCSYM_NamedRoot *SymbolToAdd, BCSYM_NamedRoot *SymbolToLink = NULL );
    void FindCycles(bool CheckRecordCycles = false, bool useSymbolErrorTable = false );
    CDoubleList<SymbolNode> *GetSymbolNodeList()
    {
        return &SymbolsToCheck;
    }

    bool HasCycle()
    {
        return m_CycleFound;
    }

private:

    void MergeCycleLists(CycleInfoList *ChildCycleList, CycleInfoList ** ParentCycleList );
    void CheckCyclesOnNode(SymbolNode *pnode, bool CheckRecordCycles);
    void BuildCycleList(SymbolNode *pnode, bool CheckRecordCycles, BCSYM *ReferringMember, CycleInfoList **ppcycle);
    void ReportErrorOnNode(SymbolNode *pnode, CycleInfoList *pcycleThisNode, bool useSymbolErrorTable);
    void GetName(SymbolNode *pnode, StringBuffer *pbuf);
    SymbolNode *FindNode(BCSYM_NamedRoot *pnamed);
    void BreakCycle(BCSYM_NamedRoot *pnode, CycleInfoList *ListOfCycleLists);
    ErrorTable * GetErrorTable(SymbolNode * pNode, bool useSymbolErrorTable);

    // Datamembers

    CDoubleList<SymbolNode> SymbolsToCheck;
    SymbolNode *m_SymbolHash[256];
    Compiler *m_CompilerInstance;
    NorlsAllocator *m_Allocator;
    ErrorTable *m_ErrorLog;
    ERRID m_erridHeader;
    ERRID m_erridElement;
    bool m_CycleFound; // True if we've found a cycle.
};
