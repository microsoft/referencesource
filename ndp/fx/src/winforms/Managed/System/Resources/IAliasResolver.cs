//------------------------------------------------------------------------------
// <copyright file="IAliasResolver.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#if SYSTEM_WEB  // See DevDiv 9030
namespace System.PrivateResources {
#else
namespace System.Resources {
#endif

    using System.Diagnostics;
    using System.Runtime.Serialization;
    using System;
    using System.Reflection;
    /// <include file='doc\IAliasResolver.uex' path='docs/doc[@for="IAliasResolver"]/*' />
    /// <devdoc>
    /// Summary of IAliasResolver.
    /// </devdoc>
    internal interface IAliasResolver {
        AssemblyName ResolveAlias(string alias);
        void PushAlias(string alias, AssemblyName name);
    }
}

