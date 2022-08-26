//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//  BugID 1010656
//  Developer: SherifM
//  Reason: Approved by Actprog
[module: SuppressMessage("Microsoft.Usage", "CA2216:DisposableTypesShouldDeclareFinalizer", Scope="member", Target="MS.Internal.PtsHost.UnmanagedHandle.Dispose():System.Void")]


//  BugID Multiple bugs.
//  Developer: SherifM
//  Reason: AvoidExcessiveComplexity, Approved by Actprog
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Globalization.BamlResourceDeserializer.LoadBamlImp(System.IO.Stream):MS.Internal.Globalization.BamlTree")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Media.Animation.ThicknessAnimationUsingKeyFrames.ResolveKeyTimes(System.Windows.Media.Animation.AnimationClock):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Shapes.Glyphs.ParseGlyphsProperty(System.Windows.Media.GlyphTypeface,System.String,System.Boolean,System.Collections.Generic.List`1<System.Windows.Shapes.Glyphs+ParsedGlyphData>&,System.UInt16[]&):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Documents.TextDocumentView.GetPositionAtNextLine(System.Collections.ObjectModel.ReadOnlyCollection`1<MS.Internal.Documents.ParagraphResult>,System.Windows.Documents.ITextPointer,System.Double,System.Int32&):System.Windows.Documents.ITextPointer")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Ink.SelectionEditingBehavior.ChangeFeedbackRectangle(System.Windows.Point):System.Windows.Rect")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemAdded(System.Object,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.ItemContainerGenerator.OnItemRemoved(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.MenuItem.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.ComboBox.KeyDownHandler(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.ListBox.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.KeyboardNavigation.GetPrevTab(System.Windows.DependencyObject,System.Windows.DependencyObject,System.Boolean):System.Windows.DependencyObject")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.ToolBarTray.ProcessThumbDragDelta(System.Windows.Controls.Primitives.DragDeltaEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.PtsHost.TextParaClient.NextCaretUnitPositionFromDcpCompositeLines(System.Int32,System.Windows.Documents.ITextPointer,System.Windows.Documents.LogicalDirection,MS.Internal.PtsHost.UnsafeNativeMethods.PTS+FSTEXTDETAILSFULL&):System.Windows.Documents.ITextPointer")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor.MoveToNext(System.Windows.Documents.ITextPointer,System.Windows.Automation.Text.TextUnit,System.Windows.Documents.LogicalDirection):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Automation.TextRangeAdaptor..cctor()")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.Primitives.TickBar.OnRender(System.Windows.Media.DrawingContext):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.Primitives.Selector.OnItemsChanged(System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.FixedTextBuilder._FlowOrderAnalysis(System.Int32,System.Collections.IEnumerable,System.Int32,System.Int32,System.Int32[],System.Windows.Documents.FixedNode,System.Windows.Documents.FixedNode,System.Windows.Documents.FlowNode[]&,System.Windows.Documents.FixedNode[][]&):System.Collections.ArrayList")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.TextSchema.IsValidParent(System.Type,System.Type):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.TextEditorLists.OnListCommand(System.Object,System.Windows.Input.ExecuteEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.ImmComposition.OnWmImeNotify(System.IntPtr,System.IntPtr,System.IntPtr,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.ImmComposition.UpdateCompositionString(System.Char[],System.Char[],System.Int32,System.Int32,System.Int32[],System.Byte[]):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.Table.ValidateTableWidths(System.Double,System.Double&):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlReader.ReadNextRecord():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlReader.ProcessKeyTree():System.Windows.Markup.BamlReader+BamlKeyInfo")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlReader.ProcessPropertyRecord():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.TemplateBamlRecordReader.ReadElementEndRecordForVisualTrigger():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.StyleBamlRecordReader.ReadElementEndRecordForVisualTrigger():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.StyleBamlRecordReader.ReadPropertyComplexStartRecord(System.Windows.Markup.BamlPropertyComplexStartRecord):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlParser.ProcessXamlNode(System.Windows.Markup.XamlNode,System.Boolean&,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlMapTable.GetConstructor(System.Windows.Markup.BamlMapTable+KnownElements):System.Windows.Markup.BamlMapTable+ObjectConstructor")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlMapTable.GetKnownConverterTypeFromType(System.Type):System.Type")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlMapTable.CreateKnownConverterFromId(System.Int16,System.Type):System.ComponentModel.TypeConverter")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.MarkupExtensionParser.TokenizeAttributes(System.String,System.Int32,System.Int32):System.Collections.ArrayList")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlRecordReader.ReadElementEndRecord(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlRecordReader.ReadRecord(System.Windows.Markup.BamlRecord):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlRecordReader.ReadTextRecord(System.Windows.Markup.BamlTextRecord):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlTypeMapper.GetClrInfoForClass(System.Boolean,System.Type,System.String,System.String,System.String,System.String&):System.Reflection.MemberInfo")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.CompileBamlTag(System.Xml.XmlNodeType,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.DetermineIfPropertyComplex(System.Boolean,System.String&,System.Boolean&,System.Boolean&):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.CompileElement(System.String,System.String,System.Int32,System.Type,System.String,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.WriteDefAttributes(System.Int32,System.String,System.String,System.Type,System.Collections.ArrayList&,System.Collections.Specialized.HybridDictionary,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.CompilePI():System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.AddNodeToCollection(System.Windows.Markup.XamlNode):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.WritePropertyAttribute(System.Type,System.Collections.Specialized.HybridDictionary,System.Reflection.PropertyInfo,System.String,System.String,System.String,System.String,System.Object,System.String,System.String,System.String,System.Type,System.Int32,System.Int32,System.Int32,System.Collections.ArrayList&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlRecordManager.AllocateRecord(System.Windows.Markup.BamlRecordType):System.Windows.Markup.BamlRecord")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Data.ListCollectionView.ProcessCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Data.BindingExpression.TransferValue(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemParameters.InvalidateCache():System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemParameters.InvalidateCache(System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemColors.SlotToFlag(System.Windows.SystemColors+CacheSlot):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.GetRawValue(System.Windows.DependencyProperty,System.Object,System.Windows.PropertyMetadata):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.OnPropertyInvalidated(System.Windows.DependencyProperty,System.Windows.PropertyMetadata):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.TreeWalkHelper.InvalidateStyleAndReferences(System.Windows.DependencyObject,System.Windows.ResourcesChangeInfo,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemResourceKey.get_Resource():System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Window.CreateSourceWindowImpl():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.GetChildValueHelper(System.Windows.UncommonField`1<System.Collections.Specialized.HybridDictionary[]>,MS.Utility.ItemStructList`1<System.Windows.ChildValueLookup>&,System.Windows.DependencyProperty,System.Windows.DependencyObject,System.Windows.FrameworkElement,System.Windows.FrameworkContentElement,System.Int32,System.Boolean,System.Windows.ValueLookupType&,System.Windows.FrameworkElementFactory,System.Object):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.DoTemplateInvalidations(System.Windows.FrameworkElement,System.Windows.FrameworkContentElement,System.Windows.FrameworkTemplate,System.Windows.Documents.TableTemplate):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.UpdateLoadedFlags(System.Windows.DependencyObject,System.Windows.Style,System.Windows.Style,System.Windows.FrameworkTemplate,System.Windows.FrameworkTemplate,System.Windows.Documents.TableTemplate,System.Windows.Documents.TableTemplate):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkContentElement.GetRawValue(System.Windows.DependencyProperty,System.Object,System.Windows.PropertyMetadata):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CommandConverter.GetKnownCommand(System.String,System.Type):System.Windows.Input.UICommand")]

//  BugID Multiple bugs.
//  Developer: SherifM
//  Reason: EnumStorageShouldBeInt32, Approved by Actprog.
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Markup.BamlAttributeUsage")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.ShutdownMode")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.ReasonSessionEnding")]
[module: SuppressMessage("Microsoft.Design", "CA1028:EnumStorageShouldBeInt32", Scope="type", Target="System.Windows.Navigation.NavigationMode")]


//  BugID: 1010856 & 1017686
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.ColumnDefinitionCollection.Contains(System.Windows.Controls.ColumnDefinition):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.ColumnDefinitionCollection.IndexOf(System.Windows.Controls.ColumnDefinition):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.ColumnDefinitionCollection.Remove(System.Windows.Controls.ColumnDefinition):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.RowDefinitionCollection.IndexOf(System.Windows.Controls.RowDefinition):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.RowDefinitionCollection.Remove(System.Windows.Controls.RowDefinition):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.RowDefinitionCollection.Contains(System.Windows.Controls.RowDefinition):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.ScrollViewer.OnScrollChanged(System.Windows.Controls.ScrollChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.GetIsSharedSizeScope(System.Windows.UIElement):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.GetColumnSpan(System.Windows.UIElement):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.GetColumn(System.Windows.UIElement):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.SetRow(System.Windows.UIElement,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.SetRowSpan(System.Windows.UIElement,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.GetRowSpan(System.Windows.UIElement):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.SetColumnSpan(System.Windows.UIElement,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.SetColumn(System.Windows.UIElement,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.SetIsSharedSizeScope(System.Windows.UIElement,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Grid.GetRow(System.Windows.UIElement):System.Int32")]

//  BugID: 1010759
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.FrameworkElement.Resources")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.PropertyPath.PathParameters")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Application.Resources")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.FrameworkContentElement.Resources")]

//  BugID: 1061862
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.DockPanel.GetDock(System.Windows.UIElement):System.Windows.Controls.Dock")]

//  BugID: 1061856
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Annotations.AnnotationService.IsEnabled(System.Windows.Controls.DocumentViewer):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Annotations.AnnotationService.Disable(System.Windows.Controls.DocumentViewer):System.Void")]

//  BugID: Dictionary
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Shapes.Glyphs.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Shapes.Glyphs.FontRenderingEmSizeProperty")]

//  BugID: 1038628
//  Developer: SherifM
//  Reason: Approved by Actprog.
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.ScrollViewer.ViewportWidthProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.ScrollViewer.ViewportWidth")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.ScrollViewer.ViewportHeight")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.ScrollViewer.ViewportHeightProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Controls.Viewbox")]

//**************************************************************************************************************************
// Bug ID: 1011072
// Developer: Sherifm
// Reason: Approved by PMVT
// NEED dictionary modifications
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Shapes.Glyphs.BidiLevelProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Shapes.Glyphs.FontRenderingEmSizeProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Shapes.Glyphs.BidiLevel")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Shapes.Glyphs.FontRenderingEmSize")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Shapes.Polyline")]

//**************************************************************************************************************************
// Bug ID: 1010991
// Developer: Sherifm
// Reason: Approved by PMVT
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ThicknessKeyFrameCollection.Add(System.Windows.Media.Animation.ThicknessKeyFrame):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ThicknessKeyFrameCollection.Insert(System.Int32,System.Windows.Media.Animation.ThicknessKeyFrame):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.ThicknessKeyFrameCollection.Remove(System.Windows.Media.Animation.ThicknessKeyFrame):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038633
// Developer: NeilKr
// Reason: The event engine calls this and it is better to keep the derived for symmetry with the static helpers & other events that do use derived members.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Primitives.Slider.OnThumbDragCompleted(System.Windows.Controls.Primitives.DragCompletedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Primitives.Slider.OnThumbDragStarted(System.Windows.Controls.Primitives.DragStartedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038625, 1101263
// Developer: GHermann
// Reason: These collections need to be readwrite for parser reasons.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.Control.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.Text.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Documents.Paragraph.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Documents.Inline.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Documents.TextFlow.TextEffects")]

//**************************************************************************************************************************
// Bug ID: 1038626
// Developer: GregLett
// Reason: LineUp follows the pattern of the other "compound terms" on the same object like MouseUp, PageUp.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.StackPanel.System.Windows.Controls.Primitives.IScrollInfo.LineUp():System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.ScrollContentPresenter.LineUp():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038631
// Developer: NeilKr
// Reason: If property is read only we cannot use TemplateBind in markup.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.Primitives.TickBar.Ticks")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.Primitives.Slider.Ticks")]

//**************************************************************************************************************************
// Bug ID: 1011073
// Developer: Sherif
// Reason: See bug for more details.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Shapes.Polygon.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Shapes.Polyline.Points")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Shapes.Shape.StrokeDashArray")]

//**************************************************************************************************************************
// Bug ID: 1080447
// Developer: EVeselov
// Reason:  The usage is *not* intended to be a single word.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.MoveDownByLine")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.MoveLeftByWord")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.MoveRightByWord")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.MoveUpByLine")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.SelectDownByLine")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.SelectLeftByWord")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.SelectRightByWord")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Documents.EditingCommands.SelectUpByLine")]

//**************************************************************************************************************************
// Bug ID: 1080737, 1080179
// Developer: MCalkins
// Reason: Collection properties are allowed to be read/write in the Freezable model.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimationUsingKeyFrames.System.Windows.Media.Animation.IKeyFrameAnimation.KeyFrames")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessAnimationUsingKeyFrames.KeyFrames")]

//**************************************************************************************************************************
// Bug ID: 1061854
// Developer: SujalP
// Reason: This rule does not apply well in this instance. We are taking a string from URI, then
// modifying it, and then constructing a URI from that string again.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2234:PassSystemUriObjectsInsteadOfStrings", Scope="member", Target="System.Windows.Application.LoadComponent(System.Object,System.Uri):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1020624
// Developer: DRelyea
// Reason: Not fixing to maintain compat with the WinForms API
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.MessageBoxOptions.RtlReading")]

//**************************************************************************************************************************
// Bug ID: 1010891
// Developer: KenLai
// Reason: no strongly typed overload possible, these collections contain items of type Object.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1039:ListsAreStronglyTyped", Scope="type", Target="System.Windows.Data.ListCollectionView")]
[module: SuppressMessage("Microsoft.Design", "CA1039:ListsAreStronglyTyped", Scope="type", Target="System.Windows.Data.BindingListCollectionView")]

//**************************************************************************************************************************
// Bug ID: 1048317
// Developer: SamBent
// Reason: The items in CompositeCollection have type Object. There's no way to be more type-safe than we already are.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1039:ListsAreStronglyTyped", Scope="type", Target="System.Windows.Data.CompositeCollection")]

//**************************************************************************************************************************
// Bug ID: 1086742
// Developer: cedricd
// Reason: This structure goes straight into unmanaged code and cannot be changed.  I've verfied it is always 64bit aligned
//         so this rule appears bogus.  I have not had the time to debug the rule to verify why it flagged this as a problem
//         so I've assigned M11 bug 1132271 to myself.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Portability", "CA1900:ValueTypeFieldsShouldBePortable", Scope="type", Target="MS.Utility.TraceProvider+BaseEvent")]



//**************************************************************************************************************************
// Bug ID: 113119 & 1131908 & 1010853
// Developer: SamBent
// Reason: The items in CompositeCollection have type Object. There's no way to be more type-safe than we already are.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.GetChildValueHelper(System.Windows.UncommonField`1<System.Collections.Hashtable[]>,MS.Utility.ItemStructList`1<System.Windows.ChildValueLookup>&,System.Windows.DependencyObject,System.Windows.FrameworkElement,System.Windows.FrameworkContentElement,System.Int32,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.ImmComposition.OnWmImeNotify(System.IntPtr,System.IntPtr):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="MS.Internal.AppModel.RootBrowserWindow")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.Primitives.TextBoxBase+ActionContextMenuItem")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.Primitives.TextBoxBase+TextBoxContextMenu")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.Primitives.TextBoxBase+SuggestionContextMenuItem")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.RadioButton")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.CheckBox")]

//**************************************************************************************************************************
// Bug ID: 1131908
// Developer: SherifM
// Reason: As per actprog request.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.ImmComposition.OnWmImeNotify(System.IntPtr,System.IntPtr):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010739
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1027:MarkEnumsWithFlags", Scope="type", Target="System.Windows.MessageBoxImage")]

//**************************************************************************************************************************
// Bug ID: 1010743
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.Interop.HwndHost.MessageHook")]


//**************************************************************************************************************************
// Bug ID: 1010854
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.ScrollViewer.LineUp():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1010874
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.IScrollInfo.LineUp():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010866 1010862
// Developer: SherifM
// Reason: Approved by Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1039:ListsAreStronglyTyped", Scope="type", Target="System.Windows.Controls.ItemCollection")]

//**************************************************************************************************************************
// Bug ID: 1010754, 1169097
// Developer: a-shsolk
// Reason: Resolved as Won't Fix by bchapman; not closed
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="member", Target="e:System.Windows.Interop.HwndHost.#MessageHook")]

//**************************************************************************************************************************
// Bug ID: 1010863
// Developer: a-shsolk
// Reason: Resolved as Dead Code by actdrx (hgrant); closed by dbrown
//*************************************************************************************************************************** ]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type", Target = "System.Windows.Controls.HeaderedContentControl")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type", Target = "System.Windows.Controls.HeaderedItemsControl")]

//**************************************************************************************************************************
// Bug ID: 1038599
// Developer: a-shsolk
// Reason: Resolved as Won't Fix by younggk
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2216:DisposableTypesShouldDeclareFinalizer", Scope = "member", Target = "MS.Internal.Progressivity.ByteRangeDownloader.System.IDisposable.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038632
// Developer: a-shsolk
// Reason: Resolved as By Design by psarrett; closed by dbrown
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Controls.Primitives.TextBoxBase.LineUp():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1048310
// Developer: a-shsolk
// Reason: Resolved as By Design by actcomp (robertin); Closed by dbrown
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.LineUpCommand")]

//**************************************************************************************************************************
// Bug ID: 1048316
// Developer: a-shsolk
// Reason: 	Resolved as Duplicate of Bug 938461(wont fix) by actdata (sambent); Closed by dbrown
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Data.CollectionView")]

//**************************************************************************************************************************
// Bug ID: 1048318
// Developer: a-shsolk
// Reason: Resolved as Duplicate of Bug 938168(wont fix) by actdata (sambent); Closed by dbrown
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2109:ReviewVisibleEventHandlers", Scope = "member", Target = "System.Windows.Data.CollectionView.RaiseCurrentChanging(System.Object,System.ComponentModel.CancelEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1048320
// Developer: a-shsolk
// Reason: Resolved as By Design by garyyang; not closed
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Documents.FlowDocument.TextEffects")]

//**************************************************************************************************************************
// Bug ID: 1048321
// Developer: a-shsolk
// Reason: Resolved as By Design by benwest; Closed by mruiz
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextRange.Select(System.Windows.Documents.TextPointer,System.Windows.Documents.TextPointer):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextRange.Contains(System.Windows.Documents.TextPointer):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1061821
// Developer: a-shsolk
// Reason: Resolved as Won't Fix by brucemac
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.IO.Packaging.PackWebRequest.Headers")]

//**************************************************************************************************************************
// Bug ID: 1061825
// Developer: a-shsolk
// Reason: Resolved as By Design by johnlarc
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "System.IO.Packaging.PackWebResponse.System.IDisposable.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1061828, 1186028, 1186029
// Developer: a-shsolk
// Reason: Resolved as Duplicate of Bug 1010713(wont fix) by brucemac;
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2237:MarkISerializableTypesWithSerializable", Scope="type", Target="System.IO.Packaging.PackWebRequest")]
[module: SuppressMessage("Microsoft.Usage", "CA2237:MarkISerializableTypesWithSerializable", Scope="type", Target="System.IO.Packaging.PackWebResponse")]

//**************************************************************************************************************************
// Bug ID: 1080705
// Developer: a-shsolk
// Reason: Resolved as Duplicate of Bug 1080433 (wont fix) by kusumav; closed by sunpil
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2235:MarkAllNonSerializableFields", Scope = "member", Target = "MS.Internal.Utility.SponsorHelper._lease")]

//**************************************************************************************************************************
// Bug ID: 1081881
// Developer: a-shsolk
// Reason: Resolved as By Design by actdata (davidjen); violation on 'OneTime' which is intentionally a 2 word
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope = "member", Target = "System.Windows.Data.BindingMode.OneTime")]

//**************************************************************************************************************************
// Bug ID: 1061860
// Developer: garyyang
// Reason: MIL collections in Dependency properties can be read-write in Freezable model
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.TextBlock.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.TextBlock.TextEffects")]

//**************************************************************************************************************************
// Bug ID: 1010892
// Developer: DavidJen
// Reason: XmlDataProvider is built explicitly around the Xml DOM.
// It is safe to exclude a message from this rule if the declaring type requires the specific functionality provided by the concrete type.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1059:MembersShouldNotExposeCertainConcreteTypes", Scope = "member", Target = "System.Windows.Data.XmlDataProvider.Document")]

//**************************************************************************************************************************
// Bug ID: 1048323
// Developer: SherifM
// Reason: Actprog
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.DocumentPageView.DocumentPaginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Documents.IDocumentPaginator")]


//**************************************************************************************************************************
// Bug ID: 1010890 938461
// Developer: SherifM
// Reason: See bug.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Data.ListCollectionView")]
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Data.BindingListCollectionView")]


//**************************************************************************************************************************
// Bug ID: 1106758
// Developer: SherifM
// Reason:
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Developer: RRelyea
// Reason: suppress all warning level violations of the AvoidExcessiveComplexity
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.MonthCalendar.OnPreviewKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.DatePicker.KeyDownHandler(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.TreeView.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.TreeViewItem.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.PtsHost.MbpInfo.FromElement(System.Windows.DependencyObject):MS.Internal.PtsHost.MbpInfo")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.Find(MS.Internal.Documents.FindToolBar):System.Windows.Documents.ITextRange")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.Glyphs.ParseGlyphsProperty(System.Windows.Media.GlyphTypeface,System.String,System.Boolean,System.Collections.Generic.List`1<System.Windows.Documents.Glyphs+ParsedGlyphData>&,System.UInt16[]&):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.FixedSOMPageConstructor.ProcessPath(System.Windows.Shapes.Path):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.FixedSOMPageConstructor._DetectTables():System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.BamlRecordReader.GetElementAndFlags(System.Int16,System.Object&,System.Windows.Markup.ReaderFlags&,System.Type&,System.Int16&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlTypeMapper.GetClrInfoForClass(System.Boolean,System.Type,System.String,System.String,System.String,System.Boolean,System.String&):System.Reflection.MemberInfo")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.XamlReaderHelper.CompileElement(System.String,System.String,System.Int32,System.Type,System.String,System.Boolean,System.Boolean&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.TreeWalkHelper.OnResourcesChanged(System.Windows.DependencyObject,System.Windows.ResourcesChangeInfo):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.FindResourceInternal(System.Windows.IResourceHost,System.Object,System.Object,System.Object,System.Object&,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.GetChildValueHelper(System.Windows.UncommonField`1<System.Collections.Specialized.HybridDictionary[]>,MS.Utility.ItemStructList`1<System.Windows.ChildValueLookup>&,System.Windows.DependencyObject,System.Windows.FrameworkElement,System.Windows.FrameworkContentElement,System.Int32,System.Boolean,System.Windows.ValueLookupType&):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.KeyboardNavigation.GetPrevTab(System.Windows.DependencyObject,System.Windows.DependencyObject,System.Boolean):System.Windows.DependencyObject")]
// Reason: suppress ok violations of the AvoidExcessiveComplexity
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Markup.KnownTypes.GetKnownTypeConverterIdForProperty(MS.Internal.Markup.KnownElements,System.String):MS.Internal.Markup.KnownElements")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Markup.KnownTypes.GetKnownTypeConverterId(MS.Internal.Markup.KnownElements):MS.Internal.Markup.KnownElements")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.KnownTypes.CreateKnownElement(System.Windows.Markup.KnownElements):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.KnownTypes.GetKnownTypeConverterId(System.Windows.Markup.KnownElements):System.Windows.Markup.KnownElements")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.KnownTypes.GetKnownTypeConverterIdForProperty(System.Windows.Markup.KnownElements,System.String):System.Windows.Markup.KnownElements")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Input.CommandValueSerializer.GetKnownCommand(System.String,System.Type):System.Windows.Input.UICommand")]

//**************************************************************************************************************************
// Developer: RRelyea
// Reason: ok to exclude...internal classes only...
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="MS.Internal.Documents.FindToolBar")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="MS.Internal.Documents.ZoomComboBox")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Documents.TextEditorContextMenu+EditorContextMenu")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Documents.TextEditorContextMenu+EditorMenuItem")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Documents.TextEditorContextMenu+ReconversionMenuItem")]

//**************************************************************************************************************************
// Bug ID: 1196960, 1196961
// Developer: RRelyea
// Reason: lots of false hits...including this case.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableTemplate.FindName(System.String,System.Windows.FrameworkContentElement):System.Windows.DependencyObject")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.FrameworkTemplate.FindName(System.String,System.Windows.FrameworkElement):System.Windows.DependencyObject")]


//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Annotations.Component.AdornerPresentationContext..ctor(System.Windows.Documents.AdornerLayer,MS.Internal.Annotations.Component.AnnotationAdorner)")]
#endif

//**************************************************************************************************************************
// Bug ID: 10154709
// Developer: PeterOst
// Reason: This is auto-generated code that creates large switch statements for known avalon types for making
//         parsing more efficient.
// Status: needs to be approved.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Markup.KnownTypes.GetKnownTypeConverterIdForProperty(MS.Internal.Markup.KnownElements,System.String):MS.Internal.Markup.KnownElements")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Markup.KnownTypes.GetKnownTypeConverterId(MS.Internal.Markup.KnownElements):MS.Internal.Markup.KnownElements")]

//**************************************************************************************************************************
// Bug ID: 1070228
// Developer: WayneZeng
// Reason: This is by design in Tablet Avalon API
// Status: PMVT is following up with Tablet to make sure this is necessary
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Controls.InkCanvas.Strokes")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Controls.InkPresenter.Strokes")]

//**************************************************************************************************************************
// Bug ID: 1010627
// Developer: SherifM
// Reason: Pending ActProg approval.
// Status: being addressed globally
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.PathParser.Parse(System.String):MS.Internal.Data.SourceValueInfo[]")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.ReplaceItem(System.Int32,System.Object,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.GetInfo(System.Int32,System.Object,MS.Internal.Data.PropertyPathWorker+SourceValueState&):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.DefaultValueConverter.Create(System.Type,System.Type,System.Boolean):System.Windows.Data.IValueConverter")]

//**************************************************************************************************************************
// Bug ID: 1213054
// Developer: SamBent
// Reason: Pending ActProg approval.  (Dupe of 1010627.  Return type changed.)
// Status: being addressed globally
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.DefaultValueConverter.Create(System.Type,System.Type,System.Boolean):System.ComponentModel.IValueConverter")]

//**************************************************************************************************************************
// Bug ID: 1182084
// Developer: SamBent
// Reason: Method contains a large switch statement.
// Status: being addressed globally
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.RelativeObjectRef.GetDataObjectImpl(System.Windows.DependencyObject,System.Boolean):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1010731
// Developer: BenCar
// Reason: Approved by PMVT, these methods contains large switch statements.
// Status: being addressed globally
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemParameters.InvalidateCache():System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemParameters.InvalidateCache(System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemColors.SlotToFlag(System.Windows.SystemColors+CacheSlot):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.SystemResourceKey.get_Resource():System.Object")]

//**************************************************************************************************************************
// Bug ID: 1079500
// Developer: EVeselov
// Reason: This method is specifically designed to only take UIElements for layout/rendering reasons;
// Status: PMVT discussing global surpression
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TextPointer.InsertUIElement(System.Windows.UIElement):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1080162
// Developer: EVeselov
// Reason: This method is specifically designed to only take TextPointer;
// Status: PMVT discussing global surpression
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TextRange.SelectWord(System.Windows.Documents.TextPointer):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1049128
// Developer: EVeselov
// Reason: These methods are specifically designed to only take UIElement or ContentElement;
// Status: PMVT discussing global surpression
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextPointer.InsertEmbeddedElement(System.Windows.UIElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextPointer.InsertEmbeddedElement(System.Windows.ContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextRange.Append(System.Windows.UIElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.TextRange.Append(System.Windows.ContentElement):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1038639
// Developer: GHermann
// Reason: Collections are strongly typed and should remain so by design.
// Status: PMVT discussing global surpression
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowCollection.Insert(System.Int32,System.Windows.Documents.TableRow):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowCollection.Remove(System.Windows.Documents.TableRow):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowCollection.Add(System.Windows.Documents.TableRow):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableCellCollection.Remove(System.Windows.Documents.TableCell):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableCellCollection.Insert(System.Int32,System.Windows.Documents.TableCell):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableCellCollection.Add(System.Windows.Documents.TableCell):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1129829
// Developer: psarrett
// Reason: This class will be removed in M11.
// Status: this will stay in proposed until Peter removes LollipopAdorner
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Documents.LollipopAdorner.GetLollipopDrawingData(System.Windows.Point&,System.Windows.Point&,System.Windows.Point&):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Documents.LollipopAdorner.GetLollipopDrawingData(System.Windows.Point&,System.Windows.Point&,System.Windows.Point&):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1045:DoNotPassTypesByReference", Scope="member", Target="System.Windows.Documents.LollipopAdorner.GetLollipopDrawingData(System.Windows.Point&,System.Windows.Point&,System.Windows.Point&):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010154 1010155, 1319524, 1319527
// Reason: fxcop rule needs fix...
// Status: need fix in m11, but will still need the exclusion...
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.ContentPresenter..ctor()")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Primitives.RepeatButton..ctor()")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Primitives.Thumb..ctor()")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Primitives.PopupRoot..ctor()")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Documents.CaretElement..ctor(System.Windows.Documents.ITextView, bool)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Input.KeyboardNavigation+FocusVisualAdorner..ctor(System.Windows.UIElement,System.Windows.Style)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.GridSplitter+PreviewAdorner..ctor(System.Windows.Controls.GridSplitter,System.Windows.Style)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Internal.Controls.TemplatedAdorner..ctor(System.Windows.UIElement,System.Windows.Controls.ControlTemplate)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="MS.Internal.Annotations.Component.AnnotationAdorner..ctor(MS.Internal.Annotations.Component.IAnnotationComponent,System.Windows.UIElement)")]

//**************************************************************************************************************************
// Bug ID: 1182071
// Developer: SamBent
// Reason: This name is analogous to approved name ICollectionView (bugs 938461, 1048300).
// Status: needs approval
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Data.IGroupableCollectionView")]

//**************************************************************************************************************************
// Bug ID: 1182066
// Developer: rruiz
// Reason: The AnnotationService framework is being limited to content in DocumentViewerBase and its subclasses.  Though we
//         could theoretically accept any DependencyObject we do not have the resources for that wider support.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Annotations.AnnotationService.GetService(System.Windows.Controls.Primitives.DocumentViewerBase):System.Windows.Annotations.AnnotationService")]


//**************************************************************************************************************************
// Bug ID: 1182061
// Developer: shibai
// Reason: This method is specifically designed to only take the specified EventArgs
// Status: Approved
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.MonthCalendar.OnFirstVisibleMonthChanged(System.Windows.RoutedPropertyChangedEventArgs`1<System.DateTime>):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.MonthCalendar.OnDateSelectionChanged(System.Windows.Controls.DateSelectionChangedEventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1182063
// Developer: shibai
// Reason: This method is specifically designed to only take the specified EventArgs
// Status: Approved
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.DatePicker.OnValueChanged(System.Windows.RoutedPropertyChangedEventArgs`1<System.Nullable`1<System.DateTime>>):System.Void")]

//**************************************************************************************************************************
// Developer: garyyang
// Reason: The property name Uid follows the x:Uid name in XAML markup. They need to be consisent as they refer to the same thing.
//         The localization dictionary is indexed by a BamlLocalizableResourceKey. It is a data store that identify a localizable resource.
// Status: needs approval
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1043:UseIntegralOrStringArgumentForIndexers", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizationDictionary.Item[System.Windows.Markup.Localizer.BamlLocalizableResourceKey]")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizableResourceKey.Uid")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizableResourceKey..ctor(System.String,System.String,System.String)")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.XamlParseException.UidContext", MessageId="Uid")]

//  BugID: 1182065
//  Developer: GregLett / DmitryT
//  Reason: Canvas will only use this property on UIElements, not DP's.  The method type gives developers some warning of that.
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.SetBottom(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetBottom(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.SetLeft(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetLeft(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.SetRight(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetRight(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.SetTop(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetTop(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetBottom(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetTop(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetLeft(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Canvas.GetRight(System.Windows.UIElement):System.Double")]



//**************************************************************************************************************************
// Bug ID: 1189994, 1206646, 1321044, 1411074, 1418974
// Developer: RogerCh, Cedricd, GillesK
// Reason: This is by design for this API.  We could take a base class but from a programming model perspective
// we explicitly don't want to.  We only want to allow use of Storyboards on FrameworkElements and FrameworkContentElements
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Begin(System.Windows.FrameworkElement,System.Windows.Media.Animation.HandoffBehavior,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Seek(System.Windows.FrameworkElement,System.TimeSpan,System.Windows.Media.Animation.TimeSeekOrigin):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Pause(System.Windows.FrameworkElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SkipToFill(System.Windows.FrameworkContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SkipToFill(System.Windows.FrameworkElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Resume(System.Windows.FrameworkElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SetSpeedRatio(System.Windows.FrameworkContentElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Stop(System.Windows.FrameworkContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Seek(System.Windows.FrameworkContentElement,System.TimeSpan,System.Windows.Media.Animation.TimeSeekOrigin):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Resume(System.Windows.FrameworkContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SetSpeedRatio(System.Windows.FrameworkElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Pause(System.Windows.FrameworkContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Stop(System.Windows.FrameworkElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Begin(System.Windows.FrameworkContentElement,System.Windows.Media.Animation.HandoffBehavior,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SeekAlignedToLastTick(System.Windows.FrameworkContentElement,System.TimeSpan,System.Windows.Media.Animation.TimeSeekOrigin):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.SeekAlignedToLastTick(System.Windows.FrameworkElement,System.TimeSpan,System.Windows.Media.Animation.TimeSeekOrigin):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentIteration(System.Windows.FrameworkContentElement):System.Nullable`1<System.Int32>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentGlobalSpeed(System.Windows.FrameworkElement):System.Nullable`1<System.Double>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentIteration(System.Windows.FrameworkElement):System.Nullable`1<System.Int32>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentState(System.Windows.FrameworkContentElement):System.Windows.Media.Animation.ClockState")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentTime(System.Windows.FrameworkElement):System.Nullable`1<System.TimeSpan>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentProgress(System.Windows.FrameworkElement):System.Nullable`1<System.Double>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentProgress(System.Windows.FrameworkContentElement):System.Nullable`1<System.Double>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentTime(System.Windows.FrameworkContentElement):System.Nullable`1<System.TimeSpan>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentGlobalSpeed(System.Windows.FrameworkContentElement):System.Nullable`1<System.Double>")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetCurrentState(System.Windows.FrameworkElement):System.Windows.Media.Animation.ClockState")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Begin(System.Windows.FrameworkElement,System.Windows.FrameworkTemplate,System.Windows.Media.Animation.HandoffBehavior,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetIsPaused(System.Windows.FrameworkElement):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.GetIsPaused(System.Windows.FrameworkContentElement):System.Boolean")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Remove(System.Windows.FrameworkContentElement):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Media.Animation.Storyboard.Remove(System.Windows.FrameworkElement):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1182067
// Developer: ghermann
// Reason: Inline is conceptually the correct element name and has passed winfx api review.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", Scope="type", Target="System.Windows.Documents.DocumentStructures.Inline")]

//**************************************************************************************************************************
// Bug ID: 1182070
// Developer: ghermann
// Reason: While a base type is possible to use, it doesn't make sense from an API standpoint.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowGroupCollection.Add(System.Windows.Documents.TableRowGroup):System.Int32")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowGroupCollection.Insert(System.Int32,System.Windows.Documents.TableRowGroup):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.TableRowGroupCollection.Remove(System.Windows.Documents.TableRowGroup):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1206642,1184021
// Developer: bencar
// Reason: Spelling of these identifiers is correct.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Controls.Resizer")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Controls.VirtualizingStackPanel")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.GetIsVirtualizing(System.Windows.DependencyObject):System.Boolean")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.SetIsVirtualizing(System.Windows.DependencyObject,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.IsVirtualizingProperty")]

//**************************************************************************************************************************
// Bug ID: 1188356,1218567
// Developer: bencar
// Reason: These collection properties are DPs. Text properties were already approved in other elements.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.AccessText.TextDecorations")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.AccessText.TextEffects")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.Slider.Ticks")]

//**************************************************************************************************************************
// Bug ID: 1208876,1184023,1206647,1218564
// Developer: bencar
// Reason: Virtuals exposing events.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Control.OnMouseDoubleClick(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Control.OnPreviewMouseDoubleClick(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Slider.OnThumbDragCompleted(System.Windows.Controls.Primitives.DragCompletedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Slider.OnThumbDragStarted(System.Windows.Controls.Primitives.DragStartedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.TreeView.OnSelectedItemChanged(System.Windows.RoutedPropertyChangedEventArgs`1<System.Object>):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.OnCleanUpVirtualizedItem(System.Windows.Controls.CleanUpVirtualizedItemEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1196959
// Developer: darind
// Reason: After discussing this issue with khaleds & jeesmc, changing the name would result in a breaking change.  A DCR will be created to fix this in Beta2
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1716:IdentifiersShouldNotMatchKeywords", Scope = "type", Target = "System.Windows.Documents.DocumentStructures.Break")]

//**************************************************************************************************************************
// Bug ID: 1182056
// Developer: CedricD
// Reason: This rule incorrectly flagged this code.  It's an interface and has no finalizer.
//***************************************************************************************************************************\
[module: SuppressMessage("Microsoft.Usage", "CA2221:FinalizersShouldBeProtected", Scope="member", Target="MS.Internal.Utility.IAssemblyName.Finalize():System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1182193
// Developer: mikehill
// Reason: This is internal to reduce API surface area (test load).  It's a callback from the XmlCompatibilityReader
// to allow extensibility, to allow someone else to decide if an Xml namespace is supported.  This allows a perf optimization
// because in the XamlParser we already have all the Mapper information to answer the question.
//***************************************************************************************************************************\
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt0002:FriendsOkAttributeFound", Scope="type", Target="System.Windows.Markup.IsXmlNamespaceSupportedCallback")]

//**************************************************************************************************************************
// Bug ID: 1167229
// Developer: jdmack
// Reason: These are equivilents of the native the Win32 message API parameters, and as such should preserve the Win32 spelling
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Interop.HwndHost.WndProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr,System.Boolean&):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1222526
// Reason: Already follows PMVT pattern for accessing DPs in the ctor:
//         http://avalon/pmvt/Design%20Guidelines/DoNotCallOverridableMethodsInAConstructor.htm
// Status: ...however, code will be removed when bug #1219113 is addressed.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Controls.Viewport3D..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1169099
// Developer: hamidm
// Reason: We don't return a copy of the array, thus we have no perf implications. We don't care if the array is tampered with.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Scope="member", Target="System.Windows.StartupEventArgs.Args")]

//**************************************************************************************************************************
// Bug ID: 1212727
// Developer: dwaynen
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Markup.Mapper.GetClrInfoForClass(System.Boolean,System.Type,System.String,System.String,System.String,System.Boolean,System.String&):System.Reflection.MemberInfo")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.FindResourceInternal(System.Windows.IResourceHost,System.Object,System.Object,System.Object,System.Object&,System.Boolean):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1188354
// Developer: robertan
// Reason: This was by design and our API requires Panels over just standard FrameworkElements
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.AddPanel(System.Windows.Controls.Panel,System.String):System.Void")]

//**************************************************************************************************************************
// Developer: junwg
// Reason: Derive ListView from ListBox is approved in at architect meeting.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope="type", Target="System.Windows.Controls.ListView")]

//**************************************************************************************************************************
// Developer: junwg
// Reason: This property act as the target of binding in theme. So setter is necessary.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope = "member", Target = "System.Windows.Controls.Primitives.GridViewRowPresenterBase.Columns")]
//**************************************************************************************************************************
// Developer: brianad
// Reason: Serializers is a proper industry term to refer serialization classes
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerProvider.InstalledSerializers", MessageId="Serializers")]

//**************************************************************************************************************************
// Developer: brianad
// Reason: Paginator is a proper industry term to refer to a class that does pagination
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.Write(System.Windows.Documents.IDocumentPaginator,System.Printing.Configuration.PrintTicket):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.WriteAsync(System.Windows.Documents.IDocumentPaginator,System.Printing.Configuration.PrintTicket,System.Object):System.Void")]

//**************************************************************************************************************************
// Developer: brianad
// Reason: GAC is the correct abrviation for the Global Assembly Cache
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1705:LongAcronymsShouldBePascalCased", Scope="member", Target="System.Windows.Documents.Serialization.SerializerDescriptor.IsInGAC", MessageId="Member")]

//**************************************************************************************************************************
// Bug ID: 1235318
// Developer: rruiz
// Reason: This is the word created by the Flow team for objects that know how to create pages from content.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Annotations.AnnotationDocumentPaginator")]

//**************************************************************************************************************************
// Bug ID: 1235316
// Developer: BenCar
// Reason: This is the approved name.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Controls.VirtualizingPanel")]

//**************************************************************************************************************************
// Bug ID: 1233101
// Developer: dwaynen
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.OnPropertyChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.FindResourceInternal(System.Windows.IResourceHost,System.Object,System.Object,System.Windows.DependencyObject,System.Object&,System.Boolean,System.Windows.DependencyProperty,System.Boolean,System.Boolean):System.Object")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkElement.GetRawValue(System.Windows.DependencyProperty,System.Object,System.Windows.PropertyMetadata,System.Windows.BaseValueSourceInternal&):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1233102
// Developer: dwaynen
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.StyleHelper.GetChildValueHelper(System.Windows.UncommonField`1<System.Collections.Specialized.HybridDictionary[]>,MS.Utility.ItemStructList`1<System.Windows.ChildValueLookup>&,System.Windows.DependencyProperty,System.Windows.DependencyObject,System.Windows.FrameworkElement,System.Windows.FrameworkContentElement,System.Int32,System.Boolean,System.Windows.ValueLookupType&,System.Windows.FrameworkElementFactory):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1233103
// Developer: dwaynen
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.FrameworkContentElement.GetRawValue(System.Windows.DependencyProperty,System.Object,System.Windows.PropertyMetadata,System.Windows.BaseValueSourceInternal&):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1244807
// Developer: dwaynen
// Reason: no good solution
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.FrameworkContentElement..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1244799
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="Microsoft.Win32.CommonDialog.HookProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1244799
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="Microsoft.Win32.CommonDialog.HookProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]
//**************************************************************************************************************************
// Bug ID: 1244799
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="Microsoft.Win32.CommonDialog.HookProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]
//**************************************************************************************************************************
// Bug ID: 1244799
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="Microsoft.Win32.CommonDialog.HookProc(System.IntPtr,System.Int32,System.IntPtr,System.IntPtr):System.IntPtr")]

//**************************************************************************************************************************
// Bug ID: 1244800
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="Microsoft.Win32.OpenFileDialog.Multiselect")]

//**************************************************************************************************************************
// Bug ID: 1244803
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="Microsoft.Win32.FileDialog.OnFileOk(System.ComponentModel.CancelEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1244803
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="Microsoft.Win32.FileDialog.FileOk")]

//**************************************************************************************************************************
// Bug ID: 1244804
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Scope="member", Target="Microsoft.Win32.FileDialog.SafeFileNames")]

//**************************************************************************************************************************
// Bug ID: 1244804
// Developer: avappdev
// Reason: Consistent With WinForm
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Performance", "CA1819:PropertiesShouldNotReturnArrays", Scope="member", Target="Microsoft.Win32.FileDialog.FileNames")]

//**************************************************************************************************************************
// Bug ID: 1249905
// Developer: benwest
// Reason: Method is virtual and overrides need derived type.
//**************************************************************************************************************************

[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase.OnTextChanged(System.Windows.Controls.TextChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1233098
// Developer: dwaynen
// Reason: The decision was made to make DependencyPropertyChangedEventArgs a struct for perf reasons.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.FrameworkElement.DataContextChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="e:System.Windows.FrameworkContentElement.DataContextChanged")]

//**************************************************************************************************************************
// Bug ID: 1234539
// Developer: dwaynen
// Reason: No good alternative
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Navigation.NavigationWindow..ctor(System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope="member", Target="System.Windows.Navigation.NavigationWindow..ctor()")]

//**************************************************************************************************************************
// Bug ID: 1302462
// Developer: kenlai
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="MS.Internal.Data.CompositeCollectionView.ProcessCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: robertan
// Reason: The term Paginator exists in several other places within the APIs.  This method is just following the naming
//         of the data type.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "System.Windows.Controls.PrintDialog.PrintDocument(System.Windows.Documents.IDocumentPaginator,System.String):System.Void")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: shibai
// Reason: complex code happens
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.Find(MS.Internal.Documents.FindToolBar):System.Windows.Documents.ITextRange")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Controls.FlowDocumentScrollViewer.OnKeyDown(System.Windows.Input.KeyEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: junwg
// Reason: This member is event, but the [NonSerialized] attibute can only be applied to fields
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2235:MarkAllNonSerializableFields", Scope = "member", Target = "System.Windows.Controls.GridViewColumnCollection._columnCollectionChanged")]

//**************************************************************************************************************************
// Bug ID: 1292620
// Developer: waynezen
// Reason: Those DPs have been aliased to the corresponding Canvas' DPs. Canvas only accepts UIElement as its parameter.
// Status: Approved
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.GetTop(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.GetRight(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.SetLeft(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.SetBottom(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.SetTop(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.SetRight(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.GetLeft(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.InkCanvas.GetBottom(System.Windows.UIElement):System.Double")]

//**************************************************************************************************************************
// Bug ID: 1319511
// Developer: waynezen
// Reason: Those protected methods are specifically designed to only take the specified argument types.
// Status: Approved
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnStrokeCollected(System.Windows.Controls.InkCanvasStrokeCollectedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnGesture(System.Windows.Controls.InkCanvasGestureEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313623
// Developer: grzegorz
// Reason: Paginator name has been already approved.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.PrintDialog.PrintDocument(System.Windows.Documents.DocumentPaginator,System.String):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313623
// Developer: grzegorz
// Reason: Paginator name has been already approved.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.WriteAsync(System.Windows.Documents.DocumentPaginator,System.Printing.PrintTicket,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.Write(System.Windows.Documents.DocumentPaginator,System.Printing.PrintTicket):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313636, 1313632, 1313645, 1319518, 1313640
// Developer: benwest
// Reason: These bugs flag switch statements that are not, in fact, complex.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.XamlToRtfWriter.WriteInlineChild(System.Windows.Documents.DocumentNode):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.XamlParserHelper.IsAnsi(System.Char):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.FormatState.IsEqual(System.Windows.Documents.FormatState):System.Boolean")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.FontTableEntry.CharSetToCodePage(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.RtfToXamlReader.HandleControl(System.Windows.Documents.Token,System.Windows.Documents.ControlWordInfo):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.RtfToXamlReader.HandleTableProperties(System.Windows.Documents.Token,System.Windows.Documents.FormatState):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.RtfToXamlReader.HandleListTokens(System.Windows.Documents.Token,System.Windows.Documents.FormatState):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "System.Windows.Documents.RtfToXamlReader.HandleOldListTokens(System.Windows.Documents.Token,System.Windows.Documents.FormatState):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1313640
// Developer: sangilj
// Reason: This method is not actually complex -- refactoring it will only reduce its legibility.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.RtfToXamlReader.ProcessField():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1325741
// Developer: jdersch
// Reason: ZoomComboBox is internal (no public dev confusion) and there is no way we can eliminate base classes since we
//         do not own them (and it's probably a bit late in the dev cycle to refactor Avalon).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1501:AvoidExcessiveInheritance", Scope = "type", Target = "MS.Internal.Documents.Application.ZoomComboBox")]

//**************************************************************************************************************************
// Bug ID: 1325737, 1325738, 1325739, 1325740
// Developer: sangilj
// Reason: These bugs flag switch statements that are not, in fact, complex.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.XamlIn.HandleAttributes(System.Windows.Documents.ConverterState,System.Windows.Documents.IXamlAttributes,System.Windows.Documents.DocumentNode,System.Windows.Documents.XamlTag,System.Windows.Documents.DocumentNodeArray):System.Windows.Documents.XamlToRtfError")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.XamlParserHelper.AppendRtfChar(System.Text.StringBuilder,System.Char,System.Int32,System.Text.Encoding&,System.Byte[],System.Char[]):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.RtfToXamlReader.SetFontTableEntryCodepage(System.Windows.Documents.Token,System.Windows.Documents.FontTableEntry):System.Void")]
[module: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope="member", Target="System.Windows.Documents.Converters.MarkerRomanCountToString(System.Text.StringBuilder,System.Windows.Documents.MarkerStyle,System.Int64):System.String")]

//**************************************************************************************************************************
// Bug ID: 1340750, 1340751, 1340752, 1340753, 1340754, 1340755, 1340756, 1340757, 1340758
// Developer: arathira
// Reason: These Dispose methods call base.Dispose but do not place the call in a finally block because they do not throw
// exceptions and cleanup is not necessary.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.AttachedObject.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.OptimalBreakSession.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.FlowDocumentPage.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.ContainerParagraph.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.FigureParaClient.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.Section.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.FloaterParaClient.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.LineBreakpoint.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope = "member", Target = "MS.Internal.PtsHost.LineBreakRecord.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1340749
// Developer: dmitryt
// Reason: The GridLines are meant to be the Lines separating the cells of some Grid. This is not the single word but valid combination of two words.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.Grid.ShowGridLinesProperty", MessageId="GridLines")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.Controls.Grid.ShowGridLines", MessageId="GridLines")]

//**************************************************************************************************************************
// Bug ID: 1340761, 1340762, 1340763
// Developer: hamidm
// Reason: These are all breaking changes and AppModel is not doing taking any breaking changes now
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.MessageBoxResult.OK", MessageId="Member")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.MessageBoxButton.OKCancel", MessageId="Member")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.MessageBoxButton.OK", MessageId="Member")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", Scope="member", Target="System.Windows.ReasonSessionEnding.Logoff", MessageId="Logoff")]

//**************************************************************************************************************************
// Bug ID: 1337365 1337366
// Developer: robertan
// Reason: We need to expose the Xml DOM for the print ticket we are currently using so ISVs can monitor and manipulate the
//         print ticket so that we can pick the changes up dynamically.  Other classes only allow read-only access to this
//         data.  This is used for our extensibility model in the dialog.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1059:MembersShouldNotExposeCertainConcreteTypes", Scope = "member", Target = "System.Windows.Controls.PrintDialog.PrintTicketXmlDocument")]
[module: SuppressMessage("Microsoft.Design", "CA1059:MembersShouldNotExposeCertainConcreteTypes", Scope = "member", Target = "System.Windows.Controls.PrintDialogBase.PrintTicketXmlDocument")]

//***************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: this is a bug in FxCop.  EventArgs *should* be named 'e'
//***************************************************************************************************************************
//1340704
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.Thumb.OnDraggingChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId="0#e")]
//1340706
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.ButtonBase.OnIsPressedChanged(System.Windows.DependencyPropertyChangedEventArgs):System.Void", MessageId="0#e")]

//***************************************************************************************************************************
// Bug ID: various
// Developer: kenlai
// Reason: "Multi" is the proper prefix for these
//***************************************************************************************************************************
//1340712
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Data.MultiBinding", MessageId="Multi")]
//1340713
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Data.PriorityBindingExpression.ParentMultiBinding", MessageId="Multi")]
//1340714
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Data.BindingOperations.GetMultiBindingExpression(System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Windows.Data.MultiBindingExpression", MessageId="Multi")]
//1388178
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Data.BindingOperations.GetMultiBinding(System.Windows.DependencyObject,System.Windows.DependencyProperty):System.Windows.Data.MultiBinding", MessageId="Multi")]
//1340715
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Data.MultiBindingExpressionBase", MessageId="Multi")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Data.MultiBindingExpressionBase.ParentMultiBinding", MessageId="Multi")]
//1340716
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Data.IMultiValueConverter", MessageId="Multi")]
//1340719
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Data.MultiBindingBase", MessageId="Multi")]
//1340720
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.Data.MultiBindingExpression", MessageId="Multi")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Data.MultiBindingExpression.ParentMultiBinding", MessageId="Multi")]
//1340722
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.MultiDataTrigger", MessageId="Multi")]
//1340723
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="type", Target="System.Windows.MultiTrigger", MessageId="Multi")]

//**************************************************************************************************************************
// Bug ID: 1340760
// Developer: kenlai
// Reason: OK is OK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope="member", Target="System.Windows.Data.CollectionView.OKToChangeCurrent():System.Boolean", MessageId="Member")]

//**************************************************************************************************************************
// Bug ID: 1344477, 1344478
// Developer: grzegorz
// Reason: These Dispose methods call base.Dispose but do not place the call in a finally block because they do not throw
// exceptions and cleanup is not necessary.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope="member", Target="MS.Internal.PtsHost.TableParagraph.Dispose():System.Void")]
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope="member", Target="MS.Internal.PtsHost.RowParagraph.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1340765
// Developer: benwest
// Reason: The property name Uid follows the x:Uid name in XAML markup. They need to be consisent as they refer to the same thing.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Uid")]

//**************************************************************************************************************************
// Bug ID: 1340765
// Developer: benwest
// Reason: The text in this exception strings maps to an existing interface name and must be consistent.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Paginators")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="Multi")]

//*************************************************************************************************************************
// Bug ID: 1225797
// Developer: dwaynen
//*************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1032:ImplementStandardExceptionConstructors", Scope="type", Target="System.Windows.ResourceReferenceKeyNotFoundException")]

//*************************************************************************************************************************
// Bug ID: 1313624, 1313625
// Developer: grzegorz
//*************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.Windows.Annotations.Storage.AnnotationStore)", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.IO.Stream)", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.Primitives.DocumentPageView.DocumentPaginator", MessageId="Paginator")]

//**************************************************************************************************************************
// Bug ID: 1293966
// Developer: eleese
// Reason: These functions were recently changed to use UIElement for consistency with the equivalent functions in Canvas.
// There is no reason to use these on anything that isn't a UIElement, as FixedPage doesn't support non-UIElement children
// and won't handle hyperlink events from anything that isn't a UIElement.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.SetRight(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.GetRight(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.GetLeft(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.SetNavigateUri(System.Windows.UIElement,System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.SetTop(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.SetBottom(System.Windows.UIElement,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.GetNavigateUri(System.Windows.UIElement):System.Uri")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.GetTop(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.GetBottom(System.Windows.UIElement):System.Double")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Documents.FixedPage.SetLeft(System.Windows.UIElement,System.Double):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1370518
// Developer: grzegorz
// Reason: These Dispose methods call base.Dispose but do not place the call in a finally block because they do not throw
// exceptions and cleanup is not necessary.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope="member", Target="MS.Internal.PtsHost.InlineObject.Dispose():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1010918
// Developer: garyyang
// Reason: Suppressing by request of garyyang
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Documents.TextElement.TextEffects")]

//**************************************************************************************************************************
// Bug ID: 1366654
// Developer: olego
// Reason: SetDock does not make sense for UIElement's base types as DockPanel accepts only UIElements and derived types as its children.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.DockPanel.SetDock(System.Windows.UIElement,System.Windows.Controls.Dock):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1380340
// Developer: AtanasK
// Reason: This method is specifically designed to only take SelectionChangedEventArgs params;
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.Primitives.Selector.OnSelectionChanged(System.Windows.Controls.SelectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1397872
// Developer: benwest
// Reason: ITextPointer is an internal interface -- we cannot expose it on a public API.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.RichTextBox.GetNextSpellingErrorPosition(System.Windows.Documents.TextPointer,System.Windows.Documents.LogicalDirection):System.Windows.Documents.TextPointer")]

//**************************************************************************************************************************
// Bug ID: 1397873
// Developer: benwest
// Reason: This collection property is a DP. Setter already approved in other elements.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Controls.TextBox.TextDecorations")]


//**************************************************************************************************************************
// Bug ID: 1414744
// Developer: benwest
// Reason: "Prereform" and "postreform" are terms used to describe spelling rules.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.SpellingReform.PreAndPostreform", MessageId="Postreform")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.SpellingReform.Prereform", MessageId="Prereform")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Controls.SpellingReform.Postreform", MessageId="Postreform")]


//**************************************************************************************************************************
// Bug ID: 1414745
// Developer: benwest
// Reason: TextBoxBase is the most general object these attached properties may be set on, at runtime.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.SpellCheck.SetSpellingReform(System.Windows.Controls.Primitives.TextBoxBase,System.Windows.Controls.SpellingReform):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.SpellCheck.SetIsEnabled(System.Windows.Controls.Primitives.TextBoxBase,System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1431116
// Developer: RobertWl/AlikK
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="nonparent")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="nonanimatable")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="multistep")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope="resource", Target="ExceptionStringTable.resources", MessageId="nonabstract")]

//**************************************************************************************************************************
// Bug ID: 1453718
// Developer: Dmitryt - we need this uncalled code for logistic reasons of x-integrations in avalon branch for FebCTP release.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Automation.Peers.PasswordBoxAutomationPeer.RaiseValuePropertyChangedEvent(System.String,System.String):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.Automation.Peers.PasswordBoxAutomationPeer.RaiseIsReadOnlyPropertyChangedEvent(System.Boolean,System.Boolean):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1456666, 1456667
// Developer: Dmitryt - in this context, listview is a valid word, it's not lis, not view, not ListView - if it was a Button,
// the parameter name could have been "button". it's not a hungarian notation which the rule seems to be targeted for
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Peers.GridViewAutomationPeer..ctor(System.Windows.Controls.GridView,System.Windows.Controls.ListView)", MessageId="1#listview")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Automation.Peers.GridViewItemAutomationPeer..ctor(System.Object,System.Windows.Automation.Peers.ListViewAutomationPeer)", MessageId="1#listview")]

//**************************************************************************************************************************
// Bug ID: 1463917
// Developer: olego - Panel accepts only UIElement and derived classes as children. Thus Panel.SetZIndex / Panel.GetZIndex
// do not make sense for UIElement's base types.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Panel.SetZIndex(System.Windows.UIElement,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Controls.Panel.GetZIndex(System.Windows.UIElement):System.Int32")]

//**************************************************************************************************************************
// Bug ID: 1497997
// Developer: ghermann - Class dispose already calls base class dispose. The issue here is wrapping other methods in finally block -
// this is unnecessary as dispose should not throw, and the call chain only references internal methods which have been verified.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2215:DisposeMethodsShouldCallBaseClassDispose", Scope="member", Target="MS.Internal.PtsHost.UIElementParagraph.Dispose():System.Void")]

//  BugID: 1507387
//  Developer: RRuiz
//  Reason: The annotation service feature is limited to a small set of controls.  Even though these methods only use DependencyObject
//          API, changing the API to take a DepObj would mislead the developer.
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Annotations.AnnotationService.GetService(System.Windows.Controls.FlowDocumentScrollViewer):System.Windows.Annotations.AnnotationService")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.Annotations.AnnotationService.GetService(System.Windows.Controls.FlowDocumentReader):System.Windows.Annotations.AnnotationService")]

//**************************************************************************************************************************
// Bug ID: 1543037, 1543040, 1543041, 1543042, 1543043, 1543045, 1600752, 1600754
// BamlRecord has a BitVector32 field (_flags), that is shared by subclasses to save working set.  Sharing flags like this
// is easier in e.g. FrameworkElement, where the class hierarchy is linear, but can be bug-prone otherwise.  To make the
// code less fragile, each class abstractly provides it's last section to subclasses.  Since not all subclasses
// use it, it leads to uncalled code.  As such, though, it doesn't hurt us, and having this design avoids difficult to
// debug bugs.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlAttributeInfoRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlDefAttributeKeyStringRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlDefAttributeKeyTypeRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlPIMappingRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlStringInfoRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlTypeInfoRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlAssemblyInfoRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlPropertyCustomRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.Markup.BamlPropertyWithExtensionRecord.get_LastFlagsSection():System.Collections.Specialized.BitVector32+Section")]

//**************************************************************************************************************************
// Bug ID: 1561169
// Developer: garyyang - Uid is a wellknown name in localization, for example, you can specify x:Uid in XAML.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizerError.UidMissingOnChildElement")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizerError.DuplicateUid")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Markup.Localizer.BamlLocalizerError.InvalidUid")]

//**************************************************************************************************************************
// Bug ID: 1575805
// Developer: rruiz - Paginator has already been approved.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.Windows.Annotations.Storage.AnnotationStore,System.Windows.FlowDirection)", MessageId="0#Paginator")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="System.Windows.Annotations.AnnotationDocumentPaginator..ctor(System.Windows.Documents.DocumentPaginator,System.IO.Stream,System.Windows.FlowDirection)", MessageId="0#Paginator")]

//**************************************************************************************************************************
// Bug ID: 1580911, 1580912
// Developer: tjhsiang - Making all .Resources properties Read-Write, for the sake of consistency
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.FrameworkTemplate.Resources")]
[module: SuppressMessage("Microsoft.Usage", "CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="System.Windows.Style.Resources")]

//**************************************************************************************************************************
// Bug ID: 1583231, 1583235
// Developer: RukeH
// Reason: The naming of these identifiers are conformed to the standard COM interface IPersistFile, which was redfined in our code.
//          We should keep the names consistent to what is released and documented to prevent confusion.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.Load(System.String,System.Int32):System.Void", MessageId = "0#psz")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope="member", Target="MS.Internal.Interop.IPersistFile.Save(System.String,System.Boolean):System.Void", MessageId="1#f")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.Save(System.String,System.Boolean):System.Void", MessageId = "0#psz")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.GetCurFile(System.String&):System.Int32", MessageId = "0#ppsz")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.SaveCompleted(System.String):System.Void", MessageId = "0#psz")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.GetClassID(System.Guid&):System.Void", MessageId = "0#")]
[module: SuppressMessage("Microsoft.Naming", "CA1706:ShortAcronymsShouldBeUppercase", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.GetClassID(System.Guid&):System.Void", MessageId = "Member")]
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "member", Target = "MS.Internal.Interop.IPersistFile.GetClassID(System.Guid&):System.Void", MessageId = "0#p")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: kurtb
// Reason: We want to keep adorner layer lookup limited to the 2D tree
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Documents.AdornerLayer.GetAdornerLayer(System.Windows.Media.Visual):System.Windows.Documents.AdornerLayer")]

//**************************************************************************************************************************
// Bug ID: no bug
// Developer: sambent
// Reason: These refer to the StringFormat property, not to the String type
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.ContentPresenter.#OnContentStringFormatChanged(System.String,System.String)", MessageId="string")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.GridViewColumn.#OnHeaderStringFormatChanged(System.String,System.String)", MessageId="string")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.HeaderedContentControl.#OnHeaderStringFormatChanged(System.String,System.String)", MessageId="string")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.HeaderedItemsControl.#OnHeaderStringFormatChanged(System.String,System.String)", MessageId="string")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.ItemsControl.#OnItemStringFormatChanged(System.String,System.String)", MessageId="string")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope="member", Target="System.Windows.Controls.ContentControl.#OnContentStringFormatChanged(System.String,System.String)", MessageId="string")]

//**************************************************************************************************************************
// Bug ID: 202865 (DevDiv)
// Developer: AtanasK
// Reason: This name was approved at the MultiSelector spec review. Also the work 'Multi' exists in the custom dictionary
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly", Scope = "type", Target = "System.Windows.Controls.Primitives.MultiSelector")]

//**************************************************************************************************************************
// Bug ID: 632977
// Developer: sambent
// Reason:  We're creating an exception due to a condition detected elsewhere (asynchronously).
//          The argument name refers to the point of detection, not to the point of creating the exception.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Data.ClrBindingWorker.#OnGetValueCallback(MS.Internal.Data.AsyncDataRequest)")]
[module: SuppressMessage("Microsoft.Usage","CA2208:InstantiateArgumentExceptionsCorrectly", Scope="member", Target="MS.Internal.Data.PropertyPathWorker.#RawValue(System.Int32)")]

//**************************************************************************************************************************
// Bug ID: 632977
// Developer: sambent
// Reason:  Inheriting names from caller.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="o", Scope="member", Target="System.Windows.Data.ListCollectionView.#Compare(System.Object,System.Object)")]


//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: Value and Type property names are flagged by fxcop as violations 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Controls.Primitives.RangeBase.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Controls.Primitives.Track.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.DataTrigger.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.FrameworkElementFactory.#Type")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.TemplatePartAttribute.#Type")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Trigger.#Value")]
[module: SuppressMessage("Microsoft.Naming", "CA1721:PropertyNamesShouldNotMatchGetMethods", Scope = "member", Target = "System.Windows.Media.Animation.ThicknessKeyFrame.#Value")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: DependencyPropertyChangedEventArgs is not derived from EventArgs. This cannot be changed now
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.FrameworkContentElement.#DataContextChanged")]
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope = "member", Target = "System.Windows.FrameworkElement.#DataContextChanged")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: parameter name targetObject
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "object", Scope = "member", Target = "System.Windows.Setter.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "object", Scope = "member", Target = "System.Windows.Setter.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", MessageId = "object", Scope = "member", Target = "System.Windows.Trigger.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: GridLines is flagged because Gridline is a dictionary word
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#GridLinesVisibility")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#GridLinesVisibilityProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#HorizontalGridLinesBrush")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#HorizontalGridLinesBrushProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#VerticalGridLinesBrush")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "member", Target = "System.Windows.Controls.DataGrid.#VerticalGridLinesBrushProperty")]
[module: SuppressMessage("Microsoft.Naming", "CA1702:CompoundWordsShouldBeCasedCorrectly", MessageId = "GridLines", Scope = "type", Target = "System.Windows.Controls.DataGridGridLinesVisibility")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: ArgumentException thrown inside a property setter should have the paramName as the property name. 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Controls.PrintDialog.#set_PageRange(System.Windows.Controls.PageRange)")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: We don't want to change the parameter to base type 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.SpellCheck.#GetIsEnabled(System.Windows.Controls.Primitives.TextBoxBase)")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.VisualStateManager.#GetCustomVisualStateManager(System.Windows.FrameworkElement)")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.VisualStateManager.#SetCustomVisualStateManager(System.Windows.FrameworkElement,System.Windows.VisualStateManager)")]
[module: SuppressMessage("Microsoft.Design", "CA1011:ConsiderPassingBaseTypesAsParameters", Scope = "member", Target = "System.Windows.Controls.DataGridRow.#GetRowContainingElement(System.Windows.FrameworkElement)")]
//**************************************************************************************************************************
// Bug ID: 632973 (DevDiv)
// Developer: kedecond
// Reason: The type name should be plural e.g. DataGridHeadersVisibilities. We can't change this now.  
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1714:FlagsEnumsShouldHavePluralNames", Scope = "type", Target = "System.Windows.Controls.DataGridHeadersVisibility")]

//**************************************************************************************************************************
// Bug ID: 632978 (DevDiv TFS Dev10)
// Developer: bartde
// Reason: Those would be breaking changes to fix.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design", "CA1009:DeclareEventHandlersCorrectly", Scope="member", Target="System.Windows.Interop.HwndHost.#MessageHook")]
[module: SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="member", Target="System.Windows.Interop.HwndHost.#MessageHook")]

//**************************************************************************************************************************
// Bug ID: 632972 (DevDiv TFS Dev10)
// Developer: jezhan
// Reason: The names are meaningful
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Baml2006.KeyRecord.#.ctor(System.Boolean,System.Boolean,System.Int32,System.String)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Baml2006.Baml2006SchemaContext.#GetTextFromBinary(System.Byte[],System.Int16,System.Xaml.XamlMemberBase,System.Xaml.XamlType)")]

//**************************************************************************************************************************
// Bug ID: 632972 (DevDiv TFS Dev10)
// Developer: jezhan
// Reason: The arguments are meaningful
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Baml2006.DependencyAccessorInfo.#GetDependencyObject(System.Object[],System.Int32)")]
[module: SuppressMessage("Microsoft.Usage", "CA2208:InstantiateArgumentExceptionsCorrectly", Scope = "member", Target = "System.Windows.Baml2006.RoutedAdderInfo.#GetDependencyObject(System.Object[],System.Int32)")]


//**************************************************************************************************************************
// Bug ID: 632972 (DevDiv TFS Dev10)
// Developer: jezhan
// Reason: This would break public API
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.XamlWriter.#Save(System.Object,System.IO.TextWriter)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.XamlWriter.#Save(System.Object,System.IO.Stream)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.XamlWriter.#Save(System.Object,System.Xml.XmlWriter)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.XamlWriter.#Save(System.Object,System.Windows.Markup.XamlDesignerSerializationManager)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.XamlWriter.#Save(System.Object)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Markup.Localizer.BamlLocalizabilityResolver.#GetPropertyLocalizability(System.String,System.String,System.String)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Condition.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.Condition.#ReceiveTypeConverter(System.Object,System.Windows.Markup.XamlSetTypeConverterEventArgs)")]
[module: SuppressMessage("Microsoft.Naming", "CA1720:IdentifiersShouldNotContainTypeNames", Scope = "member", Target = "System.Windows.DataTrigger.#ReceiveMarkupExtension(System.Object,System.Windows.Markup.XamlSetMarkupExtensionEventArgs)")]

//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: JoeCast
// Reason: This is an intentional deviation to be consistent with WPF patterns of mutable collections.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA2227:CollectionPropertiesShouldBeReadOnly", Scope="member", Target="Microsoft.Win32.FileDialog.#CustomPlaces")]

//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: kedecond
// Reason: This method is present so that Push method (which is public in parent class) can never be called on derived class
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Windows.TemplateContent+StackOfFrames.#Push(System.Windows.TemplateContent+Frame)")]
//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: kedecond
// Reason: CellAutomationValueHolder is a sealed class, so that virtual methods cannot be overridden. So it is OK to call these in the contructor.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors", Scope = "member", Target = "System.Windows.Controls.DataGrid+CellAutomationValueHolder.#.ctor(System.Object,System.Windows.Controls.DataGridColumn)")]
//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: kedecond
// Reason: hotfix is flagged by fxcop as spelling error
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId = "hotfix", Scope = "resource", Target = "ExceptionStringTable.resources")]
//**************************************************************************************************************************
// Bug ID: 692896 (DevDiv TFS Dev10)
// Developer: kedecond
// Reason: These are helper methods that call another method to raise events They donot raise any events themselves.
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope = "member", Target = "System.Windows.VisualStateManager.#RaiseCurrentStateChanged(System.Windows.VisualStateGroup,System.Windows.VisualState,System.Windows.VisualState,System.Windows.FrameworkElement,System.Windows.FrameworkElement)")]
[module: SuppressMessage("Microsoft.Design", "CA1030:UseEventsWhereAppropriate", Scope = "member", Target = "System.Windows.VisualStateManager.#RaiseCurrentStateChanging(System.Windows.VisualStateGroup,System.Windows.VisualState,System.Windows.VisualState,System.Windows.FrameworkElement,System.Windows.FrameworkElement)")]
//**************************************************************************************************************************
// Bug ID: none
// Developer: ifeanyie
// Reason: This would break public API
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Paginator", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.#Write(System.Windows.Documents.DocumentPaginator)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Paginator", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.#WriteAsync(System.Windows.Documents.DocumentPaginator)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Paginator", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.#WriteAsync(System.Windows.Documents.DocumentPaginator,System.Object)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Paginator", Scope="member", Target="System.Windows.Documents.Serialization.SerializerWriter.#WriteAsync(System.Windows.Documents.DocumentPaginator,System.Printing.PrintTicket)")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.TableStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.TableRowStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.TableRowGroupStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.TableCellStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.StoryFragments")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.StoryFragment")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.SectionStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.ParagraphStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.ListStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.ListItemStructure")]
[module: SuppressMessage("Microsoft.Naming","CA1710:IdentifiersShouldHaveCorrectSuffix", Scope="type", Target="System.Windows.Documents.DocumentStructures.FigureStructure")]

[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="fwlink", Scope="resource", Target="ExceptionStringTable.resources")]


//**************************************************************************************************************************
// Bug ID: none
// Developer: pantal
// Reason: These items would break existing public API's, we can't change them.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndHost.#TranslateCharCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndHost.#TranslateAcceleratorCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="0#", Scope="member", Target="System.Windows.Interop.HwndHost.#OnMnemonicCore(System.Windows.Interop.MSG&,System.Windows.Input.ModifierKeys)")]

[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="e", Scope="member", Target="System.Windows.Controls.Primitives.Thumb.#OnDraggingChanged(System.Windows.DependencyPropertyChangedEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="e", Scope="member", Target="System.Windows.Controls.Primitives.ButtonBase.#OnIsPressedChanged(System.Windows.DependencyPropertyChangedEventArgs)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Bidi", Scope="member", Target="System.Windows.Documents.Glyphs.#BidiLevelProperty")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Bidi", Scope="member", Target="System.Windows.Documents.Glyphs.#BidiLevel")]


//**************************************************************************************************************************
// Bug ID: none
// Developer: pantal
// Reason: Won't fix legacy items, clearing up FxCop scans.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="unsecure", Scope="resource", Target="ExceptionStringTable.resources")]
[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="assemblyname", Scope="resource", Target="ExceptionStringTable.resources")]
[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="Unsecure", Scope="resource", Target="ExceptionStringTable.resources")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.AppModel.XappLauncherApp", Scope="member", Target="System.Windows.Interop.DocObjHost.#MS.Internal.AppModel.IBrowserHostServices.Run(System.String,System.String,MS.Internal.AppModel.MimeType,System.String,System.String,System.Object,System.Object,System.Boolean,System.Boolean,System.Boolean,System.Boolean,MS.Internal.AppModel.INativeProgressPage,System.String,System.String,System.String,System.String,MS.Internal.AppModel.IHostBrowser)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Internal.AppModel.XappLauncherApp", Scope="member", Target="System.Windows.Interop.DocObjHost.#MS.Internal.AppModel.IBrowserHostServices.Run(System.String,System.String,MS.Internal.AppModel.MimeType,System.String,System.String,System.Object,System.Object,MS.Internal.AppModel.HostingFlags,MS.Internal.AppModel.INativeProgressPage,System.String,System.String,System.String,System.String,MS.Internal.AppModel.IHostBrowser)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.UnsafeNativeMethods.ExtractIconEx(System.String,System.Int32,MS.Win32.NativeMethods+IconHandle@,MS.Win32.NativeMethods+IconHandle@,System.Int32)", Scope="member", Target="MS.Internal.AppModel.IconHelper.#GetDefaultIconHandles(MS.Win32.NativeMethods+IconHandle&,MS.Win32.NativeMethods+IconHandle&)")]
[module: SuppressMessage("Microsoft.Usage","CA1806:DoNotIgnoreMethodResults", MessageId="MS.Win32.NativeMethods+IOleCommandTarget.Exec(MS.Win32.NativeMethods+GUID,System.Int32,System.Int32,System.Object[],System.Int32)", Scope="member", Target="System.Windows.Controls.WebBrowser.#DoNavigate(System.Uri,System.Object&,System.Object&,System.Object&,System.Boolean)")]
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="System.Windows.Window+HwndStyleManager.#System.IDisposable.Dispose()")]



//**************************************************************************************************************************
// Bug ID: 744160
// Developer: brandf
// Reason: In Justification field
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1011:ConsiderPassingBaseTypesAsParameters", Scope="member", Target="System.Windows.VisualStateManager.#GetVisualStateGroups(System.Windows.FrameworkElement)", Justification="VisualStateManager is a FrameworkElement concept, just because we only call DO methods here, doesn't mean we should expose a DO.")]

//**************************************************************************************************************************
// Bug ID: none
// Developer: pantal
// Reason: PTS Cache is explicitly managing lifetime here.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage", "CA1816:CallGCSuppressFinalizeCorrectly", Scope = "member", Target = "MS.Internal.PtsHost.PtsCache.#CreatePTSContext(System.Int32,System.Windows.Media.TextFormattingMode)")]


//**************************************************************************************************************************
// Bug ID: none
// Developer: pantal
// Reason: Hwnd is a valid name. 
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId="Hwnd", Scope="resource", Target="ExceptionStringTable.resources")]


//**************************************************************************************************************************
// New since v4 RTM:
//**************************************************************************************************************************

//**************************************************************************************************************************
// By design per SamBent.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1720:IdentifiersShouldNotContainTypeNames", MessageId="object", Scope="member", Target="System.Windows.Data.BindingOperations.#EnableCollectionSynchronization(System.Collections.IEnumerable,System.Object)")]

//**************************************************************************************************************************
// Bug ID: none
// Developer: juhanit
// Reason: GC.KeepAlive is needed here. SafeHandle cannot be used for delegates.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Interop.MouseHookManager.#HookThreadProc()")]


//**************************************************************************************************************************
// Release is being used as a Dispose method, so this is by design.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Usage","CA1816:CallGCSuppressFinalizeCorrectly", Scope="member", Target="Standard.MessageWindow.#Release()")]

//**************************************************************************************************************************
// Bug ID: none
// Developer: ifeanyie
// Reason: Virtualizable is accepted terminology
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Virtualizable", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.#IsContainerVirtualizableProperty")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Virtualizable", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.#GetIsContainerVirtualizable(System.Windows.DependencyObject)")]
[module: SuppressMessage("Microsoft.Naming","CA1704:IdentifiersShouldBeSpelledCorrectly", MessageId="Virtualizable", Scope="member", Target="System.Windows.Controls.VirtualizingPanel.#SetIsContainerVirtualizable(System.Windows.DependencyObject,System.Boolean)")]


//**************************************************************************************************************************
// Bug ID: 204636
// Developer: varsham
// Reason: The value is transformed from a deferred value to an actual one. 
// Hence is it appropriate to pass it through by reference.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Design","CA1045:DoNotPassTypesByReference", MessageId="1#", Scope="member", Target="System.Windows.ResourceDictionary.#OnGettingValue(System.Object,System.Object&,System.Boolean&)")]
