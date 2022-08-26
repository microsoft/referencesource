//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Controls.ActiveXHost.DoVerb(System.Int32):System.Boolean", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Documents.DocumentsTrace.TraceCallers(System.Int32):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.GetInfo(System.Int32,System.Object,MS.Internal.Data.PropertyPathWorker+SourceValueState&):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.GetInfo(System.Int32,System.Object,MS.Internal.Data.PropertyPathWorker+SourceValueState&):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.GetInfo(System.Int32,System.Object,MS.Internal.Data.PropertyPathWorker+SourceValueState&):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextBuilder.EnsureTextOMForPage(System.Int32):System.Boolean", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextBuilder._GetFixedNodes(System.Windows.Documents.FixedPageStructure,System.Collections.IEnumerable,System.Int32,System.Int32,System.Int32[],System.Boolean,System.Collections.Generic.List`1<System.Windows.Documents.FixedNode>,System.Windows.Media.Matrix):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextBuilder.AddVirtualPage():System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPage.GetElement(System.Windows.Documents.FixedNode):System.Windows.DependencyObject", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPage.GetElement(System.Windows.Documents.FixedNode):System.Windows.DependencyObject", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FlowNode.ToString():System.String", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPagesChanged(System.Object,System.Windows.Documents.PagesChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._GetPageAsyncDelegate(System.Object):System.Object", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._NotifyGetPageAsyncCompleted(System.Windows.Documents.DocumentPage,System.Int32,System.Exception,System.Boolean,System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnGetPageCompleted(System.Object,System.Windows.Documents.GetPageCompletedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPaginationCompleted(System.Object,System.EventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence.System.Windows.Markup.IAddChild.AddChild(System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence.CancelAsync(System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence.GetPage(System.Int32):System.Windows.Documents.DocumentPage", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence.GetPageAsync(System.Int32,System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPaginationProgress(System.Object,System.Windows.Documents.PaginationProgressEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnDocumentReferenceInitialized(System.Object,System.EventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.HighlightVisual.OnRender(System.Windows.Media.DrawingContext):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextView.GetRawRectangleFromTextPosition(System.Windows.Documents.ITextPointer,System.Windows.Media.Transform&):System.Windows.Rect", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextView.GetLineRange(System.Windows.Documents.ITextPointer):System.Windows.Documents.TextSegment", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextView.GetPositionAtNextLine(System.Windows.Documents.ITextPointer,System.Double,System.Int32,System.Double&,System.Int32&):System.Windows.Documents.ITextPointer", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextView.GetTextPositionFromPoint(System.Windows.Point,System.Boolean):System.Windows.Documents.ITextPointer", MessageId="System.String.Format(System.String,System.Object,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextView._CreateTextPointer(System.Windows.Documents.FixedPosition,System.Windows.Documents.LogicalDirection):System.Windows.Documents.ITextPointer", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DebugVisualAdorner.OnRender(System.Windows.Media.DrawingContext):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DebugVisualAdorner._RenderMarkupOrder(System.Windows.Media.DrawingContext,System.Collections.Generic.List`1<System.Windows.Documents.FixedNode>):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedSOMPage.Render(System.Windows.Media.DrawingContext,System.String,System.Windows.Documents.DrawDebugVisual):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceHighlightLayer.RaiseHighlightChangedEvent(System.Collections.IList):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedSOMTable.Render(System.Windows.Media.DrawingContext,System.String,System.Windows.Documents.DrawDebugVisual):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedSOMFixedBlock.Render(System.Windows.Media.DrawingContext,System.String,System.Windows.Documents.DrawDebugVisual):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextBuilder+FlowModelBuilder.DumpFlowNode(System.Windows.Documents.FlowNode):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedTextBuilder+FlowModelBuilder.DumpToFile(System.String):System.Void", MessageId="System.IO.StringWriter.#ctor")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.SyncGetPageWithCheck(System.Int32):System.Windows.Documents.FixedPage", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.SyncGetPage(System.Int32,System.Boolean):System.Windows.Documents.FixedPage", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument._NotifyGetPageAsyncCompleted(System.Windows.Documents.DocumentPage,System.Int32,System.Exception,System.Boolean,System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.GetPageAsyncDelegate(System.Object):System.Object", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.GetPageAsync(System.Int32,System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.CancelAsync(System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.OnGetPageRootCompleted(System.Object,System.Windows.Documents.GetPageRootCompletedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.OnHighlightChanged(System.Object,System.Windows.Documents.HighlightChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.System.Windows.Markup.IAddChild.AddChild(System.Object):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.OnPageLoaded(System.Object,System.EventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.GetPage(System.Int32):System.Windows.Documents.DocumentPage", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.GetObjectPosition(System.Object):System.Windows.Documents.ContentPosition", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedDocument.GetPageSize(System.Windows.Size&,System.Int32):System.Boolean", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedSOMTableRow.Render(System.Windows.Media.DrawingContext,System.String,System.Windows.Documents.DrawDebugVisual):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextView.GetPositionAtNextLine(System.Windows.Documents.ITextPointer,System.Double,System.Int32,System.Double&,System.Int32&):System.Windows.Documents.ITextPointer", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextView.GetRawRectangleFromTextPosition(System.Windows.Documents.ITextPointer,System.Windows.Media.Transform&):System.Windows.Rect", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextView.Contains(System.Windows.Documents.ITextPointer):System.Boolean", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextView.GetLineRange(System.Windows.Documents.ITextPointer):System.Windows.Documents.TextSegment", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextView.GetTextPositionFromPoint(System.Windows.Point,System.Boolean):System.Windows.Documents.ITextPointer", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.PageContent.GetPageRootAsync(System.Boolean):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.PageContent.GetPageRootAsyncCancel():System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.PageContent.GetPageRoot(System.Boolean):System.Windows.Documents.FixedPage", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.RenderLines(System.Windows.Media.DrawingContext):System.Void", MessageId="System.Int32.ToString")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.RenderFlowNode(System.Windows.Media.DrawingContext):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.RenderFlowNode(System.Windows.Media.DrawingContext):System.Void", MessageId="System.Convert.ToString(System.Int32)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.ToString():System.String", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.SetupLineResults(System.Windows.Documents.FixedLineResult[]):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedPageStructure.SetupLineResults(System.Windows.Documents.FixedLineResult[]):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextContainer._OnHighlightChanged(System.Object,System.Windows.Documents.HighlightChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextContainer._OnHighlightChanged(System.Object,System.Windows.Documents.HighlightChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.FixedHighlight.ComputeDesignRect():System.Windows.Rect", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentReference.GetDocument(System.Boolean):System.Windows.Documents.FixedDocument", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentReference.GetDocument(System.Boolean):System.Windows.Documents.FixedDocument", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentReference.Dump():System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentReference.OnSourceChanged(System.Windows.DependencyObject,System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Documents.DocumentReference.OnSourceChanged(System.Windows.DependencyObject,System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId="System.String.Format(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Window.CalculateCenterScreenPosition(System.Runtime.InteropServices.HandleRef,System.Windows.Size,System.Double&,System.Double&):System.Void", MessageId="System.String.Format(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Window.UpdateHwndRestoreBounds(System.Double,System.Windows.Window+BoundsSpecified):System.Void", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.SystemResourceKey..ctor(System.Windows.SystemResourceKeyID)", MessageId="System.String.Format(System.String,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.IO.Packaging.XamlFilter.DumpElementTable():System.String", MessageId="System.String.Format(System.String,System.Object,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Baml2006.Baml2006SchemaContext.#GetAssemblyName(System.Int16)", MessageId="System.Int16.ToString")]
[module: SuppressMessage("Microsoft.Globalization","CA1305:SpecifyIFormatProvider", Scope="member", Target="System.Windows.Baml2006.Baml2006SchemaContext.#GetXamlType(System.Int16)", MessageId="System.Int16.ToString")]

#endif

//**************************************************************************************************************************
// Bug ID: 1048365
// Developer: ChaiwatP (sambent)
// Reason: These are trace strings used for debugging. They are not visible to users.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.Subscribe(System.Windows.Data.CollectionContainer):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.ProcessCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.OnContainedCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1050486
// Developer: ChaiwatP (bchapman)
// Reason:
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Win32.HwndWrapper..ctor(System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.Int32,System.String,System.IntPtr,MS.Win32.HwndWrapperHook[])")]

//**************************************************************************************************************************
// Bug ID: 1061909
// Developer: ChaiwatP (ghermann)
// Reason: In this case, calling the ctor with cultureinfo would force this into the 'override' state, which is not what we want. we allocate this
// 	object and fill in the fields appropriately.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1304:SpecifyCultureInfo", Scope="member", Target="MS.Internal.Text.DynamicPropertyReader.GetNumberSubstitution(System.Windows.DependencyObject):System.Windows.Media.NumberSubstitution")]

//**************************************************************************************************************************
// Bug ID: 1070239
// Developer: ChaiwatP (sambent)
// Reason: These are trace strings used for debugging. They are not visible to users.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.Unsubscribe(System.Windows.Data.CollectionContainer,System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1079520
// Developer: ChaiwatP (sambent)
// Reason: These are trace strings used for debugging. They are not visible to users.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemAdded(System.Object,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnRefresh():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1081890
// Developer: ChaiwatP (davidjen)
// Reason: By design, resurfaced as new bug since renamed class Binding to BindingExpression. All Literals are input to internal dev trace logs that
//	Only are active if built with special #define (not set by build labs).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.CollectionContainer.HookUpToCollection(System.Collections.IEnumerable,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.DetachOverride():System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.AttachOverride(System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpressionBase.SetStatus(System.Windows.Data.BindingStatus):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.Deactivate():System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.DetachFromContext():System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.AttachToContext(System.Windows.Data.BindingExpression+AttachAttempt):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.OnDataChanged(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.Activate(System.Object):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1092101
// Developer: ChaiwatP (actdata)
// Reason: By design, resurfaced as new bug since renamed class Binding to BindingExpression. All Literals are input to internal dev trace logs that
//	Only are active if built with special #define (not set by build labs).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.DereferenceDataSource(System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1206726
// Developer: ChaiwatP (sambent)
// Reason: These are trace strings used for debugging. They are not visible to users.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.System.Windows.IWeakEventListener.ReceiveWeakEvent(System.Type,System.Object,System.EventArgs):System.Boolean")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.BindingExpression.DereferenceDataProvider(System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1038727
// Developer: ChaiwatP (actacc)
// Reason: The accessibility team is asking for an exemption for all the FxCop violations for Automation namespaces.
// For that reason, I suggest you guys should go ahead and move the violations below to the suppression assemblies.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.ValidateAndThrow(System.Windows.Automation.Provider.ITextRangeProvider):MS.Internal.Automation.TextRangeAdaptor")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.System.Windows.Automation.Provider.ITextRangeProvider.GetText(System.Int32,System.Boolean):System.String")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.System.Windows.Automation.Provider.ITextRangeProvider.FindText(System.String,System.Boolean,System.Boolean,System.Boolean):System.Windows.Automation.Provider.ITextRangeProvider")]

//**************************************************************************************************************************
// Bug ID: 1313655
// Developer: KenLai
// Reason: String literal is name of variable and does not need to be localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Data.XmlNamespaceMappingCollection.CopyTo(System.Windows.Data.XmlNamespaceMapping[],System.Int32):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1325745
// Developer: MingChan
// Reason: We access and use the public properties(day,year,month) of DateTime object seperately and they are integer values
// which are independent of the CultureInfo. CultureInfo has no effect on converting an integer to string.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1305:SpecifyIFormatProvider", Scope="member", Target="MS.Internal.DeploymentExceptionMapper.ConstructFwlinkUrl(System.String,System.Uri&):System.Boolean", MessageId="System.Int32.ToString")]

//**************************************************************************************************************************
// Bug ID: 1312776
// Developer: Sangilj
// Reason: String literals are all control character, so it doesn't need to be localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Documents.RtfToXamlReader.ProcessTextSymbol(System.Windows.Documents.Token):System.Void", MessageId="System.Windows.Documents.Token.set_Text(System.String)")]

//**************************************************************************************************************************
// Bug ID: 1312776, 1424261
// Developer: Sangilj
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Documents.RtfToXamlReader.HandleControl(System.Windows.Documents.RtfToken,System.Windows.Documents.RtfControlWordInfo):System.Void", MessageId="System.Windows.Documents.RtfToXamlReader.ProcessText(System.String)")]

//**************************************************************************************************************************
// Bug ID: 1432995, 1432996
// Developer: mharper
// Reason: These are trace strings used for debugging. They are not visible to users.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.TraceContainerCollectionChange(System.Object,System.Collections.Specialized.NotifyCollectionChangedAction,System.Object,System.Object):System.Void", MessageId="MS.Internal.Utility.TraceLog.Add(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemMoved(System.Object,System.Int32,System.Int32):System.Void", MessageId="MS.Internal.Utility.TraceLog.Add(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemRemoved(System.Object,System.Int32):System.Void", MessageId="MS.Internal.Utility.TraceLog.Add(System.String,System.Object[])")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemReplaced(System.Object,System.Object,System.Int32):System.Void", MessageId="MS.Internal.Utility.TraceLog.Add(System.String,System.Object[])")]

//**************************************************************************************************************************
// Bug ID: 1453526
// Developer: PJoshi
// Reason: String literal passed is Unicode space character, so it doesn't need to be localized.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="System.Windows.Documents.TextEditorTyping.OnSpace(System.Object,System.Windows.Input.ExecutedRoutedEventArgs):System.Void", MessageId="System.Windows.Documents.TextEditorTyping+TextInputItem.#ctor(System.Windows.Documents.TextEditor,System.String,System.Boolean)")]

//**************************************************************************************************************************
// Bug ID: 1625248
// Developer: KenLai
// Reason: String literals are used for trace messages and will not be localized
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.OnTrace(MS.Internal.AvTraceBuilder,System.Object[],System.Int32):System.Void", MessageId="MS.Internal.AvTraceBuilder.Append(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.DescribeTarget(MS.Internal.AvTraceBuilder,System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Void", MessageId="MS.Internal.AvTraceBuilder.Append(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.DescribeTarget(MS.Internal.AvTraceBuilder,System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Void", MessageId="MS.Internal.AvTraceBuilder.AppendFormat(System.String,System.String,System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.DescribeSourceObject(MS.Internal.AvTraceBuilder,System.Object):System.Void", MessageId="MS.Internal.AvTraceBuilder.AppendFormat(System.String,System.String,System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.DescribeSourceObject(MS.Internal.AvTraceBuilder,System.Object):System.Void", MessageId="MS.Internal.AvTraceBuilder.AppendFormat(System.String,System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.Describe(MS.Internal.AvTraceBuilder,System.Object):System.Void", MessageId="MS.Internal.AvTraceBuilder.Append(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope="member", Target="MS.Internal.TraceData.Describe(MS.Internal.AvTraceBuilder,System.Object):System.Void", MessageId="MS.Internal.AvTraceBuilder.AppendFormat(System.String,System.String)")]



//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 632972 (DevDiv TFS Dev10)
// Developer: jezhan
// Reason: xmlns syntax is not subject to localization
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Windows.Baml2006.Baml2006Reader.#Logic_GetFullXmlns(System.String)")]
[module: SuppressMessage("Microsoft.Globalization","CA1305:SpecifyIFormatProvider", MessageId="System.Int32.ToString(System.IFormatProvider)", Scope="member", Target="System.Windows.Markup.XamlTemplateSerializer.#ThrowException(System.String,System.Int32,System.Int32,System.Exception)", Justification="Retain compat with v3")]
