using System.Collections.Generic;
using System.Threading.Tasks;
using System.IO;
using System.Diagnostics.CodeAnalysis;
using System.Net.Http.Headers;
using System.Diagnostics.Contracts;
using System.Text;

namespace System.Net.Http
{
    [SuppressMessage("Microsoft.Naming", "CA1710:IdentifiersShouldHaveCorrectSuffix",
        Justification = "Represents a multipart/* content. Even if a collection of HttpContent is stored, " +
        "suffix Collection is not appropriate.")]
    public class MultipartContent : HttpContent, IEnumerable<HttpContent>
    {
        #region Fields

        private const string crlf = "\r\n";

        private List<HttpContent> nestedContent;
        private string boundary;

        // Temp context for serialization
        private int nextContentIndex;
        private Stream outputStream;
        private TaskCompletionSource<Object> tcs;

        #endregion Fields

        #region Construction

        public MultipartContent()
            : this("mixed", GetDefaultBoundary())
        { }

        public MultipartContent(string subtype)
            : this(subtype, GetDefaultBoundary())
        { }

        public MultipartContent(string subtype, string boundary)
        {
            if (string.IsNullOrWhiteSpace(subtype))
            {
                throw new ArgumentException(SR.net_http_argument_empty_string, "subtype");
            }
            Contract.EndContractBlock();
            ValidateBoundary(boundary);

            this.boundary = boundary;

            string quotedBoundary = boundary;
            if (!quotedBoundary.StartsWith("\"", StringComparison.Ordinal))
            {
                quotedBoundary = "\"" + quotedBoundary + "\""; // "boundary"
            }

            MediaTypeHeaderValue contentType = new MediaTypeHeaderValue("multipart/" + subtype);
            contentType.Parameters.Add(new NameValueHeaderValue("boundary", quotedBoundary));
            Headers.ContentType = contentType;

            nestedContent = new List<HttpContent>();
        }

        private static void ValidateBoundary(string boundary)
        {
            // NameValueHeaderValue is too restrictive for boundary.
            // Instead validate it ourselves and then quote it.

            // NameValueHeaderValue validates for Token or Quoted-String.  
            // Token will prevent a boundary from having the following valid characters: SP ( ) ? = : / ,
            //  and allow it to have the following invalid characters: ! # $ % & ^ ~ `
            // Quoted-String will let it have almost anything bounded by quotes.
            
            if (string.IsNullOrWhiteSpace(boundary))
            {
                throw new ArgumentException(SR.net_http_argument_empty_string, "boundary");
            }

            // RFC 2046 Section 5.1.1
            // boundary := 0*69<bchars> bcharsnospace
            // bchars := bcharsnospace / " "
            // bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" / "+" / "_" / "," / "-" / "." / "/" / ":" / "=" / "?"
            if (boundary.Length > 70)
            {
                throw new ArgumentOutOfRangeException("boundary", boundary,
                    string.Format(System.Globalization.CultureInfo.InvariantCulture, SR.net_http_content_field_too_long, 70));
            }
            // Cannot end with space
            if (boundary.EndsWith(" ", StringComparison.Ordinal))
            {
                throw new ArgumentException(string.Format(System.Globalization.CultureInfo.InvariantCulture, SR.net_http_headers_invalid_value, boundary), "boundary");
            }
            Contract.EndContractBlock();

            string allowedMarks = @"'()+_,-./:=? ";

            foreach (char ch in boundary)
            {
                if (('0' <= ch && ch <= '9') || // Digit
                    ('a' <= ch && ch <= 'z') || // alpha
                    ('A' <= ch && ch <= 'Z') || // ALPHA
                    (allowedMarks.IndexOf(ch) >= 0)) // Marks
                {
                    // Valid
                }
                else
                {
                    throw new ArgumentException(string.Format(System.Globalization.CultureInfo.InvariantCulture, SR.net_http_headers_invalid_value, boundary), "boundary");
                }
            }
        }

        private static string GetDefaultBoundary()
        {
            return Guid.NewGuid().ToString();
        }

        public virtual void Add(HttpContent content)
        {
            if (content == null)
            {
                throw new ArgumentNullException("content");
            }
            Contract.EndContractBlock();

            nestedContent.Add(content);
        }

        #endregion Construction

        #region Dispose

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                foreach (HttpContent content in nestedContent)
                {
                    content.Dispose();
                }
                nestedContent.Clear();
            }
            base.Dispose(disposing);
        }

        #endregion Dispose

        #region IEnumerable<HttpContent> Members

        public IEnumerator<HttpContent> GetEnumerator()
        {
            return nestedContent.GetEnumerator();
        }

        #endregion

        #region IEnumerable Members

        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return nestedContent.GetEnumerator();
        }

        #endregion

        #region Serialization
        
        // for-each content
        //   write "--" + boundary
        //   for-each content header
        //     write header: header-value
        //   write content.CopyTo[Async]
        // write "--" + boundary + "--"
        // Can't be canceled directly by the user.  If the overall request is canceled 
        // then the stream will be closed an an exception thrown.
        protected override Task SerializeToStreamAsync(Stream stream, TransportContext context)
        {
            Contract.Assert(stream != null);
            Contract.Assert(outputStream == null, "Opperation already in progress");
            Contract.Assert(tcs == null, "Opperation already in progress");
            Contract.Assert(nextContentIndex == 0, "Opperation already in progress");

            // Keep a local copy in case the operation completes and cleans up synchronously.
            TaskCompletionSource<Object> localTcs = new TaskCompletionSource<Object>();
            tcs = localTcs;
            outputStream = stream;
            nextContentIndex = 0;

            // Start Boundary, chain everything else
            EncodeStringToStreamAsync(outputStream, "--" + boundary + crlf)
                .ContinueWithStandard(WriteNextContentHeadersAsync);

            return localTcs.Task;
        }

        private void WriteNextContentHeadersAsync(Task task)
        {
            if (task.IsFaulted)
            {
                HandleAsyncException("WriteNextContentHeadersAsync", task.Exception.GetBaseException());
                return;
            }

            try
            {
                // Base case, no more content, finish
                if (nextContentIndex >= nestedContent.Count)
                {
                    WriteTerminatingBoundaryAsync();
                    return;
                }

                string internalBoundary = crlf + "--" + boundary + crlf;
                StringBuilder output = new StringBuilder();
                if (nextContentIndex == 0)
                {
                    // First time, don't write dividing boundary
                }
                else
                {
                    output.Append(internalBoundary);
                }

                HttpContent content = nestedContent[nextContentIndex];

                // Headers
                foreach (KeyValuePair<string, IEnumerable<string>> headerPair in content.Headers)
                {
                    output.Append(headerPair.Key + ": " + string.Join(", ", headerPair.Value) + crlf);
                }

                output.Append(crlf); // Extra CRLF to end headers (even if there are no headers)

                EncodeStringToStreamAsync(outputStream, output.ToString())
                    .ContinueWithStandard(WriteNextContentAsync);
            }
            catch (Exception ex)
            {
                HandleAsyncException("WriteNextContentHeadersAsync", ex);
            }
        }

        private void WriteNextContentAsync(Task task)
        {
            if (task.IsFaulted)
            {
                HandleAsyncException("WriteNextContentAsync", task.Exception.GetBaseException());
                return;
            }

            try
            {

                HttpContent content = nestedContent[nextContentIndex];
                nextContentIndex++; // Next call will operate on the next content object

                content.CopyToAsync(outputStream)
                    .ContinueWithStandard(WriteNextContentHeadersAsync);
            }
            catch (Exception ex)
            {
                HandleAsyncException("WriteNextContentAsync", ex);
            }
        }

        // Final step, write the footer boundary
        private void WriteTerminatingBoundaryAsync()
        {
            try
            {
                EncodeStringToStreamAsync(outputStream, crlf + "--" + boundary + "--" + crlf)
                    .ContinueWithStandard(task =>
                    {
                        if (task.IsFaulted)
                        {
                            HandleAsyncException("WriteTerminatingBoundaryAsync", task.Exception.GetBaseException());
                            return;
                        }

                        TaskCompletionSource<object> lastTcs = CleanupAsync();
                        lastTcs.TrySetResult(null); // This was the final opperation
                    });
            }
            catch (Exception ex)
            {
                HandleAsyncException("WriteTerminatingBoundaryAsync", ex);
            }
        }

        private static Task EncodeStringToStreamAsync(Stream stream, string input)
        {
            byte[] buffer = HttpRuleParser.DefaultHttpEncoding.GetBytes(input);
            return Task.Factory.FromAsync(stream.BeginWrite, stream.EndWrite, buffer, 0, buffer.Length, null);
        }

        private TaskCompletionSource<object> CleanupAsync()
        {
            Contract.Requires(tcs != null, "Operation already cleaned up");
            TaskCompletionSource<object> toReturn = tcs;
            outputStream = null;
            nextContentIndex = 0;
            tcs = null;
            return toReturn;
        }

        private void HandleAsyncException(string method, Exception ex)
        {
            if (Logging.On) Logging.Exception(Logging.Http, this, method, ex);
            TaskCompletionSource<object> lastTcs = CleanupAsync();
            lastTcs.TrySetException(ex);
        }

        protected internal override bool TryComputeLength(out long length)
        {
            long currentLength = 0;
            long internalBoundaryLength = GetEncodedLength(crlf + "--" + boundary + crlf);
            
            // Start Boundary
            currentLength += GetEncodedLength("--" + boundary + crlf);

            bool first = true;
            foreach (HttpContent content in nestedContent)
            {
                if (first)
                {
                    first = false; // First boundary already written
                }
                else
                {
                    // Internal Boundary
                    currentLength += internalBoundaryLength;    
                }

                // Headers
                foreach (KeyValuePair<string, IEnumerable<string>> headerPair in content.Headers)
                {
                    currentLength += GetEncodedLength(headerPair.Key + ": " + string.Join(", ", headerPair.Value) + crlf);
                }

                currentLength += crlf.Length;

                // Content
                long tempContentLength = 0;                
                if (!content.TryComputeLength(out tempContentLength))
                {
                    length = 0;
                    return false;
                }
                currentLength += tempContentLength;
            }

            // Terminating boundary
            currentLength += GetEncodedLength(crlf + "--" + boundary + "--" + crlf);

            length = currentLength;
            return true;
        }

        private static int GetEncodedLength(string input)
        {
            return HttpRuleParser.DefaultHttpEncoding.GetByteCount(input);
        }

        #endregion Serialization
    }
}
