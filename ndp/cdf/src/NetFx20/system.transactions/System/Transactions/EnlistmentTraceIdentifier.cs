using System;

namespace System.Transactions
{
    /// <summary>
    /// This identifier is used in tracing to distiguish transaction
    /// enlistments.  This identifier is only unique within
    /// a given AppDomain.
    /// </summary>
    internal struct EnlistmentTraceIdentifier
    {
        public static readonly EnlistmentTraceIdentifier Empty = new EnlistmentTraceIdentifier();

        public EnlistmentTraceIdentifier(
            Guid resourceManagerIdentifier,
            TransactionTraceIdentifier transactionTraceId,
            int enlistmentIdentifier
            )
        {
            this.resourceManagerIdentifier = resourceManagerIdentifier;
            this.transactionTraceIdentifier = transactionTraceId;
            this.enlistmentIdentifier = enlistmentIdentifier;
        }

        private Guid resourceManagerIdentifier;
        /// <summary>
        /// The Guid identifier of the resource manager that made the
        /// enlistment.  If the enlistment is a Volatile enlistment,
        /// this value will be Guid.Empty.
        /// </summary>
        public Guid ResourceManagerIdentifier
        {
            get
            {
                return this.resourceManagerIdentifier;
            }
        }

        private TransactionTraceIdentifier transactionTraceIdentifier;
        /// <summary>
        /// The TransactionTraceIdentifier for the transaction that is
        /// enlisted upon.
        /// </summary>
        public TransactionTraceIdentifier TransactionTraceId
        {
            get
            {
                return this.transactionTraceIdentifier;
            }
        }

        private int enlistmentIdentifier;
        /// <summary>
        /// A value that distiguishes between multiple enlistments on the same
        /// transaction instance by the same resource manager.
        /// </summary>
        public int EnlistmentIdentifier
        {
            get
            {
                return this.enlistmentIdentifier;
            }
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();  // Don't have anything better to do.
        }

        public override bool Equals ( object objectToCompare )
        {
            if ( ! ( objectToCompare is EnlistmentTraceIdentifier ) )
            {
                return false;
            }

            EnlistmentTraceIdentifier id = ( EnlistmentTraceIdentifier )objectToCompare;
            if ( (id.ResourceManagerIdentifier != this.ResourceManagerIdentifier ) ||
                ( id.TransactionTraceId != this.TransactionTraceId ) ||
                ( id.EnlistmentIdentifier != this.EnlistmentIdentifier ) )
            {
                return false;
            }
            return true;
        }

        public static bool operator ==( EnlistmentTraceIdentifier id1, EnlistmentTraceIdentifier id2 )
        {
            return id1.Equals( id2 );
        }


        // We need to equality operator and the compiler doesn't let us have an equality operator without an inequality operator,
        // so we added it and FXCop doesn't like the fact that we don't call it.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        public static bool operator !=( EnlistmentTraceIdentifier id1, EnlistmentTraceIdentifier id2 )
        {
            return !id1.Equals( id2 );
        }
    }

}

