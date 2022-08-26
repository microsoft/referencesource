#ifndef __PRINTSYSTEMJOBINFO_HPP__
#define __PRINTSYSTEMJOBINFO_HPP__

/*++

    Copyright (C) 2002 - 2003 Microsoft Corporation
    All rights reserved.

    Module Name:
        PrintSystemJobInfo.hpp

    Abstract:
        This file inlcudes the declaration of the PrintSystemJobInfo
        which is an encapsulation of spooler related operation and properties
        on a Print Job.

    Author:
        Khaled Sedky (khaleds) 10-November-2003

    Revision History:
--*/

using namespace System::Windows::Xps::Packaging;

namespace System
{
namespace Printing
{
    private enum class PrintJobInfoProperty
    {
        Name                        ,
        JobIdentifier               ,
        JobType                     ,
        Submitter                   ,
        Priority                    ,
        PositionInQueue             ,
        StartTimeOfDay              ,
        UntilTimeOfDay              ,
        NumberOfPages               ,
        NumberOfPagesPrinted        ,
        JobSize                     ,
        TimeJobSubmitted            ,
        TimeSinceStartedPrinting    ,
        JobStatus                   ,
        HostingPrintQueue           ,
        HostingPrintServer
    };

    public ref class PrintSystemJobInfo :
    public PrintSystemObject
    {
        internal:

        ///<SecurityNote>
        /// Critical    - Uses type from non-APTCA reachframework.dll (PrintTicket)
        /// PublicOK    - Type use is safe.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrintSystemJobInfo(
            PrintQueue^                 printQueue,
            PrintTicket^                printTicket
            );

        ///<SecurityNote>
        /// Critical    - Uses type from non-APTCA reachframework.dll (PrintTicket)
        /// PublicOK    - Type use is safe.
        ///</SecurityNote>
        [SecuritySafeCritical]
        PrintSystemJobInfo(
            PrintQueue^                 printQueue,
            String^                     jobName,
            PrintTicket^                printTicket
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsDocument)
        ///    Safe 	- Types use is safe
        /// <SecurityNote>
        [SecuritySafeCritical]
        PrintSystemJobInfo(
            PrintQueue^                 printQueue,
            String^                     jobName,
            String^                     documentPath,
            Boolean                     fastCopy,
            PrintTicket^                printTicket
            );

        PrintSystemJobInfo(
            PrintQueue^                 printQueue,
            Int32                       jobIdentifier
            );

        //
        // This class constructor is intended for
        // browsable Print Objects. To give an example,
        // this is instantiated when enumerating
        // PrintJobs on a PrintQueue
        PrintSystemJobInfo(
            PrintQueue^                 printQueue,
            array<String^>^             propertyFilter
            );

        ///<SecurityNote>
        /// Critical    - Uses type from non-APTCA reachframework.dll (PrintTicket)
        /// PublicOK    - Type use is safe.
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        PrintSystemJobInfo^
        Add(
            PrintQueue^     printQueue,
            PrintTicket^    printTicket
            );

        ///<SecurityNote>
        /// Critical    - Uses type from non-APTCA reachframework.dll (PrintTicket)
        /// PublicOK    - Type use is safe.
        ///</SecurityNote>
        [SecuritySafeCritical]
        static
        PrintSystemJobInfo^
        Add(
            PrintQueue^     printQueue,
            String^         jobName,
            PrintTicket^    printTicket
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsDocument)
        ///    Safe 	- Types use is safe
        /// <SecurityNote>
        [SecuritySafeCritical]
        static
        PrintSystemJobInfo^
        Add(
            PrintQueue^         printQueue,
            String^             jobName,
            String^             documentPath,
            Boolean             fastCopy,
            PrintTicket^        printTicket
            );

        public:

        static
        PrintSystemJobInfo^
        Get(
            PrintQueue^     printQueue,
            Int32           jobIdentifier
            );

        ///<SecurityNote>
        /// Critical    - Calls ThunkSetJob which in turn calls UnsafeNativeMethods::InvokeSetJob
        ///               which has SUC applied
        /// TreatAsSafe - ThunkSetJob demands DefaultPrinting.
        void
        Pause(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls ThunkSetJob which in turn calls UnsafeNativeMethods::InvokeSetJob
        ///               which has SUC applied
        /// TreatAsSafe - ThunkSetJob demands DefaultPrinting.
        void
        Resume(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls ThunkSetJob which in turn calls UnsafeNativeMethods::InvokeSetJob
        ///               which has SUC applied
        /// TreatAsSafe - ThunkSetJob demands DefaultPrinting.
        void
        Cancel(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls ThunkSetJob which in turn calls UnsafeNativeMethods::InvokeSetJob
        ///               which has SUC applied
        /// TreatAsSafe - ThunkSetJob demands DefaultPrinting.
        void
        Restart(
            void
            );

        property
        Stream^
        JobStream
        {
            Stream^ get();
        }

        property
        Int32
        JobIdentifier
        {
            public:
                Int32 get();
            internal:
                void set(Int32 newJobIdentifier);
        }

        property
        String^
        Submitter
        {
            public:
                String^ get();
            internal:
                void set(String^     newSubmitter);
        }

        property
        PrintJobPriority
        Priority
        {
            public:
                PrintJobPriority get();
            internal:
                void set(PrintJobPriority newPriority);
        }

        property
        Int32
        PositionInPrintQueue
        {
            public:
                Int32 get();
            internal:
                void set(Int32 positionInQueue);
        }

        property
        Int32
        StartTimeOfDay
        {
            public:
                Int32 get();
            internal:
                void set(Int32 newStartTime);
        }

        property
        Int32
        UntilTimeOfDay
        {
            public:
                Int32 get();
            internal:
                void set(Int32 newUntilTime);
        }

        property
        Int32
        NumberOfPages
        {
            public:
                Int32 get();
            internal:
                void set(Int32   newNumberOfPages);
        }

        property
        Int32
        NumberOfPagesPrinted
        {
            public:
                Int32 get();
            internal:
                void set(Int32   newNumberOfPagesPrinted);
        }

        property
        Int32
        JobSize
        {
            public:
                Int32 get();
            internal:
                void set(Int32   newJobSize);
        }

        property
        DateTime
        TimeJobSubmitted
        {
            public:
                DateTime get();
            internal:
                void set(DateTime    newTimeJobSubmitted);
        }

        property
        Int32
        TimeSinceStartedPrinting
        {
            Int32 get();
        }

        property
        PrintJobStatus
        JobStatus
        {
            public:
                PrintJobStatus get();
            internal:
                void set(PrintJobStatus  status);
        }

        property
        Boolean
        IsCompleted
        {
            Boolean get();
        }

        property
        Boolean
        IsDeleting
        {
            Boolean get();
        }

        property
        Boolean
        IsPaused
        {
            Boolean get();
        }

        property
        Boolean
        IsPrinted
        {
            Boolean get();
        }

        property
        Boolean
        IsRestarted
        {
            Boolean get();
        }

        property
        Boolean
        IsSpooling
        {
            Boolean get();
        }

        property
        Boolean
        IsInError
        {
            Boolean get();
        }

        property
        Boolean
        IsPrinting
        {
            Boolean get();
        }

        property
        Boolean
        IsOffline
        {
            Boolean get();
        }

        property
        Boolean
        IsPaperOut
        {
            Boolean get();
        }

        property
        Boolean
        IsDeleted
        {
            Boolean get();
        }

        property
        Boolean
        IsBlocked
        {
            Boolean get();
        }

        property
        Boolean
        IsUserInterventionRequired
        {
            Boolean get();
        }

        property
        Boolean
        IsRetained
        {
            Boolean get();
        }

        property
        String^
        JobName
        {
            String^ get();
            void set(String^ newJobName);
        }

        property
        PrintQueue^
        HostingPrintQueue
        {
            public:
                PrintQueue^ get();
            internal:
                void set(PrintQueue^ printQueue);
        }

        property
        PrintServer^
        HostingPrintServer
        {
            public:
                PrintServer^ get();
            internal:
                void set(PrintServer^ printServer);
        }

        virtual
        void
        Commit(
            void
            ) override;

        virtual
        void
        Refresh(
            void
            ) override;

        protected:

        virtual
        void
        InternalDispose(
            bool disposing
            ) override sealed;

        internal:

        property
        Int32
        PrioritySecondary
        {
            void set(Int32 newPrioritySecondary);
        }

        property
        Int32
        JobStatusSecondary
        {
            void set(Int32  status);
        }

        property
        Boolean
        DownLevelSystem
        {
            Boolean get();
            void set(Boolean    value);
        }

        virtual
        PrintPropertyDictionary^
        get_InternalPropertiesCollection(
            String^ attributeName
            ) override;

        static
        PrintProperty^
        CreateAttributeNoValue(
            String^ attributeName
            );

        static
        PrintProperty^
        CreateAttributeValue(
            String^ attributeName,
            Object^ attributeValue
            );

        static
        PrintProperty^
        CreateAttributeNoValueLinked(
            String^             attributeName,
            MulticastDelegate^  delegate
            );

        static
        PrintProperty^
        CreateAttributeValueLinked(
            String^             attributeName,
            Object^             attributeValue,
            MulticastDelegate^  delegate
            );

        static
        void
        RegisterAttributesNamesTypes(
            void
            );

        static
        array<String^>^
        GetAllPropertiesFilter(
            void
            );

        static
        PrintSystemObject^
        Instantiate(
            Object^             printQueue,
            array<String^>^     propertiesFilter
            );

        static
        String^             defaultJobName;

        private:

        array<MulticastDelegate^>^
        CreatePropertiesDelegates(
            void
            );

        void
        Initialize(
            void
            );

        void
        InitializeInternalCollections(
            void
            );

        void
        PopulateJobProperties(
            array<String^>^     propertiesAsStrings
            );

        static
        PrintSystemJobInfo(
            void
            )
        {
            attributeNameTypes        = gcnew Hashtable();
            upLevelToDownLevelMapping = gcnew Hashtable();

            defaultJobName     = gcnew String("Print System Document");

            //
            // Map upLevel properties to downLevel properties
            //
            if(upLevelAttributeName->Length != secondaryAttributeNames->Length)
            {
                InternalExceptionResourceManager^ manager = gcnew InternalExceptionResourceManager();

                String^ resString = manager->GetString("IndexOutOfRangeException.InvalidSize",
                                                       System::Threading::Thread::CurrentThread->CurrentUICulture);

                String^ exceptionMessage = String::Format(System::Threading::Thread::CurrentThread->CurrentUICulture,
                                                          resString,
                                                          "upLevelAttributeName",
                                                          "secondaryAttributeNames");

                throw gcnew IndexOutOfRangeException(exceptionMessage);
            }

            for(Int32 numOfMappings = 0;
                numOfMappings < upLevelAttributeName->Length;
                numOfMappings++)
            {
                upLevelToDownLevelMapping->Add(upLevelAttributeName[numOfMappings],
                                               secondaryAttributeNames[numOfMappings]);
            }
        }

        void
        CopyFileStreamToPrinter(
            String^         xpsFileName,
            Stream^         printStream
            );

        void
        VerifyAccess(
            void
            );

        internal:

        /// <SecurityNote>
        ///     Critical    : Creates critical type from non-APTCA reachframework.dll
        ///     TreatAsSafe : type is safe exception type
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        Exception^
        PrintSystemJobInfo::CreatePrintJobException(
            String^ messageId
            );

        /// <SecurityNote>
        ///     Critical    : Creates critical type from non-APTCA reachframework.dll
        ///     TreatAsSafe : type is safe exception type
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        Exception^
        PrintSystemJobInfo::CreatePrintJobException(
            int hresult,
            String^ messageId
            );

        /// <SecurityNote>
        ///     Critical    : Creates critical type from non-APTCA reachframework.dll
        ///     TreatAsSafe : type is safe exception type
        /// </SecurityNote>
        [SecuritySafeCritical]
        static
        Exception^
        PrintSystemJobInfo::CreatePrintJobException(
            String^ messageId,
            Exception^ innerException
            );

        private:
        /// <SecurityNote>
        ///     Critical    : Uses type from non-APTCA reachframework.dll
        ///     TreatAsSafe : type is safe
        /// </SecurityNote>
        [SecuritySafeCritical]
        bool
        IsErrorInvalidParameter(
            int hResult
            );

        PrintQueueStream^   printStream;

        Int32               jobIdentifier;
        String^             submitter;
        PrintJobPriority    priority;
        Int32               positionInPrintQueue;
        Int32               startTime;
        Int32               untilTime;
        Int32               numberOfPages;
        Int32               numberOfPagesPrinted;
        Int32               jobSize;
        DateTime            timeJobSubmitted;
        Int32               timeSinceStartedPrinting;
        PrintJobStatus      jobStatus;
        Boolean             isCompleted;
        Boolean             isDeleting;
        Boolean             isPaused;
        Boolean             isPrinted;
        Boolean             isRestarted;
        Boolean             isSpooling;
        Boolean             isInError;
        Boolean             isPrinting;
        Boolean             isOffline;
        Boolean             isPaperOut;
        Boolean             isDeleted;
        Boolean             isBlocked;
        Boolean             isUserInterventionRequired;
        Boolean             isRetained;
        String^             jobName;
        PrintQueue^         hostingPrintQueue;
        PrintServer^        hostingPrintServer;

        static
        Hashtable^          attributeNameTypes;
        Boolean             isDownLevelSystem;

        PrintSystemDispatcherObject^    accessVerifier;

        //
        // The following is the necessary data members to link the
        // compile time properties with the named properties in the
        // associated collection
        //
        static array<String^>^ primaryAttributeNames =
        {
            //S"Name",
            "JobIdentifier",
            //"JobType",
            "Submitter",
            "Priority",
            "PositionInQueue",
            "StartTimeOfDay",
            "UntilTimeOfDay",
            "NumberOfPages",
            "NumberOfPagesPrinted",
            "JobSize",
            "TimeJobSubmitted",
            "TimeSinceStartedPrinting",
            "JobStatus",
            "HostingPrintQueue",
            "HostingPrintServer"
        };

        static array<Type^>^ primaryAttributeTypes =
        {
            //String::typeid,
            Int32::typeid,
            //PrintJobType::typeid,
            String::typeid,
            PrintJobPriority::typeid,
            Int32::typeid,
            Int32::typeid,
            Int32::typeid,
            Int32::typeid,
            Int32::typeid,
            Int32::typeid,
            DateTime::typeid,
            Int32::typeid,
            PrintJobStatus::typeid,
            PrintQueue::typeid,
            PrintServer::typeid
        };

        static array<String^>^ secondaryAttributeNames =
        {
            "JobPriority",
            "Status",
            "PrintQueue",
            "PrintServer"
        };

        static array<Type^>^ secondaryAttributeTypes =
        {
            Int32::typeid,
            Int32::typeid,
            String::typeid,
            String::typeid
        };

        static array<String^>^ upLevelAttributeName =
        {
            "Priority",
            "JobStatus",
            "HostingPrintQueue",
            "HostingPrintServer"
        };

        static Hashtable^ upLevelToDownLevelMapping;

        Hashtable^                              collectionsTable;
        PrintPropertyDictionary^                thunkPropertiesCollection;

        array<String^>^                         refreshPropertiesFilter;
        Boolean                                 reportProgress;
    };

    public ref class PrintJobInfoCollection :
    public PrintSystemObjects,
    public System::Collections::Generic::IEnumerable<PrintSystemJobInfo^>,
    public IEnumerable,public IDisposable
    {
        public:

        PrintJobInfoCollection(
            PrintQueue^         printQueue,
            array<String^>^     propertyFilter
            );

        ~PrintJobInfoCollection(
            void
            );

        virtual System::Collections::IEnumerator^ GetNonGenericEnumerator() = System::Collections::IEnumerable::GetEnumerator;


        virtual
        System::
        Collections::
        Generic::
        IEnumerator<PrintSystemJobInfo^>^
        GetEnumerator(
            void
            );


        void
        Add(
            PrintSystemJobInfo^ printObject
            );

        private:

        PrintJobInfoCollection(
            void
            )
        {
        }

        void
        VerifyAccess(
            void
            );



        System::Collections::Generic::
        Queue<PrintSystemJobInfo^>^          jobInfoCollection;
        PrintQueue^                          hostingPrintQueue;
        PrintSystemDispatcherObject^    accessVerifier;

    };
}
}

#endif

