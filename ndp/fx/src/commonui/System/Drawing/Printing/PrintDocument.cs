//------------------------------------------------------------------------------
// <copyright file="PrintDocument.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Drawing.Printing {

    using Microsoft.Win32;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;    
    using System.Drawing.Design;
    using System.Reflection;
    using System.Security;
    using System.Security.Permissions;
    using System.IO;
    using System.Runtime.Versioning;

    /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument"]/*' />
    /// <devdoc>
    ///    <para>Defines a reusable object that sends output to the
    ///       printer.</para>
    /// </devdoc>
    [
    ToolboxItemFilter("System.Drawing.Printing"),
    DefaultProperty("DocumentName"),
    SRDescription(SR.PrintDocumentDesc),
    DefaultEvent("PrintPage")
    ]
    public class PrintDocument : Component {
        private string documentName = "document";

        private PrintEventHandler beginPrintHandler;
        private PrintEventHandler endPrintHandler;
        private PrintPageEventHandler printPageHandler;
        private QueryPageSettingsEventHandler queryHandler;

        private PrinterSettings printerSettings = new PrinterSettings();
        private PageSettings defaultPageSettings;

        private PrintController printController;

        private bool originAtMargins;
        private bool userSetPageSettings;

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.PrintDocument"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Drawing.Printing.PrintDocument'/>
        /// class.</para>
        /// </devdoc>
        public PrintDocument() {
            defaultPageSettings = new PageSettings(printerSettings);
        }
        
        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.DefaultPageSettings"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the
        ///       default
        ///       page settings for the document being printed.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.PDOCdocumentPageSettingsDescr)
        ]
        public PageSettings DefaultPageSettings {
            get { return defaultPageSettings;}
            set { 
                if (value == null)
                    value = new PageSettings();
                defaultPageSettings = value;
                userSetPageSettings = true;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.DocumentName"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the name to display to the user while printing the document;
        ///       for example, in a print status dialog or a printer
        ///       queue.</para>
        /// </devdoc>
        [
        DefaultValue("document"),
        SRDescription(SR.PDOCdocumentNameDescr)
        ]
        public string DocumentName {
            get { return documentName;}

            set {
                if (value == null)
                    value = "";
                documentName = value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.OriginAtMargins"]/*' />
        // If true, positions the origin of the graphics object 
        // associated with the page at the point just inside
        // the user-specified margins of the page.
        // If false, the graphics origin is at the top-left
        // corner of the printable area of the page.
        //
        [
        DefaultValue(false),
        SRDescription(SR.PDOCoriginAtMarginsDescr)
        ]
        public bool OriginAtMargins {
            get
            {
                return originAtMargins;
            }
            set
            {
                originAtMargins = value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.PrintController"]/*' />
        /// <devdoc>
        /// <para>Gets or sets the <see cref='System.Drawing.Printing.PrintController'/> 
        /// that guides the printing process.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.PDOCprintControllerDescr)
        ]
        public PrintController PrintController {
            get { 
                IntSecurity.SafePrinting.Demand();

                if (printController == null) {
                    printController = new StandardPrintController();
                    new ReflectionPermission(PermissionState.Unrestricted).Assert();
                    try {
                        try {
                            // SECREVIEW 332064: this is here because System.Drawing doesnt want a dependency on
                            // System.Windows.Forms.  Since this creates a public type in another assembly, this 
                            // appears to be OK.
                            Type type = Type.GetType("System.Windows.Forms.PrintControllerWithStatusDialog, " + AssemblyRef.SystemWindowsForms);
                            printController = (PrintController) Activator.CreateInstance(type, 
                                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, 
                                null, new object[] { printController}, null);
                        }
                        catch (TypeLoadException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                        catch (TargetInvocationException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                        catch (MissingMethodException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                        catch (MethodAccessException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                        catch (MemberAccessException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                        catch (FileNotFoundException) {
                            Debug.Fail("Can't find System.Windows.Forms.PrintControllerWithStatusDialog, proceeding with StandardPrintController");
                        }
                    }
                    finally {
                        CodeAccessPermission.RevertAssert();
                    }
                }
                return printController;
            }
            set {
                IntSecurity.SafePrinting.Demand();

                printController = value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.PrinterSettings"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the printer on which the
        ///       document is printed.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        SRDescription(SR.PDOCprinterSettingsDescr)
        ]
        public PrinterSettings PrinterSettings {
            get { return printerSettings;}
            set { 
                if (value == null)
                    value = new PrinterSettings();
                printerSettings = value;
                // reset the PageSettings that match the PrinterSettings only if we have created the defaultPageSettings..
                if (!userSetPageSettings)
                {
                    defaultPageSettings = printerSettings.DefaultPageSettings;
                }
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.BeginPrint"]/*' />
        /// <devdoc>
        /// <para>Occurs when the <see cref='System.Drawing.Printing.PrintDocument.Print'/> method is called, before 
        ///    the
        ///    first page prints.</para>
        /// </devdoc>
        [SRDescription(SR.PDOCbeginPrintDescr)]
        public event PrintEventHandler BeginPrint {
            add {
                beginPrintHandler += value;
            }
            remove {
                beginPrintHandler -= value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.EndPrint"]/*' />
        /// <devdoc>
        /// <para>Occurs when <see cref='System.Drawing.Printing.PrintDocument.Print'/> is
        ///    called, after the last page is printed.</para>
        /// </devdoc>
        [SRDescription(SR.PDOCendPrintDescr)]
        public event PrintEventHandler EndPrint {
            add {
                endPrintHandler += value;
            }
            remove {
                endPrintHandler -= value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.PrintPage"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a page is printed. </para>
        /// </devdoc>
        [SRDescription(SR.PDOCprintPageDescr)]
        public event PrintPageEventHandler PrintPage {
            add {
                printPageHandler += value;
            }
            remove {
                printPageHandler -= value;
            }
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.QueryPageSettings"]/*' />
        /// <devdoc>
        ///    <para>Occurs</para>
        /// </devdoc>
        [SRDescription(SR.PDOCqueryPageSettingsDescr)]
        public event QueryPageSettingsEventHandler QueryPageSettings {
            add {
                queryHandler += value;
            }
            remove {
                queryHandler -= value;
            }
        }

        internal void _OnBeginPrint(PrintEventArgs e) {
            OnBeginPrint(e);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.OnBeginPrint"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='E:System.Drawing.Printing.PrintDocument.BeginPrint'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnBeginPrint(PrintEventArgs e) {
            if (beginPrintHandler != null)
                beginPrintHandler(this, e);
        }

        internal void _OnEndPrint(PrintEventArgs e) {
            OnEndPrint(e);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.OnEndPrint"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='E:System.Drawing.Printing.PrintDocument.EndPrint'/>
        /// event.</para>
        /// </devdoc>
        protected virtual void OnEndPrint(PrintEventArgs e) {
            if (endPrintHandler != null)
                endPrintHandler(this, e);
        }

        internal void _OnPrintPage(PrintPageEventArgs e) {
            OnPrintPage(e);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.OnPrintPage"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='E:System.Drawing.Printing.PrintDocument.PrintPage'/>
        /// event.</para>
        /// </devdoc>
        // 
        protected virtual void OnPrintPage(PrintPageEventArgs e) {
            if (printPageHandler != null)
                printPageHandler(this, e);
        }

        internal void _OnQueryPageSettings(QueryPageSettingsEventArgs e) {
            OnQueryPageSettings(e);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.OnQueryPageSettings"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='E:System.Drawing.Printing.PrintDocument.QueryPageSettings'/> event.</para>
        /// </devdoc>
        protected virtual void OnQueryPageSettings(QueryPageSettingsEventArgs e) {
            if (queryHandler != null)
                queryHandler(this, e);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.Print"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Prints the document.
        ///    </para>
        /// </devdoc>
        [ResourceExposure(ResourceScope.Process)]
        [ResourceConsumption(ResourceScope.Process)]
        public void Print() {
            // It is possible to SetPrinterName using signed secured dll which can be used to by-pass Printing security model.
            // hence here check if the PrinterSettings.IsDefaultPrinter and if not demand AllPrinting.
            // Refer : VsWhidbey : 235920
            if (!this.PrinterSettings.IsDefaultPrinter && !this.PrinterSettings.PrintDialogDisplayed)
            {
                IntSecurity.AllPrinting.Demand();
            }
            PrintController controller = PrintController;
            controller.Print(this);
        }

        /// <include file='doc\PrintDocument.uex' path='docs/doc[@for="PrintDocument.ToString"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Provides some interesting information about the PrintDocument in
        ///       String form.
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return "[PrintDocument " + DocumentName + "]";
        }
    }
}

