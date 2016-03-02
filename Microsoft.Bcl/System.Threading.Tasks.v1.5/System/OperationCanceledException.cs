using System.Threading;

namespace System
{
    /// <summary>The exception that is thrown in a thread upon cancellation of an operation that the thread was executing.</summary>
    public class OperationCanceledException : Exception
    {
        private CancellationToken _cancellationToken;

        /// <summary>Gets a token associated with the operation that was canceled.</summary>
        public CancellationToken CancellationToken
        {
            get { return _cancellationToken; }
            private set { _cancellationToken = value; }
        }

        /// <summary>Initializes the exception.</summary>
        public OperationCanceledException()
            : base(Strings.OperationCanceled)
        {
        }

        /// <summary>Initializes the exception.</summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public OperationCanceledException(String message)
            : base(message)
        {
        }

        /// <summary>Initializes the exception.</summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception.</param>
        public OperationCanceledException(String message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>Initializes the exception.</summary>
        /// <param name="token">A cancellation token associated with the operation that was canceled.</param>
        public OperationCanceledException(CancellationToken token)
            : this()
        {
            CancellationToken = token;
        }

        /// <summary>Initializes the exception.</summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="token">A cancellation token associated with the operation that was canceled.</param>
        public OperationCanceledException(String message, CancellationToken token)
            : this(message)
        {
            CancellationToken = token;
        }

        /// <summary>Initializes the exception.</summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception.</param>
        /// <param name="token">A cancellation token associated with the operation that was canceled.</param>
        public OperationCanceledException(String message, Exception innerException, CancellationToken token)
            : this(message, innerException)
        {
            CancellationToken = token;
        }
    }
}