//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#pragma once

template
<
    typename KeyType,
    typename KeyOperations = SimpleKeyOperationsT<KeyType>
>
class SparseGraphT
{
private:
    struct SparceGraphNode : RedBlackNodeBaseT<KeyType>
    {
        ULONG iIndex;
    };

    typedef RedBlackTreeT
    <
        KeyType,
        KeyOperations,
        SparceGraphNode
    >
    InternalNodeTree;

    typedef DynamicArray<ULONG> DA_ULONG;

public:
    SparseGraphT() :
        m_pPaths(NULL),
        m_nrlsNodes(NORLSLOC),
        m_treeNodes(&m_nrlsNodes)
    {
    }

    virtual
    ~SparseGraphT()
    {
        ULONG cNodes = m_daEdges.Count();
        ULONG iIndex = 0;

        for (iIndex = 0; iIndex < cNodes; iIndex++)
        {
            DA_ULONG *pdaEdges = m_daEdges.Element(iIndex);
            delete pdaEdges;
        }

        m_daEdges.Collapse();
    }

    ULONG GetNodeIndex(const KeyType * pNode)
    {
        SparceGraphNode *pGraphNode = NULL;

        if (m_treeNodes.Insert(pNode, &pGraphNode))
        {
            pGraphNode->iIndex = m_daEdges.Count();

            DA_ULONG *pdaEdges = new DA_ULONG();
            m_daEdges.AddElement(pdaEdges);
        }

        if (pGraphNode)
        {
            return pGraphNode->iIndex;
        }

        return  0;
    }

    void InsertEdge(
        ULONG iSource,
        ULONG iTarget)
    {
        AssertIfFalse(iSource <m_daEdges.Count());
        AssertIfFalse(iTarget <m_daEdges.Count());

        InsertSortedNoDuplicate2(
            iTarget,
            m_daEdges.Element(iSource),
            &BinaryValueCompare<ULONG>);
    }

    void InsertEdge(
        const KeyType * pSrcNode,
        const KeyType * pDestNode)
    {
        InsertEdge(GetNodeIndex(pSrcNode), GetNodeIndex(pDestNode));
    }

    bool DoesPathExist(
        ULONG iSource,
        ULONG iTarget)
    {
        AssertIfTrue(m_pPaths != NULL);

        ULONG cNodes = m_daEdges.Count();

        AssertIfFalse(iSource < m_daEdges.Count());
        AssertIfFalse(iTarget < m_daEdges.Count());

        bool WasPathFound = false;

        if (cNodes > 0)
        {
            m_pPaths = VBAllocator::AllocateArray<bool>(cNodes);

            InternalDFS(iSource);

            WasPathFound = m_pPaths[iTarget];

            VBFree(m_pPaths);
            m_pPaths = NULL;
        }

        return WasPathFound;
    }

    bool DoesPathExist(
        const KeyType * pSrcNode,
        const KeyType * pDestNode)
    {
        return DoesPathExist(GetNodeIndex(pSrcNode), GetNodeIndex(pDestNode));
    }

private:
    void InternalDFS(ULONG SourceIndex)
    {
        m_pPaths[SourceIndex] = true;

        DA_ULONG *pdaEdges = m_daEdges.Element(SourceIndex);
        AssertIfNull(pdaEdges);

        for (ULONG iIndex = 0; iIndex < pdaEdges->Count(); iIndex++)
        {
            if (!m_pPaths[pdaEdges->Element(iIndex)])
            {
                InternalDFS(pdaEdges->Element(iIndex));
            }
        }
    }

private:
    bool *m_pPaths;

    DynamicArray<DA_ULONG *> m_daEdges;
    InternalNodeTree m_treeNodes;
    NorlsAllocator m_nrlsNodes;
};
