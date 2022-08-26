// This file is used by Code Analysis to maintain SuppressMessage
// attributes that are applied to this project. Project-level
// suppressions either have no target or are given a specific target
// and scoped to a namespace, type, member, etc.
//
// To add a suppression to this file, right-click the message in the
// Error List, point to "Suppress Message(s)", and click "In Project
// Suppression File". You do not need to add suppressions to this
// file manually.

using System.Diagnostics.CodeAnalysis;

[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId = "Edm", Scope = "resource", Target = "System.Data.Entity.Design.resources")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Design.Error.#ArgumentNull(System.String)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Design.Error.#ArgumentOutOfRange(System.String)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Design.Error.#NotImplemented()")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.Services.Design.Error.#NotSupported()")]
[module: SuppressMessage("Microsoft.Performance", "CA1823", Scope = "member", Target = "System.Data.Metadata.Edm.EdmConstants.#UnboundedValue")]
[module: SuppressMessage("Microsoft.Design", "CA1001", Scope = "type", Target = "System.Data.Common.Utils.GlobalDictionary'2")]
[module: SuppressMessage("Microsoft.Naming", "CA1703:ResourceStringsShouldBeSpelledCorrectly", MessageId = "datasvcmap", Scope = "resource", Target = "System.Data.Services.Design.resources")]

// Violations in the generated Resource file; can't prevent these from being generated...
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.EDesignRes.#GetObject(System.String)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.EDesignRes.#Resources")]
[module: SuppressMessage("Microsoft.Performance", "CA1805:DoNotInitializeUnnecessarily", Scope = "member", Target = "System.Data.EDesignRes..cctor()")]
[module: SuppressMessage("Microsoft.Performance", "CA1805:DoNotInitializeUnnecessarily", Scope = "member", Target = "System.Data.EDesignResDescriptionAttribute..ctor(System.String)")]
[module: SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode", Scope = "member", Target = "System.Data.EDesignRes.#GetString(System.String,System.Boolean&)")]
