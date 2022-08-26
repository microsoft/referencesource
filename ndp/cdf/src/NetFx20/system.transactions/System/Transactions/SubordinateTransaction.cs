//-----------------------------------------------------------------------------
// <copyright file="SubordinateTransaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace System.Transactions
{
    using System;
    using System.Runtime.Serialization;
    using System.Transactions.Diagnostics;

    /// <include file='doc\SubordinateTransaction.uex' path='docs/doc[@for="SubordinateTransaction"]/*' />
    // When we serialize a SubordinateTransaction, we specify the type OletxTransaction, so a SubordinateTransaction never
    // actually gets deserialized.
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2229:ImplementSerializationConstructors")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA2240:ImplementISerializableCorrectly")]
    [Serializable]
    public sealed class SubordinateTransaction : Transaction
    {
        // Create a transaction with the given settings
        //
        public SubordinateTransaction(
            IsolationLevel isoLevel,
            ISimpleTransactionSuperior superior
            ) : base( isoLevel, superior )
        {
        }
    }
}

