//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Defines the file-level XMLDoc object. This object manages all
//  XMLDocs created by the owning sourcefile.
//
//  Defines the XMLDoc nodes which are created for each XMLDoc block.
//  Defines a bunch of other helpers.
//
//-------------------------------------------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable : 4200)

class XMLDocFile;

#define RECURSIVE_BANNER

const unsigned MaximumBannerSize = 100;

// Used to represent each node with an index (e.g. second child) in the root of the XML tree.
struct XMLDocFlagsAndNodesIndex
{
    XMLDOCFLAGS m_Flag;
    long        m_NodeIndex;
};

// XML string append helprs.
static
void XMLDocStartTag(
    BaseWriter * pBufferToUse,
    const STRING * pXMLDocTag)
{
    pBufferToUse->AppendChar(CHAR_XMLOpenTag);
    pBufferToUse->AppendString(pXMLDocTag);
    pBufferToUse->AppendString(WIDE(">\r\n"));
}

static
void XMLDocEndTag(
    BaseWriter * pBufferToUse,
    const STRING * pXMLDocTag)
{
    pBufferToUse->AppendString(WIDE("\r\n<"));
    pBufferToUse->AppendChar(CHAR_Forwartd_Slash);
    pBufferToUse->AppendString(pXMLDocTag);
    pBufferToUse->AppendChar(CHAR_XMLCloseTag);
}

// A little hacky, but here it goes. If there is an error in the XMLDoc region, then we will squiggle the whole
// region. If we do that, then if the user colapses the region and then double clicks on the tasklist error, the region
// will get selected but it will not expand. To get around that, we exclude the first XMLDoc token "'@" from the span
// of the XML doc comment. This makes it such that any errors will be reported inside the region, which forces the region
// to expand if it was already collapled.
// This location is used if we can't tell where exactly the error is in the XML block or if the whole block has an error
// (e.g. doesn't belong to a valid language element).
static
Location GetDefaultXMLDocErrorLocation(Location XMLDocBlockLocation)
{
    XMLDocBlockLocation.m_lBegColumn += 2;
    return XMLDocBlockLocation;
}

#if IDE 

class ParamInfo
{
    friend class XMLDocNode;

public:
    ParamInfo() {}
    ~ParamInfo() { SysFreeString(m_ParamName); SysFreeString(m_ParamText); }
private:
    BSTR m_ParamName;
    BSTR m_ParamText;
};

#endif

#if IDE 
class XMLParseTreeCache
{
public:

    XMLParseTreeCache();
    ~XMLParseTreeCache();

    NorlsAllocator* GetNorlsAllocator()
    {
        return &m_norlsAlloc;
    }

    bool TryGetText(
        _In_ SourceFile* pSourceFile,
        _Deref_out_ Text** ppText);
    void SetText(
        _In_ SourceFile* pSourceFile,
        _In_ Text* pText);
    bool TryGetParseTree(
        _In_ SourceFile* pSourceFile,
        _Deref_out_ ParseTree::FileBlockStatement** ppTree);
    void SetParseTree(
        _In_ SourceFile* pSourceFile,
        _In_opt_ ParseTree::FileBlockStatement* pTree);
    ParseTree::StatementList** GetCachedCommentLocation();

    static
    bool TryGetCurrent(_Deref_out_ XMLParseTreeCache** ppCurrent);
private:

    static
    XMLParseTreeCache** GetStorageForThread();

    NorlsAllocator m_norlsAlloc;
    DynamicHashTable<STRING*,Text*,NorlsAllocWrapper> m_textMap;
    DynamicHashTable<STRING*,ParseTree::FileBlockStatement*,NorlsAllocWrapper> m_parseTreeMap;
    XMLParseTreeCache* m_pPrevious;

    // used for caching the result of each node's location in the parse tree
    ParseTree::StatementList *m_pCachedCommentLocation; 

    static XMLParseTreeCache* s_pCacheMainThread;
    static XMLParseTreeCache* s_pCacheBackgroundThread;
};

#endif

//Defines a functor for use with XMLDocNode::PrefixTraversal
//that processes the text associated with a node in a DOM document
//by appropiately interperting the values of <see>, <seeAlso>, and
//<paramref> tags.
class NodeTextProcessor
{
public:
    NodeTextProcessor(StringBuffer * pBuffer) :
      m_pBuffer(pBuffer)
    {
        VSASSERT(pBuffer, "The buffer should not be NULL");
    }

    NodeTextProcessor(const NodeTextProcessor &src) :
        m_pBuffer(src.m_pBuffer)
    {
    }

    void operator()(IXMLDOMNode * pNode);
private:
    void ProcessText(BSTR text);
    bool IsElement(IXMLDOMNode * pNode);
    bool IsSeeOrSeeAlso(IXMLDOMNode * pNode);
    bool IsParamRef(IXMLDOMNode * pNode);
    bool IsCharData(IXMLDOMNode * pNode);
    bool IsAttribute(IXMLDOMNode * pNode);

    bool GetNodeValue(
        IXMLDOMNode * pNode,
        CComVariant &refValue);

    bool GetAttributeValue(
        IXMLDOMNode * pNode,
        const wchar_t * nodeName,
        CComVariant &refValue);

    DOMNodeType GetNodeType(IXMLDOMNode * pNode);

    StringBuffer * m_pBuffer;
};


// Every XMLDoc comment entity creats one of these structures. We can get to
// the owning symbol as well as the XMLDom tree from here.
class XMLDocNode : ZeroInit<XMLDocNode>
{
    friend class XMLDocFile;

public:
    XMLDocNode() {}
    ~XMLDocNode() {}

#if IDE 
    HRESULT GetSummaryText(BSTR *pbstrSummaryText);

    HRESULT GetParamCount(long *pParams);

    HRESULT GetParamTextAt(
        long iParam,
        BSTR * pbstrName,
        BSTR * pbstrText);

    HRESULT GetReturnsText(BSTR *pbstrReturnsText);

    HRESULT GetRemarksText(BSTR *pbstrRemarksText);

    HRESULT GetExceptionCount(long *piExceptions);

    HRESULT GetExceptionTextAt(
        long iException,
        BSTR * pbstrType,
        BSTR * pbstrText);

    HRESULT GetFilterPriority(long *piFilterPriority);

    HRESULT GetCompletionListText(BSTR *pbstrCompletionListText);

    HRESULT GetCompletionListTextAt(
        long iParam,
        BSTR * pbstrCompletionListText);

    HRESULT GetPermissionSet(BSTR * pbstrPermissionSetXML);

    HRESULT GetTypeParamCount(long * piTypeParams);

    HRESULT GetTypeParamTextAt(
        long iTypeParam,
        BSTR * pbstrName,
        BSTR * pbstrText);

    HRESULT GetParamCountHelper(
        DynamicArray<ParamInfo> * pdaParamArray,
        long * pParams);

    HRESULT GetParamTextAtHelper(
        DynamicArray<ParamInfo> * pdaParamArray,
        long iParam,
        BSTR * pbstrName,
        BSTR * pbstrText);

    struct CapabilityInfo
    {
        CComBSTR m_bstrType;
        CComBSTR m_bstrDescription;
    };

    HRESULT GetAssociatedCapabilities(DynamicArray<CapabilityInfo> &daCapabilities);

#endif IDE
    void ReplaceIncludeNode(
        IXMLDOMNode * pOriginalNode,
        IXMLDOMNodeList * pNewIncludeNodeList,
        IXMLDOMNode * pNewIncludeNode);

    void ResolveIncludeTag(IXMLDOMNode *pIncludeTag);

    Location * GetCommentLocation() 
    {
         return &m_CommentTextSpan;
    }

    void RestoreSymbol()
    {
        m_pOwningSymbol->SetXMLDocNode(this);
    }

    XMLDocNode *m_next;
    XMLDocFile* GetParent() 
    {
        return m_pParentXMLDocFile;
    }

protected:
    SourceFile *GetSourceFile();
    Compiler *GetCompiler();

private:
    static const wchar_t * m_uniqueWellKnownTags[]; //used to initialize the hash set above;
    static const unsigned int m_uniqueWellKnownTagCount;

    static bool IsUniqueTag(const wchar_t * pTagName);

    void InitNode(
          XMLDocFile *pXMLDocFile,
          BCSYM_NamedRoot *pOwningSymbol,
          ParseTree::CommentBlockStatement *pCommentBlock,
          Location XMLDocLocation
    #if IDE 
          ,NorlsAllocator *pAllocatorForXml
    #endif
          );

    void ReleaseNode();
    void ReleaseNodeTree();

#if IDE 
    XMLElement * FindRootXMLDocNodeFromIndex(
        XMLStartTag * pXMLNode,
        long RootNodeIndex);

    XMLBaseNode * FindMostEncapsulatingXMLDocNode(
        XMLStartTag * pXMLNode,
        long ErrorPosition,
        XMLBaseTag * * ppXMLNode);

    Location CalculateRealSpan(
        long ErrorPosition,
        ERRID * pSubstituteErrorID,
        _Deref_opt_out_z_ WCHAR * * pExtraText,
        XMLBaseNode * pXMLDocNodeContainingError,
        XMLBaseTag * pXMLNode);
#endif IDE

    HRESULT StoreResolvedName(
        IXMLDOMNode * pCrefAttribute,
        BCSYM_NamedRoot * pBoundSymbol);

    HRESULT StoreUnResolvedName(
        IXMLDOMNode * pCrefAttribute,
        _In_z_ WCHAR * badString);

    long GetNodeIndex(XMLDOCFLAGS Flag);

    Location GetXMLDocErrorLocation(
        long ErrorPosition,
        ERRID * pSubstituteErrorID,
        _Deref_out_z_ WCHAR * * pExtraText);

    Location GetXMLDocErrorLocationFromNodeIndex(Stack<long> * pIndexStack);

    Location GetXMLDocErrorLocationFromNodeIndex(long NodeIndex);

    // Parses an XML string and returns an XMLDom root node.
    void LoadXMLDocString(
        bool reportErrors,
        _Inout_opt_ IXMLDOMDocument **ppOutput);

    void LoadXMLFile(
        BSTR FileName,
        _Inout_opt_ IXMLDOMDocument * * ppOutput);

    // Verifies that the parsed XML is valid for this particular code element (symbol).
    // Returns true if verification passes or false if not.
    WCHAR *StringFromXMLDOCFlags(XMLDOCFLAGS AllKnownTages);

    STRING *GetStringOfSymbol();
    BCSYM_Proc *GetDelegateInvokeSymbol();

    bool VerifyIncludeFileAndPath(
        IXMLDOMNode * pIncludeNode,
        Stack<long> * pIndexStack,
        bool reportErrors);

    bool VerifyAllIncludeTags(IXMLDOMElement *pRootElement, bool reportErrors);
    bool VerifyException(IXMLDOMNode *pParameterNode, long NodeIndex, bool reportErrors);
    bool VerifyGenericParameter(IXMLDOMNode *pParameterNode, long NodeIndex, bool reportErrors);
    bool VerifyParameter(IXMLDOMNode *pParameterNode, long NodeIndex, bool reportErrors);
    bool VerifyXMLDocAgainstSymbol(IXMLDOMElement *pXMLRootElement, bool reportErrors);
    void GenerateIllegalTagErrors(XMLDOCFLAGS Flag);
    bool VerifyNoIllegalTags(bool reportErrors);
    XMLDOCFLAGS GetTagFlag(_In_z_ WCHAR *pTagName);
    bool CompareAttributes(IXMLDOMNode *pFirstElement, IXMLDOMNode *pSecondElement);
    bool VerifyXMLDocRootChildren(IXMLDOMElement *pXMLRootElement, bool reportErrors);

    void BuildXMLDomTree(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput);
    void BindXMLDomTree(IXMLDOMElement *pRootElement, bool reportErrors);

    HRESULT PerformActualBinding(IXMLDOMNode  *pCrefAttribute, Stack<long> *pIndexStack, bool reportErrors);
    void VerifyCRefAttributeOnNode(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors);
    void RecursivelyBindXMLTree(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors);

    bool RecursivelyVerifyIncludeNodes(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors);
    void ResolveIncludeTags(IXMLDOMDocument *pDocument);

    void ResolveMemberName(IXMLDOMDocument *pDocument);

    void ProcessXMLDocCommentNode(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput);
    void BuildXMLDocCommentNode(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput);

    HRESULT CreateXMLDocument(REFIID riid, void **ppXMLDocument);

#if IDE 
    void EnsureLightWeightXMLDocParseTreeLoaded();
    ParseTree::CommentBlockStatement *GetXMLDocCommentBlock(NorlsAllocator *pAllocator);

    HRESULT GetGenericNodeText(IXMLDOMDocument *pDoc, BSTR *pbstrText, _In_z_ const WCHAR * NodeType);

    // Generates text for see, param, etc
    void RenderGenericNodeText(BSTR *pbstrText, _In_z_ const WCHAR * wszNodeType);
    void RenderGenericNodeText(CXMLDocParser *pXMLDocParser, XMLStartTag *pStartTag, StringBuffer *psbText);

    // Synchronization
    CriticalSection m_XMLDocNodeCriticalSection;

    void LockNode() { m_XMLDocNodeCriticalSection.Enter(); }
    void UnLockNode() { m_XMLDocNodeCriticalSection.Leave(); }

    DynamicArray<ParamInfo> *m_pArrayOfParameters;
    DynamicArray<ParamInfo> *m_pArrayOfGenericParameters;
    DynamicArray<ParamInfo> *m_pArrayOfExceptions;
    NorlsAllocator *m_pAllocatorForXml;
    XMLStartTag    *m_pXMLDocRoot;

    bool m_fDidRunValidation;

#else
    //We only store a string buffer for XML outside of the IDE.
    //This uses less memory for IDE builds. Inside the IDE, parse trees
    //are cached and generally have long live times. To avoid duplication,
    //we eliminate the separate buffer and use an XMLCommentStream to "lift"
    //the XML comment text directly out of the parse tree
    //(see the m_pCommentBlock member above). However, inside  vbc.exe parse
    //trees have a verry short life time and don't exist at the point where
    //XML needs to be generated, so we make a copy of the text and store it
    //in this buffer.

    StringBuffer * m_pXMLDocString;
#endif

    template <typename F, typename P>
    void PrefixTraversal(IXMLDOMNode * pRoot, F functor, P descendPredicate = P());

    DynamicArray<XMLDocFlagsAndNodesIndex> m_XMLDocFlagsAndNodesIndex;

    BCSYM_NamedRoot *m_pOwningSymbol;
    BCSYM_Proc *m_pInvokeProcForDelegates;

    // Containing file.
    XMLDocFile *m_pParentXMLDocFile;

    TrackedLocation m_CommentTextSpan;

    XMLDOCFLAGS m_UsedKnownTags;
    bool m_IsBadXMLDocNode;     // Set to true when the XML is not parsable.

#if IDE
private:
    class XMLDocNodeLockHolder
    {
    public:
        XMLDocNodeLockHolder(XMLDocNode *pXMLDocNode);
        ~XMLDocNodeLockHolder();

    private:
        XMLDocNode *m_pXMLDocNode;
    };
#endif IDE
};

// This object manages XMLDocs associated with a SourceFile. This object contains two linked
// lists, one for all XMLDom comments that appear in the owning file, and one a subset of those
// nodes that contain CRef attributes.
class XMLDocFile : CSingleList<XMLDocNode>, ZeroInit<XMLDocFile>
{
public:

    // Constructor and Destructor
    XMLDocFile(Compiler *pCompiler, SourceFile *pSourceFile);
    ~XMLDocFile();

    // Adds an XMLDoc comment to the symbol passed in. If the XML fails verification, the symbol is unchanged
    // and an error is reported to the user.
    TrackedLocation *AddXMLDocComment(ParseTree::CommentBlockStatement *pCommentBlock, BCSYM_NamedRoot *pNamedElement);
    void DisableXMLDocNode(_In_ XMLDocNode *pXMLDocNode);
    void ReEnableXMLDocNodes();

    // Builds and returns an XMLDoc comment string for the owning file.
    HRESULT GetXMLDocForFile(BaseWriter *pXMLDocForFile);

    void DemoteXMLDocFile(CompilationState CS);
    void BindAllXMLDocCommentNodes();

    SourceFile *GetSourceFile() { return m_pSourceFile; }

#if IDE 

    ParseTree::FileBlockStatement *GetOrCreateCachedDeclTrees(XMLParseTreeCache *pCache);

    // Recursively collects the text of a tag and the text for all its children tags.
    static void CollectBannerString(XMLStartTag *pStartTag, StringBuffer *pXMLString, CXMLDocParser *pXMLDocParser);

    // Extracts an XML string from a CommentBlock.
    static HRESULT ExtractBannerFromXMLDocCommentBlock(StringBuffer *pXMLString, ParseTree::CommentBlockStatement *pCommentBlock);
#endif IDE

private:
    // Extracts an XML string from a CommentBlock.
    void static ExtractXMLDocStringFromCommentBlock(StringBuffer *pXMLString, ParseTree::CommentBlockStatement *pCommentBlock);

    // Releases all the XML documents that were created as a result of parsing XMLDocs for this file.
    void ReleaseAllXMLDocuments();
    void ReleaseXMLTrees();

    bool m_AreAllXMLDocCommentsProcessed;

    // Context
    Compiler *m_pCompiler;

    // Containing file.
    SourceFile *m_pSourceFile;

    //List of excluded XMLDoc nodes - too many XML comments on a partial type
    CSingleList<XMLDocNode> m_ExcludedXMLDocNodes;

#if IDE 
    NorlsAllocator *m_pAllocatorForXml;
#endif
};

template <typename F, typename P>
void XMLDocNode::PrefixTraversal(IXMLDOMNode * pRoot, F functor, P descendPredicate)
{
    Stack<IXMLDOMNode *> stack;
    CComPtr<IXMLDOMNode> pCurrent(pRoot);

    if (descendPredicate(pCurrent))
    {
        stack.Push(pCurrent.Detach());
    }


    while (! (stack.Empty()))
    {
        pCurrent.Attach(stack.Top());
        stack.Pop();

        functor(pCurrent);

        CComPtr<IXMLDOMNode> pChild;

        IfFailContinue(pCurrent->get_lastChild(&pChild));

        while (pChild)
        {
            CComPtr<IXMLDOMNode> pTmp(pChild);

            if (descendPredicate(pChild))
            {
                stack.Push(pChild.Detach());
            }
            else
            {
                pChild.Release();
            }
            IfFailBreak(pTmp->get_previousSibling(&pChild));
        }
    }
}

#pragma warning(pop)
