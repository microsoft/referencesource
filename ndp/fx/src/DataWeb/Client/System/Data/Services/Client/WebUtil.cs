//---------------------------------------------------------------------
// <copyright file="WebUtil.cs" company="Microsoft">
//      Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
// <summary>
// static utility functions
// </summary>
//---------------------------------------------------------------------

namespace System.Data.Services.Client
{
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Reflection;
    using System.Runtime.CompilerServices;
#if !ASTORIA_LIGHT // Data.Services http stack
    using System.Net;
#else
    using System.Data.Services.Http;
#endif

    /// <summary>web utility functions</summary>
    internal static partial class WebUtil
    {
        /// <summary>
        /// Whether DataServiceCollection&lt;&gt; type is available. 
        /// </summary>
        private static bool? dataServiceCollectionAvailable = null;

        /// <summary>
        /// Returns true if DataServiceCollection&lt;&gt; type is available or false otherwise.
        /// </summary>
        private static bool DataServiceCollectionAvailable
        {
            get
            {
                if (dataServiceCollectionAvailable == null)
                {
                    try
                    {
                        dataServiceCollectionAvailable = GetDataServiceCollectionOfTType() != null;
                    }
                    catch (FileNotFoundException)
                    {
                        // the assembly or one of its dependencies (read: WindowsBase.dll) was not found. DataServiceCollection is not available.
                        dataServiceCollectionAvailable = false;
                    }
                }

                Debug.Assert(dataServiceCollectionAvailable != null, "observableCollectionOfTAvailable must not be null here.");

                return (bool)dataServiceCollectionAvailable;
            }
        }

        /// <summary>copy from one stream to another</summary>
        /// <param name="input">input stream</param>
        /// <param name="output">output stream</param>
        /// <param name="refBuffer">reusable buffer</param>
        /// <returns>count of copied bytes</returns>
        internal static long CopyStream(Stream input, Stream output, ref byte[] refBuffer)
        {
            Debug.Assert(null != input, "null input stream");
            Debug.Assert(null != output, "null output stream");

            long total = 0;
            byte[] buffer = refBuffer;
            if (null == buffer)
            {
                refBuffer = buffer = new byte[1000];
            }

            int count = 0;
            while (input.CanRead && (0 < (count = input.Read(buffer, 0, buffer.Length))))
            {
                output.Write(buffer, 0, count);
                total += count;
            }

            return total;
        }

        /// <summary>get response object from possible WebException</summary>
        /// <param name="exception">exception to probe</param>
        /// <param name="response">http web respose object from exception</param>
        internal static void GetHttpWebResponse(InvalidOperationException exception, ref HttpWebResponse response)
        {
            if (null == response)
            {
                WebException webexception = (exception as WebException);
                if (null != webexception)
                {
                    response = (HttpWebResponse)webexception.Response;
                }
            }
        }

        /// <summary>is this a success status code</summary>
        /// <param name="status">status code</param>
        /// <returns>true if status is between 200-299</returns>
        internal static bool SuccessStatusCode(HttpStatusCode status)
        {
            return (200 <= (int)status && (int)status < 300);
        }

        /// <summary>
        /// turn the response object headers into a dictionary
        /// </summary>
        /// <param name="response">response</param>
        /// <returns>dictionary</returns>
        internal static Dictionary<string, string> WrapResponseHeaders(HttpWebResponse response)
        {
            Dictionary<string, string> headers = new Dictionary<string, string>(EqualityComparer<string>.Default);
            if (null != response)
            {
                foreach (string name in response.Headers.AllKeys)
                {
                    headers.Add(name, response.Headers[name]);
                }
            }

            return headers;
        }

        /// <summary>
        /// Applies headers in the dictionary to a web request.
        /// This has a special case for some of the headers to set the properties on the request instead of using the Headers collection.
        /// </summary>
        /// <param name="headers">The dictionary with the headers to apply.</param>
        /// <param name="request">The request to apply the headers to.</param>
        /// <param name="ignoreAcceptHeader">If set to true the Accept header will be ignored 
        /// and the request.Accept property will not be touched.</param>
        internal static void ApplyHeadersToRequest(Dictionary<string, string> headers, HttpWebRequest request, bool ignoreAcceptHeader)
        {
            foreach (KeyValuePair<string, string> header in headers)
            {
                if (string.Equals(header.Key, XmlConstants.HttpRequestAccept, StringComparison.Ordinal))
                {
                    if (!ignoreAcceptHeader)
                    {
                        request.Accept = header.Value;
                    }
                }
                else if (string.Equals(header.Key, XmlConstants.HttpContentType, StringComparison.Ordinal))
                {
                    request.ContentType = header.Value;
                }
                else
                {
                    request.Headers[header.Key] = header.Value;
                }
            }
        }

        /// <summary>
        /// Checks if the given type is DataServiceCollection&lt;&gt; type.
        /// </summary>
        /// <param name="t">Type to be checked.</param>
        /// <returns>true if the provided type is DataServiceCollection&lt;&gt; or false otherwise.</returns>
        internal static bool IsDataServiceCollectionType(Type t)
        {
            if (DataServiceCollectionAvailable)
            {
                return t == GetDataServiceCollectionOfTType();
            }

            return false;
        }

        /// <summary>
        /// Creates an instance of DataServiceCollection&lt;&gt; class using provided types.
        /// </summary>
        /// <param name="typeArguments">Types to be used for creating DataServiceCollection&lt;&gt; object.</param>
        /// <returns>
        /// Instance of DataServiceCollection&lt;&gt; class created using provided types or null if DataServiceCollection&lt;&gt;
        /// type is not avaiable.
        /// </returns>
        internal static Type GetDataServiceCollectionOfT(params Type[] typeArguments)
        {
            if (DataServiceCollectionAvailable)
            {
                Debug.Assert(
                    GetDataServiceCollectionOfTType() != null, 
                    "DataServiceCollection is available so GetDataServiceCollectionOfTType() must not return null.");
                
                return GetDataServiceCollectionOfTType().MakeGenericType(typeArguments);
            }

            return null;
        }

        /// <summary>
        /// Forces loading WindowsBase assembly. If WindowsBase assembly is not present JITter will throw an exception. 
        /// This method MUST NOT be inlined otherwise we won't be able to catch the exception by JITter in the caller.
        /// </summary>
        /// <returns>typeof(DataServiceCollection&lt;&gt;)</returns>
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static Type GetDataServiceCollectionOfTType()
        {
            return typeof(DataServiceCollection<>);
        }
    }
}
