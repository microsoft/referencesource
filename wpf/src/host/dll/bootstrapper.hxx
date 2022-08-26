//+-----------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Description:
//     Defines the BootStrapper class of PresentationHost.
//
//  History
//      2002/06/25-murrayw
//          Created
//      2007/09/20-Microsoft
//          Ported Windows->DevDiv. See SourcesHistory.txt.
//
//------------------------------------------------------------------------

#pragma once

class BootStrapper
{

public:
    BootStrapper(__in COleDocument* pOleDoc);
    ~BootStrapper();

    void AvalonExecute(__in_opt IStream *loadStream = NULL); // triggers Watson and terminates on failure

private:
    COleDocument         * m_pOleDoc;
    ICorRuntimeHost      * m_pHost;
};
