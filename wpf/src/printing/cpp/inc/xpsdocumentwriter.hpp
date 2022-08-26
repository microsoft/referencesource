#ifndef __XPSDOCUMENTWRITER_HPP__
#define __XPSDOCUMENTWRITER_HPP__
/*++

    Copyright (C) 2004 - 2005 Microsoft Corporation
    All rights reserved.

    Module Name:

        XPSDocumentWriter.hpp

    Abstract:

        This object is instantiated against an XPSEmitter object. It is a public object to be used
        to serialize Visuals to Print Subsystem objeects.

    Author:

        Ali Naqvi (alinaqvi) - 22nd May 2005

    Revision History:
--*/
using namespace System::Runtime::Serialization;
using namespace System::Windows::Xps::Packaging;
using namespace System::Windows::Xps::Serialization;
using namespace System::IO;
using namespace System::IO::Packaging;
using namespace System::Windows::Documents::Serialization;

namespace System
{
namespace Windows
{
namespace Xps
{

    [AttributeUsage(
        AttributeTargets::Class |
        AttributeTargets::Property |
        AttributeTargets::Method |
        AttributeTargets::Struct |
        AttributeTargets::Enum |
        AttributeTargets::Interface |
        AttributeTargets::Delegate |
        AttributeTargets::Constructor,
        AllowMultiple = false,
        Inherited = true)
    ]

    private ref class FriendAccessAllowedAttribute sealed : Attribute
    {
    };

    ref class VisualsToXpsDocument;

    public enum class XpsDocumentNotificationLevel
    {
        None                        = 0,
        ReceiveNotificationEnabled  = 1,
        ReceiveNotificationDisabled = 2
    };


    ///<SecurityNote>
    /// Most public methods on this type are critical. They provide access to printer devices.
    /// In order for these methods to be PublicOK
    /// any other public methods that expose an instance of XpsDocumentWriter must
    /// first prompt the user with the print dialog.
    ///</SecurityNote>
    public ref class XpsDocumentWriter: public SerializerWriter
    {

    internal:

        /// <summary>
        /// Instantiates a <c>XpsDocumentWriter</c> against an object implementing <c>XPSEmitter</c>.
        /// </summary>
        /// <param name="serializeReach"><c>XPSEmitter</c> object that will serialize and write the document objects.</param>
        ///<SecurityNote>
        /// Critical    - Accesses types from non-APTCA reachframework.dll         //            (MXDWSerializationManager, XpsDocument, IXpsFixedDocumentSequenceReader, PrintTicket, PrintTicketLevel)
        /// TreatAsSafe - Demands default printing
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         System::Security::Permissions::SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        [FriendAccessAllowed]
        XpsDocumentWriter(
            PrintQueue^    printQueue
            );

        /// <summary>
        /// Instantiates a <c>XpsDocumentWriter</c> against an object implementing <c>XPSEmitter</c>.
        /// </summary>
        /// <param name="serializeReach"><c>XPSEmitter</c> object that will serialize and write the document objects.</param>
        ///<SecurityNote>
        /// Critical    - Accesses types from non-APTCA reachframework.dll        //            (MXDWSerializationManager, XpsDocument, IXpsFixedDocumentSequenceReader, PrintTicket, PrintTicketLevel)
        /// TreatAsSafe - Demands default printing
        ///</SecurityNote>
        [SecuritySafeCritical]
        [System::Drawing::Printing::PrintingPermission(
         System::Security::Permissions::SecurityAction::Demand,
         Level = System::Drawing::Printing::PrintingPermissionLevel::DefaultPrinting)]
        [FriendAccessAllowed]
        XpsDocumentWriter(
            XpsDocument^    document
            );

    internal:

        /// <summary>
        /// Instantiates a <c>XpsDocumentWriter</c> against an object implementing <c>XPSEmitter</c>.
        /// </summary>
        /// <param name="serializeReach"><c>XPSEmitter</c> object that will serialize and write the document objects.</param>
        /// <param name="bogus"><c>Bogus</c> Bogus param to have a second internal constructor.</param>
        ///<SecurityNote>
        /// Critical - Internal constructor to be used in Partial Trust only. Caller to check emitter's validity.
        ///</SecurityNote>
        [SecurityCritical]
        XpsDocumentWriter(
            PrintQueue^     printQueue,
            Object^         bogus
            );

        /// <summary>
        /// Writes a <c>FixedDocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocumentSequence"><c>FixedDocumentSequence</c> object to be written.</param>
        /// <param name="printJobIdentifier">Print Job identifier.</param>
        /// <SecurityNote>
        /// Critical:      Although it returns the print job id, considerent critical information in Partial trust
        /// TreatAsSafe  : The caller needs DefaultPrinting permission to get the print job id from serialization manager.
        ///                See the PackageSerializationManager.JobIdentifier property demanding in wcp/Print/Reach/Serialization/manager/MetroSerializationManager.cs
        /// </SecurityNote>
        [SecuritySafeCritical]
        void
        BeginPrintFixedDocumentSequence(
            System::Windows::Documents::FixedDocumentSequence^      fixedDocumentSequence,
            Int32&                                                  printJobIdentifier
            );

        /// <summary>
        /// Writes a <c>FixedDocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocumentSequence"><c>FixedDocumentSequence</c> object to be written.</param>
        /// <param name="printTicket">Print ticket.</param>
        /// <param name="printJobIdentifier">Print Job identifier.</param>
        /// <SecurityNote>
        /// Critical:      Although it returns the print job id, considerent critical information in Partial trust
        /// TreatAsSafe  : The caller needs DefaultPrinting permission to get the print job id from serialization manager.
        ///                See the PackageSerializationManager.JobIdentifier property demanding in wcp/Print/Reach/Serialization/manager/MetroSerializationManager.cs
        /// </SecurityNote>
        [SecuritySafeCritical]
        void
        BeginPrintFixedDocumentSequence(
            System::Windows::Documents::FixedDocumentSequence^      fixedDocumentSequence,
            PrintTicket^                                            printTicket,
            Int32&                                                  printJobIdentifier
            );

        /// <summary>
        /// Dispose objects and deletes the print job.
        /// </summary>
        /// <SecurityNote>
        ///    Critical - Calls critical EndWrite
        ///</SecurityNote>
        [SecurityCritical]
        void
        EndPrintFixedDocumentSequence(
            void
            );

    public:

        /// <summary>
        /// Writes an <c>XpsDocument</c> to the destination object.
        /// </summary>
        /// <param name="documentPath"><c>XpsDocument</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsDocument, IXpsFixedDocumentSequenceReader
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        Write(
            String^      documentPath
            );

        /// <summary>
        /// Writes an <c>XpsDocument</c> to the destination object.
        /// </summary>
        /// <param name="documentPath"><c>XpsDocument</c> object to be written.</param>
        /// <param name="notificationLevel"><c>XpsDocumentNotificationLevel</C>
        /// granularity of notification. if ReceiveNotificationEnabled is set, then the document would be re-serialized and extented
        /// XPS content can't be preserved.
        /// </param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsDocument, IXpsFixedDocumentSequenceReader        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        Write(
            String^                         documentPath,
            XpsDocumentNotificationLevel    notificationLevel
            );

        /// <summary>
        /// Writes an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicketLevel)
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::DocumentPaginator^      documentPaginator
            ) override;

        /// <aummary>
        /// Writes an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::DocumentPaginator^      documentPaginator,
            PrintTicket^                                        printTicket
            ) override;

        /// <summary>
        /// Writes a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Media::Visual^     visual
            ) override;

        /// <summary>
        /// Writes a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket
            ) override;

        /// <summary>
        /// Writes a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedDocumentSequence^      fixedDocumentSequence
            ) override;

        /// <summary>
        /// Writes a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedDocumentSequence^      fixedDocumentSequence,
            PrintTicket^                                            printTicket
            ) override;

        /// <summary>
        /// Writes a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedDocument^      fixedDocument
            ) override;

        /// <summary>
        /// Writes a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedDocument^      fixedDocument,
            PrintTicket^                                    printTicket
            ) override;

        /// <summary>
        /// Writes a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedPage^          fixedPage
            ) override;

        /// Writes a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Documents::FixedPage^          fixedPage,
            PrintTicket^                                    printTicket
            ) override;


        /// <summary>
        /// Asynchronously Writes an <c>XpsDocument</c> to the destination object.
        /// </summary>
        /// <param name="documentPath"><c>XpsDocument</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        WriteAsync(
            String^      documentPath
            );

        /// <summary>
        /// Asynchronously Writes an <c>XpsDocument</c> to the destination object.
        /// </summary>
        /// <param name="documentPath"><c>XpsDocument</c> object to be written.</param>
        /// <param name="notificationLevel"><c>XpsDocumentNotificationLevel</C>
        /// granularity of notification. if ReceiveNotificationEnabled is set, then the document would be re-serialized and extented
        /// XPS content can't be preserved.
        /// </param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        WriteAsync(
            String^                         documentPath,
            XpsDocumentNotificationLevel    notificationLevel
            );

        /// <summary>
        /// Asynchronously write an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::DocumentPaginator^      documentPaginator
            ) override;

        /// <summary>
        /// Asynchronously write an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::DocumentPaginator^      documentPaginator,
            PrintTicket^                                        printTicket
            ) override;

        /// <summary>
        /// Asynchronously write an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::DocumentPaginator^      documentPaginator,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write an <c>DocumentPaginator</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentPaginator"><c>DocumentPaginator</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::DocumentPaginator^      documentPaginator,
            PrintTicket^                                        printTicket,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual
            ) override;

        /// <summary>
        /// Asynchronously write a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket
            ) override;

        /// <summary>
        /// Asynchronously write a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            Object^                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>Visual</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="visual"><c>Visual</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket,
            Object^                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocumentSequence^  fixedDocumentSequence
            ) override;

        /// <summary>
        /// Asynchronously write a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocumentSequence^  fixedDocumentSequence,
            PrintTicket^                                        printTicket
            ) override;

        /// <summary>
        /// Asynchronously write a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocumentSequence^  fixedDocumentSequence,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>DocumentSequence</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="documentSequence"><c>DocumentSequence</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocumentSequence^  fixedDocumentSequence,
            PrintTicket^                                        printTicket,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocument^          fixedDocument
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocument^          fixedDocument,
            PrintTicket^                                        printTicket
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocument^          fixedDocument,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedDocument</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedDocument"><c>FixedDocument</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedDocument^          fixedDocument,
            PrintTicket^                                        printTicket,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedPage^              fixedPage
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedPage^              fixedPage,
            PrintTicket^                                        printTicket
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedPage^              fixedPage,
            Object^                                             userSuppliedState
            ) override;

        /// <summary>
        /// Asynchronously write a <c>FixedPage</c> to the <c>XPSEmitter</c> object.
        /// </summary>
        /// <param name="fixedPage"><c>FixedPage</c> object to be written.</param>
        /// <param name="printTicket"><c>PrintTicket</c> to apply to object.</param>
        /// <param name="userSuppliedState">User supplied object.</param>
        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicket)
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Documents::FixedPage^              fixedPage,
            PrintTicket^                                        printTicket,
            Object^                                             userSuppliedState
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///               (XpsSerializationManagerAsync, NgcSerializationManagerAsync)
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        CancelAsync(
            ) override;

        /// <summary>
        /// Creates and returns the <c>VisualsToXpsDocument</c> visuals collater for batch writing.
        /// </summary>
        /// <returns><c>VisualsToXpsDocument</c></returns>
        /// <SecurityNote>
        ///    Critical - Calls critical VisualsToXpsDocument ctor
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        SerializerWriterCollator^
        CreateVisualsCollator(
            ) override;
        /*++

            Function Name:
                CreateVisualsCollator

            Description:
                Creates and returns a VisualsToXPSDocument visuals collater for batch writing.

            Parameters:

                documentSequencePrintTicket     -   PrintTicket to use on the FixedDocumentSequence
                documentPrintTicket             -   PrintTicket to use on the FixedDocument

            Return Value:

                VisualsToXpsDocument

        --*/
        /// <SecurityNote>
        ///    Critical - Calls critical VisualsToXpsDocument ctor
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        SerializerWriterCollator^
        XpsDocumentWriter::
        CreateVisualsCollator(
            PrintTicket^    documentSequencePrintTicket OPTIONAL,
            PrintTicket^    documentPrintTicket         OPTIONAL
            ) override;

        event WritingPrintTicketRequiredEventHandler^ WritingPrintTicketRequired
        {
            public:

            virtual
            void
            add(
                WritingPrintTicketRequiredEventHandler^   handler
                ) override
            {
                _WritingPrintTicketRequired+=handler;
            }

            virtual
            void
            remove(
                WritingPrintTicketRequiredEventHandler^    handler
                ) override
            {
                _WritingPrintTicketRequired-=handler;
            }

            virtual
            void
            raise(
                Object^                                sender,
                 WritingPrintTicketRequiredEventArgs^   e
                 )
            {
                _WritingPrintTicketRequired(sender,e);
            }


        }

        event WritingProgressChangedEventHandler^     WritingProgressChanged
         {
            public:

            virtual
            void
            add(
                WritingProgressChangedEventHandler^   handler
                ) override
            {
                _WritingProgressChanged+=handler;
            }

            virtual
            void
            remove(
                WritingProgressChangedEventHandler^    handler
                ) override
            {
                _WritingProgressChanged-=handler;
             }

            virtual
            void
            raise(
                Object^                                sender,
                 WritingProgressChangedEventArgs^   e
                 )
            {
                _WritingProgressChanged(sender,e);
            }
        }

        event WritingCompletedEventHandler^           WritingCompleted
        {
            public:

            virtual
            void
            add(
                WritingCompletedEventHandler^   handler
                ) override
            {
                _WritingCompleted+=handler;
            }

            virtual
            void
            remove(
                WritingCompletedEventHandler^    handler
                ) override
            {
                _WritingCompleted-=handler;
             }

            virtual
            void
            raise(
                Object^                                sender,
                 WritingCompletedEventArgs^   e
                 )
            {
                _WritingCompleted(sender,e);
            }
        }

        event WritingCancelledEventHandler^           WritingCancelled
        {
            public:

            virtual
            void
            add(
                WritingCancelledEventHandler^   handler
                ) override
            {
                _WritingCancelled+=handler;
                _writingCancelledEventHandlersCount++;
            }

            virtual
            void
            remove(
                WritingCancelledEventHandler^    handler
                ) override
            {
                if(_writingCancelledEventHandlersCount>0)
                {
                    _WritingCancelled-=handler;
                    _writingCancelledEventHandlersCount--;
                }
                else
                {
                    throw gcnew InvalidOperationException();
                }
            }


            virtual
            void
            raise(
                Object^                     sender,
                WritingCancelledEventArgs^  args
                )
            {
                _WritingCancelled(sender,args);
            }
        }


    internal:

        event WritingPrintTicketRequiredEventHandler^ _WritingPrintTicketRequired
        {
            internal:

            void
            add(
                WritingPrintTicketRequiredEventHandler^   handler
                )
            {
                m_WritingPrintTicketRequired += handler;
            }

            void
            remove(
                WritingPrintTicketRequiredEventHandler^    handler
                )
            {
                m_WritingPrintTicketRequired -= handler;
            }

            /// <SecurityNote>
            ///  Critical    - Eventhandler signature uses type from non-APTCA reachframework.dll (PrintTicket, PrintTicketLevel)
            ///  TreatAsSafe - Type is safe
            ///</SecurityNote>
            [SecuritySafeCritical]
            void
            raise(
                Object^                                sender,
                 WritingPrintTicketRequiredEventArgs^   e
                 )
            {
                WritingPrintTicketRequiredEventHandler^ handler = m_WritingPrintTicketRequired;
                if(handler != nullptr)
                {
                    handler(sender,e);
                }
            }
        }

        event WritingProgressChangedEventHandler^ _WritingProgressChanged;
        event WritingCompletedEventHandler^       _WritingCompleted;
        event WritingCancelledEventHandler^ _WritingCancelled;

    internal:
        /// <SecurityNote>
        ///  Critical    - Uses type from non-APTCA reachframework.dll (XpsSerializationPrintTicketRequiredEventArgs)
        /// <SecurityNote>
        [SecurityCritical]
        void
        ForwardUserPrintTicket(
            Object^                                                                               sender,
            System::Windows::Xps::Serialization::XpsSerializationPrintTicketRequiredEventArgs^    args
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsSerializationCompletedEventArgs)
        /// <SecurityNote>
        [SecurityCritical]
        void
        ForwardWriteCompletedEvent(
            Object^                                                                   sender,
            System::Windows::Xps::Serialization::XpsSerializationCompletedEventArgs^  args
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsSerializationProgressChangedEventArgs)
        /// <SecurityNote>
        [SecurityCritical]
        void
        ForwardProgressChangedEvent(
            Object^                                                                           sender,
            System::Windows::Xps::Serialization::XpsSerializationProgressChangedEventArgs^    args
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsWritingProgressChangeLevel        /// <SecurityNote>
        [SecurityCritical]
        WritingProgressChangeLevel
        TranslateProgressChangeLevel(
            System::
            Windows::
            Xps::Serialization::XpsWritingProgressChangeLevel xpsChangeLevel
            );

        /// <SecurityNote>
        ///  Critical    - Uses type from non-APTCA reachframework.dll (PrintTicket)
        ///  TreatAsSafe - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        void
        CloneSourcePrintTicket(
            Object^                                                                               sender,
            System::Windows::Xps::Serialization::XpsSerializationPrintTicketRequiredEventArgs^    args
            );

        void
        EndBatchMode(
            void
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PackageSerializationManager, XpsSerializationPrintTicketRequiredEventHandler)
        /// <SecurityNote>
        [SecurityCritical]
        void
        SetPrintTicketEventHandler(
            System::Windows::Xps::Serialization::PackageSerializationManager^ manager,
            XpsSerializationPrintTicketRequiredEventHandler^                  eventHandler
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PackageSerializationManager)
        /// <SecurityNote>
        [SecurityCritical]
        void
        SetCompletionEventHandler(
            System::Windows::Xps::Serialization::PackageSerializationManager^ manager,
            Object^                                                           userState
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PackageSerializationManager)
        /// <SecurityNote>
        [SecurityCritical]
        void
        SetProgressChangedEventHandler(
            System::Windows::Xps::Serialization::PackageSerializationManager^ manager,
            Object^                                                           userState
            );

        property
        PrintTicket^
            CurrentUserPrintTicket
        {
            /// <SecurityNote>
            ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicket)
            ///    Safe 	- Type is safe
            /// <SecurityNote>
            [SecuritySafeCritical]
            void set(PrintTicket^ userPrintTicket);
        }

        property
        System::Windows::Xps::Serialization::PrintTicketLevel
        CurrentWriteLevel
        {
            /// <SecurityNote>
            ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicketLevel)
            ///    Safe 	- Type is safe
            /// <SecurityNote>
            [SecuritySafeCritical]
             void set(System::Windows::Xps::Serialization::PrintTicketLevel writeLevel);
        }

        void
        OnWritingPrintTicketRequired(
            Object^                                sender,
            WritingPrintTicketRequiredEventArgs^   args
            );

        bool
        OnWritingCanceled(
            Object^     sender,
            Exception^  exception
            );


        private:

        void
        InitializeSequences(
            void
            );

        ///<SecurityNote>
        /// Critical    - Calls CreateXPSDocument when printing to MXDW driver
        /// TreatAsSafe - Only creates files at path passed back from the MXDW driver which is trusted
        ///</SecurityNote>
        [SecuritySafeCritical]
        bool
        BeginWrite(
            bool                batchMode,
            bool                asyncMode,
            bool                setPrintTicketHandler,
            PrintTicket^        printTicket,
            System::Windows::
            Xps::
            Serialization::
            PrintTicketLevel    printTicketLevel,
            bool                printJobIdentifierSet
            );

        /// <SecurityNote>
        ///    Critical - Calls critical EndWrite
        /// <SecurityNote>
        [SecurityCritical]
        void
        EndWrite(
            bool disposeManager
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///               (MXDWSerializationManager, PrintingCanceledException)
        /// <SecurityNote>
        [SecurityCritical]
        void
        EndWrite(
            bool disposeManager,
            bool abort
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PackageSerializationManager, PrintingCanceledException)
        /// <SecurityNote>
        [SecurityCritical]
        void
        SaveAsXaml(
            Object^     serializedObject,
            bool        isSync
            );

        ///<SecurityNote>
        /// Critical    - When running in PT, we need to evelate to determine if the driver is Mxdw
        /// TreatAsSafe - the driver name is not handed outside of the method, besides it's not considered to be critical in PT.
        ///</SecurityNote>
        [SecuritySafeCritical]
        bool
        MxdwConversionRequired(
            PrintQueue^  printQueue
            );

        ///<SecurityNote>
        /// Critical    - Returns User selected path from MXDW driver
        ///</SecurityNote>
        [SecurityCritical]
        String^
        MxdwInitializeOptimizationConversion(
            PrintQueue^ printQueue
            );

        ///<SecurityNote>
        /// Critical    - Will create an arbitrary file at the passed in file name
        ///</SecurityNote>
        [SecurityCritical]
        void
        CreateXPSDocument(
            String^     documentName
            );

        void
        VerifyAccess(
            void
            );


        private:

        enum class DocumentWriterState
        {
            kRegularMode,
            kBatchMode,
            kDone,
            kCancelled
        };

        PrintQueue^                         destinationPrintQueue;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        XpsDocument^                        destinationDocument;

        DocumentWriterState                 currentState;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        PrintTicket^                        currentUserPrintTicket;

        Object^                             _currentUserState;
        ArrayList^                          _printTicketSequences;
        ArrayList^                          _writingProgressSequences;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        MXDWSerializationManager^           _mxdwManager;

        Package^                            _mxdwPackage;
        Boolean                             _isDocumentCloned;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        XpsDocument^                        _sourceXpsDocument;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        IXpsFixedDocumentSequenceReader^    _sourceXpsFixedDocumentSequenceReader;

        Package^                            _sourcePackage;
        Int32                               _writingCancelledEventHandlersCount;
        PrintSystemDispatcherObject^    accessVerifier;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        System::Windows::Xps::Serialization::PrintTicketLevel              currentWriteLevel;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        System::Windows::Xps::Serialization::PackageSerializationManager^  _manager;

        WritingPrintTicketRequiredEventHandler^ m_WritingPrintTicketRequired;
    };


    public ref class VisualsToXpsDocument:SerializerWriterCollator
    {

    internal:


        /// <SecurityNote>
        ///     Critical - Initializes critical members(_mxdwManager, destinationDocument)
        ///     Safe     - Initializes to safe values (null)
        /// <SecurityNote>
        [SecuritySafeCritical]
        VisualsToXpsDocument(
            XpsDocumentWriter^  writer,
            PrintQueue^         printQueue
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsDocument)
        /// <SecurityNote>
        [SecurityCritical]
        VisualsToXpsDocument(
            XpsDocumentWriter^  writer,
            XpsDocument^        document
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicket)
        /// <SecurityNote>
        [SecurityCritical]
        VisualsToXpsDocument(
            XpsDocumentWriter^  writer,
            PrintQueue^         printQueue,
            PrintTicket^        documentSequencePrintTicket,
            PrintTicket^        documentPrintTicket
            );

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (PrintTicket, XpsDocument)
        /// <SecurityNote>
        [SecurityCritical]
        VisualsToXpsDocument(
            XpsDocumentWriter^  writer,
            XpsDocument^        document,
            PrintTicket^        documentSequencePrintTicket,
            PrintTicket^        documentPrintTicket
            );

    public:

        virtual
        void
        BeginBatchWrite(
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        EndBatchWrite(
            ) override;

        virtual
        void
        Write(
            System::Windows::Media::Visual^     visual
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Write(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket
            ) override;

        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket
            ) override;

        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            Object^                             userSuppliedState
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        WriteAsync(
            System::Windows::Media::Visual^     visual,
            PrintTicket^                        printTicket,
            Object^                             userSuppliedState
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsSerializationManager, NgcSerializationManager)
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        CancelAsync(
            ) override;

        /// <SecurityNote>
        ///    Critical - Uses type from non-APTCA reachframework.dll (XpsSerializationManager, NgcSerializationManager)
        ///    PublicOK - User is always prompted with a trusted Print Dialog before an XpsDocumentWriter instance is exposed in partial trust
        ///</SecurityNote>
        [SecuritySafeCritical]
        virtual
        void
        Cancel(
            void
            ) override;

    internal:

    /// <SecurityNote>
    ///    Critical - Uses type from non-APTCA reachframework.dll
    /// <SecurityNote>
    [SecurityCritical]
    void
    SetPrintTicketEventHandler(
        System::Windows::Xps::Serialization::PackageSerializationManager^ manager
        );

    /// <SecurityNote>
    ///    Critical - Uses type from non-APTCA reachframework.dll
    /// <SecurityNote>
    [SecurityCritical]
    void
    ForwardUserPrintTicket(
        Object^                                                                               sender,
        System::Windows::Xps::Serialization::XpsSerializationPrintTicketRequiredEventArgs^    args
        );

    private:

        ///<SecurityNote>
        /// Critical    - Calls CreateXPSDocument when printing to MXDW driver
        /// TreatAsSafe - Only creates files at path passed back from the MXDW driver which is trusted
        ///</SecurityNote>
        [SecuritySafeCritical]
        bool
        WriteVisual(
            bool                asyncMode,
            PrintTicket^        printTicket,
            System::Windows::
            Xps::
            Serialization::
            PrintTicketLevel    printTicketLevel,
            System::
            Windows::
            Media::
            Visual^             visual
            );

        ///<SecurityNote>
        /// Critical    - When running in PT, we need to evelate to determine if the driver is Mxdw
        /// TreatAsSafe - the driver name is not handed outside of the method, besides it's not considered to be critical in PT.
        ///</SecurityNote>
        [SecuritySafeCritical]
        bool
        MxdwConversionRequired(
            PrintQueue^  printQueue
            );

        ///<SecurityNote>
        /// Critical    - Exposes user selected file path
        ///</SecurityNote>
        [SecurityCritical]
        String^
        MxdwInitializeOptimizationConversion(
            PrintQueue^ printQueue
            );

        ///<SecurityNote>
        /// Critical    - Will create an arbitrary file at the passed in file name
        ///</SecurityNote>
        [SecurityCritical]
        void
        CreateXPSDocument(
            String^     documentName
            );

        void
        InitializeSequences(
            void
            );

        void
        VerifyAccess(
            void
            );


        enum class VisualsCollaterState
        {
            kUninit,
            kSync,
            kAsync,
            kDone,
            kCancelled
        };

        Object^                     _currentUserState;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        PrintTicket^                _documentSequencePrintTicket;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        PrintTicket^                _documentPrintTicket;

        XpsDocumentWriter^          parentWriter;
        VisualsCollaterState        currentState;
        PrintQueue^                 destinationPrintQueue;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        XpsDocument^                destinationDocument;
        bool                        isPrintTicketEventHandlerSet;
        bool                        isCompletionEventHandlerSet;
        bool                        isProgressChangedEventHandlerSet;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        MXDWSerializationManager^   _mxdwManager;

        Package^                    _mxdwPackage;
        Hashtable^                  _printTicketsTable;
        ArrayList^                  _printTicketSequences;
        Int32                       _numberOfVisualsCollated;
        PrintSystemDispatcherObject^    accessVerifier;

        ///<SecurityNote>
        /// Critical    - Field for critical type
        ///</SecurityNote>
        [SecurityCritical]
        System::Windows::Xps::Serialization::PackageSerializationManager^  _manager;
    };


    /// <summary>
    /// This class is used to throw exceptions from the XpsDocumentWriter and related classes.
    /// </summary>
    [System::Serializable]
    public ref class XpsWriterException : public Exception
    {
    public:
        /// <summary>
        ///
        /// </summary>
        XpsWriterException(
            );

        /// <summary>
        ///
        /// </summary>
        /// <param name="message"></param>
        XpsWriterException(
            String^     message
            );

        /// <summary>
        ///
        /// </summary>
        /// <param name="message"></param>
        /// <param name="innerException"></param>
        XpsWriterException(
            String^     message,
            Exception^  innerException
            );

	protected:
		XpsWriterException(
		    SerializationInfo   ^info,
		    StreamingContext    context
		    );

    internal:

        static void
        ThrowException(
            String^ message
            );
    };
}
}
}

#endif // __XPSDOCUMENTWRITER_HPP__
