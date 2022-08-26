//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//RukmaniG
//Bug# 1195508
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.AppModel.OleCmdHelper.UpdateMappingTable(System.Collections.Hashtable):System.Void")]

//RukmaniG
//Bug# 1195509
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="MS.Internal.AppModel.ApplicationProxyInternal.Stop():System.Boolean")]

//RukmaniG
//Bug #1195641
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnDeactivate(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnSessionEnding(System.Windows.SessionEndingCancelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnStartup(System.Windows.StartupEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnExit(System.Windows.ExitEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnActivate(System.EventArgs):System.Void")]

//RukmaniG
//Bug #1195648
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireNavigationProgress(System.Uri):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireLoadCompleted(System.Boolean,System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireNavigated(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireNavigationStopped(System.Object):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireFragmentNavigation(System.String):System.Boolean")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationService.FireNavigating(System.Uri,System.Object,System.Object,System.Net.WebRequest):System.Boolean")]

//**************************************************************************************************************************
// Bug ID: 1195649
// Developer: huwang
// Reason: All those events are internal. There is no internal state depending on the success of the event.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationWindow.BackForwardStateChangeHandler(System.Object,System.EventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1195650
// Developer: huwang
// Reason: All those events are internal.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.LimitedJournalEntryStackEnumerable.PropogateCollectionChanged(System.Object,System.Collections.Specialized.NotifyCollectionChangedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195651
// Developer: huwang
// Reason: All those events are internal.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.Journal.UpdateView():System.Void")]


//RukmaniG
//Bug #1195652
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnNavigationProgress(System.Windows.Navigation.NavigationProgressEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnLoadCompleted(System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnFragmentNavigation(System.Windows.Navigation.FragmentNavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnNavigating(System.Windows.Navigation.NavigatingCancelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnNavigated(System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationApplication.OnNavigationStopped(System.Windows.Navigation.NavigationEventArgs):System.Void")]

//RukmaniG
//Bug #1195654
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.PageFunction`1.RaiseTypedReturnEvent(System.Windows.Navigation.PageFunctionBase,System.Windows.Navigation.RaiseTypedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1195655
// Developer: huwang
// Reason: All those events are internal.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.JournalEntryStack.OnCollectionChanged():System.Void")]



//RukmaniG
//Bug #1195661
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="Microsoft.Internal.DeploymentUI.InstallationErrorPage.OnRetry(System.Object,System.Windows.RoutedEventArgs):System.Void")]

//RukmaniG
//Bug #1195662
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="Microsoft.Internal.DeploymentUI.InstallationProgressPage.OnCancel(System.Object,System.Windows.RoutedEventArgs):System.Void")]

//RukmaniG
//Bug #1198042
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigatingCancelEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationProgressEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.NavigationEventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Navigation.FragmentNavigationEventArgs):System.Void")]

//RukmaniG
//Bug #1199948
//See bug description for more details
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnActivated(System.EventArgs):System.Void")]
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Windows.Application.OnDeactivated(System.EventArgs):System.Void")]


//**************************************************************************************************************************
// Bug ID: 1195655
// Developer: rruiz
// Reason: Event has been reviewed.  Its fired when a DependencyPropertyChanged event is fired.  No state is kept and no work is done after the event is raised.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1001:RaiseEventCallerMustBeHardened", Scope="member", Target="System.Windows.Controls.StickyNoteControl.OnExpandedChanged(System.Windows.RoutedEventArgs):System.Void")]

//**************************************************************************************************************************
// Bug ID: 1341049
// Developer: HamidM
// Reason: _dialogOwnerHandle is simply a reference to hwnd NOT owned by the Window class.  As such, there is no
//         need to change it to SafeHandle
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Window._dialogOwnerHandle")]

//**************************************************************************************************************************
// Bug ID: 1340942
// Developer: Kiranku
// Reason: False positive.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Utility.TraceProvider.Finalize():System.Void")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//**************************************************************************************************************************
// Bug ID: DevDivBugs 181013
// Developer: AndreN for HuWang
// Reason: _dialogPreviousActiveHandle is simply a reference to an hwnd, not memory or a file.  As such, there is no
//         need to change it to SafeHandle
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="System.Windows.Window.#_dialogPreviousActiveHandle")]

//**************************************************************************************************************************
// Bug ID: 632977
// Developer: SamBent
// Reason: False positive
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Data.BindingExpression.#AttachOverride(System.Windows.DependencyObject,System.Windows.DependencyProperty)")]

[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Reflection.Assembly.LoadWithPartialName", Scope="member", Target="System.Windows.Baml2006.Baml2006SchemaContext.#ResolveAssembly(System.Windows.Baml2006.Baml2006SchemaContext+BamlAssembly)", Justification="Need to support loading assemblies by short names.")]

[module: SuppressMessage("Microsoft.Reliability","CA2001:AvoidCallingProblematicMethods", MessageId="System.Reflection.Assembly.LoadWithPartialName", Scope="member", Target="System.Windows.Baml2006.Baml6Assembly.#get_Assembly()", Justification="Need to support loading assemblies by short names.")]

//**************************************************************************************************************************
// Developer: ifeanyie
// Reason: False positive
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Internal.Interop.MouseHookManager.#HookThreadProc(System.Object)")]

//**************************************************************************************************************************
// Developer: andren
// Reason: False positive.  The call to GC.KeepAlive is required.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="System.Windows.Controls.ListBox.#MakeAnchorSelection(System.Windows.Controls.ListBoxItem,System.Boolean)")]

