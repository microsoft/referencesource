//---------------------------------------------------------------------
// <copyright file="GlobalSuppressions.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>global code analysis suppressions</summary>
//---------------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;

#if ASTORIA_LIGHT
[module: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1033:InterfaceMethodsShouldBeCallableByChildTypes", Scope = "member", Target = "System.Data.Services.Client.DataServiceQuery`1.#System.Collections.IEnumerable.GetEnumerator()")]
[module: System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Design", "CA1033:InterfaceMethodsShouldBeCallableByChildTypes", Scope = "member", Target = "System.Data.Services.Client.DataServiceQuery`1.#System.Collections.Generic.IEnumerable`1<!0>.GetEnumerator()")]
#endif
[module: SuppressMessage("Microsoft.Design", "CA1020:AvoidNamespacesWithFewTypes", Scope = "namespace", Target = "System.Data.Services.Common")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId = "orderby", Scope = "resource", Target = "System.Data.Services.Client.resources")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.Strings.#ClientType_MissingOpenProperty(System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.Strings.#Clienttype_MultipleOpenProperty(System.Object)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.Strings.#ClientType_NullOpenProperties(System.Object)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.Strings.#Collection_NullCollectionReference(System.Object,System.Object)")]
[module: SuppressMessage("Microsoft.Usage", "CA2201:DoNotRaiseReservedExceptionTypes", Scope = "member", Target = "System.Data.Services.Client.AtomMaterializer.#ProjectionGetEntry(System.Data.Services.Client.AtomEntry,System.String)")]
[module: SuppressMessage("Microsoft.Usage", "CA2201:DoNotRaiseReservedExceptionTypes", Scope = "member", Target = "System.Data.Services.Client.AtomMaterializer.#ProjectionInitializeEntity(System.Data.Services.Client.AtomMaterializer,System.Data.Services.Client.AtomEntry,System.Type,System.Type,System.String[],System.Func`4<System.Object,System.Object,System.Type,System.Object>[])")]

// Violations in the generated Resource file; can't prevent these from being generated...
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.TextRes.#GetObject(System.String)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.TextRes.#Resources")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Client.TextRes.#GetString(System.String,System.Boolean&)")]
[module: SuppressMessage("Microsoft.Performance", "CA1805:DoNotInitializeUnnecessarily", Scope = "member", Target = "System.Data.Services.Client.TextRes..cctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1805:DoNotInitializeUnnecessarily", Scope = "member", Target = "System.Data.Services.Client.TextResDescriptionAttribute..ctor(System.String)")]
