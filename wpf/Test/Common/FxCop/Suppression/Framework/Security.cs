//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//**************************************************************************************************************************
// Bug ID:    1398189
// Developer: grzegorz
// Reason:    Reviewed call graph. All calllers are marked as SecurityTreatAsSafe.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySubpageBasicColumnList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSTRACKDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySubpageDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSSUBPAGEDETAILS&):System.Int32")]

//**************************************************************************************************************************
// Bug ID:    1390397
// Developer: grzegorz
// Reason:    Those mathods are reviewed. They are moved from FlowDocumnetScrollViewer, which has been already reviewed.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentViewerHelper.ShowFindUnsuccessfulMessage(MS.Internal.Documents.FindToolBar):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentViewerHelper.ToggleFindToolBar(System.Windows.Controls.Decorator,System.EventHandler,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentViewerHelper.Find(MS.Internal.Documents.FindToolBar,System.Windows.Documents.TextEditor):System.Windows.Documents.ITextRange")]

//**************************************************************************************************************************
// Bug ID:    1380381
// Developer: darind
// Reason:    The code has passed review from the Avalon Partial Trust Team previously and no risks were found
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope = "member", Target = "System.Windows.Controls.PrintDialog.CreateWriter(System.String):System.Windows.Xps.XpsDocumentWriter")]

//*********************************************************************************************************************************
// Bug ID:    1337369
// Developer: robertan
// Reason: Reviewed the methods, and either a demand is in place or there is no risk.  These are calls from PF.dll to PUI.dll
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.GetPrintTicketSnapshot():System.Printing.PrintTicket")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.ChangePrintTicket(System.Printing.PrintTicket):System.Void")]

//*********************************************************************************************************************************
// Bug ID:    1341047
// Developer: robertan
// Reason: I reviewed this variable usage.  This variable represents a native Win32 Window handle that we are just referencing
//         for the purpose of parenting a dialog.  No cleanup is needed on this reference and we just throw it away when we are
//         done with it.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx._ownerHandle")]

//**************************************************************************************************************************
// Bug ID: 1344580
// Developer: FrankGor
// Reason: By Design and PT Reviewed; calling PackageDocument.Package is expected to be safe here.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.ApplicationProxyInternal.Run(MS.Internal.AppModel.ApplicationProxyInternal+InitData):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1344580
// Developer: FrankGor
// Reason: By Design and PT Reviewed; calling DocumentManager.CleanUp is expected to be safe here.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.ApplicationProxyInternal.Cleanup():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1337448
// Developer: OliverFo
// Reason: By Design and PT Reviewed; the XpsSerializer creation is guarded by a full trust demand
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Documents.Serialization.SerializerProvider.CreateSystemSerializerDescriptor():System.Windows.Documents.Serialization.SerializerDescriptor")]

//**************************************************************************************************************************
// Bug ID: 1337453
// Developer: OliverFo
// Reason: The call to Assembly.LoadFrom is required to load a plug-in serializer.
//         There is a demand for full trust before this method call, so it is safe
//         It has been avpt reviewed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Documents.Serialization.SerializerDescriptor.CreateSerializerFactory():System.Windows.Documents.Serialization.ISerializerFactory", MessageId="System.Reflection.Assembly.LoadFrom")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: shibai
// Reason: Please see FlowDocumentPageViewer and DocumentViewerBase for details, FlowDocumentScrollViewer works similar as these classes
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.ToggleFindToolBar(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.OnFindCommand():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.Find(MS.Internal.Documents.FindToolBar):System.Windows.Documents.ITextRange")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.OnFindInvoked(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.MediaEndedEvent")]

//**************************************************************************************************************************
// Bug ID: 1261979
// Developer: AtanasK
// Approved: SRamani and BenCar
// Reason: There is an event (ItemContainerGenerator.ItemsChanged) that is being attached to in Panel.
// The event handler call path eventually makes its way to VirtualizingPanel.OnItemsChanged where the event arguments
// are exposed. I don’t think there are any security issues with doing that.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Controls.VirtualizingPanel.OnItemsChanged(System.Object,System.Windows.Controls.Primitives.ItemsChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1284600
// Developer: grzegorz
// Reason: MS.Internal.PtsHost.PtsContext.OnDestroyBreakRecord is marked as SecurityCritical and SecurityTreatAsSafe.
//     It calls security critical methods and used critical data, but only internally and does not allow any access to it.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsDestroyPageBreakRecord(System.IntPtr,System.IntPtr):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1235317
// Developer: BenCar
// Reason: There are no security demands or asserts involved with this method.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.OnItemsChanged(System.Object,System.Windows.Controls.ItemsChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1227704
// Developer: neilkr
// Reason: ListBox simply re-exposes the Selected/Unselected events from Selector that were already excluded on 6/20/05
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ListBoxItem.SelectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ListBoxItem.UnselectedEvent")]

//*********************************************************************************************************************************
// Bug ID:    N/A
// Developer: robertan
// Reason: All below supressions have been approved already (in Approved file).  However, the namespaces changes in a few places and
//         I was unsure of whether I was allowed to edit the approved file rather than re-submitting in the proposed.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.ShowDialog():System.Nullable`1<System.Boolean>")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.Initialize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.AddPanel(System.Windows.Controls.Panel,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.OnPrintTicketChanging(System.Windows.Controls.PrintTicketChangingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.ChangePrinter(System.Printing.PrintQueue):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.RemovePanel(System.String):System.Void")]

[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialog.InitializeFullTrust():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialog.UpdatePrintableAreaSize():System.Void")]

[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.PrintDialog.CreateWriter():System.Windows.Xps.XpsDocumentWriter")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AcquirePrintQueue(System.String):System.Printing.PrintQueue")]


//**************************************************************************************************************************
// Bug ID: 1196958
// Developer: SamBent
// Reason: False positive.  This static field's value is actually immutable.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.GroupStyle.DefaultGroupPanel")]

//**************************************************************************************************************************
// Bug ID: 1182165
// Developer: jeremyns
// Reason: DocumentApplicationUI is marked as [FriendAccessAllowed], so this call should be permitted.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.OnLoadCompleted(System.Windows.Navigation.NavigationEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: BenCar
// Reason: Approved by avpttrev. Method does not expose any critical data in return values or parameters.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.SystemResources.EnsureResourceChangeListener():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1106780 - FxCop: AptcaMethodsShouldOnlyCallAptcaMethods::MS.Internal.AppModel (presentationframework.dll 4 violation)
// Developer: drelyea
// Reason: Per marka, I have put a demand in all four of these methods. We do not want to make PresentationUI APTCA.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.XappLauncherApp.HandleError(System.Exception,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.XappLauncherApp.DoErrorUI(System.Exception):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.XappLauncherApp.DoDownloadUI():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.XappLauncherApp.DoDownloadProgressChanged(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.XappLauncherApp.HandleCancel():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.XappLauncherApp.DoGetManifestCompleted(System.Object):System.Object")]


// GregLett
// Bug# 1184848
// Command declaration pattern is public static readonly.  These are simply the latest two in 17 such commands on SB.
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToHorizontalOffsetCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToVerticalOffsetCommand")]

//**************************************************************************************************************************
// Bug ID: 1206758
// Developer: jeremyns
// Reason: Reviewed the methods, and no risk was found.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.OnAddJournalEntry(System.Object,MS.Internal.Documents.Application.DocumentApplicationJournalEntryEventArgs):System.Void")]
//**************************************************************************************************************************
// Bug ID: 1196994
// Developer: jeremyns
// Reason: Reviewed the methods, and no risk was found.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.InstantiateFindToolBar():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.GoToFind():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.OnDocumentChanged():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.OnFindInvoked(System.Object,System.EventArgs):System.Void")]
//**************************************************************************************************************************
// Bug ID: "Private run"
// Developer: jeremyns
// Reason: Reviewed the methods, and no risk was found.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.Application.DocumentApplicationJournalEntry.Replay(System.Windows.Navigation.NavigationService):System.Windows.Navigation.JournalEntry")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.InitializeDocumentApplicationDocumentViewer():System.Void")]




//**************************************************************************************************************************
// Bug ID: Private run
// Developer: andren
// Reason: partial trust security team has reviewed all of these
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.MSInternal", "CA900:AptcaAssembliesShouldBeReviewed")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="namespace")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Utility.BindUriHelper.IsSameUri(System.Uri,System.Uri,System.Uri,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Utility.BindUriHelper.EnsureAppDomainBaseUri():System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.Utility.LoadWrapper.Load(System.String,System.String):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.Utility.LoadWrapper.Load(System.String,System.String):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.ComboBox.System.Windows.Automation.Provider.IValueProvider.SetValue(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.GetVisualRoot(System.Windows.DependencyObject):System.Windows.Media.Visual")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.TextBox.System.Windows.Automation.Provider.IValueProvider.SetValue(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetFullAssemblyNameFromPartialName(System.Reflection.Assembly,System.String):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.SafeSecurityHelper.MapWindowPoints(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+RECT&,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetAssemblyPartialName(System.Reflection.Assembly):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.IsConnectedToPresentationSource(System.Windows.Media.Visual):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Navigation.Loader.OnBindDoneRequest(MS.Internal.Navigation.BindRequest):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Navigation.BindRequest.StartBindDefault(MS.Internal.Navigation.BindRequest):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.Navigation.BindRequest.StartBindDefault(MS.Internal.Navigation.BindRequest):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.Primitives.RangeBase.System.Windows.Automation.Provider.IRangeValueProvider.SetValue(System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.LoadFragment(System.IO.TextReader):System.Object[]")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.Load(System.IO.Stream):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.LoadFragment(System.IO.TextReader,System.Windows.Markup.ParserContext):System.Object[]")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.Parser.XmlTreeBuildDefault(System.Windows.Markup.ParserContext,System.IO.Stream,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.LoadBaml(System.IO.Stream,System.Object,System.Windows.Markup.ParserContext,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.LoadBaml(System.IO.Stream):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.Load(System.Xml.XmlReader):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlReader.XmlTreeBuildDefault(System.Windows.Markup.ParserContext,System.Xml.XmlReader,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2110:SecureGetObjectDataOverrides", Scope="member", Target="System.Windows.Markup.XamlSerializationCallbackException.GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.BamlRecordReader.ReadElementStartRecord(System.Windows.Markup.BamlElementStartRecord):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.BamlRecordReader.BaseReadElementStartRecord(System.Windows.Markup.BamlElementStartRecord,System.Windows.Markup.BamlTypeInfoRecord):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlTypeMapper.ParseProperty(System.Type,System.String,System.Object,System.ComponentModel.ITypeDescriptorContext,System.Windows.Markup.ParserContext,System.String,System.Int16):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.GetTemporaryAvalonXmlnsDefinitions():System.String[]")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Markup.XamlSerializerUtil.TraceEvent(System.Object,System.Int32,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Data.CollectionView.OnCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,System.Boolean&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetDC(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+ANIMATIONINFO,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntReleaseDC(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLong(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,System.Int32&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2101:SpecifyMarshalingForPInvokeStringArguments", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetModuleFileName(System.Runtime.InteropServices.HandleRef,System.Text.StringBuilder,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetSystemMetrics(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+RECT&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetDeviceCaps(System.Runtime.InteropServices.HandleRef,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetWindowLongPtr(System.Runtime.InteropServices.HandleRef,System.Int32):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+NONCLIENTMETRICS,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SetFocus(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+ICONMETRICS,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.SystemParametersInfo(System.Int32,System.Int32,MS.Win32.NativeMethods+HIGHCONTRAST_I&,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+EtwTrace.TraceEvent(System.UInt64,System.Char*):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.SystemResources.CacheResource(System.Object,System.Object,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.SystemResources.LoadThemeDictionary(System.Reflection.Assembly,System.Boolean):System.Windows.ResourceDictionary")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.SystemResources.FindResourceInternal(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.SystemResources.LoadGenericDictionary(System.Reflection.Assembly,System.Boolean):System.Windows.ResourceDictionary")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Window+SourceWindowHelper.GetHwndNonClientAreaSizeInMeasureUnits():System.Windows.Size")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.FrameworkElement.GetInheritableValue(System.Windows.DependencyProperty):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Window.InternalClose(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Application.LoadFromExe(System.String,System.Uri,System.IServiceProvider):System.Windows.Application")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.FrameworkElementFactory.InstantiateSingleElement(System.Windows.FrameworkElement):System.Windows.FrameworkElement")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.FrameworkContentElement.GetRawValue(System.Windows.DependencyProperty,System.Object,System.Windows.PropertyMetadata):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Navigation.NavigationService.CallUpdateTravelLog():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="MS.Internal.Navigation.BindRequest.HandleResponse(System.IAsyncResult):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="MS.Internal.Navigation.BindRequest.StartBind():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="System.Windows.Documents.PageContentAsyncResult.Dispatch(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="System.Windows.Markup.TreeBuilderXamlTranslator.ReadXamlAsync():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ToolTip.OpenedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ToolTip.ClosedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.IsCheckedChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.SubmenuOpenedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.SubmenuClosedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.ClickEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ToolTipService.ToolTipClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ToolTipService.ToolTipOpeningEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.CheckBox.CheckStateChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ScrollViewer.ScrollChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.PasswordBox.PasswordChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Group.EmptyGroup")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Group.UniversalGroup")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.LostFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.GotFocusEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ContextMenu.OpenedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ContextMenu.ClosedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TextBlock.BaselineOffsetChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ContextMenuService.ContextMenuOpeningEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.ContextMenuService.ContextMenuClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationComponentChooser.None")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.LineUpCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToHomeCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollHereCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.PageRightCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToRightEndCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.LineRightCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.PageDownCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.PageUpCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToLeftEndCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.LineLeftCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToBottomCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToTopCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToEndCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.PageLeftCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.LineDownCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Selector.SelectionChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Selector.IsSelectedChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ToggleButton.IsCheckedChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Thumb.DragDeltaEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Thumb.DragCompletedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Thumb.DragStartedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ButtonBase.ClickEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase.TextChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.RangeBase.ValueChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.DocumentPage.Missing")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.ContentPosition.Missing")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.ContextMenuClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.ContextMenuOpeningEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.ToolTipOpeningEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.ToolTipClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.ContextMenuClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.ToolTipClosingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.ContextMenuOpeningEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.ToolTipOpeningEvent")]

// *** added 1/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PointUtil.ClientToScreen(System.Windows.Point,System.Windows.PresentationSource):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Documents.TextEditorCopyPaste._RegisterClassHandlers(System.Type,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ClientToScreen(System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+POINT):System.Int32")]

// *** added 2/22/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.EnableFocusCues(System.Windows.DependencyObject,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Navigation.NavigationService.ConfigHttpWebRequest(System.Net.HttpWebRequest):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.ClientToScreen(System.Windows.UIElement,System.Windows.Point):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryLineListSingle(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSLINEDESCRIPTIONSINGLE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryPageDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSPAGEDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsUpdateBottomlessPage(System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSFMTRBL&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsClearUpdateInfoInPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryTextDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSTEXTDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsCreatePageBottomless(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSFMTRBL&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryLineListComposite(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSLINEDESCRIPTIONCOMPOSITE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQuerySectionDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSSECTIONDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQuerySectionBasicColumnList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSTRACKDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryTrackParaList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSPARADESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryPageSectionList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSSECTIONDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsDestroyPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryLineCompositeElementList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.PTS+FSLINEELEMENT*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.CreateDocContext(MS.Internal.PtsHost.PTS+FSCONTEXTINFO&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.FsQueryTrackDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.PTS+FSTRACKDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Navigation.Loader.RequestContentHelper(System.Uri,MS.Internal.AppModel.IAsyncGetStreamEvents,System.Object,System.Windows.Threading.Dispatcher,MS.Internal.Navigation.BindRequest+OnBindDone,System.Boolean,System.Boolean,System.IO.Stream&,System.String&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Navigation.BindRequest.SetWebRequestCache():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.Primitives.Popup+PopupSecurityHelper.GetPresentationSource(System.Windows.Media.Visual):System.Windows.PresentationSource")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.Primitives.Popup+PopupSecurityHelper.GetHandle(System.Windows.Interop.HwndSource):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Navigation.NavigationService.CallUpdateTravelLog(System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.Navigate(System.Windows.DependencyObject,System.Windows.TraversalRequest,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.GetRectangle(System.Windows.DependencyObject):System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetFocus():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IntGetCursorPos(MS.Win32.NativeMethods+POINT):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.TextServicesLoader.TIPsWantToRun():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Application.EnsureHwndSource():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Expander.ExpandedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Expander.CollapsingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Expander.ExpandingEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Expander.CollapsedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Validation.ErrorEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase.SelectionChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.Hyperlink.ClickEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Documents.Hyperlink.RequestNavigateEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Data.Binding.TargetUpdatedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Data.Binding.SourceUpdatedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.RequestBringIntoViewEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.UnloadedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.LoadedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.LoadedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkContentElement.UnloadedEvent")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Win32.HandleCollector+HandleType.Add(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmGetContext(System.Runtime.InteropServices.HandleRef):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmNotifyIME(System.Runtime.InteropServices.HandleRef,System.Int32,System.Int32,System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.ImmReleaseContext(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetLoadedAssembly(System.String,System.Boolean):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Documents.ImmComposition.CompleteComposition():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Documents.ImmComposition.UpdateSource(System.Windows.Interop.HwndSource,System.Windows.Interop.HwndSource):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Documents.ImmComposition.GetImmComposition(System.Windows.FrameworkElement):System.Windows.Documents.ImmComposition")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.IsWindow(System.Runtime.InteropServices.HandleRef):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetKeyState(System.Int32):System.Int16")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods.GetIconInfoImpl(System.Runtime.InteropServices.HandleRef,MS.Win32.UnsafeNativeMethods+ICONINFO_IMPL):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.PTS.CreateInstalledObjectsInfo(MS.Internal.PtsHost.PTS+FSIMETHODS&,MS.Internal.PtsHost.PTS+FSIMETHODS&,System.IntPtr&,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnNavigating(System.Object,System.Windows.Navigation.NavigatingCancelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnNavigated(System.Object,System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnNavigationProgress(System.Object,System.Windows.Navigation.NavigationProgressEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnFragmentNavigation(System.Object,System.Windows.Navigation.FragmentNavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnNavigationStopped(System.Object,System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Application.OnLoadCompleted(System.Object,System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.Primitives.Popup.GetMouseCursorSize(System.Int32&,System.Int32&,System.Int32&,System.Int32&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Window+SourceWindowHelper.get_UnsecureHandle():System.IntPtr")]

// *** added 2/28/05:

[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryTextDetails(System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSTEXTDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsClearUpdateInfoInPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQuerySectionDetails(System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSSECTIONDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.CreateInstalledObjectsInfo(MS.Win32.UnsafeNativeMethods+PTS+FSIMETHODS&,MS.Win32.UnsafeNativeMethods+PTS+FSIMETHODS&,System.IntPtr&,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryLineListSingle(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSLINEDESCRIPTIONSINGLE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryPageSectionList(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSSECTIONDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQuerySectionBasicColumnList(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSTRACKDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsUpdateBottomlessPage(System.IntPtr,System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSFMTRBL&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsCreatePageBottomless(System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSFMTRBL&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryLineCompositeElementList(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSLINEELEMENT*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryPageDetails(System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSPAGEDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsDestroyPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryTrackParaList(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSPARADESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryTrackDetails(System.IntPtr,System.IntPtr,MS.Win32.UnsafeNativeMethods+PTS+FSTRACKDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.FsQueryLineListComposite(System.IntPtr,System.IntPtr,System.Int32,MS.Win32.UnsafeNativeMethods+PTS+FSLINEDESCRIPTIONCOMPOSITE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.UnsafeNativeMethods+PTS.CreateDocContext(MS.Win32.UnsafeNativeMethods+PTS+FSCONTEXTINFO&,System.IntPtr&):System.Int32")]

// *** added 3/7/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.DataStreams.LoadSubStreams(System.Windows.UIElement,System.Collections.ArrayList):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableThicknessTypeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableThicknessTypeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableThicknessTypeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Media.Animation.NullableThicknessTypeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.RoutedEventConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.RoutedEventConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.RoutedEventConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.RoutedEventConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.DependencyPropertyConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.DependencyPropertyConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.DependencyPropertyConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.DependencyPropertyConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CornerRadiusConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CornerRadiusConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CornerRadiusConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.CornerRadiusConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.IconDataConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.IconDataConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.IconDataConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.IconDataConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.GridLengthConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.GridLengthConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.GridLengthConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.GridLengthConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PropertyPathConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PropertyPathConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PropertyPathConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.PropertyPathConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.LengthConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.LengthConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.LengthConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.LengthConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DialogResultConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DialogResultConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DialogResultConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DialogResultConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontSizeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontSizeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontSizeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FontSizeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ThicknessConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ThicknessConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ThicknessConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ThicknessConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CommandConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CommandConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CommandConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Input.CommandConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]

// *** added 3/11/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.CanNavigateToUrlWithZoneCheck(System.Uri,System.Uri):MS.Internal.AppModel.LaunchResult")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.SafeLaunchBrowserOnlyIfPossible(System.Uri,System.Uri,System.Boolean):MS.Internal.AppModel.LaunchResult")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.EnsureSecurityManager():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.IsZoneElevationSettingPrompt(System.Uri):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandFileIOReadPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.GetFullAssemblyNameFromPartialName(System.Reflection.Assembly,System.String):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.GetLoadedAssembly(System.String,System.Boolean):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.ClientToScreen(System.Windows.UIElement,System.Windows.Point):System.Windows.Point")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.MapWindowPoints(System.Runtime.InteropServices.HandleRef,System.Runtime.InteropServices.HandleRef,MS.Win32.NativeMethods+RECT&,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.GetAssemblyPartialName(System.Reflection.Assembly):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SafeSecurityHelper.IsConnectedToPresentationSource(System.Windows.Media.Visual):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.TemplateKeyConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.TemplateKeyConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.TemplateKeyConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.TemplateKeyConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryTrackDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSTRACKDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySectionBasicColumnList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSTRACKDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.CreateDocContext(MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSCONTEXTINFO&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsClearUpdateInfoInPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryPageDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSPAGEDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryLineCompositeElementList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSLINEELEMENT*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryLineListSingle(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSLINEDESCRIPTIONSINGLE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.CreateInstalledObjectsInfo(MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSIMETHODS&,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSIMETHODS&,System.IntPtr&,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySectionDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSSECTIONDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryPageSectionList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSSECTIONDESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsCreatePageBottomless(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSFMTRBL&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryLineListComposite(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSLINEDESCRIPTIONCOMPOSITE*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryTextDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSTEXTDETAILS&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsUpdateBottomlessPage(System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSFMTRBL&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsDestroyPage(System.IntPtr,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQueryTrackParaList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSPARADESCRIPTION*,System.Int32&):System.Int32")]

// *** added 3/17/05:
//fix bug 1117688
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.InstantiateFindDialog():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.OnFindClicked(System.Object,System.EventArgs):System.Void")]

// *** added 3/21/05:
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Window.InternalClose(System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Compile.UnsafeNativeMethods.FindMimeFromData(System.Runtime.InteropServices.ComTypes.IBindCtx,System.String,System.IntPtr,System.Int32,System.String,System.Int32,System.String&,System.Int32):System.Void")]

// *** added 3/24/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Compile.UnsafeNativeMethods.FindMimeFromData(System.Runtime.InteropServices.ComTypes.IBindCtx,System.String,System.IntPtr,System.Int32,System.String,System.Int32,System.String&,System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Documents.TextServicesHost.OnUnregisterTextStore(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.Menu.SetupMainMenu():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Data.CollectionView.OnCurrentChanging(System.Object,System.ComponentModel.CurrentChangingEventArgs):System.Void")]

// *** added 5/6/05:
// bugs 1160477, others
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Documents.Hyperlink.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Documents.Hyperlink.OnMouseLeftButtonUp(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Window+SourceWindowHelper.get_CriticalHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandPathDiscovery(System.String):System.Void")]

// *** added 5/19/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsUpdateFinitePage(System.IntPtr,System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSFMTR&,System.IntPtr&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsCreatePageFinite(System.IntPtr,System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSFMTR&,System.IntPtr&,System.IntPtr&):System.Int32")]

// *** added 5/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MonthCalendar.FirstVisibleMonthChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MonthCalendar.DateSelectionChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.DatePicker.ValueChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.KeyboardNavigation.GotFocusTargetEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Input.KeyboardNavigation.LostFocusTargetEvent")]

// *** added 6/2/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.CleanUpVirtualizedItemEvent")]
//bug 1182332
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.NullableDateTimeConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.NullableDateTimeConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.NullableDateTimeConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.NullableDateTimeConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]

// *** added 6/6/05:
//bug 1186038
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.CreateUriDiscoveryPermission(System.Uri):System.Security.CodeAccessPermission")]

// *** added 6/9/05:
//bug 1189999
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Application.GetSystemSound(System.String):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.EnsureAppVroot():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandWebPermission(System.Uri):System.Void")]

// *** added 6/15/05:

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.IsZoneElevationSettingPrompt(System.Uri):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.AVElementHelper..ctor(System.Windows.Controls.MediaElement)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.PopupControlService..ctor()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.PasswordBox.get_Password():System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.OnPreviewMouseLeftButtonDown(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.Popup.GetMouseCursorSize(System.Int32&,System.Int32&,System.Int32&,System.Int32&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.ButtonBase.GetMouseLeftButtonReleased():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.FrameworkTextComposition.Complete():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.WinEventHandler.Clear():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.TextServicesHost.DeactivateThreadManager():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.TextServicesHost.OnUnregisterTextStore(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.SystemParameters.get_HighContrast():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Window.get_RestoreBounds():System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Application.InBrowserHostedApp():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.FrameworkContentElement.OnAncestorChangedInternal(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.KeyboardNavigation..ctor()")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Input.KeyboardNavigation.IsKeyboardMostRecentInputDevice():System.Boolean")]

// *** added 6/20/05:
//bugs 1198039,40,41, others
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Application.GetSystemSound(System.String):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.GroupStyle.DefaultGroupPanel")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.DeleteAnnotationsCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.CreateTextStickyNoteCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.ClearHighlightsCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.CreateHighlightCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.DeleteStickyNotesCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Annotations.AnnotationService.CreateInkStickyNoteCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.OleCmdHelper.EnsureApplicationCommandsTable():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.InitializeDocumentApplicationDocumentViewer():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.StickyNoteControl.DeleteNoteCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.StickyNoteControl.InkCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToHorizontalOffsetCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.ScrollToVerticalOffsetCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Selector.UnselectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.Selector.SelectedEvent")]

// *** added 6/22/05:
//bug 1200451
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.CallerHasWebPermission(System.Uri):System.Boolean")]

// *** added 6/27/05:
//bug 1203389-92,1206762
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Data.MultiBindingExpressionBaseConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Data.MultiBindingExpressionBaseConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Data.BindingExpressionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Data.BindingExpressionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TemplateBindingExtensionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.TemplateBindingExtensionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FigureLengthConverter.CanConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FigureLengthConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FigureLengthConverter.ConvertFrom(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.FigureLengthConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DynamicResourceExtensionConverter.ConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Globalization.CultureInfo,System.Object,System.Type):System.Object")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.DynamicResourceExtensionConverter.CanConvertTo(System.ComponentModel.ITypeDescriptorContext,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeView.SelectedItemChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeStartedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeCompletedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeDeltaEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.UnselectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.ExpandedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.CollapsedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.SelectedEvent")]
//bug 1206757
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.AppModel.ApplicationProxyInternal.InitContainer():System.Void")]
//bugs 1206756,59,60,61
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.ApplicationProxyInternal.InitContainer():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.OnAddJournalEntry(System.Object,MS.Internal.Documents.Application.DocumentApplicationJournalEntryEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.InstantiateFindToolBar():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.GoToFind():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.OnDocumentChanged():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.OnFindInvoked(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.OnFindCommand():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.OnFindInvoked(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.ToggleFindToolBar(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.Application.DocumentApplicationJournalEntry.Replay(System.Windows.Navigation.NavigationService):System.Windows.Navigation.JournalEntry")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.Find(MS.Internal.Documents.FindToolBar):System.Windows.Documents.ITextRange")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.Initialize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.AddPanel(System.Windows.Controls.Panel,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.ChangePrinter(System.Printing.PrintQueue):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.ConfigureDefault(System.Printing.Dialogs.PrintDialogConfiguration):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.RemovePanel(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog.OnPrintTicketChanging(System.Printing.Dialogs.PrintTicketChangingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Control.MouseDoubleClickEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Control.PreviewMouseDoubleClickEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.FrameworkElement.SizeChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.CheckedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MenuItem.UncheckedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ToggleButton.UncheckedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ToggleButton.CheckedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.ToggleButton.IndeterminateEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeCompletedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeDeltaEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Resizer.ResizeStartedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeView.SelectedItemChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.CollapsedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.ExpandedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.SelectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.TreeViewItem.UnselectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.CleanUpVirtualizedItemEvent")]

// *** added 7/5/05:
//bugs 1214008,09,10
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.DocumentViewer.ProcessFindKeys(System.Windows.Input.KeyEventArgs):System.Windows.Input.KeyEventArgs")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.ShowMessageBoxHelper(System.Windows.Window,System.String,System.String,System.Windows.MessageBoxButton,System.Windows.MessageBoxImage):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.DocObjHost.InitializeLifetimeService():System.Object")]

// *** added 7/11/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsTransformRectangle(System.UInt32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSRECT&,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSRECT&,System.UInt32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSRECT&):System.Int32")]

// *** added 7/18/05:
//bug 1225805
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.AppModel.ContentFilePart.CriticalOpenFile(System.String):System.IO.Stream")]

// *** added 7/25/05:
//bug 1235323
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.OnItemsChanged(System.Object,System.Windows.Controls.ItemsChangedEventArgs):System.Void")]

// *** added 7/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentApplication.OnNavigating(System.Windows.Navigation.NavigatingCancelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.Application.DocumentApplicationJournalEntry.Replay(System.Windows.Navigation.NavigationService,System.Windows.Navigation.NavigationMode):System.Void")]
//bugs 1243965,6
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="System.Windows.Documents.Serialization.SerializerProvider.CreateSerializerWriter(System.Windows.Documents.Serialization.SerializerDescriptor,System.IO.Stream):System.Windows.Documents.Serialization.SerializerWriter")]
[module: SuppressMessage("Microsoft.Security", "CA2113:SecureLateBindingMethods", Scope="member", Target="System.Windows.Documents.Serialization.SerializerProvider.CreateSerializerWriter(System.Windows.Documents.Serialization.SerializerDescriptor,System.IO.Stream):System.Windows.Documents.Serialization.SerializerWriter")]

// *** added 8/4/05:
// bugs 1246830,1
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Ink.HighContrastHelper.get_HighContrast():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Controls.InkCanvas.CopyToDataObject():MS.Internal.Ink.InkCanvasClipboardDataFormats")]

// *** added 8/8/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.MediaFailedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.MediaOpenedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.BufferingStartedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.BufferingEndedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.CanIncreaseZoomPropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.FlowDocumentPageViewer.CanDecreaseZoomPropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.CanGoToPreviousPagePropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.CanGoToNextPagePropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.MasterPageNumberPropertyKey")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.PageCountPropertyKey")]

// *** added 8/18/05:
//bugs 1261983,4
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.OnItemsChanged(System.Object,System.Windows.Controls.Primitives.ItemsChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Printing.Dialogs.PrintDialog+PrintDialogCallbackHandler.PrinterAdded(System.String):System.Void")]
//bug 1269153
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandMediaAccessPermission(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.CreateMediaAccessPermission(System.String):System.Security.CodeAccessPermission")]

// *** added 8/25/05:
//bug 1275225
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Interop.DocObjHost.InitializeLifetimeService():System.Object")]
//bug 1278663
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.NullableBoolConverter.GetStandardValuesExclusive(System.ComponentModel.ITypeDescriptorContext):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.NullableBoolConverter.GetStandardValues(System.ComponentModel.ITypeDescriptorContext):System.ComponentModel.TypeConverter+StandardValuesCollection")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.NullableBoolConverter.GetStandardValuesSupported(System.ComponentModel.ITypeDescriptorContext):System.Boolean")]
//bug 1278664
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySubtrackParaList(System.IntPtr,System.IntPtr,System.Int32,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSPARADESCRIPTION*,System.Int32&):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.PtsHost.UnsafeNativeMethods.PTS.FsQuerySubtrackDetails(System.IntPtr,System.IntPtr,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSSUBTRACKDETAILS&):System.Int32")]

// *** added 9/12/05:
//bug 1294062
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]
//bug 1294060
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]


// *** added 9/22/05:
//bugs 1309471,4,5
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.PrintDialogBase.SetPrintButtonCaption(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.PrintDialogBase+PrintDialogCallbackHandler.PrinterAdded(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.PageSetupDialog.Initialize():System.Void")]
//bug 1309483
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Printing.UnsafeNativeMethods.GlobalLock(System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Printing.UnsafeNativeMethods.PrintDlgEx(System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Printing.UnsafeNativeMethods.GlobalUnlock(System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Printing.UnsafeNativeMethods.GetDesktopWindow():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Internal.Printing.UnsafeNativeMethods.GlobalFree(System.IntPtr):System.IntPtr")]

// *** added 9/26/05:
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.GestureEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.EditingModeInvertedChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.ActiveEditingModeChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.StrokeCollectedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.EditingModeChangedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.InkCanvas.StrokeErasedEvent")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.FlowDocumentReader.SwitchViewingModeCommand")]
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope="member", Target="System.Windows.Controls.MediaElement.MediaEndedEvent")]
//bug 1322635
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="System.Windows.Documents.NaturalLanguageHyphenator+UnsafeNativeMethods.NlCreateHyphenator():System.IntPtr")]

// *** added 10/10/05:
//bug 1337370
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.CallerHasMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.DemandMediaPermission(System.Security.Permissions.MediaPermissionAudio,System.Security.Permissions.MediaPermissionVideo,System.Security.Permissions.MediaPermissionImage):System.Void")]

// *** added 10/12/05:

[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.ExtractPrinterNameAndDevMode(System.IntPtr,System.String&,System.IntPtr&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.FreeUnmanagedPrintDlgExStruct(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AcquireResultFromPrintDlgExStruct(System.IntPtr):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.Is64Bit():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Printing.PartialTrustPrintDialogHelper+PrintDlgEx.AllocateUnmanagedPrintDlgExStruct():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.GridViewColumnHeader.GetCursor(System.Int32):System.Windows.Input.Cursor")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="Microsoft.Win32.FileDialog.RunDialog(System.IntPtr):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="Microsoft.Win32.FileDialog.HookProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Application.RunInternal(System.Windows.Window,System.String[]):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.FrameworkContentElement.OnAncestorChangedInternal(System.Windows.TreeChangeInfo):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.DocObjHost.InitializeLifetimeService():System.Object")]

// *** added 10/17/05:
//bug 1345942
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope="member", Target="Microsoft.Win32.FileDialog.PromptUserIfAppropriate(System.String):System.Boolean")]
//bug 1345943
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="Microsoft.Win32.OpenFileDialog.OpenFile():System.IO.Stream")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="Microsoft.Win32.OpenFileDialog.OpenFiles():System.IO.Stream[]")]
// bug 1345944
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="Microsoft.Win32.FileDialog.PromptUserIfAppropriate(System.String):System.Boolean")]
//Bug 1354760
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.SizeOf(System.Type):System.Int32")]
//Bug 1363668
//This is marked critical and callers are tracked
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope = "member", Target = "UnsafeNativeMethods.IntGetWindowTextLength(HandleRef):Int32")]

// *** added 10/31/05:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Utility.AssemblyCacheEnum.GetFullName(MS.Internal.Utility.IAssemblyName):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Utility.AssemblyCacheEnum..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Utility.AssemblyCacheEnum.GetNextAssembly():System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.Popup.GetMaxBoundingRectangle(MS.Win32.NativeMethods+RECT):MS.Win32.NativeMethods+RECT")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Markup.XmlnsCache.SetCacheFileName():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Markup.XmlnsCache.EnumTheGac():System.Void")]

// *** added 11/21/05:
//bug 1380335
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnMouseLeftButtonUp(System.Windows.Input.MouseButtonEventArgs):System.Void", MessageId="0#")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnAccessKey(System.Windows.Input.AccessKeyEventArgs):System.Void", MessageId="0#")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnMouseRightButtonUp(System.Windows.Input.MouseButtonEventArgs):System.Void", MessageId="0#")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void", MessageId="0#")]

// *** added 11/28/05
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies")]

// *** added 12/6/05:
//bug 1419780
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.ExtractAppDomainPermissionSet():System.Security.PermissionSet")]

//**************************************************************************************************************************
// Bug ID: 1512058
// Alias: ArielKi
// Reason: Methods are SecurityCritical, LinkDemand method exposure got reviewed and appropriately constrained.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.WpfPayload.CreateImagePart(System.IO.Packaging.PackagePart,System.Windows.Media.Imaging.BitmapSource,System.String,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.WpfPayload.ImagesAreIdentical(System.Windows.Media.Imaging.BitmapSource,System.Windows.Media.Imaging.BitmapSource):System.Boolean")]

//*** added 2/16/06:
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Markup.XmlnsCache.EnumTheGac():System.Windows.Markup.XmlnsCache+LoadResult")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.ThrowExceptionForHR(System.Int32):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1577605
// Alias: ArielKi
// Reason: Methods are SecurityCritical, LinkDemand method exposure got reviewed and appropriately constrained.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.Popup+PopupSecurityHelper.GetMouseCursorPos(System.Windows.Media.Visual):MS.Win32.NativeMethods+POINT")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.Popup+PopupSecurityHelper.GetTransformToDevice():System.Windows.Media.Matrix")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.Primitives.Popup+PopupSecurityHelper.GetTransformToDevice(System.Windows.Media.Visual):System.Windows.Media.Matrix")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndHost.UpdateWindowPos():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1607364
// Alias: ArielKi
// Reason: LinkDemand method exposure got reviewed and appropriately constrained.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.PresentationFramework.SecurityHelper.ExtractAppDomainPermissionSet():System.Security.PermissionSet")]

//**************************************************************************************************************************
// Bug ID: 1700696 1700696
// Alias: Akaza
// Reason: FxCop: ReviewVisibleEventHandlers::System.Windows.Controls.MenuItem (presentationframework.dll 1 violation)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnMouseRightButtonDown(System.Windows.Input.MouseButtonEventArgs):System.Void", MessageId="0#")]

[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnMouseLeftButtonUp(System.Windows.Input.MouseButtonEventArgs):System.Void", MessageId="0#")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID:    1380377
// Developer: Wchao
// Reason: This warning concerns an unmanaged objects which we keep as IntPtr inside the correspondent managed wrapper class.
//         This unmanaged object is not exposed to untrusted code. It is disposed and become invalid once the managed object
//         is disposed. So, they are not subject to handle-recylcing attack.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "System.Windows.Documents.NaturalLanguageHyphenator._hyphenatorResource")]

//**************************************************************************************************************************
// Bug ID:    1240400
// Developer: hongchen
// Reason: This was reviewed and approved before. But we are doing a BC to change class GridViewColumnHeader's namespace from
//         System.Windows.Controls.Primitives to System.Windows.Controls.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Controls.GridViewColumnHeader.GetCursor(System.Int32):System.Windows.Input.Cursor")]

//**************************************************************************************************************************
// Bug ID:    1830375
// Developer: hamidm
// Reason: AKaza agrees that we can exclude this violation b/c the method calling the LinkDemand
// method is marked as SecurityCritical
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Application.RunDispatcher(System.Object):System.Object")]

//*********************************************************************************************************************************
// Bug ID:    1420415
// Developer: robertan
// Reason: Reviewed the methods, and either a demand is in place or there is no risk.  These are calls from PF.dll to PUI.dll
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.PrintDialogBase.set_PrintQueue(System.Printing.PrintQueue):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.PrintDialogBase.RefreshPrintersCache():System.Void")]

//*********************************************************************************************************************************
// Bug ID:    1432336 1432337
// Developer: robertan
// Reason: Reviewed the methods, and either a demand is in place or there is no risk.  These are calls from PF.dll to S.P.dll / ReachFramework.dll
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PageSetupDialog.UpdatePrintableAreaSize():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialog.OnDialogClosing(System.ComponentModel.CancelEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID:    1399561
// Developer: dwaynen
// Reason: This was part of Akhil's work to prevent programatic paste in partial trust.  He confirmed it is OK to suppress.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope="member", Target="System.Windows.Controls.MenuItem.OnMouseLeftButtonDown(System.Windows.Input.MouseButtonEventArgs):System.Void", MessageId="0#")]


//**************************************************************************************************************************
// Bug ID:    1419776
// Developer: yutakas
// Reason: Reviewed the methods by Akhil. It has [SecurtyCritical] to call Composition.Complete which has a link demand
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.TextEditor.CompleteComposition():System.Void")]


//**************************************************************************************************************************
// Bug ID:    1470607
// Developer: wchao
// Reason: ReleaseComObject does not return sensitive info. It takes no param and is safe to be wrapped within an internal class.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.SpellerInterop.Dispose(System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID:    1471279
// Developer: vivekd
// Reason: Reviewed the methods by Akhil. He confirmed that it is OK to suppress
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.XappLauncherApp.HandleError(System.Exception,System.String,System.Uri):System.Void")]


//*********************************************************************************************************************************
// Bug ID:    1522010 1522011 1522013
// Developer: robertan
// Reason: All below supressions have been approved already (in Approved file).  However, the class names changed in a couple places
//         and I was unsure of whether I was allowed to edit the approved file rather than re-submitting in the proposed.
//         NOTE:  AllocateAndInitializeDevNames and AllocateAndInitializeDevMode are new to the LinkDemand suppression for this
//                check-in.  I have reviewed these and the use of AllocHGlobal is safe in our context.
//                AllocateAndInitializeDevMode is also new for the Aptca suppressions.  Our use of the print ticket converter is
//                safe here as well.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AllocateAndInitializeDevMode(System.String,System.Printing.PrintTicket):System.IntPtr")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler._ownerHandle")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.ExtractPrintDataAndDevMode(System.IntPtr,System.String&,System.UInt32&,System.Windows.Controls.PageRange&,System.IntPtr&):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.FreeUnmanagedPrintDlgExStruct(System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AcquireResultFromPrintDlgExStruct(System.IntPtr):System.UInt32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AcquirePrintTicket(System.IntPtr,System.String):System.Printing.PrintTicket")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.Is64Bit():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AllocateUnmanagedPrintDlgExStruct():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AllocateAndInitializeDevNames(System.String):System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "MS.Internal.Printing.Win32PrintDialog+PrintDlgExMarshaler.AllocateAndInitializeDevMode(System.String,System.Printing.PrintTicket):System.IntPtr")]

//*********************************************************************************************************************************
// Bug ID:    1522012
// Developer: robertan
// Reason: This suppression is the same as the suppressions that were once on PrintDialogBase::GetPrintTicket and SetPrintTicket.
//         We are only creating a "Default" print ticket using the PrintTicket::ctor().  This is not unsafe and exposes no extra
//         data.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialog.get_PrintTicket():System.Printing.PrintTicket")]

//*********************************************************************************************************************************
// Bug ID:    1522014 1522015
// Developer: robertan
// Reason: This warning concerns an unmanaged objects which we keep as IntPtr inside our managed marshaler.  The rational for this
//         suppression is that the the handles may not represent resources we are either responsible for allocating or destroying,
//         rather they truely point to jointly managed memory between us and Win32.  The uses of which have been examined and are
//         safe for the lifetime of the object.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "MS.Internal.Printing.NativeMethods+PRINTDLGEX32.lpPageRanges")]
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope = "member", Target = "MS.Internal.Printing.NativeMethods+PRINTDLGEX64.lpPageRanges")]

//*********************************************************************************************************************************
// Bug ID:    1520819
// Developer: grzegorz
// Reason: Body/paras of Find method has been changed without affecting any security critical code. Just simple rearrangement.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.Documents.DocumentViewerHelper.Find(MS.Internal.Documents.FindToolBar,System.Windows.Documents.TextEditor,System.Windows.Documents.ITextView,System.Windows.Documents.ITextView):System.Windows.Documents.ITextRange")]

//*********************************************************************************************************************************
// Bug ID:    1520820
// Developer: grzegorz
// Reason: OnKeyDown is simply copy-paste of existing code like this from FlowDocumentScrollViewer. We are setting a property
//         defined in non-APTCA assembly. This call does not entail any risk.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="System.Windows.Controls.FlowDocumentReader.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]


//*********************************************************************************************************************************
// Bug ID:    1523315
// Developer: huwang
// Reason: The WndProc is already marked as SecurityCritial. Talked Ariel. He agreed that we should suppress this.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]


//*********************************************************************************************************************************
// Bug ID: 1571473
// Developer: benwest
// Reason: All of these callers are SecurityCritical -- it is acceptable for them to call link demanded methods.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.SpellerInterop.EnumTextSegments(System.Char[],System.Int32,System.Windows.Documents.SpellerInterop+EnumSentencesCallback,System.Windows.Documents.SpellerInterop+EnumTextSegmentsCallback,System.Object):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.SpellerInterop.SetContextOption(System.String,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.SpellerInterop..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1580227
// Developer: jdersch
// Reason: We are explicitly not demanding for SerializationFormatter permissions here to avoid a SecurityException when
//         the Exception is marshaled across AppDomain boundaries.  We are not leaking critical data when this happens.
//         This has been reviewed by the partial trust team.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2110:SecureGetObjectDataOverrides", Scope = "member", Target = "MS.Internal.AppModel.ApplicationProxyInternal+CrossDomainSerializableException.GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1585483 - FxCop: AptcaMethodsShouldOnlyCallAptcaMethods::MS.Internal.AppModel.RootBrowserWindow (presentationframework.dll 4 violation)
// Developer: rruiz
// Reason: This method has been reviewed by security team.  We are using asserts appropriately and protecting private data.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope="member", Target="MS.Internal.AppModel.RootBrowserWindow.GetImageableRect(System.Windows.Controls.PrintDialog):System.Windows.Rect")]



//**************************************************************************************************************************
// Bug ID: 1595817 - FxCop: AptcaMethodsShouldOnlyCallAptcaMethods::MS.Internal.AppModel.XappLauncherApp (presentationframework.dll 1 violation)
// Developer: vivekd
// Reason: This method has been reviewed by security team.  Cost of creating another APTCA dlls is big
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "MS.Internal.AppModel.XappLauncherApp.HandleError(System.Exception,System.String,System.Uri,System.String):System.Void")]


//*********************************************************************************************************************************
// Bug ID:    1600494
// Developer: robertan
// Reason: This method is safely calling the PrintTicket constructor and the resulting object is being sufficiently protected.
//         Also, this method is critical and private so will not be called by 3rd parties directly.
//*********************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2116:AptcaMethodsShouldOnlyCallAptcaMethods", Scope = "member", Target = "System.Windows.Controls.PrintDialog.AcquireDefaultPrintTicket(System.Printing.PrintQueue):System.Printing.PrintTicket")]


//**************************************************************************************************************************
// Bug ID:    1640311
// Developer: rruiz
// Reason: This method calls Marshal.SizeOf (which has a link demand) but does not expose the returned size.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.ShellExecuteDefaultBrowser(System.Uri):System.Void")]


// **************************************************************************************************************************
// Bug ID:    1636835
// Developer: rruiz
// Reason: This method calls Marshal.SizeOf (which has a link demand) but does not expose the returned size.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.AppModel.AppSecurityManager.UnsafeLaunchBrowser(System.Uri):System.Void")]

// **************************************************************************************************************************
// Bug ID: 1641123, 164124
// Developer: benwest
// Reason: These methods are safe to override but not safe to call from untrusted code.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Controls.PasswordBox.OnContextMenuOpening(System.Windows.Controls.ContextMenuEventArgs):System.Void", MessageId = "0#")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Controls.Primitives.TextBoxBase.OnContextMenuOpening(System.Windows.Controls.ContextMenuEventArgs):System.Void", MessageId = "0#")]

// **************************************************************************************************************************
// Bug ID: 1641128, 1641129
// Developer: benwest
// Reason: All of these callers are SecurityCritical -- it is acceptable for them to call link demanded methods.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.CaretElement.Win32CreateCaret():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.CaretElement.Win32DestroyCaret():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.ImmComposition.OnGotFocus(System.Windows.Documents.TextEditor):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.ImmComposition.UpdateNearCaretCompositionWindow():System.Void")]

// **************************************************************************************************************************
// Bug ID: 1870119
// Developer: sangilj
// Reason: This Method calls FindNLSString (which has a link demand) but it has SecurityCritical.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Documents.TextFindEngine.FindNLSString(System.Int32,System.UInt32,System.String,System.String,System.Int32&):System.Int32")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************


// **************************************************************************************************************************
// Bug ID: 1963850
// Developer: benwest
// Reason: All of these callers are SecurityCritical -- it is acceptable for them to call link demanded methods.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.FrameworkTextComposition.CompleteCurrentComposition(MS.Win32.UnsafeNativeMethods+ITfDocumentMgr):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.FrameworkTextComposition.GetComposition(MS.Win32.UnsafeNativeMethods+ITfContext):MS.Win32.UnsafeNativeMethods+ITfCompositionView")]


//**************************************************************************************************************************
// Bug ID: DevDiv 107460
// Developer: cedricd
// Reason: FxCop is confused here. We're implementing an interface, not overriding a virtual method.
// We've added LinkDemands to the interface methods on IKeyboardInputSink.  All classes implementing this interface (HwndHost in this case)
// use an explicit interface implementation.  The only way to call the method is through the interface, so the link demand will be enforced.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndHost.System.Windows.Interop.IKeyboardInputSink.RegisterKeyboardInputSink(System.Windows.Interop.IKeyboardInputSink):System.Windows.Interop.IKeyboardInputSite")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndHost.System.Windows.Interop.IKeyboardInputSink.TranslateAccelerator(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndHost.OnKeyUp(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndHost.System.Windows.Interop.IKeyboardInputSink.TranslateChar(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndHost.System.Windows.Interop.IKeyboardInputSink.set_KeyboardInputSite(System.Windows.Interop.IKeyboardInputSite):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.HwndHost.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Interop.HwndHost.System.Windows.Interop.IKeyboardInputSink.OnMnemonic(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys):System.Boolean")]


// **************************************************************************************************************************
// Bug ID: DevDiv 198726
// Developer: andren
// Reason: Marshal.FinalReleaseComObject is only being called with the specific connection point container object.  It won't release any random COM object.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Controls.ConnectionPointCookie.#.ctor(System.Object,System.Object,System.Type)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="MS.Internal.Controls.ConnectionPointCookie.#Disconnect()")]

// **************************************************************************************************************************
// Bug ID: DevDiv 198726
// Developer: andren
// Reason: There's already an available way to verify something is COMVisible w/o trying to get ObjectForScripting to do it.
// The CLR Security team told us they would thus be removing the link demand from Marshal.IsTypeVisibleFromCom.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Controls.WebBrowser.#set_ObjectForScripting(System.Object)")]

// **************************************************************************************************************************
// Bug ID: DevDiv 198726
// Developer: andren
// Reason: Marshal.FinalReleaseComObject is only being called with the specific AxInstance this object owns, not any random COM object.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#TransitionFromLoadedToPassive()")]

// **************************************************************************************************************************
// Bug ID: DevDiv 198726
// Developer: andren
// Reason: FxCop doesn't know that the LinkDemand for an interface must be on the interface definition (IKeyboardInputSink.cs).
// Any LinkDemand on an actual interface method implementation is ignored.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.WebBrowser.#System.Windows.Interop.IKeyboardInputSink.TranslateAccelerator(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]

// **************************************************************************************************************************
// Bug ID: DevDiv 198726
// Developer: andren
// Reason: Added a LinkDemand to ActiveXHost.ActiveXSite.  Now, everyone calling that is complaining.  But they don't
// directly return the ActiveXSite to the caller, so they are OK.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#DoVerb(System.Int32)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#OnWindowPositionChanged(System.Windows.Rect)")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#StartEvents()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#StopEvents()")]
[module: SuppressMessage("Microsoft.Security","CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope="member", Target="System.Windows.Interop.ActiveXHost.#TransitionFromLoadedToRunning()")]

// **************************************************************************************************************************
// Bug ID: DevDiv 200291
// Developer: grzegorz
// Reason: Method has link demand, but overrides method without link demand. In this case, we do not trust the input 
// ContextMenuEventArgs (which may have its UserInitiated bit set), so we do not tolerate this method being called in 
// partial trust by a derived class. However, it is perfectly reasonable for a derived class in partial trust to override 
// this method and bring up a custom menu without any clipboard related items.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase.#OnContextMenuOpening(System.Windows.Controls.ContextMenuEventArgs)")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: Implements a method from ISerializable interface, not overriding. Fxcop does not differentiate between implementing and overriding
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope = "member", Target = "System.Windows.Navigation.JournalEntry.#GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]

//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason:  Even though the signature looks like an event handler's, these methods are not event handlers, they donot raise any events
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Setter.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Setter.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Trigger.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]

//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason:  DependencyPropertyKeys are generally declared read-only. 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2104:DoNotDeclareReadOnlyMutableReferenceTypes", Scope = "member", Target = "System.Windows.Controls.DataGridColumn.#ActualWidthPropertyKey")]

//**************************************************************************************************************************
// Feature: Win7 Integration
// Developer: joecast
// Reason: Ignorable FxCop warnings: Intentional naming violations.  LinkDemands coming through because
//         the class is locked down.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Shell.JumpList.#BeginInit()")]
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Shell.JumpList.#EndInit()")]

//**************************************************************************************************************************
// Bug ID: 632972 (DevDiv TFS Dev10)
// Developer: jezhan
// Reason: This would break public API
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Condition.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Condition.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.DataTrigger.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]
//**************************************************************************************************************************
// Developer: arathira
// Reason: Callers are marked SecurityCritical and TreatAsSafe because CompositionTarget.TransformToDevice
// and CompositionTarget.TransformToDevice are treated as safe.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.FrameworkElement.#GetDpi(System.Windows.Vector,System.Windows.FrameworkElement,System.Double&,System.Double&)")]
//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: kedecond
// Reason: XamlLoadPermission constructor saves a copy of AllowedAccess, so it should be safe.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Windows.DeferrableContent.#.ctor(System.IO.Stream,System.Windows.Baml2006.Baml2006SchemaContext,System.Xaml.IXamlObjectWriterFactory,System.IServiceProvider,System.Object)")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Windows.SystemResources+ResourceDictionaries.#LoadDictionary(System.Reflection.Assembly,System.String,System.String,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 732400 (DevDiv TFS Dev10)
// Developer: juhanit
// Reason: There is a LinkDemand on PresentationSource.get_CompositionTarget. GetClippedPositionOffsets calls this getter, 
// but GetClippedPositionOffsets is marked as SecurityCritical and the CompositionTarget is never exposed.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Scope = "member", Target = "System.Windows.Documents.TextEditorContextMenu.#GetClippedPositionOffsets(System.Windows.Documents.TextEditor,System.Windows.Documents.ITextPointer,System.Windows.Documents.LogicalDirection,System.Double&,System.Double&)")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: hrachyam
// Reason: These methods have passed security reviews.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Documents.SpellerInterop.#AddLexicon(System.String)", Justification="By Design and reviewed by Security Team")]
[module: SuppressMessage("Microsoft.Security","CA2103:ReviewImperativeSecurity", Scope="member", Target="System.Windows.Documents.SpellerInterop.#LoadDictionary(System.Uri,System.String)", Justification="By design and reviewed by Security Team")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: hrachyam
// Reason: The intent of this method is to accept as input parameter ONLY TextBoxBase derived objects.
// Changing type of input parameter from TextoxBase to DependencyObject, as suggested by FxCop message for this issue is not appropriate.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.SpellCheck.#GetCustomDictionaries(System.Windows.Controls.Primitives.TextBoxBase)")]

// New suppressions since V4 RTM:

//**************************************************************************************************************************
// We have equivalent full demands in place on these methods since LinkDemands are no longer allowed.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.ResourceReferenceKeyNotFoundException.#GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]
[module: SuppressMessage("Microsoft.Security","CA2123:OverrideLinkDemandsShouldBeIdenticalToBase", Scope="member", Target="System.Windows.Markup.XamlParseException.#GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]
