//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;

//**************************************************************************************************************************
// Bug ID: 1061939 
// Developer: IgorBel 
// Reason: In trhis particulart cases we are calling the slow Enum.IsDefined function in the following cases on the Fikle Open transaction 
// which is inherently slow as a IO operation, or container add part transaction, which is also slow as it will result in IO
// We do not believe that switching this checks to afaster versions will be detectable by any reasonable perf tests.   
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1803:AvoidCostlyCallsWherePossible", Scope="member", Target="MS.Internal.IO.Zip.ZipArchive.ValidateModeAccessShareStreaming(System.IO.Stream,System.IO.FileMode,System.IO.FileAccess,System.IO.FileShare,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1803:AvoidCostlyCallsWherePossible", Scope="member", Target="MS.Internal.IO.Zip.ZipArchive.AddFile(System.String,MS.Internal.IO.Zip.CompressionMethodEnum,MS.Internal.IO.Zip.DeflateOptionEnum):MS.Internal.IO.Zip.ZipFileInfo")]




//***************************************************************************************************************************
// Proposed Suppressions from 3.0 RTM
//***************************************************************************************************************************


//**************************************************************************************************************************
// Bug ID: 1843710
// Developer: KenLai
// Reason: these only appear on CHK builds
//**************************************************************************************************************************
#if DEBUG
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetGrantsFromBoundUseLicense(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.Security.RightsManagement.ContentUser):System.Collections.Generic.List`1<System.Security.RightsManagement.ContentGrant>")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetGrantsFromBoundUseLicenseList(System.Collections.Generic.List`1<MS.Internal.Security.RightsManagement.SafeRightsManagementHandle>,System.Security.RightsManagement.ContentUser):System.Collections.Generic.List`1<System.Security.RightsManagement.ContentGrant>")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetBoundLicenseStringAttribute(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32):System.String")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Internal.Security.RightsManagement.ClientSession.GetBoundLicenseDateTimeAttribute(MS.Internal.Security.RightsManagement.SafeRightsManagementHandle,System.String,System.UInt32,System.DateTime):System.DateTime")]
#endif

//**************************************************************************************************************************
// Bug ID: 1074773
// Developer: chandras
// Reason: This SRID is defined in shared\resources\exceptionstringtable.txt and used by other DLLs, so putting a suppression for this in WindowsBase.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionIsFixedSize():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: 1117678
// Developer: chandras
// Reason: This SRID is defined in shared\resources\exceptionstringtable.txt and used by other DLLs, so putting a suppression for this in WindowsBase.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_CollectionNumberOfElementsMustBeGreaterThanZero():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: 1099475
// Developer: chandras
// Reason: This SRID is defined in shared\resources\exceptionstringtable.txt and used by other DLLs, so putting a suppression for this in WindowsBase.dll
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_Animation_KeyTime_InvalidWhenDurationIsForever():System.Windows.SRID")]

//**************************************************************************************************************************
// Bug ID: 1100749
// Developer: cedricd
// Reason: InsertionMap is in the Shared directory and is thus copied in every assembly. Others use it.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.InsertionSortMap.Sort():System.Void")]

//**************************************************************************************************************************
// Bug ID: 1131122
// Developer: patricsw
// Reason: All of these methods are part of a generic data structure object. Although the methods are not currently
//         called it would seem silly to delete them so that in the future somebody would have to reimplement them
//         if they needed them (i.e. it sorts of like implementing a list with GetFirst()/GetLast() and deleting
//         GetLast() because it hasn't been used yet..for code maintenance it makes sense (IMHO) to leave these in).
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.Add(System.Int32):System.Int32")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.AppendTo(MS.Utility.ItemStructList`1<T>&):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.Clear():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.Contains(T):System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.EnsureIndex(System.Int32):System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.IsValidIndex(System.Int32):System.Boolean")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.Sort():System.Void")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="MS.Utility.ItemStructList`1.ToArray():T[]")]


// Bug ID: 1154712
// Developer: cedricd
// Reason: InsertionSortMap, FrugalMap, and FrugalList are helper data structures that are compiled into PC, PF, and WB.  Even though
// WindowsBase isn't using this, one of the others may be, so it would be an error to remove it.  At the very least these
// should be excluded for maintainability reasons.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.InsertionSortMap")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.FrugalMap")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.FrugalStructList`1")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="type", Target="MS.Utility.FrugalObjectList`1")]

// Bug ID: 1653880
// Developer: SamBent
// Reason:  Dispose is not always able to remove the subclass from the WndProc chain.
//          The Finalizer uses a much more aggressive approach that always removes
//          the subclass, perhaps at the expense of other subclasses.  Suppressing
//          the finalizer might skip this step during shutdown, which could lead to
//          a crash later on if the managed WndProc is called after the CLR has shut down.
[module: SuppressMessage("Microsoft.Performance", "CA1816:DisposeMethodsShouldCallSuppressFinalize", Scope="member", Target="MS.Win32.HwndSubclass.Dispose():System.Void")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

// Not removing resource strings for SP1
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Windows.SRID.get_SortDescriptionPropertyNameCannotBeEmpty():System.Windows.SRID")]


//***************************************************************************************************************************
// New Suppressions since 4.0 RTM
//***************************************************************************************************************************

//***************************************************************************************************************************
// Source code is not owned by WPF.  This method may be used by others, so suppress the warning.
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.IO.Compression.ZLibNative+NativeZLibDLLStub+SafeLibraryHandle.#.ctor(System.IntPtr)")]
