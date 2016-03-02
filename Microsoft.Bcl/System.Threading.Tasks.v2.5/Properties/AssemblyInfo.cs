//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//--------------------------------------------------------------------------
using System;
using System.Reflection;
using System.Resources;
using System.Runtime.CompilerServices;
using System.Security;
using System.Threading;
using System.Threading.Tasks;

// General Information about an assembly is controlled through the following 
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
[assembly: AssemblyTitle("System.Threading.Tasks")]
[assembly: AssemblyDescription("")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyProduct("System.Threading.Tasks")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]
[assembly: NeutralResourcesLanguage("en")]

[assembly: TypeForwardedTo(typeof(AggregateException))]
[assembly: TypeForwardedTo(typeof(OperationCanceledException))]
[assembly: TypeForwardedTo(typeof(CancellationToken))]
[assembly: TypeForwardedTo(typeof(CancellationTokenRegistration))]
[assembly: TypeForwardedTo(typeof(CancellationTokenSource))]
[assembly: TypeForwardedTo(typeof(Task))]
[assembly: TypeForwardedTo(typeof(Task<>))]
[assembly: TypeForwardedTo(typeof(TaskCanceledException))]
[assembly: TypeForwardedTo(typeof(TaskCompletionSource<>))]
[assembly: TypeForwardedTo(typeof(TaskContinuationOptions))]
[assembly: TypeForwardedTo(typeof(TaskCreationOptions))]
[assembly: TypeForwardedTo(typeof(TaskExtensions))]
[assembly: TypeForwardedTo(typeof(TaskFactory))]
[assembly: TypeForwardedTo(typeof(TaskFactory<>))]
[assembly: TypeForwardedTo(typeof(TaskScheduler))]
[assembly: TypeForwardedTo(typeof(TaskSchedulerException))]
[assembly: TypeForwardedTo(typeof(TaskStatus))]
[assembly: TypeForwardedTo(typeof(UnobservedTaskExceptionEventArgs))]
