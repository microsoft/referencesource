// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       CommonChannelKeys.cs
//
//  Summary:    Common transport keys used in channels.
//
//==========================================================================


namespace System.Runtime.Remoting.Channels
{

    public class CommonTransportKeys
    { 
        // The ip address from which an incoming request arrived.
        public const String IPAddress = "__IPAddress";

        // A unique id given to each incoming socket connection.
        public const String ConnectionId = "__ConnectionId";  

        // The request uri to use for this request or from the incoming request
        public const String RequestUri = "__RequestUri";
        
        
    } // CommonTransportKeys
    
} // namespace System.Runtime.Remoting.Channels
