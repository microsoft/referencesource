// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//==========================================================================
//  File:       HttpChannelHelper.cs
//
//  Summary:    Implements helper methods for http client and server channels.
//
//==========================================================================

using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Globalization;

namespace System.Runtime.Remoting.Channels.Http
{

    
    internal static class HttpChannelHelper
    {
        private const String _http = "http://";
#if !FEATURE_PAL
        private const String _https = "https://";
#endif

        private static char[] s_semicolonSeparator = new char[]{';'};

    
        // Determine if the url starts with "http://"
#if !FEATURE_PAL
        // or "https://"
#endif
        internal static int StartsWithHttp(String url)
        {
            int urlLength = url.Length;
            
            if (StringHelper.StartsWithAsciiIgnoreCasePrefixLower(url, _http))
                return _http.Length;
#if !FEATURE_PAL
            else
            if (StringHelper.StartsWithAsciiIgnoreCasePrefixLower(url, _https))
                return _https.Length;
#endif
            else
                return -1;
        } // StartsWithHttp
 

        // Used by http channels to implement IChannel::Parse.
        // It returns the channel uri and places object uri into out parameter.
        internal static String ParseURL(String url, out String objectURI)
        {            
            // Set the out parameters
            objectURI = null;

            int separator = StartsWithHttp(url);
            if (separator == -1)
                return null;

            // find next slash (after end of scheme)
            separator = url.IndexOf('/', separator);
            if (-1 == separator)
            {
                return url;  // means that the url is just "tcp://foo:90" or something like that
            }

            // Extract the channel URI which is the prefix
            String channelURI = url.Substring(0, separator);

            // Extract the object URI which is the suffix (leave the slash)
            objectURI = url.Substring(separator);

            InternalRemotingServices.RemotingTrace("HTTPChannel.Parse URI in: " + url);
            InternalRemotingServices.RemotingTrace("HTTPChannel.Parse channelURI: " + channelURI);
            InternalRemotingServices.RemotingTrace("HTTPChannel.Parse objectURI: " + objectURI);

            return channelURI;
        } // ParseURL


        internal static String GetObjectUriFromRequestUri(String uri)
        {
            // We assume uri may be in one of the following forms
            //   http://myhost.com/myobject.rem
#if !FEATURE_PAL
            //   https://myhost.com/myobject.rem
#endif
            //   /myobject.rem
            //   /myobject
            //   myobject.rem
            // In all cases, myobject is considered to be the object URI (.rem might be absent)

            int start, end; // range of characters to use
            int index;
            start = 0;
            end = uri.Length;

            // first see if uri starts with http://
#if !FEATURE_PAL
            // or https://
#endif
            // and remove up to next slash if it does
            start = StartsWithHttp(uri);
            if (start != -1)
            {
                // remove domain name as well
                index = uri.IndexOf('/', start);
                if (index != -1)
                    start = index + 1;
                else
                    start = end; // uri will end up being ""
            }
            else
            {
                // remove "/" if this is an absolute path
                start = 0; 
                if (uri[start] == '/')
                   start++;
            }

            // remove query string if present ('?' and everything past it)
            index = uri.IndexOf('?');
            if (index != -1)
                end = index;

            if (start < end)
                return CoreChannel.RemoveApplicationNameFromUri(uri.Substring(start, end - start));
            else
                return "";
        } // GetObjectURIFromRequestURI

        
        internal static void ParseContentType(String contentType,
                                              out String value,
                                              out String charset)
        {
            charset = null;
        
            if (contentType == null)
            {
                value = null;
                return;
            }
        
            String[] parts = contentType.Split(s_semicolonSeparator);

            // the actual content-type value is always first
            value = parts[0];

            // examine name value pairs and look for charset
            if (parts.Length > 0)
            {
                foreach (String part in parts)
                {
                    int index = part.IndexOf('=');
                    if (index != -1)
                    {
                        String key = part.Substring(0, index).Trim();
                        if (String.Compare(key, "charset", StringComparison.OrdinalIgnoreCase) == 0)
                        {
                            if ((index + 1) < part.Length)
                            {
                                // we had to make sure there is something after the 
                                //   equals sign.
                                charset = part.Substring(index + 1);
                            }
                            else
                            {
                                charset = null;
                            }
                            return;
                        }
                    }
                } // foreach
            }
        } // ParseContentType

        internal static String ReplaceChannelUriWithThisString(String url, String channelUri) 
        {
            // NOTE: channelUri is assumed to be scheme://machinename:port
            //   with NO trailing slash.

            String oldChannelUri;
            String objUri;
            oldChannelUri = HttpChannelHelper.ParseURL(url, out objUri);
            InternalRemotingServices.RemotingAssert(oldChannelUri != null, "http url expected.");
            InternalRemotingServices.RemotingAssert(objUri != null, "non-null objUri expected.");
            
            return channelUri + objUri;
        } // ReplaceChannelUriWithThisString


        // returns url with the machine name replaced with the ip address.
        internal static String ReplaceMachineNameWithThisString(String url, String newMachineName)
        {
            String objectUri;
            String channelUri = ParseURL(url, out objectUri);    

            // find bounds of machine name
            int index = StartsWithHttp(url);
            if (index == -1)
                return url;
   
            int colonIndex = channelUri.IndexOf(':', index);
            if (colonIndex == -1)
                colonIndex = channelUri.Length;

            // machine name is between index and up to but not including colonIndex, 
            //   so we will replace those characters with the ip address.
            String newUrl = url.Substring(0, index) + newMachineName + url.Substring(colonIndex);
            return newUrl;
        } // ReplaceMachineNameWithIpAddress


        // Decodes a uri while it is in byte array form
        internal static void DecodeUriInPlace(byte[] uriBytes, out int length)
        {
            int percentsFound = 0;
            int count = uriBytes.Length;
            length = count;
            int co = 0;
            while (co < count)
            {
                if (uriBytes[co] == (byte)'%')
                {
                    // determine location to write to (we skip 2 character for each percent)
                    int writePos = co - (percentsFound * 2);

                    // decode in place by collapsing bytes "%XY" (actual byte is 16*Dec(X) + Dec(Y))
                    uriBytes[writePos] = (byte)
                        (16 * CharacterHexDigitToDecimal(uriBytes[co + 1]) +
                         CharacterHexDigitToDecimal(uriBytes[co + 2]));

                    percentsFound++;      
                    length -= 2; // we eliminated 2 characters from the length
                    co += 3;
                }
                else
                {
                    if (percentsFound != 0)
                    {
                        // we have to copy characters back into place since we will skip some characters

                        // determine location to write to (we skip 2 character for each percent)
                        int writePos = co - (percentsFound * 2);

                        // copy character back into place
                        uriBytes[writePos] = uriBytes[co];
                    }

                    co++;
                }
            }
            
        } // DecodeUri



        // reading helper functions
        internal static int CharacterHexDigitToDecimal(byte b)
        {
            switch ((char)b)
            {
            case 'F':
            case 'f': return 15;
            case 'E':
            case 'e': return 14;
            case 'D':
            case 'd': return 13;
            case 'C':
            case 'c': return 12;
            case 'B':
            case 'b': return 11;
            case 'A':
            case 'a': return 10;
            default: return b - (byte)'0';
            }
        } // CharacterHexDigitToDecimal


        internal static char DecimalToCharacterHexDigit(int i)
        {
            switch (i)
            {
            case 15: return 'F';
            case 14: return 'E';
            case 13: return 'D';
            case 12: return 'C';
            case 11: return 'B';
            case 10: return 'A';
            default: return (char)(i + (byte)'0');
            }

        } // DecimalToCharacterHexDigit
                

    
    } // class HttpChannelHelper



    internal static class HttpEncodingHelper
    {
        internal static String EncodeUriAsXLinkHref(String uri)
        {
            if (uri == null)
                return null;
        
            // uses modified encoding rules from xlink href spec for encoding uri's.
            // http://www.w3.org/TR/2000/PR-xlink-20001220/#link-locators

            byte[] uriBytes = Encoding.UTF8.GetBytes(uri);

            StringBuilder sb = new StringBuilder(uri.Length);

            // iterate over uri bytes and build up an encoded string.
            foreach (byte b in uriBytes)
            {
                if (!EscapeInXLinkHref(b))
                {
                    sb.Append((char)b);
                }
                else
                {
                    // the character needs to be encoded as %HH
                    sb.Append('%');
                    sb.Append(HttpChannelHelper.DecimalToCharacterHexDigit(b >> 4));
                    sb.Append(HttpChannelHelper.DecimalToCharacterHexDigit(b & 0xF));
                }
            }

            return sb.ToString();            
        } // EncodeUriAsXLinkHref


        internal static bool EscapeInXLinkHref(byte ch)
        {
            if ((ch <= 32) || // control characters and space
                (ch >= 128) ||  // non-ascii characters
                (ch == (byte)'<') ||
                (ch == (byte)'>') ||
                (ch == (byte)'"'))        
            {
                return true;
            }
                   
            return false;
        } // EscapeInXLinkHref


        internal static String DecodeUri(String uri)
        {
            byte[] uriBytes = Encoding.UTF8.GetBytes(uri);

            int length;
            HttpChannelHelper.DecodeUriInPlace(uriBytes, out length);

            String newUri = Encoding.UTF8.GetString(uriBytes, 0, length);
            return newUri;
        } // DecodeUri
        
    } // class HttpEncodingHelper



} // namespace System.Runtime.Remoting.Channels.Http
