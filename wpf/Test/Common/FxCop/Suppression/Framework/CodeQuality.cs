//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//SherifM
//bug# 1038718
//As per Actprog request.
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="System.Windows.Application.set_StartupUri(System.Uri):System.Void")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1074773
// Developer: chandras
// Reason: As per CoreUI Leads, this is an Architecture issue of property engine, so created a task 32917 to track this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.DocumentViewer..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1010162
// Developer: MCalkins
// Reason: Most of these pass the new requirements for this, one specific category
//         does not and we will have to fix those in M11.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineThicknessKeyFrame..ctor(System.Windows.Thickness,System.Windows.Media.Animation.KeyTime,System.Windows.Media.Animation.KeySpline)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.SplineThicknessKeyFrame..ctor(System.Windows.Media.Animation.SplineThicknessKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Media.Animation.ThicknessAnimation,System.Windows.Media.Animation.Animatable+CloneType)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Media.Animation.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Media.Animation.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessKeyFrame..ctor(System.Windows.Media.Animation.ThicknessKeyFrame)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessKeyFrame..ctor(System.Windows.Thickness,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessKeyFrame..ctor(System.Windows.Thickness)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimationUsingKeyFrames..ctor(System.Windows.Media.Animation.ThicknessAnimationUsingKeyFrames,System.Windows.Media.Animation.Animatable+CloneType)")]

//**************************************************************************************************************************
// Bug ID: 1038684
// Developer: BChapman/Chandras
// Reason: This is Win32 Interop Code and need to be that way. We send a destroywindow message and expect the window
//         to be deleted in correct way by win32 and we shouldn't be dealing with dispose here as it can turn the
//     win32 event state wacky.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="System.Windows.Interop.HwndHost.Dispose(System.Boolean):System.Void", MessageId="_hwndHostedSubclass")]
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="System.Windows.Interop.HwndHost.Dispose(System.Boolean):System.Void", MessageId="_hwndWrapperSubclass")]

//**************************************************************************************************************************
// Bug ID: 1038660
// Developer: SherifM
// Reason: UIAutomation, no plans to fix FxCop bugs.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.System.Windows.Automation.Provider.ITextRangeProvider.FindText(System.String,System.Boolean,System.Boolean,System.Boolean):System.Windows.Automation.Provider.ITextRangeProvider")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.System.Windows.Automation.Provider.ITextRangeProvider.GetText(System.Int32,System.Boolean):System.String")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.ValidateAndThrow(System.Windows.Automation.Provider.ITextRangeProvider):MS.Internal.Automation.TextRangeAdaptor")]

//**************************************************************************************************************************
// Bug ID: 1061887
// Developer: SherifM
// Reason: See bug.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="System.IO.Packaging.PackWebResponse.System.IDisposable.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1070236
// Developer: SherifM
// Reason: See bug.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.InkCanvas..ctor()")]


//**************************************************************************************************************************
// Bug ID: 1163773
// Developer: CedricD
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineThicknessKeyFrame..ctor(System.Windows.Thickness,System.Windows.Media.Animation.KeyTime)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.SplineThicknessKeyFrame..ctor(System.Windows.Thickness)")]

//**************************************************************************************************************************
// Bug ID: 1206718
// Developer: JeremyNS
// Reason: The conflict is with MS.Internal.Documents.Application, which does not have any public members (nor will it ever).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1724:TypeNamesShouldNotMatchNamespaces", Scope="type", Target="System.Windows.Application")]

//**************************************************************************************************************************
// Bug ID: 1218572, 1319521
// Developer: DwayneN, KenLai
// Reason: As per CoreUI Leads, this is an Architecture issue of property engine.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Input.KeyboardNavigation+FocusVisualAdorner..ctor(System.Windows.ContentElement,System.Windows.UIElement,System.Windows.Style)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Input.KeyboardNavigation+FocusVisualAdorner..ctor(System.Windows.UIElement,System.Windows.Style)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Internal.Controls.TemplatedAdorner..ctor(System.Windows.UIElement,System.Windows.Controls.ControlTemplate)")]


//**************************************************************************************************************************
// Bug ID: 1235319
// Developer: rruiz
// Reason: This is the word used by the Flow team to describe objects that can create pages from content.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2204:LiteralsShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.IDocumentPaginator,System.Windows.Annotations.Storage.AnnotationStore)")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.WriteAsync(System.Windows.Documents.DocumentPaginator,System.Printing.PrintTicket):System.Void", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.Write(System.Windows.Documents.DocumentPaginator):System.Void", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.WriteAsync(System.Windows.Documents.DocumentPaginator,System.Object):System.Void", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.WriteAsync(System.Windows.Documents.DocumentPaginator):System.Void", MessageId="0#Paginator")]

//**************************************************************************************************************************
// Bug ID: 1233106
// Developer: BenCar
// Reason: As per CoreUI Leads, this is an Architecture issue of property engine. Also GetValueCore (the virtual method) is sealed in a base class (FrameworkElement)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Primitives.Selector..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1233107
// Developer: psarrett
// Reason: As per CoreUI Leads, this is an Architecture issue of property engine.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1284595
// Developer: grzegorz
// Reason: The same instance of PageBreakRecord is stored in 2 places:
//     1) PtsPage - creates it and uses it internally, also passes the instance to BreakRecordTable
//     2) BreakRecordTable (lifetime of this object is greater than PtsPage) – stores to enable formatting
//        of any page without calculating preceding pages.
//     BreakRecordTable when invalidated calls PageBreakRecord.Dispose(). Since lifetime of BreakRecordTable
//     is greater than PtsPage, PtsPage cannot call Dispose(), because this object will be used later.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="MS.Internal.PtsHost.PtsPage.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313652
// Developer: grzegorz
// Reason: DocumentPageView.Dispose calls internally DestroyDocumentPage() which calls _documentPage.Dispose(). This is false warning.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="System.Windows.Controls.Primitives.DocumentPageView.System.IDisposable.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1325743
// Developer: ChangoV
// Reason: There is no real issue here, since this is a closed hierarchy of classes, and we don't
//   override this DependencyObject.GetValueCore(). (And the fact that there is such an override
//   is completely irrelevant to JournalEntry.)
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Navigation.JournalEntry..ctor(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext)")]

//**************************************************************************************************************************
// Bug ID: 1340803
// Developer: arathira
// Reason: The _breakRecord member in PtsPage is a reference to a break record that is owned by the the BreakRecordTable. The
// record should be disposed with the BreakRecordTable and not here.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope = "member", Target = "MS.Internal.PtsHost.PtsPage.Dispose(System.Boolean):System.Void", MessageId = "_breakRecord")]

//**************************************************************************************************************************
// Bug ID: 1344480
// Developer: grzegorz
// Reason: The _firstChild member is Disposed in loop together with other children on TableParagraph.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2213:DisposableFieldsShouldBeDisposed", Scope="member", Target="MS.Internal.PtsHost.TableParagraph.Dispose():System.Void", MessageId="_firstChild")]


//**************************************************************************************************************************
// Bug IDs: 1309458
// Developer: gillesk,
// Reason: Excluding this following the guidelines at http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimation..ctor(System.Windows.Thickness,System.Windows.Thickness,System.Windows.Duration,System.Windows.Media.Animation.FillBehavior)")]

//**************************************************************************************************************************
// Bug IDs: 1380350
// Developer: eveselov
// Reason: Formatting properties must be transferred from RichTextBox to its FlowDocument during construction; So there is no way to avoid FlowDocument.SetValue calls.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Controls.RichTextBox..ctor(System.Windows.Documents.FlowDocument)")]

//**************************************************************************************************************************
// Bug IDs: 1426660
// Developer: jdersch
// Reason: We are validating the input to ensure it falls within valid range; the analysis tools don't appear to be picking this up.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope = "member", Target = "System.Windows.Controls.DocumentViewer.OnBringIntoView(System.Windows.DependencyObject,System.Windows.Rect,System.Int32):System.Void", MessageId = "pageNumber-1")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug IDs: 121542
// Developer: benwest
// Reason: "mergeable" is used in published apis.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.TextElementEditingBehaviorAttribute.IsMergeable", MessageId="Mergeable")]


//**************************************************************************************************************************
// Bug IDs: 140708
// Developer: sambent
// Reason: A new ShutDownListener is "used" because it listens to events like AppDomain.DomainUnload.
//      The reference implicit in the event's delegate list keeps the object alive.  FxCop doesn't understand this.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="MS.Internal.PtsHost.PtsCache.#.ctor(System.Windows.Threading.Dispatcher)", MessageId="MS.Internal.PtsHost.PtsCache+PtsCacheShutDownListener")]
[module: SuppressMessage("Microsoft.Usage", "CA1806:DoNotIgnoreMethodResults", Scope="member", Target="MS.Internal.Data.DataBindEngine.#.ctor()", MessageId="MS.Internal.Data.DataBindEngine+DataBindEngineShutDownListener")]


//**************************************************************************************************************************
// Bug IDs: no bug
// Developer: sambent
// Reason: FxCop doesn't understand that index must be >0 because of the previous line:  if (index == 0) throw...
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2233:OperationsShouldNotOverflow", Scope="member", Target="System.Windows.Data.BindingListCollectionView.#RemoveAt(System.Int32)", MessageId="index-1")]

//**************************************************************************************************************************
// Feature: Win7 Integration
// Developer: joecast
// Reason: Ignorable FxCop warnings: Intentional naming violations.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Shell.TaskbarItemInfo.#ThumbButtonInfos")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Infos", Scope="member", Target="System.Windows.Shell.TaskbarItemInfo.#ThumbButtonInfos")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Infos", Scope="member", Target="System.Windows.Shell.TaskbarItemInfo.#ThumbButtonInfosProperty")]

//**************************************************************************************************************************
// Bug ID: 692896 Dev10
// Developer: joecast
// Reason: Context is understood and these are ignorable.  We don't want to unnecessarily touch this code.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.NativeMethods+IOleCommandTarget.Exec(MS.Win32.NativeMethods+GUID,System.Int32,System.Int32,System.Object[],System.Int32)", Scope="member", Target="System.Windows.Controls.WebBrowser.#DoNavigate(System.Uri,System.Object&,System.Object&,System.Object&)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.AppModel.XappLauncherApp", Scope="member", Target="System.Windows.Interop.DocObjHost.#MS.Internal.AppModel.IBrowserHostServices.Run(System.String,System.String,MS.Internal.AppModel.MimeType,System.String,System.String,System.Object,System.Object,System.String,System.Boolean,System.Boolean,System.Boolean,System.Boolean,MS.Internal.AppModel.INativeProgressPage,System.String,System.String,System.String,System.String)")]

