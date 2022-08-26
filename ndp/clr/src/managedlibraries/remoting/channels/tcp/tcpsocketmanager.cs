// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       TcpSocketManager.cs
//
//  Summary:    Provides a base for the client and server tcp socket 
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


namespace System.Runtime.Remoting.Channels.Tcp
{

    // A client socket manager instance should encapsulate the socket
    //   for the purpose of reading a response
    internal abstract class TcpSocketHandler : SocketHandler
    {            
        private static byte[] s_protocolPreamble = Encoding.ASCII.GetBytes(".NET");
        private static byte[] s_protocolVersion1_0 = new byte[]{1,0};


        public TcpSocketHandler(Socket socket, Stream stream) : this(socket, null, stream)
        {          
        } // TcpSocketHandler    
    
        public TcpSocketHandler(Socket socket, RequestQueue requestQueue, Stream stream) : 
                    base(socket, requestQueue, stream)
        {
        } // TcpSocketHandler    


        private void ReadAndMatchPreamble()
        {
            // make sure that the incoming data starts with the preamble
            InternalRemotingServices.RemotingAssert(
                s_protocolPreamble.Length == 4, 
                "The preamble is supposed to be 4 bytes ('.NET'). Somebody changed it...");

            if (ReadAndMatchFourBytes(s_protocolPreamble) == false)
            {
                throw new RemotingException(
                    CoreChannel.GetResourceString("Remoting_Tcp_ExpectingPreamble"));
            }
        } // ReadAndMatchPreamble

        protected void WritePreambleAndVersion(Stream outputStream)
        {
            outputStream.Write(s_protocolPreamble, 0, s_protocolPreamble.Length);
            outputStream.Write(s_protocolVersion1_0, 0, s_protocolVersion1_0.Length);
        } // WritePreamble


        protected void ReadVersionAndOperation(out UInt16 operation)
        {
            // check for the preamble
            ReadAndMatchPreamble();
        
            // Check the version number.
            byte majorVersion = (byte)ReadByte();
            byte minorVersion = (byte)ReadByte();
            if ((majorVersion != 1) || (minorVersion != 0))
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_UnknownProtocolVersion"),
                        majorVersion.ToString(CultureInfo.CurrentCulture) + "." + minorVersion.ToString(CultureInfo.CurrentCulture)));
            }

            // Read the operation
            operation = ReadUInt16();

        } // ReadVersionAndOperation


        protected void ReadContentLength(out bool chunked, out int contentLength)
        {        
            contentLength = -1;
        
            UInt16 header = ReadUInt16();
            if (header == TcpContentDelimiter.Chunked)
            {
                chunked = true;
            }
            else
            if (header == TcpContentDelimiter.ContentLength)
            {
                chunked = false;
                contentLength = ReadInt32();
            }
            else
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_ExpectingContentLengthHeader"),
                        header.ToString(CultureInfo.CurrentCulture)));
                    
            }            
        } // ReadContentLength 


        protected void ReadToEndOfHeaders(BaseTransportHeaders headers)
        {
            bool bError = false;
            String statusPhrase = null; 
        
            UInt16 headerType = ReadUInt16();
            while (headerType != TcpHeaders.EndOfHeaders)
            {
                if (headerType == TcpHeaders.Custom)
                {
                    String headerName = ReadCountedString();
                    String headerValue = ReadCountedString();

                    headers[headerName] = headerValue;
                }
                else
                if (headerType == TcpHeaders.RequestUri)
                {         
                    ReadAndVerifyHeaderFormat("RequestUri", TcpHeaderFormat.CountedString);
                
                    // read uri (and make sure that no channel specific data is present)
                    String uri = ReadCountedString();
                    
                    String channelURI;
                    String objectURI;
                    channelURI = TcpChannelHelper.ParseURL(uri, out objectURI);
                    if (channelURI == null)
                        objectURI = uri;              
            
                    headers.RequestUri = objectURI;
                }
                else
                if (headerType == TcpHeaders.StatusCode)
                {
                    ReadAndVerifyHeaderFormat("StatusCode", TcpHeaderFormat.UInt16);
                    
                    UInt16 statusCode = ReadUInt16();
                    // We'll throw an exception here if there was an error. If an error
                    //   occurs above the transport level, the status code will still be
                    //   success here.
                    if (statusCode != TcpStatusCode.Success)
                        bError = true;
                }
                else
                if (headerType == TcpHeaders.StatusPhrase)
                {
                    ReadAndVerifyHeaderFormat("StatusPhrase", TcpHeaderFormat.CountedString);
                
                    statusPhrase = ReadCountedString();
                }
                else
                if (headerType == TcpHeaders.ContentType)
                {
                    ReadAndVerifyHeaderFormat("Content-Type", TcpHeaderFormat.CountedString);
                
                    String contentType = ReadCountedString();
                    headers.ContentType = contentType;
                }
                else
                {
                    // unknown header: Read header format and ignore rest of data
                    byte headerFormat = (byte)ReadByte();

                    switch (headerFormat)
                    {
                    case TcpHeaderFormat.Void: break;
                    case TcpHeaderFormat.CountedString: ReadCountedString(); break;
                    case TcpHeaderFormat.Byte: ReadByte(); break;
                    case TcpHeaderFormat.UInt16: ReadUInt16(); break;
                    case TcpHeaderFormat.Int32: ReadInt32(); break;

                    default:
                    {
                        // unknown format
                        throw new RemotingException(
                            String.Format(
                                CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_UnknownHeaderType"),
                                headerType, headerFormat));
                    }
                    
                    } // switch (format)
                
                }

                // read next header token
                headerType = ReadUInt16();
            } // loop until end of headers         

            // if an error occurred, throw an exception
            if (bError)
            {
                if (statusPhrase == null)
                    statusPhrase = "";
                    
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_GenericServerError"),
                        statusPhrase));
            }
        } // ReadToEndOfHeaders


        protected void WriteHeaders(ITransportHeaders headers, Stream outputStream)
        {
            IEnumerator it = null;
            BaseTransportHeaders wkHeaders = headers as BaseTransportHeaders;

            if (wkHeaders != null)
            {
                // write out well known headers
                //   NOTE: RequestUri is written out elsewhere.
            
                if (wkHeaders.ContentType != null)
                {
                    WriteContentTypeHeader(wkHeaders.ContentType, outputStream);
                }

                it = wkHeaders.GetOtherHeadersEnumerator();
            }
            else
            {
                it = headers.GetEnumerator();
            }

        
            // write custom headers
            if (it != null)
            {
                while (it.MoveNext())
                {
                    DictionaryEntry header = (DictionaryEntry)it.Current;
            
                    String headerName = (String)header.Key;

                    if (!StringHelper.StartsWithDoubleUnderscore(headerName)) // exclude special headers
                    {
                        String headerValue = header.Value.ToString();

                        if (wkHeaders == null)
                        {
                            if (String.Compare(headerName, "Content-Type", StringComparison.OrdinalIgnoreCase) == 0)
                            {
                                WriteContentTypeHeader(headerValue, outputStream);
                                continue;
                            }
                        }

                        WriteCustomHeader(headerName, headerValue, outputStream);
                    }                                
                } // while (it.MoveNext())            
            }

            // write EndOfHeaders token
            WriteUInt16(TcpHeaders.EndOfHeaders, outputStream);
        } // WriteHeaders


        private void WriteContentTypeHeader(String value, Stream outputStream)
        {
            WriteUInt16(TcpHeaders.ContentType, outputStream);
            WriteByte(TcpHeaderFormat.CountedString, outputStream);  
            WriteCountedString(value, outputStream);
        } // WriteContentTypeHeader

        private void WriteCustomHeader(String name, String value, Stream outputStream)
        {
            WriteUInt16(TcpHeaders.Custom, outputStream);                    
            WriteCountedString(name, outputStream);
            WriteCountedString(value, outputStream);
        } // WriteCustomHeader
        


        protected String ReadCountedString()
        {
            // strings are formatted as follows 
            // [string format (1-byte)][encoded-size (int32)][string value (encoded-size length in bytes)]
            
            byte strFormat = (byte)ReadByte();
            int strDataSize = ReadInt32();

            if (strDataSize > 0)
            {
                byte[] data = new byte[strDataSize];

                // SocketHander::Read waits until it reads all requested data
                Read(data, 0, strDataSize);

                switch (strFormat)
                {
                    case TcpStringFormat.Unicode:
                        return Encoding.Unicode.GetString(data);

                    case TcpStringFormat.UTF8:
                        return Encoding.UTF8.GetString(data);

                    default:
                        throw new RemotingException(
                            String.Format(
                                CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_UnrecognizedStringFormat"),
                                strFormat.ToString(CultureInfo.CurrentCulture)));
                }
            }
            else
            {
                return null;
            }
        } // ReadCountedString


        protected void WriteCountedString(String str, Stream outputStream)
        {
            // strings are formatted as follows [string length (int32)][string value (unicode)]
            int strLength = 0;
            if (str != null)
                strLength = str.Length;

            if (strLength > 0)
            {            
                byte[] strBytes = Encoding.UTF8.GetBytes(str);          

                // write string format
                WriteByte(TcpStringFormat.UTF8, outputStream);

                // write string data size
                WriteInt32(strBytes.Length, outputStream);
                
                // write string data
                outputStream.Write(strBytes, 0, strBytes.Length);  
            }
            else
            {
                // write string format
                //   (just call it Unicode (doesn't matter since there is no data))
                WriteByte(TcpStringFormat.Unicode, outputStream);
            
                // stream data size is 0.
                WriteInt32(0, outputStream);
            }
        } // WriteCountedString


        private void ReadAndVerifyHeaderFormat(String headerName, byte expectedFormat)
        {
            byte headerFormat = (byte)ReadByte();

            if (headerFormat != expectedFormat)
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString("Remoting_Tcp_IncorrectHeaderFormat"),
                        expectedFormat, headerName));
            }
        } // ReadAndVerifyHeaderFormat
   
    } // TcpSocketHandler

} // namespace System.Runtime.Remoting.Channels
