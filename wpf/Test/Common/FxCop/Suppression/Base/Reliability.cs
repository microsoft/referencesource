//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;



//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************

	
//**************************************************************************************************************************
// Bug ID: 1340942
// Developer: Kiranku
// Reason: False positive.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability", "CA2004:RemoveCallsToGCKeepAlive", Scope="member", Target="MS.Utility.TraceProvider.Finalize():System.Void")]

// DavidJen
// Bug #1214012
// the CollectionChanged event is never called, only used to test if the OC has listeners
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.Collections.ObjectModel.ObservableCollection`1.CheckReentrancy():System.Void")]

// DavidJen
// Bug #1213089
// no GroupDescription state is affected should the OnPropertyChanged event fail with an exception
[module: SuppressMessage("Microsoft.AvalonObjectModelInternal", "AvOMInt1000:PublicEventCallerMustBeHardened", Scope="member", Target="System.ComponentModel.GroupDescription.OnPropertyChanged(System.String):System.Void")]



//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************




//**************************************************************************************************************************
// Bug ID: No Bug
// Developer: pantal
// Reason: Won't fix legacy items, clearing up FxCop scans.
//**************************************************************************************************************************
[module: SuppressMessage("Microsoft.Reliability","CA2006:UseSafeHandleToEncapsulateNativeResources", Scope="member", Target="MS.Internal.Interop.PROPVARIANT.#pointerVal")]