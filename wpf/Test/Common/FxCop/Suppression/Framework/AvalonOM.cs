//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1183034 & 1183035
// Developer: rruiz
// Reason: Both these methods do nothing but call the event.  In the case of AnnotationResourceCollection.OnPropertyChanged
//         the method is only called from an event listener itself.  In the case of ObservableDictionary.FireDictionaryChanged
//         is called at the end of methods that deal with state to prevent any corruption of state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.Annotations.AnnotationResourceCollection.OnPropertyChanged(System.Object,System.ComponentModel.PropertyChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.Annotations.ObservableDictionary.FireDictionaryChanged():System.Void")]


//**************************************************************************************************************************
// Bug ID: 1182889
// Developer: shibai
// Reason: I have reviewed entry points into that method and have ensured that if an exception were to be thrown, the control would not be in an inconsistent state.
// Status: Approved
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.DatePicker.OnInvalidEntryError(System.Windows.Controls.InvalidEntryErrorEventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1187569 & 1187570
// Developer: SamBent
// Reason: All local state is correct before raising these events.  Exceptions won't cause any harm.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.Data.CollectionViewGroupRoot.OnGroupByChanged():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Data.GroupDescription.OnPropertyChanged(System.String):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195537
// Developer: shibai
// Reason: I have reviewed entry points into that method and have ensured that if an exception were to be thrown, the control would not be in an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Expander.OnCollapsed():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Expander.OnExpanded():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195529
// Developer: shibai
// Reason: I have reviewed entry points into that method and have ensured that if an exception were to be thrown, the control would not be in an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.DatePicker.OnValueChanged(System.Windows.RoutedPropertyChangedEventArgs`1<System.Nullable`1<System.DateTime>>):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195518
// Developer: shibai
// Reason: I have reviewed entry points into that method and have ensured that if an exception were to be thrown, the control would not be in an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar.OnFirstVisibleMonthChanged(System.Windows.RoutedPropertyChangedEventArgs`1<System.DateTime>):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar.OnDateSelectionChanged(System.Windows.Controls.DateSelectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195542
// Developer: shibai
// Reason: I have reviewed entry points into that method and have ensured that if an exception were to be thrown, the control would not be in an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.InsertDate(System.Int32,System.DateTime):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.Clear():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.Remove(System.DateTime):System.Boolean")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.RemoveAt(System.Int32):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195525
// Developer: waynezen
// Reason: According to the call graphs http://tabletpc/longhorn/Specs/exception%20hardening.vsd
//         We raise the event at the bottom of the methods. No state needs to be flagged.
//         Or the code has been flagged properly.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionResizing(System.Windows.Controls.InkCanvasSelectionEditingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnEditingModeInvertedChanging(System.Windows.Controls.InkCanvasEditingModeChangingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnStrokesReplaced(System.Windows.Controls.InkCanvasStrokesReplacedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnDefaultDrawingAttributesReplaced(System.Windows.Ink.DrawingAttributesReplacedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnStrokeErasing(System.Windows.Controls.InkCanvasStrokeErasingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionResized(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionChanged(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionChanging(System.Windows.Controls.InkCanvasSelectionChangingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnStrokeCollected(System.Windows.Controls.InkCanvasStrokeCollectedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnEditingModeChanged(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnGesture(System.Windows.Controls.InkCanvasGestureEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnEditingModeInvertedChanged(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnStrokeErased(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionMoving(System.Windows.Controls.InkCanvasSelectionEditingEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnSelectionMoved(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.InkCanvas.OnEditingModeChanging(System.Windows.Controls.InkCanvasEditingModeChangingEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195560
// Developer: bencar
// Reason: This method and all its callers do not change state following the event being called. The event is the last thing that happens in the dispatcher queue item.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.VirtualizingStackPanel.OnCleanUpVirtualizedItem(System.Windows.Controls.CleanUpVirtualizedItemEventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1195503, 1195535, 1187123, 1195596, 1195547, 1195588, 1195521, 1195594
// Developer: benwest
// Reason: All of the flagged code and callers to the component boundary have been reviewed and hardened to
//         recoverable exceptions.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="<CrtImplementationDetails>.ModuleUninitializer.SingletonDomainUnload(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.PasswordTextContainer.AddChange(System.Windows.Documents.ITextPointer,System.Int32,System.Windows.Documents.PrecursorTextChange):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextContainer.BeforeAddChange():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextRange.System.Windows.Documents.ITextRange.FireChanged():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.PasswordBox.OnContentTextChanged(System.Object,System.Windows.Documents.TextContainerChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.Primitives.TextBoxBase.OnSelectionChanged(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.RichTextBox.OnMouseWheel(System.Windows.Input.MouseWheelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextEditor.OnTextContainerChanged(System.Object,System.Windows.Documents.TextContainerChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextEditorCopyPaste._CreateDataObject(System.Windows.Documents.TextEditor,System.Boolean):System.Windows.IDataObject")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextEditorCopyPaste._DoPaste(System.Windows.Documents.TextEditor,System.Windows.IDataObject,System.Boolean):System.Boolean")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.TextEditorCopyPaste.ConfirmDataFormatSetting(System.Windows.FrameworkElement,System.Windows.IDataObject,System.String):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1195573
// Developer: neilkr
// Reason: The RaiseEvent call is the last statement of all calling functions.  No state changes occur after calls to this function.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.Primitives.ScrollBar.RaiseScrollEvent(System.Windows.Controls.Primitives.ScrollEventType):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1206793 and 1182179
// Developer: jeremyns
// Reason: Both of these methods have been reviewed, and need access to objects defined within PresentationUI.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt0001:FriendShouldCallOnlyApprovedInternals", Scope="member", Target="MS.Internal.Documents.DocumentApplication.OnAddJournalEntry(System.Object,MS.Internal.Documents.Application.DocumentApplicationJournalEntryEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt0001:FriendShouldCallOnlyApprovedInternals", Scope="member", Target="MS.Internal.Documents.DocumentApplication..ctor()")]

//**************************************************************************************************************************
// Bug IDs: 1195516, 1195531, 1195534, 1195548, 1195552, 1195554, 1195557, 1195575, 1195578, 1195580, 1195581, 1195586, 1195590, 1195658
// Developer: AtanasK
// Reason: Methods have been reviewed as a part of task 28478 and if an exception is thrown we make sure the control would not be in an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.ToolTip.OnOpened(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.ToolTip.OnClosed(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Frame.OnContentRendered(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.InvokeClickAfterRender(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnChecked(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnClick():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnIsSelectedInvalidated(System.Windows.DependencyObject):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnSubmenuClosed(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnSubmenuOpened(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MenuItem.OnUnchecked(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Button.UpdateIsDefaulted(System.Windows.IInputElement):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.TabItem.HandleIsSelectedChanged(System.Boolean,System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.ListBoxItem.HandleIsSelectedChanged(System.Boolean,System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.ContextMenu.OnClosed(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.ContextMenu.OnOpened(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Selector.OnSelectionChanged(System.Windows.Controls.SelectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.ToggleButton.OnChecked(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.ToggleButton.OnIndeterminate(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.ToggleButton.OnUnchecked(System.Windows.RoutedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Thumb.CancelDrag():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Thumb.OnMouseLeftButtonDown(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Thumb.OnMouseLeftButtonUp(System.Windows.Input.MouseButtonEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Thumb.OnMouseMove(System.Windows.Input.MouseEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Popup.CalculatePositionRelative(System.Windows.Controls.Primitives.PlacementMode,System.Windows.UIElement,System.Windows.Media.Matrix):System.Boolean")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.Popup.FirePopupCouldClose():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.ButtonBase.OnClick():System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.Primitives.RangeBase.OnValueChanged(System.Double,System.Double):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Input.KeyboardNavigation.NotifyFocusChanged(System.Object,System.Windows.Input.KeyboardFocusChangedEventArgs):System.Void")]



//**************************************************************************************************************************
// Bug ID: 1195566, 1195567, 1195568, 1195569, 1195513, 1194316
// Developer: rruiz
// Reason: All of these methods have been reviewed - they all fire the events after all state has been updated and none of them
//         do any catching.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Annotation.FireAuthorEvent(System.Object,System.Windows.Annotations.AnnotationAction):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Annotation.FireResourceEvent(System.Windows.Annotations.AnnotationResource,System.Windows.Annotations.AnnotationAction,System.Windows.Annotations.AnnotationResourceChangedEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.AnnotationResource.FireResourceChanged(System.String):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.LocatorBase.FireLocatorChanged(System.String):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.LocatorPart.OnPropertyChanged(System.Object,System.ComponentModel.PropertyChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Storage.AnnotationStore.OnAnchorChanged(System.Windows.Annotations.AnnotationResourceChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Storage.AnnotationStore.OnAuthorChanged(System.Windows.Annotations.AnnotationAuthorChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Storage.AnnotationStore.OnCargoChanged(System.Windows.Annotations.AnnotationResourceChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Annotations.Storage.AnnotationStore.OnStoreContentChanged(System.Windows.Annotations.Storage.StoreContentAction,System.Windows.Annotations.Annotation):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1206784
// Developer: shibai
// Reason: This event is only used internally. I have reviewed the codes, there is not an inconsistent state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.OnCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedAction,System.Object,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.MonthCalendar+DateCollection.OnCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedAction):System.Void")]

//**************************************************************************************************************************
// Bug ID: 121992,1219923
// Developer: bencar
// Reason: Confirmed that no state is changed after calling the event.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.CalendarDate.OnPropertyChanged(System.String):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.CalendarMonthGroup.OnPropertyChanged(System.String):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1198044
// Developer: darind
// Reason: Presently we are using internal methods and we will change our code to utilize public API’s when they are available in Beta 2
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt0001:FriendShouldCallOnlyApprovedInternals", Scope = "member", Target = "System.Windows.Documents.FixedDocument.CreateStructureUri(System.Uri):System.Uri")]

//**************************************************************************************************************************
// Bug ID: 1218617
// Developer: darind
// Reason: Confirmed that no state is changed after calling the event.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Documents.FixedDocument.OnInitialized(System.Object,System.EventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195591, 1195595, 1195601, 1195603, 1195604, 1195605
// Developer: eleese
// Reason: No public substitute for BitmapVisualManager, and I have reviewed all cases below to make sure 
//         state doesn't change after calling an event.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.PageContentAsyncResult.Dispatch(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPagesChanged(System.Object,System.Windows.Documents.PagesChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence.GetPageForContentPositionAsync(System.Windows.Documents.ContentPosition,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._NotifyGetPageAsyncCompleted(System.Windows.Documents.DocumentPage,System.Int32,System.Exception,System.Boolean,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPaginationCompleted(System.Object,System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocumentSequence._OnChildPaginationProgress(System.Object,System.Windows.Documents.PaginationProgressEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocument.GetPageForContentPositionAsync(System.Windows.Documents.ContentPosition,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocument._NotifyGetPageAsyncCompleted(System.Windows.Documents.DocumentPage,System.Int32,System.Exception,System.Boolean,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FixedDocument.OnPageContentAppended(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.PageContent._NotifyPageCompleted(System.Windows.Documents.FixedPage,System.Exception,System.Boolean,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.DocumentReferenceCollection.OnCollectionChanged(System.Collections.Specialized.NotifyCollectionChangedAction,System.Object,System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.DocumentSequenceTextContainer.AddChange(System.Windows.Documents.ITextPointer,System.Int32,System.Windows.Documents.PrecursorTextChange):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1188375
// Developer: robertan
// Reason: We raise the event at the bottom of the methods.  Exceptions will not leave the control in an unknown state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.PrintDialog.OnPrintTicketChanged(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.PrintDialog.OnPrinterChanged(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.PrintDialog.NotifyPropertyChange(System.String):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.PrintDialog.OnPrintTicketChanging(System.Windows.Controls.PrintTicketChangingEventArgs):System.Void")]

//**************************************************************************************************************************
// Developer: junwg
// Reason: This method does nothing but call the event. 
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.GridViewColumn.OnPropertyChanged(System.String):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.GridViewColumnCollection.OnCollectionChanged(System.Windows.Controls.GridViewColumnCollectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195511
// Developer: grzegorz
// Reason: Internal event. All methods raising this event have been verified. Exceptions will not leave the control in an unknown state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.Documents.TextViewBase.OnUpdated(System.EventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195583
// Developer: grzegorz
// Reason: All methods raising this event have been verified. Exceptions will not leave the control in an unknown state.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.Primitives.DocumentViewerBase.OnPageViewsChanged():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195610
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.Document.GetPageAsync(System.Int32,System.Object):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195610
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.Document.GetPageForContentPositionAsync(System.Windows.Documents.ContentPosition,System.Object):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195592
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.DocumentPageView.OnPageConnected(System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1195592
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.DocumentPageView.OnPageDisconnected():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195608
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FlowDocument.OnPagePaddingChanged(System.Windows.Thickness):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195608
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FlowDocument.OnPageSizeChanged(System.Windows.Size):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195608
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FlowDocument.OnPagesChanged(System.Int32,System.Int32):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195608
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FlowDocument.OnPaginationProgressAsync(System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: 1195608
// Developer: grzegorz
// Reason: Remains in a consistent state after a recoverable Exception is thrown.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Documents.FlowDocument.OnPaginationCompletedAsync(System.Object):System.Object")]

//**************************************************************************************************************************
// Bug ID: N/A
// Developer: shibai
// Reason: All of the flagged code and callers to the component boundary have been reviewed and hardened to
//         recoverable exceptions.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "System.Windows.Controls.FlowDocumentReader+MySinglePageViewer.FireMasterPageNumberChangedEvent():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195512
// Developer: jdersch
// Reason: First, the events being invoked are internal (and are therefore only used by internal code).  Second,
//         the method is already hardened against recoverable exceptions as the invocation happens at the end of the method
//         and no state can be lost.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "MS.Internal.Documents.PageCache.GetPageCompletedDelegate(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "MS.Internal.Documents.PageCache.PaginationProgressDelegate(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "MS.Internal.Documents.PageCache.PaginationCompletedDelegate(System.Object):System.Object")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope = "member", Target = "MS.Internal.Documents.PageCache.PagesChangedDelegate(System.Object):System.Object")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

