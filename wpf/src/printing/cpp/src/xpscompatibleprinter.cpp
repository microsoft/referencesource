/*++

    Copyright (C) 2002 - 2015 Microsoft Corporation
    All rights reserved.

    Module Name:

        XpsCompatiblePrinter.cpp

    Abstract:

        Managed wrapper for Print Document Package API Interfaces.

    Revision History:

--*/

#include "win32inc.hpp"
#include "Shlwapi.h"
#include <xpsobjectmodel_1.h>

using namespace System;
using namespace System::Collections::Specialized;
using namespace System::Printing;
using namespace System::Security::Permissions;
using namespace System::Runtime::InteropServices;
using namespace System::Threading;
using namespace System::Windows::Xps::Packaging;

#ifndef  __INTEROPNAMESPACEUSAGE_HPP__
#include <InteropNamespaceUsage.hpp>
#endif

#ifndef  __PRINTSYSTEMINTEROPINC_HPP__
#include <PrintSystemInteropInc.hpp>
#endif

#ifndef  __PRINTERDATATYPES_HPP__
#include <PrinterDataTypes.hpp>
#endif

#ifndef  __PRINTSYSTEMINC_HPP__
#include <PrintSystemInc.hpp>
#endif

#ifndef  __XPSPRINTSTREAM_HPP__
#include <XpsPrintStream.hpp>
#endif




///<SecurityNote>
/// Critical    - Sets critical member printerName used to access a printer device to untrusted value
/// TreatAsSafe - this class demands DefaultPrinting.
///</SecurityNote>
[SecuritySafeCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
XpsCompatiblePrinter::
XpsCompatiblePrinter(
String^ printerName
) :
    _printerName(printerName),
    _printDocPackageTarget(nullptr),
    _xpsPackageTarget(nullptr),
    _xpsPackageStatusProvider(nullptr)
{
}

XpsCompatiblePrinter::
~XpsCompatiblePrinter()
{
    AbortPrinter();
    EndDocPrinter();
}

// Warning 4714 (__forceinline function not inlined)
// is expected here because StartDocPrinter is marked with [SecuritySafeCritical]
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
/// Critical    - Calls into the native methods to start a print job
/// TreatAsSafe - This class demands DefaultPrinting
///</SecurityNote>
[SecuritySafeCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
void
XpsCompatiblePrinter::
StartDocPrinter(
    DocInfoThree^ docInfo,
    PrintTicket^ printTicket,
    bool mustSetPrintJobIdentifier
    )
{
    int hr = 0;

    RCW::IPrintDocumentPackageTarget^ tempPrintDocPackageTarget;
    RCW::IXpsDocumentPackageTarget^ tempXpsPackageTarget;

    if (printTicket == nullptr)
    {
        printTicket = gcnew PrintTicket();
    }
    XpsPrintStream^ ticketStream = XpsPrintStream::CreateXpsPrintStream();
    printTicket->SaveTo(ticketStream);

    hr = PresentationNativeUnsafeNativeMethods::PrintToPackageTarget(
        _printerName,
        docInfo->docName,
        ticketStream->GetManagedIStream(),
        tempPrintDocPackageTarget,
        tempXpsPackageTarget);

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

    if (mustSetPrintJobIdentifier)
    {
        _xpsPackageStatusProvider = gcnew RCW::PrintDocumentPackageStatusProvider(tempPrintDocPackageTarget);
    }
    
    _printDocPackageTarget = tempPrintDocPackageTarget;
    _xpsPackageTarget = tempXpsPackageTarget;
}

#pragma warning (pop)

///<SecurityNote>
/// Critical    - Releases printer resources
/// TreatAsSafe - This class demands DefaultPrinting.
///</SecurityNote>
[SecuritySafeCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
void
XpsCompatiblePrinter::
EndDocPrinter(
    void
    )
{
    try
    {
        if (_packageWriter != nullptr)
        {
            _packageWriter->Close();
            Marshal::FinalReleaseComObject(_packageWriter);
            _packageWriter = nullptr;
        }

        if (_printDocPackageTarget != nullptr)
        {
            Marshal::FinalReleaseComObject(_printDocPackageTarget);
            _printDocPackageTarget = nullptr;
        }

        if (_xpsPackageTarget != nullptr)
        {
            Marshal::FinalReleaseComObject(_xpsPackageTarget);
            _xpsPackageTarget = nullptr;
        }
    }
    catch (COMException^)
    {
        throw gcnew PrintingCanceledException();
    }
    catch (ArgumentException^)
    {
        throw gcnew PrintingCanceledException();
    }
}

///<SecurityNote>
/// Critical    - Calls Cancel on critical member printDocPackageTarget
/// TreatAsSafe - This class demands DefaultPrinting.
///</SecurityNote>
[SecuritySafeCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
void
XpsCompatiblePrinter::
AbortPrinter(
    void
    )
{
    try
    {
        if (_printDocPackageTarget != nullptr)
        {
            _printDocPackageTarget->Cancel();
        }

        if (_packageWriter != nullptr)
        {
            // Do not call Close on the packageWriter, if we do we may end up
            // printing the incomplete document instead of canceling
            Marshal::FinalReleaseComObject(_packageWriter);
            _packageWriter = nullptr;
        }
    }
    catch (COMException^)
    {
        throw gcnew PrintingCanceledException();
    }
    catch (ArgumentException^)
    {
        throw gcnew PrintingCanceledException();
    }
}

///<SecurityNote>
/// Critical    - Critical method used to obtain a print job identifier
/// TreatAsSafe - This class demands DefaultPrinting.
///</SecurityNote>
[SecuritySafeCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
int
XpsCompatiblePrinter::JobIdentifier::
get(
    void
    )
{
    int jobId = 0;
    ManualResetEvent^ idEvent = _xpsPackageStatusProvider->JobIdAcquiredEvent;

    if (idEvent != nullptr)
    {
        idEvent->WaitOne();
        return _xpsPackageStatusProvider->JobId;
    }

    return jobId;
}

///<SecurityNote>
/// Critical    - Exposes critical member xpsPackageTarget
///</SecurityNote>
[SecurityCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
RCW::IXpsDocumentPackageTarget^
XpsCompatiblePrinter::XpsPackageTarget::
get(
void
)
{
    return _xpsPackageTarget;
}

///<SecurityNote>
/// Critical    - Sets critical member XpsPackageWriter
///</SecurityNote>
[SecurityCritical]
[System::Drawing::Printing::PrintingPermission(
    SecurityAction::Demand,
    Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
void
XpsCompatiblePrinter::XpsOMPackageWriter::
set(
RCW::IXpsOMPackageWriter^ packageWriter
)
{
    _packageWriter = packageWriter;
}


