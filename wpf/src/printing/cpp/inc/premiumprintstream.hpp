/*++

    Copyright (C) 2004 - 2005 Microsoft Corporation
    All rights reserved.

    Module Name:

        PremiumPrintStream.hpp

    Abstract:

        Provides a managed stream that allows writing to the Spl file consumed by the Print
        Spooler process.

    Author:

        Ali Naqvi (alinaqvi) - Microsoft 29 2005

    Revision History:

--*/

#ifndef __PREMIUMPRINTSTREAM_HPP__
#define __PREMIUMPRINTSTREAM_HPP__

namespace System
{
namespace Printing
{
    public ref class PrintQueueStream :
    public Stream
    {
        public:

        ///<SecurityNote>
        /// Critical    - Calls helper method InitializePrintStream which asserts DefaultPrinting
        ///               if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical]
        PrintQueueStream(
            PrintQueue^     printQueue,
            String^         printJobName,
            Boolean         commitDataOnClose,
            PrintTicket^    printTicket
            );

        ///<SecurityNote>
        /// Critical    - Calls helper method InitializePrintStream which asserts DefaultPrinting
        ///               if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical]
        PrintQueueStream(
            PrintQueue^     printQueue,
            String^         printJobName,
            Boolean         commitDataOnClose
            );

        ///<SecurityNote>
        /// Critical    - Calls helper method InitializePrintStream which asserts DefaultPrinting
        ///               if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical]
        PrintQueueStream(
            PrintQueue^     printQueue,
            String^         printJobName
            );

        ///<SecurityNote>
        /// Critical    - Asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        ~PrintQueueStream(
            );

        property
        Boolean
        CanRead
        {
            virtual Boolean get() override;
        }

        property
        Boolean
        CanWrite
        {
            virtual Boolean get() override;
        }

        property
        Boolean
        CanSeek
        {
            virtual Boolean get() override;
        }

        property
        virtual
        Int64
        Length
        {
            ///<SecurityNote>
            /// Critical    - This returns the lenght of the spool file stream
            ///               which was obtained as a result of an elevation.
            /// PublicOK    - This is considered safe as the user's own job size is not private data.
            ///</SecurityNote>
            [SecurityCritical]
            virtual Int64 get() override;
        }

        property
        Int64
        virtual
        Position
        {
            ///<SecurityNote>
            /// Critical    - This returns the lenght of the spool file stream represented
            ///               by spoolFileStream, which was obtained as a result of an elevation.
            /// PublicOK    - This is considered safe as the user's own job size is not private data.
            ///</SecurityNote>
            [SecurityCritical]
            virtual Int64 get() override;

            ///<SecurityNote>
            /// Critical    - This returns the lenght of the spool file stream represented
            ///               by spoolFileStream, which was obtained as a result of an elevation.
            /// PublicOK    - This is considered safe as the user's own job size is not private data.
            ///</SecurityNote>
            [SecurityCritical]
            virtual void set(Int64     value) override;
        }

        property
        Int32
        JobIdentifier
        {
            ///<SecurityNote>
            /// Critical    - The jobIdentifier was obtained as a result of an elevation.
            /// PublicOK    - We demand DefaultPrinting which is not granted in PT
            ///</SecurityNote>
            [SecurityCritical]
            Int32 get();
        }

        [Permissions::HostProtection(ExternalThreading=true)]
        virtual
        IAsyncResult^
        BeginWrite(
            array<Byte>^    buffer,
            Int32           offset,
            Int32           count,
            AsyncCallback^  callback,
            Object^         state
            ) override;

        virtual
        void
        EndWrite(
            IAsyncResult^   asyncResult
            ) override;

        ///<SecurityNote>
        /// Critical    - This reads data out of the spool file stream which was obtained as a result of an elevation.
        /// PublicOK    - This is considered safe as the user's own job data is not private data.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        int
        Read(
            array<Byte>^    buffer,
            int             offset,
            int             count
            ) override;

        ///<SecurityNote>
        /// Critical    - This writes data to the spool file stream which was obtained as a result of an elevation.
        /// Critical    - This asserts for DefaultPrinting to cancel the job in Partial Trust
        /// PublicOK    - Both operations are considered safe as the user owns the job.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Write(
            array<Byte>^  buffer,
            int           offset,
            int           count
            ) override;

        ///<SecurityNote>
        /// Critical    - This flushes the spool file stream which was obtained as a result of an elevation.
        /// PublicOK    - This is considered safe as the user owns the job.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Flush(
            ) override;

        ///<SecurityNote>
        /// Critical    - This flushes the spool file stream which was obtained as a result of an elevation.
        /// PublicOK    - This is considered safe as the user owns the job.
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        Int64
        Seek(
            Int64           offset,
            SeekOrigin      origin
            ) override;

        ///<SecurityNote>
        /// Critical    - Calls method AbortOrCancel
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical]
        virtual
        void
        Close(
            void
            ) override;

        virtual
        void
        SetLength(
            Int64   value
            ) override;


        ///<SecurityNote>
        /// Critical    - Asserts DefaultPrinting to report printing progress to the printer device.
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        void
        HandlePackagingProgressEvent(
            Object^                                                       sender,
            System::Windows::Xps::Packaging::PackagingProgressEventArgs^  e
            );

        internal:
        ///<SecurityNote>
        /// Critical    - Calls method AbortOrCancel
        ///</SecurityNote>
        [SecurityCritical]
        void
         Abort(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls helper method InitializePrintStream which asserts DefaultPrinting
        ///               if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical]
        PrintQueueStream(
            PrintQueue^     printQueue,
            String^         printJobName,
            Boolean         commitDataOnClose,
            PrintTicket^    printTicket,
            Boolean         fastCopy
            );

        protected:

        ///<SecurityNote>
        /// Critical    - Asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// PublicOK    - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        !PrintQueueStream(
            );

        private:

        ///<SecurityNote>
        /// Critical    - Helper methoid which asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// TreatAsSafe - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        void
        InitializePrintStream(
            PrintTicket^ printTicket
            );

        ///<SecurityNote>
        /// Critical    - Helper methoid which asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to create the print job and implicitly enable printing in Partial Trust
        /// TreatAsSafe - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecurityCritical, SecurityTreatAsSafe]
        void
        InitializePrintStream(
            PrintTicket^ printTicket,
            Boolean      fastCopy
            );

        ///<SecurityNote>
        /// Critical    - Asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to complete the print job and implicitly enable printing in Partial Trust
        /// TreatAsSafe - This is considered safe, because the PrintQueue has the InPartialTrust set only if
        ///               created by the Avalon Print UI
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        CommitDataToPrinter(
            void
            );

        ///<SecurityNote>
        /// Critical    - Asserts DefaultPrinting if the PrintQueue has the InPartialTrust property set to True
        ///               The assert is needed to complete the print job and implicitly enable printing in Partial Trust
        ///</SecurityNote>
        [SecurityCritical]
        void
        PrintQueueStream::
        AbortOrCancel(
            bool abort
            );

        ///<SecurityNote>
        /// Critical    - References type from non-APTCA reachframework.dll
        /// TreatAsSafe - Type is safe Exception type
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        Exception^
        PrintQueueStream::CreatePrintingCanceledException(
            int hresult,
            String^ messageId
            );

        PrintQueue^     printQueue;

        ///<SecurityNote>
        /// Critical    - When the print job is Partial Trust
        ///               we need to assert for DefaultPrinting
        ///</SecurityNote>
        [SecurityCritical]
        Int32           jobIdentifier;

        /// <summary>
        /// Number of bytes which need to be commited to Print Spooler.
        /// These are the sum of bytes that are being written to the stream for a single page
        /// </summary>
        Int64           bytesToCommit;

        /// <summary>
        /// Keeps track of number of bytes which were commited to Print Spooler.
        /// Represent the position in the stream up to which the data was commited to Spooler.
        /// </summary>
        Int64           bytesPreviouslyCommited;
        /// <summary>
        /// Controls the way the data is commited to Spooler. This is hardcoded so tha data
        /// is always commited on a per page base. WE are keeping this for further extensibility.
        /// </summary>
        Boolean         commitStreamDataOnClose;
        /// <summary>
        /// The name of the print job for which this stream was created.
        /// </summary>
        String^         printJobName;

        /// <summary>
        /// Object closed
        /// </summary>
        Boolean         streamClosed;

        /// <summary>
        /// Stream aborted
        /// </summary>
        Boolean			streamAborted;

        ///<SecurityNote>
        /// Critical    - Type is critical
        ///</SecurityNote>
        [SecurityCritical]
        MS::Internal::PrintWin32Thunk::PrinterThunkHandlerBase^    printerThunkHandler;

        PrintSystemDispatcherObject^    accessVerifier;

        Boolean                             isFinalizer;
    };

    private ref class WritePrinterAsyncResult :
    public IAsyncResult
    {
        public:

        WritePrinterAsyncResult(
            Stream^             stream,
            array<Byte>^        array,
            Int32               offset,
            Int32               numBytes,
            AsyncCallback^      userCallBack,
            Object^             stateObject
            );

        property
        Object^
        AsyncState
        {
            virtual Object^ get();
        }

        property
        System::Threading::WaitHandle^
        AsyncWaitHandle
        {
            virtual System::Threading::WaitHandle^ get();
        }

        property
        AsyncCallback^
        AsyncCallBack
        {
            AsyncCallback^ get();
        }

        property
        bool
        CompletedSynchronously
        {
            virtual bool get();
        }

        property
        bool
        IsCompleted
        {
            virtual bool get();
            void set(bool);
        }

        internal:

        void
        AsyncWrite(
            void
            );

        private:

        Stream^                             printStream;
        Boolean                             isCompleted;
        System::Threading::AutoResetEvent^  writeCompletedEvent;
        AsyncCallback^                      userCallBack;
        Object^                             userObject;
        array<Byte>^                        dataArray;
        Int32                               dataOffset;
        Int32                               numberOfBytes;
    };


}
}

#endif // __PREMIUMPRINTSTREAM_HPP__

