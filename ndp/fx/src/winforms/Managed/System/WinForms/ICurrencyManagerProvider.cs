//------------------------------------------------------------------------------
// <copyright file="ICurrencyManagerProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;

    /// <include file='doc\ICurrencyManagerProvider.uex' path='docs/doc[@for="ICurrencyManagerProvider"]/*' />
    /// <devdoc>
    /// </devdoc>
    [SRDescription(SR.ICurrencyManagerProviderDescr)]
    public interface ICurrencyManagerProvider {

        /// <include file='doc\ICurrencyManagerProvider.uex' path='docs/doc[@for="ICurrencyManagerProvider.CurrencyManager"]/*' />
        /// <devdoc>
        ///     Return the main currency manager for this data source.
        /// </devdoc>
        CurrencyManager CurrencyManager { get; }

        /// <include file='doc\ICurrencyManagerProvider.uex' path='docs/doc[@for="ICurrencyManagerProvider.GetRelatedCurrencyManager"]/*' />
        /// <devdoc>
        ///     Return a related currency manager for specified data member on this data source.
        ///     If data member is null or empty, this method returns the data source's main currency
        ///     manager (ie. this method returns the same value as the CurrencyManager property).
        /// </devdoc>
        CurrencyManager GetRelatedCurrencyManager(string dataMember);
    }

}
