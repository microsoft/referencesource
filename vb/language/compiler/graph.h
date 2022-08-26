//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Functions and datastructures for basic Graph operations
//
//-------------------------------------------------------------------------------------------------

#pragma once

class GraphNode;
class StronglyConnectedComponent;

typedef ArrayList<GraphNode*, NorlsAllocWrapper>            GraphNodeArrayList;
typedef ArrayListIterator<GraphNode *, NorlsAllocWrapper>   GraphNodeArrayListIterator;
typedef List<GraphNode*, NorlsAllocWrapper>                 GraphNodeList;
typedef ListValueIterator<GraphNode *, NorlsAllocWrapper>   GraphNodeListIterator;

class GraphNodeComparer
{
public:
    GraphNodeComparer()
    {
    }

    static
    int Compare(
        GraphNode * left,
        GraphNode * right)
    {
        return left == right ? 0 : 1;
    }
};

struct GraphAlgorithmData
{
    // DFS specific fields
    unsigned short Color;
    unsigned int StartTime;
    unsigned int FinishTime;

    // Used for quick lookup of which strongly connected compoent this node is in.
    StronglyConnectedComponent* StronglyConnectedComponent;

    void Initialize();
};
class GraphNode;

class Graph
{
    friend GraphNode;

public:
    Graph(
        _In_ NorlsAllocator * allocator,
        NorlsAllocWrapper * allocWrapper);

    void AddEdge(
        _Out_ GraphNode * source,
        _Out_ GraphNode * target);

    void AddNode(_Out_ GraphNode * node);

    void RemoveEdge(
        _Inout_ GraphNode * source,
        _Inout_ GraphNode * target);

    Graph* BuildStronglyConnectedComponents();

    void TopoSort(_Inout_ GraphNodeList& resultList);

    static
    void BuildStronglyConnectedComponentsDfs(
        GraphNode * current,
        _In_ void * data);

    static
    void TopoSortDfs(
        GraphNode * current,
        _Inout_ void * data);

    void Dfs(
        void (*enter)(GraphNode* current, void* data),
        void (*leave)(GraphNode* current, void* data),
        bool useOutgoingEdges,
        _Inout_ void* data);

    void DfsVisit(
        GraphNode* node,
        void (*enter)(GraphNode* current, void* data),
        void (*leave)(GraphNode* current, void* data),
        bool useOutgoingEdges,
        _Inout_ void* data,
        _Inout_ unsigned int &time);

    NorlsAllocator * GetAllocator();
    NorlsAllocWrapper * GetAllocatorWrapper();
    GraphNodeArrayList * GetVertices();

private:
    NorlsAllocator*     m_allocator;
    NorlsAllocWrapper*  m_allocatorWrapper;

protected:
    void StartAlgorithm();
    void FinishAlgorithm();
    bool Contains(GraphNode * node);
    bool m_AlgorithmInProgress;

    // Please ensure declaration after the allocators since they need to be assigned before this constructor gets called.
private:
    GraphNodeArrayList  m_vertices;

};

#pragma prefast(suppress: 25094, "No destructors in the Graph's")
class GraphNode
{
    friend Graph;

protected:
    GraphNode(_In_ Graph *graph);

public:
    unsigned int    m_NodeType;

#if DEBUG
    WCHAR * DbgPrint()
    {
        const size_t bufferSize = 1024;
        WCHAR *text[bufferSize];
        #pragma prefast(suppress: 28645, "No MessageBox being called here.")
        StringBuffer buffer = StringBuffer((WCHAR*)text, (size_t)bufferSize);
        DbgPrint(&buffer);

        // Need to copy buffer because StringBuffers desctructor will
        // write out NULL in the buffer, even though you pass a buffer
        // on the heap :(
        int textSize = (buffer.GetStringLength() + 1) * sizeof(WCHAR);
        WCHAR* textTemp = (WCHAR*)m_Graph->GetAllocator()->Alloc(textSize);
        memcpy(textTemp, buffer.GetString(), textSize);
        return textTemp;
    }

    virtual
    void DbgPrint(StringBuffer * buffer)
    {
        buffer->AppendString(L"Node:");
    }
#endif

    GraphNodeArrayList& GetIncomingEdges();
    GraphNodeArrayList& GetOutgoingEdges();

protected:
    Graph               *m_Graph;

private:
    GraphNodeArrayList  m_Incoming;
    GraphNodeArrayList  m_Outgoing;

    GraphAlgorithmData  m_AlgorithmData;
};


class StronglyConnectedComponent : public GraphNode
{
public:
    StronglyConnectedComponent(_In_ Graph * graph);

#if DEBUG
    virtual
    void DbgPrint(StringBuffer * buffer)
    {
        buffer->AppendString(L"SSC {");
        GraphNodeListIterator iter(&m_childNodes);
        while (iter.MoveNext())
        {
            ((StronglyConnectedComponent*)iter.Current())->DbgPrint(buffer);
            buffer->AppendString(L",");
        }
        buffer->AppendString(L"}");
    };
#endif

    GraphNodeList*  GetChildNodes();

private:
    GraphNodeList   m_childNodes;
};
