using System;
using System.ComponentModel;
using System.IO;
using System.Net;
using System.Text;
using System.Threading.Tasks;

/// <summary>
///     Provides asynchronous wrappers for .NET Framework operations.
/// </summary>
public static partial class AsyncPlatformExtensions
{
    /// <summary>Downloads the resource with the specified URI as a string, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI from which to download data.</param>
    /// <returns>A Task that contains the downloaded string.</returns>
    public static Task<string> DownloadStringTaskAsync(this WebClient webClient, string address)
    {
        return DownloadStringTaskAsync(webClient, webClient.GetUri(address));
    }

    /// <summary>Downloads the resource with the specified URI as a string, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI from which to download data.</param>
    /// <returns>A Task that contains the downloaded string.</returns>
    public static Task<string> DownloadStringTaskAsync(this WebClient webClient, Uri address)
    {
        // Create the task to be returned
        var tcs = new TaskCompletionSource<string>(address);

        // Setup the callback event handler
        DownloadStringCompletedEventHandler completedHandler = null;
        completedHandler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.DownloadStringCompleted -= completedHandler);
        webClient.DownloadStringCompleted += completedHandler;

        // Start the async operation.
        try
        {
            webClient.DownloadStringAsync(address, tcs);
        }
        catch
        {
            webClient.DownloadStringCompleted -= completedHandler;
            throw;
        }

        // Return the task that represents the async operation
        return tcs.Task;
    }

    /// <summary>Opens a readable stream for the data downloaded from a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenReadTaskAsync(this WebClient webClient, string address)
    {
        return OpenReadTaskAsync(webClient, webClient.GetUri(address));
    }

    /// <summary>Opens a readable stream for the data downloaded from a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenReadTaskAsync(this WebClient webClient, Uri address)
    {
        // Create the task to be returned
        var tcs = new TaskCompletionSource<Stream>(address);

        // Setup the callback event handler
        OpenReadCompletedEventHandler handler = null;
        handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.OpenReadCompleted -= handler);
        webClient.OpenReadCompleted += handler;

        // Start the async operation.
        try { webClient.OpenReadAsync(address, tcs); }
        catch
        {
            webClient.OpenReadCompleted -= handler;
            throw;
        }

        // Return the task that represents the async operation
        return tcs.Task;
    }

    /// <summary>Opens a writeable stream for uploading data to a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenWriteTaskAsync(this WebClient webClient, string address)
    {
        return OpenWriteTaskAsync(webClient, webClient.GetUri(address), null);
    }

    /// <summary>Opens a writeable stream for uploading data to a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenWriteTaskAsync(this WebClient webClient, Uri address)
    {
        return OpenWriteTaskAsync(webClient, address, null);
    }

    /// <summary>Opens a writeable stream for uploading data to a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <param name="method">The HTTP method that should be used to open the stream.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenWriteTaskAsync(this WebClient webClient, string address, string method)
    {
        return OpenWriteTaskAsync(webClient, webClient.GetUri(address), method);
    }

    /// <summary>Opens a writeable stream for uploading data to a resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI for which the stream should be opened.</param>
    /// <param name="method">The HTTP method that should be used to open the stream.</param>
    /// <returns>A Task that contains the opened stream.</returns>
    public static Task<Stream> OpenWriteTaskAsync(this WebClient webClient, Uri address, string method)
    {
        // Create the task to be returned
        var tcs = new TaskCompletionSource<Stream>(address);

        // Setup the callback event handler
        OpenWriteCompletedEventHandler handler = null;
        handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.OpenWriteCompleted -= handler);
        webClient.OpenWriteCompleted += handler;

        // Start the async operation.
        try { webClient.OpenWriteAsync(address, method, tcs); }
        catch
        {
            webClient.OpenWriteCompleted -= handler;
            throw;
        }

        // Return the task that represents the async operation
        return tcs.Task;
    }

    /// <summary>Uploads data in a string to the specified resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI to which the data should be uploaded.</param>
    /// <param name="data">The data to upload.</param>
    /// <returns>A Task containing the data in the response from the upload.</returns>
    public static Task<string> UploadStringTaskAsync(this WebClient webClient, string address, string data)
    {
        return UploadStringTaskAsync(webClient, address, null, data);
    }

    /// <summary>Uploads data in a string to the specified resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI to which the data should be uploaded.</param>
    /// <param name="data">The data to upload.</param>
    /// <returns>A Task containing the data in the response from the upload.</returns>
    public static Task<string> UploadStringTaskAsync(this WebClient webClient, Uri address, string data)
    {
        return UploadStringTaskAsync(webClient, address, null, data);
    }

    /// <summary>Uploads data in a string to the specified resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI to which the data should be uploaded.</param>
    /// <param name="method">The HTTP method that should be used to upload the data.</param>
    /// <param name="data">The data to upload.</param>
    /// <returns>A Task containing the data in the response from the upload.</returns>
    public static Task<string> UploadStringTaskAsync(this WebClient webClient, string address, string method, string data)
    {
        return UploadStringTaskAsync(webClient, webClient.GetUri(address), method, data);
    }

    /// <summary>Uploads data in a string to the specified resource, asynchronously.</summary>
    /// <param name="webClient">The WebClient.</param>
    /// <param name="address">The URI to which the data should be uploaded.</param>
    /// <param name="method">The HTTP method that should be used to upload the data.</param>
    /// <param name="data">The data to upload.</param>
    /// <returns>A Task containing the data in the response from the upload.</returns>
    public static Task<string> UploadStringTaskAsync(this WebClient webClient, Uri address, string method, string data)
    {
        // Create the task to be returned
        var tcs = new TaskCompletionSource<string>(address);

        // Setup the callback event handler
        UploadStringCompletedEventHandler handler = null;
        handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.UploadStringCompleted -= handler);
        webClient.UploadStringCompleted += handler;

        // Start the async operation.
        try { webClient.UploadStringAsync(address, method, data, tcs); }
        catch
        {
            webClient.UploadStringCompleted -= handler;
            throw;
        }

        // Return the task that represents the async operation
        return tcs.Task;
    }

    /// <summary>Converts a path to a Uri using the WebClient's logic.</summary>
    /// <remarks>Based on WebClient's private GetUri method.</remarks>
    private static Uri GetUri(this WebClient webClient, string path)
    {
        Uri uri;
        var baseAddress = webClient.BaseAddress;
        if (!string.IsNullOrEmpty(baseAddress))
        {
            if (!Uri.TryCreate(new Uri(baseAddress), path, out uri))
            {
                return new Uri(path);

            }
        }
        else if (!Uri.TryCreate(path, UriKind.Absolute, out uri))
        {
            return new Uri(path);
        }
        return GetUri(webClient, uri);
    }

#if SILVERLIGHT
    /// <summary>Converts a path to a Uri using the WebClient's logic.</summary>
    /// <remarks>Based on WebClient's private GetUri method.</remarks>
    private static Uri GetUri(this WebClient webClient, Uri address)
    {
        if (address == null) throw new ArgumentNullException("address");
        Uri result = address;
        if ((!address.IsAbsoluteUri && (!string.IsNullOrEmpty(webClient.BaseAddress))) &&
            !Uri.TryCreate(new Uri(webClient.BaseAddress), address, out result))
        {
            return address;
        }
        return result;
    }
#else
        /// <summary>Converts a path to a Uri using the WebClient's logic.</summary>
        /// <remarks>Based on WebClient's private GetUri method.</remarks>
        private static Uri GetUri(this WebClient webClient, Uri address)
        {
            if (address == null) throw new ArgumentNullException("address");

            Uri result = address;
            var baseAddress = webClient.BaseAddress;
            if ((!address.IsAbsoluteUri && (!string.IsNullOrEmpty(baseAddress))) && !Uri.TryCreate(webClient.GetUri(baseAddress), address, out result))
            {
                return address;
            }
            if ((result.Query != null) && !(result.Query == string.Empty))
            {
                return result;
            }

            var builder = new StringBuilder();
            string str = string.Empty;
            var queryString = webClient.QueryString;
            for (int i = 0; i < queryString.Count; i++)
            {
                builder.Append(str + queryString.AllKeys[i] + "=" + queryString[i]);
                str = "&";
            }
            return new UriBuilder(result) { Query = builder.ToString() }.Uri;
        }
#endif

#if !SILVERLIGHT
        /// <summary>Downloads the resource with the specified URI as a byte array, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI from which to download data.</param>
        /// <returns>A Task that contains the downloaded data.</returns>
        public static Task<byte[]> DownloadDataTaskAsync(this WebClient webClient, string address)
        {
            return DownloadDataTaskAsync(webClient, webClient.GetUri(address));
        }

        /// <summary>Downloads the resource with the specified URI as a byte array, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI from which to download data.</param>
        /// <returns>A Task that contains the downloaded data.</returns>
        public static Task<byte[]> DownloadDataTaskAsync(this WebClient webClient, Uri address)
        {
            // Create the task to be returned
            var tcs = new TaskCompletionSource<byte[]>(address);

            // Setup the callback event handler
            DownloadDataCompletedEventHandler completedHandler = null;
            completedHandler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.DownloadDataCompleted -= completedHandler);
            webClient.DownloadDataCompleted += completedHandler;

            // Start the async operation.
            try
            {
                webClient.DownloadDataAsync(address, tcs);
            }
            catch
            {
                webClient.DownloadDataCompleted -= completedHandler;
                throw;
            }

            // Return the task that represents the async operation
            return tcs.Task;
        }

        /// <summary>Downloads the resource with the specified URI to a local file, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI from which to download data.</param>
        /// <param name="fileName">The name of the local file that is to receive the data.</param>
        /// <returns>A Task that contains the downloaded data.</returns>
        public static Task DownloadFileTaskAsync(this WebClient webClient, string address, string fileName)
        {
            return DownloadFileTaskAsync(webClient, webClient.GetUri(address), fileName);
        }

        /// <summary>Downloads the resource with the specified URI to a local file, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI from which to download data.</param>
        /// <param name="fileName">The name of the local file that is to receive the data.</param>
        /// <returns>A Task that contains the downloaded data.</returns>
        public static Task DownloadFileTaskAsync(this WebClient webClient, Uri address, string fileName)
        {
            // Create the task to be returned
            var tcs = new TaskCompletionSource<object>(address);

            // Setup the callback event handler
            AsyncCompletedEventHandler completedHandler = null;
            completedHandler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => null, () =>
            {
                webClient.DownloadFileCompleted -= completedHandler;
            });
            webClient.DownloadFileCompleted += completedHandler;

            // Start the async operation.
            try
            {
                webClient.DownloadFileAsync(address, fileName, tcs);
            }
            catch
            {
                webClient.DownloadFileCompleted -= completedHandler;
                throw;
            }

            // Return the task that represents the async operation
            return tcs.Task;
        }

        /// <summary>Uploads data to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the data should be uploaded.</param>
        /// <param name="data">The data to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadDataTaskAsync(this WebClient webClient, string address, byte[] data)
        {
            return UploadDataTaskAsync(webClient, webClient.GetUri(address), null, data);
        }

        /// <summary>Uploads data to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the data should be uploaded.</param>
        /// <param name="data">The data to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadDataTaskAsync(this WebClient webClient, Uri address, byte[] data)
        {
            return UploadDataTaskAsync(webClient, address, null, data);
        }

        /// <summary>Uploads data to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the data should be uploaded.</param>
        /// <param name="method">The HTTP method that should be used to upload the data.</param>
        /// <param name="data">The data to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadDataTaskAsync(this WebClient webClient, string address, string method, byte[] data)
        {
            return UploadDataTaskAsync(webClient, webClient.GetUri(address), method, data);
        }

        /// <summary>Uploads data to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the data should be uploaded.</param>
        /// <param name="method">The HTTP method that should be used to upload the data.</param>
        /// <param name="data">The data to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadDataTaskAsync(this WebClient webClient, Uri address, string method, byte[] data)
        {
            // Create the task to be returned
            var tcs = new TaskCompletionSource<byte[]>(address);

            // Setup the callback event handler
            UploadDataCompletedEventHandler handler = null;
            handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.UploadDataCompleted -= handler);
            webClient.UploadDataCompleted += handler;

            // Start the async operation.
            try { webClient.UploadDataAsync(address, method, data, tcs); }
            catch
            {
                webClient.UploadDataCompleted -= handler;
                throw;
            }

            // Return the task that represents the async operation
            return tcs.Task;
        }

        /// <summary>Uploads a file to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the file should be uploaded.</param>
        /// <param name="fileName">A path to the file to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadFileTaskAsync(this WebClient webClient, string address, string fileName)
        {
            return UploadFileTaskAsync(webClient, webClient.GetUri(address), null, fileName);
        }

        /// <summary>Uploads a file to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the file should be uploaded.</param>
        /// <param name="fileName">A path to the file to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadFileTaskAsync(this WebClient webClient, Uri address, string fileName)
        {
            return UploadFileTaskAsync(webClient, address, null, fileName);
        }

        /// <summary>Uploads a file to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the file should be uploaded.</param>
        /// <param name="method">The HTTP method that should be used to upload the file.</param>
        /// <param name="fileName">A path to the file to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadFileTaskAsync(this WebClient webClient, string address, string method, string fileName)
        {
            return UploadFileTaskAsync(webClient, webClient.GetUri(address), method, fileName);
        }

        /// <summary>Uploads a file to the specified resource, asynchronously.</summary>
        /// <param name="webClient">The WebClient.</param>
        /// <param name="address">The URI to which the file should be uploaded.</param>
        /// <param name="method">The HTTP method that should be used to upload the file.</param>
        /// <param name="fileName">A path to the file to upload.</param>
        /// <returns>A Task containing the data in the response from the upload.</returns>
        public static Task<byte[]> UploadFileTaskAsync(this WebClient webClient, Uri address, string method, string fileName)
        {
            // Create the task to be returned
            var tcs = new TaskCompletionSource<byte[]>(address);

            // Setup the callback event handler
            UploadFileCompletedEventHandler handler = null;
            handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => webClient.UploadFileCompleted -= handler);
            webClient.UploadFileCompleted += handler;

            // Start the async operation.
            try { webClient.UploadFileAsync(address, method, fileName, tcs); }
            catch
            {
                webClient.UploadFileCompleted -= handler;
                throw;
            }

            // Return the task that represents the async operation
            return tcs.Task;
        }
#endif
}
