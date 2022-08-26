//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Functions and datastructures for Type Inference
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

Graph::Graph(
    _In_ NorlsAllocator * allocator,
    NorlsAllocWrapper * allocWrapper) :
    m_allocator(allocator),
    m_allocatorWrapper(allocWrapper),
    m_vertices(*allocWrapper),
    m_AlgorithmInProgress(false)
{
}

void Graph::AddEdge(
    _Out_ GraphNode * source,
    _Out_ GraphNode * target)
{
    if (!Contains(source))
    {
        AddNode(source);
    }
    if (!Contains(target))
    {
        AddNode(target);
    }

    source->m_Outgoing.Add(target);
    target->m_Incoming.Add(source);
}

void Graph::AddNode(_Out_ GraphNode * node)
{
    VSASSERT(!Contains(node), "Node already exists, why are you adding a vertice twice?");
    VSASSERT(node->m_Graph == NULL || node->m_Graph == this, "Node is already owned by a different tree?");
    node->m_Graph = this;
    m_vertices.Add(node);
}

void Graph::RemoveEdge(
    _Inout_ GraphNode * source,
    _Inout_ GraphNode * target)
{
    VSASSERT(Contains(source), "Source Node should be part of graph");
    VSASSERT(Contains(target), "Target Node should be part of graph");

    GraphNodeComparer comparer;
    source->m_Outgoing.RemoveLinear(target, comparer);
    target->m_Incoming.RemoveLinear(source, comparer);
}


NorlsAllocator * Graph::GetAllocator()
{
    return m_allocator;
}

NorlsAllocWrapper * Graph::GetAllocatorWrapper()
{
    return m_allocatorWrapper;
}

GraphNodeArrayList * Graph::GetVertices()
{
    return &m_vertices;
}


Graph * Graph::BuildStronglyConnectedComponents()
{
    Graph* sccGraph = new (*m_allocator) Graph(m_allocator, m_allocatorWrapper);

    // Step 1: Perform regular Dfs and build a list with the deepest node first.
    GraphNodeList orderedList(*GetAllocatorWrapper());
    Dfs(NULL, &TopoSortDfs, true /*useOutgoingEdges*/, &orderedList);

    // Step 2: Reset graph algorith Data
    GraphNodeListIterator resetIter(&orderedList);
    while (resetIter.MoveNext())
    {
        resetIter.Current()->m_AlgorithmData.Initialize();
    }

    // Step 3: Walk the nodes backwards and perform DFS. Place each tree in the forrest in a seperate node.
    unsigned int time = 0;
    GraphNodeListIterator iter(&orderedList);
    while (iter.MoveNext())
    {
        GraphNode* current = iter.Current();
        if (current->m_AlgorithmData.Color == 0)
        {
            StronglyConnectedComponent* sccNode = new(*m_allocator) StronglyConnectedComponent(sccGraph);

            DfsVisit(current, NULL, &BuildStronglyConnectedComponentsDfs, false /*useOutgoingEdges*/, sccNode, time);
            sccGraph->AddNode(sccNode);
        }
    }

    // Step 4: Link incoming and outgoing edges.
    GraphNodeArrayListIterator sccNodeIter(&sccGraph->m_vertices);
    while (sccNodeIter.MoveNext())
    {
        StronglyConnectedComponent* sccNode = (StronglyConnectedComponent*)sccNodeIter.Current();
        GraphNodeListIterator innerNodeIter(sccNode->GetChildNodes());
        while (innerNodeIter.MoveNext())
        {
            GraphNodeArrayListIterator innerOutGoingInter(&innerNodeIter.Current()->GetOutgoingEdges());
            while (innerOutGoingInter.MoveNext())
            {
                StronglyConnectedComponent* target = innerOutGoingInter.Current()->m_AlgorithmData.StronglyConnectedComponent;
                // Don't create self-edges.
                if (sccNode != target)
                {
                    sccGraph->AddEdge(sccNode, target);
                }
            }
        }
    }

    return sccGraph;
}

/*static*/
void Graph::BuildStronglyConnectedComponentsDfs(
    GraphNode * current,
    _In_ void * data)
{
    StronglyConnectedComponent* sccNode = (StronglyConnectedComponent*)data;
    sccNode->GetChildNodes()->Add(current);
    current->m_AlgorithmData.StronglyConnectedComponent = sccNode;
}

void Graph::TopoSort(_Inout_ GraphNodeList &resultList)
{
    Dfs(NULL, &TopoSortDfs, true /*useOutgoingEdges*/, &resultList);
}

/*static */
void Graph::TopoSortDfs(
    GraphNode * current,
    _Inout_ void * data)
{
    GraphNodeList* sortedList = (GraphNodeList*)data;
    sortedList->AddFirst(current);
}

void Graph::Dfs(
    void (*enter)(GraphNode* current, void* data),
    void (*leave)(GraphNode* current, void* data),
    bool useOutgoingEdges,
    _Inout_ void* data)
{
    StartAlgorithm();

    GraphNodeArrayListIterator iter(&m_vertices);
    unsigned int time = 0;
    while (iter.MoveNext())
    {
        GraphNode* current = iter.Current();
        if (current->m_AlgorithmData.Color == 0)
        {
            DfsVisit(current, enter, leave, useOutgoingEdges, data, time);
        }
    }

    FinishAlgorithm();
}

void Graph::DfsVisit(
    GraphNode* node,
    void (*enter)(GraphNode* current, void* data),
    void (*leave)(GraphNode* current, void* data),
    bool useOutgoingEdges,
    _Inout_ void* data,
    _Inout_ unsigned int &time)
{
    node->m_AlgorithmData.Color = 1; // grey

    time++;
    node->m_AlgorithmData.StartTime = time;
    if (enter)
    {
        enter(node, data);
    }

    GraphNodeArrayListIterator iter(useOutgoingEdges ? &node->m_Outgoing : &node->m_Incoming );
    while (iter.MoveNext())
    {
        GraphNode* current = iter.Current();
        if (current->m_AlgorithmData.Color == 0)
        {
            DfsVisit(current, enter, leave, useOutgoingEdges, data,time);
        }
    }

    time++;
    node->m_AlgorithmData.FinishTime = time;
    node->m_AlgorithmData.Color = 2; // black

    if (leave)
    {
        leave(node, data);
    }
}

void Graph::StartAlgorithm()
{
    VSASSERT(!m_AlgorithmInProgress, "Already have an algorithm running on this Graph!");

    m_AlgorithmInProgress = true;

    GraphNodeArrayListIterator iter(&m_vertices);
    while (iter.MoveNext())
    {
        iter.Current()->m_AlgorithmData.Initialize();
    }
}

void Graph::FinishAlgorithm()
{
    VSASSERT(m_AlgorithmInProgress, "No algorithm running, how can we finish one?");
    m_AlgorithmInProgress = false;
}

bool Graph::Contains(GraphNode * node)
{
    GraphNodeComparer comparer;
    return m_vertices.LinearSearch<GraphNodeComparer>(node, NULL, comparer);
}

GraphNode::GraphNode(_In_ Graph * graph) :
    m_Incoming(*graph->GetAllocatorWrapper()) ,
    m_Outgoing(*graph->GetAllocatorWrapper())
{
    m_Graph = graph;
}

GraphNodeArrayList& GraphNode::GetIncomingEdges()
{
    return m_Incoming;
}

GraphNodeArrayList& GraphNode::GetOutgoingEdges()
{
    return m_Outgoing;
}

void GraphAlgorithmData::Initialize()
{
    Color = 0;
    StartTime = 0;
    FinishTime = 0;
    StronglyConnectedComponent = NULL;
}

StronglyConnectedComponent::StronglyConnectedComponent(_In_ Graph * graph) :
    GraphNode(graph),
    m_childNodes(*graph->GetAllocatorWrapper())
{
}

GraphNodeList * StronglyConnectedComponent::GetChildNodes()
{
    return &m_childNodes;
}
