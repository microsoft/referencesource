//---------------------------------------------------------------------
// <copyright file="Error.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static error utility functions
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System;

    /// <summary>
    ///    Strongly-typed and parameterized exception factory.
    /// </summary>
    internal static partial class Error
    {
        /// <summary>
        /// create and trace new ArgumentException
        /// </summary>
        /// <param name="message">exception message</param>
        /// <param name="parameterName">parameter name in error</param>
        /// <returns>ArgumentException</returns>
        internal static ArgumentException Argument(string message, string parameterName)
        {
            return Trace(new ArgumentException(message, parameterName));
        }

        /// <summary>
        /// create and trace new InvalidOperationException
        /// </summary>
        /// <param name="message">exception message</param>
        /// <returns>InvalidOperationException</returns>
        internal static InvalidOperationException InvalidOperation(string message)
        {
            return Trace(new InvalidOperationException(message));
        }

        /// <summary>
        /// create and trace new InvalidOperationException
        /// </summary>
        /// <param name="message">exception message</param>
        /// <param name="innerException">innerException</param>
        /// <returns>InvalidOperationException</returns>
        internal static InvalidOperationException InvalidOperation(string message, Exception innerException)
        {
            return Trace(new InvalidOperationException(message, innerException));
        }

        /// <summary>
        /// Create and trace a NotSupportedException with a message
        /// </summary>
        /// <param name="message">Message for the exception</param>
        /// <returns>NotSupportedException</returns>
        internal static NotSupportedException NotSupported(string message)
        {
            return Trace(new NotSupportedException(message));
        }

        /// <summary>
        /// create and throw a ThrowObjectDisposed with a type name
        /// </summary>
        /// <param name="type">type being thrown on</param>
        internal static void ThrowObjectDisposed(Type type)
        {
            throw Trace(new ObjectDisposedException(type.ToString()));
        }

        /// <summary>
        /// create and trace a 
        /// </summary>
        /// <param name="errorCode">errorCode</param>
        /// <param name="message">message</param>
        /// <returns>InvalidOperationException</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801", Justification = "errorCode ignored for code sharing")]
        internal static InvalidOperationException HttpHeaderFailure(int errorCode, string message)
        {
            return Trace(new InvalidOperationException(message));
        }

        /// <summary>create exception when missing an expected batch boundary</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingBoundary()
        {
            return InvalidOperation(Strings.BatchStream_MissingBoundary);
        }

        /// <summary>create exception when the expected content is missing</summary>
        /// <param name="state">http method operation</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamContentExpected(BatchStreamState state)
        {
            return InvalidOperation(Strings.BatchStream_ContentExpected(state.ToString()));
        }

        /// <summary>create exception when unexpected content is discovered</summary>
        /// <param name="state">http method operation</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamContentUnexpected(BatchStreamState state)
        {
            return InvalidOperation(Strings.BatchStream_ContentUnexpected(state.ToString()));
        }

        /// <summary>create exception when Get operation is specified in changeset</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamGetMethodNotSupportInChangeset()
        {
            return InvalidOperation(Strings.BatchStream_GetMethodNotSupportedInChangeset);
        }

        /// <summary>create exception when invalid batch request is specified</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidBatchFormat()
        {
            return InvalidOperation(Strings.BatchStream_InvalidBatchFormat);
        }

        /// <summary>create exception when boundary delimiter is not valid</summary>
        /// <param name="delimiter">boundary delimiter</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidDelimiter(string delimiter)
        {
            return InvalidOperation(Strings.BatchStream_InvalidDelimiter(delimiter));
        }

        /// <summary>create exception when end changeset boundary delimiter is missing</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingEndChangesetDelimiter()
        {
            return InvalidOperation(Strings.BatchStream_MissingEndChangesetDelimiter);
        }

        /// <summary>create exception when header value specified is not valid</summary>
        /// <param name="headerValue">headerValue</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidHeaderValueSpecified(string headerValue)
        {
            return InvalidOperation(Strings.BatchStream_InvalidHeaderValueSpecified(headerValue));
        }

        /// <summary>create exception when content length is not valid</summary>
        /// <param name="contentLength">contentLength</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidContentLengthSpecified(string contentLength)
        {
            return InvalidOperation(Strings.BatchStream_InvalidContentLengthSpecified(contentLength));
        }

        /// <summary>create exception when CUD operation is specified in batch</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamOnlyGETOperationsCanBeSpecifiedInBatch()
        {
            return InvalidOperation(Strings.BatchStream_OnlyGETOperationsCanBeSpecifiedInBatch);
        }

        /// <summary>create exception when operation header is specified in start of changeset</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidOperationHeaderSpecified()
        {
            return InvalidOperation(Strings.BatchStream_InvalidOperationHeaderSpecified);
        }

        /// <summary>create exception when http method name is not valid</summary>
        /// <param name="methodName">methodName</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidHttpMethodName(string methodName)
        {
            return InvalidOperation(Strings.BatchStream_InvalidHttpMethodName(methodName));
        }

        /// <summary>create exception when more data is specified after end of batch delimiter.</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMoreDataAfterEndOfBatch()
        {
            return InvalidOperation(Strings.BatchStream_MoreDataAfterEndOfBatch);
        }

        /// <summary>internal error where batch stream does do look ahead when read request is smaller than boundary delimiter</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInternalBufferRequestTooSmall()
        {
            return InvalidOperation(Strings.BatchStream_InternalBufferRequestTooSmall);
        }

        /// <summary>method not supported</summary>
        /// <param name="m">method</param>
        /// <returns>exception to throw</returns>
        internal static NotSupportedException MethodNotSupported(System.Linq.Expressions.MethodCallExpression m)
        {
            return Error.NotSupported(Strings.ALinq_MethodNotSupported(m.Method.Name));
        }

        /// <summary>throw an exception because unexpected batch content was encounted</summary>
        /// <param name="value">internal error</param>
        internal static void ThrowBatchUnexpectedContent(InternalError value)
        {
            throw InvalidOperation(Strings.Batch_UnexpectedContent((int)value));
        }

        /// <summary>throw an exception because expected batch content was not encountered</summary>
        /// <param name="value">internal error</param>
        internal static void ThrowBatchExpectedResponse(InternalError value)
        {
            throw InvalidOperation(Strings.Batch_ExpectedResponse((int)value));
        }

        /// <summary>internal error where the first request header is not of the form: 'MethodName' 'Url' 'Version'</summary>
        /// <param name="header">actual header value specified in the payload.</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidMethodHeaderSpecified(string header)
        {
            return InvalidOperation(Strings.BatchStream_InvalidMethodHeaderSpecified(header));
        }

        /// <summary>internal error when http version in batching request is not valid</summary>
        /// <param name="actualVersion">actual version as specified in the payload.</param>
        /// <param name="expectedVersion">expected version value.</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidHttpVersionSpecified(string actualVersion, string expectedVersion)
        {
            return InvalidOperation(Strings.BatchStream_InvalidHttpVersionSpecified(actualVersion, expectedVersion));
        }

        /// <summary>internal error when number of headers at the start of each operation is not 2</summary>
        /// <param name="header1">valid header name</param>
        /// <param name="header2">valid header name</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidNumberOfHeadersAtOperationStart(string header1, string header2)
        {
            return InvalidOperation(Strings.BatchStream_InvalidNumberOfHeadersAtOperationStart(header1, header2));
        }

        /// <summary>internal error Content-Transfer-Encoding is not specified or its value is not 'binary'</summary>
        /// <param name="headerName">name of the header</param>
        /// <param name="headerValue">expected value of the header</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingOrInvalidContentEncodingHeader(string headerName, string headerValue)
        {
            return InvalidOperation(Strings.BatchStream_MissingOrInvalidContentEncodingHeader(headerName, headerValue));
        }

        /// <summary>internal error number of headers at the start of changeset is not correct</summary>
        /// <param name="header1">valid header name</param>
        /// <param name="header2">valid header name</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidNumberOfHeadersAtChangeSetStart(string header1, string header2)
        {
            return InvalidOperation(Strings.BatchStream_InvalidNumberOfHeadersAtChangeSetStart(header1, header2));
        }

        /// <summary>internal error when content type header is missing</summary>
        /// <param name="headerName">name of the missing header</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingContentTypeHeader(string headerName)
        {
            return InvalidOperation(Strings.BatchStream_MissingContentTypeHeader(headerName));
        }

        /// <summary>internal error when content type header value is invalid.</summary>
        /// <param name="headerName">name of the header whose value is not correct.</param>
        /// <param name="headerValue">actual value as specified in the payload</param>
        /// <param name="mime1">expected value 1</param>
        /// <param name="mime2">expected value 2</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidContentTypeSpecified(string headerName, string headerValue, string mime1, string mime2)
        {
            return InvalidOperation(Strings.BatchStream_InvalidContentTypeSpecified(headerName, headerValue, mime1, mime2));
        }

        /// <summary>unexpected xml when reading web responses</summary>
        /// <param name="value">internal error</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException InternalError(InternalError value)
        {
            return InvalidOperation(Strings.Context_InternalError((int)value));
        }

        /// <summary>throw exception for unexpected xml when reading web responses</summary>
        /// <param name="value">internal error</param>
        internal static void ThrowInternalError(InternalError value)
        {
            throw InternalError(value);
        }

        /// <summary>
        /// Trace the exception
        /// </summary>
        /// <typeparam name="T">type of the exception</typeparam>
        /// <param name="exception">exception object to trace</param>
        /// <returns>the exception parameter</returns>
        private static T Trace<T>(T exception) where T : Exception
        {
            return exception;
        }
    }

    /// <summary>unique numbers for repeated error messages for unlikely, unactionable exceptions</summary>
    internal enum InternalError
    {
        UnexpectedXmlNodeTypeWhenReading = 1,
        UnexpectedXmlNodeTypeWhenSkipping = 2,
        UnexpectedEndWhenSkipping = 3,
        UnexpectedReadState = 4,
        UnexpectedRequestBufferSizeTooSmall = 5,
        UnvalidatedEntityState = 6,
        NullResponseStream = 7,
        EntityNotDeleted = 8,
        EntityNotAddedState = 9,
        LinkNotAddedState = 10,
        EntryNotModified = 11,
        LinkBadState = 12,
        UnexpectedBeginChangeSet = 13,
        UnexpectedBatchState = 14,
        ChangeResponseMissingContentID = 15,
        ChangeResponseUnknownContentID = 16,
        TooManyBatchResponse = 17,

        InvalidEndGetRequestStream = 20,
        InvalidEndGetRequestCompleted = 21,
        InvalidEndGetRequestStreamRequest = 22,
        InvalidEndGetRequestStreamStream = 23,
        InvalidEndGetRequestStreamContent = 24,
        InvalidEndGetRequestStreamContentLength = 25,

        InvalidEndWrite = 30,
        InvalidEndWriteCompleted = 31,
        InvalidEndWriteRequest = 32,
        InvalidEndWriteStream = 33,

        InvalidEndGetResponse = 40,
        InvalidEndGetResponseCompleted = 41,
        InvalidEndGetResponseRequest = 42,
        InvalidEndGetResponseResponse = 43,
        InvalidAsyncResponseStreamCopy = 44,
        InvalidAsyncResponseStreamCopyBuffer = 45,

        InvalidEndRead = 50,
        InvalidEndReadCompleted = 51,
        InvalidEndReadStream = 52,
        InvalidEndReadCopy = 53,
        InvalidEndReadBuffer = 54,

        InvalidSaveNextChange = 60,
        InvalidBeginNextChange = 61,
        SaveNextChangeIncomplete = 62,

        InvalidGetRequestStream = 70,
        InvalidGetResponse = 71,
    }
}
