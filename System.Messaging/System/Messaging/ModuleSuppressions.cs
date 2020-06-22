//
// Module-level FxCop supressions are kept in this file
//
using System.Diagnostics.CodeAnalysis;

//
// TODO, eugenesh: bug 532346
// Fixing two groups of issues below (SpecifyStringComparison and UseOrdinalStringComparison)
// will introduce too much churn at this late stage of Whidbey RTM.
// Therefore, we exclude now and need to fix these violatins in Orcas
//
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.ValidatePath(System.String,System.Boolean):System.Boolean", MessageId = "System.String.EndsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.ValidatePath(System.String,System.Boolean):System.Boolean", MessageId = "System.String.StartsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.get_QueuePath():System.String", MessageId = "System.String.StartsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.IsCanonicalPath(System.String,System.Boolean):System.Boolean", MessageId = "System.String.EndsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.IsCanonicalPath(System.String,System.Boolean):System.Boolean", MessageId = "System.String.StartsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.Exists(System.String):System.Boolean", MessageId = "System.String.StartsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.get_FormatName():System.String", MessageId = "System.String.StartsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.ResolveFormatNameFromQueuePath(System.String,System.Boolean):System.String", MessageId = "System.String.EndsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA1307:SpecifyStringComparison", Scope = "member", Target = "System.Messaging.AccessControlList.MakeAcl(System.IntPtr):System.IntPtr", MessageId = "System.String.EndsWith(System.String)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.ReceiveBy(System.String,System.TimeSpan,System.Boolean,System.Boolean,System.Boolean,System.Messaging.MessageQueueTransaction,System.Messaging.MessageQueueTransactionType):System.Messaging.Message", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.GetPublicQueuesByMachine(System.String):System.Messaging.MessageQueue[]", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.GetPrivateQueuesByMachine(System.String):System.Messaging.MessageQueue[]", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueue.ResolveFormatNameFromQueuePath(System.String,System.Boolean):System.String", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueueEnumerator.MoveNext():System.Boolean", MessageId = "System.String.Compare(System.String,System.Int32,System.String,System.Int32,System.Int32,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.MessageQueuePermission.FromXml(System.Security.SecurityElement):System.Void", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.Design.MessageQueueConverter..cctor()", MessageId = "System.Collections.Hashtable.#ctor(System.Collections.IEqualityComparer)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.Design.QueuePathDialog.ChoosePath():System.Void", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]
[module: SuppressMessage("Microsoft.Globalization", "CA130:UseOrdinalStringComparison", Scope = "member", Target = "System.Messaging.Design.QueuePathDialog.OnSelectQueue(System.Messaging.MessageQueue,System.Int32):System.Void", MessageId = "System.String.Compare(System.String,System.String,System.Boolean,System.Globalization.CultureInfo)")]


// performance messages
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Design.SizeConverter..ctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Design.MessageFormatterConverter..ctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Design.TimeoutConverter..ctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Design.MessageQueueConverter..ctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Res.GetObject(System.String):System.Object")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Messaging.Res.get_Resources():System.Resources.ResourceManager")]
[module: SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Scope = "type", Target = "System.Messaging.ResCategoryAttribute")]

//naming messages
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope = "resource", Target = "System.Messaging.resources", MessageId = "msmq")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", Scope = "resource", Target = "System.Messaging.resources", MessageId = "propid")]

//reliability messages

// we have a number of public APIs that return handles as IntPtr. Because we use SafeHandles internally,
// we have to call DangerousGetHandle to get IntPtr.
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.MessageQueue.get_WriteHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.MessageQueue.get_ReadHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.MessageQueueEnumerator.get_LocatorHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.MessageEnumerator.get_CursorHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.Message.set_SecurityContext(System.Messaging.SecurityContext):System.Void")]

// we have to pass IntPtr to BindToThreadPool, so we need to call DangerousGetHandle
[module: SuppressMessage("Microsoft.Reliability", "CA2001:AvoidCallingProblematicMethods", Scope = "member", Target = "System.Messaging.MessageQueue+MQCacheableInfo.BindToThreadPool():System.Void")]

//
// We have a group of "meta-messages" related to CAS. All but one have been reviewed in Everett
//
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.add_ReceiveCompleted(System.Messaging.ReceiveCompletedEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.SetPermissions(System.Messaging.AccessControlList):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.GenerateQueueProperties():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.get_CanWrite():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.GetMessageEnumerator():System.Messaging.MessageEnumerator")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.get_CanRead():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.get_WriteHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.ReceiveCurrent(System.TimeSpan,System.Int32,System.Messaging.Interop.CursorHandle,System.Messaging.MessagePropertyFilter,System.Messaging.MessageQueueTransaction,System.Messaging.MessageQueueTransactionType):System.Messaging.Message")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.ResetPermissions():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.SaveQueueProperties():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.InternalReceiveByLookupId(System.Boolean,System.Messaging.MessageLookupAction,System.Int64,System.Messaging.MessageQueueTransaction,System.Messaging.MessageQueueTransactionType):System.Messaging.Message")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.Delete(System.String):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.SendInternal(System.Object,System.Messaging.MessageQueueTransaction,System.Messaging.MessageQueueTransactionType):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.ReceiveAsync(System.TimeSpan,System.Messaging.Interop.CursorHandle,System.Int32,System.AsyncCallback,System.Object):System.IAsyncResult")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.Purge():System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.get_ReadHandle():System.IntPtr")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.get_Transactional():System.Boolean")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.GetMessageEnumerator2():System.Messaging.MessageEnumerator")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueue.add_PeekCompleted(System.Messaging.PeekCompletedEventHandler):System.Void")]
[module: SuppressMessage("Microsoft.Security", "CA2103:ReviewImperativeSecurity", Scope = "member", Target = "System.Messaging.MessageQueuePermissionAttribute.CreatePermission():System.Security.IPermission")]
[module: SuppressMessage("Microsoft.Security", "CA2106:SecureAsserts", Scope = "member", Target = "System.Messaging.MessageQueueCriteria.set_MachineName(System.String):System.Void")]
// Following two calls don't do check on MessageEnumerator.MoveNext path. However, MoveNext by itself doesn't expose any information.
// Current or RemoveCurrent will do the check
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope = "member", Target = "System.Messaging.Interop.UnsafeNativeMethods.MQReceiveMessage(System.Messaging.Interop.MessageQueueHandle,System.UInt32,System.Int32,System.Messaging.Interop.MessagePropertyVariants+MQPROPS,System.Threading.NativeOverlapped*,System.Messaging.Interop.SafeNativeMethods+ReceiveCallback,System.Messaging.Interop.CursorHandle,System.IntPtr):System.Int32")]
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope = "member", Target = "System.Messaging.Interop.UnsafeNativeMethods.MQReceiveMessage(System.Messaging.Interop.MessageQueueHandle,System.UInt32,System.Int32,System.Messaging.Interop.MessagePropertyVariants+MQPROPS,System.Threading.NativeOverlapped*,System.Messaging.Interop.SafeNativeMethods+ReceiveCallback,System.Messaging.Interop.CursorHandle,System.Messaging.Interop.ITransaction):System.Int32")]
// This call is exposed through Cursor ctor called in MessageQueue.CreateCursor. However, Cursor by itself doesn't expose any information.
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope = "member", Target = "System.Messaging.Interop.UnsafeNativeMethods.IntMQOpenQueue(System.String,System.Int32,System.Int32,System.Messaging.Interop.MessageQueueHandle&):System.Int32")]
// This call is exposed through Message.SourceMachine. Message has been received, implying that receiver has necessary permissions.
[module: SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage", Scope = "member", Target = "System.Messaging.Interop.UnsafeNativeMethods.IntMQGetMachineProperties(System.String,System.IntPtr,System.Messaging.Interop.MessagePropertyVariants+MQPROPS):System.Int32")]




