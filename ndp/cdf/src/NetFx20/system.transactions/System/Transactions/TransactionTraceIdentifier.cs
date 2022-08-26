using System;

namespace System.Transactions
{
    /// <summary>
    /// This identifier is used in tracing to distiguish instances
    /// of transaction objects.  This identifier is only unique within
    /// a given AppDomain.
    /// </summary>
    internal struct TransactionTraceIdentifier
    {
        public static readonly TransactionTraceIdentifier Empty = new TransactionTraceIdentifier();

        public TransactionTraceIdentifier(
            string transactionIdentifier,
            int cloneIdentifier
            )
        {
            this.transactionIdentifier = transactionIdentifier;
            this.cloneIdentifier = cloneIdentifier;
        }
        
        private string transactionIdentifier;
        /// <summary>
        /// The string representation of the transaction identifier.
        /// </summary>
        public string TransactionIdentifier
        {
            get
            {
                return this.transactionIdentifier;
            }
        }

        private int cloneIdentifier;
        /// <summary>
        /// An integer value that allows different clones of the same
        /// transaction to be distiguished in the tracing.
        /// </summary>
        public int CloneIdentifier
        {
            get
            {
                return this.cloneIdentifier;
            }
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();  // Don't have anything better to do.
        }

        public override bool Equals ( object objectToCompare )
        {
            if ( ! ( objectToCompare is TransactionTraceIdentifier ) )
            {
                return false;
            }

            TransactionTraceIdentifier id = (TransactionTraceIdentifier) objectToCompare;
            if ( ( id.TransactionIdentifier != this.TransactionIdentifier ) ||
                ( id.CloneIdentifier != this.CloneIdentifier ) )
            {
                return false;
            }
            return true;
        }

        public static bool operator ==( TransactionTraceIdentifier id1, TransactionTraceIdentifier id2 )
        {
            return id1.Equals( id2 );
        }


        public static bool operator !=( TransactionTraceIdentifier id1, TransactionTraceIdentifier id2 )
        {
            return !id1.Equals( id2 );
        }
    }
}

