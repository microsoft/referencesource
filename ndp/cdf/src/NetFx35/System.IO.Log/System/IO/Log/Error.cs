//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
namespace System.IO.Log
{
    using System;
    using System.Diagnostics;
    using System.Runtime.InteropServices;


    static class Error
    {
        // A collection of errors, taken from various places in the
        // windows headers.
        //
        public const uint ERROR_SUCCESS = 0;
        public const uint ERROR_FILE_NOT_FOUND = 2;
        public const uint ERROR_PATH_NOT_FOUND = 3;
        public const uint ERROR_ACCESS_DENIED = 5;
        public const uint ERROR_INVALID_HANDLE = 6;
        public const uint ERROR_OUTOFMEMORY = 14;
        public const uint ERROR_SHARING_VIOLATION = 32;
        public const uint ERROR_HANDLE_EOF = 38;
        public const uint ERROR_NOT_SUPPORTED = 50;
        public const uint ERROR_DUP_NAME = 52;
        public const uint ERROR_BAD_NETPATH = 53;
        public const uint ERROR_INVALID_PARAMETER = 87;
        public const uint ERROR_BUFFER_OVERFLOW = 111;
        public const uint ERROR_INVALID_NAME = 123;
        public const uint ERROR_BAD_PATHNAME = 161;
        public const uint ERROR_ALREADY_EXISTS = 183;
        public const uint ERROR_MORE_DATA = 234;
        public const uint ERROR_NO_MORE_ITEMS = 259;
        public const uint ERROR_OPERATION_ABORTED = 995;
        public const uint ERROR_IO_PENDING = 997;
        public const uint ERROR_IO_DEVICE = 1117;
        public const uint ERROR_NOT_FOUND = 1168;
        public const uint ERROR_NO_SYSTEM_RESOURCES = 1450;
        public const uint ERROR_INVALID_USER_BUFFER = 1784;
        public const uint ERROR_CANT_RESOLVE_FILENAME = 1921;
        public const uint ERROR_INVALID_OPERATION = 4317;
        public const uint ERROR_INVALID_STATE = 5023;
        public const uint ERROR_LOG_SECTOR_PARITY_INVALID = 6601;
        public const uint ERROR_LOG_BLOCK_INCOMPLETE = 6603;
        public const uint ERROR_LOG_INVALID_RANGE = 6604;
        public const uint ERROR_LOG_READ_CONTEXT_INVALID = 6606;
        public const uint ERROR_LOG_BLOCK_VERSION = 6608;
        public const uint ERROR_LOG_BLOCK_INVALID = 6609;
        public const uint ERROR_LOG_NO_RESTART = 6611;
        public const uint ERROR_LOG_METADATA_CORRUPT = 6612;
        public const uint ERROR_LOG_RESERVATION_INVALID = 6615;
        public const uint ERROR_LOG_CANT_DELETE = 6616;
        public const uint ERROR_LOG_START_OF_LOG = 6618;
        public const uint ERROR_LOG_POLICY_NOT_INSTALLED = 6620;
        public const uint ERROR_LOG_POLICY_INVALID = 6621;
        public const uint ERROR_LOG_POLICY_CONFLICT = 6622;
        public const uint ERROR_LOG_TAIL_INVALID = 6627;
        public const uint ERROR_LOG_FULL = 6628;
        public const uint ERROR_LOG_NOT_ENOUGH_CONTAINERS = 6635;
        public const uint ERROR_LOG_FULL_HANDLER_IN_PROGRESS = 6638;


        // More error codes from simple file log -
        public const uint ERROR_MEDIUM_FULL = 112;
        public const uint ERROR_DISK_FULL = 127;
        public const uint ERROR_INVALID_HEADER = 251;
        public const uint ERROR_FILE_CORRUPT = 258;
        public const uint ERROR_E_OLDFORMAT = 260;

        // We would like to distinguish between a sequence full condition and log store
        // operation filling the disk. 

        public const uint LOGSTORE_ERROR_DISK_FULL = 112112112;

        public static Exception ExceptionForKnownCode(uint errorCode,
                                                      Exception innerException)
        {
            // This is the big mapping of code to exception for codes
            // that we recognize. It is called when we need to map to
            // a specific exception, and we recognize that the code
            // being returned is one that can legitimately come from
            // the called routine.
            //
            // If you get back a code that is not supposed to come
            // from the function you called, then you should use
            // ExceptionForUnknownCode instead, regardless of whether
            // or not we have a mapping for that code.
            //
            // The following codes should never be recognized:
            //     ERROR_BUFFER_OVERFLOW
            //     ERROR_MORE_DATA
            //     ERROR_NO_MORE_ITEMS
            //     ERROR_IO_PENDING
            //     ERROR_INVALID_USER_BUFFER
            //     ERROR_LOG_NO_RESTART
            //     ERROR_LOG_START_OF_LOG
            //
            switch (errorCode)
            {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    return new FileNotFoundException(
                        SR.GetString(SR.IO_FileNotFound),
                        innerException);

                case ERROR_CANT_RESOLVE_FILENAME:
                    return new ArgumentException(
                        SR.GetString(SR.Argument_LogFileNameInvalid),
                        innerException);

                case ERROR_ACCESS_DENIED:
                    return new UnauthorizedAccessException(
                        SR.GetString(SR.Unauthorized_AccessDenied),
                        innerException);

                case ERROR_INVALID_HANDLE:
                case ERROR_LOG_READ_CONTEXT_INVALID:
                    if (innerException == null)
                    {
                        return new ObjectDisposedException(String.Empty);
                    }
                    else
                    {
                        return new ObjectDisposedException(
                            innerException.Message,
                            innerException);
                    }

                case ERROR_OUTOFMEMORY:
                    return new OutOfMemoryException();

                case ERROR_SHARING_VIOLATION:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_SharingViolation),
                                               GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_SharingViolation),
                                               innerException);
                    }

                case ERROR_HANDLE_EOF:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_EndOfLog),
                                            GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_EndOfLog),
                                            innerException);
                    }

                case ERROR_NOT_SUPPORTED:
                    if (innerException == null)
                    {
                        return new NotSupportedException();
                    }
                    else
                    {
                        return new NotSupportedException(
                                    innerException.Message,
                                    innerException);
                    }

                case ERROR_INVALID_PARAMETER:
                    if (innerException == null)
                    {
                        return new ArgumentException();
                    }
                    else
                    {
                        return new ArgumentException(
                            innerException.Message,
                            innerException);
                    }
                case ERROR_LOG_BLOCK_INVALID:
                case ERROR_LOG_BLOCK_VERSION:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_InvalidLogFileFormat),
                                                GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_InvalidLogFileFormat),
                                                innerException);
                    }

                case ERROR_INVALID_NAME:
                case ERROR_BAD_PATHNAME:
                case ERROR_BAD_NETPATH:
                    return new ArgumentException(
                        SR.GetString(SR.Argument_FileNameInvalid),
                        innerException);

                case ERROR_ALREADY_EXISTS:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_AlreadyExists),
                                               GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_AlreadyExists),
                                               innerException);
                    }

                case ERROR_IO_DEVICE:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_DeviceHresult, GetHRForCode(errorCode)),
                                               GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_DeviceException, innerException.Message),
                                               innerException);
                    }

                case ERROR_INVALID_OPERATION:
                case ERROR_INVALID_STATE:
                    if (innerException == null)
                    {
                        return new InvalidOperationException();
                    }
                    else
                    {
                        return new InvalidOperationException(innerException.Message, innerException);
                    }

                case ERROR_LOG_BLOCK_INCOMPLETE:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_BlockIncomplete),
                                            GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_BlockIncomplete),
                                            innerException);
                    }

                case ERROR_LOG_INVALID_RANGE:
                    return new ArgumentOutOfRangeException(
                        SR.GetString(SR.Argument_LogInvalidRange),
                        innerException);

                case ERROR_LOG_CANT_DELETE:
                    if (innerException == null)
                    {
                        return new IOException(SR.GetString(SR.IO_LogCannotDelete),
                                               GetHRForCode(errorCode));
                    }
                    else
                    {
                        return new IOException(SR.GetString(SR.IO_LogCannotDelete),
                                               innerException);
                    }

                case ERROR_LOG_TAIL_INVALID:
                    return new ArgumentOutOfRangeException(
                        SR.GetString(SR.Argument_TailInvalid),
                        innerException);

                case LOGSTORE_ERROR_DISK_FULL:
                    return new IOException(SR.GetString(SR.DiskFull));

                case ERROR_MEDIUM_FULL:
                case ERROR_DISK_FULL:
                    return new SequenceFullException(SR.GetString(SR.DiskFull),
                                 innerException);

                case ERROR_LOG_FULL:
                case ERROR_NO_SYSTEM_RESOURCES:
                case ERROR_LOG_FULL_HANDLER_IN_PROGRESS:
                    return new SequenceFullException(SR.GetString(SR.SequenceFull),
                                                     innerException);

                case ERROR_LOG_POLICY_INVALID:
                    return new InvalidOperationException(
                        SR.GetString(SR.InvalidOperation_LogPolicyInvalid),
                        innerException);

                case ERROR_LOG_POLICY_CONFLICT:
                    return new InvalidOperationException(
                        SR.GetString(SR.InvalidOperation_LogPolicyConflict),
                        innerException);

                case ERROR_FILE_CORRUPT:
                case ERROR_INVALID_HEADER:
                case ERROR_LOG_METADATA_CORRUPT:
                    return new IOException(
                        SR.GetString(SR.IO_LogCorrupt),
                        innerException);

                case ERROR_E_OLDFORMAT:
                    return new IOException(
                        SR.GetString(SR.LogRecSeq_IncompatibleVersion),
                        innerException);

                case ERROR_NOT_FOUND:
                    return new ArgumentException(SR.GetString(SR.Argument_NotFound),
                                        innerException);
                case ERROR_LOG_RESERVATION_INVALID:
                    return new ArgumentException(SR.GetString(SR.Argument_InvalidReservation));

                case ERROR_LOG_NOT_ENOUGH_CONTAINERS:
                    return new InvalidOperationException(
                        SR.GetString(SR.InvalidOperation_MustHaveExtents),
                        innerException);
            }

            // The error codes in the above list are explicitly recognized by the (internal) caller as 
            // being known and understood. If we recognize and understand a new error code, it 
            // should be added to this list. Otherwise, the error code should be converted to an 
            // exception using ExceptionForUnknownCode. If we reach this point, we have a bug 
            // in our own error handling code.
            DiagnosticUtility.DebugAssert("Either the code is unknown, or it should not have failed.");
            return ExceptionForUnknownCode(errorCode);
        }

        public static Exception ExceptionForKnownCode(uint errorCode)
        {
            return ExceptionForKnownCode(errorCode, null);
        }

        public static Exception ExceptionForUnknownCode(uint errorCode)
        {
            int hr = GetHRForCode(errorCode);
            return new IOException(SR.GetString(SR.IO_UnexpectedException, hr), hr);
        }

        public static Exception ExceptionForUnknownCode(uint errorCode,
                                                        Exception innerException)
        {
            int hr = GetHRForCode(errorCode);
            return new IOException(SR.GetString(SR.IO_UnexpectedException, hr),
                                   innerException);
        }

        public static int GetHRForCode(uint errorCode)
        {
            if ((errorCode & 0x80000000) != 0x80000000)
            {
                errorCode = (uint)((errorCode & 0x0000FFFF) |
                                   unchecked((int)0x80070000));
            }

            return unchecked((int)errorCode);
        }

        public static Exception ConvertKnownException(COMException comException)
        {
            uint errorCode = GetErrorCodeFromHR(comException.ErrorCode);
            Exception exception = ExceptionForKnownCode(errorCode, comException);
            if (exception == null)
            {
                exception = ExceptionForUnknownCode(errorCode, comException);
            }

            return exception;
        }

        public static uint GetErrorCodeFromHR(int hresult)
        {
            uint u_hresult = unchecked((uint)hresult);
            return u_hresult & 0x0000FFFF;
        }

        public static Exception ArgumentInvalid(string resourceCode)
        {
            return new ArgumentException(SR.GetString(resourceCode));
        }

        public static Exception ArgumentNull(string argument)
        {
            return new ArgumentNullException(argument);
        }

        public static Exception ArgumentOutOfRange(string argument)
        {
            return new ArgumentOutOfRangeException(argument);
        }

        public static Exception ArgumentOutOfRange(string argument, Exception exp)
        {
            return new ArgumentOutOfRangeException(SR.GetString(argument), exp);
        }

        public static Exception DuplicateEnd()
        {
            return new InvalidOperationException(SR.GetString(SR.AsyncResult_DuplicateEnd));
        }

        public static Exception EnumEnded()
        {
            return new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumEnded));
        }

        public static Exception EnumNotStarted()
        {
            return new InvalidOperationException(SR.GetString(SR.InvalidOperation_EnumNotStarted));
        }

        public static Exception InvalidAsyncResult()
        {
            return new ArgumentException(SR.GetString(SR.AsyncResult_Invalid));
        }

        public static Exception InvalidOperation(string resourceCode)
        {
            return new InvalidOperationException(SR.GetString(resourceCode));
        }

        public static Exception IncompatibleVersion()
        {
            return new IOException(SR.GetString(SR.LogRecSeq_IncompatibleVersion));
        }

        public static Exception LogCorrupt()
        {
            return new IOException(SR.GetString(SR.IO_LogCorrupt));
        }

        public static Exception NotArchivable()
        {
            return new NotSupportedException(SR.GetString(SR.LogStore_NotArchivable));
        }

        public static Exception NotSupported()
        {
            return new NotSupportedException();
        }

        public static Exception NotSupported(string resource)
        {
            return new NotSupportedException(SR.GetString(resource));
        }

        public static Exception PlatformNotSupported()
        {
            return new PlatformNotSupportedException(SR.GetString(SR.NotSupported_Platform));
        }

        public static Exception ObjectDisposed()
        {
            return new ObjectDisposedException(null);
        }

        public static Exception ReservationNotFound()
        {
            return new ReservationNotFoundException();
        }

        public static Exception SequenceNumberInvalid()
        {
            return new ArgumentException(
                                    SR.GetString(
                                        SR.Argument_InvalidSequenceNumber));
        }

        public static Exception SequenceNumberNotActive(string paramName)
        {
            return new ArgumentOutOfRangeException(
                    paramName,
                    SR.GetString(
                        SR.Argument_SequenceNumberNotActive));
        }
    }
}
