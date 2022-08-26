using System.Runtime.CompilerServices;
using System.Reflection;
using System.Diagnostics.CodeAnalysis;

[assembly:System.Security.SecurityCritical]

[assembly:StringFreezingAttribute()]
[assembly:DefaultDependencyAttribute(LoadHint.Always)]

// Generated code suppressions
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.SR.get_Resources():System.Resources.ResourceManager")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.SR.GetString(System.String,System.Object[]):System.String")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.SR.GetObject(System.String):System.Object")]


[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+PreconditionException..ctor(System.String,System.Exception)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+PostconditionException..ctor(System.String,System.Exception)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+PostconditionException..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+InvariantException..ctor(System.String,System.Exception)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+InvariantException..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.CodeContract+AssumptionException..ctor(System.String,System.Exception)")]
[module: SuppressMessage("Microsoft.Performance","CA1812:AvoidUninstantiatedInternalClasses", Scope="type", Target="System.Diagnostics.Contracts.CodeContract+AssertionException")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.RewriterInvariant(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.RewriterEnsures(System.Boolean):System.Void")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.Result<T>():T")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.Parameter<T>(T&):T")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.Old<T>(T):T")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.ForAll(System.Int32,System.Int32,System.Predicate`1<System.Int32>):System.Boolean")]
[module: SuppressMessage("Microsoft.Performance","CA1811:AvoidUncalledPrivateCode", Scope="member", Target="System.Diagnostics.Contracts.Contract.Exists(System.Int32,System.Int32,System.Predicate`1<System.Int32>):System.Boolean")]

