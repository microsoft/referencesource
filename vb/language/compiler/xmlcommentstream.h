//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Defines an implementation of the ISequentialStream interface that allows an
//  XML DOM Document to be constructued directly over a parse tree (specifically
//  a CommentBlockStatement) without requiring an intermediate string buffer.
//
//  Unfortunately the class has to provide its own implementation of IUnknown
//  because it needs to be used from the command compiler, which currently
//  will not work with ATL (it ----s up when <atlcom.h> is included, which
//  is necessary in order to use CComObjectRootEx).
//
//  When read, the stream will yield the following contents:
//  1. A "synthetic" opening root tag (<member name="">)
//  2. The XML contents of the comment block
//  3. A "synthetic" closing root tag (</member>)
//
//-------------------------------------------------------------------------------------------------

#pragma once

class XMLCommentStream : public ISequentialStream
{
public:
    NEW_MUST_ZERO();

    XMLCommentStream(
        ParseTree::CommentBlockStatement * pCommentBlock,
        bool skipBOMAndDecl = false);

    ~XMLCommentStream();

    STDMETHOD(Read)(
        void *pv, 
        ULONG cb, 
        _Out_opt_ ULONG *pcbRead);

    STDMETHOD(Write)(
        const void *pv, 
        ULONG cb, 
        ULONG *pcbWritten);

    STDMETHOD_(ULONG,AddRef)();

    STDMETHOD(QueryInterface)(
        REFIID iid, 
        _Out_opt_ void **ppvObject);

    STDMETHOD_(ULONG,Release)();

    void Clear();

    static const ULONG s_SkipCount;
    static const ULONG s_BomCount;

private:
    void GetNextComment();
    void IndicateEndState();
    
    void GetByte(
        _Inout_opt_ BYTE * * ppOutput,
        _Inout_opt_ ULONG * pBytesRemaining,
        _Inout_opt_ ULONG * pBytesRead);

    static const wchar_t *s_pStartTag;
    static const wchar_t *s_pEndTag;
    static const wchar_t *s_pSeparator;
    static const BYTE s_UnicodeBOM[];
    static const ULONG s_StartTagSize;
    static const ULONG s_EndTagSize;
    static const ULONG s_SeparatorSize;

    enum ReadState
    {
        RS_Invalid,                     // The stream is in an invalid state.
        RS_BOM,                         // The stream is positioned inside the unicode byte ordering mark.
        RS_StartTag,                    // The stream is positioned inside the synthetic "<member>" tag.
        RS_CommentBlock,                // The reader is currently processing the contents of the comment block.
        RS_Separator,                   // The reader is currently processing the line terminator string the seperates comment block entries.
        RS_EndTag,                      // The reader is currently positioned inside the artifical "</member>" tag.
        RS_End                          // The reader is currently positioned after the end of the stream.
    };

    ReadState m_readState;

    const BYTE *m_pData;
    ULONG m_DataCount;

    //stores the current statement in the
    //comment block being processed
    ParseTree::StatementList *m_pCurrentStatement;

    //stores the current comment within the
    //current statement being processed.
    ParseTree::CommentList *m_pCurrentComment;

    ParseTree::CommentBlockStatement *m_pCommentBlock;

    ULONG m_refCount;
};
