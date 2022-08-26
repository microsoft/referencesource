// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       TcpWireProtocol.cs
//
//  Summary:    Class for managing a socket connection.
//
//==========================================================================


using System;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;


namespace System.Runtime.Remoting.Channels.Tcp
{

    // After the version, one of the following 16-bit opcodes will appear.
    internal static class TcpOperations
    {
        internal const UInt16 Request       = 0;
        internal const UInt16 OneWayRequest = 1;
        internal const UInt16 Reply         = 2;
    } // TcpOpcodes

    internal static class TcpContentDelimiter
    {
        internal const UInt16 ContentLength     = 0; 
        internal const UInt16 Chunked           = 1;
    }

    // These are special header values. (Custom can be used for arbitrary
    //   pairs).
    internal static class TcpHeaders
    {
        internal const UInt16 EndOfHeaders      = 0; // (can appear once at end of headers)
        internal const UInt16 Custom            = 1;
        internal const UInt16 StatusCode        = 2;
        internal const UInt16 StatusPhrase      = 3;
        internal const UInt16 RequestUri        = 4;
        internal const UInt16 CloseConnection   = 5;
        internal const UInt16 ContentType       = 6;
    } // TcpHeaders

    // These are used by special headers (non-Custom) to indicate the format of the data,
    //   so that unknown special headers added in the future can be ignored.
    internal static class TcpHeaderFormat
    {
        internal const byte Void          = 0; // There is no further data.
        internal const byte CountedString = 1; // A single counted string follows.
        internal const byte Byte          = 2; // A single byte follows
        internal const byte UInt16        = 3; // A single unsigned 16-bit int follows.        
        internal const byte Int32         = 4; // A single 32-bit int follows.
    } // TcpHeaderFormat

    
    // status codes only apply to the transmission of data itself.
    internal static class TcpStatusCode
    {
        internal const UInt16 Success      = 0; // data was successfully received
        internal const UInt16 GenericError = 1; // an unknown error occurred
    } // TcpStatusCode


    // string format indicators
    // (strings have the following format on the wire:
    //   [format (1-byte)][encoded-size (int32)][string data (encoded-size bytes in length)]
    internal static class TcpStringFormat
    {
        internal const byte Unicode = 0;
        internal const byte UTF8    = 1;
    } // TcpStringFormat
    


} // namespace System.Runtime.Remoting.Channels.Tcp
