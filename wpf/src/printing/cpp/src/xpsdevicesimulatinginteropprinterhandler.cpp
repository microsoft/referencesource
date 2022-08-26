/*++

    Copyright (C) 2002 - 2003 Microsoft Corporation
    All rights reserved.

    Module Name:

        InteropPrinterHandler.hpp

    Abstract:

        Managed wrapper for Win32 print APIs. This object wraps a printer handle
        and does gets, sets and enum operations.It also provides static methods
        for adding and deleting a printer and enumerating printers on a print server.

    Author:

        Adina Trufinescu April 24, 2003

    Revision History:
--*/

#include "win32inc.hpp"

using namespace System;
using namespace System::IO;
using namespace System::Collections;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Collections::Specialized;
using namespace System::Xml;
using namespace System::Xml::XPath;
using namespace System::Drawing::Printing;

using namespace System::Windows::Xps::Packaging;
using namespace System::Security::Permissions;

#ifndef  __INTEROPNAMESPACEUSAGE_HPP__
#include <InteropNamespaceUsage.hpp>
#endif

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif

#ifndef  __PRINTERDATATYPES_HPP__
#include <PrinterDataTypes.hpp>
#endif

#ifndef  __GENERICTHUNKINGINC_HPP__
#include <GenericThunkingInc.hpp>
#endif

#ifndef  __PRINTSYSTEMINC_HPP__
#include <PrintSystemInc.hpp>
#endif

#ifndef  __XPSJOBSTREAM_HPP__
#include <XpsPrintJobStream.hpp>
#endif

using namespace System;
using namespace MS::Internal::PrintWin32Thunk;
using namespace MS::Internal::PrintWin32Thunk::DirectInteropForPrintQueue;
using namespace MS::Internal::PrintWin32Thunk::DirectInteropForJob;
using namespace MS::Internal::PrintWin32Thunk::Win32ApiThunk;
using namespace System::Threading;

using namespace System::Printing;
using namespace System::Printing::IndexedProperties;

using namespace MS::Internal::PrintWin32Thunk::Win32ApiThunk;

///<SecurityNote>
/// Critical    - stores a win32 print handle
/// TreatAsSafe - the win32PrintHandle is created by calling ThunkAddPrinter
///               ThunkAddPrinter is only enabled in FullTrust and demands AllPrinting permission
///</SecurityNote>
XpsDeviceSimulatingPrintThunkHandler::
XpsDeviceSimulatingPrintThunkHandler(
    String^ printerName
) :
    printerName(printerName),
    spoolerStream(nullptr),
    xpsPrintJob(NULL)
{
}

    /*++

Routine Name:

    get_IsInvalid

Routine Description:

    Checks the object validity

Arguments:

    None

Return Value:

    true if the object contains a valid Win3e2 printer handle

--*/
Boolean
XpsDeviceSimulatingPrintThunkHandler::IsInvalid::
get(
    void
    )
{
    return xpsPrintJob == NULL;
}

Boolean
XpsDeviceSimulatingPrintThunkHandler::
ReleaseHandle(
    void
    )
{
    return true;
}

// Dev11:#158013: Warning 4714 (__forceinline function not inlined)
// is expected here because PrintQueueStream::Write is marked with [SecurityCritical]
// and tries to inline HRESULT_FROM_WIN32.
// Starting with changeset 172903 (see also Dev11 bugs 4172, 134965, 134979),
// inlining is prevented when the caller or the callee
// are marked with any security attribute (critical, safecritical, treatassafecritical).
// This is over conservative and misses inlining opportunities occasionaly,
// but currently there is no way of determining accurately the transparency level of a function
// in the native compiler since there are no public APIs provided by CLR at the moment.
// Replicating CLR transparency rules on the native side is not ideal either.
// The solution chosen is to allow inlining only when there is clear evidence
// for the caller and the callee to be transparent.
#pragma warning (push)
#pragma warning (disable : 4714)

///<SecurityNote>
/// Critical    - Implementations are expected to call native methods to initialize a printer device for printing
/// TreatAsSafe - this class demands DefaultPrinting
///</SecurityNote>
Int32
XpsDeviceSimulatingPrintThunkHandler::
ThunkStartDocPrinter(
    DocInfoThree^         docInfo,
    PrintTicket^ printTicket
    )
{
    assert(NULL == xpsPrintJob);

    ManualResetEvent^ tempCompletedEvent = gcnew ManualResetEvent(false);
    AutoResetEvent^ tempProgressEvent = gcnew AutoResetEvent(false);

    VOID *tempJob = NULL;
    VOID *tempDocStream = NULL;
    VOID *tempTicketStream = NULL;
    int hr = 0;

    // Get the 'fast copy' flag;  see remarks in PrintQueueStream::InitializePrintStream
    Boolean fastCopy = (docInfo->docFlags & 0x40000000) != 0;   // 0x40000000 = fastCopy
    docInfo->docFlags = docInfo->docFlags & ~0x40000000;

    // Call StartXpsPrintJob.  If the Microsoft XPS Document Writer (or similar
    // device) was selected, this will prompt for a file.  If a print ticket was
    // passed to us, we will pass it to StartXpsPrintJob and wait for a Job ID.
    if(printTicket == nullptr)
    {
        hr = PresentationNativeUnsafeNativeMethods::LateBoundStartXpsPrintJob(
            printerName,
            docInfo->docName,
            docInfo->docOutputFile,
            tempProgressEvent->SafeWaitHandle,
            tempCompletedEvent->SafeWaitHandle,
            NULL,
            0,
            &tempJob,
            &tempDocStream,
            NULL);
    }
    else
    {
        hr = PresentationNativeUnsafeNativeMethods::LateBoundStartXpsPrintJob(
            printerName,
            docInfo->docName,
            docInfo->docOutputFile,
            tempProgressEvent->SafeWaitHandle,
            tempCompletedEvent->SafeWaitHandle,
            NULL,
            0,
            &tempJob,
            &tempDocStream,
            &tempTicketStream);
    }

    // Note: if MXDW was selected, but the user canceled the file prompt, this
    // will return an error code that we convert into PrintingCanceledException.
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) ||
        hr == HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED))
    {
        throw gcnew PrintingCanceledException(
            hr,
            "PrintSystemException.PrintingCancelled.Generic"
            );
    }
    else
    {
        InternalPrintSystemException::ThrowIfNotCOMSuccess(hr);
    }

    xpsPrintJob = (IXpsPrintJob *)tempJob;
    spoolerStream = gcnew XpsPrintJobStream((IXpsPrintJobStream *)tempDocStream, tempCompletedEvent, false, true);

    if(printTicket != nullptr)
    {
        // Write the print ticket to the print ticket stream, and close the stream.
        XpsPrintJobStream^ ticketStream = gcnew XpsPrintJobStream((IXpsPrintJobStream *)tempTicketStream, nullptr, false, true);
        printTicket->SaveTo(ticketStream);
        delete ticketStream;
    }

    // Get the job ID, which may or may not be available.
    XPS_JOB_STATUS status = {0};
    hr = xpsPrintJob->GetJobStatus(&status);
    InternalPrintSystemException::ThrowIfNotCOMSuccess(hr);

    if(status.jobId == 0)
    {
        // The job ID was not initially available, wait for the progress
        // event to be signalled and then get the job ID again.
        //
        // We do this only if there's a print ticket, or in 'fast copy' mode.
        // Otherwise the wait would never return.
        if (fastCopy || printTicket != nullptr)
        {
            tempProgressEvent->WaitOne();

            hr = xpsPrintJob->GetJobStatus(&status);
            InternalPrintSystemException::ThrowIfNotCOMSuccess(hr);
        }
    }
    delete tempProgressEvent;
    jobIdentifier = status.jobId;

    return jobIdentifier;
}
#pragma warning (pop)

///<SecurityNote>
/// Critical    - calls UnsafeNativeMethods::InvokeEndDocPrinter which has SUC applied
/// TreatAsSafe - this class demands DefaultPrinting.
///</SecurityNote>
Boolean
XpsDeviceSimulatingPrintThunkHandler::
ThunkEndDocPrinter(
    void
    )
{
    // Order matters
    // To properly end printing
    // Delete spoolerStream
    // Release xpPrintJob

    if(spoolerStream != nullptr)
    {
        delete spoolerStream;
        spoolerStream = nullptr;
    }

    if(xpsPrintJob != NULL)
    {
        xpsPrintJob->Release();
        xpsPrintJob = NULL;
    }

    return true;
}


///<SecurityNote>
/// Critical    - calls UnsafeNativeMethods::ThunkAbortPrinter which has SUC applied
/// TreatAsSafe - this class demands DefaultPrinting.
///</SecurityNote>
Boolean
XpsDeviceSimulatingPrintThunkHandler::
ThunkAbortPrinter(
    void
    )
{
    bool result = false;

    if(NULL != xpsPrintJob)
    {
        if(0 <= xpsPrintJob->Cancel()) // SUCCEEDED macro doesn't compile
        {
            // Order matters
            // To properly cancel printing
            // Cancel xpsPrintJob
            // Delete spoolerStream
            // Release xpPrintJob

            if(spoolerStream != nullptr)
            {
                delete spoolerStream;
                spoolerStream = nullptr;
            }

            if(xpsPrintJob != NULL)
            {
                xpsPrintJob->Release();
                xpsPrintJob = NULL;
            }


            result = true;
        }
    }
    else
    {
        result = true;
    }

    spoolerStream = nullptr;
    return result;
}

void
XpsDeviceSimulatingPrintThunkHandler::
ThunkOpenSpoolStream(
    void
    )
{
}

void
XpsDeviceSimulatingPrintThunkHandler::
ThunkCommitSpoolData(
    Int32                   bytes
    )
{
}

Boolean
XpsDeviceSimulatingPrintThunkHandler::
ThunkCloseSpoolStream(
    void
    )
{
    return true;
}

int
XpsDeviceSimulatingPrintThunkHandler::JobIdentifier::
get(
    void
    )
{
    if(xpsPrintJob != NULL && jobIdentifier != 0)
    {
        XPS_JOB_STATUS status = {0};
        ::HRESULT hr = xpsPrintJob->GetJobStatus(&status);

        InternalPrintSystemException::ThrowIfNotCOMSuccess(hr);

        jobIdentifier = status.jobId;
    }

    return jobIdentifier;
}


Stream^
XpsDeviceSimulatingPrintThunkHandler::SpoolStream::
get(
    void
    )
{
    return spoolerStream;
}

Int32
XpsDeviceSimulatingPrintThunkHandler::
ThunkReportJobProgress(
    Int32                                                           jobId,
    JobOperation                                                    jobOperation,
    System::Windows::Xps::Packaging::PackagingAction                packagingAction
    )
{
    return 0;
}


