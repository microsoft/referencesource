//
// FxCop Violation Message Suppressions
//  Approved List
//

using System.Diagnostics.CodeAnalysis;


//**************************************************************************************************************************
// Bug ID: Private run
// Developer: andren
// Reason: partial trust security team has reviewed all of these
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="System.Windows.Markup.Compiler.XamlReaderHelper.GetTemporaryAvalonXmlnsDefinitions():System.String[]")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetAssemblyPartialName(System.Reflection.Assembly):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetFullAssemblyNameFromPartialName(System.Reflection.Assembly,System.String):System.String")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.Utility.LoadWrapper.Load(System.String,System.String):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope="member", Target="MS.Internal.Utility.LoadWrapper.Load(System.String,System.String):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="Microsoft.Build.Tasks.Windows.UidManager.Execute():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="MS.Internal.MarkupCompiler._Compile(System.String,System.Boolean,System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="MS.Internal.MarkupCompiler.Initialize(System.String):System.String")]
[module: SuppressMessage("Microsoft.Security", "CA2102:CatchNonClsCompliantExceptionsInGeneralHandlers", Scope="member", Target="MS.Internal.MarkupCompiler.Compile(MS.Internal.CompilationUnit):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetLoadedAssembly(System.String,System.Boolean):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies", Scope="member", Target="System.Windows.Markup.Compiler.XamlReaderHelper.GetTemporaryAvalonXmlnsDefinitions():System.String[]")]
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetLoadedAssembly(System.String,System.Boolean):System.Reflection.Assembly")]
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetAssemblyPartialName(System.Reflection.Assembly):System.String")]
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies", Scope="member", Target="MS.Internal.SafeSecurityHelper.GetFullAssemblyNameFromPartialName(System.Reflection.Assembly,System.String):System.String")]

// *** added 3/16/05:
[module: SuppressMessage("Microsoft.Security", "CA2110:SecureGetObjectDataOverrides", Scope="member", Target="System.Windows.Markup.Compiler.XamlSerializationCallbackException.GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext):System.Void")]

// *** added 3/20/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Compile.UnsafeNativeMethods.FindMimeFromData(System.Runtime.InteropServices.ComTypes.IBindCtx,System.String,System.IntPtr,System.Int32,System.String,System.Int32,System.String&,System.Int32):System.Void")]

// *** added 3/24/05:
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope="member", Target="MS.Win32.Compile.UnsafeNativeMethods.FindMimeFromData(System.Runtime.InteropServices.ComTypes.IBindCtx,System.String,System.IntPtr,System.Int32,System.String,System.Int32,System.String&,System.Int32):System.Int32")]
//bug 1131029:
[module: SuppressMessage("Microsoft.Security", "CA2110:SecureGetObjectDataOverrides", Scope="member", Target="MS.Internal.Markup.XamlSerializationCallbackException.GetObjectData(System.Runtime.Serialization.SerializationInfo,System.Runtime.Serialization.StreamingContext):System.Void")]

// *** added 11/28/05
[module: SuppressMessage("Microsoft.SecurityCritical", "Test002:SecurityCriticalMembersMustExistInCriticalTypesAndAssemblies")]


//***************************************************************************************************************************
// New Suppressions since 3.0 RTM
//***************************************************************************************************************************

//***************************************************************************************************************************
// New Suppressions since v4 RTM
//***************************************************************************************************************************

//Some SafeCritical attributes in code shared between base and pbt lead to this warning.  Didn't want to modify the dev code to avoid this, so suppressing.
[module: SuppressMessage("Microsoft.Security","CA2136:TransparencyAnnotationsShouldNotConflictFxCopRule", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.ContainerUtilities.#SizeOfInt16()")]
[module: SuppressMessage("Microsoft.Security","CA2136:TransparencyAnnotationsShouldNotConflictFxCopRule", Scope="member", Target="MS.Internal.IO.Packaging.CompoundFile.ContainerUtilities.#SizeOfInt32()")]

//**************************************************************************************************************************
// Bug ID: Private run
// Developer: ifeanyie
// Reason: We do not need the hash for cryptographic reasons and we need to match the hash algorithm used by CodeChecksumPragma.ChecksumAlgorithmId
//***************************************************************************************************************************
[module: SuppressMessage("Microsoft.Cryptographic.Standard","CA5350:MD5CannotBeUsed", Scope="member", Target="MS.Internal.MarkupCompiler.#GenerateSource()")]
[module: SuppressMessage("Microsoft.Cryptographic.Standard","CA5354:SHA1CannotBeUsed", Scope="member", Target="MS.Internal.MarkupCompiler.#GenerateSource()")]
[module: SuppressMessage("Microsoft.Cryptographic.Standard","CA5350:MD5CannotBeUsed", Scope="member", Target="MS.Internal.TaskFileService.#GetChecksum(System.String,System.Guid)")]
[module: SuppressMessage("Microsoft.Cryptographic.Standard","CA5354:SHA1CannotBeUsed", Scope="member", Target="MS.Internal.TaskFileService.#GetChecksum(System.String,System.Guid)")]
