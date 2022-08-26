using System;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace System.Transactions
{
    public struct TransactionOptions
    {
        private TimeSpan timeout;
        private System.Transactions.IsolationLevel isolationLevel;

        public TimeSpan Timeout
        {
            get { return this.timeout; }
            set { this.timeout = value; }
        }

        public System.Transactions.IsolationLevel IsolationLevel
        {
            get { return this.isolationLevel; }
            set { this.isolationLevel = value; }
        }


        public override int GetHashCode()
        {
            return base.GetHashCode();  // Don't have anything better to do.
        }


        public override bool Equals( object obj )
        {
            if ( !(obj is TransactionOptions) )
            {
                // Can't use 'as' for a value type
                return false;
            }
            TransactionOptions opts = (TransactionOptions)obj;

            return (opts.timeout == this.timeout) && (opts.isolationLevel == this.isolationLevel);
        }


        // Changing paramater names would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static bool operator ==( TransactionOptions x, TransactionOptions y )
        {
            return x.Equals( y );
        }


        // Changing paramater names would be a breaking change for little benefit.
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Naming", "CA1704:IdentifiersShouldBeSpelledCorrectly")]
        public static bool operator !=( TransactionOptions x, TransactionOptions y )
        {
            return !x.Equals( y );
        }
    }
}
