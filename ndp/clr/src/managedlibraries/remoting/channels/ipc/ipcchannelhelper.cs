// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       IpcChannelHelper.cs
//  Author:   Microsoft@Microsoft.Com
//  Summary:    Implements helper methods for Ipc client and server channels.
//
//==========================================================================

using System;
using System.IO;
using System.Collections;
using System.Runtime.Remoting.Channels;

namespace System.Runtime.Remoting.Channels.Ipc
{
    
    internal static class IpcChannelHelper
    {
        private const String _ipc = "ipc://";
        
        // see if the string starts with "ipc://"
        internal static bool StartsWithIpc(String url)
        {
            return StringHelper.StartsWithAsciiIgnoreCasePrefixLower(url, _ipc);
        } // StartsWithIpc

        // Used by Ipc channels to implement IChannel::Parse.
        // It returns the channel uri and places object uri into out parameter.
        internal static String ParseURL(String url, out String objectURI)
        {            
            if (url == null)
                throw new ArgumentNullException("url");

            // Set the out parameters
            objectURI = null;

            int separator;

            // Find the starting point of ipc://
            // NOTE: We are using this version of String.Compare to ensure
            // that string operations are case-insensitive!!
            if (StartsWithIpc(url))
            {
                separator = _ipc.Length;
            }
            else
            {
                return null;
            }

            // find next slash (after end of scheme)
            separator = url.IndexOf('/', separator);
            if (-1 == separator)
            {
                return url; // means that the url is just "Ipc://foo" or something like that
            }

            // Extract the channel URI which is the prefix
            String channelURI = url.Substring(0, separator);

            // Extract the object URI which is the suffix
            objectURI = url.Substring(separator); // leave the slash

            return channelURI;
        } // ParseURL
    } // class IpcChannelHelper


} // namespace System.Runtime.Remoting.Channels.Ipc
