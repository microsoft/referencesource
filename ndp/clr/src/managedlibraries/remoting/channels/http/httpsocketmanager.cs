// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       HttpSocketManager.cs
//
//  Summary:    Provides a base for the client and server http socket 
//              managers.
//
//==========================================================================


using System;
using System.Collections;
using System.Globalization;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;


namespace System.Runtime.Remoting.Channels.Http
{

    // A client socket manager instance should encapsulate the socket
    //   for the purpose of reading a response
    internal abstract class HttpSocketHandler : SocketHandler
    {            
        private static byte[] s_httpVersion = Encoding.ASCII.GetBytes("HTTP/1.1");
        private static byte[] s_httpVersionAndSpace = Encoding.ASCII.GetBytes("HTTP/1.1 ");
        private static byte[] s_headerSeparator = new byte[]{(byte)':',(byte)' '};
        private static byte[] s_endOfLine = new byte[]{(byte)'\r',(byte)'\n'};

    
        public HttpSocketHandler(Socket socket, RequestQueue requestQueue, Stream stream) : base(socket, requestQueue, stream)
        {          
        } // HttpSocketHandler


        protected void ReadToEndOfHeaders(BaseTransportHeaders headers, 
                                          out bool bChunked,
                                          out int contentLength,
                                          ref bool bKeepAlive,
                                          ref bool bSendContinue)
        {
            bChunked = false;
            contentLength = 0;
        
            // read and process headers
            for (;;)
            {
                String header = ReadToEndOfLine();

                // stop reading headers at first blank line
                if (header.Length == 0)
                    break;
                
                int sep = header.IndexOf(":");
                String headerName = header.Substring(0,sep);
                String headerValue = header.Substring(sep+1+1); // skip semi-colon and space

                if (String.Compare(headerName, "Transfer-Encoding", StringComparison.OrdinalIgnoreCase) == 0)          
                {
                    if (String.Compare(headerValue, "chunked", StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        bChunked = true;
                    }
                }
                else
                if (String.Compare(headerName, "Connection", StringComparison.OrdinalIgnoreCase) == 0)
                {
                    if (String.Compare(headerValue, "Keep-Alive", StringComparison.OrdinalIgnoreCase) == 0)
                        bKeepAlive = true;
                    else
                    if (String.Compare(headerValue, "Close", StringComparison.OrdinalIgnoreCase) == 0)
                        bKeepAlive = false;
                }
                else
                if (String.Compare(headerName, "Expect", StringComparison.OrdinalIgnoreCase) == 0)
                {
                    if (String.Compare(headerValue, "100-continue", StringComparison.OrdinalIgnoreCase) == 0)
                        bSendContinue = true;
                }
                else
                if (String.Compare(headerName, "Content-Length", StringComparison.OrdinalIgnoreCase) == 0)
                {
                    contentLength = Int32.Parse(headerValue, CultureInfo.InvariantCulture);
                }
                else
                {                
                    headers[headerName] = headerValue;
                }
            }
        } // ReadToEndOfHeaders


        protected void WriteHeaders(ITransportHeaders headers, Stream outputStream)
        {
            if (headers == null)
                return;
        
            foreach (DictionaryEntry header in headers)
            {
                String headerName = (String)header.Key;

                if (!headerName.StartsWith("__", StringComparison.Ordinal)) // exclude special headers
                {
                    WriteHeader(headerName, (String)header.Value, outputStream);
                }
            }

            // write end of headers "\r\n"
            outputStream.Write(s_endOfLine, 0, s_endOfLine.Length);
        } // WriteHeaders

        private void WriteHeader(String name, String value, Stream outputStream)
        {
            byte[] nameBytes = Encoding.ASCII.GetBytes(name); 
            byte[] valueBytes = Encoding.ASCII.GetBytes(value); 
            
            outputStream.Write(nameBytes, 0, nameBytes.Length);
            outputStream.Write(s_headerSeparator, 0, s_headerSeparator.Length);
            outputStream.Write(valueBytes, 0, valueBytes.Length);
            outputStream.Write(s_endOfLine, 0, s_endOfLine.Length);            
        } // WriteHeader

        protected void WriteResponseFirstLine(String statusCode, String reasonPhrase, Stream outputStream)
        {
            byte[] statusCodeBytes = Encoding.ASCII.GetBytes(statusCode); 
            byte[] reasonPhraseBytes = Encoding.ASCII.GetBytes(reasonPhrase); 
        
            outputStream.Write(s_httpVersionAndSpace, 0, s_httpVersionAndSpace.Length);
            outputStream.Write(statusCodeBytes, 0, statusCodeBytes.Length);
            outputStream.WriteByte((byte)' ');
            outputStream.Write(reasonPhraseBytes, 0, reasonPhraseBytes.Length);
            outputStream.Write(s_endOfLine, 0, s_endOfLine.Length);
        } // WriteResponseFirstLine        


    } // class HttpSocketHandler


} // namespace System.Runtime.Remoting.Channels.Tcp

