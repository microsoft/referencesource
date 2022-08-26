// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       TcpChannelHelper.cs
//
//  Summary:    Implements helper methods for tcp client and server channels.
//
//==========================================================================

using System;
using System.Text;
using System.Runtime.Remoting.Channels;

namespace System.Runtime.Remoting.Channels.Tcp
{
    
    internal static class TcpChannelHelper
    {
        private const String _tcp = "tcp://";
        
        // Used by tcp channels to implement IChannel::Parse.
        // It returns the channel uri and places object uri into out parameter.
        internal static String ParseURL(String url, out String objectURI)
        {            
            // Set the out parameters
            objectURI = null;

            int separator;

            // Find the starting point of tcp://
            // NOTE: We are using this version of String.Compare to ensure
            // that string operations are case-insensitive!!
            if (StringHelper.StartsWithAsciiIgnoreCasePrefixLower(url, _tcp))
            {
                separator = _tcp.Length;
            }
            else
            {
                return null;
            }

            // find next slash (after end of scheme)
            separator = url.IndexOf('/', separator);
            if (-1 == separator)
            {
                return url; // means that the url is just "tcp://foo:90" or something like that
            }

            // Extract the channel URI which is the prefix
            String channelURI = url.Substring(0, separator);

            // Extract the object URI which is the suffix
            objectURI = url.Substring(separator); // leave the slash

            return channelURI;
        } // ParseURL

    
    } // class TcpChannelHelper


} // namespace System.Runtime.Remoting.Channels.Tcp
