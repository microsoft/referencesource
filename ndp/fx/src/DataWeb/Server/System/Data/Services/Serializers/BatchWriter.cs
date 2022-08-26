//---------------------------------------------------------------------
// <copyright file="BatchWriter.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
//      Provides a base class for DataWeb services.
// </summary>
//
// @owner  Microsoft
//---------------------------------------------------------------------

namespace System.Data.Services.Serializers
{
    #region Namespaces.

    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;

    #endregion Namespaces.

    /// <summary>
    /// Static helper class to write responses for batch requests
    /// </summary>
    internal static class BatchWriter
    {
        /// <summary>
        /// Writes the start of the changeset response
        /// </summary>
        /// <param name="writer">writer to which the response needs to be written</param>
        /// <param name="batchBoundary">batch boundary</param>
        /// <param name="changesetBoundary">changeset boundary</param>
        internal static void WriteStartBatchBoundary(StreamWriter writer, string batchBoundary, string changesetBoundary)
        {
            WriterStartBoundary(writer, batchBoundary);
            writer.WriteLine(
                "{0}: {1}; {2}={3}",
                XmlConstants.HttpContentType,
                XmlConstants.MimeMultiPartMixed,
                XmlConstants.HttpMultipartBoundary,
                changesetBoundary);
            writer.WriteLine(); // NewLine to seperate the header from message
        }

        /// <summary>Write the boundary and header information.</summary>
        /// <param name="writer">writer to which the response needs to be written</param>
        /// <param name="host">host containing the value of the response headers</param>
        /// <param name="contentId">content-id string that needs to be written</param>
        /// <param name="boundary">boundary string that needs to be written</param>
        internal static void WriteBoundaryAndHeaders(StreamWriter writer, IDataServiceHost2 host, string contentId, string boundary)
        {
            Debug.Assert(writer != null, "writer != null");
            Debug.Assert(host != null, "host != null");
            Debug.Assert(boundary != null, "boundary != null");
            WriterStartBoundary(writer, boundary);

            // First write the headers to indicate that the payload below is a http request
            WriteHeaderValue(writer, XmlConstants.HttpContentType, XmlConstants.MimeApplicationHttp);
            WriteHeaderValue(writer, XmlConstants.HttpContentTransferEncoding, XmlConstants.BatchRequestContentTransferEncoding);
            writer.WriteLine(); // NewLine to seperate the batch headers from http headers

            // In error cases, we create a dummy host, which has no request header information.
            // Hence we need to handle the case here.
            writer.WriteLine("{0} {1} {2}", XmlConstants.HttpVersionInBatching, host.ResponseStatusCode, WebUtil.GetStatusCodeText(host.ResponseStatusCode));

            if (null != contentId)
            {
                WriteHeaderValue(writer, XmlConstants.HttpContentID, contentId);
            }

            System.Net.WebHeaderCollection responseHeaders = host.ResponseHeaders;
            foreach (string header in responseHeaders.AllKeys)
            {
                WriteHeaderValue(writer, header, responseHeaders[header]);
            }

            writer.WriteLine(); // NewLine to seperate the header from message
        }

        /// <summary>
        /// Write the end boundary
        /// </summary>
        /// <param name="writer">writer to which the response needs to be written</param>
        /// <param name="boundary">end boundary string.</param>
        internal static void WriteEndBoundary(StreamWriter writer, string boundary)
        {
            writer.WriteLine("--{0}--", boundary);
        }

        /// <summary>
        /// Write the start boundary
        /// </summary>
        /// <param name="writer">writer to which the response needs to be written</param>
        /// <param name="boundary">boundary string.</param>
        private static void WriterStartBoundary(StreamWriter writer, string boundary)
        {
            writer.WriteLine("--{0}", boundary);
        }

        /// <summary>
        /// Write the header name and value
        /// </summary>
        /// <param name="writer">writer to which the response needs to be written</param>
        /// <param name="headerName">name of the header whose value needs to be written.</param>
        /// <param name="headerValue">value of the header that needs to be written.</param>
        private static void WriteHeaderValue(StreamWriter writer, string headerName, object headerValue)
        {
            if (headerValue != null)
            {
                string text = Convert.ToString(headerValue, System.Globalization.CultureInfo.InvariantCulture);
                if (!String.IsNullOrEmpty(text))
                {
                    writer.WriteLine("{0}: {1}", headerName, text);
                }
            }
        }
    }
}
