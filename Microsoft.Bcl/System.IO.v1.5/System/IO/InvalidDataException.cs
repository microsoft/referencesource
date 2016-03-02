namespace System.IO {
    
    using System;

    /// <summary>
    /// The exception that is thrown when a data stream is in an invalid format.
    /// </summary>
    public sealed class InvalidDataException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="T:System.IO.InvalidDataException" /> class.
        /// </summary>
        public InvalidDataException () 
            : base(Strings.GenericInvalidData) {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="T:System.IO.InvalidDataException" /> class with a specified error message.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        public InvalidDataException (String message) 
            : base(message) {
        }
    
        /// <summary>
        /// Initializes a new instance of the <see cref="T:System.IO.InvalidDataException" /> class with a reference to the inner exception that is the cause of this exception.</summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception. If the <paramref name="innerException" /> parameter is not null, the current exception is raised in a catch block that handles the inner exception.</param>
        public InvalidDataException (String message, Exception innerException) 
            : base(message, innerException) {
        }

    }
}
