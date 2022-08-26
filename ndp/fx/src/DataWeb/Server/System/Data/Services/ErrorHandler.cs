//---------------------------------------------------------------------
// <copyright file="ErrorHandler.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides some utility functions for this project
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services
{
    #region Namespaces.

    using System;
    using System.Data.Services.Serializers;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Xml;

    #endregion Namespaces.

    /// <summary>
    /// Provides support for orchestrating error handling at different points in the processing cycle and for
    /// serializing structured errors.
    /// </summary>
    internal class ErrorHandler
    {
        #region Private fields.

        /// <summary>Arguments for the exception being handled.</summary>
        private HandleExceptionArgs exceptionArgs;

        /// <summary>Encoding to created over stream; null if a higher-level writer will be provided.</summary>
        private Encoding encoding;

        #endregion Private fields.

        #region Constructors.

        /// <summary>Initializes a new <see cref="ErrorHandler"/> instance.</summary>
        /// <param name="args">Arguments for the exception being handled.</param>
        /// <param name="encoding">Encoding to created over stream; null if a higher-level writer will be provided.</param>
        private ErrorHandler(HandleExceptionArgs args, Encoding encoding)
        {
            Debug.Assert(args != null, "args != null");
            this.exceptionArgs = args;
            this.encoding = encoding;
        }

        #endregion Constructors.

        #region Internal methods.

        /// <summary>Handles an exception when processing a batch response.</summary>
        /// <param name='service'>Data service doing the processing.</param>
        /// <param name="host">host to which we need to write the exception message</param>
        /// <param name='exception'>Exception thrown.</param>
        /// <param name='writer'>Output writer for the batch.</param>
        internal static void HandleBatchProcessException(IDataService service, DataServiceHostWrapper host, Exception exception, StreamWriter writer)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(host != null, "host != null");
            Debug.Assert(exception != null, "exception != null");
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(service.Configuration != null, "service.Configuration != null");
            Debug.Assert(WebUtil.IsCatchableExceptionType(exception), "WebUtil.IsCatchableExceptionType(exception)");

            string contentType;
            Encoding encoding;
            TryGetResponseFormatForError(host, out contentType, out encoding);

            HandleExceptionArgs args = new HandleExceptionArgs(exception, false, contentType, service.Configuration.UseVerboseErrors);
            service.InternalHandleException(args);
            host.ResponseVersion = XmlConstants.DataServiceVersion1Dot0 + ";";
            host.ProcessException(args);

            writer.Flush();
            Action<Stream> errorWriter = ProcessBenignException(exception, service);
            if (errorWriter == null)
            {
                errorWriter = CreateErrorSerializer(args, encoding);
            }

            errorWriter(writer.BaseStream);
            writer.WriteLine();
        }

        /// <summary>Handles an exception when processing a batch request.</summary>
        /// <param name='service'>Data service doing the processing.</param>
        /// <param name='exception'>Exception thrown.</param>
        /// <param name='writer'>Output writer for the batch.</param>
        internal static void HandleBatchRequestException(IDataService service, Exception exception, StreamWriter writer)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(exception != null, "exception != null");
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(service.Configuration != null, "service.Configuration != null - it should have been initialized by now");
            Debug.Assert(WebUtil.IsCatchableExceptionType(exception), "WebUtil.IsCatchableExceptionType(exception) - ");

            string contentType;
            Encoding encoding;
            DataServiceHostWrapper host = service.OperationContext == null ? null : service.OperationContext.Host;
            TryGetResponseFormatForError(host, out contentType, out encoding);
            encoding = writer.Encoding;

            HandleExceptionArgs args = new HandleExceptionArgs(exception, false, contentType, service.Configuration.UseVerboseErrors);
            service.InternalHandleException(args);
            writer.Flush();

            Action<Stream> errorWriter = CreateErrorSerializer(args, encoding);
            errorWriter(writer.BaseStream);
            writer.WriteLine();
        }

        /// <summary>Handles an exception before the response has been written out.</summary>
        /// <param name='exception'>Exception thrown.</param>
        /// <param name='service'>Data service doing the processing.</param>
        /// <param name='accept'>'Accept' header value; possibly null.</param>
        /// <param name='acceptCharset'>'Accept-Charset' header; possibly null.</param>
        /// <returns>An action that can serialize the exception into a stream.</returns>
        internal static Action<Stream> HandleBeforeWritingException(Exception exception, IDataService service, string accept, string acceptCharset)
        {
            Debug.Assert(WebUtil.IsCatchableExceptionType(exception), "WebUtil.IsCatchableExceptionType(exception)");
            Debug.Assert(exception != null, "exception != null");
            Debug.Assert(service != null, "service != null");

            string contentType;
            Encoding encoding;
            TryGetResponseFormatForError(accept, acceptCharset, out contentType, out encoding);

            bool verbose = (service.Configuration != null) ? service.Configuration.UseVerboseErrors : false;
            HandleExceptionArgs args = new HandleExceptionArgs(exception, false, contentType, verbose);
            service.InternalHandleException(args);
            DataServiceHostWrapper host = service.OperationContext.Host;
            host.ResponseVersion = XmlConstants.DataServiceVersion1Dot0 + ";";
            host.ProcessException(args);
            Action<Stream> action = ProcessBenignException(exception, service);
            if (action != null)
            {
                return action;
            }
            else
            {
                return CreateErrorSerializer(args, encoding);
            }
        }

        /// <summary>Handles an exception while the response is being written out.</summary>
        /// <param name='exception'>Exception thrown.</param>
        /// <param name='service'>Data service doing the processing.</param>
        /// <param name='contentType'>MIME type of output stream.</param>
        /// <param name='exceptionWriter'>Serializer-specific exception writer.</param>
        internal static void HandleDuringWritingException(Exception exception, IDataService service, string contentType, IExceptionWriter exceptionWriter)
        {
            Debug.Assert(service != null, "service != null");
            Debug.Assert(exception != null, "exception != null");
            Debug.Assert(exceptionWriter != null, "exceptionWriter != null");
            Debug.Assert(service.Configuration != null, "service.Configuration != null");
            Debug.Assert(WebUtil.IsCatchableExceptionType(exception), "WebUtil.IsCatchableExceptionType(exception)");

            HandleExceptionArgs args = new HandleExceptionArgs(exception, true, contentType, service.Configuration.UseVerboseErrors);
            service.InternalHandleException(args);
            service.OperationContext.Host.ProcessException(args);
            exceptionWriter.WriteException(args);
        }

        /// <summary>Handles the specified <paramref name='exception'/>.</summary>
        /// <param name='exception'>Exception to handle</param>
        /// <remarks>The caller should re-throw the original exception if this method returns normally.</remarks>
        internal static void HandleTargetInvocationException(TargetInvocationException exception)
        {
            Debug.Assert(exception != null, "exception != null");

            DataServiceException dataException = exception.InnerException as DataServiceException;
            if (dataException == null)
            {
                return;
            }

            throw new DataServiceException(
                dataException.StatusCode,
                dataException.ErrorCode,
                dataException.Message,
                dataException.MessageLanguage,
                exception);
        }

        /// <summary>Serializes an error in JSON format.</summary>
        /// <param name='args'>Arguments describing the error.</param>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        internal static void SerializeJsonError(HandleExceptionArgs args, JsonWriter writer)
        {
            Debug.Assert(args != null, "args != null");
            Debug.Assert(writer != null, "writer != null");
            ErrorHandler serializer = new ErrorHandler(args, null);
            serializer.SerializeJsonError(writer);
        }

        /// <summary>Serializes an error in XML format.</summary>
        /// <param name='args'>Arguments describing the error.</param>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        internal static void SerializeXmlError(HandleExceptionArgs args, XmlWriter writer)
        {
            Debug.Assert(args != null, "args != null");
            Debug.Assert(writer != null, "writer != null");
            ErrorHandler serializer = new ErrorHandler(args, null);
            serializer.SerializeXmlError(writer);
        }

        #endregion Internal methods.

        #region Private methods.

        /// <summary>
        /// Check to see if the given excpetion is a benign one such as statusCode = 304.  If yes we return an action that can
        /// serialize the exception into a stream.  Other wise we return null.
        /// </summary>
        /// <param name="exception">Exception to be processed</param>
        /// <param name="service">Data service instance</param>
        /// <returns>An action that can serialize the exception into a stream.</returns>
        private static Action<Stream> ProcessBenignException(Exception exception, IDataService service)
        {
            DataServiceException dataServiceException = exception as DataServiceException;
            if (dataServiceException != null)
            {
                if (dataServiceException.StatusCode == (int)System.Net.HttpStatusCode.NotModified)
                {
                    DataServiceHostWrapper host = service.OperationContext.Host;
                    Debug.Assert(host != null, "host != null");
                    host.ResponseStatusCode = (int)System.Net.HttpStatusCode.NotModified;

                    // For 304, we MUST return an empty message-body.
                    return WebUtil.GetEmptyStreamWriter();
                }
            }

            return null;
        }

        /// <summary>Creates a delegate that can serialize an error for the specified arguments.</summary>
        /// <param name="args">Arguments for the exception being handled.</param>
        /// <param name="encoding">Encoding to created over stream.</param>
        /// <returns>A delegate that can serialize an error for the specified arguments.</returns>
        private static Action<Stream> CreateErrorSerializer(HandleExceptionArgs args, Encoding encoding)
        {
            Debug.Assert(args != null, "args != null");
            Debug.Assert(encoding != null, "encoding != null");
            ErrorHandler handler = new ErrorHandler(args, encoding);
            if (WebUtil.CompareMimeType(args.ResponseContentType, XmlConstants.MimeApplicationJson))
            {
                return handler.SerializeJsonErrorToStream;
            }
            else
            {
                return handler.SerializeXmlErrorToStream;
            }
        }

        /// <summary>
        /// Gets values describing the <paramref name='exception' /> if it's a DataServiceException;
        /// defaults otherwise.
        /// </summary>
        /// <param name='exception'>Exception to extract value from.</param>
        /// <param name='errorCode'>Error code from the <paramref name='exception' />; blank if not available.</param>
        /// <param name='message'>Message from the <paramref name='exception' />; blank if not available.</param>
        /// <param name='messageLang'>Message language from the <paramref name='exception' />; current default if not available.</param>
        /// <returns>The cast DataServiceException; possibly null.</returns>
        private static DataServiceException ExtractErrorValues(Exception exception, out string errorCode, out string message, out string messageLang)
        {
            DataServiceException dataException = exception as DataServiceException;
            if (dataException != null)
            {
                errorCode = dataException.ErrorCode ?? "";
                message = dataException.Message ?? "";
                messageLang = dataException.MessageLanguage ?? CultureInfo.CurrentCulture.Name;
                return dataException;
            }
            else
            {
                errorCode = "";
                message = Strings.DataServiceException_GeneralError;
                messageLang = CultureInfo.CurrentCulture.Name;
                return null;
            }
        }

        /// <summary>Serializes an exception in JSON format.</summary>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        /// <param name='exception'>Exception to serialize.</param>
        private static void SerializeJsonException(JsonWriter writer, Exception exception)
        {
            string elementName = XmlConstants.JsonErrorInner;
            int nestingDepth = 0;
            while (exception != null)
            {
                writer.WriteName(elementName);
                writer.StartObjectScope();
                nestingDepth++;

                string exceptionMessage = exception.Message ?? String.Empty;
                writer.WriteName(XmlConstants.JsonErrorMessage);
                writer.WriteValue(exceptionMessage);

                string exceptionType = exception.GetType().FullName;
                writer.WriteName(XmlConstants.JsonErrorType);
                writer.WriteValue(exceptionType);

                string exceptionStackTrace = exception.StackTrace ?? String.Empty;
                writer.WriteName(XmlConstants.JsonErrorStackTrace);
                writer.WriteValue(exceptionStackTrace);

                exception = exception.InnerException;
                elementName = XmlConstants.JsonErrorInternalException;
            }

            while (nestingDepth > 0)
            {
                writer.EndScope();  // </innererror>
                nestingDepth--;
            }
        }

        /// <summary>Serializes an exception in XML format.</summary>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        /// <param name='exception'>Exception to serialize.</param>
        private static void SerializeXmlException(XmlWriter writer, Exception exception)
        {
            string elementName = XmlConstants.XmlErrorInnerElementName;
            int nestingDepth = 0;
            while (exception != null)
            {
                // Inner Error Tag namespace changed to DataWebMetadataNamespace
                // Maybe DataWebNamespace should be used on all error tags? Up to debate...
                // NOTE: this is a breaking change from V1
                writer.WriteStartElement(elementName, XmlConstants.DataWebMetadataNamespace);
                nestingDepth++;

                string exceptionMessage = exception.Message ?? String.Empty;
                writer.WriteStartElement(XmlConstants.XmlErrorMessageElementName, XmlConstants.DataWebMetadataNamespace);
                writer.WriteString(exceptionMessage);
                writer.WriteEndElement();   // </message>

                string exceptionType = exception.GetType().FullName;
                writer.WriteStartElement(XmlConstants.XmlErrorTypeElementName, XmlConstants.DataWebMetadataNamespace);
                writer.WriteString(exceptionType);
                writer.WriteEndElement();   // </type>

                string exceptionStackTrace = exception.StackTrace ?? String.Empty;
                writer.WriteStartElement(XmlConstants.XmlErrorStackTraceElementName, XmlConstants.DataWebMetadataNamespace);
                writer.WriteString(exceptionStackTrace);
                writer.WriteEndElement();   // </stacktrace>

                exception = exception.InnerException;
                elementName = XmlConstants.XmlErrorInternalExceptionElementName;
            }

            while (nestingDepth > 0)
            {
                writer.WriteEndElement();   // </innererror>
                nestingDepth--;
            }
        }

        /// <summary>Gets content type and encoding information from the host if possible; defaults otherwise.</summary>
        /// <param name="host">Host to get headers from (possibly null).</param>
        /// <param name="contentType">After invocation, content type for the exception.</param>
        /// <param name="encoding">After invocation, encoding for the exception.</param>
        private static void TryGetResponseFormatForError(DataServiceHostWrapper host, out string contentType, out Encoding encoding)
        {
            TryGetResponseFormatForError(
                (host != null) ? host.RequestAccept : null,
                (host != null) ? host.RequestAcceptCharSet : null,
                out contentType,
                out encoding);
        }

        /// <summary>Gets content type and encoding information from the headers if possible; defaults otherwise.</summary>
        /// <param name="accept">A comma-separated list of client-supported MIME Accept types.</param>
        /// <param name="acceptCharset">The specification for the character set encoding that the client requested.</param>
        /// <param name="contentType">After invocation, content type for the exception.</param>
        /// <param name="encoding">After invocation, encoding for the exception.</param>
        private static void TryGetResponseFormatForError(string accept, string acceptCharset, out string contentType, out Encoding encoding)
        {
            contentType = null;
            encoding = null;
            if (accept != null)
            {
                try
                {
                    string[] availableTypes = new string[] { XmlConstants.MimeApplicationXml, XmlConstants.MimeApplicationJson };
                    contentType = HttpProcessUtility.SelectMimeType(accept, availableTypes);
                }
                catch (DataServiceException)
                {
                    // Ignore formatting erros in Accept and rely on text.
                }
            }

            if (acceptCharset != null)
            {
                try
                {
                    encoding = HttpProcessUtility.EncodingFromAcceptCharset(acceptCharset);
                }
                catch (DataServiceException)
                {
                    // Ignore formatting erros in Accept-Charset and rely on text.
                }
            }

            contentType = contentType ?? XmlConstants.MimeApplicationXml;
            encoding = encoding ?? HttpProcessUtility.FallbackEncoding;
        }

        /// <summary>Serializes an error in JSON format.</summary>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        private void SerializeJsonError(JsonWriter writer)
        {
            Debug.Assert(writer != null, "writer != null");
            writer.StartObjectScope();  // Wrapper for error.
            writer.WriteName(XmlConstants.JsonError);

            string errorCode, message, messageLang;
            DataServiceException dataException = ExtractErrorValues(this.exceptionArgs.Exception, out errorCode, out message, out messageLang);
            writer.StartObjectScope();

            writer.WriteName(XmlConstants.JsonErrorCode);
            writer.WriteValue(errorCode);

            writer.WriteName(XmlConstants.JsonErrorMessage);
            writer.StartObjectScope();
            writer.WriteName(XmlConstants.XmlLangAttributeName);
            writer.WriteValue(messageLang);
            writer.WriteName(XmlConstants.JsonErrorValue);
            writer.WriteValue(message);
            writer.EndScope();  // </message>

            if (this.exceptionArgs.UseVerboseErrors)
            {
                Exception exception = (dataException == null) ? this.exceptionArgs.Exception : dataException.InnerException;
                SerializeJsonException(writer, exception);
            }

            writer.EndScope();  // </error>
            writer.EndScope();  // </error wrapper>
            writer.Flush();
        }

        /// <summary>Serializes an error in XML format.</summary>
        /// <param name='writer'>Writer to which error should be serialized.</param>
        private void SerializeXmlError(XmlWriter writer)
        {
            Debug.Assert(writer != null, "writer != null");
            writer.WriteStartElement(XmlConstants.XmlErrorElementName, XmlConstants.DataWebMetadataNamespace);
            string errorCode, message, messageLang;
            DataServiceException dataException = ExtractErrorValues(this.exceptionArgs.Exception, out errorCode, out message, out messageLang);

            writer.WriteStartElement(XmlConstants.XmlErrorCodeElementName, XmlConstants.DataWebMetadataNamespace);
            writer.WriteString(errorCode);
            writer.WriteEndElement();   // </code>

            writer.WriteStartElement(XmlConstants.XmlErrorMessageElementName, XmlConstants.DataWebMetadataNamespace);
            writer.WriteAttributeString(
                XmlConstants.XmlNamespacePrefix,    // prefix
                XmlConstants.XmlLangAttributeName,  // localName
                null,                               // ns
                messageLang);                       // value
            writer.WriteString(message);
            writer.WriteEndElement();   // </message>

            if (this.exceptionArgs.UseVerboseErrors)
            {
                Exception exception = (dataException == null) ? this.exceptionArgs.Exception : dataException.InnerException;
                SerializeXmlException(writer, exception);
            }

            writer.WriteEndElement();   // </error>
            writer.Flush();
        }

        /// <summary>Serializes the current exception description to the specified <paramref name="stream"/>.</summary>
        /// <param name="stream">Stream to write to.</param>
        private void SerializeJsonErrorToStream(Stream stream)
        {
            Debug.Assert(stream != null, "stream != null");
            JsonWriter jsonWriter = new JsonWriter(new StreamWriter(stream, this.encoding));
            try
            {
                SerializeJsonError(jsonWriter);
            }
            finally
            {
                // We should not close the writer, since the stream is owned by the underlying host.
                jsonWriter.Flush();
            }
        }

        /// <summary>Serializes the current exception description to the specified <paramref name="stream"/>.</summary>
        /// <param name="stream">Stream to write to.</param>
        private void SerializeXmlErrorToStream(Stream stream)
        {
            Debug.Assert(stream != null, "stream != null");
            using (XmlWriter writer = XmlUtil.CreateXmlWriterAndWriteProcessingInstruction(stream, this.encoding))
            {
                SerializeXmlError(writer);
            }
        }

        #endregion Private methods.
    }
}
