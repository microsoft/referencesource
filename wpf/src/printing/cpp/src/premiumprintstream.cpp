/*++

    Copyright (C) 2004 - 2005 Microsoft Corporation
    All rights reserved.

    Module Name:

        PrintQueueStream.cpp

    Abstract:

        Provides a managed stream that allows writing to the Spl file consumed by the Print
        Spooler process.

    Author:

        Ali Naqvi (alinaqvi) - Microsoft 29 2005

    Revision History:

       Adina Trufinescu (adinatru) June 2005
       Enabling support for printing in Partial Trust

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

using namespace System::Printing;
using namespace System::Printing::Interop;
using namespace System::Threading;
using namespace Microsoft::Win32::SafeHandles;
using namespace System::Security;
using namespace System::Security::Permissions;
using namespace System::Drawing::Printing;

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif

#ifndef  __GENERICTHUNKINGINC_HPP__
#include <GenericThunkingInc.hpp>
#endif

#ifndef  __PRINTSYSTEMINC_HPP__
#include <PrintSystemInc.hpp>
#endif



using namespace MS::Internal::PrintWin32Thunk;

PrintQueueStream::
PrintQueueStream(
    PrintQueue^     printQueue,
    String^         printJobName,
    Boolean         commitDataOnClose,
    PrintTicket^    printTicket
    ):
printQueue(printQueue),
printJobName(printJobName),
jobIdentifier(0),
bytesToCommit(0),
bytesPreviouslyCommited(0),
commitStreamDataOnClose(commitDataOnClose),
printerThunkHandler(nullptr),
streamClosed(false),
streamAborted(false),
accessVerifier(nullptr),
isFinalizer(false)
{
    InitializePrintStream(printTicket);
}

PrintQueueStream::
PrintQueueStream(
    PrintQueue^     printQueue,
    String^         printJobName,
    Boolean         commitDataOnClose,
    PrintTicket^    printTicket,
    Boolean         fastCopy
    ):
printQueue(printQueue),
printJobName(printJobName),
jobIdentifier(0),
bytesToCommit(0),
bytesPreviouslyCommited(0),
commitStreamDataOnClose(commitDataOnClose),
printerThunkHandler(nullptr),
streamClosed(false),
streamAborted(false),
accessVerifier(nullptr),
isFinalizer(false)
{
    InitializePrintStream(printTicket, fastCopy);
}

PrintQueueStream::
PrintQueueStream(
    PrintQueue^     printQueue,
    String^         printJobName,
    Boolean         commitDataOnClose
    ):
printQueue(printQueue),
printJobName(printJobName),
jobIdentifier(0),
bytesToCommit(0),
bytesPreviouslyCommited(0),
commitStreamDataOnClose(commitDataOnClose),
printerThunkHandler(nullptr),
streamClosed(false),
streamAborted(false),
accessVerifier(nullptr),
isFinalizer(false)
{
    InitializePrintStream(nullptr);
}


PrintQueueStream::
PrintQueueStream(
    PrintQueue^     printQueue,
    String^         printJobName
    ):
printQueue(printQueue),
printJobName(printJobName),
jobIdentifier(0),
bytesToCommit(0),
bytesPreviouslyCommited(0),
commitStreamDataOnClose(false),
printerThunkHandler(nullptr),
streamClosed(false),
streamAborted(false),
accessVerifier(nullptr),
isFinalizer(false)
{
    InitializePrintStream(nullptr);
}

void
PrintQueueStream::
InitializePrintStream(
    PrintTicket^ printTicket
    )
{
    InitializePrintStream(printTicket, false);
}

void
PrintQueueStream::
InitializePrintStream(
    PrintTicket^ printTicket,
    Boolean      fastCopy
    )
{

    //
    // This condition holds true only if the printQueue was created as a result passing
    // a NULL PrintQueue pointer to PrintQueue::CreateXPSDocumentWriter.
    // This is a Partial Trust scenario, when the calling code doesn't have permissions to
    // create a PrintQueue object and deffers the creation to the Avalon Print UI
    // By asserting permissions here, we allow ThunkCloseSpoolFileHandle and ThunkEndDocPrinter
    // to succeed, which is considered Safe in PArtial Trust, once the dialog was opened.
    //
    accessVerifier = gcnew PrintSystemDispatcherObject();

    if (printQueue->InPartialTrust)
    {
        (gcnew PrintingPermission(PrintingPermissionLevel::DefaultPrinting))->Assert();
    }

    try
    {
        // for Dev11 457051, we need to inform the XpsDeviceSimulatingPrintThunkHandler
        // whether this is a 'fast copy' print job.  Normally you'd do that by passing
        // a parameter, but we can't change the signature of a virtual method
        // (ThunkStartDocPrinter) in a servicing patch, even though it's internal.
        // Instead we put the information into the docFlags field of the DocInfoThree
        // parameter, using a bit that's very unlikely to be used by anyone else
        // (winspool.h only defines one bit in this field).   Naturally, we have
        // to clear that bit (in all overrides of ThunkStartDocPrinter before
        // passing the DocInfoThree into the OS printing components.
        Int32 flags = fastCopy ? 0x40000001 : 1;        // 0x40000000 = fastCopy

        printerThunkHandler = printQueue->CreatePrintThunkHandler();

        DocInfoThree^ docInfo = gcnew DocInfoThree(printJobName,
                                                   printQueue->QueuePort->Name,
                                                   DocInfoThree::defaultDataType,
                                                   flags);

        jobIdentifier = printerThunkHandler->ThunkStartDocPrinter(docInfo, printTicket);

        printerThunkHandler->ThunkOpenSpoolStream();
    }
    catch (InternalPrintSystemException^ internalException)
    {
        throw PrintSystemJobInfo::CreatePrintJobException(internalException->HResult,
                                      "PrintSystemException.PrintSystemJobInfo.Create");
    }
    __finally
    {

        if (printQueue->InPartialTrust)
        {
            SecurityPermission::RevertAssert();
        }
    }
}

PrintQueueStream::
~PrintQueueStream(
    )
{
    Close();
}

PrintQueueStream::
!PrintQueueStream(
    )
{
    isFinalizer = true;

    //Attempt to close unmanaged resources only
    if (!this->streamClosed)
    {
        // note, if printerThunkHandler is finalized first, this might leak
        // the spool file handle as we will be unable to call ThunkAbortPrinter().
        if (printerThunkHandler != nullptr  &&
            !printerThunkHandler->IsInvalid)
        {
            printerThunkHandler->ThunkAbortPrinter();
            delete printerThunkHandler;
        }
    }
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

void
PrintQueueStream::
Write(
    array<unsigned char>^   array,
    int                     offset,
    int                     numBytes
    )
{
    if (printerThunkHandler != nullptr)
    {
        if (printQueue->PrintingIsCancelled)
        {
            //
            // This condition holds true only if the printQueue was created as a result passing
            // a NULL PrintQueue pointer to PrintQueue::CreateXPSDocumentWriter.
            // This is a Partial Trust scenario, when the calling code doesn't have permissions to
            // create a PrintQueue object and deffers the creation to the Avalon Print UI
            // By asserting permissions here, we allow ThunkCloseSpoolFileHandle and ThunkEndDocPrinter
            // to succeed, which is considered Safe in PArtial Trust, once the dialog was opened.
            //
            if (printQueue->InPartialTrust)
            {
                (gcnew PrintingPermission(PrintingPermissionLevel::DefaultPrinting))->Assert();
            }

            try
            {
                printerThunkHandler->ThunkAbortPrinter();
            }
            __finally
            {
                if (printQueue->InPartialTrust)
                {
                    SecurityPermission::RevertAssert();
                }
            }

            printQueue->PrintingIsCancelled = false;

            throw CreatePrintingCanceledException(HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED),
                                                  "PrintSystemException.PrintingCancelled.Generic");
        }
        else
        {
            //Do not write to the spoolFileStream if its aborted
            if (!streamAborted)
            {
                printerThunkHandler->SpoolStream->Write(array, offset, numBytes);

                //
                // Computing the number of bytes that need to be commited to Spooler
                // when the FixedPageAdded notification comes in.
                //
                if (!commitStreamDataOnClose)
                {
                    bytesToCommit += numBytes;
                }
            }
        }
    }
}
#pragma warning (pop)

int
PrintQueueStream::
Read(
    array<unsigned char>^   array,
    int                     offset,
    int                     count
    )
{
    return printerThunkHandler->SpoolStream->Read(array, offset, count);
}

IAsyncResult^
PrintQueueStream::
BeginWrite(
    array<Byte>^    buffer,
    Int32           offset,
    Int32           numBytes,
    AsyncCallback^  userCallBack,
    Object^         stateObject
    )
{
    WritePrinterAsyncResult^ writeAsyncResult = nullptr;

    if (buffer == nullptr || numBytes == 0 || numBytes > buffer->Length)
    {
        throw gcnew ArgumentNullException("buffer");
    }

    if (printQueue->PrintingIsCancelled)
    {
        throw CreatePrintingCanceledException(HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED),
                                              "PrintSystemException.PrintingCancelled.Generic");
    }
    else
    {
        writeAsyncResult = gcnew  WritePrinterAsyncResult(this,
                                                          buffer,
                                                          offset,
                                                          numBytes,
                                                          userCallBack,
                                                          stateObject);

        Thread^ asyncWriteThread = gcnew Thread(gcnew ThreadStart(writeAsyncResult,
                                                                &WritePrinterAsyncResult::AsyncWrite));

        asyncWriteThread->Start();

    }

    return writeAsyncResult;
}

void
PrintQueueStream::
EndWrite(
    IAsyncResult^   asyncResult
    )
{
    if (printQueue->PrintingIsCancelled)
    {
        throw CreatePrintingCanceledException(HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED),
                                              "PrintSystemException.PrintingCancelled.Generic");
    }
    else if (asyncResult == nullptr)
    {
        throw gcnew ArgumentNullException("asyncResult");
    }
    else
    {
        asyncResult->AsyncWaitHandle->WaitOne();
    }
}

Int64
PrintQueueStream::
Seek(
    Int64         offset,
    SeekOrigin    origin
    )
{
   return printerThunkHandler->SpoolStream->Seek(offset, origin);
}

void
PrintQueueStream::
Abort(
    void
    )
{
    streamAborted = true;
}

void
PrintQueueStream::
Close(
    void
    )
{
    AbortOrCancel( streamAborted );
}


void
PrintQueueStream::
AbortOrCancel(
    bool abort
    )
{
    if (!this->streamClosed)
    {
        System::Threading::Monitor::Enter(accessVerifier);

        try
        {
            if (printerThunkHandler != nullptr)
            {
                //
                // Adjust the number of bytesToCommit to the filestream length if xps streaming serialization
                // is not enabled.
                //
                if (commitStreamDataOnClose)
                {
                    bytesToCommit = this->Length;
                }

                //
                // This condition holds true only if the printQueue was created as a result passing
                // a NULL PrintQueue pointer to PrintQueue::CreateXPSDocumentWriter.
                // This is a Partial Trust scenario, when the calling code doesn't have permissions to
                // create a PrintQueue object and deffers the creation to the Avalon Print UI
                // By asserting permissions here, we allow ThunkCloseSpoolFileHandle and ThunkEndDocPrinter
                // to succeed, which is considered Safe in PArtial Trust, once the dialog was opened.
                //
                if (printQueue->InPartialTrust)
                {
                    (gcnew PrintingPermission(PrintingPermissionLevel::DefaultPrinting))->Assert();
                }

                try
                {
                    if (printQueue->PrintingIsCancelled ||
                        bytesToCommit == 0 ||
                        abort)
                    {
                        printerThunkHandler->ThunkAbortPrinter();
                        printQueue->PrintingIsCancelled = false;
                    }
                    else
                    {
                        //
                        // If Xps streaming is enabled, then we'll commit the data to Spooler as we get notifcations
                        // for pages written. In that case bytesToCommit is the sum of number of written bytes since a
                        // last commit operation and get reset to 0 as the data is commited.
                        // If interleaving is not enabled we write the data when the stream is closed,
                        // in which case bytesToCommit should be the total length of the stream.
                        //
                        CommitDataToPrinter();

                        printerThunkHandler->ThunkEndDocPrinter();

                        delete printerThunkHandler;
                        printerThunkHandler = nullptr;
                        jobIdentifier = 0;
                        bytesToCommit = 0;
                        bytesPreviouslyCommited = 0;
                        printerThunkHandler = nullptr;
                    }
                }
                catch (InternalPrintSystemException^ internalException)
                {
                    throw PrintSystemJobInfo::CreatePrintJobException(internalException->HResult,
                                                  "PrintSystemException.PrintSystemJobInfo.Generic");
                }
                __finally
                {
                    if (printQueue->InPartialTrust)
                    {
                        SecurityPermission::RevertAssert();
                    }
                }
            }
        }
        __finally
        {
            this->streamClosed = true;
            System::Threading::Monitor::Exit(accessVerifier);
        }
    }
}

Int32
PrintQueueStream::JobIdentifier::
get(
    void
    )
{
    if (!printQueue->InPartialTrust)
    {
        MS::Internal::PrintWin32Thunk::SecurityHelper::DemandDefaultPrinting();
    }

    if (printerThunkHandler != nullptr)
    {
        jobIdentifier = printerThunkHandler->JobIdentifier;
    }

    return jobIdentifier;
}

Boolean
PrintQueueStream::CanRead::
get(
    void
    )
{
    return true;
}

Boolean
PrintQueueStream::CanWrite::
get(
    void
    )
{
    return true;
}

Boolean
PrintQueueStream::CanSeek::
get(
    void
    )
{
    return true;
}

Int64
PrintQueueStream::Length::
get(
    void
    )
{
    if(printerThunkHandler->SpoolStream)
    {
        return printerThunkHandler->SpoolStream->Length;
    }
    else
    {
        return 0;
    }
}

Int64
PrintQueueStream::Position::
get(
    void
    )
{
    if(printerThunkHandler->SpoolStream)
    {
        return printerThunkHandler->SpoolStream->Position;
    }
    else
    {
        return 0;
    }
}

void
PrintQueueStream::Position::
set(
    Int64     position
    )
{
    printerThunkHandler->SpoolStream->Position = position;
}

void
PrintQueueStream::
SetLength(
    Int64   value
    )
{
    throw gcnew NotSupportedException;
}

void
PrintQueueStream::
Flush(
    )
{
    if (!streamAborted)
    {
        printerThunkHandler->SpoolStream->Flush();
    }
}

void
PrintQueueStream::
HandlePackagingProgressEvent(
    Object^                     sender,
    PackagingProgressEventArgs^ e
    )
{
    //
    // If Xps streaming  is enabled, then we'll commit the data to Spooler as we get notifcations
    // for pages written. In that case bytesToCommit is the sum of number of written bytes since a
    // last commit operation and get reset to 0 as the data is commited.
    // If interleaving is not enabled we write the data when the stream is closed,
    // in which case bytesToCommit should be the total length of the stream.
    //
    if (e->Action == PackagingAction::FixedPageCompleted &&
        commitStreamDataOnClose == false)
    {
        CommitDataToPrinter();
    }

    try
    {
        if (printQueue->InPartialTrust)
        {
            (gcnew PrintingPermission(PrintingPermissionLevel::DefaultPrinting))->Assert();
        }

        try
        {
            printerThunkHandler->ThunkReportJobProgress(jobIdentifier,
                                                        JobOperation::JobProduction,
                                                        e->Action);
        }
        __finally
        {
            if (printQueue->InPartialTrust)
            {
                SecurityPermission::RevertAssert();
            }
        }
    }
    catch(InternalPrintSystemException^ internalException)
    {
        throw PrintSystemJobInfo::CreatePrintJobException(internalException->HResult,
                                      "PrintSystemException.PrintSystemJobInfo.ReportJobProgress");
    }
}

// Dev11:#158013: Warning 4714 (__forceinline function not inlined)
// is expected here because PrintQueueStream::CommitDataToPrinter is marked with [SecurityCritical]
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

void
PrintQueueStream::
CommitDataToPrinter(
    void
    )
{
    Int64 commited  = 0;

    if (printQueue->InPartialTrust)
    {
        (gcnew PrintingPermission(PrintingPermissionLevel::DefaultPrinting))->Assert();
    }

    try
    {
        //
        // The the spool file position to the last position up to which
        // the data was commited. Spooler will move the file pointer to the new position.
        // If we don't do this, the position will be moved by the Spooler beyond the end of the file.
        //

        Int64   previousPositioninFile = 0;

        if(printerThunkHandler->SpoolStream->CanSeek)
        {
            previousPositioninFile = printerThunkHandler->SpoolStream->Position;
            printerThunkHandler->SpoolStream->Position = bytesPreviouslyCommited;
        }

        while (bytesToCommit > 0)
        {
            commited = bytesToCommit;

            printerThunkHandler->ThunkCommitSpoolData((Int32)commited);

            //
            // bytesPreviouslyCommited is updated with the last position up to which the data was comitted.
            //
            bytesPreviouslyCommited += commited;
            bytesToCommit -= commited;
        }

        if(printerThunkHandler->SpoolStream->CanSeek)
        {
            printerThunkHandler->SpoolStream->Position = previousPositioninFile;
        }
    }
    catch (InternalPrintSystemException^ internalException)
    {
        printerThunkHandler->ThunkAbortPrinter();
        if (internalException->HResult == HRESULT_FROM_WIN32(ERROR_CANCELLED) ||
            internalException->HResult == HRESULT_FROM_WIN32(ERROR_PRINT_CANCELLED))
        {
            throw CreatePrintingCanceledException(internalException->HResult,
                                                  "PrintSystemException.PrintingCancelled.Generic");
        }
        else
        {
            throw PrintSystemJobInfo::CreatePrintJobException(internalException->HResult,
                                          "PrintSystemException.PrintSystemJobInfo.Generic");
        }
    }
    __finally
    {
        if (printQueue->InPartialTrust)
        {
            SecurityPermission::RevertAssert();
        }
    }
}
#pragma warning (pop)

__declspec(noinline)
Exception^
PrintQueueStream::CreatePrintingCanceledException(
    int hresult,
    String^ messageId
    )
{
    return gcnew PrintingCanceledException(hresult, messageId);
}

/*-------------------------------------------------------------------------------------------*/
/*                      Implementation of WritePrinterAsyncResult                            */
/*-------------------------------------------------------------------------------------------*/

WritePrinterAsyncResult::
WritePrinterAsyncResult(
    Stream^             stream,
    array<Byte>^        array,
    Int32               offset,
    Int32               numBytes,
    AsyncCallback^      callBack,
    Object^             stateObject
    ) :
    printStream(stream),
    userObject(stateObject),
    userCallBack(callBack),
    dataArray(array),
    dataOffset(offset),
    numberOfBytes(numBytes)
{
    writeCompletedEvent = gcnew AutoResetEvent(false);
}

Object^
WritePrinterAsyncResult::AsyncState::
get(
    void
    )
{
    return userObject;
}

WaitHandle^
WritePrinterAsyncResult::AsyncWaitHandle::
get(
    void
    )
{
    return writeCompletedEvent;
}

bool
WritePrinterAsyncResult::CompletedSynchronously::
get(
    void
    )
{
    return false;
}

bool
WritePrinterAsyncResult::IsCompleted::
get(
    void
    )
{
    return isCompleted;
}

void
WritePrinterAsyncResult::IsCompleted::
set(
    bool    writeCompleted
    )
{
    isCompleted = writeCompleted;
}

AsyncCallback^
WritePrinterAsyncResult::AsyncCallBack::
get(
    void
    )
{
    return userCallBack;
}

void
WritePrinterAsyncResult::
AsyncWrite(
    void
    )
{
    printStream->Write(this->dataArray,
                       this->dataOffset,
                       this->numberOfBytes);

    this->IsCompleted = true;

    writeCompletedEvent->Set();

    if (this->AsyncCallBack)
    {
        this->AsyncCallBack->Invoke(this);
    }
}
