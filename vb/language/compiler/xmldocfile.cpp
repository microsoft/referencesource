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

#include "StdAfx.h"

#pragma warning(push)
#pragma warning(disable:6307)//,"WCHAR* to BSTR is OK is this case")

struct XMLCHARESCAPEDATA
{
    const WCHAR *wszEscapeSequence;
    size_t cchEscapeSequence;
    WCHAR wch;
};

static XMLCHARESCAPEDATA s_rgXMLCharEscapeData[] =
{
    {L"&amp;",    5,  L'&'},
    {L"&lt;",     4,  L'<'},
    {L"&gt;",     4,  L'>'},
    {L"&quot;",   6,  L'"'},
    {L"&apos;",   6,  L'\''},
};

DOMNodeType NodeTextProcessor::GetNodeType(IXMLDOMNode * pNode)
{
    VSASSERT(pNode, "The node should not be null!");
    DOMNodeType nodeType = NODE_INVALID;

    if (pNode)
    {
        if (FAILED(pNode->get_nodeType(&nodeType)))
        {
            nodeType = NODE_INVALID;
        }
    }

    return nodeType;
}

bool NodeTextProcessor::IsElement(IXMLDOMNode * pNode)
{
    return GetNodeType(pNode) == NODE_ELEMENT;
}

bool NodeTextProcessor::IsSeeOrSeeAlso(IXMLDOMNode * pNode)
{
    VSASSERT(pNode, "The node should not be null!");
    if (pNode)
    {
        CComBSTR bstrName;

        if (SUCCEEDED(pNode->get_nodeName(&bstrName)))
        {
            return bstrName == XMLDoc_See || bstrName == XMLDoc_SeeAlso;
        }
    }
    return false;
}

bool NodeTextProcessor::IsParamRef(IXMLDOMNode * pNode)
{
    VSASSERT(pNode, "The node should not be null!");
    if (pNode)
    {
        CComBSTR bstrName;

        if (SUCCEEDED(pNode->get_nodeName(&bstrName)))
        {
            return bstrName == XMLDoc_ParamRef;
        }
    }
    return false;
}

bool NodeTextProcessor::IsCharData(IXMLDOMNode * pNode)
{
    return GetNodeType(pNode) == NODE_TEXT;
}

bool NodeTextProcessor::IsAttribute(IXMLDOMNode * pNode)
{
    return GetNodeType(pNode) == NODE_ATTRIBUTE;
}

bool NodeTextProcessor::GetNodeValue(
    IXMLDOMNode * pNode,
    CComVariant &refValue)
{
    VSASSERT(pNode, "The node should not be null!");
    if (pNode)
    {
        CComVariant tmp;

        if (SUCCEEDED(pNode->get_nodeValue(&tmp)) && tmp.vt == VT_BSTR && SysStringLen(tmp.bstrVal))
        {
            tmp.Detach(&refValue);
            return true;
        }
    }
    return false;
}

bool NodeTextProcessor::GetAttributeValue(
    IXMLDOMNode * pNode,
    const wchar_t * nodeName,
    CComVariant &refValue)
{
    VSASSERT(pNode && nodeName, "Either the node or the nodeName was null!");
    if (pNode && nodeName)
    {
        CComPtr<IXMLDOMNamedNodeMap> pAttributeMap;

        if (SUCCEEDED(pNode->get_attributes(&pAttributeMap)) && pAttributeMap)
        {
            CComPtr<IXMLDOMNode> pAttribute;

            if (SUCCEEDED(pAttributeMap->getNamedItem(CComBSTR(nodeName), &pAttribute)) && pAttribute)
            {
                return GetNodeValue(pAttribute, refValue);
            }
        }
    }
    return false;
}

void NodeTextProcessor::operator()(IXMLDOMNode * pNode)
{
    CComVariant varValue;
    if (IsElement(pNode))
    {
        VARIANT_BOOL hasChildren = FALSE;
        if (FAILED(pNode->hasChildNodes(&hasChildren)))
        {
            hasChildren = FALSE;
        }

        if (IsSeeOrSeeAlso(pNode))
        {
            if
            (
                !hasChildren &&
                (
                    GetAttributeValue(pNode, XMLDoc_CRef, varValue) ||
                    GetAttributeValue(pNode, XMLDoc_LangWord, varValue)
                 )
            )
            {
                ProcessText(varValue.bstrVal);
            }
        }
        else if (IsParamRef(pNode))
        {

            if (! hasChildren && GetAttributeValue(pNode,XMLDoc_Name, varValue))
            {
                ProcessText(varValue.bstrVal);
            }
        }
    }
    else if (IsCharData(pNode) && GetNodeValue(pNode, varValue))
    {
        ProcessText(varValue.bstrVal);
    }
}

void NodeTextProcessor::ProcessText(BSTR text)
{
    VSASSERT(text, "The string should not be null");
    if (text)
    {
        m_pBuffer->AppendWithLength(text, SysStringLen(text));
    }
}

//////////////////////////////////////////////////////////////////////////////
// XMLDocNode.
//////////////////////////////////////////////////////////////////////////////

//Note that this array is sorted. KEEP IT THAT WAY.
//This is a static array, and I'm too lazy to write all the syncronization code
//to do the sort in code, so I just sorted it my self statically. We do binary search
//against this array, so if its not sorted the compiler will break.
const wchar_t * XMLDocNode::m_uniqueWellKnownTags[] =
{
    L"include",
    L"param",
    L"permission",
    L"remarks",
    L"returns",
    L"summary",
    L"typeparam",
    L"value"
};

const unsigned int XMLDocNode::m_uniqueWellKnownTagCount = sizeof(m_uniqueWellKnownTags) / sizeof(const wchar_t *);


#if IDE 

XMLParseTreeCache* XMLParseTreeCache::s_pCacheMainThread;
XMLParseTreeCache* XMLParseTreeCache::s_pCacheBackgroundThread;

XMLParseTreeCache::XMLParseTreeCache() :
    m_norlsAlloc(NORLSLOC),
    m_textMap(NorlsAllocWrapper(&m_norlsAlloc)),
    m_parseTreeMap(NorlsAllocWrapper(&m_norlsAlloc)),
    m_pPrevious(NULL),
    m_pCachedCommentLocation(NULL)
{
    XMLParseTreeCache** ppLoc = GetStorageForThread();
    m_pPrevious = *ppLoc;
    *ppLoc = this;
}


XMLParseTreeCache::~XMLParseTreeCache()
{
    XMLParseTreeCache** ppLoc = GetStorageForThread();
    *ppLoc = m_pPrevious;
}

// static
bool
XMLParseTreeCache::TryGetCurrent( _Deref_out_ XMLParseTreeCache** ppCurrent)
{
    ThrowIfNull(ppCurrent);
    *ppCurrent = *GetStorageForThread();
    return *ppCurrent;
}

// static
XMLParseTreeCache**
XMLParseTreeCache::GetStorageForThread()
{
    if ( GetCompilerSharedState()->IsInMainThread() )
    {
        return &s_pCacheMainThread;
    }
    else
    {
        return &s_pCacheBackgroundThread;
    }
}

bool 
XMLParseTreeCache::TryGetText(
    _In_ SourceFile* pSourceFile,
    _Deref_out_ Text** ppText)
{
    ThrowIfNull(pSourceFile);
    ThrowIfNull(ppText);
    return m_textMap.GetValue(pSourceFile->GetFileName(), ppText);
}

void 
XMLParseTreeCache::SetText(
    _In_ SourceFile* pSourceFile,
    _In_ Text* pText)
{
    ThrowIfNull(pSourceFile);
    ThrowIfNull(pText);
    m_textMap.SetValue(pSourceFile->GetFileName(), pText);
}

bool 
XMLParseTreeCache::TryGetParseTree(
    _In_ SourceFile* pSourceFile,
    _Deref_out_ ParseTree::FileBlockStatement** ppTree)
{
    ThrowIfNull(pSourceFile);
    ThrowIfNull(ppTree);
    return m_parseTreeMap.GetValue(pSourceFile->GetFileName(), ppTree);
}

void 
XMLParseTreeCache::SetParseTree(
    _In_ SourceFile* pSourceFile,
    _In_opt_ ParseTree::FileBlockStatement* pTree)
{
    ThrowIfNull(pSourceFile);
    m_parseTreeMap.SetValue(pSourceFile->GetFileName(), pTree);
}

ParseTree::StatementList** XMLParseTreeCache::GetCachedCommentLocation()
{
    return &m_pCachedCommentLocation;
}

#endif

/*****************************************************************************
;InitNode

Initializes a new XMLDocNode node.
*****************************************************************************/
void XMLDocNode::InitNode
(
    XMLDocFile *pXMLDocFile,
    BCSYM_NamedRoot *pOwningSymbol,
    ParseTree::CommentBlockStatement *pCommentBlock,
    Location XMLDocLocation
#if IDE 
    ,NorlsAllocator *pAllocatorForXml
#endif
)
{
    m_pParentXMLDocFile = pXMLDocFile;
    m_IsBadXMLDocNode = false;
    m_pOwningSymbol = pOwningSymbol;
    m_pInvokeProcForDelegates = NULL;

    // Save the location of the XMLDoc comment block so it can be used later for error reporting.
    m_CommentTextSpan.SetLocation(&XMLDocLocation);

#if IDE 
    m_pArrayOfParameters = new DynamicArray<ParamInfo>();
    m_pArrayOfGenericParameters = new DynamicArray<ParamInfo>();
    m_pArrayOfExceptions = new DynamicArray<ParamInfo>();
    m_XMLDocNodeCriticalSection.CriticalSection::CriticalSection();
    m_fDidRunValidation = false;
    m_pAllocatorForXml = pAllocatorForXml;
#else
    XMLCommentStream *pStream = new (zeromemory) XMLCommentStream(pCommentBlock, true);
    CComQIPtr<ISequentialStream> pSeqStream(pStream);

    m_pXMLDocString = new StringBuffer(pSeqStream);

    pStream->Clear();
#endif IDE
}

/*****************************************************************************
;ReleaseNode

Releases all resources used by this node.
*****************************************************************************/
void XMLDocNode::ReleaseNode()
{
#if IDE 
    LockNode();
    m_fDidRunValidation = false;
#endif IDE

    m_pOwningSymbol->SetXMLDocNode(NULL);
    m_pOwningSymbol = m_pInvokeProcForDelegates = NULL;

    m_IsBadXMLDocNode = false;


    m_XMLDocFlagsAndNodesIndex.Collapse();

#if IDE 
    delete m_pArrayOfParameters;
    delete m_pArrayOfGenericParameters;
    delete m_pArrayOfExceptions;

    m_pArrayOfParameters = NULL;
    m_pArrayOfGenericParameters = NULL;
    m_pArrayOfExceptions = NULL;
    m_pXMLDocRoot = NULL;

    UnLockNode();

    m_XMLDocNodeCriticalSection.CriticalSection::~CriticalSection();
#else
    if (m_pXMLDocString)
    {
        delete m_pXMLDocString;
        m_pXMLDocString = NULL;
    }
#endif IDE
}

/*****************************************************************************
;ReleaseNodeTree

Release the XML tree but don't release the XML string.
*****************************************************************************/
void XMLDocNode::ReleaseNodeTree()
{
#if IDE 
    LockNode();
    m_fDidRunValidation = false;
#endif IDE

    m_IsBadXMLDocNode = false;

    m_XMLDocFlagsAndNodesIndex.Collapse();

#if IDE 
    m_pXMLDocRoot = NULL;

    UnLockNode();
#endif IDE
}

#if IDE 

/*****************************************************************************
;GetGenericNodeText

Returns the text for a generic XML tag. This is used for getting the summary,
remarks, or whatever is needed out of the XML doc comment node.
*****************************************************************************/
HRESULT XMLDocNode::GetGenericNodeText(IXMLDOMDocument *pDoc,  BSTR *pbstrText, _In_z_ const WCHAR * wszNodeType)
{
    HRESULT hr = NO_ERROR;
    CComPtr<IXMLDOMElement> spRootElement;
    CComPtr<IXMLDOMNode> spNode;

    VerifyInPtr(pDoc);

    hr = pDoc->get_documentElement(&spRootElement);

    if (hr == S_OK)   // documentElement can return S_FALSE, so don't use SUCCEEDED macro 
    					// (which succeeds on S_FALSE and any positive int) devdiv 147526
    {
        hr = spRootElement->selectSingleNode(CComBSTR(wszNodeType), &spNode);

        if (hr== S_OK  && spNode)
        {
            hr = spNode->get_text(pbstrText);

            if (hr == S_OK)
            {
                // Remove any extra white spaces from the string before returning.
                RemoveExtraWhiteSpacesFromString(*pbstrText);
            }
        }
    }
    return hr;
}

void XMLDocNode::RenderGenericNodeText(BSTR *pbstrText, _In_z_ const WCHAR * wszNodeType)
{
    AssertIfNull(pbstrText);
    AssertIfNull(wszNodeType);

    EnsureLightWeightXMLDocParseTreeLoaded();

    StringBuffer sbText;

    if (m_pXMLDocRoot &&
        m_pXMLDocRoot->m_pFirstChild &&
        m_pXMLDocRoot->m_pFirstChild->IsStartTag() &&
        m_pXMLDocRoot->m_pFirstChild->AsStartTag()->NameMatches(XMLDoc_Member))
    {
        XMLElement *pElement = m_pXMLDocRoot->m_pFirstChild->AsStartTag()->m_pFirstChild;
        STRING *pstrTagName = GetCompiler()->AddString(wszNodeType);

        // Loop over all tags and try to find a valid tag to use as a banner (see rules above).
        while (pElement)
        {
            if (pElement->m_type == XET_StartTag)
            {
                XMLStartTag *pStartTag = pElement->AsStartTag();

                if (pStartTag->NameMatches(pstrTagName))
                {
                    CXMLDocParser parser(GetCompiler(), m_pAllocatorForXml);
                    RenderGenericNodeText(&parser, pStartTag, &sbText);

                    break;
                }
            }

            pElement = pElement->m_pNext;
        }
    }

    RemoveExtraWhiteSpacesFromString(sbText.GetString());
    *pbstrText = ::SysAllocString(sbText.GetString());
}


void XMLDocNode::RenderGenericNodeText(CXMLDocParser *pXMLDocParser, XMLStartTag *pStartTag, StringBuffer *psbText)
{
    XMLElement *pElement = pStartTag->m_pFirstChild;

    while (pElement)
    {
        if (pElement->IsCharData())
        {
            const WCHAR *wszCharData = pElement->AsCharData()->m_wszString;
            if (wszCharData)
            {
                // Render the char data, taking escape sequences into account
                while (wszCharData[0] != UCH_NULL)
                {
                    bool fEscapeSequence = false;

                    if (wszCharData[0] == L'&')
                    {
                        for (int i = 0; i < DIM(s_rgXMLCharEscapeData); i++)
                        {
                            if (CompareCaseN(wszCharData, s_rgXMLCharEscapeData[i].wszEscapeSequence, s_rgXMLCharEscapeData[i].cchEscapeSequence) == 0)
                            {
                                psbText->AppendChar(s_rgXMLCharEscapeData[i].wch);

                                wszCharData += s_rgXMLCharEscapeData[i].cchEscapeSequence;
                                fEscapeSequence = true;
                                break;
                            }
                        }
                    }

                    if (!fEscapeSequence)
                    {
                        psbText->AppendChar(wszCharData[0]);
                        wszCharData++;
                    }
                }
            }
        }
        else if (pElement->IsStartTag() || pElement->IsEmptyTag())
        {
            XMLBaseTag *pBaseTag = pElement->AsTag();

            const WCHAR *wszValue = NULL;

            if (pBaseTag->NameMatches(XMLDoc_See) || pBaseTag->NameMatches(XMLDoc_SeeAlso))
            {
                pXMLDocParser->ParseAttributes(pBaseTag);

                wszValue = pElement->IsStartTag() ? pBaseTag->AsStartTag()->GetInteriorText() : NULL;

                if (! wszValue || ! wcslen(wszValue))
                {
                    wszValue = pBaseTag->GetAttributeValue(XMLDoc_CRef);

                    if (!wszValue)
                    {
                        wszValue = pBaseTag->GetAttributeValue(XMLDoc_LangWord);
                    }

                    if (wszValue)
                    {
                        psbText->AppendString(wszValue);
                    }
                }
            }
            else if (pBaseTag->NameMatches(XMLDoc_ParamRef) || pBaseTag->NameMatches(XMLDoc_TypeParamRef))
            {
                pXMLDocParser->ParseAttributes(pBaseTag);

                wszValue = pElement->IsStartTag() ? pBaseTag->AsStartTag()->GetInteriorText() : NULL;

                if (! wszValue || !wcslen(wszValue))
                {
                    wszValue = pBaseTag->GetAttributeValue(XMLDoc_Name);

                    if (wszValue)
                    {
                        psbText->AppendString(wszValue);
                    }
                }
            }

            if (pElement->IsStartTag())
            {
                RenderGenericNodeText(pXMLDocParser, pElement->AsStartTag(), psbText);
            }
        }

        pElement = pElement->m_pNext;
    }
}

/*****************************************************************************
;GetSummaryText

Returns the "summary" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "summary" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetSummaryText(BSTR *pbstrSummaryText)
{
    if (pbstrSummaryText)
    {
        *pbstrSummaryText = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    RenderGenericNodeText(pbstrSummaryText, XMLDoc_Summary);

    UnLockNode();

    return S_OK;
}

/*****************************************************************************
;GetParamCount

Returns the number of parameters defined on this XML doc node. This has to
match the same function on IVsXMLMemberData, which is used for getting the
number of "param" tags for an XML element defined in Metadata reference.

*****************************************************************************/
HRESULT XMLDocNode::GetParamCount(long *pParams)
{
    return GetParamCountHelper(m_pArrayOfParameters, pParams);
}

/*****************************************************************************
;GetParamTextAt
*****************************************************************************/
HRESULT XMLDocNode::GetParamTextAt(long iParam, BSTR *pbstrName, BSTR *pbstrText)
{
    return GetParamTextAtHelper(m_pArrayOfParameters, iParam, pbstrName, pbstrText);
}

/*****************************************************************************
;GetExceptionCount

Returns the number of exceptions defined on this XML doc node. This has to
match the same function on IVsXMLMemberData, which is used for getting the
number of "exception" tags for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetExceptionCount(long *pParams)
{
    return GetParamCountHelper(m_pArrayOfExceptions, pParams);
}

/*****************************************************************************
;GetExceptionTextAt

*****************************************************************************/
HRESULT XMLDocNode::GetExceptionTextAt(long iParam, BSTR *pbstrType, BSTR *pbstrText)
{
    return GetParamTextAtHelper(m_pArrayOfExceptions, iParam, pbstrType, pbstrText);
}

/*****************************************************************************
;GetTypeParamCount

Returns the number of generic parameters defined on this XML doc node. This has to
match the same function on IVsXMLMemberData, which is used for getting the
number of "typeparam" tags for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetTypeParamCount(long *pParams)
{
    return GetParamCountHelper(m_pArrayOfGenericParameters, pParams);
}

/*****************************************************************************
;GetTypeParamTextAt

*****************************************************************************/
HRESULT XMLDocNode::GetTypeParamTextAt(long iParam, BSTR *pbstrName, BSTR *pbstrText)
{
    return GetParamTextAtHelper(m_pArrayOfGenericParameters, iParam, pbstrName, pbstrText);
}

// Helper to do the real work.
HRESULT XMLDocNode::GetParamCountHelper(DynamicArray<ParamInfo> *pdaParamArray, long *pParams)
{
    AssertIfNull(pdaParamArray);

    HRESULT hr = NO_ERROR;

    if (!pParams)
    {
        VSFAIL("Error: E_POINTER");
        hr = E_POINTER;
    }
    else
    {
        LockNode();

        CComPtr<IXMLDOMDocument> spDoc;
        BuildXMLDocCommentNode(false, &spDoc);

        if (spDoc)
        {
            if (!m_IsBadXMLDocNode)
            {
                *pParams = pdaParamArray->Count();
            }
        }
        else
        {
            hr = E_FAIL;
        }

        UnLockNode();
    }

    return hr;
}

// Helper to do the real work.
HRESULT XMLDocNode::GetParamTextAtHelper(DynamicArray<ParamInfo> *pdaParamArray, long iParam, BSTR *pbstrName, BSTR *pbstrText)
{
    HRESULT hr = S_OK;

    if (iParam < 0)
    {
        VSFAIL("Error: E_UNEXPECTED");
        return E_UNEXPECTED;
    }

    if (!pbstrText || !pbstrName)
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    CComPtr<IXMLDOMDocument> spDoc;
    BuildXMLDocCommentNode(false, &spDoc);

    if (spDoc)
    {
        if (!m_IsBadXMLDocNode && (ULONG)iParam < pdaParamArray->Count())
        {
            *pbstrName = SysAllocString(pdaParamArray->Element(iParam).m_ParamName);
            *pbstrText = SysAllocString(pdaParamArray->Element(iParam).m_ParamText);

            // Remove any extra white spaces from the string before returning.
            RemoveExtraWhiteSpacesFromString(*pbstrText);
            hr = NO_ERROR;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    UnLockNode();

    return hr;
}

/*****************************************************************************
;GetAssociatedCapabilities

Returns the associated capabilities defined on this XML doc node. This has to
match the same function on IVsXMLMemberData4, which is used for getting the
required capabilities for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetAssociatedCapabilities(DynamicArray<CapabilityInfo> &daCapabilities)
{
    XMLDocNodeLockHolder lockHolder(this);

    EnsureLightWeightXMLDocParseTreeLoaded();

    if (!m_pXMLDocRoot ||
        !m_pXMLDocRoot->m_pFirstChild ||
        !m_pXMLDocRoot->m_pFirstChild->IsStartTag() ||
        !m_pXMLDocRoot->m_pFirstChild->AsStartTag()->NameMatches(XMLDoc_Member))
    {
        return E_FAIL;
    }

    // Scan the children looking for capability tags
    for (XMLElement *pElement = m_pXMLDocRoot->m_pFirstChild->AsStartTag()->m_pFirstChild;
         pElement;
         pElement = pElement->m_pNext)
    {
        if (pElement->m_type == XET_StartTag)
        {
            XMLStartTag *pStartTag = pElement->AsStartTag();

            if (pStartTag->NameMatches(XMLDoc_Capability))
            {
                CXMLDocParser parser(GetCompiler(), m_pAllocatorForXml);
                parser.ParseAttributes(pStartTag);

                const WCHAR *wszTypeValue = pStartTag->GetAttributeValue(XMLDoc_Type);
                if (wszTypeValue != NULL)
                {
                    StringBuffer sbText;

                    RenderGenericNodeText(&parser, pStartTag, &sbText);
                    RemoveExtraWhiteSpacesFromString(sbText.GetString());

                    // Using Add to get an instance of CapabilityInfo to fill (to avoid extra copying)
                    CapabilityInfo &info = daCapabilities.Add();
                    info.m_bstrType = wszTypeValue;
                    info.m_bstrDescription = sbText.GetString();
                }
            }
        }
    }

    return S_OK;
}

/*****************************************************************************
;GetReturnsText

Returns the "returns" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "returns" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetReturnsText(BSTR *pbstrReturnsText)
{
    if (pbstrReturnsText)
    {
        *pbstrReturnsText = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    RenderGenericNodeText(pbstrReturnsText, XMLDoc_Returns);

    UnLockNode();

    return S_OK;
}

/*****************************************************************************
;GetRemarksText

Returns the "remarks" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "remarks" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetRemarksText(BSTR *pbstrRemarksText)
{
    if (pbstrRemarksText)
    {
        *pbstrRemarksText = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    RenderGenericNodeText(pbstrRemarksText, XMLDoc_Remarks);

    UnLockNode();

    return S_OK;
}

/*****************************************************************************
;GetFilterPriority

Returns the filter priority associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the filter priority for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetFilterPriority(long *piFilterPriority)
{
    HRESULT hr = NO_ERROR;

    if (!piFilterPriority)
    {
        VSFAIL("Error: E_POINTER");
        hr = E_POINTER;
    }
    else
    {

        LockNode();

        CComPtr<IXMLDOMDocument> spDoc;
        BuildXMLDocCommentNode(false, &spDoc);

        if (spDoc)
        {
            if (!m_IsBadXMLDocNode)
            {
                CComBSTR bstrFilterPriority;
                hr = GetGenericNodeText(spDoc, &bstrFilterPriority, XMLDoc_FilterPriority);

                if (hr == S_OK)
                {
                    // Convert the filter priority to a numerical value
                    *piFilterPriority = _wtol(bstrFilterPriority);

                    if (*piFilterPriority == 0)
                    {
                        hr = S_FALSE;
                    }
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }

        UnLockNode();
    }

    return hr;
}

/*****************************************************************************
;GetCompletionListText

Returns the "completionlist" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "completionlist" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetCompletionListText(BSTR *pbstrCompletionListText)
{
    HRESULT hr = E_FAIL;

    if (pbstrCompletionListText)
    {
        *pbstrCompletionListText = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    CComPtr<IXMLDOMDocument> spDoc;
    BuildXMLDocCommentNode(false, &spDoc);

    if (spDoc)
    {
        CComPtr<IXMLDOMElement> pRootElement;
        spDoc->get_documentElement(&pRootElement);

        if (pRootElement)
        {
            if (!m_IsBadXMLDocNode)
            {
                hr = NO_ERROR;
                CComPtr<IXMLDOMNode> spNode;

                // Get the completion list node
                hr = pRootElement->selectSingleNode(CComBSTR(XMLDoc_CompletionList), &spNode);
                if (SUCCEEDED(hr) && spNode)
                {
                    // Get the cref attribute
                    CComPtr<IXMLDOMNode> spCRefNode;
                    hr = spNode->selectSingleNode(CComBSTR(L"./@" XMLDoc_CRef), &spCRefNode);

                    if (SUCCEEDED(hr) && spCRefNode)
                    {
                        CComBSTR bstrText;
                        hr = spCRefNode->get_text(&bstrText);
                        if (SUCCEEDED(hr))
                        {
                            // Chop off the prefix if it exists and there is a character after it
                            if (bstrText.Length() > 2 && bstrText[1] == L':')
                            {
                                *pbstrCompletionListText = ::SysAllocStringLen(bstrText + 2, bstrText.Length() - 2);
                            }
                            else
                            {
                                *pbstrCompletionListText = bstrText.Detach();
                            }
                        }
                    }
                }
            }
        }
    }

    UnLockNode();

    return hr;
}

/*****************************************************************************
;GetCompletionListTextAt

Returns the "completionlist" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "completionlist" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetCompletionListTextAt(long iParam, BSTR *pbstrCompletionListText)
{
    HRESULT hr = E_FAIL;

    if (pbstrCompletionListText)
    {
        *pbstrCompletionListText = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    CComPtr<IXMLDOMDocument> spDoc;
    CComPtr<IXMLDOMElement> spRootElement;

    BuildXMLDocCommentNode(false, &spDoc);

    if (spDoc)
    {
        spDoc->get_documentElement(&spRootElement);
        if (spRootElement)
        {
            if (!m_IsBadXMLDocNode)
            {
                hr = NO_ERROR;
                CComPtr<IXMLDOMNodeList> spNodeList;

                hr = spRootElement->selectNodes(CComBSTR(L"./" XMLDoc_Param), &spNodeList);
                if (SUCCEEDED(hr) && spNodeList)
                {
                    long lListLength = 0;
                    hr = spNodeList->get_length(&lListLength);
                    if (SUCCEEDED(hr) && iParam < lListLength)
                    {
                        CComPtr<IXMLDOMNode> spParamNode;
                        hr = spNodeList->get_item(iParam, &spParamNode);
                        if (SUCCEEDED(hr) && spParamNode)
                        {
                            // Get the completion list attribute
                            CComPtr<IXMLDOMNode> spCompletionListNode;
                            hr = spParamNode->selectSingleNode(CComBSTR(L"./@" XMLDoc_CompletionList), &spCompletionListNode);

                            if (SUCCEEDED(hr) && spCompletionListNode)
                            {
                                CComBSTR bstrText;
                                hr = spCompletionListNode->get_text(&bstrText);
                                if (SUCCEEDED(hr))
                                {
                                    // Chop off the prefix if it exists and there is a character after it
                                    if (bstrText.Length() > 2 && bstrText[1] == L':')
                                    {
                                        *pbstrCompletionListText = ::SysAllocStringLen(bstrText + 2, bstrText.Length() - 2);
                                    }
                                    else
                                    {
                                        *pbstrCompletionListText = bstrText.Detach();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    UnLockNode();
    return hr;
}

/*****************************************************************************
;GetPermissionSet

Returns the "permissionset" text associated with this node (If any). This has to
match the same function on IVsXMLMemberData, which is used for getting
the "permissionset" for an XML element defined in Metadata reference.
*****************************************************************************/
HRESULT XMLDocNode::GetPermissionSet(BSTR *pbstrPermissionSetXML)
{
    HRESULT hr = E_FAIL;

    if (pbstrPermissionSetXML)
    {
        *pbstrPermissionSetXML = NULL;
    }
    else
    {
        VSFAIL("Error: E_POINTER");
        return E_POINTER;
    }

    LockNode();

    CComPtr<IXMLDOMDocument> spDoc;

    BuildXMLDocCommentNode(false, &spDoc);

    if (spDoc)
    {
        CComPtr<IXMLDOMElement> spRootElement;

        spDoc->get_documentElement(&spRootElement);

        if (spRootElement)
        {
            if (!m_IsBadXMLDocNode)
            {
                CComPtr<IXMLDOMNode> spNode;
                hr = spRootElement->selectSingleNode(CComBSTR(XMLDoc_PermissionSet), &spNode);

                if (SUCCEEDED(hr) && spNode)
                {
                    // Return the raw xml of the node
                    hr = spNode->get_xml(pbstrPermissionSetXML);
                }
            }
        }
    }

    UnLockNode();

    return hr;
}

/*****************************************************************************
;EnsureLightWeightXMLDocParseTreeLoaded

Loads the CXMLDocParser for this node if it isn't already loaded.
*****************************************************************************/
void XMLDocNode::EnsureLightWeightXMLDocParseTreeLoaded()
{
    VSASSERT(m_pAllocatorForXml, "Allocator must not be NULL");

    if (!m_pXMLDocRoot)
    {
        NorlsAllocator nrlsTemp(NORLSLOC);
        ParseTree::CommentBlockStatement *pCommentBlock = GetXMLDocCommentBlock(&nrlsTemp);

        if (pCommentBlock)
        {
            XMLCommentStream *pStream = new (zeromemory) XMLCommentStream(pCommentBlock, true);

            CComQIPtr<ISequentialStream> pSeqStream(pStream);

            StringBuffer buffer(pSeqStream);
            pStream->Clear();
            CXMLDocParser XMLParser(GetCompiler(), m_pAllocatorForXml);
            XMLParser.ParseXMLDoc(buffer.GetString(), buffer.GetStringLength(), &m_pXMLDocRoot);
            VSASSERT(m_pXMLDocRoot, "How come parsing XML failed?");
        }
    }
}

/*****************************************************************************
;FindRootXMLDocNodeFromIndex

Finds a root XMLDoc node given the index of the node in the root nodes list.
*****************************************************************************/
XMLElement *XMLDocNode::FindRootXMLDocNodeFromIndex(XMLStartTag *pXMLNode, long RootNodeIndex)
{
    VSASSERT(pXMLNode, "Bad params");

    long Index = -1;
    XMLElement *pXMLNodeChild = pXMLNode->m_pFirstChild;

    while (pXMLNodeChild)
    {
        // We only care about start tags.
        if (pXMLNodeChild->m_type == XET_StartTag || pXMLNodeChild->m_type == XET_EmptyTag)
        {
            ++Index;

            // If this is the index we are looking for, go ahead and get out of the loop.
            if (Index == RootNodeIndex)
            {
                break;
            }
        }

        pXMLNodeChild = pXMLNodeChild->m_pNext;
    }

    return pXMLNodeChild;
}

/*****************************************************************************
;FindMostEncapsulatingXMLDocNode

Tries to find the most encapsulating XMLDoc node which contains a spesific
location.
*****************************************************************************/
XMLBaseNode *XMLDocNode::FindMostEncapsulatingXMLDocNode(XMLStartTag *pXMLNode, long ErrorPosition, XMLBaseTag **ppXMLNode)
{
    VSASSERT(pXMLNode && ppXMLNode, "Bad params");
    VSASSERT(ErrorPosition >= pXMLNode->m_lBlockStart && ErrorPosition <= pXMLNode->m_lBlockEnd,
        "Error position should be within XML block");

    XMLElement *pXMLNodeChild = pXMLNode->m_pFirstChild;

    while (pXMLNodeChild)
    {
        switch (pXMLNodeChild->m_type)
        {
        case XET_CharData :
        case XET_EmptyTag :
        case XET_UnterminatedStartTag :
            if (pXMLNodeChild->IsWithin(ErrorPosition))
            {
                return pXMLNodeChild;
            }

            break;

        case XET_StartTag :
            {
                XMLStartTag *pStartTag = pXMLNodeChild->AsStartTag();

                // Error is in the Start tag
                if (pStartTag->IsWithin(ErrorPosition))
                {
                    return pStartTag;
                }

                // Error is inside the body of a tag, do a recursive call on the this tag.
                if (pStartTag->IsWithinChildBlock(ErrorPosition, false))
                {
                    return FindMostEncapsulatingXMLDocNode(pStartTag, ErrorPosition, ppXMLNode);
                }
            }

            break;

        case XET_EndTag :
            {
                // Error is in the End tag
                if (pXMLNodeChild->IsWithin(ErrorPosition))
                {
                    XMLEndTag *pEndTag = pXMLNodeChild->AsEndTag();

                    if (pEndTag && pEndTag->m_pStartTag && pEndTag->m_pStartTag->m_pLastChild)
                    {
                        switch (pEndTag->m_pStartTag->m_pLastChild->m_type)
                        {
                        case XET_StartTag :
                            *ppXMLNode = pEndTag->m_pStartTag->m_pLastChild->AsStartTag();
                            break;

                        case XET_UnterminatedStartTag :
                            *ppXMLNode = pEndTag->m_pStartTag->m_pLastChild->AsUnterminatedStartTag();
                            break;

                        default :
                            *ppXMLNode = NULL;
                            break;
                        }

                        return pXMLNodeChild;
                    }
                }
            }

            break;

        default:
            VSFAIL("Unknown XML Node type");
            break;
        }

        pXMLNodeChild = pXMLNodeChild->m_pNext;
    }

    return pXMLNode;
}

/*****************************************************************************
;GetXMLDocCommentBlock

Retrieves the XMLDoc String based on the location.
*****************************************************************************/
ParseTree::CommentBlockStatement *XMLDocNode::GetXMLDocCommentBlock
(
    NorlsAllocator *pAllocator
)
{
    ParseTree::FileBlockStatement *pParseTreeDecls = NULL;
    ParseTree::StatementList **ppCachedCommentLocation = NULL;

    XMLParseTreeCache *pCache = NULL;
    if ( !XMLParseTreeCache::TryGetCurrent(&pCache) )
    {
        // We're not caching the values so build them here
        Text SourceText;
        SourceText.Init(GetSourceFile());

        ::GetDeclTrees(
            GetCompiler(),
            &SourceText,
            GetSourceFile(),
            pAllocator,
            pAllocator,
            NULL,
            NULL,
            NULL,
            &pParseTreeDecls);
    }
    else
    {
        pParseTreeDecls =  m_pParentXMLDocFile->GetOrCreateCachedDeclTrees(pCache);
        ppCachedCommentLocation = pCache->GetCachedCommentLocation();
    }

    if (pParseTreeDecls)
    {
        ParseTree::Statement *pStatement = ParseTreeHelpers::GetStatementContainingLocation(pParseTreeDecls, &m_CommentTextSpan, true, ppCachedCommentLocation);

// this assert will fire in DevDiv 147526: the parsetree is stale.
//VSASSERT(pStatement && pStatement->Opcode == 
//        ParseTree::Statement::CommentBlock && pStatement->AsCommentBlock()->IsXMLDocComment, 
//        "The identified parse tree is not of the correct type (it should be a comment). Chances are that the location search is operating over stale data.");

        if (pStatement && pStatement->Opcode == ParseTree::Statement::CommentBlock && pStatement->AsCommentBlock()->IsXMLDocComment)
        {
            return pStatement->AsCommentBlock();
        }
    }

    return NULL;
}

/*****************************************************************************
;CalculateRealSpan

Given an offset within the XML buffer and an XMLDoc node, this function will
calculate the real span of the error. This function returns a new suggested
error in pSubstituteErrorID if it discovers that it can generate a better
error message than the default one.
*****************************************************************************/
Location XMLDocNode::CalculateRealSpan(long ErrorPosition, ERRID *pSubstituteErrorID, _Deref_opt_out_z_ WCHAR **pExtraText,
                                       XMLBaseNode *pXMLDocNodeContainingError, XMLBaseTag *pXMLNode)
{
    long AccumilatedOffset = VBMath::Convert<long>(XMLCommentStream::s_SkipCount);    // Start off with the size of the <member> tag.

    long ErrorBeginOffset = VBMath::Convert<long>(pXMLDocNodeContainingError->m_lStart);
    long ErrorEndOffset   = VBMath::Convert<long>(pXMLDocNodeContainingError->m_lEnd);

    Location NewErrorLocation;
    CXMLDocLineMapper LineMapper;

    // Get the XMLDoc block.
    NorlsAllocator nrlsTemp(NORLSLOC);
    ParseTree::CommentBlockStatement *pCommentBlock = GetXMLDocCommentBlock(&nrlsTemp);

    // If for some reason we can't get it, we will resort to using the whole span for the block.
    if (!pCommentBlock)
    {
        VSFAIL("XMLDoc comment block is NULL");
        return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
    }

    VSASSERT(pCommentBlock && pCommentBlock->IsXMLDocComment && pCommentBlock->Children, "Bad XMLDoc comment block");

    ParseTree::StatementList *pXMLCommentsList = pCommentBlock->Children;

    // Loop over all comment satements in this comment block and add a mapping for each to the linemapper.
    while (pXMLCommentsList)
    {
        ParseTree::Statement *pEmptyStatementForXMLDocComment = pXMLCommentsList->Element;
        VSASSERT(pEmptyStatementForXMLDocComment && pEmptyStatementForXMLDocComment->Opcode == ParseTree::Statement::Empty, "Empty statement is NULL!");

        ParseTree::CommentList *pXMLCommentsListPerStatement = pEmptyStatementForXMLDocComment->Comments;
        VSASSERT(pXMLCommentsListPerStatement, "XMLDoc Comment List is empty!");

        // Loop over all comments, usually there is only one, but we may support more than one per statement.
        while (pXMLCommentsListPerStatement)
        {
            ParseTree::Comment *pComment = pXMLCommentsListPerStatement->Element;
            VSASSERT(pComment && pComment->IsGeneratedFromXMLDocToken, "Bad XMLDoc comment!");

            long CommentStart = pComment->TextSpan.m_lBegColumn + g_tkKwdNameLengths[tkXMLDocComment];
            LineMapper.AddLineMapping(AccumilatedOffset, CommentStart);

            // Add the width of the comment text and the CRLF
            AccumilatedOffset += pComment->TextSpan.m_lEndColumn - CommentStart + 1 + CCH_CRLF;

            pXMLCommentsListPerStatement = pXMLCommentsListPerStatement->Next;
        }

        pXMLCommentsList = pXMLCommentsList->NextInBlock;
    }

    if (pXMLNode && ErrorPosition > AccumilatedOffset)
    {
        // If we find out that the offset we are looking for is outside the comment block span, the we know that the error is reported
        // on the </member> tag, so instead, we use the previous tag to report a the error on.
        LineMapper.GetLineIndexOfPosition(VBMath::Convert<long>(pXMLNode->m_lStart), 
                                          &NewErrorLocation.m_lBegLine, 
                                          &NewErrorLocation.m_lBegColumn);

        LineMapper.GetLineIndexOfPosition(VBMath::Convert<long>(pXMLNode->m_lEnd), 
                                          &NewErrorLocation.m_lEndLine, 
                                          &NewErrorLocation.m_lEndColumn);

        *pSubstituteErrorID = WRNID_XMLDocStartTagWithNoEndTag;
        *pExtraText = pXMLNode->m_name.m_pstrName;
    }
    else
    {
        if ((ULONG)ErrorEndOffset < XMLCommentStream::s_SkipCount)
        {
            ErrorBeginOffset = ErrorEndOffset = XMLCommentStream::s_SkipCount;
        }

        LineMapper.GetLineIndexOfPosition(ErrorBeginOffset, &(NewErrorLocation.m_lBegLine), &(NewErrorLocation.m_lBegColumn));
        LineMapper.GetLineIndexOfPosition(ErrorEndOffset, &(NewErrorLocation.m_lEndLine), &(NewErrorLocation.m_lEndColumn));
    }

#pragma warning (push)
#pragma warning (disable:6001) // Location is initialized above
    // Adjust by the line offsets.
    NewErrorLocation.m_lBegLine += m_CommentTextSpan.m_lBegLine;
    NewErrorLocation.m_lEndLine += m_CommentTextSpan.m_lBegLine;
#pragma warning (pop)

    return NewErrorLocation;
}

#endif IDE

/*****************************************************************************
;GetNodeIndex

Returns the node index for a given node type (identified by Flag)
*****************************************************************************/
long XMLDocNode::GetNodeIndex(XMLDOCFLAGS Flag)
{
    for (ULONG i = 0; i < m_XMLDocFlagsAndNodesIndex.Count(); ++i)
    {
        if (m_XMLDocFlagsAndNodesIndex.Element(i).m_Flag == Flag)
        {
            return m_XMLDocFlagsAndNodesIndex.Element(i).m_NodeIndex;
        }
    }

    VSFAIL("XMLDoc Node was not found");
    return -1;      // -1 indicates that the node wasn't found.
}

/*****************************************************************************
;GetXMLDocErrorLocationFromNodeIndex

Returns the location for an XMLDoc node with an error giving a node index
in the root of the XMLDoc tree.
*****************************************************************************/
Location XMLDocNode::GetXMLDocErrorLocationFromNodeIndex(long NodeIndex)
{
#if IDE 
    EnsureLightWeightXMLDocParseTreeLoaded();

    if (m_pXMLDocRoot && NodeIndex >= 0)
    {
        VSASSERT(m_pXMLDocRoot->m_pFirstChild && m_pXMLDocRoot->m_pFirstChild->IsStartTag(), "Bad XML root node");
        XMLElement *pXMLDocNodeContainingError = FindRootXMLDocNodeFromIndex(m_pXMLDocRoot->m_pFirstChild->AsStartTag(), NodeIndex);

        if (pXMLDocNodeContainingError)
        {
            // We just take the mid point as the point of error.
            long ErrorPosition = VBMath::Convert<long>((pXMLDocNodeContainingError->m_lStart + pXMLDocNodeContainingError->m_lEnd) / 2);
            return CalculateRealSpan(ErrorPosition, NULL, NULL, pXMLDocNodeContainingError, NULL);
        }
        else
        {
            VSFAIL("Expected a valid XMLBaseNode");
        }
    }

    return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);

#else
    return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
#endif IDE
}

/*****************************************************************************
;GetXMLDocErrorLocationFromNodeIndex

Returns the location for an XMLDoc node with an error giving a node list of
indexes starting from the root the XMLDoc tree.
*****************************************************************************/
Location XMLDocNode::GetXMLDocErrorLocationFromNodeIndex(Stack<long> *pIndexStack)
{
#if IDE 
    EnsureLightWeightXMLDocParseTreeLoaded();

    if (m_pXMLDocRoot)
    {
        VSASSERT(m_pXMLDocRoot->m_pFirstChild && m_pXMLDocRoot->m_pFirstChild->IsStartTag(), "Bad XML root node");
        XMLStartTag *pXMLNode = m_pXMLDocRoot->m_pFirstChild->AsStartTag();
        XMLElement *pXMLDocNodeContainingError = NULL;

        // Walk the stack of indexes looking for the right nodes.
        for (ULONG i = 0; i < pIndexStack->Count(); ++i)
        {
            long Index = pIndexStack->Element(i);

            pXMLDocNodeContainingError = FindRootXMLDocNodeFromIndex(pXMLNode, Index);

            if (!pXMLDocNodeContainingError)
            {
                VSFAIL("Expected a valid XMLBaseNode");
                return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
            }
            else
            {
                if (pXMLDocNodeContainingError->IsStartTag())
                {
                    pXMLNode = pXMLDocNodeContainingError->AsStartTag();
                }
                else
                {
                    VSASSERT(pXMLDocNodeContainingError->IsEmptyTag(), "Must be an Empty tag");
                    break;
                }
            }
        }

        // We just take the mid point as the point of error.
        long ErrorPosition = VBMath::Convert<long>((pXMLDocNodeContainingError->m_lStart + pXMLDocNodeContainingError->m_lEnd) / 2);
        return CalculateRealSpan(ErrorPosition, NULL, NULL, pXMLDocNodeContainingError, NULL);
    }
    else
    {
        return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
    }

#else
    return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
#endif IDE
}

/*****************************************************************************
;GetXMLDocErrorLocation

Returns the error location for an XMLDoc
*****************************************************************************/
Location XMLDocNode::GetXMLDocErrorLocation(long ErrorPosition, ERRID *pSubstituteErrorID, _Deref_out_z_ WCHAR **pExtraText)
{

#if IDE 
    VSASSERT(pSubstituteErrorID && pExtraText, "Bad input");

    EnsureLightWeightXMLDocParseTreeLoaded();

    if (m_pXMLDocRoot)
    {
        XMLBaseTag  *pXMLNode = NULL;
        XMLBaseNode *pXMLDocNodeContainingError = FindMostEncapsulatingXMLDocNode(m_pXMLDocRoot, ErrorPosition, &pXMLNode);

        return CalculateRealSpan(ErrorPosition, pSubstituteErrorID, pExtraText, pXMLDocNodeContainingError, pXMLNode);
    }
    else
    {
        return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
    }

#else
    return GetDefaultXMLDocErrorLocation(m_CommentTextSpan);
#endif IDE
}

/*****************************************************************************
;LoadXMLDocString

Uses the MSXML parser to parse the XML string passed in and reports any
parse errors to the user if reportErrors is True
*****************************************************************************/
void XMLDocNode::LoadXMLDocString(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL isSuccessful = VARIANT_FALSE;

    // The XML document that parses XMLDoc comments.
    CComPtr<IXMLDOMDocument> spXMLDocument;

    // Initialize and create the document object
    hr = CreateXMLDocument(IID_IXMLDOMDocument, (void **) &spXMLDocument);

    if (SUCCEEDED(hr))
    {
        hr  = spXMLDocument->put_async(VARIANT_FALSE);

        if (SUCCEEDED((hr)))
        {
#if IDE 
            NorlsAllocator nrlsTemp(NORLSLOC);
            ParseTree::CommentBlockStatement *pCommentBlock = GetXMLDocCommentBlock(&nrlsTemp);

            if (pCommentBlock)
            {
                // Create a stream to read the XML from the parse trees.
                XMLCommentStream *pStream = new (zeromemory) XMLCommentStream(pCommentBlock);

                CComQIPtr<IUnknown, &IID_IUnknown> pUnk(pStream);
                CComVariant varStream(pUnk);

                // Load the XML text into the parser
                hr = spXMLDocument->load(varStream, &isSuccessful);

                // Release the stream.
                pStream->Clear();
                varStream.Clear();

                pUnk.Release();
#else
                hr = spXMLDocument->loadXML(m_pXMLDocString->GetString(), &isSuccessful);
#endif

                if (FAILED(hr) || isSuccessful == VARIANT_FALSE)
                {
                    // XML parsing failed, get the error message from the XML parser and report it
                    // to the user.
                    CComPtr<IXMLDOMParseError>   spXMLDOMDocError;

                    if(spXMLDocument->get_parseError(&spXMLDOMDocError) == S_OK)
                    {
                        CComBSTR bstrError;

                        if(spXMLDOMDocError->get_reason(&bstrError) == S_OK)
                        {
                            long ErrorPosition = 0;

                            if (spXMLDOMDocError->get_filepos(&ErrorPosition) == S_OK)
                            {
                                StripTrailingBlanksAndLineBreaks(bstrError, bstrError.Length());

                                ERRID SubstituteErrorID = 0;
                                WCHAR *ExtraText = NULL;


                                // Get the actual error location.

#if IDE 
                                // The light weight XML parser does not understand the unicode BOM
                                // so it is given a stream that does not contain it. This means that we have to give
                                // GetXMLDocErrorLocation an error possition offset to the left by the length of the missing text
                                // (otherwise errors will be reported in the wrong locations).
                                ErrorPosition -= XMLCommentStream::s_BomCount;
#endif
                                Location ErrorLocation = GetXMLDocErrorLocation(ErrorPosition, &SubstituteErrorID, &ExtraText);

                                if (SubstituteErrorID == 0)
                                {
                                    GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocParseError1, &ErrorLocation, bstrError);
                                }
                                else
                                {
                                    // If there is a substitute error, reprot the subtituted error instead.
                                    GetSourceFile()->GetErrorTable()->CreateError(SubstituteErrorID, &ErrorLocation, ExtraText);
                                }
                            }
                            else
                            {
                                VSFAIL("Unable to get XML parse error");
                            }
                        }
                        else
                        {
                            VSFAIL("Unable to get XML parse error");
                        }
                    }
                    else
                    {
                        VSFAIL("Unable to get XML parse error");
                    }

                    spXMLDocument = NULL;
                }
#if IDE 
            }
            else
            {
                if (reportErrors)
                {
                    GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocParseError1, &m_CommentTextSpan, L"");
                }
            }
#endif
        }
    }

    VSASSERT(!ppOutput || ! *ppOutput, "The provided pointer pointer points to a non null pointer");

    if (ppOutput && !*ppOutput)
    {
        *ppOutput = spXMLDocument.Detach();
    }

}

/*****************************************************************************
;LoadXMLFile

Loads and parses an XML file. This is used when resolving ?include? tags.
*****************************************************************************/
void XMLDocNode::LoadXMLFile(BSTR FileName, _Inout_opt_ IXMLDOMDocument **ppOutput)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL isSuccessful;

    // The XML document that parses XMLDoc comments.
    CComPtr<IXMLDOMDocument> spXMLDocument;

    // Initialize and create the document object
    hr = CreateXMLDocument(IID_IXMLDOMDocument, (void **) &spXMLDocument);

    if (SUCCEEDED((hr)))
    {
        hr  = spXMLDocument->put_async(VARIANT_FALSE);

        if (SUCCEEDED((hr)))
        {
            CComVariant InputFileName = FileName;

            // Load the XML text into the parser
            hr = spXMLDocument->load(InputFileName, &isSuccessful);

            if (FAILED(hr) || isSuccessful == VARIANT_FALSE)
            {
                spXMLDocument = NULL;
            }
        }
    }

    VSASSERT(!ppOutput || !*ppOutput, "The provided pointer pointer points to a non null pointer");

    if (ppOutput && !*ppOutput)
    {
        *ppOutput = spXMLDocument.Detach();
    }
}

/*****************************************************************************
;GetTagFlag

Given a sting representation of an XML tag, this function returns the
bit-field associated with that tag. This is the inverse of StringFromXMLDOCFlags().
*****************************************************************************/
XMLDOCFLAGS XMLDocNode::GetTagFlag(_In_z_ WCHAR *pTagName)
{
    if (TagMatchesCase(pTagName, XMLDoc_Summary))
    {
        return TAG_SUMMARY;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Remarks))
    {
        return TAG_REMARKS;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Returns))
    {
        return TAG_RETURNS;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Param))
    {
        return TAG_PARAM;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_TypeParam))
    {
        return TAG_TYPEPARAM;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Exception))
    {
        return TAG_EXCEPTION;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_See))
    {
        return TAG_SEE;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_SeeAlso))
    {
        return TAG_SEEALSO;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Value))
    {
        return TAG_VALUE;
    }
    else if (TagMatchesCase(pTagName, XMLDoc_Include))
    {
        return TAG_INCLUDE;
    }

    return TAG_NONE;
}

/*****************************************************************************
;StringFromXMLDOCFlags

Returns a string representation of an bit-field that represents all known
XML doc tags. Note that is function looks ?slow?, but in reality, only the
first eight tags are used, so we don?t go all the way down to the bottom
of the function. This is done for completion. This is the inverse of GetTagFlag().
*****************************************************************************/
WCHAR *XMLDocNode::StringFromXMLDOCFlags(XMLDOCFLAGS AllKnownTags)
{
    if (AllKnownTags == TAG_NONE)
    {
        return NULL;
    }
    else if (AllKnownTags & TAG_SUMMARY)
    {
        return XMLDoc_Summary;
    }
    else if(AllKnownTags & TAG_PARAM)
    {
        return XMLDoc_Param;
    }
    else if(AllKnownTags & TAG_PARAMREF)
    {
        return XMLDoc_ParamRef;
    }
    else if(AllKnownTags & TAG_VALUE)
    {
        return XMLDoc_Value;
    }
    else if(AllKnownTags & TAG_INCLUDE)
    {
        return XMLDoc_Include;
    }
    else if(AllKnownTags & TAG_RETURNS)
    {
        return XMLDoc_Returns;
    }
    else if (AllKnownTags & TAG_TYPEPARAM)
    {
        return XMLDoc_TypeParam;
    }
    else if(AllKnownTags & TAG_REMARKS)
    {
        return XMLDoc_Remarks;
    }
    else if(AllKnownTags & TAG_PERMISSION)
    {
        return XMLDoc_Permission;
    }
    else if(AllKnownTags & TAG_SEE)
    {
        return XMLDoc_See;
    }
    else if(AllKnownTags & TAG_SEEALSO)
    {
        return XMLDoc_SeeAlso;
    }
    else if(AllKnownTags & TAG_EXAMPLE)
    {
        return XMLDoc_Example;
    }
    else if(AllKnownTags & TAG_EXCEPTION)
    {
        return XMLDoc_Exception;
    }
    else if(AllKnownTags & TAG_C)
    {
        return XMLDoc_C;
    }
    else if(AllKnownTags & TAG_CODE)
    {
        return XMLDoc_Code;
    }
    else if(AllKnownTags & TAG_LIST)
    {
        return XMLDoc_List;
    }
    else if(AllKnownTags & TAG_PARA)
    {
        return XMLDoc_Para;
    }
    else
    {
        return NULL;
    }
}

/*****************************************************************************
;GenerateIllegalTagErrors

Generates one error for each illegal XML tag on this node.
*****************************************************************************/
void XMLDocNode::GenerateIllegalTagErrors(XMLDOCFLAGS ElementFlags)
{
    ElementFlags = m_UsedKnownTags & ElementFlags;
    XMLDOCFLAGS Flag = 0x00000001;

    while (Flag <= TAG_LAST_TAG)
    {
        XMLDOCFLAGS BadFlag = Flag & ElementFlags;

        if (BadFlag != 0)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocIllegalTagOnElement2,
                &GetXMLDocErrorLocationFromNodeIndex(GetNodeIndex(BadFlag)),
                StringFromXMLDOCFlags(BadFlag),
                GetStringOfSymbol());
        }

        Flag = Flag << 1;
    }
}

/*****************************************************************************
;VerifyNoIllegalTags

Verifies that there are no tags that are used in the wrong context. For
example, we want to generate an error if an XML doc comment has a <param>
tag but the comment block applies to a type. Errors are reported here as
appropriate.
*****************************************************************************/
bool XMLDocNode::VerifyNoIllegalTags(bool reportErrors)
{
    bool IsValid = true;

    switch (m_pOwningSymbol->GetKind())
    {
    case SYM_Class :
        if (m_pOwningSymbol->IsDelegate())
        {
            if (m_UsedKnownTags & BCSYM_Delegate_Disallowed_TAGS)
            {
                if (reportErrors)
                {
                    GenerateIllegalTagErrors(BCSYM_Delegate_Disallowed_TAGS);
                }
                IsValid = false;
            }

            if ((m_UsedKnownTags & TAG_RETURNS) && !GetDelegateInvokeSymbol()->GetType())
            {
                if (reportErrors)
                {
                    GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocIllegalTagOnElement2,
                        &GetXMLDocErrorLocationFromNodeIndex(GetNodeIndex(TAG_RETURNS)),
                        StringFromXMLDOCFlags(TAG_RETURNS),
                        WIDE("delegate sub"));
                }

                IsValid = false;
            }
        }
        else
        {
            XMLDOCFLAGS Flags = BCSYM_Class_Disallowed_TAGS;

            // Disallow generic param for enums and modules.
            if (m_pOwningSymbol->IsEnum() || m_pOwningSymbol->PClass()->IsStdModule())
            {
                Flags |= TAG_TYPEPARAM;
            }

            if (m_UsedKnownTags & Flags)
            {
                if (reportErrors)
                {
                    GenerateIllegalTagErrors(Flags);
                }
                IsValid = false;
            }
        }
        break;

    case SYM_Interface :
        if (m_UsedKnownTags & BCSYM_Interface_Disallowed_TAGS)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(BCSYM_Interface_Disallowed_TAGS);
            }
            IsValid = false;
        }
        break;

    case SYM_EventDecl :
        if (m_UsedKnownTags & BCSYM_Event_Disallowed_TAGS)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(BCSYM_Event_Disallowed_TAGS);
            }
            IsValid = false;
        }
        break;

    case SYM_DllDeclare :
    case SYM_UserDefinedOperator :
        if (m_UsedKnownTags & TAG_TYPEPARAM)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(TAG_TYPEPARAM);
            }
            IsValid = false;
        }

        __fallthrough; // Fall through

    case SYM_MethodImpl :
    case SYM_MethodDecl :
        if (m_UsedKnownTags & BCSYM_Proc_Disallowed_TAGS)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(BCSYM_Proc_Disallowed_TAGS);
            }
            IsValid = false;
        }

        // If this is a Sub but there is a <returns> tag, then generate an error.
        if ((m_UsedKnownTags & TAG_RETURNS) && !m_pOwningSymbol->PProc()->GetType())
        {
            if (reportErrors)
            {
                // Dll declare are special because they may or may not have a return value, depending on the usage of Sub|function in
                // the Dll declare statement.
                if (m_pOwningSymbol->GetKind() == SYM_DllDeclare)
                {
                    GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocReturnsOnADeclareSub,
                        &GetXMLDocErrorLocationFromNodeIndex(GetNodeIndex(TAG_RETURNS)));
                }
                else
                {
                    GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocIllegalTagOnElement2,
                        &GetXMLDocErrorLocationFromNodeIndex(GetNodeIndex(TAG_RETURNS)),
                        StringFromXMLDOCFlags(m_UsedKnownTags & TAG_RETURNS),
                        GetStringOfSymbol());
                }
            }

            IsValid = false;
        }
        break;

    case SYM_Property :
        if (m_UsedKnownTags & BCSYM_Property_Disallowed_TAGS)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(BCSYM_Property_Disallowed_TAGS);
            }
            IsValid = false;
        }

        // <returns> tags is invalid on WriteOnly properties.
        if ((m_UsedKnownTags & TAG_RETURNS) && m_pOwningSymbol->PProperty()->IsWriteOnly())
        {
            if (reportErrors)
            {
                GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocReturnsOnWriteOnlyProperty,
                    &GetXMLDocErrorLocationFromNodeIndex(GetNodeIndex(TAG_RETURNS)));
            }

            IsValid = false;
        }
        break;

    case SYM_Variable :
    case SYM_VariableWithValue :
    case SYM_VariableWithArraySizes :
        if (m_UsedKnownTags & BCSYM_Variable_Disallowed_TAGS)
        {
            if (reportErrors)
            {
                GenerateIllegalTagErrors(BCSYM_Variable_Disallowed_TAGS);
            }
            IsValid = false;
        }

        break;

    default:
        VSFAIL("Unknown symbol type.");
    }

    return IsValid;
}

/*****************************************************************************
;CompareAttributes

Compares attributes defined on an XML doc comment tag. The idea is that if
there are two tags with the same tag name and the same set of attributes,
then an error should be generated.
*****************************************************************************/
bool XMLDocNode::CompareAttributes(IXMLDOMNode *pFirstElement, IXMLDOMNode *pSecondElement)
{
    // Compare attributes here.
    CComPtr<IXMLDOMNamedNodeMap> spFirstAttributeMap;
    CComPtr<IXMLDOMNamedNodeMap> spSecondAttributeMap;

    IfFailThrow(pFirstElement->get_attributes(&spFirstAttributeMap));
    IfFailThrow(pSecondElement->get_attributes(&spSecondAttributeMap));

    if (!spFirstAttributeMap || !spSecondAttributeMap)
    {
        return true;
    }

    long FirstListLength  = 0;
    long SecondListLength = 0;

    IfFailThrow(spFirstAttributeMap->get_length(&FirstListLength));
    IfFailThrow(spSecondAttributeMap->get_length(&SecondListLength));

    if (FirstListLength != SecondListLength)
    {
        return false;        // they can't be the same if the length is different
    }

    bool Same = true;

    for (long i = 0; Same && i < FirstListLength; ++i)
    {
        CComPtr<IXMLDOMNode> spAttributeNode;
        IfFailThrow(spFirstAttributeMap->get_item(i, &spAttributeNode));

        BSTR AttributeName;
        IfFailThrow(spAttributeNode->get_nodeName(&AttributeName));

        CComPtr<IXMLDOMNode> spAttributeNodeInSecondList;

        IfFailThrow(spSecondAttributeMap->getNamedItem(AttributeName, &spAttributeNodeInSecondList));

        if (spAttributeNodeInSecondList)
        {
            // Attribute with the same name exists, check the value of the attribute as well
            BSTR FirstAttributeValue, SecondAttributeValue;

            IfFailThrow(spAttributeNode->get_text(&FirstAttributeValue));
            IfFailThrow(spAttributeNodeInSecondList->get_text(&SecondAttributeValue));

            if (!TagMatchesCase(FirstAttributeValue, SecondAttributeValue))
            {
                Same = false;
            }

            SysFreeString(FirstAttributeValue);
            SysFreeString(SecondAttributeValue);
        }
        else
        {
            Same = false;
        }

        SysFreeString(AttributeName);
    }

    return Same;
}


typedef int (__cdecl bsearch_comparison_function)(const void *, const void *);

int compareStrings(const wchar_t * const * ppStr1, const wchar_t * const * ppStr2)
{
    return wcscmp(*ppStr1, *ppStr2);
}

bool XMLDocNode::IsUniqueTag(const wchar_t * pTagName)
{
    return BSearchEx<const wchar_t *, const wchar_t *>((const wchar_t * const *)&pTagName, m_uniqueWellKnownTags, m_uniqueWellKnownTagCount, compareStrings, NULL);
}


/*****************************************************************************
;VerifyXMLDocRootChildren

Verifies that no two tags look exactly the same and also that parameters
are uses correctly with respect to the symbol associated with this XML doc comment.
*****************************************************************************/
bool XMLDocNode::VerifyXMLDocRootChildren(IXMLDOMElement *pXMLRootElement, bool reportErrors)
{
#if IDE 
    m_pArrayOfParameters->Destroy();
    m_pArrayOfGenericParameters->Destroy();
    m_pArrayOfExceptions->Destroy();
#endif IDE

    if (!pXMLRootElement)
    {
        return false;
    }

    HRESULT hr = S_OK;

    CComPtr<IXMLDOMNode> spXMLChildNode;
    CComPtr<IXMLDOMNodeList> spChildrenList;

    IfFailThrow(pXMLRootElement->get_childNodes(&spChildrenList));

    if (!spChildrenList)
    {
        return true;
    }

    DynamicArray<IXMLDOMNode *> ChildrenArray;

    spChildrenList->nextNode(&spXMLChildNode);     // Get the first child node

    // We add all root children to an array so we can compare each pair of tags together.
    while (spXMLChildNode)
    {
        BSTR NodeText;
        IfFailThrow(spXMLChildNode->get_nodeName(&NodeText));

        if (CompareCase(NodeText, XMLDoc_Text) != 0)
        {
            ChildrenArray.Add() = spXMLChildNode.Detach();
        }

        SysFreeString(NodeText);
        spXMLChildNode = NULL;
        spChildrenList->nextNode(&spXMLChildNode);
    }

    bool VerificationSucceeded = true;
    bool IsSameParameter = false;

    for (unsigned i = 0; i < ChildrenArray.Count(); ++i)
    {
        BSTR FirstText;
        ChildrenArray.Element(i)->get_nodeName(&FirstText);

        bool Same = false;

        for (unsigned j = i + 1; !Same && j < ChildrenArray.Count(); ++j)
        {
            BSTR SecondText;
            IfFailThrow(ChildrenArray.Element(j)->get_nodeName(&SecondText));

            // "#text" is special because it is generated by the XML parser every time this XML has the following:
            // <summary>Blah</summary>abc
            // <remarks>Blah 2</remarks>
            // Notice how the ?abc? part is not inside any XML tags, so the XML parser generates a ?#text? tag for it.
            // We don?t want to get hung on these tags because there may be more than one of them around.
            //
            // Microsoft
            // 12/12/2005
            // see VSWhidbey bug # 566346
            // we need to relax this error for <exception> tags, because it is legal to have multiple exception
            // tags with the same attributes.
            //
            //
            // Microsoft
            // see Dev Div Bugs # 4389
            // we need to relax the error further, because we allow custom tags to be present, so we now
            // only generate this error for "well known unique" xml comment tags.

            if (TagMatchesCase(FirstText, SecondText) &&  IsUniqueTag(SecondText))
            {
                // For events, it is legal to have two or more paramters with the same name,
                // so we have to allow this. (e.g. Event E1(ByVal x As Integer, ByVal x As Double))
                if (!m_pOwningSymbol->IsEventDecl() || CompareCase(FirstText, XMLDoc_Param) != 0)
                {
                    // Make sure they are not exactly the same.
                    Same = CompareAttributes(ChildrenArray.Element(i), ChildrenArray.Element(j));

                    if (Same)
                    {
                        IsSameParameter = true;
                        if (reportErrors)
                        {
                            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocDuplicateXMLNode1, &GetXMLDocErrorLocationFromNodeIndex(j), FirstText);
                        }
                    }
                }
            }

            SysFreeString(SecondText);
        }

        XMLDOCFLAGS NewTagFlag = GetTagFlag(FirstText);
        m_UsedKnownTags |= NewTagFlag;

        XMLDocFlagsAndNodesIndex NewArrayElement;

        NewArrayElement.m_Flag = NewTagFlag;
        NewArrayElement.m_NodeIndex = i;
        m_XMLDocFlagsAndNodesIndex.AddElement(NewArrayElement);

        SysFreeString(FirstText);

        if (!Same)
        {
            switch (NewTagFlag)
            {
            case TAG_PARAM:
                if (!VerifyParameter(ChildrenArray.Element(i), i, reportErrors))
                {
                    VerificationSucceeded = false;
                }
                break;

            case TAG_TYPEPARAM:
                if (!VerifyGenericParameter(ChildrenArray.Element(i), i, reportErrors))
                {
                    VerificationSucceeded = false;
                }
                break;

            case TAG_EXCEPTION:
                if (!VerifyException(ChildrenArray.Element(i), i, reportErrors))
                {
                    VerificationSucceeded = false;
                }
                break;
            }
        }

        RELEASE(ChildrenArray.Element(i));
    }

    return (VerificationSucceeded && !IsSameParameter);
}

/*****************************************************************************
;GetStringOfSymbol

Returns a string representing the name of the type of sybmol
associated with this node.
*****************************************************************************/
STRING *XMLDocNode::GetStringOfSymbol()
{
    if (m_pOwningSymbol->IsDelegate())
    {
        return GetCompiler()->AddString(WIDE("delegate"));
    }
    else
    {
        return StringOfSymbol(GetCompiler(), m_pOwningSymbol);
    }
}

/*****************************************************************************
;GetDelegateInvokeSymbol

Retruns the Delegate Invoke sybmol.
*****************************************************************************/
BCSYM_Proc *XMLDocNode::GetDelegateInvokeSymbol()
{
    VSASSERT(m_pOwningSymbol->IsDelegate(), "Has to be a Delegate Sybmol");

    if (!m_pInvokeProcForDelegates)
    {
        BCSYM_NamedRoot *pInvokeMethod = GetInvokeFromDelegate(m_pOwningSymbol, GetCompiler());

        VSASSERT(pInvokeMethod && pInvokeMethod->IsProc(), "Invoke must be a proc");
        m_pInvokeProcForDelegates = pInvokeMethod->PProc();
    }

    return m_pInvokeProcForDelegates;
}

/*****************************************************************************
;VerifyIncludeFileAndPath

Verifies that an ?include? tag is well formed. In particular, we verify that
there is a ?file? and a ?path? attribute on the tag. We will not validate the
contents of these tags since this is only done when the XML file is generated.
*****************************************************************************/
bool XMLDocNode::VerifyIncludeFileAndPath(IXMLDOMNode *pIncludeNode, Stack<long> *pIndexStack, bool reportErrors)
{
    VSASSERT(pIncludeNode, "Bad param");

    if (pIncludeNode)
    {

        CComPtr<IXMLDOMNode> spFileAttributeNode;     // Get the "file" attribute.
        CComPtr<IXMLDOMNode> spPathAttributeNode;     // Get the "path" attribute.

        if (FAILED(pIncludeNode->selectSingleNode((BSTR)L"@file", &spFileAttributeNode)) ||
            FAILED(pIncludeNode->selectSingleNode((BSTR)L"@path", &spPathAttributeNode)))
        {
            return false;
        }

        BSTR FileAttributeValue = NULL;
        BSTR PathAttributeValue = NULL;

        if (spFileAttributeNode && spPathAttributeNode)
        {
            IfFailThrow(spFileAttributeNode->get_text(&FileAttributeValue));
            IfFailThrow(spPathAttributeNode->get_text(&PathAttributeValue));

            if (FileAttributeValue && PathAttributeValue)
            {
                return true;
            }

            return false;
        }

        // If one or both attributes are missing, report the error(s)
        if (!spFileAttributeNode && reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLMissingFileOrPathAttribute1, &GetXMLDocErrorLocationFromNodeIndex(pIndexStack), XMLDoc_File);
        }

        if (!spPathAttributeNode && reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLMissingFileOrPathAttribute1, &GetXMLDocErrorLocationFromNodeIndex(pIndexStack), XMLDoc_Path);
        }

        SysFreeString(FileAttributeValue);
        SysFreeString(PathAttributeValue);
    }

    return false;
}

/*****************************************************************************
;VerifyAllIncludeTags

Verifies all include tags applied to one XML doc node.
*****************************************************************************/
bool XMLDocNode::VerifyAllIncludeTags(IXMLDOMElement * pRootElement, bool reportErrors)
{
    bool Succeeded = true;
    VB_ENTRY();
    if (!m_IsBadXMLDocNode)
    {
        CComQIPtr<IXMLDOMNode> spRootNode(pRootElement);
        // This stack keeps track of the index path to the node so we can get to it later.
        Stack<long> *pIndexStack = new Stack<long>();

        if (!spRootNode || !RecursivelyVerifyIncludeNodes(spRootNode, pIndexStack, reportErrors))
        {
            Succeeded = false;
        }

        VSASSERT(pIndexStack->Count() == 0, "Index stack should be empty");

        delete pIndexStack;
    }
    VB_EXIT_NORETURN();

    return Succeeded;
}

/*****************************************************************************
;VerifyException

Verifies one generic parameter node. Verification means that we need to make sure
that the XML doc param corresponds to an actual generic parameter on the type or proc.
*****************************************************************************/
bool XMLDocNode::VerifyException(IXMLDOMNode *pParameterNode, long NodeIndex, bool reportErrors)
{
    bool Success = true;

    if (!m_pOwningSymbol->IsProc() && !m_pOwningSymbol->IsDelegate())
    {
        // We can't verify parameters on any thing other than a Proc or a Property.
        return false;
    }

    CComPtr<IXMLDOMNode> spParamNameNode;         // First, try to find the "name" attribute

    // Microsoft 11/18/01:  IXMLRootElement's use of BSTR is incorrect -- although the interface
    // requires a BSTR, they are actually expecting a WCHAR*.  To keep PREFast from choking
    // on this, we'll add the cast, which programmatically does nothing.
    if (FAILED(pParameterNode->selectSingleNode((BSTR)L"@cref", &spParamNameNode)))
    {
        return false;           // If there is not "name" attribute, fail.
    }

    BSTR AttributeValue = NULL;

    if (spParamNameNode)
    {
        IfFailThrow(spParamNameNode->get_text(&AttributeValue));
    }

    if (AttributeValue)
    {
#if IDE 
        // If found, then cache this information so we can use it later on when we are asked about param info.
        m_pArrayOfExceptions->Grow();
        if (SysStringLen(AttributeValue) > 2 && AttributeValue[0] == L'T' && AttributeValue[1] == L':')
        {
            m_pArrayOfExceptions->Element(m_pArrayOfExceptions->Count() - 1).m_ParamName =
                SysAllocString(AttributeValue + 2);

            SysFreeString(AttributeValue);
        }
        else
        {
            m_pArrayOfExceptions->Element(m_pArrayOfExceptions->Count() - 1).m_ParamName = AttributeValue;
        }

        StringBuffer buffer;
        PrefixTraversal(pParameterNode, NodeTextProcessor(&buffer), AlwaysTrue<IXMLDOMNode *>());
        CComBSTR bstrParamText(buffer.GetString());
        m_pArrayOfExceptions->Element(m_pArrayOfExceptions->Count() - 1).m_ParamText = bstrParamText.Detach();
#endif IDE
    }
    else
    {
        if (reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocExceptionTagWithoutCRef, &GetXMLDocErrorLocationFromNodeIndex(NodeIndex));
        }
        Success = false;
    }

    return Success;
}

/*****************************************************************************
;VerifyGenericParameter

Verifies one generic parameter node. Verification means that we need to make sure
that the XML doc param corresponds to an actual generic parameter on the type or proc.
*****************************************************************************/
bool XMLDocNode::VerifyGenericParameter(IXMLDOMNode *pParameterNode, long NodeIndex,bool reportErrors)
{
    bool Success = true;
    BCSYM_GenericParam *pGenericParameter = m_pOwningSymbol->GetFirstGenericParam();

    if (!m_pOwningSymbol->IsProc() &&
        !m_pOwningSymbol->IsDelegate() &&
        !m_pOwningSymbol->IsInterface() &&
        !IsClassOrStructOnly(m_pOwningSymbol))
    {
        // We can't verify generic parameters on any thing other than a Proc or a Type.
        return false;
    }

    CComPtr<IXMLDOMNode> spParamNameNode;         // First, try to find the "name" attribute

    // Microsoft 11/18/01:  IXMLRootElement's use of BSTR is incorrect -- although the interface
    // requires a BSTR, they are actually expecting a WCHAR*.  To keep PREFast from choking
    // on this, we'll add the cast, which programmatically does nothing.
    if (FAILED(pParameterNode->selectSingleNode((BSTR)L"@name", &spParamNameNode)))
    {
        return false;           // If there is not "name" attribute, fail.
    }

    BSTR AttributeValue = NULL;

    if (spParamNameNode)
    {
        IfFailThrow(spParamNameNode->get_text(&AttributeValue));
    }

    bool FoundTheRightParameter = false;

    if (AttributeValue)
    {
        // Then, try to match that param with all parameters declared on the proc
        while (pGenericParameter && !FoundTheRightParameter)
        {
            if (CompareNoCase(AttributeValue, pGenericParameter->GetName()) == 0)
            {
                FoundTheRightParameter = true;

#if IDE 
                // If found, then cache this information so we can use it later on when we are asked about param info.
                m_pArrayOfGenericParameters->Grow();
                m_pArrayOfGenericParameters->Element(m_pArrayOfGenericParameters->Count() - 1).m_ParamName = AttributeValue;

                BSTR ParamText = NULL;
                IfFailThrow(pParameterNode->get_text(&ParamText));
                m_pArrayOfGenericParameters->Element(m_pArrayOfGenericParameters->Count() - 1).m_ParamText = ParamText;
#endif IDE
            }
            else
            {
                pGenericParameter = pGenericParameter->GetNextParam();
            }
        }

        if (!FoundTheRightParameter)
        {
            if (reportErrors)
            {
                GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocBadGenericParamTag2,
                    &GetXMLDocErrorLocationFromNodeIndex(NodeIndex),
                    AttributeValue,
                    GetStringOfSymbol());
            }

            Success = false;
            SysFreeString(AttributeValue);
        }
    }
    else
    {
        if (reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocGenericParamTagWithoutName, &GetXMLDocErrorLocationFromNodeIndex(NodeIndex));
        }
        Success = false;
    }

    return Success;
}


/*****************************************************************************
;VerifyParameter

Verifies one parameter node. Verification means that we need to make sure
that the XML doc param corresponds to an actual parameter on the proc.
*****************************************************************************/
bool XMLDocNode::VerifyParameter(IXMLDOMNode *pParameterNode, long NodeIndex, bool reportErrors)
{
    bool Success = true;
    BCSYM_Param *pParameter = NULL;

    if (m_pOwningSymbol->IsProc() && !m_pOwningSymbol->IsSyntheticMethod())
    {
        pParameter = m_pOwningSymbol->PProc()->GetFirstParam();
    }
    else
    {
        if (m_pOwningSymbol->IsDelegate())
        {
            pParameter = GetDelegateInvokeSymbol()->GetFirstParam();
        }
        else
        {
            // We can't verify parameters on any thing other than a Proc, Property, or Event.
            return Success;
        }
    }

    CComPtr<IXMLDOMNode> spParamNameNode;         // First, try to find the "name" attribute

    // Microsoft 11/18/01:  IXMLRootElement's use of BSTR is incorrect -- although the interface
    // requires a BSTR, they are actually expecting a WCHAR*.  To keep PREFast from choking
    // on this, we'll add the cast, which programmatically does nothing.
    if (FAILED(pParameterNode->selectSingleNode((BSTR)L"@name", &spParamNameNode)))
    {
        return false;           // If there is not "name" attribute, fail.
    }

    BSTR AttributeValue = NULL;

    if (spParamNameNode)
    {
        IfFailThrow(spParamNameNode->get_text(&AttributeValue));
    }

    bool FoundTheRightParameter = false;

    if (AttributeValue)
    {
        // Then, try to match that param with all parameters declared on the proc
        while (pParameter && !FoundTheRightParameter)
        {
            if (CompareNoCase(AttributeValue, pParameter->GetName()) == 0)
            {
                FoundTheRightParameter = true;

#if IDE 

                // If found, then cache this information so we can use it later on when we are asked about param info.
                m_pArrayOfParameters->Grow();
                m_pArrayOfParameters->Element(m_pArrayOfParameters->Count() - 1).m_ParamName = AttributeValue;

                StringBuffer buffer;
                PrefixTraversal(pParameterNode, NodeTextProcessor(&buffer), AlwaysTrue<IXMLDOMNode *>());
                CComBSTR bstrParamText(buffer.GetString());
                m_pArrayOfParameters->Element(m_pArrayOfParameters->Count() - 1).m_ParamText = bstrParamText.Detach();
#endif IDE
            }
            else
            {
                pParameter = pParameter->GetNext();
            }
        }

        if (!FoundTheRightParameter)
        {
            if (reportErrors)
            {
                GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocBadParamTag2,
                    &GetXMLDocErrorLocationFromNodeIndex(NodeIndex),
                    AttributeValue,
                    GetStringOfSymbol());
            }

            Success = false;
            SysFreeString(AttributeValue);
        }
    }
    else
    {
        if (reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocParamTagWithoutName, &GetXMLDocErrorLocationFromNodeIndex(NodeIndex));
        }
        Success = false;
    }

    return Success;
}

/*****************************************************************************
;VerifyXMLDocAgainstSymbol

Verifies that the parsed XML is valid for this particular code element (symbol).
Returns true if verification passes and false if not.
*****************************************************************************/
bool XMLDocNode::VerifyXMLDocAgainstSymbol(IXMLDOMElement *pXMLRootElement, bool reportErrors)
{
    bool Success = true;

    if (!VerifyXMLDocRootChildren(pXMLRootElement,reportErrors))
    {
        Success = false;
    }

    if ((Success || reportErrors) && !VerifyNoIllegalTags(reportErrors))
    {
        Success = false;
    }

    if ((Success || reportErrors) && !VerifyAllIncludeTags(pXMLRootElement, reportErrors))
    {
        Success = false;
    }

    return Success;
}

/*****************************************************************************
;PerformActualBinding

Bind a cref attibute to the element it refers to. This function will only
fail if we can't store the resolved or unresolved name in the XML tree.
*****************************************************************************/
HRESULT XMLDocNode::PerformActualBinding(IXMLDOMNode  *pCrefAttribute, Stack<long> *pIndexStack, bool reportErrors)
{
    if (GetSourceFile()->GetCompState() < CS_Bound)
    {
        return NO_ERROR;
    }

    CComVariant varCrefValue;

    pCrefAttribute->get_nodeValue(&varCrefValue);
    VSASSERT(varCrefValue.vt == VT_BSTR, "A BSTR was expected!");

    NorlsAllocator Allocator(NORLSLOC);
    Parser Parser(
        &Allocator,
        GetCompiler(),
        GetSourceFile()->GetCompilerHost(),
        false,
        GetSourceFile()->GetProject()->GetCompilingLanguageVersion());

    Location DummyLocation = {0};
    DummyLocation.Invalidate();
    bool ParseErrors = false;
    bool IsBadName = false;
    BCSYM_NamedRoot *pBoundSymbol = NULL;

    ParseTree::Name *pParseTreeName = Parser.ParseName(
        &DummyLocation,
        varCrefValue.bstrVal,
        SysStringLen(varCrefValue.bstrVal),
        &ParseErrors,
        true, /* AllowGlobalNameSpace */
        true /* AllowGenericArguments */ );

    if (pParseTreeName && !ParseErrors)
    {
        BCSYM_Container *pLookup = m_pOwningSymbol->GetContainer();
        CompilationCaches *pCompilationCaches = GetCompilerCompilationCaches();

        pBoundSymbol =
            Semantics::InterpretName
            (
                pParseTreeName,
                pLookup->GetHash(),
                NULL,
                // Dev11 405893:  Don't require type arguments to bind, and allow bases to be any generic arity
                NameSearchEventReference | NameSearchDoNotBindTypeArguments | NameSearchForceUnspecifiedBaseArity,
                NULL,
                NULL,
                GetCompiler(),
                GetSourceFile()->GetCompilerHost(),
                pCompilationCaches, //CompilationCaches
                GetSourceFile(),
                IsBadName,
                NULL,
                &Allocator,
                -1
            );
    }
    else if (CompareNoCase(varCrefValue.bstrVal, GetCompiler()->TokenToString(tkOBJECT)) == 0)
    {
        //Object is a special case
        pBoundSymbol = GetSourceFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(FX::ObjectType);

    }
    else
    {
        //Check to see if we have an instrinsic type
        ParseErrors = false;
        bool TypeIsBad = false;

        ParseTree::Type *pParseTreeType = Parser.ParseTypeName(ParseErrors, varCrefValue.bstrVal);

        if (pParseTreeType && !ParseErrors)
        {
            switch (pParseTreeType->Opcode)
            {
            case ParseTree::Type::Short:
            case ParseTree::Type::UnsignedShort:
            case ParseTree::Type::Integer:
            case ParseTree::Type::UnsignedInteger:
            case ParseTree::Type::Long:
            case ParseTree::Type::UnsignedLong:
            case ParseTree::Type::Decimal:
            case ParseTree::Type::Single:
            case ParseTree::Type::Double:
            case ParseTree::Type::SignedByte:
            case ParseTree::Type::Byte:
            case ParseTree::Type::Boolean:
            case ParseTree::Type::Char:
            case ParseTree::Type::Date:
            case ParseTree::Type::String:

                pBoundSymbol = GetSourceFile()->GetCompilerHost()->GetFXSymbolProvider()->GetType(MapTypeToVType(pParseTreeType->Opcode));
                break;
            }
        }
    }

    pBoundSymbol = pBoundSymbol && pBoundSymbol->IsGenericBinding() ?
        pBoundSymbol->PGenericBinding()->GetGeneric() :
    pBoundSymbol;

    if (pBoundSymbol && !pBoundSymbol->IsGenericBadNamedRoot())
    {
        if (reportErrors)
        {
            return S_OK;
        }
        else
        {
            return StoreResolvedName(pCrefAttribute, pBoundSymbol);
        }
    }
    else
    {
        if (reportErrors)
        {
            GetSourceFile()->GetErrorTable()->CreateError(WRNID_XMLDocCrefAttributeNotFound1, &GetXMLDocErrorLocationFromNodeIndex(pIndexStack), varCrefValue.bstrVal);
            return S_OK;
        }
        else
        {
            return StoreUnResolvedName(pCrefAttribute, varCrefValue.bstrVal);
        }
    }
}

HRESULT XMLDocNode::StoreResolvedName(IXMLDOMNode * pCrefAttribute, BCSYM_NamedRoot *pBoundSymbol)
{
    CComVariant newCrefValue(pBoundSymbol->GetDocCommentSignature(GetCompiler()));

    return pCrefAttribute->put_nodeValue(newCrefValue);
}

HRESULT XMLDocNode::StoreUnResolvedName(IXMLDOMNode * pCrefAttribute, _In_z_ WCHAR *badString)
{
    CComBSTR bstrNewCref;

    bstrNewCref = L"!:";
    bstrNewCref += badString;

    CComVariant varNewCref(bstrNewCref);

    return pCrefAttribute->put_nodeValue(varNewCref);
}

/*****************************************************************************
;VerifyCRefAttributeOnNode

Binds cref a attribute on a node.
*****************************************************************************/
void XMLDocNode::VerifyCRefAttributeOnNode(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors)
{
    if (pXMLNode)
    {
        CComPtr<IXMLDOMNamedNodeMap> spAttributeMap;
        CComBSTR bstrAttribute = L"cref";
        CComPtr<IXMLDOMNode> spCrefNode;

        IfFailThrow(pXMLNode->get_attributes(&spAttributeMap));
        IfFailThrow(spAttributeMap->getNamedItem(bstrAttribute, &spCrefNode));
        if (spCrefNode)
        {
            CComBSTR bstrCrefValue;
            IfFailThrow(spCrefNode->get_text(&bstrCrefValue));

            // If the second character of the crefStr is ':', make no changes to the cref attribute; it's already resolved.
            bool IsAlreadyResolved = (bstrCrefValue.Length() >= 2 && bstrCrefValue[0] != L':' && bstrCrefValue[1] == L':');

            if (!IsAlreadyResolved)
            {
                PerformActualBinding(spCrefNode, pIndexStack, reportErrors);
            }
        }
    }
}

/*****************************************************************************
;RecursivelyVerifyIncludeNodes

Verifies that Include nodes are well formed for this node and any children
nodes.
*****************************************************************************/
bool XMLDocNode::RecursivelyVerifyIncludeNodes(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors)
{
    HRESULT hr = NO_ERROR;
    bool Succeeded = true;

    CComPtr<IXMLDOMNode> spXMLChildNode;
    CComPtr<IXMLDOMNodeList> spChildrenList;
    long Index = 0;

    IfFailThrow(pXMLNode->get_childNodes(&spChildrenList));

    while ((spChildrenList->nextNode(&spXMLChildNode) == S_OK) && spXMLChildNode && (Succeeded || reportErrors))
    {
        DOMNodeType Type;

        // If we can't get the type of the node, then there is something really bad going on.
        if (spXMLChildNode->get_nodeType(&Type) == S_OK && Type == NODE_ELEMENT)
        {
            pIndexStack->Push(Index);
            {
                CComBSTR sbstrNodeName;
                IfFailThrow(spXMLChildNode->get_nodeName(&sbstrNodeName));

                if (CompareNoCase(sbstrNodeName, XMLDoc_Include) == 0)
                {
                    Succeeded = VerifyIncludeFileAndPath(spXMLChildNode, pIndexStack, reportErrors);
                }

                if (Succeeded || reportErrors)
                {
                    RecursivelyVerifyIncludeNodes(spXMLChildNode, pIndexStack, reportErrors);
                }
            }
            pIndexStack->Pop();

            ++Index;
        }

        spXMLChildNode = NULL;
    }

    return Succeeded;
}

/*****************************************************************************
;RecursivelyProcessXMLTree

Binds cref attributes for this node and all its children.
*****************************************************************************/
void XMLDocNode::RecursivelyBindXMLTree(IXMLDOMNode *pXMLNode, Stack<long> *pIndexStack, bool reportErrors)
{
    HRESULT hr = NO_ERROR;

    CComPtr<IXMLDOMNode> spXMLChildNode;
    CComPtr<IXMLDOMNodeList> spChildrenList;
    long Index = 0;

    IfFailThrow(pXMLNode->get_childNodes(&spChildrenList));

    while ((spChildrenList->nextNode(&spXMLChildNode) == S_OK) && spXMLChildNode)
    {
        DOMNodeType Type;

        // If we can't get the type of the node, then there is something really bad going on.
        if (spXMLChildNode->get_nodeType(&Type) == S_OK && Type == NODE_ELEMENT)
        {
            pIndexStack->Push(Index);
            {
                VerifyCRefAttributeOnNode(spXMLChildNode, pIndexStack, reportErrors);
                RecursivelyBindXMLTree(spXMLChildNode, pIndexStack, reportErrors);
            }
            pIndexStack->Pop();

            ++Index;
        }

        spXMLChildNode = NULL;
    }
}

/*****************************************************************************
;BindXMLDomTree

Collects all "cref" attributes and tries to bind all of them to the CodeElements
they refer to.
*****************************************************************************/
void XMLDocNode::BindXMLDomTree(IXMLDOMElement * pRootElement, bool reportErrors)
{
    VB_ENTRY();

    if (!m_IsBadXMLDocNode)
    {
        Stack<long> *pIndexStack = new Stack<long>();

        RecursivelyBindXMLTree(CComQIPtr<IXMLDOMNode>(pRootElement), pIndexStack, reportErrors);
        VSASSERT(pIndexStack->Count() == 0, "Index stack should be empty");

        delete pIndexStack;
    }

    VB_EXIT_NORETURN();
}

/*****************************************************************************
;ReplaceIncludeNode

Replaces the ?include? tag with the actual XML that is included from the
external file. This is done so that when the XML for the whole file is generated,
the included XML is used, not the original ?include? tag.
*****************************************************************************/
void XMLDocNode::ReplaceIncludeNode(IXMLDOMNode *pOriginalNode, IXMLDOMNodeList *pNewIncludeNodeList, IXMLDOMNode *pNewIncludeNode)
{
    HRESULT hr = NO_ERROR;

    CComPtr<IXMLDOMNode> spXMLParentNode;
    IfFailThrow(pOriginalNode->get_parentNode(&spXMLParentNode));

    IfFailThrow(spXMLParentNode->removeChild(pOriginalNode, NULL));

    CComPtr<IXMLDOMNode> spXMLNewNode;
    if (pNewIncludeNodeList)
    {
        hr = pNewIncludeNodeList->nextNode(&spXMLNewNode);

        //Add each of child nodes from the include file to our parent node.
        while (SUCCEEDED(hr) && spXMLNewNode)
        {
            IfFailThrow(spXMLParentNode->appendChild(spXMLNewNode, NULL));

            spXMLNewNode = NULL;
            pNewIncludeNodeList->nextNode(&spXMLNewNode);
        }
    }
    else if (pNewIncludeNode)
    {
        IfFailThrow(spXMLParentNode->appendChild(pNewIncludeNode, NULL));
    }

    CComPtr<IXMLDOMNode> spXMLChildNode;
    CComPtr<IXMLDOMNodeList> spChildrenList;

    // Now walk the list of child elements from the original include node
    // and add these to our parent as well.
    // Child elements of the include tag and the include file are basically folded together
    //in the resulting XML file
    if (SUCCEEDED(pOriginalNode->get_childNodes(&spChildrenList)) && spChildrenList)
    {
        hr = spChildrenList->nextNode(&spXMLChildNode);

        // Make sure children are moved appropriately.
        while (SUCCEEDED(hr) && spXMLChildNode)
        {
            CComPtr<IXMLDOMNode> spXMLRemovedChildNode;

            IfFailThrow(pOriginalNode->removeChild(spXMLChildNode, &spXMLRemovedChildNode));

            CComPtr<IXMLDOMNode> spXMLInsertedChildNode;
            IfFailThrow(spXMLParentNode->appendChild(spXMLRemovedChildNode, &spXMLInsertedChildNode));

            spXMLChildNode = NULL;
            hr = spChildrenList->nextNode(&spXMLChildNode);
        }
    }
}

void XMLDocNode::ResolveIncludeTag(IXMLDOMNode * pIncludeTag)
{
    CComPtr<IXMLDOMNamedNodeMap> spAttributes;
    CComPtr<IXMLDOMNode> spFileName;
    CComPtr<IXMLDOMNode> spPath;

    HRESULT hr = NO_ERROR;

    CComBSTR fileNameValue;
    CComBSTR pathValue;

    if  (
        SUCCEEDED(pIncludeTag->get_attributes(&spAttributes)) &&
        SUCCEEDED(spAttributes->getNamedItem(CComBSTR(L"file"), &spFileName)) &&
        SUCCEEDED(spAttributes->getNamedItem(CComBSTR(L"path"), &spPath)) &&
        SUCCEEDED(spFileName->get_text(&fileNameValue)) &&
        SUCCEEDED(spPath->get_text(&pathValue)) &&
        fileNameValue &&
        pathValue
        )

    {
        // Load up the XML file (we assume it is an XML file).
        CComPtr<IXMLDOMComment> spComment;
        CComPtr<IXMLDOMDocument> spDocumentToInclude;

        LoadXMLFile(fileNameValue, &spDocumentToInclude);

        CComPtr<IXMLDOMNode> spIncludedNode;
        CComPtr<IXMLDOMNodeList> spIncludedNodeList;
        CComBSTR sbstrErrorMessage;

        if (spDocumentToInclude)
        {
            // Try to apply the XPath to the XMLDom tree just created.
            bool fReportError =
                S_OK != spDocumentToInclude->selectNodes(pathValue, &spIncludedNodeList) ||
                !spIncludedNodeList;

            if (!fReportError)
            {
                long lLength = 0;

                fReportError =
                    S_OK != spIncludedNodeList->get_length(&lLength) ||
                    lLength == 0;
            }

            if (fReportError)
            {
                spDocumentToInclude.Release();

                // If we can't apply the XPath, we generate a generic error stating that fact.
                IfFailThrow(CreateXMLDocument(IID_IXMLDOMDocument, (void **) &spDocumentToInclude));

                StringBuffer sbError;

                ResLoadStringRepl(WRNID_XMLDocInvalidXMLFragment,
                    &sbError,
                    pathValue,
                    fileNameValue);

                sbstrErrorMessage = sbError.GetString();

                spDocumentToInclude->createComment(sbstrErrorMessage, &spComment);

                if (spComment)
                {
                    spComment->QueryInterface(IID_IXMLDOMNode, (void **)&spIncludedNode);

                    spIncludedNodeList.Release();
                }
            }
        }
        else
        {
            // If we can't open up the file, spit the error in the XML tree. This needs to be done so that
            // the resulting XML file will have the error embedded as a comment in it.
            IfFailThrow(CreateXMLDocument(IID_IXMLDOMDocument, (void **) &spDocumentToInclude));

            StringBuffer sbError;

            ResLoadStringRepl(WRNID_XMLDocBadFormedXML,
                &sbError,
                fileNameValue);

            sbstrErrorMessage = sbError.GetString();

            spDocumentToInclude->createComment(sbstrErrorMessage, &spComment);

            if (spComment)
            {
                spComment->QueryInterface(IID_IXMLDOMNode, (void **)&spIncludedNode);
            }
        }

        if (spIncludedNodeList || spIncludedNode)
        {
            ReplaceIncludeNode(pIncludeTag, spIncludedNodeList, spIncludedNode);
        }
    }
}

/*****************************************************************************
;ResolveIncludeTags

Resolves the <include> tag to the thingy that it refers to. This means that
we will open up a file, parse it, and apply and XPath query on it to find
the element to include.
*****************************************************************************/
void XMLDocNode::ResolveIncludeTags(IXMLDOMDocument * pDocument)
{
    Stack<IXMLDOMNode *> workList;
    CComPtr<IXMLDOMElement> spElementTemp;
    CComPtr<IXMLDOMNode> spNodeTemp;
    CComPtr<IXMLDOMNode> spChildTemp;
    CComPtr<IXMLDOMNodeList> spNodeListTemp;
    CComBSTR nodeName;

    VB_ENTRY();
    hr = pDocument->get_documentElement(&spElementTemp);

    if (SUCCEEDEDHR(hr) && spElementTemp && SUCCEEDEDHR(spElementTemp->QueryInterface(IID_IXMLDOMNode, (void **)&spNodeTemp)))
    {
        workList.Push(spNodeTemp.Detach());
    }

    spElementTemp.Release();

    while (workList.Count())
    {
        spNodeTemp.Attach(workList.Top());
        workList.Pop();

        if (SUCCEEDEDHR(spNodeTemp->get_childNodes(&spNodeListTemp)))
        {
            spNodeListTemp->nextNode(&spChildTemp);

            while (spChildTemp)
            {
                DOMNodeType nodeType;

                spChildTemp->get_nodeType(&nodeType);

                if (nodeType == NODE_ELEMENT)
                {
                    workList.Push(spChildTemp.Detach());
                }
                else
                {
                    spChildTemp.Release();
                }

                spNodeListTemp->nextNode(&spChildTemp);
            }
        }

        if (SUCCEEDEDHR(spNodeTemp->get_nodeName(&nodeName)) && nodeName == L"include")
        {
            ResolveIncludeTag(spNodeTemp);
        }

        nodeName.Empty();

        spNodeTemp.Release();
        spNodeListTemp.Release();

    }
    VB_EXIT_NORETURN();
}


/*****************************************************************************
;ResolveMemberName

Fill in the name of the <memeber name=""> tag. This is done by setting
the value of the empty name attribute.
*****************************************************************************/
void XMLDocNode::ResolveMemberName(IXMLDOMDocument *pDoc)
{
    VB_ENTRY();
    CComPtr<IXMLDOMElement> spDocElement;
    pDoc->get_documentElement(&spDocElement);

    CComVariant NameAttributeValue(m_pOwningSymbol->GetDocCommentSignature(GetCompiler()));

    IfFailThrow(spDocElement->setAttribute(CComBSTR(XMLDoc_Name), NameAttributeValue));
    VB_EXIT_NORETURN();
}

void XMLDocNode::BuildXMLDocCommentNode(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput)
{
#if IDE 
    LockNode();
#endif

    BuildXMLDomTree(reportErrors, ppOutput);

#if IDE 
    UnLockNode();
#endif

}

HRESULT XMLDocNode::CreateXMLDocument(REFIID riid, void **ppXMLDocument)
{
    HRESULT hr = E_FAIL;

    if (GetCompiler())
    {
        hr =  GetCompiler()->CreateXMLDocument(riid, ppXMLDocument);
    }

    return hr;
}

void XMLDocNode::ProcessXMLDocCommentNode(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput)
{
#if IDE 
    LockNode();
#endif
    CComPtr<IXMLDOMDocument> spDoc;

    BuildXMLDomTree(reportErrors, &spDoc);

    if (spDoc)
    {
        CComPtr<IXMLDOMElement> spRootElement;
        spDoc->get_documentElement(&spRootElement);

        if (spRootElement)
        {
            BindXMLDomTree(spRootElement, reportErrors);
        }
    }

    VSASSERT(!ppOutput || !*ppOutput, "The provided pointer pointer points to a non null pointer");

    if (ppOutput && !*ppOutput)
    {
        *ppOutput = spDoc.Detach();
    }


#if IDE 
    if (reportErrors)
    {
        m_fDidRunValidation = true;
    }

    UnLockNode();
#endif
}


/*****************************************************************************
;BuildXMLDomTree

Builds the XML doc tree from the string of XML generated from the XML doc
comment block.
*****************************************************************************/
void XMLDocNode::BuildXMLDomTree(bool reportErrors, _Inout_opt_ IXMLDOMDocument **ppOutput)
{
    HRESULT hr = NO_ERROR;

    CComPtr<IXMLDOMDocument> spXMLDocument;

    if (!m_IsBadXMLDocNode)
    {
        VB_ENTRY_NOHR();
        LoadXMLDocString(reportErrors, &spXMLDocument); // Load in the XML parser

        if (spXMLDocument)
        {
            // Verify that the XML matches the symbol it is attached to
            CComPtr<IXMLDOMElement> spXMLRootElement;
            if (spXMLDocument->get_documentElement(&spXMLRootElement) != S_OK)  // don't allow S_FALSE
            {
                m_IsBadXMLDocNode = true;
            } 
            else
            {
                if (
                    !(
#if IDE 
                    m_fDidRunValidation ||
#endif
                    m_IsBadXMLDocNode ||
                    VerifyXMLDocAgainstSymbol(spXMLRootElement, reportErrors)
                    )
                   )
                {
                    m_IsBadXMLDocNode = true;
                }
            }
        }
        else
        {
            // The node is bad only if the XML is not parsable.
            m_IsBadXMLDocNode = true;
        }
        VB_EXIT_NORETURN();
    }

    VSASSERT(!ppOutput || ! *ppOutput, "The provided pointer pointer points to a non null pointer");

    if (ppOutput && ! *ppOutput && !m_IsBadXMLDocNode && hr == S_OK)
    {
        *ppOutput = spXMLDocument.Detach();
    }
}

SourceFile *
XMLDocNode::GetSourceFile()
{
    return m_pParentXMLDocFile->GetSourceFile();
}

Compiler *
XMLDocNode::GetCompiler()
{
    return GetSourceFile()->GetCompiler();
}

#if IDE

XMLDocNode::XMLDocNodeLockHolder::XMLDocNodeLockHolder(XMLDocNode *pXMLDocNode)
{
    AssertIfNull(pXMLDocNode);

    m_pXMLDocNode = pXMLDocNode;
    m_pXMLDocNode->LockNode();
}

XMLDocNode::XMLDocNodeLockHolder::~XMLDocNodeLockHolder()
{
    m_pXMLDocNode->UnLockNode();
}

#endif IDE


//////////////////////////////////////////////////////////////////////////////
// End XMLDocNode, begin XMLDocFile.
//////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
;XMLDocFile

*****************************************************************************/
XMLDocFile::XMLDocFile
(
 Compiler *pCompiler,
 SourceFile *pSourceFile
 )
 : m_pSourceFile(pSourceFile)
 , m_pCompiler(pCompiler)
#if IDE 
 , m_pAllocatorForXml(NULL)
#endif
{
}

/*****************************************************************************
;~XMLDocFile

Release all XML doc nodes that we may have built already.
*****************************************************************************/
XMLDocFile::~XMLDocFile()
{
    ReleaseAllXMLDocuments();
}

/*****************************************************************************
;ExtractXMLDocStringFromCommentBlock

Extracts an XML string from the CommentBlock and inserts it into the
StringBuffer passed in.
*****************************************************************************/
void XMLDocFile::ExtractXMLDocStringFromCommentBlock(StringBuffer *pXMLString,
                                                     ParseTree::CommentBlockStatement *pCommentBlock)
{
    VSASSERT(pCommentBlock->IsXMLDocComment, "Why are we trying to parse a non-XMLDoc comment?");
    VSASSERT(pCommentBlock->Children, "XMLDoc comment has no childern!");

    ParseTree::StatementList *pXMLCommentsList = pCommentBlock->Children;

    while(pXMLCommentsList)
    {
        ParseTree::Statement *pEmptyStatementForXMLDocComment = pXMLCommentsList->Element;
        VSASSERT(pEmptyStatementForXMLDocComment && pEmptyStatementForXMLDocComment->Opcode == ParseTree::Statement::Empty, "Empty statement is NULL!");

        ParseTree::CommentList *pXMLCommentsListPerStatement = pEmptyStatementForXMLDocComment->Comments;
        VSASSERT(pXMLCommentsListPerStatement, "XMLDoc Comment List is empty!");

        // Loop over all comments, usually there is only one, but we may support more than one per statement.
        while(pXMLCommentsListPerStatement)
        {
            ParseTree::Comment *pComment = pXMLCommentsListPerStatement->Element;
            VSASSERT(pComment && pComment->IsGeneratedFromXMLDocToken, "Bad XMLDoc comment!");

            if (pComment->LengthInCharacters > 0)
            {
                pXMLString->AppendWithLength(pComment->Spelling, pComment->LengthInCharacters);

                // Preserve CR/LF characters in the comment string (VS51513)
                pXMLString->AppendChar(UCH_CR);
                pXMLString->AppendChar(UCH_LF);
            }

            pXMLCommentsListPerStatement = pXMLCommentsListPerStatement->Next;
        }

        pXMLCommentsList = pXMLCommentsList->NextInBlock;
    }
}

#if IDE 

ParseTree::FileBlockStatement *XMLDocFile::GetOrCreateCachedDeclTrees(XMLParseTreeCache *pCache)
{
    ThrowIfNull(pCache);

    ParseTree::FileBlockStatement* pTree = NULL;

    // First look for it in the cache
    if ( pCache->TryGetParseTree(m_pSourceFile, &pTree) )
    {
        return pTree;
    }

    // Build the tree.  Use the cached Text if available
    ErrorTable Errors(m_pCompiler, m_pSourceFile->GetProject(), NULL);
    Text* pText = NULL; 
    Text builtText;
    if ( !pCache->TryGetText(m_pSourceFile, &pText) )
    {
        // Build the text but do not cache it.  This is stored on the compiler
        // heap and we don't want to be caching it on the XMLParseTreeCache which is
        // actually stored in the Norls heap
        builtText.Init(m_pSourceFile);
        pText = &builtText;
    }

    ::GetDeclTrees(
        m_pCompiler, 
        pText,
        m_pSourceFile, 
        pCache->GetNorlsAllocator(),
        pCache->GetNorlsAllocator(),
        &Errors,
        NULL,
        NULL,
        &pTree);

#if DEBUG
    const WCHAR *wszTextStart = NULL;
    size_t cchText = 0; // our text may possibly be blank: Dev11 86130
    pText->GetText(&wszTextStart, &cchText);
    VSASSERT(pTree || (cchText == 0), "pTree should be valid or we're going to take a perf hit");
#endif
    pCache->SetParseTree(m_pSourceFile, pTree);
    return pTree;
}

/*****************************************************************************
;CollectBannerString

Recursively collects the text of a tag and the text for all its children tags.
This is controlled by the RECURSIVE_BANNER switch, so if we just want the text
for this tag, but none of its children, then we can undefine RECURSIVE_BANNER.

*****************************************************************************/
void XMLDocFile::CollectBannerString(XMLStartTag *pStartTag, StringBuffer *pXMLString, CXMLDocParser *pXMLDocParser)
{
    XMLElement *pNodeChild = pStartTag->m_pFirstChild;

    // Loop over all tags, and dig into children tags.
    while (pNodeChild)
    {
        switch (pNodeChild->m_type)
        {
#ifdef RECURSIVE_BANNER
            case XET_StartTag:
            {
                // Dev10#414367: Omit spaces around <see> and <param> tags.
                auto pTag = pNodeChild->AsTag();
                auto omitSpaces = 
                    pTag->NameMatches(XMLDoc_See) || 
                    pTag->NameMatches(XMLDoc_SeeAlso) || 
                    pTag->NameMatches(XMLDoc_ParamRef) || 
                    pTag->NameMatches(XMLDoc_TypeParamRef);

                if (!omitSpaces)
                {
                    pXMLString->AppendChar(CHAR_Space);
                }
                CollectBannerString(pNodeChild->AsStartTag(), pXMLString, pXMLDocParser);
                if (!omitSpaces)
                {
                    pXMLString->AppendChar(CHAR_Space);
                }
                break;
            }
            case XET_EmptyTag:
            {
                // Dev10#414367: Emit value of "cref" attribute on empty <see> and <seealso> tags.
                auto pTag = pNodeChild->AsTag();
                if (pTag->NameMatches(XMLDoc_See) || pTag->NameMatches(XMLDoc_SeeAlso))
                {
                    pXMLDocParser->ParseAttributes(pTag);
                    auto wszValue = pTag->GetAttributeValue(XMLDoc_CRef);
                    if (!wszValue)
                    {
                        wszValue = pTag->GetAttributeValue(XMLDoc_LangWord);
                    }
                    pXMLString->AppendString(wszValue);
                }
                else if (pTag->NameMatches(XMLDoc_ParamRef) || pTag->NameMatches(XMLDoc_TypeParamRef))
                {
                    pXMLDocParser->ParseAttributes(pTag);
                    pXMLString->AppendString(pTag->GetAttributeValue(XMLDoc_Name));
                }
                break;
            }
#endif RECURSIVE_BANNER
            case XET_CharData:
            {
                pXMLString->AppendString(pNodeChild->AsCharData()->m_wszString);
                break;
            }
        }

        pNodeChild = pNodeChild->m_pNext;
    }
}

/*****************************************************************************
;ExtractBannerFromXMLDocCommentBlock

This function needs to be super fast, so we can't use msxml parser.

Here are the rule for the Banner:
1) Try to find a <summary> tag, even with attributes, and first
one found, we will try to get the text inside the tag and use that as the
banner.

2) If we can't find a <summary> tag, we get the text of the first
valid XML tag. This valid XML tag can have attributes.

In any one of those two cases, we remove extra spaces and CRLFs from the string,
and if that makes the banner empty, then we skip that tag and keep searching for
the next valid tag that will give us a non-empty string.

Also, in any case above, we dig into children tags and we extract the text (not the tags)
from each, so the banner will be the union of the text for the tag and the text for all
its children tags.

Banner string can never be > 100 characters long, if it is, it will get truncated.

If we can't find any valid banner according to the rules above, we use <XML> as
a banner.

*****************************************************************************/
HRESULT XMLDocFile::ExtractBannerFromXMLDocCommentBlock(StringBuffer *pXMLString,
                                                        ParseTree::CommentBlockStatement *pCommentBlock)
{
    VB_ENTRY();
    StringBuffer CommentString;
    XMLDocFile::ExtractXMLDocStringFromCommentBlock(&CommentString, pCommentBlock);

    NorlsAllocator AllocatorForXml(NORLSLOC);

    // Create a light-weight XML parser for banner generation.
    XMLStartTag *pXMLDocRoot = NULL;
    CXMLDocParser XMLParser(GetCompilerPackage(), &AllocatorForXml);

    XMLParser.ParseXMLDoc(CommentString.GetString(), CommentString.GetStringLength(), &pXMLDocRoot);

    if (pXMLDocRoot)
    {
        XMLElement  *pXMLBannerNode   = pXMLDocRoot->m_pFirstChild;
        STRING      *pSummaryTagValue = GetCompilerPackage()->AddString(XMLDoc_Summary);

        // Loop over all tags and try to find a valid tag to use as a banner (see rules above).
        while (pXMLBannerNode)
        {
            if (pXMLBannerNode->m_type == XET_StartTag)
            {
                XMLStartTag *pStartTag = pXMLBannerNode->AsStartTag();

                if (pStartTag->m_name.m_pstrName == pSummaryTagValue)
                {
                    // Found a summary tag, get the text.
                    CollectBannerString(pStartTag, pXMLString, &XMLParser);
                    pXMLString->StripExtraWhiteSpacesAndLineBreaks();

                    if (pXMLString->GetStringLength() > 0)
                    {
                        goto End;
                    }
                }
            }

            pXMLBannerNode = pXMLBannerNode->m_pNext;
        }
    }

End:
    // If we couldn't do anything about it, we just use the default tag.
    if (pXMLString->GetStringLength() == 0)
    {
        ResLoadStringRepl(STRID_XMLDocCommentBanner, pXMLString);
    }
    else
    {
        pXMLString->Truncate(MaximumBannerSize);
    }
    VB_EXIT();
}
#endif 

/*****************************************************************************
;ReleaseAllXMLDocuments

Releases all the XML documents that were created as a result of parsing
XML comments for this file.
*****************************************************************************/
void XMLDocFile::ReleaseAllXMLDocuments()
{
    CSingleListIter<XMLDocNode> XMLDocNodeIterator(this);
    XMLDocNode *pCurrentNode = NULL;

    // Iterate over all XMLDoc comments and release the XMLDom tree for each.
    // This should be the only thing that needs to be released because the nodes
    // are created on the non-reease allocator, so they don't need to be explicitly
    // released.
    while (pCurrentNode = XMLDocNodeIterator.Next())
    {
        pCurrentNode->ReleaseNode();
    }

    //Same goes for the excluded XMLDoc comments
    XMLDocNodeIterator.Init(&m_ExcludedXMLDocNodes);
    pCurrentNode = NULL;
    while (pCurrentNode = XMLDocNodeIterator.Next())
    {
        pCurrentNode->ReleaseNode();
    }

    // Reset all member variables.
    CSingleList<XMLDocNode>::Clear();
    m_AreAllXMLDocCommentsProcessed = false;
    m_ExcludedXMLDocNodes.Clear();

#if IDE 
    delete m_pAllocatorForXml;
    m_pAllocatorForXml = NULL;
#endif
}

/*****************************************************************************
;ReleaseXMLTrees

Releases the XML trees built for all nodes in this file. This function
will not delete the XML string because it can be used later to re-build
the XML tree.
*****************************************************************************/
void XMLDocFile::ReleaseXMLTrees()
{
    CSingleListIter<XMLDocNode> XMLDocNodeIterator(this);
    XMLDocNode *pCurrentNode = NULL;

    while (pCurrentNode = XMLDocNodeIterator.Next())
    {
        pCurrentNode->ReleaseNodeTree();
    }

    m_AreAllXMLDocCommentsProcessed = false;
}

/*****************************************************************************
;ReEnableXMLDocNodes

Re-add disabled nodes to the main list. Called when we decompile down to declared.
Since these nodes were created from declared we add them back so that they are not lost going to bound.
*****************************************************************************/
void XMLDocFile::ReEnableXMLDocNodes()
{
    //Put back all ---- nodes
    CSingleListIter<XMLDocNode> XMLDocNodeIterator(&m_ExcludedXMLDocNodes);
    XMLDocNode *pCurrentNode = NULL;
    while (pCurrentNode = XMLDocNodeIterator.Next())
    {
        m_ExcludedXMLDocNodes.Remove(pCurrentNode);
        CSingleList<XMLDocNode>::InsertLast(pCurrentNode);
        pCurrentNode->RestoreSymbol();
    }

}

void XMLDocFile::BindAllXMLDocCommentNodes()
{
    if (!m_AreAllXMLDocCommentsProcessed)
    {
        CSingleListIter<XMLDocNode> XMLDocNodeIterator(this);
        XMLDocNode *pCurrentNode = XMLDocNodeIterator.Next();

        while (pCurrentNode)
        {
            pCurrentNode->ProcessXMLDocCommentNode(true, NULL);
            pCurrentNode = XMLDocNodeIterator.Next();
        }

        m_AreAllXMLDocCommentsProcessed = true;
    }
}


/*****************************************************************************
;AddXMLDocComment

Builds an XMLDoc comment from the CommentBlock passed in and if no errors
are detected in the parsed XML, adds it to the list of XMLDoc comments
associated with the owning file.
*****************************************************************************/
TrackedLocation *XMLDocFile::AddXMLDocComment(ParseTree::CommentBlockStatement *pCommentBlock,
                                              BCSYM_NamedRoot *pNamedElement)
{
    XMLDocNode *pNewDocNode = (XMLDocNode *) m_pSourceFile->SymbolStorage()->Alloc(sizeof(XMLDocNode));

    VB_ENTRY();

    if (!pNewDocNode)
    {
        VSFAIL("Unable to allocate memory for an XMLDoc node");
        return NULL;
    }

#if IDE 
    if (!m_pAllocatorForXml)
    {
        m_pAllocatorForXml = new NorlsAllocator(NORLSLOC);
    }

    pNewDocNode->InitNode(this, pNamedElement, pCommentBlock, pCommentBlock->BodyTextSpan, m_pAllocatorForXml);
#else
    pNewDocNode->InitNode(this, pNamedElement, pCommentBlock, pCommentBlock->BodyTextSpan);
#endif

#if IDE 
    pNewDocNode->LockNode();
#endif IDE

    // And link it in the linked list of nodes.
    CSingleList<XMLDocNode>::InsertLast(pNewDocNode);

    pNamedElement->SetXMLDocNode(pNewDocNode);

    VB_EXIT_NORETURN();

#if IDE 
    pNewDocNode->UnLockNode();
#endif IDE

    return &(pNewDocNode->m_CommentTextSpan);
}


/*****************************************************************************
;DisableXMLDocNode
Called when a partial type has multiple XMLDoc comments.
We remove each XMLDoc node from the active list and add it to the
disabled list. When we come down from Bound to Declared we re-add these to the
valid list so they are not lost forever.
*****************************************************************************/
void XMLDocFile::DisableXMLDocNode(_In_ XMLDocNode *pXMLDocNode)
{
    CSingleListIter<XMLDocNode> XMLDocNodeIterator(this);
    XMLDocNode *pCurrent = NULL;

    while (pCurrent = XMLDocNodeIterator.Next())
    {
        if (pCurrent == pXMLDocNode)
        {
            CSingleList<XMLDocNode>::Remove(pCurrent);
            m_ExcludedXMLDocNodes.InsertLast(pXMLDocNode);
            break;
        }
    }

}

/*****************************************************************************
;DemoteXMLDocFile

Process a demotion as a result of demoting the source file.
*****************************************************************************/
void XMLDocFile::DemoteXMLDocFile(CompilationState CS)
{
    switch (CS)
    {
    case CS_NoState:
        // Everything goes here.
        ReleaseAllXMLDocuments();
        break;

    case CS_Declared:
        ReEnableXMLDocNodes();
        __fallthrough; //fall through
    case CS_Bound:
    case CS_TypesEmitted:
        // Since XML trees are built going to CS_Compiled, we need to throw them away
        // as soon as we decompile to or below CS_TypesEmitted. This has to be done
        // so we would regenerate any errors that may have been reported.
        ReleaseXMLTrees();
        break;

    case CS_Compiled:
        // Nothing to do, nothing to throw away.
        break;

    default:
        VSFAIL("Unknown compiler state");
    }
}

/*****************************************************************************
;GetXMLDocForFile

Builds and returns an XMLDoc comment string for the owning file. This is
only done when the assembly is generated.
*****************************************************************************/
HRESULT XMLDocFile::GetXMLDocForFile(BaseWriter *pXMLDocForFile)
{
    HRESULT hr = NO_ERROR;
    CSingleListIter<XMLDocNode> XMLDocNodeIterator(this);
    XMLDocNode *pCurrentNode = XMLDocNodeIterator.Next();

#if IDE 
    XMLParseTreeCache cache;
#endif

    while (pCurrentNode)
    {
#if IDE 
        pCurrentNode->LockNode();
#endif IDE

        VB_ENTRY_NOHR();
        CComPtr<IXMLDOMDocument> spDocument;
        CComPtr<IXMLDOMElement> spRootElement;

        pCurrentNode->ProcessXMLDocCommentNode(false, &spDocument);

        if (spDocument)
        {
            spDocument->get_documentElement(&spRootElement);

            if (!pCurrentNode->m_IsBadXMLDocNode)
            {
                // Resolve all include tags and fill in the "name" attribute on the <member> tag.

                pCurrentNode->ResolveIncludeTags(spDocument);
                pCurrentNode->ResolveMemberName(spDocument);

                CComBSTR bstrText;
                spRootElement->get_xml(&bstrText);

                pXMLDocForFile->AppendString(bstrText);
            }
        }

        VB_EXIT_NORETURN();

#if IDE 
        pCurrentNode->UnLockNode();
#endif IDE

        pCurrentNode = XMLDocNodeIterator.Next();
    }

    return hr;
}

#pragma warning(pop)
