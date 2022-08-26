//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Red Black Tree and Interval Tree Templates.
//
//-------------------------------------------------------------------------------------------------

#pragma once

//============================================================================
// Red Black Tree Templates:
//
// The following templates contain the implementation of a Red Black Tree described in:
//     CORMEN, Thomas; LEISERSON, Charles and RIVEST, Ronald: "Introduction to Algorithms"
//     McGraw Hill, 24th printing, 2000.
//
//============================================================================


//============================================================================
// RedBlackColor:
//      Enumeration that defines the possible colors of a node in the Red-Black Tree.
//============================================================================
enum RedBlackColor
{
    rbcBlack,
    rbcRed
};

//============================================================================
// RedBlackNodeBaseT:
//
//      Base class template for nodes that form part of a Red Black Tree. You
//      should use this type as a base class for your own node.
//
//      Note: although the fileds for this class are public, they were not intended
//      to be modified by the user.
//============================================================================
template <typename KeyType>
struct RedBlackNodeBaseT
{
    private:
        typedef RedBlackNodeBaseT<KeyType> RedBlackNodeBase;

    public:
        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        RedBlackNodeBaseT() :
            pParent(NULL),
            pLeft (NULL),
            pRight(NULL),
            color(rbcBlack)
#if DEBUG
            ,pOwner(NULL)
#endif
        {
        }

    // Public members to manipulate tree structure.
    public:
        RedBlackNodeBase *pParent;       // Pointer to the node's parent.
        RedBlackNodeBase *pLeft;         //Pointer to the node's left child.
        RedBlackNodeBase *pRight;        //Pointer to the node's right child.

        RedBlackColor color;                // Color of the node

        KeyType key;                        // Index key

#if DEBUG
        void *pOwner;           // Pointer to the tree that owns this node.
#endif
};

//============================================================================
// SimpleKeyOperationsT:
//
//      Simple behavior class template for intrinsic types. It is used as a default implementation for the
//      KeyOperations template parameter.
//============================================================================
template <typename KeyType>
struct SimpleKeyOperationsT
{
    int compare(
        const KeyType * pKey1,
        const KeyType * pKey2)
    {
        return CompareValues(*pKey1, *pKey2);
    }

    void copy(
        _Out_ KeyType * pKeyDest,
        const KeyType * pKeySrc)
    {
        *pKeyDest = *pKeySrc;
    }

    static
    void destroy(KeyType * pKey)
    {
    }
};

//============================================================================
// EmptyNodeOperationsT:
//
//      Simple behavior class template for nodes without any additional information. It is used as a default
//      implementation for the NodeOperations template parameter.
//============================================================================
template <typename NodeType>
struct EmptyNodeOperationsT
{
    void copy(
        NodeType * pNodeDest,
        const NodeType * pNodeSrc)
    {
    }

    void destroy(NodeType *pNode)
    {
    }
};

// 




//============================================================================
// RedBlackTreeT:
//
//      Template implementation of Red Black Binary Search Tree. The tree doesn't
//      support insertion of duplicate items.
//
//  Template Parameters:
//
//      KeyType: Specifies the type of the index key.
//
//      NodeType: [Optional] Specifies the type of the nodes that the tree will
//                      hold. If specified, this type must privately derive from
//                      RedBlackNodeBaseT. If it is not specified, a simple
//                      implementation of RedBlackNodeBaseT specialized for
//                      KeyType, without any additional data, will be used.
//
//      KeyOperations: [Optional] Specifies an inline class to manipulate the
//                             keys of the tree. If it is not specified, a simple
//                             implementation for intrinsic types will be used. The
//                             type should define the following methods,
//                             which should adequately handle the requirements
//                             for the data:
//
//           int compare(const <KeyType> *key1, const <KeyType> *key2)
//                  - performs a comparison between key1 and key2
//                  - returns: 0 if key1 = key2
//                                 <0 if key1 < key2
//                                 >0 if key1 > key2
//
//           void destroy (<KeyType> *key)
//                  - performs any required cleanup for key.
//
//           void copy (<KeyType> *destkey, const <KeyType> *srckey)
//                  - copies the contents of srckey into destkey.
//
//      NodeOperations: [Optional] Specifies an inline class to manipulate
//                               additional information in the node. If it is not
//                               specified, a default implementation for nodes
//                               without any additional data will be used. The
//                               type should define the following methods,
//                               which should adequately handle the requirements
//                               for the data:
//
//              void destroy (NodeType *node)
//                  - performs any required cleanup on the additional information on the node.
//
//              void copy (NodeType *destnode, const NodeType *srcnode)
//                  - copies the additional contents of srcnode into destnode.
//
//       CriticalSectionType: [Optional] Specify a critical section to use for
//                                     synchornization. Must be either
//                                     CComAutoCriticalSection or CComFakeCriticalSection
//============================================================================
template
    <
        typename KeyType,
        typename KeyOperations = SimpleKeyOperationsT<KeyType>,
        typename NodeType  = RedBlackNodeBaseT<KeyType>,
        typename NodeOperations = EmptyNodeOperationsT<NodeType>,
        typename CriticalSectionType = CComFakeCriticalSection
    >
class RedBlackTreeT
{
    friend class CVbCompilerCompCache; // test hook
    // ------------------------------------------------------------------------
    // Typedef's to avoid using template parameters all around the place.
    public:
        typedef NodeType RedBlackNode;

    protected:
        typedef RedBlackNodeBaseT<KeyType> RedBlackNodeBase;
        typedef RedBlackTreeT<KeyType, KeyOperations, NodeType, NodeOperations, CriticalSectionType> RedBlackTree;


    public:
        NorlsAllocator * GetNorlsAllocator()
        {
            return m_pNoReleaseAllocator;
        }

        CriticalSectionType& GetCriticalSection()
        {
            return m_cs;
        }

#if DEBUG
        CComBSTR GetNodeDump(_In_opt_ bool fOutputDebugWindow ) ; // dump the contents into a string
        void ValidateNodes();   // assert on the nodes
#endif DEBUG

        //============================================================================
        //     Nested class that defines an in-order iterator for Red Black Trees.
        //============================================================================
        class Iterator
        {
            public:
                //============================================================================
                // Constructor: Initialize the iterator for the given tree.
                //============================================================================
                Iterator
                (
                    RedBlackTree *pRedBlackTree  // [in] Tree to get the iterator for. It shouldn't be null
                ) :
                    m_pTree(pRedBlackTree)
                {
                    VSASSERT(pRedBlackTree != NULL, "Invalid Null Pointer");
                    VSASSERT(pRedBlackTree->m_pNoReleaseAllocator != NULL, "Invalid Null Pointer");

                    Reset();
                }

                //============================================================================
                // Reset the iterator to the first element.
                //============================================================================
                void Reset()
                {
                    // Clear the stack.
                    m_NodeStack.Reset();

                    // Set the initial stack.
                    AddToStack(m_pTree->m_pRoot);
                }

                //============================================================================
                // Get the next 'in-order' element from the tree
                //
                //    Return values:
                //        A pointer to the next node in the tree, or
                //        NULL if  the iterator has reached the end of the tree.
                //============================================================================
                RedBlackNode * Next()
                {
                    // Check if there are any elements left.
                    if (m_NodeStack.Empty())
                    {
                        return NULL;
                    }

                    // Retrieve the current element.
                    RedBlackNodeBase *pCurrentNode = m_NodeStack.Top();
                    m_NodeStack.Pop();

                    // Add the right subtree to the stack.
                    AddToStack(pCurrentNode->pRight);

#if DEBUG_VERIFY_REDBLACKTREE
                    if (!m_NodeStack.Empty())
                    {
                        VSASSERT(m_pTree->TreeSuccessor(pCurrentNode) == m_NodeStack.Top(), "Invalid Successor");
                    }
#endif

                    // Return the current node
                    return static_cast<RedBlackNode *>(pCurrentNode);
                }

            private:
                //============================================================================
                // AddToStack:
                //
                //      Adds the left branch of the given node to the stack.
                //============================================================================
                void AddToStack(RedBlackNodeBase * pIndexNode)
                {
                    VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

                    // Iterate over all the left children.
                    while (pIndexNode != &m_pTree->m_Sentinel)
                    {
                        m_NodeStack.Push(pIndexNode);

                        pIndexNode = pIndexNode->pLeft;
                    }
                }

            private:
                Stack <RedBlackNodeBase *> m_NodeStack;     // Stack for in-order walk.
                RedBlackTree *m_pTree;   // Current tree.
        };

    friend Iterator;

    public:
        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        RedBlackTreeT
        (
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        ) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(NULL),
            m_pRoot(&m_Sentinel),
            m_cNodes(0),
            m_pLastUnusedNode(NULL),
            m_nodeOperations(),
            m_keyOperations()
        {
            // Clear the contents of the tree
            Clear();
        }

        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        RedBlackTreeT
        (
            _In_ NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        ) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(pNoReleaseAllocator),
            m_pRoot(&m_Sentinel),
            m_cNodes(0),
            m_pLastUnusedNode(NULL),
            m_nodeOperations(),
            m_keyOperations()
        {
            // Clear the contents of the tree
            Clear();
        }

        RedBlackTreeT(
            NorlsAllocator * pNoReleaseAllocator,
            bool ShouldReleaseData,
            const NodeOperations &nodeOps,
            const KeyOperations &keyOps) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(pNoReleaseAllocator),
            m_pRoot(&m_Sentinel),
            m_cNodes(0),
            m_nodeOperations(nodeOps),
            m_keyOperations(keyOps)
        {
            Clear();
        }

        const KeyOperations & GetKeyOperations()
        {
            return m_keyOperations;
        }

        //============================================================================
        // Destructor: Clear the tree
        //============================================================================
        ~RedBlackTreeT()
        {
            // Clear the contents of the tree
            Clear();
        }

        //============================================================================
        // Init: Initializes the red black tree.
        //============================================================================
        void
        Init
        (
            NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        )
        {
            VSASSERT(m_pNoReleaseAllocator == NULL, "Red Black Tree has already been initialized");

            // The tree can only be initialized once.
            if (m_pNoReleaseAllocator == NULL)
            {
                m_pNoReleaseAllocator = pNoReleaseAllocator;
                m_ShouldReleaseData = ShouldReleaseData;

                // Clear the contents of the tree
                Clear();
            }
        }

        void
        Init
        (
            NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            const KeyOperations &keyOps,            // [in] Key operations (will be copied)
            bool ShouldReleaseData = false          // [in, optional] should the data be released before releasing the node?
        )
        {
            Init(pNoReleaseAllocator, ShouldReleaseData);

            m_keyOperations = keyOps;
        }

        NorlsAllocator *
        GetAllocator()
        {
            return m_pNoReleaseAllocator;
        }

        //============================================================================
        // Clear: Clears the contents of the tree.
        //============================================================================
        void Clear()
        {
            // If the flag is set, recursively walk the tree and call the behavior classes' destroy method.
            if (m_ShouldReleaseData)
            {
                ClearSubTree(m_pRoot);
            }

            m_cNodes = 0;
            m_pRoot = &m_Sentinel;
            m_pLastUnusedNode = NULL;

#if DEBUG
            m_IsRemove = false;
            ClearTreeStats();
#endif
        }

        ULONG Count()
        {
            return m_cNodes;
        }

        //============================================================================
        // Find:
        //
        //      Searches the tree for a node that matches the specified key.
        //
        //      Return values:
        //          -True, if a node matching the key was found; or
        //          -False, if such a node couldn't be found.
        //============================================================================
        bool
        Find
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            _Out_opt_ NodeType **ppNode = NULL  // [optional, out] If not NULL, on return it will contain the pointer to the node matching the given key or NULL if it wasn't found.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            RedBlackNodeBase *pIndexNode = m_pRoot;

            while (pIndexNode != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(pKey, &pIndexNode->key);

#if DEBUG
                if (m_IsRemove)
                {
                     // Total depth of all combined deletes to obtain the average.
                    m_cDeletionDepth++;
                }
                else
                {
                     // Total depth of all combined searches to obtain the average.
                    m_cSearchDepth++;
                }
#endif

                if (iComp == 0)
                {
#if DEBUG
                    if (m_IsRemove)
                    {
                         // Number of deletion hits
                        m_cDeletionHits++;
                    }
                    else
                    {
                        // Number of hits
                        m_cSearchHits++;
                    }
#endif

                    if (ppNode)
                    {
                        *ppNode = static_cast<NodeType *>(pIndexNode);
                    }

                    return true;
                }
                else if (iComp < 0)
                {
                    pIndexNode = pIndexNode->pLeft;
                }
                else
                {
                    pIndexNode = pIndexNode->pRight;
                }
            }

#if DEBUG
            if (m_IsRemove)
            {
                 // Number of deletion hits
                m_cDeletionMisses++;
            }
            else
            {
                // Number of misses
                m_cSearchMisses++;
            }
#endif

            if (ppNode)
            {
                *ppNode = NULL;
            }

            return false;
        }

        //============================================================================
        // Insert:
        //
        //      Function to insert a new node with the given key into the appropriate position in the search tree.
        //
        //      Return value:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        Insert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            _Out_opt_ NodeType **ppNode  = NULL // [optional, out] If not null, on return it will contain the pointer to the node matching the given key.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            RedBlackNodeBase *pReturnedNode = NULL;

            // Insert the new node
            bool IsNewNode = RBInsert(pKey, &pReturnedNode);

            // Convert the node pointer to the appropriate class
            if (ppNode)
            {
                *ppNode = static_cast<NodeType *> (pReturnedNode);
            }

#if DEBUG_VERIFY_REDBLACKTREE
            //      Make sure properties still hold
            VerifyTree();
#endif

            if (IsNewNode)
            {
                m_cNodes++;
            }

            // Return whether it is a new node or not.
            return IsNewNode;
        }

        //============================================================================
        // Lock:
        //
        //      Function to lock the tree by using a critical section. Use
        //      UnLock to release the trees.
        //
        //      Make sure to specifiy the right template argument for
        //      the critical section.
        //
        //============================================================================
        void Lock()
        {
            m_cs.Lock();
        }

        //============================================================================
        // Remove:
        //
        //      Function to remove the node that matches the given key from the tree.
        //
        //      Return value:
        //          -True, if the node was removed from the tree; or
        //          -False, if the node doesn't exist in tree.
        //============================================================================
        bool
        Remove
        (
            const KeyType *pKey    // [in] Key of the node to delete. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            NodeType *pNodeToDelete = NULL;

#if DEBUG
            m_IsRemove = true;
#endif

            if (Find(pKey, &pNodeToDelete))
            {
                RBDelete(pNodeToDelete);
                m_cNodes--;

#if DEBUG_VERIFY_REDBLACKTREE
                VerifyTree();
                m_IsRemove = false;
#endif
                return true;
            }

            return false;
        }

        //============================================================================
        // Remove:
        //
        //      Function to remove the given node from the tree.
        //
        //      Return value:
        //          -True:  the node was removed from the tree.
        //============================================================================
        bool
        Remove
        (
            NodeType *pNodeToDelete        // [in] Node to delete. It shouldn't be NULL.
        )
        {
            VSASSERT(pNodeToDelete != NULL, "Invalid Null Pointer");

#if DEBUG
            VSASSERT(pNodeToDelete->pOwner == this, "The node doesn't belong to the tree");
#endif

            RBDelete(pNodeToDelete);
            m_cNodes--;

#if DEBUG_VERIFY_REDBLACKTREE
            VerifyTree();
#endif
            return true;
        }

        //============================================================================
        // UnLock:
        //
        //      Function to unlock the tree. Use Lock to lock it..
        //
        //      Make sure to specifiy the right template argument for
        //      the critical section.
        //
        //============================================================================
        void UnLock()
        {
            m_cs.Unlock();
        }

    protected:
        //============================================================================
        // ClearSubTree:
        //
        //      Helper function that recursively calls the destroy method of the policy classes for each
        //      node in the tree.
        //
        //============================================================================
        void
        ClearSubTree
        (
            RedBlackNodeBase *pNode        // [in] Current sub tree. It shouldn't be NULL.
        )
        {
            VSASSERT (pNode != NULL, "Invalid NULL pointer");

            if (pNode != &m_Sentinel)
            {
                // Call the destroy methods on the behavior classes.
                m_keyOperations.destroy(&pNode->key);
                m_nodeOperations.destroy(static_cast<NodeType *> (pNode));

                // Recursively release the child trees.
                ClearSubTree(pNode->pLeft);
                ClearSubTree(pNode->pRight);
            }
        }

        //============================================================================
        // RBDelete:
        //
        //      Helper function to delete the given node from the tree and preserve the red black balance properties afterwards.
        //============================================================================
        void
        RBDelete
        (
            RedBlackNodeBase *pNodeToDelete  // [in] Node to remove form the tree.
        )
        {
            VSASSERT(pNodeToDelete != NULL, "Invalid Null Pointer");

            RedBlackNodeBase *pRealNodeToDelete = NULL;
            RedBlackNodeBase *pChildNode = NULL;

            // Locate the real node that will be deleted. If the pNodeToDelete has at most one child,
            // then use that one; otherwise, get the node that's the successor to pNodeToDelete.
            if (pNodeToDelete->pLeft == &m_Sentinel || pNodeToDelete->pRight == &m_Sentinel)
            {
                pRealNodeToDelete = pNodeToDelete;
            }
            else
            {
                pRealNodeToDelete = TreeSuccessor(pNodeToDelete);
            }

            // Save a pointer to the only child of pRealNodeToDelete
            if (pRealNodeToDelete->pLeft != &m_Sentinel)
            {
                pChildNode = pRealNodeToDelete->pLeft;
            }
            else
            {
                pChildNode = pRealNodeToDelete->pRight;
            }

            // Set the child's parent to its grandparent.
            pChildNode->pParent = pRealNodeToDelete->pParent;

            // Link the child node direclty to its grandparent.
            if (pRealNodeToDelete->pParent == &m_Sentinel)
            {
                m_pRoot = pChildNode;
            }
            else if (pRealNodeToDelete == pRealNodeToDelete->pParent->pLeft)
            {
                pRealNodeToDelete->pParent->pLeft = pChildNode;
            }
            else
            {
                pRealNodeToDelete->pParent->pRight = pChildNode;
            }

            // If pNodeToDelete is not the real node to delete, then copy the data over.
            if (pRealNodeToDelete != pNodeToDelete)
            {
                // If the flag is set, then release the data.
                if (m_ShouldReleaseData)
                {
                    m_keyOperations.destroy(&pNodeToDelete->key);
                    m_nodeOperations.destroy(static_cast<NodeType *> (pNodeToDelete));
                }

                // Copy the data to the new location.
                m_keyOperations.copy(&pNodeToDelete->key, &pRealNodeToDelete->key);
                m_nodeOperations.copy(static_cast<NodeType *>(pNodeToDelete), static_cast<NodeType *>(pRealNodeToDelete));
            }

            // Release the data
            if (m_ShouldReleaseData)
            {
                m_keyOperations.destroy(&pRealNodeToDelete->key);
                m_nodeOperations.destroy(static_cast<NodeType *> (pRealNodeToDelete));
            }

            // Restore the Red Black Properties.
            if (pRealNodeToDelete->color == rbcBlack)
            {
                RBDeleteFixup(pChildNode);
            }

            m_pLastUnusedNode = static_cast<NodeType *> (pRealNodeToDelete);
        }

        //============================================================================
        // RBDeleteFixup:
        //
        //      Helper function to restore the red black properties after a deletion.
        //============================================================================
        void
        RBDeleteFixup
        (
            RedBlackNodeBase *pIndexNode  // [in] Node to fix properties for
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");


            // Make sure the Red Black Properties are maintained.
            while (pIndexNode != m_pRoot && pIndexNode->color == rbcBlack)
            {
                RedBlackNodeBase *pSiblingNode = NULL;

                if (pIndexNode == pIndexNode->pParent->pLeft)
                {
                    // ---- Left child case scenario ----
                    pSiblingNode = pIndexNode->pParent->pRight;

                    if (pSiblingNode->color == rbcRed)
                    {
                        // Case 1:
                        pSiblingNode->color = rbcBlack;
                        pIndexNode->pParent->color = rbcRed;

                        RBRotateLeft(pIndexNode->pParent);

                        pSiblingNode = pIndexNode->pParent->pRight;
                    }

                    if (pSiblingNode->pLeft->color == rbcBlack && pSiblingNode->pRight->color == rbcBlack)
                    {
                        // Case 2:
                        pSiblingNode->color = rbcRed;
                        pIndexNode = pIndexNode->pParent;
                    }
                    else
                    {
                        if (pSiblingNode->pRight->color == rbcBlack)
                        {
                            //Case 3:
                            pSiblingNode->pLeft->color = rbcBlack;
                            pSiblingNode->color = rbcRed;

                            RBRotateRight(pSiblingNode);

                            pSiblingNode = pIndexNode->pParent->pRight;
                        }

                        // Case 4:
                        pSiblingNode->color = pIndexNode->pParent->color;
                        pIndexNode->pParent->color = rbcBlack;
                        pSiblingNode->pRight->color = rbcBlack;

                        RBRotateLeft(pIndexNode->pParent);

                        pIndexNode = m_pRoot;
                    }
                }
                else
                {
                    // ---- Right child case scenario ----

                    pSiblingNode = pIndexNode->pParent->pLeft;

                    if (pSiblingNode->color == rbcRed)
                    {
                        // Case 1:
                        pSiblingNode->color = rbcBlack;
                        pIndexNode->pParent->color = rbcRed;

                        RBRotateRight(pIndexNode->pParent);

                        pSiblingNode = pIndexNode->pParent->pLeft;
                    }

                    if (pSiblingNode->pRight->color == rbcBlack && pSiblingNode->pLeft->color == rbcBlack)
                    {
                        // Case 2:
                        pSiblingNode->color = rbcRed;
                        pIndexNode = pIndexNode->pParent;
                    }
                    else
                    {
                        if (pSiblingNode->pLeft->color == rbcBlack)
                        {
                            //Case 3:
                            pSiblingNode->pRight->color = rbcBlack;
                            pSiblingNode->color = rbcRed;

                            RBRotateLeft(pSiblingNode);

                            pSiblingNode = pIndexNode->pParent->pLeft;
                        }

                        // Case 4:
                        pSiblingNode->color = pIndexNode->pParent->color;
                        pIndexNode->pParent->color = rbcBlack;
                        pSiblingNode->pLeft->color = rbcBlack;

                        RBRotateRight(pIndexNode->pParent);

                        pIndexNode = m_pRoot;
                    }
                }
            }
            pIndexNode->color = rbcBlack;
        }

        //============================================================================
        // RBInsert:
        //
        //      Helper function to insert a new node with the given key into the appropriate position in the search tree
        //      and preserve the red black balance properties afterwards.
        //
        //      Return value:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        RBInsert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            _Out_ RedBlackNodeBase **ppNode  // [out] On return it will contain the pointer to the node matching the given key. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");
            VSASSERT(ppNode != NULL, "Invalid Null Pointer");

            // Try to insert a new node with the given key into the tree.
            if (!TreeInsert(pKey, ppNode))
            {
                return false;
            }

            // Set the IndexNode to the newly created node and color it red.
            RedBlackNodeBase *pIndexNode = *ppNode;
            pIndexNode->color = rbcRed;

            // Make sure the Red Black Tree properties are maintained.
            while (pIndexNode != m_pRoot && pIndexNode->pParent->color == rbcRed)
            {
                if (pIndexNode->pParent == pIndexNode->pParent->pParent->pLeft)
                {
                    RedBlackNodeBase *pUncleNode = pIndexNode->pParent->pParent->pRight;

                    if (pUncleNode->color == rbcRed)
                    {
                        // Case 1:
                        pIndexNode->pParent->color = rbcBlack;
                        pUncleNode->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        pIndexNode = pIndexNode->pParent->pParent;
                    }
                    else
                    {
                        if (pIndexNode == pIndexNode->pParent->pRight)
                        {
                            // Case 2:
                            pIndexNode = pIndexNode->pParent;

                            RBRotateLeft(pIndexNode);
                        }

                        // Case 3:
                        pIndexNode->pParent->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        RBRotateRight(pIndexNode->pParent->pParent);
                    }
                }
                else
                {
                    RedBlackNodeBase *pUncleNode = pIndexNode->pParent->pParent->pLeft;

                    if (pUncleNode->color == rbcRed)
                    {
                        // Case 1:
                        pIndexNode->pParent->color = rbcBlack;
                        pUncleNode->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        pIndexNode = pIndexNode->pParent->pParent;
                    }
                    else
                    {
                        if (pIndexNode == pIndexNode->pParent->pLeft)
                        {
                            // Case 2:
                            pIndexNode = pIndexNode->pParent;

                            RBRotateRight(pIndexNode);
                        }

                        // Case 3:
                        pIndexNode->pParent->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        RBRotateLeft(pIndexNode->pParent->pParent);
                    }
                }
            }

            m_pRoot->color = rbcBlack;

            return true;
        }

        //============================================================================
        // RBRotateLeft:
        //
        //      Helper function that performs a left rotation of the tree at the given pivot node.
        //============================================================================
        void
        RBRotateLeft
        (
            _Inout_ RedBlackNodeBase *pPivotNode    // [in] Node at which to perform the left rotation. It shouldn't be null.
        )
        {
            VSASSERT(pPivotNode != NULL, "Invalid Null Pointer");

            // Get the right child of the pivotnode.
            RedBlackNodeBase *pChildTree = pPivotNode->pRight;

            // Turn the child's left subtree into the pivot's right subtree, and update the parent if necessary.
            pPivotNode->pRight = pChildTree->pLeft;
            if (pChildTree->pLeft != &m_Sentinel)
            {
                pChildTree->pLeft->pParent = pPivotNode;
            }

            // Link the pivot's parent to the child tree.
            pChildTree->pParent = pPivotNode->pParent;

            // If the parent is NIL, then set as the child tree as the new root.
            // Else, if the pivot is the left child of its parent, set the child tree as its new left child.
            // Otherwise set the child tree as the new right child of the pivot's parent.
            if (pPivotNode->pParent == &m_Sentinel)
            {
                m_pRoot = pChildTree;
            }
            else if (pPivotNode == pPivotNode->pParent->pLeft)
            {
                pPivotNode->pParent->pLeft = pChildTree;
            }
            else
            {
                pPivotNode->pParent->pRight = pChildTree;
            }

            // Set the pivot node as the new left child of the child tree.
            pChildTree->pLeft = pPivotNode;
            pPivotNode->pParent = pChildTree;
        }

        //============================================================================
        // RightRotate:
        //
        //      Helper function that performs a right rotation of the tree at the given pivot node.
        //============================================================================
        void
        RBRotateRight
        (
            _Inout_ RedBlackNodeBase *pPivotNode    // [in] Node at which to perform the right rotation. It shouldn't be null.
        )
        {
            VSASSERT(pPivotNode != NULL, "Invalid Null Pointer");

            // Get the left child of the pivotnode.
            RedBlackNodeBase *pChildTree = pPivotNode->pLeft;

            // Turn the child's right subtree into the pivot's left subtree, and update the parent if necessary.
            pPivotNode->pLeft = pChildTree->pRight;
            if (pChildTree->pRight != &m_Sentinel)
            {
                pChildTree->pRight->pParent = pPivotNode;
            }

            // Link the pivot's parent to the child tree.
            pChildTree->pParent = pPivotNode->pParent;

            // If the parent is NIL, then set as the child tree as the new root.
            // Else, if the pivot is the left child of its parent, set the child tree as its new left child.
            // Otherwise set the child tree as the new right child of the pivot's parent.
            if (pPivotNode->pParent == &m_Sentinel)
            {
                m_pRoot = pChildTree;
            }
            else if (pPivotNode == pPivotNode->pParent->pLeft)
            {
                pPivotNode->pParent->pLeft = pChildTree;
            }
            else
            {
                pPivotNode->pParent->pRight = pChildTree;
            }

            // Set the pivot node as the new right child of the child tree.
            pChildTree->pRight = pPivotNode;
            pPivotNode->pParent = pChildTree;
        }

        //============================================================================
        // TreeInsert:
        //
        //      Helper function to insert the pNewNode node into the appropriate position in the search tree.
        //
        //      Return values:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        TreeInsert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            _Out_ RedBlackNodeBase **ppNode  // [out] On return it will contain the pointer to the node matching the given key. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            RedBlackNodeBase *pIndexParentNode = &m_Sentinel;      // Path index's parent.
            RedBlackNodeBase *pIndexNode = m_pRoot;               // Path index.

            while (pIndexNode != &m_Sentinel)
            {
                pIndexParentNode = pIndexNode;

#if DEBUG
                // Total depth of all combined insertions to obtain the average.
                m_cInsertionDepth++;
#endif

               // Compare keys
                int ret = m_keyOperations.compare(pKey, &pIndexNode->key);

                // If the comparison returned:
                //      = 0: Exit because we don't support multiple nodes with the same key.
                //      < 0: Move the index to the left child.
                //      > 0: Move the index to the right child.
                if (ret == 0)
                {
#if DEBUG
                    // Number of misses
                    m_cInsertionMisses++;
#endif
                    // Set the node that matches the key, and indicate it already existed.
                    *ppNode = pIndexNode;
                    return false;
                }
                else if (ret < 0)
                {
                    pIndexNode = pIndexNode->pLeft;
                }
                else
                {
                    pIndexNode = pIndexNode->pRight;
                }
            }

            // Create the new node
            RedBlackNodeBase *pNewNode = NULL;
            if (!m_pLastUnusedNode)
            {
                pNewNode = (NodeType *) m_pNoReleaseAllocator->Alloc(sizeof(NodeType));
            }
            else
            {
                // Reuse the last deleted node's allocated space.
                pNewNode = m_pLastUnusedNode;
                m_pLastUnusedNode = NULL;
            }

            //Call the constructor for the memory allocated
            pNewNode = new ((void*)pNewNode) NodeType;

            // Copy the key into the new node.
            m_keyOperations.copy(&pNewNode->key, pKey);

            pNewNode->pParent = pIndexParentNode;
            pNewNode->pLeft = &m_Sentinel;
            pNewNode->pRight = &m_Sentinel;

#if DEBUG
            pNewNode->pOwner = this;
#endif

            // If the parent's index is NIL this is the first node, so set the root of the tree.
            // Otherwise compare the keys and if the comparison returned:
            //       < 0: Set the new node as the left child.
            //      else: Set the new node as the right child.
            if (pIndexParentNode == &m_Sentinel)
            {
                m_pRoot = pNewNode;
            }
            else if (m_keyOperations.compare(pKey, &pIndexParentNode->key) < 0)
            {
                pIndexParentNode->pLeft = pNewNode;
            }
            else
            {
                pIndexParentNode->pRight = pNewNode;
            }

#if DEBUG
            // Number of hits
            m_cInsertionHits++;
#endif

            // Set the node that matches the key, and indicate it is new.
            *ppNode = pNewNode;
            return true;
        }

        //============================================================================
        // TreeMaximum:
        //
        //      Helper function to return the node with the maximum key in the given sub tree.
        //
        //      Return value:
        //          -Pointer to the right most node of the given sub tree.
        //============================================================================
        RedBlackNodeBase *
        TreeMaximum
        (
            RedBlackNodeBase *pIndexNode  // [In] Tree from which to find the node with the maximum key value. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // Locate the right most node.
            while (pIndexNode->pRight  != &m_Sentinel)
            {
                pIndexNode = pIndexNode->pRight;
            }

            return pIndexNode;
        }

        //============================================================================
        // TreeMinimum:
        //
        //      Helper function to return the node with the minimum key in the given sub tree.
        //
        //      Return value:
        //          -Pointer to the left most node of the given sub tree.
        //============================================================================
        RedBlackNodeBase *
        TreeMinimum
        (
            RedBlackNodeBase *pIndexNode  // [In] Tree from which to find the node with the minimum key value. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // Locate the left most node.
            while (pIndexNode->pLeft  != &m_Sentinel)
            {
                pIndexNode = pIndexNode->pLeft;
            }

            return pIndexNode;
        }

        //============================================================================
        // TreePredecessor:
        //
        //      Helper function to return the predecessor node of the given one.
        //
        //      Return value:
        //          -Pointer to the node that precedes the given one in the tree.
        //============================================================================
        RedBlackNodeBase *
        TreePredecessor
        (
            RedBlackNodeBase *pIndexNode  // [In] Node from which to find the preceeding node. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // If the left child is not empty, return the node with the maximum key in the left sub tree.
            if (pIndexNode->pLeft != &m_Sentinel)
            {
                return TreeMaximum(pIndexNode->pLeft);
            }

            RedBlackNodeBase *pParentNode = pIndexNode->pParent;

            // Iterate over the parents while the index is the left child of its parent.
            while (pParentNode != &m_Sentinel && pIndexNode == pParentNode->pLeft)
            {
                pIndexNode = pParentNode;
                pParentNode = pParentNode->pParent;
            }

            return pParentNode;
        }

        //============================================================================
        // TreeSuccessor:
        //
        //      Helper function to return the successor node of the given one.
        //
        //      Return value:
        //          -Pointer to the node that succeeds the given one in the tree.
        //============================================================================
        RedBlackNodeBase *
        TreeSuccessor
        (
            RedBlackNodeBase *pIndexNode  // [In] Node  from which to find the successor. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // If the right child is not empty, return the node with the minimum key in the right sub tree.
            if (pIndexNode->pRight != &m_Sentinel)
            {
                return TreeMinimum(pIndexNode->pRight);
            }

            RedBlackNodeBase *pParentNode = pIndexNode->pParent;

            // Iterate over the parents while the index is the right child of its parent.
            while (pParentNode != &m_Sentinel && pIndexNode == pParentNode->pRight)
            {
                pIndexNode = pParentNode;
                pParentNode = pParentNode->pParent;
            }

            return pParentNode;
        }

#if DEBUG
    public:
        //============================================================================
        // ClearTreeStats (DEBUG ONLY):
        //
        //      Function to reset tree statistics to 0.
        //============================================================================
        void ClearTreeStats()
        {
            m_cLeaves = 0;
            m_cMaxDepth = 0;
            m_cTotalDepth = 0;
            m_cSearchDepth = 0;
            m_cSearchHits = 0;
            m_cSearchMisses = 0;
            m_cDeletionDepth = 0;
            m_cDeletionHits = 0;
            m_cDeletionMisses = 0;
            m_cInsertionDepth = 0;
            m_cInsertionHits = 0;
            m_cInsertionMisses = 0;
        }

        //============================================================================
        // DumpTreeStats (DEBUG ONLY):
        //
        //      Function to dump tree statistics to the output window.
        //============================================================================
        void DumpTreeStats()
        {
            m_cLeaves = 0;
            m_cMaxDepth = 0;
            m_cTotalDepth = 0;

            ComputeStats(m_pRoot, 1);

            
            DebPrintf("Node Mem=%8u = %4u  X %5u  #Leaf=%4u", 
                sizeof(NodeType) * m_cNodes, sizeof(NodeType), m_cNodes, m_cLeaves);
  
            DebPrintf("  Dpth Max=%3u Avg=%5.1f ", m_cMaxDepth,
                        m_cNodes ? (double)(m_cTotalDepth / m_cNodes) : (double) 0.0);

            DebPrintf(" SRCH hits=%5u miss=%5u avg=%5.1f", m_cSearchHits, m_cSearchMisses, 
                (m_cSearchHits || m_cSearchMisses) ? ((double) m_cSearchDepth / (m_cSearchHits + m_cSearchMisses)) : (double) 0.0);

            DebPrintf(" DEL hits=%5u  miss=%5u avg=%5.1f", m_cDeletionHits, m_cDeletionMisses,
                (m_cDeletionHits || m_cDeletionMisses) ?  ((double) m_cDeletionDepth / (m_cDeletionHits + m_cDeletionMisses)) : (double) 0.0);

            DebPrintf("  INS hits=%5u miss=%5u avg=%5.1f\n", m_cInsertionHits,m_cInsertionMisses,
            (m_cInsertionHits || m_cInsertionMisses) ?  ((double) m_cInsertionDepth / (m_cInsertionHits + m_cInsertionMisses)) : (double) 0.0);
        }

        //============================================================================
        // VerifyTree (DEBUG ONLY):
        //
        //      Function that verifies the red black properties of a tree.
        //
        //      Return values:
        //          - True: the tree is a valid Red Black Tree
        //          - False: the tree is not a valid Red Black Tree
        //
        //============================================================================
        bool VerifyTree()
        {
            m_IsValidRedBlackTree = true;

            // Verify that the root is black
            if (m_pRoot->color != rbcBlack)
            {
                m_IsValidRedBlackTree = false;

                VSFAIL("Black root  property violated for Red-Black Tree");
                DebPrintf("Black root property violated for Red-Black Tree\n");
            }

            VerifySubTree(m_pRoot);

            return m_IsValidRedBlackTree;
        }

    protected:
        //============================================================================
        // ComputeStats (DEBUG ONLY):
        //
        //      Function that recursively computes the statistics for the tree.
        //============================================================================
        void
        ComputeStats
        (
            RedBlackNodeBase *pNode,  // Current sub tree
            unsigned long iDepth    // Current depth of the tree
        )
        {
            if (pNode != &m_Sentinel)
            {
                // Update maximum depth
                if (iDepth > m_cMaxDepth)
                {
                    m_cMaxDepth = iDepth;
                }

                // Update total depth (used for average)
                m_cTotalDepth += iDepth;

                // Update number of leaves
                if (pNode->pLeft == &m_Sentinel && pNode->pRight == &m_Sentinel)
                {
                    m_cLeaves++;
                }

                // Recurse into left and right subtrees.
                ComputeStats(pNode->pLeft, iDepth + 1);
                ComputeStats(pNode->pRight, iDepth + 1);
            }
        }

        //============================================================================
        // VerifySubTree (DEBUG ONLY):
        //
        //      Function that recursively verifies the red black properties of a tree.
        //
        //      Return value:
        //          - The black height of the given sub tree.
        //
        //============================================================================
        unsigned long
        VerifySubTree
        (
            RedBlackNodeBase *pNode        // Current sub tree.
        )
        {
            unsigned long cLeftBlackHeight = 0;
            unsigned long cRightBlackHeight = 0;

            if (pNode == &m_Sentinel)
            {
                return 0;
            }

            // Verify the BST property for the left child.
            if (pNode->pLeft != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(&pNode->pLeft->key, &pNode->key);

                if (iComp >= 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Binary Search property violated for Red-Black Tree");
                    DebPrintf("Binary Search property violated for Red-Black Tree\n");
                }
            }

            // Verify the BST property for the right child.
            if (pNode->pRight != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(&pNode->key, &pNode->pRight->key);

                if (iComp >= 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Binary Search property violated for Red-Black Tree");
                    DebPrintf("Binary Search property violated for Red-Black Tree\n");
                }
            }

            // Verify color is either Red or Black.
            switch (pNode->color)
            {
                case rbcRed:
                case rbcBlack:
                    break;

                default:
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Color property violated for Red-Black Tree");
                    DebPrintf("Color property violated for Red-Black Tree\n");
            }

            // Verify that red nodes have black children
            if (pNode->color == rbcRed)
            {
                if (pNode->pLeft->color != rbcBlack || pNode->pRight->color != rbcBlack)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Black children of red node property violated for Red-Black Tree");
                    DebPrintf("Black children of red node property violated for Red-Black Tree\n");
                }
            }

            // Verify that black heights are equal on both sides.
            cLeftBlackHeight = VerifySubTree(pNode->pLeft);
            cRightBlackHeight =VerifySubTree(pNode->pLeft);

            if (cLeftBlackHeight != cRightBlackHeight)
            {
                m_IsValidRedBlackTree = false;

                VSFAIL("Black height property violated for Red-Black Tree");
                DebPrintf("Black height property violated for Red-Black Tree\n");

                return 0;
            }

            // Return black height
            if (pNode->color == rbcBlack)
            {
                return cLeftBlackHeight + 1;
            }

            return cLeftBlackHeight;
        }
#endif

    protected:
        bool m_ShouldReleaseData: 1;     // Should the tree call the m_keyOperations.destroy and m_nodeOperations.destroy before the nodes are freed?

        NorlsAllocator *m_pNoReleaseAllocator;  // Memory allocator

        NodeType *m_pLastUnusedNode;        // Pointer to an allocated but unused Node.
                                            // This avoids the NRLS from growing even after nodes are deleted.

        RedBlackNodeBase *m_pRoot;          // Pointer to the root of the tree.
        RedBlackNodeBase m_Sentinel;        // Sentinel Node

        CriticalSectionType m_cs;   // Critical section to lock the tree.

        unsigned long m_cNodes;
        NodeOperations m_nodeOperations;
        KeyOperations m_keyOperations;

#if DEBUG
        // Red Black Tree Statistics
        bool m_IsValidRedBlackTree;
        bool m_IsRemove;                        // Are we currenlty removing a node?

        unsigned long m_cLeaves;
        unsigned long m_cMaxDepth;
        unsigned long m_cTotalDepth;
        unsigned long m_cSearchDepth;
        unsigned long m_cSearchHits;
        unsigned long m_cSearchMisses;
        unsigned long m_cDeletionDepth;
        unsigned long m_cDeletionHits;
        unsigned long m_cDeletionMisses;
        unsigned long m_cInsertionDepth;
        unsigned long m_cInsertionHits;
        unsigned long m_cInsertionMisses;
#endif
};

//============================================================================
// IntervalNodeBaseT:
//
//      Base class template for nodes that form part of an Interval Tree. You
//      should use this type as a base class for your own node.
//
//      Note: although the fileds for this class are public, they were not intended
//      to be modified by the user.
//============================================================================
template <typename KeyType, typename IntervalType>
struct IntervalNodeBaseT
{
    private:
        typedef IntervalNodeBaseT<KeyType, IntervalType> IntervalNodeBase;

    public:
        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        IntervalNodeBaseT() :
            pParent(NULL),
            pLeft (NULL),
            pRight(NULL),
            color(rbcBlack)
#if DEBUG
            ,pOwner(NULL)
#endif
        {
        }

    // Public members to manipulate tree structure.
    public:
        IntervalNodeBase *pParent;       // Pointer to the node's parent.
        IntervalNodeBase *pLeft;         //Pointer to the node's left child.
        IntervalNodeBase *pRight;        //Pointer to the node's right child.

        RedBlackColor color;                // Color of the node

        KeyType key;                        // Index key
        IntervalType maxHighInterval; // Maximum high interval end within this subtree.

#if DEBUG
        void *pOwner;           // Pointer to the tree that owns this node.
#endif
};

// 


#define DEBUG_VERIFY_INTERVALTREE DEBUG

//============================================================================
// IntervalTreeT:
//
//      Template implementation of an Interval Tree implemented on top of a
//      Red-Black Tree.
//
//  Template Parameters:
//
//      KeyType: Specifies the type of the interval to use as key for the tree.
//
//      PointType: Specifies the type of a single point in the interval.
//
//      NodeType: [Optional] Specifies the type of the nodes that the tree will
//                      hold. If specified, this type must privately derive from
//                      RedBlackNodeBaseT. If it is not specified, a simple
//                      implementation of RedBlackNodeBaseT specialized for
//                      KeyType, without any additional data, will be used.
//
//      KeyOperations: Specifies an inline class to manipulate the
//                             keys of the tree. The type should define the
//                             following methods, which should adequately
//                             handle the requirements for the data:
//
//           int compare(const <KeyType> *key1, const <KeyType> *key2)
//                  - performs a comparison between key1 and key2
//                  - returns: 0 if key1 = key2
//                                 <0 if key1 < key2
//                                 >0 if key1 > key2
//
//           bool intersect(const <KeyType> *key1, const <KeyType> *key2)
//                  - determines if key1 intersects key2
//                  - returns: true if the keys intersect; ot
//                                 false if they don't intersect.
//
//           void destroy (<KeyType> *key)
//                  - performs any required cleanup for key.
//
//           void copy (<KeyType> *destkey, const <KeyType> *srckey)
//                  - copies the contents of srckey into destkey.
//
//      PointOperations: Specifies an inline class to manipulate the
//                             individual ends of an interval. If it is not specified, a simple
//                             implementation for intrinsic types will be used. The
//                             type should define the following methods,
//                             which should adequately handle the requirements
//                             for the data:
//
//           int compare(const <PointType> *point1, const <PointType> *point2)
//                  - performs a comparison between point1 and key2
//                  - returns: 0 if point1 = point2
//                                 <0 if point1 < point2
//                                 >0 if point1 > point2
//
//           void destroy (<PointType> *point)
//                  - performs any required cleanup for point.
//
//           void copy (<PointType> *destpoint, const <PointType> *srcpoint)
//                  - copies the contents of srcpoint into destpoint.
//
//
//      KeyToPointOperations: Specifies an inline class to manipulate the
//                             intervals of the tree.  The type should define the
//                             following methods, which should adequately
//                             handle the requirements for the data:
//
//           int compareHigh(const <KeyType> *interval, const <PointType> *point)
//                  - performs a comparison between the high end of the interval and point
//                  - returns: 0 if high end = point
//                                 <0 if high end < point
//                                 >0 if high end > point
//
//           int compareLow(const <KeyType> *interval, const <PointType> *point)
//                  - performs a comparison between the low end of the interval and point
//                  - returns: 0 if low end = point
//                                 <0 if low end < point
//                                 >0 if low end > point
//
//           void copyHigh (const <KeyType> *interval, <PointType> *point)
//                  - copies the contents of the high end of the interval into point.
//
//
//      NodeOperations: [Optional] Specifies an inline class to manipulate
//                               additional information in the node. If it is not
//                               specified, a default implementation for nodes
//                               without any additional data will be used. The
//                               type should define the following methods,
//                               which should adequately handle the requirements
//                               for the data:
//
//              void destroy (NodeType *node)
//                  - performs any required cleanup on the additional information on the node.
//
//              void copy (NodeType *destnode, const NodeType *srcnode)
//                  - copies the additional contents of srcnode into destnode.
//
//       CriticalSectionType: [Optional] Specify a critical section to use for
//                                     synchornization. Must be either
//                                     CComAutoCriticalSection or CComFakeCriticalSection
//============================================================================
template
    <
        typename KeyType,
        typename PointType,
        typename KeyOperations,
        typename PointOperations,
        typename KeyToPointOperations,
        typename NodeType  = IntervalNodeBaseT<KeyType, PointType>,
        typename NodeOperations = EmptyNodeOperationsT<NodeType>,
        typename CriticalSectionType = CComFakeCriticalSection
    >
class IntervalTreeT
{
    public:
        typedef NodeType IntervalNode;

    protected:
        typedef IntervalNodeBaseT<KeyType, PointType> IntervalNodeBase;
        typedef IntervalTreeT<KeyType, PointType, KeyOperations, PointOperations, KeyToPointOperations, NodeType, NodeOperations, CriticalSectionType> IntervalTree;

    public:
        //============================================================================
        //     Nested class that defines an in-order iterator for Red Black Trees.
        //============================================================================
        class Iterator
        {
            public:
                //============================================================================
                // Constructor: Initialize the iterator for the given tree.
                //============================================================================
                Iterator
                (
                    IntervalTree *pIntervalTree  // [in] Tree to get the iterator for. It shouldn't be null
                ) :
                    m_NodeStack(pIntervalTree->m_pNoReleaseAllocator->GetCompiler()),
                    m_pTree(pIntervalTree)
                {
                    VSASSERT(pIntervalTree != NULL, "Invalid Null Pointer");
                    VSASSERT(pIntervalTree->m_pNoReleaseAllocator != NULL, "Invalid Null Pointer");

                    Reset();
                }

                //============================================================================
                // Reset the iterator to the first element.
                //============================================================================
                void
                Reset
                (
                )
                {
                    // Clear the stack.
                    m_NodeStack.Reset();

                    // Set the initial stack.
                    AddToStack(m_pTree->m_pRoot);
                }

                //============================================================================
                // Get the next 'in-order' element from the tree
                //
                //    Return values:
                //        A pointer to the next node in the tree, or
                //        NULL if  the iterator has reached the end of the tree.
                //============================================================================
                IntervalNode *
                Next
                (
                )
                {
                    // Check if there are any elements left.
                    if (m_NodeStack.Empty())
                    {
                        return NULL;
                    }

                    // Retrieve the current element.
                    IntervalNodeBase *pCurrentNode = m_NodeStack.Top();
                    m_NodeStack.Pop();

                    // Add the right subtree to the stack.
                    AddToStack(pCurrentNode->pRight);

#if DEBUG_VERIFY_INTERVALTREE
                    if (!m_NodeStack.Empty())
                    {
                        VSASSERT(m_pTree->TreeSuccessor(pCurrentNode) == m_NodeStack.Top(), "Invalid Successor");
                    }
#endif

                    // Return the current node
                    return static_cast<IntervalNode *>(pCurrentNode);
                }

            private:

                //============================================================================
                // AddToStack:
                //
                //      Adds the left branch of the given node to the stack.
                //============================================================================
                void
                AddToStack
                (
                    IntervalNodeBase *pIndexNode    // [in] Pointer to subtree. It shouldn't be NULL>
                )
                {
                    VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

                    // Iterate over all the left children.
                    while (pIndexNode != &m_pTree->m_Sentinel)
                    {
                        m_NodeStack.Push(pIndexNode);

                        pIndexNode = pIndexNode->pLeft;
                    }
                }

            private:
                Stack <IntervalNodeBase *> m_NodeStack;     // Stack for in-order walk.
                IntervalTree *m_pTree;   // Current tree.
        };

    friend Iterator;

    public:
        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        IntervalTreeT
        (
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        ) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(NULL),
            m_pRoot(&m_Sentinel),
            m_keyOperations(),
            m_pointOperations(),
            m_keyToPointOperations()
            m_nodeOperations()
        {
            // Clear the contents of the tree
            Clear();
        }

        //============================================================================
        // Constructor: Initialize default values
        //============================================================================
        IntervalTreeT
        (
            NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        ) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(pNoReleaseAllocator),
            m_pRoot(&m_Sentinel),
            m_keyOperations(),
            m_pointOperations(),
            m_keyToPointOperations(),
            m_nodeOperations()
        {
            // Clear the contents of the tree
            Clear();
        }

        IntervalTreeT
        (
            NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            bool ShouldReleaseData, // [in, optional] should the data be released before releasing the node?
            const KeyOperations & keyOperations,
            const PointOperations & pointOperations,
            const KeyToPointOperations & keyToPointOperations,
            const NodeOperations & nodeToPointOperations
        ) :
            m_ShouldReleaseData(ShouldReleaseData),
            m_pNoReleaseAllocator(pNoReleaseAllocator),
            m_pRoot(&m_Sentinel),
            m_keyOperations(keyOperations),
            m_pointOperations(pointOperations),
            m_keyToPointOperations(keyToPointOperations),
            m_nodeToPointOperations(nodeToPointOperations)
        {
            // Clear the contents of the tree
            Clear();
        }
        //============================================================================
        // Destructor: Clear the tree
        //============================================================================
        ~IntervalTreeT()
        {
            // Clear the contents of the tree
            Clear();
        }

        //============================================================================
        // Init: Initializes the red black tree.
        //============================================================================
        void
        Init
        (
            NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
            bool ShouldReleaseData = false      // [in, optional] should the data be released before releasing the node?
        )
        {
            VSASSERT(m_pNoReleaseAllocator == NULL, "Red Black Tree has already been initialized");

            // The tree can only be initialized once.
            if (m_pNoReleaseAllocator == NULL)
            {
                m_pNoReleaseAllocator = pNoReleaseAllocator;
                m_ShouldReleaseData = ShouldReleaseData;

                // Clear the contents of the tree
                Clear();
            }
        }

        //============================================================================
        // Clear: Clears the contents of the tree.
        //============================================================================
        void
        Clear()
        {
            // If the flag is set, recursively walk the tree and call the behavior classes' destroy method.
            if (m_ShouldReleaseData)
            {
                ClearSubTree(m_pRoot);
            }

            m_pRoot = &m_Sentinel;

#if DEBUG
            m_IsRemove = false;
            ClearTreeStats();
#endif
        }

        //============================================================================
        // Find:
        //
        //      Searches the tree for a node that matches the specified key.
        //
        //      Return values:
        //          -True, if a node matching the key was found; or
        //          -False, if such a node couldn't be found.
        //============================================================================
        bool
        Find
        (
            const KeyType *pKey,    // [in] Key of the  node to find. It shouldn't be NULL.
            NodeType **ppNode = NULL  // [optional, out] If not NULL, on return it will contain the pointer to the node matching the given key or NULL if it wasn't found.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pIndexNode = m_pRoot;

            while (pIndexNode != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(pKey, &pIndexNode->key);

#if DEBUG
                if (m_IsRemove)
                {
                     // Total depth of all combined deletes to obtain the average.
                    m_cDeletionDepth++;
                }
                else
                {
                     // Total depth of all combined searches to obtain the average.
                    m_cSearchDepth++;
                }
#endif

                if (iComp == 0)
                {
#if DEBUG
                    if (m_IsRemove)
                    {
                         // Number of deletion hits
                        m_cDeletionHits++;
                    }
                    else
                    {
                        // Number of hits
                        m_cSearchHits++;
                    }
#endif

                    if (ppNode)
                    {
                        *ppNode = static_cast<NodeType *>(pIndexNode);
                    }

                    return true;
                }
                else if (iComp < 0)
                {
                    pIndexNode = pIndexNode->pLeft;
                }
                else
                {
                    pIndexNode = pIndexNode->pRight;
                }
            }

#if DEBUG
            if (m_IsRemove)
            {
                 // Number of deletion hits
                m_cDeletionMisses++;
            }
            else
            {
                // Number of misses
                m_cSearchMisses++;
            }
#endif

            if (ppNode)
            {
                *ppNode = NULL;
            }

            return false;
        }

        //============================================================================
        // Insert:
        //
        //      Function to insert a new node with the given key into the appropriate position in the search tree.
        //
        //      Return value:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        Insert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            NodeType **ppNode  = NULL // [optional, out] If not null, on return it will contain the pointer to the node matching the given key. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pReturnedNode = NULL;

            // Insert the new node
            bool IsNewNode = RBInsert(pKey, &pReturnedNode);

            // Convert the node pointer to the appropriate class
            if (ppNode)
            {
                *ppNode = static_cast<NodeType *> (pReturnedNode);
            }

#if DEBUG_VERIFY_INTERVALTREE
            //      Make sure properties still hold
            VerifyTree();
#endif

            // Return whether it is a new node or not.
            return IsNewNode;
        }

        //============================================================================
        // Intersect:
        //
        //      Searches the tree for a node that intersects the specified interval.
        //
        //      Return values:
        //          -True, if a node intersecting the interval was found; or
        //          -False, if such a node couldn't be found.
        //============================================================================
        bool
        Intersect
        (
            const KeyType *pKey,    // [in] Interval to try to intersect. It shouldn't be NULL.
            NodeType **ppNode = NULL  // [optional, out] If not NULL, on return it will contain the pointer to the node matching the given key or NULL if it wasn't found.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pIndexNode = m_pRoot;

            while (pIndexNode != &m_Sentinel)
            {
                bool IsIntersect = m_keyOperations.intersect(pKey, &pIndexNode->key);

#if DEBUG
                 // Total depth of all combined searches to obtain the average.
                m_cSearchDepth++;
#endif

                if (IsIntersect)
                {
#if DEBUG
                    // Number of hits
                    m_cSearchHits++;
#endif

                    if (ppNode)
                    {
                        *ppNode = static_cast<NodeType *>(pIndexNode);
                    }

                    return true;
                }

                if (pIndexNode->pLeft != &m_Sentinel &&
                    m_keyToPointOperations.compareLow(pKey, &pIndexNode->pLeft->maxHighInterval)  < 0)
                {
                    pIndexNode = pIndexNode->pLeft;
                }
                else
                {
                    pIndexNode = pIndexNode->pRight;
                }
            }

#if DEBUG
            // Number of misses
            m_cSearchMisses++;
#endif

            if (ppNode)
            {
                *ppNode = NULL;
            }

            return false;
        }


        //============================================================================
        // Lock:
        //
        //      Function to lock the tree by using a critical section. Use
        //      UnLock to release the trees.
        //
        //      Make sure to specifiy the right template argument for
        //      the critical section.
        //============================================================================
        void
        Lock
        (
        )
        {
            m_cs.Lock();
        }

        //============================================================================
        // Remove:
        //
        //      Function to remove the node that matches the given key from the tree.
        //
        //      Return value:
        //          -True, if the node was removed from the tree; or
        //          -False, if the node doesn't exist in tree.
        //============================================================================
        bool
        Remove
        (
            const KeyType *pKey    // [in] Key of the node to delete. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pNodeToDelete = NULL;

#if DEBUG
            m_IsRemove = true;
#endif

            if (Find(pKey, &pNodeToDelete))
            {
                RBDelete(pNodeToDelete);

#if DEBUG_VERIFY_INTERVALTREE
                VerifyTree();
                m_IsRemove = false;
#endif
                return true;
            }

            return false;
        }

        //============================================================================
        // Remove:
        //
        //      Function to remove the given node from the tree.
        //
        //      Return value:
        //          -True:  the node was removed from the tree.
        //============================================================================
        bool
        Remove
        (
            NodeType *pNodeToDelete        // [in] Node to delete. It shouldn't be NULL.
        )
        {
            VSASSERT(pNodeToDelete != NULL, "Invalid Null Pointer");

#if DEBUG
            VSASSERT(pNodeToDelete->pOwner == this, "The node doesn't belong to the tree");
#endif

            RBDelete(pNodeToDelete);

#if DEBUG_VERIFY_INTERVALTREE
            VerifyTree();
#endif
            return true;
        }

        //============================================================================
        // UnLock:
        //
        //      Function to unlock the tree. Use Lock to lock it..
        //
        //      Make sure to specifiy the right template argument for
        //      the critical section.
        //
        //============================================================================
        void
        UnLock
        (
        )
        {
            m_cs.Unlock();
        }

        /// <summary>
        /// Gets node following the given one in the ordered traversal order.
        /// </summary>
        /// <param name="pCurrentNode">Current node.</param>
        /// <returns>Next node if NULL if the current node is last one.</returns>
        NodeType*
        GetSuccessorNode
        (
            NodeType *pCurrentNode
        )
        {
            VSASSERT(pCurrentNode, "pCurrentNode should not be NULL.");

            NodeType *pResult = TreeSuccessor(pCurrentNode);
            return pResult == &m_Sentinel ? NULL : static_cast<NodeType*>(pResult);
        }

    protected:
        //============================================================================
        // ClearSubTree:
        //
        //      Helper function that recursively calls the destroy method of the policy classes for each
        //      node in the tree.
        //
        //============================================================================
        void
        ClearSubTree
        (
            IntervalNodeBase *pNode        // [in] Current sub tree. It shouldn't be NULL.
        )
        {
            VSASSERT (pNode != NULL, "Invalid NULL pointer");

            if (pNode != &m_Sentinel)
            {
                // Call the destroy methods on the behavior classes.
                m_keyOperations.destroy(&pNode->key);
                m_pointOperations.destroy(&pNode->maxHighInterval);
                m_nodeOperations.destroy(static_cast<NodeType *> (pNode));

                // Recursively release the child trees.
                ClearSubTree(pNode->pLeft);
                ClearSubTree(pNode->pRight);
            }
        }

        //============================================================================
        // FixMaxPoints:
        //
        //      Helper function that fixes the maximum points, starting at the given node and
        //      walking up the parents.
        //============================================================================
        void
        FixMaxPoints
        (
            IntervalNodeBase *pNode        // [in] Current sub tree. It shouldn't be NULL.
        )
        {
            VSASSERT (pNode != NULL, "Invalid NULL pointer");

            while (pNode != &m_Sentinel)
            {
                UpdateMaxPoint(pNode);

                pNode = pNode->pParent;
            }
        }

        //============================================================================
        // RBDelete:
        //
        //      Helper function to delete the given node from the tree and preserve the red black balance properties afterwards.
        //============================================================================
        void
        RBDelete
        (
            IntervalNodeBase *pNodeToDelete  // [in] Node to remove form the tree.
        )
        {
            VSASSERT(pNodeToDelete != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pRealNodeToDelete = NULL;
            IntervalNodeBase *pChildNode = NULL;

            // Locate the real node that will be deleted. If the pNodeToDelete has at most one child,
            // then use that one; otherwise, get the node that's the successor to pNodeToDelete.
            if (pNodeToDelete->pLeft == &m_Sentinel || pNodeToDelete->pRight == &m_Sentinel)
            {
                pRealNodeToDelete = pNodeToDelete;
            }
            else
            {
                pRealNodeToDelete = TreeSuccessor(pNodeToDelete);
            }

            // Save a pointer to the only child of pRealNodeToDelete
            if (pRealNodeToDelete->pLeft != &m_Sentinel)
            {
                pChildNode = pRealNodeToDelete->pLeft;
            }
            else
            {
                pChildNode = pRealNodeToDelete->pRight;
            }

            // Set the child's parent to its grandparent.
            pChildNode->pParent = pRealNodeToDelete->pParent;

            // Link the child node direclty to its grandparent.
            if (pRealNodeToDelete->pParent == &m_Sentinel)
            {
                m_pRoot = pChildNode;
            }
            else if (pRealNodeToDelete == pRealNodeToDelete->pParent->pLeft)
            {
                pRealNodeToDelete->pParent->pLeft = pChildNode;
            }
            else
            {
                pRealNodeToDelete->pParent->pRight = pChildNode;
            }

            // If pNodeToDelete is not the real node to delete, then copy the data over.
            if (pRealNodeToDelete != pNodeToDelete)
            {
                // If the flag is set, then release the data.
                if (m_ShouldReleaseData)
                {
                    m_keyOperations.destroy(&pNodeToDelete->key);
                    m_pointOperations.destroy(&pNodeToDelete->maxHighInterval);
                    m_nodeOperations.destroy(static_cast<NodeType *>(pNodeToDelete));
                }

                // Copy the data to the new location.
                m_keyOperations.copy(&pNodeToDelete->key, &pRealNodeToDelete->key);
                m_pointOperations.copy(&pNodeToDelete->maxHighInterval, &pRealNodeToDelete->maxHighInterval);
                m_nodeOperations.copy(static_cast<NodeType *>(pNodeToDelete), static_cast<NodeType *>(pRealNodeToDelete));
            }

            // Release the data
            if (m_ShouldReleaseData)
            {
                m_keyOperations.destroy(&pRealNodeToDelete->key);
                m_pointOperations.destroy(&pRealNodeToDelete->maxHighInterval);
                m_nodeOperations.destroy(static_cast<NodeType *>(pRealNodeToDelete));
            }

            // Update the maximum points.
            FixMaxPoints(pRealNodeToDelete->pParent);

            // Restore the Red Black Properties.
            if (pRealNodeToDelete->color == rbcBlack)
            {
                RBDeleteFixup(pChildNode);
            }
        }

        //============================================================================
        // RBDeleteFixup:
        //
        //      Helper function to restore the red black properties after a deletion.
        //============================================================================
        void
        RBDeleteFixup
        (
            IntervalNodeBase *pIndexNode  // [in] Node to fix properties for
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");


            // Make sure the Red Black Properties are maintained.
            while (pIndexNode != m_pRoot && pIndexNode->color == rbcBlack)
            {
                IntervalNodeBase *pSiblingNode = NULL;

                if (pIndexNode == pIndexNode->pParent->pLeft)
                {
                    // ---- Left child case scenario ----
                    pSiblingNode = pIndexNode->pParent->pRight;

                    if (pSiblingNode->color == rbcRed)
                    {
                        // Case 1:
                        pSiblingNode->color = rbcBlack;
                        pIndexNode->pParent->color = rbcRed;

                        RBRotateLeft(pIndexNode->pParent);

                        pSiblingNode = pIndexNode->pParent->pRight;
                    }

                    if (pSiblingNode->pLeft->color == rbcBlack && pSiblingNode->pRight->color == rbcBlack)
                    {
                        // Case 2:
                        pSiblingNode->color = rbcRed;
                        pIndexNode = pIndexNode->pParent;
                    }
                    else
                    {
                        if (pSiblingNode->pRight->color == rbcBlack)
                        {
                            //Case 3:
                            pSiblingNode->pLeft->color = rbcBlack;
                            pSiblingNode->color = rbcRed;

                            RBRotateRight(pSiblingNode);

                            pSiblingNode = pIndexNode->pParent->pRight;
                        }

                        // Case 4:
                        pSiblingNode->color = pIndexNode->pParent->color;
                        pIndexNode->pParent->color = rbcBlack;
                        pSiblingNode->pRight->color = rbcBlack;

                        RBRotateLeft(pIndexNode->pParent);

                        pIndexNode = m_pRoot;
                    }
                }
                else
                {
                    // ---- Right child case scenario ----

                    pSiblingNode = pIndexNode->pParent->pLeft;

                    if (pSiblingNode->color == rbcRed)
                    {
                        // Case 1:
                        pSiblingNode->color = rbcBlack;
                        pIndexNode->pParent->color = rbcRed;

                        RBRotateRight(pIndexNode->pParent);

                        pSiblingNode = pIndexNode->pParent->pLeft;
                    }

                    if (pSiblingNode->pRight->color == rbcBlack && pSiblingNode->pLeft->color == rbcBlack)
                    {
                        // Case 2:
                        pSiblingNode->color = rbcRed;
                        pIndexNode = pIndexNode->pParent;
                    }
                    else
                    {
                        if (pSiblingNode->pLeft->color == rbcBlack)
                        {
                            //Case 3:
                            pSiblingNode->pRight->color = rbcBlack;
                            pSiblingNode->color = rbcRed;

                            RBRotateLeft(pSiblingNode);

                            pSiblingNode = pIndexNode->pParent->pLeft;
                        }

                        // Case 4:
                        pSiblingNode->color = pIndexNode->pParent->color;
                        pIndexNode->pParent->color = rbcBlack;
                        pSiblingNode->pLeft->color = rbcBlack;

                        RBRotateRight(pIndexNode->pParent);

                        pIndexNode = m_pRoot;
                    }
                }
            }
            pIndexNode->color = rbcBlack;
        }

        //============================================================================
        // RBInsert:
        //
        //      Helper function to insert a new node with the given key into the appropriate position in the search tree
        //      and preserve the red black balance properties afterwards.
        //
        //      Return value:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        RBInsert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            IntervalNodeBase **ppNode  // [out] On return it will contain the pointer to the node matching the given key. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");
            VSASSERT(ppNode != NULL, "Invalid Null Pointer");

            // Try to insert a new node with the given key into the tree.
            if (!TreeInsert(pKey, ppNode))
            {
                return false;
            }

            // Set the IndexNode to the newly created node and color it red.
            IntervalNodeBase *pIndexNode = *ppNode;
            pIndexNode->color = rbcRed;

            // Make sure the Red Black Tree properties are maintained.
            while (pIndexNode != m_pRoot && pIndexNode->pParent->color == rbcRed)
            {
                if (pIndexNode->pParent == pIndexNode->pParent->pParent->pLeft)
                {
                    IntervalNodeBase *pUncleNode = pIndexNode->pParent->pParent->pRight;

                    if (pUncleNode->color == rbcRed)
                    {
                        // Case 1:
                        pIndexNode->pParent->color = rbcBlack;
                        pUncleNode->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        pIndexNode = pIndexNode->pParent->pParent;
                    }
                    else
                    {
                        if (pIndexNode == pIndexNode->pParent->pRight)
                        {
                            // Case 2:
                            pIndexNode = pIndexNode->pParent;

                            RBRotateLeft(pIndexNode);
                        }

                        // Case 3:
                        pIndexNode->pParent->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        RBRotateRight(pIndexNode->pParent->pParent);
                    }
                }
                else
                {
                    IntervalNodeBase *pUncleNode = pIndexNode->pParent->pParent->pLeft;

                    if (pUncleNode->color == rbcRed)
                    {
                        // Case 1:
                        pIndexNode->pParent->color = rbcBlack;
                        pUncleNode->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        pIndexNode = pIndexNode->pParent->pParent;
                    }
                    else
                    {
                        if (pIndexNode == pIndexNode->pParent->pLeft)
                        {
                            // Case 2:
                            pIndexNode = pIndexNode->pParent;

                            RBRotateRight(pIndexNode);
                        }

                        // Case 3:
                        pIndexNode->pParent->color = rbcBlack;
                        pIndexNode->pParent->pParent->color = rbcRed;

                        RBRotateLeft(pIndexNode->pParent->pParent);
                    }
                }
            }

            m_pRoot->color = rbcBlack;

            return true;
        }

        //============================================================================
        // RBRotateLeft:
        //
        //      Helper function that performs a left rotation of the tree at the given pivot node.
        //============================================================================
        void
        RBRotateLeft
        (
            IntervalNodeBase *pPivotNode    // [in] Node at which to perform the left rotation. It shouldn't be null.
        )
        {
            VSASSERT(pPivotNode != NULL, "Invalid Null Pointer");

            // Get the right child of the pivotnode.
            IntervalNodeBase *pChildTree = pPivotNode->pRight;

            // Turn the child's left subtree into the pivot's right subtree, and update the parent if necessary.
            pPivotNode->pRight = pChildTree->pLeft;
            if (pChildTree->pLeft != &m_Sentinel)
            {
                pChildTree->pLeft->pParent = pPivotNode;
            }

            // Link the pivot's parent to the child tree.
            pChildTree->pParent = pPivotNode->pParent;

            // If the parent is NIL, then set as the child tree as the new root.
            // Else, if the pivot is the left child of its parent, set the child tree as its new left child.
            // Otherwise set the child tree as the new right child of the pivot's parent.
            if (pPivotNode->pParent == &m_Sentinel)
            {
                m_pRoot = pChildTree;
            }
            else if (pPivotNode == pPivotNode->pParent->pLeft)
            {
                pPivotNode->pParent->pLeft = pChildTree;
            }
            else
            {
                pPivotNode->pParent->pRight = pChildTree;
            }

            // Set the pivot node as the new left child of the child tree.
            pChildTree->pLeft = pPivotNode;
            pPivotNode->pParent = pChildTree;

            // Update the maximum points.
            UpdateMaxPoint(pPivotNode);
            UpdateMaxPoint(pChildTree);
        }

        //============================================================================
        // RightRotate:
        //
        //      Helper function that performs a right rotation of the tree at the given pivot node.
        //============================================================================
        void
        RBRotateRight
        (
            IntervalNodeBase *pPivotNode    // [in] Node at which to perform the right rotation. It shouldn't be null.
        )
        {
            VSASSERT(pPivotNode != NULL, "Invalid Null Pointer");

            // Get the left child of the pivotnode.
            IntervalNodeBase *pChildTree = pPivotNode->pLeft;

            // Turn the child's right subtree into the pivot's left subtree, and update the parent if necessary.
            pPivotNode->pLeft = pChildTree->pRight;
            if (pChildTree->pRight != &m_Sentinel)
            {
                pChildTree->pRight->pParent = pPivotNode;
            }

            // Link the pivot's parent to the child tree.
            pChildTree->pParent = pPivotNode->pParent;

            // If the parent is NIL, then set as the child tree as the new root.
            // Else, if the pivot is the left child of its parent, set the child tree as its new left child.
            // Otherwise set the child tree as the new right child of the pivot's parent.
            if (pPivotNode->pParent == &m_Sentinel)
            {
                m_pRoot = pChildTree;
            }
            else if (pPivotNode == pPivotNode->pParent->pLeft)
            {
                pPivotNode->pParent->pLeft = pChildTree;
            }
            else
            {
                pPivotNode->pParent->pRight = pChildTree;
            }

            // Set the pivot node as the new right child of the child tree.
            pChildTree->pRight = pPivotNode;
            pPivotNode->pParent = pChildTree;

            // Update the maximum points.
            UpdateMaxPoint(pPivotNode);
            UpdateMaxPoint(pChildTree);
        }

        //============================================================================
        // TreeInsert:
        //
        //      Helper function to insert the pNewNode node into the appropriate position in the search tree.
        //
        //      Return values:
        //          -True, if the node was inserted; or
        //          -False, if the node already existed in the tree.
        //============================================================================
        bool
        TreeInsert
        (
            const KeyType *pKey,    // [in] Key of the new node to insert. It shouldn't be NULL.
            IntervalNodeBase **ppNode  // [out] On return it will contain the pointer to the node matching the given key. It shouldn't be NULL.
        )
        {
            VSASSERT(pKey != NULL, "Invalid Null Pointer");

            IntervalNodeBase *pIndexParentNode = &m_Sentinel;      // Path index's parent.
            IntervalNodeBase *pIndexNode = m_pRoot;               // Path index.

            while (pIndexNode != &m_Sentinel)
            {
                pIndexParentNode = pIndexNode;

                // If the high end of the new key to insert is greater than the max high end.
                // update the field.
                if (m_keyToPointOperations.compareHigh(pKey, &pIndexNode->maxHighInterval) > 0)
                {
                    m_keyToPointOperations.copyHigh(pKey, &pIndexNode->maxHighInterval);
                }

#if DEBUG
                // Total depth of all combined insertions to obtain the average.
                m_cInsertionDepth++;
#endif

               // Compare keys
                int ret = m_keyOperations.compare(pKey, &pIndexNode->key);

                // If the comparison returned:
                //      = 0: Exit because we don't support multiple nodes with the same key.
                //      < 0: Move the index to the left child.
                //      > 0: Move the index to the right child.
                if (ret == 0)
                {
#if DEBUG
                    // Number of misses
                    m_cInsertionMisses++;
#endif
                    // Set the node that matches the key, and indicate it already existed.
                    *ppNode = pIndexNode;
                    return false;
                }
                else if (ret < 0)
                {
                    pIndexNode = pIndexNode->pLeft;
                }
                else
                {
                    pIndexNode = pIndexNode->pRight;
                }
            }

            // Create the new node
            IntervalNodeBase *pNewNode = new (*m_pNoReleaseAllocator) NodeType();

            // Copy the key into the new node.
            m_keyOperations.copy(&pNewNode->key, pKey);
            m_keyToPointOperations.copyHigh(pKey, &pNewNode->maxHighInterval);

            pNewNode->pParent = pIndexParentNode;
            pNewNode->pLeft = &m_Sentinel;
            pNewNode->pRight = &m_Sentinel;

#if DEBUG
            pNewNode->pOwner = this;
#endif

            // If the parent's index is NIL this is the first node, so set the root of the tree.
            // Otherwise compare the keys and if the comparison returned:
            //       < 0: Set the new node as the left child.
            //      else: Set the new node as the right child.
            if (pIndexParentNode == &m_Sentinel)
            {
                m_pRoot = pNewNode;
            }
            else if (m_keyOperations.compare(pKey, &pIndexParentNode->key) < 0)
            {
                pIndexParentNode->pLeft = pNewNode;
            }
            else
            {
                pIndexParentNode->pRight = pNewNode;
            }

#if DEBUG
            // Number of hits
            m_cInsertionHits++;
#endif

            // Set the node that matches the key, and indicate it is new.
            *ppNode = pNewNode;
            return true;
        }

        //============================================================================
        // TreeMaximum:
        //
        //      Helper function to return the node with the maximum key in the given sub tree.
        //
        //      Return value:
        //          -Pointer to the right most node of the given sub tree.
        //============================================================================
        IntervalNodeBase *
        TreeMaximum
        (
            IntervalNodeBase *pIndexNode  // [In] Tree from which to find the node with the maximum key value. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // Locate the right most node.
            while (pIndexNode->pRight  != &m_Sentinel)
            {
                pIndexNode = pIndexNode->pRight;
            }

            return pIndexNode;
        }

        //============================================================================
        // TreeMinimum:
        //
        //      Helper function to return the node with the minimum key in the given sub tree.
        //
        //      Return value:
        //          -Pointer to the left most node of the given sub tree.
        //============================================================================
        IntervalNodeBase *
        TreeMinimum
        (
            IntervalNodeBase *pIndexNode  // [In] Tree from which to find the node with the minimum key value. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // Locate the left most node.
            while (pIndexNode->pLeft  != &m_Sentinel)
            {
                pIndexNode = pIndexNode->pLeft;
            }

            return pIndexNode;
        }

        //============================================================================
        // TreePredecessor:
        //
        //      Helper function to return the predecessor node of the given one.
        //
        //      Return value:
        //          -Pointer to the node that precedes the given one in the tree.
        //============================================================================
        IntervalNodeBase *
        TreePredecessor
        (
            IntervalNodeBase *pIndexNode  // [In] Node from which to find the preceeding node. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // If the left child is not empty, return the node with the maximum key in the left sub tree.
            if (pIndexNode->pLeft != &m_Sentinel)
            {
                return TreeMaximum(pIndexNode->pLeft);
            }

            IntervalNodeBase *pParentNode = pIndexNode->pParent;

            // Iterate over the parents while the index is the left child of its parent.
            while (pParentNode != &m_Sentinel && pIndexNode == pParentNode->pLeft)
            {
                pIndexNode = pParentNode;
                pParentNode = pParentNode->pParent;
            }

            return pParentNode;
        }

        //============================================================================
        // TreeSuccessor:
        //
        //      Helper function to return the successor node of the given one.
        //
        //      Return value:
        //          -Pointer to the node that succeeds the given one in the tree.
        //============================================================================
        IntervalNodeBase *
        TreeSuccessor
        (
            IntervalNodeBase *pIndexNode  // [In] Node from which to find the successor node. It shouldn't be NULL.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // If the right child is not empty, return the node with the minimum key in the right sub tree.
            if (pIndexNode->pRight != &m_Sentinel)
            {
                return TreeMinimum(pIndexNode->pRight);
            }

            IntervalNodeBase *pParentNode = pIndexNode->pParent;

            // Iterate over the parents while the index is the right child of its parent.
            while (pParentNode != &m_Sentinel && pIndexNode == pParentNode->pRight)
            {
                pIndexNode = pParentNode;
                pParentNode = pParentNode->pParent;
            }

            return pParentNode;
        }

        //============================================================================
        // UpdateMaxPoint:
        //
        //      Helper function to update the max high interval end of the given node.
        //============================================================================
        void
        UpdateMaxPoint
        (
            IntervalNodeBase *pIndexNode  // [In] Node in which to update the max high interval end.
        )
        {
            VSASSERT(pIndexNode != NULL, "Invalid Null Pointer");

            // Set the maximum to the current key's high end.
            m_keyToPointOperations.copyHigh(&pIndexNode->key, &pIndexNode->maxHighInterval);

            // Set the maximum to the max of the current or the left child's max.
            if (pIndexNode->pLeft != &m_Sentinel &&
                m_pointOperations.compare(
                    &pIndexNode->maxHighInterval,
                    &pIndexNode->pLeft->maxHighInterval) < 0)
            {
                m_pointOperations.copy(&pIndexNode->maxHighInterval, &pIndexNode->pLeft->maxHighInterval);
            }

            // Set the maximum to the max of the current or the right child's max.
            if (pIndexNode->pRight != &m_Sentinel &&
                m_pointOperations.compare(
                    &pIndexNode->maxHighInterval,
                    &pIndexNode->pRight->maxHighInterval) < 0)
            {
                m_pointOperations.copy(&pIndexNode->maxHighInterval, &pIndexNode->pRight->maxHighInterval);
            }
        }

#if DEBUG
    public:
        //============================================================================
        // ClearTreeStats (DEBUG ONLY):
        //
        //      Function to reset tree statistics to 0.
        //============================================================================
        void
        ClearTreeStats
        (
        )
        {
            m_cLeaves = 0;
            m_cNodes = 0;
            m_cMaxDepth = 0;
            m_cTotalDepth = 0;
            m_cSearchDepth = 0;
            m_cSearchHits = 0;
            m_cSearchMisses = 0;
            m_cDeletionDepth = 0;
            m_cDeletionHits = 0;
            m_cDeletionMisses = 0;
            m_cInsertionDepth = 0;
            m_cInsertionHits = 0;
            m_cInsertionMisses = 0;
        }

        //============================================================================
        // DumpTreeStats (DEBUG ONLY):
        //
        //      Function to dump tree statistics to the output window.
        //============================================================================
        void
        DumpTreeStats
        (
        )
        {
            m_cLeaves = 0;
            m_cNodes = 0;
            m_cMaxDepth = 0;
            m_cTotalDepth = 0;

            ComputeStats(m_pRoot, 1);

            DebPrintf("Red Black Tree Statistics: \n");
            DebPrintf("    Number of nodes: %lu\n", m_cNodes);
            DebPrintf("    Number of leaves: %lu\n", m_cLeaves);
            DebPrintf("    ---------------\n");

            DebPrintf("    Maximum tree depth: %lu\n", m_cMaxDepth);
            DebPrintf("    Average tree depth: %lf\n", m_cNodes ? (double)(m_cTotalDepth / m_cNodes) : (double) 0.0);
            DebPrintf("    ---------------\n");

            if (m_cSearchHits || m_cSearchMisses)
            {
                DebPrintf("    Search hits: %lu\n", m_cSearchHits);
                DebPrintf("    Search misses: %lu\n", m_cSearchMisses);
                DebPrintf("    Average Search: %lf\n", (m_cSearchHits || m_cSearchMisses) ? ((double) m_cSearchDepth / (m_cSearchHits + m_cSearchMisses)) : (double) 0.0);
                DebPrintf("    ---------------\n");
            }

            if (m_cDeletionHits || m_cDeletionMisses)
            {
                DebPrintf("    Deletion hits: %lu \n", m_cDeletionHits);
                DebPrintf("    Deletion misses: %lu \n", m_cDeletionMisses);
                DebPrintf("    Average deletion: %lf\n", (m_cDeletionHits || m_cDeletionMisses) ?  ((double) m_cDeletionDepth / (m_cDeletionHits + m_cDeletionMisses)) : (double) 0.0);
                DebPrintf("    ---------------\n");
            }

            if (m_cInsertionHits || m_cInsertionMisses)
            {
                DebPrintf("    Insertion hits: %lu \n", m_cInsertionHits);
                DebPrintf("    Insertion misses: %lu \n", m_cInsertionMisses);
                DebPrintf("    Average insertion: %lf\n", (m_cInsertionHits || m_cInsertionMisses) ?  ((double) m_cInsertionDepth / (m_cInsertionHits + m_cInsertionMisses)) : (double) 0.0);
                DebPrintf("    ---------------\n");
            }
        }

        //============================================================================
        // VerifyTree (DEBUG ONLY):
        //
        //      Function that verifies the red black properties of a tree.
        //
        //      Return values:
        //          - True: the tree is a valid Red Black Tree
        //          - False: the tree is not a valid Red Black Tree
        //
        //============================================================================
        bool
        VerifyTree
        (
        )
        {
            m_IsValidRedBlackTree = true;

            // Verify that the root is black
            if (m_pRoot->color != rbcBlack)
            {
                m_IsValidRedBlackTree = false;

                VSFAIL("Black root  property violated for Red-Black Tree");
                DebPrintf("Black root property violated for Red-Black Tree\n");
            }

            VerifySubTree(m_pRoot);

            return m_IsValidRedBlackTree;
        }

    protected:
        //============================================================================
        // ComputeStats (DEBUG ONLY):
        //
        //      Function that recursively computes the statistics for the tree.
        //============================================================================
        void
        ComputeStats
        (
            IntervalNodeBase *pNode,  // Current sub tree
            unsigned long iDepth    // Current depth of the tree
        )
        {
            if (pNode != &m_Sentinel)
            {
                // Update number of nodes
                m_cNodes++;

                // Update maximum depth
                if (iDepth > m_cMaxDepth)
                {
                    m_cMaxDepth = iDepth;
                }

                // Update total depth (used for average)
                m_cTotalDepth += iDepth;

                // Update number of leaves
                if (pNode->pLeft == &m_Sentinel && pNode->pRight == &m_Sentinel)
                {
                    m_cLeaves++;
                }

                // Recurse into left and right subtrees.
                ComputeStats(pNode->pLeft, iDepth + 1);
                ComputeStats(pNode->pRight, iDepth + 1);
            }
        }

        //============================================================================
        // VerifySubTree (DEBUG ONLY):
        //
        //      Function that recursively verifies the red black properties of a tree.
        //
        //      Return value:
        //          - The black height of the given sub tree.
        //
        //============================================================================
        unsigned long
        VerifySubTree
        (
            IntervalNodeBase *pNode        // Current sub tree.
        )
        {
            unsigned long cLeftBlackHeight = 0;
            unsigned long cRightBlackHeight = 0;

            if (pNode == &m_Sentinel)
            {
                return 0;
            }

            // Verify the BST property for the left child.
            if (pNode->pLeft != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(&pNode->pLeft->key, &pNode->key);

                if (iComp >= 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Binary Search property violated for Red-Black Tree");
                    DebPrintf("Binary Search property violated for Red-Black Tree\n");
                }
            }

            // Verify the BST property for the right child.
            if (pNode->pRight != &m_Sentinel)
            {
                int iComp = m_keyOperations.compare(&pNode->key, &pNode->pRight->key);

                if (iComp >= 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Binary Search property violated for Red-Black Tree");
                    DebPrintf("Binary Search property violated for Red-Black Tree\n");
                }
            }

            // Verify the max interval properties.
            if (pNode->pLeft != &m_Sentinel)
            {
                int iComp = m_pointOperations.compare(&pNode->pLeft->maxHighInterval, &pNode->maxHighInterval);

                if (iComp > 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Max Interval property violated for Red-Black Tree");
                    DebPrintf("Max Interval property violated for Red-Black Tree\n");
                }
            }

            // Verify the max interval properties.
            if (pNode->pRight != &m_Sentinel)
            {
                int iComp = m_pointOperations.compare(&pNode->pRight->maxHighInterval, &pNode->maxHighInterval);

                if (iComp > 0)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Max Interval property violated for Red-Black Tree");
                    DebPrintf("Max Interval property violated for Red-Black Tree\n");
                }
            }

            int iComp = m_keyToPointOperations.compareHigh(&pNode->key, &pNode->maxHighInterval);

            if (iComp > 0)
            {
                m_IsValidRedBlackTree = false;

                VSFAIL("Max Interval property violated for Red-Black Tree");
                DebPrintf("Max Interval property violated for Red-Black Tree\n");
            }


            // Verify color is either Red or Black.
            switch (pNode->color)
            {
                case rbcRed:
                case rbcBlack:
                    break;

                default:
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Color property violated for Red-Black Tree");
                    DebPrintf("Color property violated for Red-Black Tree\n");
            }

            // Verify that red nodes have black children
            if (pNode->color == rbcRed)
            {
                if (pNode->pLeft->color != rbcBlack || pNode->pRight->color != rbcBlack)
                {
                    m_IsValidRedBlackTree = false;

                    VSFAIL("Black children of red node property violated for Red-Black Tree");
                    DebPrintf("Black children of red node property violated for Red-Black Tree\n");
                }
            }

            // Verify that black heights are equal on both sides.
            cLeftBlackHeight = VerifySubTree(pNode->pLeft);
            cRightBlackHeight =VerifySubTree(pNode->pLeft);

            if (cLeftBlackHeight != cRightBlackHeight)
            {
                m_IsValidRedBlackTree = false;

                VSFAIL("Black height property violated for Red-Black Tree");
                DebPrintf("Black height property violated for Red-Black Tree\n");

                return 0;
            }

            // Return black height
            if (pNode->color == rbcBlack)
            {
                return cLeftBlackHeight + 1;
            }

            return cLeftBlackHeight;
        }
#endif

    protected:
        bool m_ShouldReleaseData: 1;     // Should the tree call the m_keyOperations.destroy and m_nodeOperations.destroy before the nodes are freed?

        NorlsAllocator *m_pNoReleaseAllocator;  // Memory allocator

        IntervalNodeBase *m_pRoot;          // Pointer to the root of the tree.
        IntervalNodeBase m_Sentinel;        // Sentinel Node

        CriticalSectionType m_cs;   // Critical section to lock the tree.
        KeyOperations m_keyOperations;
        PointOperations m_pointOperations;
        KeyToPointOperations m_keyToPointOperations;
        NodeOperations m_nodeOperations;

#if DEBUG
        // Red Black Tree Statistics
        bool m_IsValidRedBlackTree;
        bool m_IsRemove;                        // Are we currenlty removing a node?

        unsigned long m_cLeaves;
        unsigned long m_cNodes;
        unsigned long m_cMaxDepth;
        unsigned long m_cTotalDepth;
        unsigned long m_cSearchDepth;
        unsigned long m_cSearchHits;
        unsigned long m_cSearchMisses;
        unsigned long m_cDeletionDepth;
        unsigned long m_cDeletionHits;
        unsigned long m_cDeletionMisses;
        unsigned long m_cInsertionDepth;
        unsigned long m_cInsertionHits;
        unsigned long m_cInsertionMisses;
#endif
};

//
// MRUList A fixed size list implementation that favors the most recently used items.
//
//============================================================================
// MRUListT:
//
//      Template implementation of a fixed size MRU (most recently used) list. The list doesn't
//      support insertion of duplicate items.
//
//      Description: Uses a redblacktree for fast lookup and a matching double linked list to maintian the
//      MRU ordering. New items are added at the beginning of the list. If an existing item is queried for it
//      is moved to the head of the list. List size is limited to max # items specified. Once the limit is reached
//      items are removed from the tail of the list (least used items).
//
//  Template Parameters:
//
//      KeyType: Specifies the type of the index key.
//
//      NodeType: [Optional] Specifies the type of the nodes that the tree will
//                      hold. If specified, this type must privately derive from
//                      RedBlackNodeBaseT. If it is not specified, a simple
//                      implementation of RedBlackNodeBaseT specialized for
//                      KeyType, without any additional data, will be used.
//
//      KeyOperations: [Optional] Specifies an inline class to manipulate the
//                             keys of the tree. If it is not specified, a simple
//                             implementation for intrinsic types will be used. The
//                             type should define the following static methods,
//                             which should adequately handle the requirements
//                             for the data:
//
//           static int compare(const <KeyType> *key1, const <KeyType> *key2)
//                  - performs a comparison between key1 and key2
//                  - returns: 0 if key1 = key2
//                                 <0 if key1 < key2
//                                 >0 if key1 > key2
//
//           static void destroy (<KeyType> *key)
//                  - performs any required cleanup for key.
//
//           static void copy (<KeyType> *destkey, const <KeyType> *srckey)
//                  - copies the contents of srckey into destkey.
//
//      NodeOperations: [Optional] Specifies an inline class to manipulate
//                               additional information in the node. If it is not
//                               specified, a default implementation for nodes
//                               without any additional data will be used. The
//                               type should define the following static methods,
//                               which should adequately handle the requirements
//                               for the data:
//
//              static void destroy (NodeType *node)
//                  - performs any required cleanup on the additional information on the node.
//
//              static void copy (NodeType *destnode, const NodeType *srcnode)
//                  - copies the additional contents of srcnode into destnode.
//
//       CriticalSectionType: [Optional] Specify a critical section to use for
//                                     synchornization. Must be either
//                                     CComAutoCriticalSection or CComFakeCriticalSection
//============================================================================
template
    <
        typename KeyType,
        typename KeyOperations = SimpleKeyOperationsT<KeyType>,
        typename NodeType = = RedBlackNodeBaseT<KeyType>,
        typename NodeOperations = EmptyNodeOperationsT<NodeType>,
        typename CriticalSectionType = CComFakeCriticalSection
    >
class MRUListT
{
public:

    //============================================================================
    // Constructor: Initialize default values
    //============================================================================
    MRUListT
    ():
        m_pNoReleaseAllocator(NULL),
        m_MaxItems(0)
    {
    }

    //============================================================================
    // Constructor: Initialize default values
    //============================================================================
    MRUListT
    (
        NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
        ULONG MaxItems,                         // [in] Maximum # of items to keep track of.
        bool ShouldReleaseData = false          // [in, optional] should the data be released before releasing the node?
    ):
        m_pNoReleaseAllocator(pNoReleaseAllocator),
        m_Tree(pNoReleaseAllocator, ShouldReleaseData),
        m_MaxItems(MaxItems)
    {
    }

    //============================================================================
    // Destructor
    //============================================================================
    ~MRUListT()
    {
        Clear();
    }

    //============================================================================
    // Initialize default values - must be used if default constructor was called
    //============================================================================
    void Init
    (
        NorlsAllocator *pNoReleaseAllocator,    // [in] Pointer to the allocator to use
        ULONG MaxItems,                         // [in] Maximum # of items to keep track of.
        bool ShouldReleaseData = false          // [in, optional] should the data be released before releasing the node?
    )
    {
        VSASSERT(m_pNoReleaseAllocator == NULL, "MRULIstT has already been initialized");

        m_pNoReleaseAllocator = pNoReleaseAllocator;
        m_Tree.Init(pNoReleaseAllocator, ShouldReleaseData);
        m_MaxItems = MaxItems;
    }

    //============================================================================
    // Insert: Give a key construct a new node item
    //      The new node will be added to the redblack tree and added at the
    //      head of the list.
    //      If we've reached the max item count the tail item of the list will
    //      be deleted from both the tree and list
    //
    //      Returns: True if the item was added
    //               False if an item already existed with that key (in this case
    //               you get back the existing item and it is moved to the head of
    //               list).
    //============================================================================
    bool Insert(KeyType *pItemKey, NodeType **ppNewItem)
    {

        MRULinkedListNode   *pListItem = NULL;
        MRUTreeNode         *pNewNode = NULL;

        //Delete the most unused item if we've reached maxitems
        if (m_Tree.Count() == m_MaxItems)
        {
            AssertIfNull(m_List.GetLast());

            MRUTreeNode *pNodeToDelete;
            if (m_Tree.Find(&m_List.GetLast()->ItemKey, &pNodeToDelete) && pNodeToDelete)
            {
                pNodeToDelete->pMatchingListItem = NULL;
                m_Tree.Remove(pNodeToDelete);

                // Delete the item from the list and fix up the tail pointer
                pListItem = m_List.GetLast();
                m_List.Remove(pListItem);
                KeyOperations::destroy(&pListItem->ItemKey);
                delete pListItem;
            }
            else
            {
                // The most likely cause of this is an incorrect Compare function for the Key
                VSFAIL("The MRUList tree is corrupt");
            }
        }

        // Insert a new Node in the redblacktree
        // If we've reached maxitems the new node will reuse the allocated
        // memory for the deleted item above. This prevents the NRLS growing larger tahn maxitems
        bool result = m_Tree.Insert(pItemKey, &pNewNode);

        //Return the new node
        if (ppNewItem)
        {
            *ppNewItem = (NodeType *)pNewNode;
        }

        if (!result)
        {
            //This means a node already existed with this key so just move the item to the head of the list.
            MostRecentlyUsed(pNewNode->pMatchingListItem);
            return result;
        }

        // Insert a new list item node
        pListItem = new (zeromemory) MRULinkedListNode();
        KeyOperations().copy(&pListItem->ItemKey , &pNewNode->key);
        m_List.InsertFirst(pListItem);

        //Add a pointer from the Tree nodes item to the corresponding list item
        pNewNode->pMatchingListItem = pListItem;

        return result;
    }

    //============================================================================
    // Find : Give a key find the matching mrulist item
    //      If found the item is moved to the head of the list.
    //
    //      Returns True if the item was found.
    //============================================================================
    bool Find(KeyType *pItemKey, NodeType **ppNodeData)
    {
        MRUTreeNode *pFoundNode = NULL;

        if (m_Tree.Find(pItemKey, &pFoundNode) && pFoundNode)
        {
            //Return the found node
            if (ppNodeData)
            {
                *ppNodeData = (NodeType *)pFoundNode;
            }

            MostRecentlyUsed(pFoundNode->pMatchingListItem);

            return true;
        }

        return false;
    }

    //============================================================================
    // Clear: Destroy the tree and list
    //      For the list we need to delete all of the MRULinkedListNode's we created.
    //============================================================================
    void Clear()
    {
        m_Tree.Clear();

        MRULinkedListNode   *pListItem = NULL;

        //Clear the linked list and delete each node.
        while (m_List.GetFirst())
        {
            pListItem = m_List.GetFirst();
            m_List.Remove(pListItem);
            KeyOperations::destroy(&pListItem->ItemKey);
            delete pListItem;
        }
    }


private:

    struct MRULinkedListNode : CDoubleLink<MRULinkedListNode>
    {
        KeyType     ItemKey;
    };

    struct MRUTreeNode : NodeType
    {
        MRULinkedListNode   *pMatchingListItem;
    };

    struct MRUTreeNodeOperations : NodeOperations
    {
        static void copy(MRUTreeNode *pNodeDest, MRUTreeNode *pNodeSrc)
        {
            NodeOperations::copy(pNodeDest, pNodeSrc);
            pNodeDest->pMatchingListItem = pNodeSrc->pMatchingListItem;
        }
    };

    //============================================================================
    // MostRecentlyUsed: Move this item to the head of the list
    //============================================================================
    void MostRecentlyUsed(MRULinkedListNode   *pListItem)
    {
        AssertIfNull(pListItem);

        // Move this item to the head of the list since it is now the
        // most recently used item
        m_List.Remove(pListItem);
        m_List.InsertFirst(pListItem);
    }

    RedBlackTreeT<KeyType,
                  KeyOperations,
                  MRUTreeNode,
                  MRUTreeNodeOperations,
                  CriticalSectionType>                  m_Tree;
    CDoubleList<MRULinkedListNode>                      m_List;
    NorlsAllocator                                      *m_pNoReleaseAllocator;
    ULONG                                               m_MaxItems;

};
