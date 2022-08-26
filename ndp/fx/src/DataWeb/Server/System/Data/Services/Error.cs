//---------------------------------------------------------------------
// <copyright file="Error.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static error utility functions
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services
{
    using System;

    /// <summary>
    ///    Strongly-typed and parameterized exception factory.
    /// </summary>
    internal static partial class Error
    {
        /// <summary>
        /// create and trace a HttpHeaderFailure
        /// </summary>
        /// <param name="errorCode">error code</param>
        /// <param name="message">error message</param>
        /// <returns>DataServiceException</returns>
        internal static DataServiceException HttpHeaderFailure(int errorCode, string message)
        {
            return Trace(new DataServiceException(errorCode, message));
        }

        /// <summary>create exception when missing an expected batch boundary</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamMissingBoundary()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.DataService_InvalidContentTypeForBatchRequest));
        }

        /// <summary>create exception when the expected content is missing</summary>
        /// <param name="state">http method operation</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamContentExpected(BatchStreamState state)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_ContentExpected(state.ToString())));
        }

        /// <summary>create exception when unexpected content is discovered</summary>
        /// <param name="state">http method operation</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamContentUnexpected(BatchStreamState state)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_ContentUnexpected(state.ToString())));
        }

        /// <summary>create exception when Get operation is specified in changeset</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamGetMethodNotSupportInChangeset()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_GetMethodNotSupportedInChangeset));
        }

        /// <summary>create exception when invalid batch request is specified</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidBatchFormat()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidBatchFormat));
        }

        /// <summary>create exception when boundary delimiter is not valid</summary>
        /// <param name="delimiter">delimiter specified as specified in the request.</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidDelimiter(string delimiter)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidDelimiter(delimiter)));
        }

        /// <summary>create exception when end changeset boundary delimiter is missing</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamMissingEndChangesetDelimiter()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_MissingEndChangesetDelimiter));
        }

        /// <summary>create exception when header value specified is not valid</summary>
        /// <param name="headerValue">header value as specified in the request.</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidHeaderValueSpecified(string headerValue)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidHeaderValueSpecified(headerValue)));
        }

        /// <summary>create exception when content length is not valid</summary>
        /// <param name="contentLength">content length as specified in the request.</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidContentLengthSpecified(string contentLength)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidContentLengthSpecified(contentLength)));
        }

        /// <summary>create exception when CUD operation is specified in batch</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamOnlyGETOperationsCanBeSpecifiedInBatch()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_OnlyGETOperationsCanBeSpecifiedInBatch));
        }

        /// <summary>create exception when operation header is specified in start of changeset</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidOperationHeaderSpecified()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidOperationHeaderSpecified));
        }

        /// <summary>create exception when http method name is not valid</summary>
        /// <param name="methodName">name of the http method as specified in the request.</param>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamInvalidHttpMethodName(string methodName)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidHttpMethodName(methodName)));
        }

        /// <summary>create exception when more data is specified after end of batch delimiter.</summary>
        /// <returns>exception to throw</returns>
        internal static DataServiceException BatchStreamMoreDataAfterEndOfBatch()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_MoreDataAfterEndOfBatch));
        }

        /// <summary>internal error where batch stream does do look ahead when read request is smaller than boundary delimiter</summary>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInternalBufferRequestTooSmall()
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InternalBufferRequestTooSmall));
        }

        /// <summary>internal error where the first request header is not of the form: 'MethodName' 'Url' 'Version'</summary>
        /// <param name="header">actual header value specified in the payload.</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidMethodHeaderSpecified(string header)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidMethodHeaderSpecified(header)));
        }

        /// <summary>internal error when http version in batching request is not valid</summary>
        /// <param name="actualVersion">actual version as specified in the payload.</param>
        /// <param name="expectedVersion">expected version value.</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidHttpVersionSpecified(string actualVersion, string expectedVersion)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidHttpVersionSpecified(actualVersion, expectedVersion)));
        }

        /// <summary>internal error when number of headers at the start of each operation is not 2</summary>
        /// <param name="header1">First valid header name</param>
        /// <param name="header2">Second valid header name</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidNumberOfHeadersAtOperationStart(string header1, string header2)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidNumberOfHeadersAtOperationStart(header1, header2)));
        }

        /// <summary>internal error Content-Transfer-Encoding is not specified or its value is not 'binary'</summary>
        /// <param name="headerName">name of the header</param>
        /// <param name="headerValue">expected value of the header</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingOrInvalidContentEncodingHeader(string headerName, string headerValue)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_MissingOrInvalidContentEncodingHeader(headerName, headerValue)));
        }

        /// <summary>internal error number of headers at the start of changeset is not correct</summary>
        /// <param name="header1">First valid header name</param>
        /// <param name="header2">Second valid header name</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidNumberOfHeadersAtChangeSetStart(string header1, string header2)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidNumberOfHeadersAtChangeSetStart(header1, header2)));
        }

        /// <summary>internal error when content type header is missing</summary>
        /// <param name="headerName">name of the missing header</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamMissingContentTypeHeader(string headerName)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_MissingContentTypeHeader(headerName)));
        }
        
        /// <summary>internal error when content type header value is invalid.</summary>
        /// <param name="headerName">name of the header whose value is not correct.</param>
        /// <param name="headerValue">actual value as specified in the payload</param>
        /// <param name="mime1">expected value 1</param>
        /// <param name="mime2">expected value 2</param>
        /// <returns>exception to throw</returns>
        internal static InvalidOperationException BatchStreamInvalidContentTypeSpecified(string headerName, string headerValue, string mime1, string mime2)
        {
            return Trace(DataServiceException.CreateBadRequestError(Strings.BatchStream_InvalidContentTypeSpecified(headerName, headerValue, mime1, mime2)));
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
}
