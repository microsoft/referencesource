//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
//-------------------------------------------------------------------------------------------------

#include "StdAfx.h"

const wchar_t *XMLCommentStream::s_pStartTag = L"<member name=\"\">\r\n";
const wchar_t *XMLCommentStream::s_pEndTag = L"\r\n</member>";
const wchar_t *XMLCommentStream::s_pSeparator = L"\r\n";
const BYTE XMLCommentStream::s_UnicodeBOM[] =
{
    0xFF,
    0xFE
};
const ULONG XMLCommentStream::s_StartTagSize = (ULONG)wcslen(XMLCommentStream::s_pStartTag) * sizeof(wchar_t);
const ULONG XMLCommentStream::s_EndTagSize = (ULONG)wcslen(XMLCommentStream::s_pEndTag) * sizeof(wchar_t);
const ULONG XMLCommentStream::s_SeparatorSize = (ULONG)wcslen(XMLCommentStream::s_pSeparator) * sizeof(wchar_t);
const ULONG XMLCommentStream::s_SkipCount = XMLCommentStream::s_StartTagSize / sizeof(wchar_t);
const ULONG XMLCommentStream::s_BomCount = sizeof(XMLCommentStream::s_UnicodeBOM) / sizeof(wchar_t);

XMLCommentStream::XMLCommentStream(
    ParseTree::CommentBlockStatement * pCommentBlock,
    bool skipBOM) :
    m_pData(NULL),
    m_DataCount(0),
    m_pCurrentStatement(NULL),
    m_pCurrentComment(NULL),
    m_pCommentBlock(pCommentBlock),
    m_readState(XMLCommentStream::RS_Invalid),
    m_refCount(0)
{
    if (pCommentBlock)
    {
        if (skipBOM)
        {
            m_readState = RS_StartTag;
            m_pData = reinterpret_cast<const BYTE *>(s_pStartTag);
            m_DataCount = s_StartTagSize;
        }
        else
        {
            m_readState = RS_BOM;
            m_pData = s_UnicodeBOM;
            m_DataCount = sizeof(s_UnicodeBOM);
        }
    }
}

XMLCommentStream::~XMLCommentStream()
{
    Clear();
}

void XMLCommentStream::Clear()
{
    m_pData = NULL;
    m_DataCount = 0;
    m_pCurrentStatement = NULL;
    m_pCurrentComment = NULL;
    m_pCommentBlock = NULL;
    m_readState = RS_Invalid;
}

void XMLCommentStream::IndicateEndState()
{
    m_readState = RS_End;
    m_pData = NULL;
    m_DataCount = 0;
    m_pCurrentComment = NULL;
    m_pCurrentStatement = NULL;
}

void XMLCommentStream::GetNextComment()
{
    m_pData = NULL;
    m_DataCount = 0;

    if (m_pCurrentComment)
    {
        m_pCurrentComment = m_pCurrentComment->Next;
    }

    if (m_readState == RS_BOM)
    {
        m_pData = reinterpret_cast<const BYTE *>(s_pStartTag);
        m_DataCount = s_StartTagSize;
        m_readState = RS_StartTag;
    }
    else
    {

        while
        (
            (m_readState == RS_StartTag || m_pCurrentStatement)
            &&
            (! m_pCurrentComment || !m_pCurrentComment->Element)
        )
        {
            if (m_readState == RS_StartTag)
            {
                m_pCurrentStatement = m_pCommentBlock->Children;
                m_readState = RS_CommentBlock;
            }
            else
            {
                m_pCurrentStatement = m_pCurrentStatement->NextInBlock;
            }

            VSASSERT(!m_pCurrentStatement || m_pCurrentStatement->Element, "The statement list should not contain a null element");
            if (m_pCurrentStatement && m_pCurrentStatement->Element)
            {
                m_pCurrentComment = m_pCurrentStatement->Element->Comments;
                VSASSERT(!m_pCurrentComment || m_pCurrentComment->Element, "The comment list should not contain a null element");

                if (m_pCurrentComment && m_pCurrentComment->Element)
                {
                    m_pData = reinterpret_cast<BYTE *>(m_pCurrentComment->Element->Spelling);
                    m_DataCount = (ULONG)(m_pCurrentComment->Element->LengthInCharacters * sizeof(wchar_t));
                    m_readState = RS_CommentBlock;
                }
            }
        }
    }

    if (!m_pData)
    {
        m_pCurrentStatement = NULL;
        m_pCurrentComment = NULL;
        m_pData = reinterpret_cast<const BYTE *>(s_pEndTag);
        m_DataCount = s_EndTagSize;
        m_readState = RS_EndTag;
    }

}

void XMLCommentStream::GetByte(
    _Inout_opt_ BYTE * * ppOutput,
    _Inout_opt_ ULONG * pBytesRemaining,
    _Inout_opt_ ULONG * pBytesRead)
{
    if (ppOutput && *ppOutput)
    {
        **ppOutput = *m_pData;
        ++(*ppOutput);
    }

    if (pBytesRemaining)
    {
        --(*pBytesRemaining);
    }

    if (pBytesRead)
    {
        ++(*pBytesRead);
    }

    ++m_pData;
    --m_DataCount;
}

STDMETHODIMP XMLCommentStream::Read(
    void * pv,
    ULONG cb,
    _Out_opt_ ULONG * pcbRead)
{
    if (m_readState == RS_Invalid)
    {
        return E_FAIL;
    }
    else
    {
        ULONG readCount = 0;
        while (cb && m_readState != RS_End)
        {
            if (m_DataCount)
            {
                GetByte((BYTE **)&pv, &cb, &readCount);
            }
            else
            {
                switch (m_readState)
                {
                case RS_EndTag:
                    IndicateEndState();
                    break;
                case RS_Separator:
                case RS_BOM:
                case RS_StartTag:
                    GetNextComment();
                    break;
                default:
                    m_readState = RS_Separator;
                    m_DataCount = s_SeparatorSize;
                    m_pData = reinterpret_cast<const BYTE *>(s_pSeparator);
                    break;
                };
            }
        }

        if (pcbRead)
        {
            *pcbRead = readCount;
        }

        if (!readCount)
        {
            return S_FALSE;
        }
        else
        {
            return S_OK;
        }
    }
}

STDMETHODIMP XMLCommentStream::Write(
    const void * pv,
    ULONG cb,
    ULONG * pcbWritten)
{
    return STG_E_ACCESSDENIED;
}

STDMETHODIMP_(ULONG) XMLCommentStream::AddRef()
{
    ++m_refCount;
    return m_refCount;
}

STDMETHODIMP XMLCommentStream::QueryInterface(
    REFIID iid,
    _Out_opt_ void * * ppvObject)
{
    if (ppvObject)
    {
        if (iid == IID_ISequentialStream)
        {
            *ppvObject = static_cast<ISequentialStream *>(this);
            ++m_refCount;

            return S_OK;
        }
        else if (iid == IID_IUnknown)
        {
            *ppvObject = static_cast<IUnknown *>(this);
            ++m_refCount;

            return S_OK;
        }
        else
        {
            *ppvObject = NULL;

            return E_NOINTERFACE;
        }
    }
    else
    {
        return E_POINTER;
    }
}

STDMETHODIMP_(ULONG) XMLCommentStream::Release()
{
    ULONG ret = --m_refCount;

    if (!ret)
    {
        delete this;
    }

    return ret;
}

