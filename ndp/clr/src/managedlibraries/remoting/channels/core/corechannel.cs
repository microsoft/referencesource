// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
using System;
using System.Text;
using System.Threading;
#if !FEATURE_PAL
using System.DirectoryServices;
#endif
using System.Reflection;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters;
using System.Runtime.Serialization.Formatters.Soap;
using System.Runtime.Serialization.Formatters.Binary;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Messaging;       
using System.Runtime.Remoting.Metadata;
using System.Security.Principal;
using System.ComponentModel;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Collections;
using System.Net.Sockets;
using System.Resources;
using System.Diagnostics;
using System.Globalization;
#if !FEATURE_PAL
using System.Web;
#endif
using System.Runtime.InteropServices;

#if FEATURE_COMINTEROP

// These two attributes are for supporting side-by-side of COM-visible
// objects with NDP 1.0 RTM. This needs to be set on all assemblies that 
// expose COM-visible types to be made Side by Side with NDP 1.0 RTM.
// This declaration covers System.Runtime.Remoting.dll
[assembly:ComCompatibleVersion(1,0,3300,0)]
[assembly:TypeLibVersion(4,0)]

#endif // FEATURE_COMINTEROP

namespace System.Runtime.Remoting.Channels
{

    // Use this internal indicator (as opposed to the nested enum found
    //   on some of the server channel sinks)
    internal enum SinkChannelProtocol
    {
        Http, // special processing needed for http
        Other
    } // ChannelProtocol

    

    internal static class CoreChannel    
    {
        private static IByteBufferPool _bufferPool = new ByteBufferPool(10, 4096);
        private static RequestQueue _requestQueue = new RequestQueue(8,4,250);        

        internal static IByteBufferPool BufferPool { get { return _bufferPool; } }
        internal static RequestQueue RequestQueue { get { return _requestQueue; } }
        
    
        internal const int MaxStringLen = 512;

        internal const String SOAPMimeType = "text/xml";
        internal const String BinaryMimeType = "application/octet-stream";

        internal const String SOAPContentType = "text/xml; charset=\"utf-8\"";

        private static String s_hostName = null; 
        private static String s_MachineName = null;
        private static String s_MachineIp = null;

        private static volatile IPHostEntry s_CachedIPHostEntry = null; 

        // Copy of consts defined in RemotingServices.cs
        internal const int CLIENT_MSG_GEN          = 1;
        internal const int CLIENT_MSG_SINK_CHAIN   = 2;
        internal const int CLIENT_MSG_SER          = 3;
        internal const int CLIENT_MSG_SEND         = 4;
        internal const int SERVER_MSG_RECEIVE      = 5;
        internal const int SERVER_MSG_DESER        = 6;
        internal const int SERVER_MSG_SINK_CHAIN   = 7;
        internal const int SERVER_MSG_STACK_BUILD  = 8;
        internal const int SERVER_DISPATCH         = 9;
        internal const int SERVER_RET_STACK_BUILD  = 10;
        internal const int SERVER_RET_SINK_CHAIN   = 11;
        internal const int SERVER_RET_SER          = 12;
        internal const int SERVER_RET_SEND         = 13;
        internal const int SERVER_RET_END          = 14;
        internal const int CLIENT_RET_RECEIVE      = 15;
        internal const int CLIENT_RET_DESER        = 16;
        internal const int CLIENT_RET_SINK_CHAIN   = 17;
        internal const int CLIENT_RET_PROPAGATION  = 18;
        internal const int CLIENT_END_CALL         = 19;
        internal const int TIMING_DATA_EOF         = 99;

        private static bool s_isClientSKUInstallationInitialized = false;
        private static bool s_isClientSKUInstallation = false;

        static CoreChannel()
        {
            try
            {
                NetworkChange.NetworkAddressChanged += new NetworkAddressChangedEventHandler(OnNetworkAddressChanged);
            }
            catch
            {
                // in case exception gets thrown from static constructor, we won't get network address changed notifications 
                // but at least we won't be left with a half constructed class ......
            }
        } // cctor

        // Returns true if the Fx Client SKU is installed, false if Full SKU
        internal static bool IsClientSKUInstallation
        {
            get
            {
                if (!s_isClientSKUInstallationInitialized)
                {
                    Type type = Type.GetType("System.Web.HttpContext, " + AssemblyRef.SystemWeb, false);
                    s_isClientSKUInstallation = (type == null);
                    s_isClientSKUInstallationInitialized = true;
                }

                return s_isClientSKUInstallation;
            }
        } // IsClientSKUInstallation

        private static void OnNetworkAddressChanged(object sender, EventArgs e)
        {
            try
            {
                UpdateCachedIPAddresses();
            }
            catch 
            {
                // this call is coming from an event; leak no exceptions
            }
        } // OnNetworkAddressChanged

        private static void UpdateCachedIPAddresses()
        {
            try
            {
                s_CachedIPHostEntry = Dns.GetHostEntry(GetMachineName());
            }
            catch (Exception exception)
            {
                s_CachedIPHostEntry = null;

                InternalRemotingServices.RemotingTrace(string.Format(
                       "CoreChannel.UpdateCachedIPAddresses caught exception '{0}'; \r\nMessage: {1}", exception.GetType().ToString(), exception.Message));

                throw; 
            }
        } // UpdateCachedIPAddresses

        internal static String GetHostName()
        {
            if (s_hostName == null)
            {
                s_hostName = Dns.GetHostName();

                if (s_hostName == null)
                {
                    throw new ArgumentNullException("hostName");
                }
            }

            return s_hostName;
        } // GetHostName

        internal static String GetMachineName()
        {
            if (s_MachineName == null)
            {     
                String machineName = GetHostName();
                if (machineName != null)
                {
                    IPHostEntry host = Dns.GetHostEntry(machineName);
                    if (host != null)
                        s_MachineName = host.HostName;
                } 

                if (s_MachineName == null)
                {
                    throw new ArgumentNullException("machine");
                }
            }
            
            return s_MachineName;      
        } // GetMachineName


        // This helper function Checks whether the remote IP Adress is actually a local address
        internal static bool IsLocalIpAddress(IPAddress remoteAddress)
        {
            if (s_CachedIPHostEntry == null)
            {
                UpdateCachedIPAddresses(); 
            }

            return IsLocalIpAddress(s_CachedIPHostEntry, remoteAddress.AddressFamily, remoteAddress);
        }
        
        //This helper function compares and IpAddress with all addresses in IpHostEntry
        internal static bool IsLocalIpAddress(IPHostEntry host, AddressFamily addressFamily, IPAddress remoteAddress)
        {
            if (host != null)
            {
                IPAddress[] addressList = host.AddressList;
                for (int i = 0; i < addressList.Length; i++)
                {
                    if (addressList[i].AddressFamily == addressFamily)
                    {
                        if(addressList[i].Equals(remoteAddress))
                            return true;
                    }
                }
            }
            return false;
        }

        // process specified host name to see if it is a meta-hostname that
        //   should be replaced with something else.
        internal static String DecodeMachineName(String machineName)
        {
            if (machineName.Equals("$hostName"))
                return GetHostName();

            return machineName;
        } // DecodeMachineName
        

        internal static String GetMachineIp()
        {
            if (s_MachineIp == null)
            {            
                String hostName = GetMachineName();

                // NOTE: We intentionally allow exceptions from these api's
                //  propagate out to the caller.
                IPHostEntry ipEntries = Dns.GetHostEntry(hostName);

                //Assumption is Socket.OSSupportsIPv4 will be false only on OS >= Vista with IPv4 turned off.
                AddressFamily addressFamily = (Socket.OSSupportsIPv4) ? AddressFamily.InterNetwork : AddressFamily.InterNetworkV6;
                IPAddress addr = GetMachineAddress(ipEntries, addressFamily);
                if (addr != null)
                {
                    s_MachineIp = addr.ToString();
                }
                
                if (s_MachineIp == null)
                {
                    throw new ArgumentNullException("ip");
                }
            }
            
            return s_MachineIp;      
        } // GetMachineIp

        // This helper function returns the first IPAddress with family 'addressFamily' from
        // host.AddressList, or null if there is no such address or if host is null.
        internal static IPAddress GetMachineAddress(IPHostEntry host, AddressFamily addressFamily)
        {
            // NOTE: We intentionally allow exceptions from these api's
            //  propagate out to the caller.
            IPAddress result = null;
            if (host != null)
            {
                // find the first address for this address family
                IPAddress[] addressList = host.AddressList;
                for (int i = 0; i < addressList.Length; i++)
                {
                    if (addressList[i].AddressFamily == addressFamily)
                    {
                        result = addressList[i];
                        break;
                    }
                }
            }
            
            //Console.WriteLine("GetMachineAddress(" + hostName + ", " + addressFamily + ") -> " + (result == null ? "<null>" : result.ToString()));
            return result;
        } // GetMachineAddress
        

        //
        // Core Serialization and Deserialization support
        //        
        internal static Header[] GetMessagePropertiesAsSoapHeader(IMessage reqMsg)
        {
            IDictionary d = reqMsg.Properties;
            if (d == null)
                return null;
                
            int count = d.Count;
            if (count == 0)
                return null;

            IDictionaryEnumerator e = (IDictionaryEnumerator) d.GetEnumerator();

            // cycle through the headers to get a length
            bool[] map = new bool[count];
            int len = 0, i=0;
            IMethodMessage msg = (IMethodMessage)reqMsg;
            while (e.MoveNext())
            {                   
                String key = (String)e.Key;
                if ((key.Length >= 2) &&
                    (String.CompareOrdinal(key, 0, "__", 0, 2)  == 0) 
                     &&
                     (
                        key.Equals("__Args") 
                                ||
                        key.Equals("__OutArgs") 
                                ||
                        key.Equals("__Return") 
                                ||
                        key.Equals("__Uri") 
                                ||
                        key.Equals("__MethodName") 
                                ||
                        (key.Equals("__MethodSignature") 
                                && (!RemotingServices.IsMethodOverloaded(msg))
                                && (!msg.HasVarArgs))
                                ||
                        key.Equals("__TypeName") 
                                ||
                        key.Equals("__Fault") 
                                ||
                        (key.Equals("__CallContext") 
                                && ((e.Value != null) ? (((LogicalCallContext)e.Value).HasInfo==false) : true))
                      )                       
                  )
                {
                        i++;
                    continue;
                }
                map[i] = true;
                i++;                
                len++;
            }
            if (len == 0)
                return null;

            
            Header[] ret = new Header[len];
            e.Reset();
            int k=0; 
            i = 0;
            while (e.MoveNext())
            {
                Object key = e.Key;
                if (!map[k])
                {
                    k++;
                    continue;
                }
                
                Header h = e.Value as Header;

                // If the property is not a header, then make a header out of it.
                if (h == null)
                {
                    h = 
                        new Header(
                            (String)key, e.Value, false,
                            "http://schemas.microsoft.com/clr/soap/messageProperties");
                }

                // <
                if (i == ret.Length)
                {
                    InternalRemotingServices.RemotingTrace("HTTPChannel::GetHeaders creating a new array of length " + (i+1) + "\n");
                    Header[] newret= new Header[i+1];
                    Array.Copy(ret, newret, i);
                    ret = newret;
                }
                ret[i] = h;
                i++;
                k++;
            }
            
            return ret;
        } // GetMessagePropertiesAsSoapHeader
        

        internal static Header[] GetSoapHeaders(IMessage reqMsg)
        {    
            // If there are message properties, we'll need to resize the header array.
            Header[] msgProperties = GetMessagePropertiesAsSoapHeader(reqMsg);

            return msgProperties;
        } // GetSoapHeaders



        internal static SoapFormatter CreateSoapFormatter(bool serialize, bool includeVersions)
        {            
            SoapFormatter remotingFormatter = new SoapFormatter();

            if (serialize)
            {
                RemotingSurrogateSelector rss = new RemotingSurrogateSelector();
                remotingFormatter.SurrogateSelector = rss;
                rss.UseSoapFormat();
            }
            else
                remotingFormatter.SurrogateSelector = null;

            remotingFormatter.Context = new StreamingContext(StreamingContextStates.Other);

            remotingFormatter.AssemblyFormat = 
                includeVersions ? 
                    FormatterAssemblyStyle.Full :
                    FormatterAssemblyStyle.Simple;

            return remotingFormatter;
        } // CreateSoapFormatter


        internal static BinaryFormatter CreateBinaryFormatter(bool serialize, 
                                                              bool includeVersionsOrStrictBinding)
        {
            BinaryFormatter remotingFormatter = new BinaryFormatter();

            if (serialize)
            {
                RemotingSurrogateSelector rss = new RemotingSurrogateSelector();
                remotingFormatter.SurrogateSelector = rss;
            }
            else
            {
                remotingFormatter.SurrogateSelector = null;
            }

            remotingFormatter.Context = new StreamingContext(StreamingContextStates.Other);

            remotingFormatter.AssemblyFormat = 
                includeVersionsOrStrictBinding ? 
                    FormatterAssemblyStyle.Full :
                    FormatterAssemblyStyle.Simple;  
            
            return remotingFormatter;
        } // CreateBinaryFormatter




        internal static void SerializeSoapMessage(IMessage msg, Stream outputStream, bool includeVersions)
        {
            // create soap formatter
            SoapFormatter fmt = CreateSoapFormatter(true, includeVersions);

            //check for special options if this is the SoapFormatter
            IMethodMessage methodMsg = msg as IMethodMessage;
            if (methodMsg != null)
            {
                MethodBase mb = methodMsg.MethodBase;
                if (mb != null)
                {
                    Type type = methodMsg.MethodBase.DeclaringType;
                    SoapTypeAttribute cache = 
                        (SoapTypeAttribute)InternalRemotingServices.GetCachedSoapAttribute(type);
                    if ((cache.SoapOptions & SoapOption.AlwaysIncludeTypes) == SoapOption.AlwaysIncludeTypes)
                        fmt.TypeFormat |= FormatterTypeStyle.TypesAlways;
                    if ((cache.SoapOptions & SoapOption.XsdString) == SoapOption.XsdString)
                        fmt.TypeFormat |= FormatterTypeStyle.XsdString;                
                }
            }
            // end of set special options for SoapFormatter
            
            Header[] h = GetSoapHeaders(msg);

            // this is to make messages within a  message serialize correctly 
            // and not use the fake type
            ((RemotingSurrogateSelector)fmt.SurrogateSelector).SetRootObject(msg);
            fmt.Serialize(outputStream, msg, h);
        } // SerializeSoapMessage

        internal static Stream SerializeSoapMessage(IMessage msg, bool includeVersions)
        {
            MemoryStream memStream = new MemoryStream();
            SerializeSoapMessage(msg, memStream, includeVersions);
            memStream.Position = 0;
            return memStream;
        } // SerializeSoapMessage
        


        internal static void SerializeBinaryMessage(IMessage msg, Stream outputStream, bool includeVersions)
        {
            // create binary formatter
            BinaryFormatter fmt = CreateBinaryFormatter(true, includeVersions);

            // WE SHOULD NOT CALL GetHeaders() here. The BinaryFormatter does special
            //   serialization for any headers that might be present.

            fmt.Serialize(outputStream, msg, null);
        } // SerializeBinaryMessage
        
        internal static Stream SerializeBinaryMessage(IMessage msg, bool includeVersions)
        {
            MemoryStream memStream = new MemoryStream();
            SerializeBinaryMessage(msg, memStream, includeVersions);
            memStream.Position = 0;
            return memStream;
        } // SerializeBinaryMessage



        // class used to pass uri into binary serializer, so that the message
        //   gets the object uri.
        private class UriHeaderHandler
        {
            String _uri = null;

            internal UriHeaderHandler(String uri)
            {
                _uri = uri;
            }
        
            public Object HeaderHandler(Header[] Headers)
            {
                return _uri;
            }
            
        } // classUriHeaderHandler


        internal static IMessage DeserializeSoapRequestMessage(
            Stream inputStream, Header[] h, bool bStrictBinding, TypeFilterLevel securityLevel)
        {
            SoapFormatter fmt = CreateSoapFormatter(false, bStrictBinding);
            fmt.FilterLevel = securityLevel;

            MethodCall mc = new MethodCall(h);
            fmt.Deserialize(inputStream, new HeaderHandler(mc.HeaderHandler));

            IMessage resMessage = (IMessage)mc;

            return resMessage;
        } // DeserializeSoapRequestMessage


        internal static IMessage DeserializeSoapResponseMessage(
            Stream inputStream, IMessage requestMsg, Header[] h, bool bStrictBinding)
        {
            SoapFormatter fmt = CreateSoapFormatter(false, bStrictBinding);

            IMethodCallMessage mcm = (IMethodCallMessage)requestMsg;
            MethodResponse mr = new MethodResponse(h, mcm);
            fmt.Deserialize(inputStream, new HeaderHandler(mr.HeaderHandler));

            IMessage resMessage = (IMessage)mr;

            return resMessage;
        } // DeserializeSoapResponseMessage


        internal static IMessage DeserializeBinaryRequestMessage(
            String objectUri, 
            Stream inputStream,
            bool bStrictBinding,
            TypeFilterLevel securityLevel)
        {
            BinaryFormatter fmt = CreateBinaryFormatter(false, bStrictBinding);
            fmt.FilterLevel = securityLevel; 

            UriHeaderHandler uriHH = new UriHeaderHandler(objectUri);

            IMessage reqMsg = 
                (IMessage)fmt.UnsafeDeserialize(inputStream, new HeaderHandler(uriHH.HeaderHandler));

            return reqMsg;
        } // DeserializeBinaryRequestMessage


        internal static IMessage DeserializeBinaryResponseMessage(
            Stream inputStream,
            IMethodCallMessage reqMsg,
            bool bStrictBinding)
        {
            BinaryFormatter fmt = CreateBinaryFormatter(false, bStrictBinding);

            IMessage replyMsg = (IMessage)fmt.UnsafeDeserializeMethodResponse(inputStream, null, reqMsg);
            return replyMsg;
        } // DeserializeBinaryResponseMessage

        internal static Stream SerializeMessage(String mimeType, IMessage msg, bool includeVersions)
        {
            Stream returnStream = new MemoryStream();
            SerializeMessage(mimeType, msg, returnStream, includeVersions);
            returnStream.Position = 0;
            return returnStream;
        } // SerializeMessage


        internal static void SerializeMessage(String mimeType, IMessage msg, Stream outputStream,
                                              bool includeVersions)
        {
            InternalRemotingServices.RemotingTrace("SerializeMessage");
            InternalRemotingServices.RemotingTrace("MimeType: " + mimeType);
            CoreChannel.DebugMessage(msg);
            
            if (string.Compare(mimeType, SOAPMimeType, StringComparison.Ordinal) == 0)
            {
                SerializeSoapMessage(msg, outputStream, includeVersions);
            }
            else
            if (string.Compare(mimeType, BinaryMimeType, StringComparison.Ordinal) == 0)
            {
                SerializeBinaryMessage(msg, outputStream, includeVersions);
            }                   

            InternalRemotingServices.RemotingTrace("SerializeMessage: OUT");
        } // SerializeMessage



        
        internal static IMessage DeserializeMessage(String mimeType, Stream xstm, bool methodRequest, IMessage msg)
        {
            return DeserializeMessage(mimeType, xstm, methodRequest, msg, null);
        }

        internal static IMessage DeserializeMessage(String mimeType, Stream xstm, bool methodRequest, IMessage msg, Header[] h)
        {
            InternalRemotingServices.RemotingTrace("DeserializeMessage");
            InternalRemotingServices.RemotingTrace("MimeType: " + mimeType);

            CoreChannel.DebugOutXMLStream(xstm, "Deserializing");

            Stream fmtStm = null;

            bool bin64encode = false;
            bool doHeaderBodyAsOne = true;

            if (string.Compare(mimeType, BinaryMimeType, StringComparison.Ordinal) == 0)
            {
                doHeaderBodyAsOne = true;
            }

            if (string.Compare(mimeType, SOAPMimeType, StringComparison.Ordinal) == 0)
            {
                doHeaderBodyAsOne = false;
            }

            if (bin64encode == false)
            {
                fmtStm  = xstm;
            }
            else
            {
                InternalRemotingServices.RemotingTrace("***************** Before base64 decode *****");

                long Position = xstm.Position;
                MemoryStream inStm = (MemoryStream)xstm;
                byte[] byteArray = inStm.ToArray();
                xstm.Position = Position;

                String base64String = Encoding.ASCII.GetString(byteArray,0, byteArray.Length);

                byte[] byteArrayContent = Convert.FromBase64String(base64String);

                MemoryStream memStm = new MemoryStream(byteArrayContent);

                fmtStm = memStm;
                InternalRemotingServices.RemotingTrace("***************** after base64 decode *****");
            }

            Object ret;
            IRemotingFormatter fmt = MimeTypeToFormatter(mimeType, false);

            if (doHeaderBodyAsOne == true)
            {
                ret = ((BinaryFormatter)fmt).UnsafeDeserializeMethodResponse(fmtStm, null, (IMethodCallMessage)msg);
            }
            else
            {
                InternalRemotingServices.RemotingTrace("Content");
                InternalRemotingServices.RemotingTrace("***************** Before Deserialize Headers *****");

                InternalRemotingServices.RemotingTrace("***************** After Deserialize Headers *****");

                InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message *****");

                if (methodRequest == true)
                {
                    MethodCall mc = new MethodCall(h);
                    InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message - as MethodCall *****");
                    fmt.Deserialize(fmtStm, new HeaderHandler(mc.HeaderHandler));
                    ret = mc;
                }
                else
                {
                    IMethodCallMessage mcm = (IMethodCallMessage)msg;
                    MethodResponse mr = new MethodResponse(h, mcm);
                    InternalRemotingServices.RemotingTrace("***************** Before Deserialize Message - as MethodResponse *****");
                    fmt.Deserialize(fmtStm, new HeaderHandler(mr.HeaderHandler));
                    ret = mr;
                }

                InternalRemotingServices.RemotingTrace("***************** After Deserialize Message *****");
            }

            // Workaround to make this method verifiable
            IMessage resMessage = (IMessage) ret;

            InternalRemotingServices.RemotingTrace("CoreChannel::DeserializeMessage OUT");
            CoreChannel.DebugMessage(resMessage);

            return resMessage;
        }
       
    

        internal static IRemotingFormatter MimeTypeToFormatter(String mimeType, bool serialize)
        {
            InternalRemotingServices.RemotingTrace("MimeTypeToFormatter: mimeType: " + mimeType);

            if (string.Compare(mimeType, SOAPMimeType, StringComparison.Ordinal) == 0)
            {
                return CreateSoapFormatter(serialize, true);
            }
            else
            if (string.Compare(mimeType, BinaryMimeType, StringComparison.Ordinal) == 0)
            {
                return CreateBinaryFormatter(serialize, true);
            }     

            return null;
        } // MimeTypeToFormatter


        //
        // Other helper methods
        //

        // Removes application name from front of uri if present.
        internal static String RemoveApplicationNameFromUri(String uri)
        {
            if (uri == null)
                return null;
        
            String appName = RemotingConfiguration.ApplicationName;
            if ((appName == null) || (appName.Length == 0))
                return uri;

            // uri must be longer than the appname plus a slash (hence the "+2")
            if (uri.Length < (appName.Length + 2))
                return uri;
            
            // case-insensitively determine if uri starts with app name
            if (String.Compare(appName, 0, uri, 0, appName.Length, StringComparison.OrdinalIgnoreCase) == 0)
            {
                // make sure a slash follows the app name (we already made sure
                //   uri was long enough above)
                if (uri[appName.Length] == '/')
                {
                    uri = uri.Substring(appName.Length + 1);
                }
            }            

            return uri;            
        } // RemoveApplicationNameFromUri


        internal static void AppendProviderToClientProviderChain(
            IClientChannelSinkProvider providerChain,
            IClientChannelSinkProvider provider)
        {
            if (providerChain == null)
                throw new ArgumentNullException("providerChain");

            // walk to last provider in chain
            while (providerChain.Next != null)
            {
                providerChain = providerChain.Next;
            }

            providerChain.Next = provider;
        } // AppendProviderToClientProviderChain

        internal static void CollectChannelDataFromServerSinkProviders(
            ChannelDataStore channelData,
            IServerChannelSinkProvider provider)
        {
            // walk chain and ask each provider for channel data
            while (provider != null)
            {
                provider.GetChannelData(channelData);
            
                provider = provider.Next;
            }
        } // CollectChannelDataFromServerSinkProviders


        // called by providers that aren't expecting custom provider data
        internal static void VerifyNoProviderData(String providerTypeName, ICollection providerData)
        {
            if ((providerData != null) && (providerData.Count > 0))
            {
                throw new RemotingException(
                    String.Format(
                        CultureInfo.CurrentCulture, CoreChannel.GetResourceString(
                            "Remoting_Providers_Config_NotExpectingProviderData"),
                        providerTypeName));
            }                    
        } // VerifyNoProviderData

        internal static void ReportUnknownProviderConfigProperty(String providerTypeName,
                                                                 String propertyName)
        {
            throw new RemotingException(
                String.Format(
                    CultureInfo.CurrentCulture, CoreChannel.GetResourceString(
                        "Remoting_Providers_Config_UnknownProperty"),
                    providerTypeName, propertyName));
        } // ReportUnknownProviderConfigProperty



        internal static SinkChannelProtocol DetermineChannelProtocol(IChannel channel)
        {
            String objectUri;
            String channelUri = channel.Parse("http://foo.com/foo", out objectUri);
            if (channelUri != null)
                return SinkChannelProtocol.Http;

            return SinkChannelProtocol.Other;
        } // DetermineChannelProtocol

        // SetupUrlBashingForIisSslIfNecessaryWorker wrapper.
        // Prevents System.Web type load for client sku installations.
        internal static bool SetupUrlBashingForIisSslIfNecessary()
        {
            bool bBashUrl;

            if (IsClientSKUInstallation)
            {
                bBashUrl = false;
            }
            else
            {
                bBashUrl = SetupUrlBashingForIisSslIfNecessaryWorker();
            }

            return bBashUrl;
        } // SetupUrlBashingForIisSslIfNecessary

        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.NoInlining)]
        internal static bool SetupUrlBashingForIisSslIfNecessaryWorker()
        {
#if FEATURE_PAL
            // Rotor doesn't support SSL.
            return false;
#else

            // If the incoming request was IIS ssl we  need to add an 
            // entry to the call context so that the ObjRef knows
            // to Microsoft the channel data.           
            // During serialization when using ssl the ObjRef url
            // must have the host name.
            
            HttpContext httpContext = HttpContext.Current;
            bool bBashUrl = false;
            
            if ((httpContext != null) && httpContext.Request.IsSecureConnection)
            {
                // create new url
                Uri requestUrl = httpContext.Request.Url;                

                StringBuilder sb = new StringBuilder(100);
                sb.Append("https://");
                sb.Append(requestUrl.Host);
                sb.Append(":");
                sb.Append(requestUrl.Port);
                sb.Append("/");
                sb.Append(RemotingConfiguration.ApplicationName);
            
                String[] bashInfo = new String[2];
                bashInfo[0] = IisHelper.ApplicationUrl;
                bashInfo[1] = sb.ToString();
                CallContext.SetData("__bashChannelUrl", bashInfo);
                bBashUrl = true;
            }

            return bBashUrl;
#endif
        } // SetupUrlBashingForIisSslIfNecessaryWorker

        internal static void CleanupUrlBashingForIisSslIfNecessary(bool bBashedUrl)
        {
#if FEATURE_PAL
            return;
#else
            if (bBashedUrl)
                CallContext.FreeNamedDataSlot("__bashChannelUrl");
#endif
        } // CleanupUrlBashingForIisSslIfNecessary

#if !FEATURE_PAL
        internal static string GetCurrentSidString() 
        {
            return WindowsIdentity.GetCurrent().User.ToString();
        }
        
        internal static string SidToString(IntPtr sidPointer) 
        {
            if (!NativeMethods.IsValidSid(sidPointer))
                throw new RemotingException(CoreChannel.GetResourceString("Remoting_InvalidSid"));
                
            StringBuilder sidString = new StringBuilder();
            IntPtr sidIdentifierAuthorityPointer = NativeMethods.GetSidIdentifierAuthority(sidPointer);            
            int lastError = Marshal.GetLastWin32Error();                                                                                 
            if (lastError != 0)        
                throw new Win32Exception(lastError);                                                           
            byte[] sidIdentifierAuthority = new byte[6];
            Marshal.Copy(sidIdentifierAuthorityPointer, sidIdentifierAuthority, 0, 6); 
            
            IntPtr subAuthorityCountPointer = NativeMethods.GetSidSubAuthorityCount(sidPointer);                    
            lastError = Marshal.GetLastWin32Error();                                                                                 
            if (lastError != 0)        
                throw new Win32Exception(lastError);                                               
            uint subAuthorityCount = (uint)Marshal.ReadByte(subAuthorityCountPointer);
                    
            if (sidIdentifierAuthority[0] != 0 && sidIdentifierAuthority[1] != 0) 
                sidString.Append(String.Format(CultureInfo.CurrentCulture, "{0:x2}{1:x2}{2:x2}{3:x2}{4:x2}{5:x2}",
                                                            sidIdentifierAuthority[0], 
                                                            sidIdentifierAuthority[1], 
                                                            sidIdentifierAuthority[2], 
                                                            sidIdentifierAuthority[3], 
                                                            sidIdentifierAuthority[4], 
                                                            sidIdentifierAuthority[5]));        
                                                                                                                        
            else 
            {
                uint number = (uint)sidIdentifierAuthority[5] +
                                    (uint)(sidIdentifierAuthority[4] << 8) +
                                    (uint)(sidIdentifierAuthority[3] << 16) +
                                    (uint)(sidIdentifierAuthority[2] << 24) ;
                
                sidString.Append(String.Format(CultureInfo.CurrentCulture, "{0:x12}", number)); 
            }            
                
            for (int index = 0; index < subAuthorityCount; ++index) 
            {
                IntPtr subAuthorityPointer = NativeMethods.GetSidSubAuthority(sidPointer, index);
                lastError = Marshal.GetLastWin32Error();                                                                                 
                if (lastError != 0)        
                    throw new Win32Exception(lastError);                                               
                    
                uint number = (uint)Marshal.ReadInt32(subAuthorityPointer);                            
                sidString.Append(String.Format(CultureInfo.CurrentCulture, "-{0:x12}", number));
            }                       
            
            return sidString.ToString();
        }    
#endif // !FEATURE_PAL
        

        //** Resource helpers ***************************************************
        internal static ResourceManager SystemResMgr;

        private static ResourceManager InitResourceManager()
        {
			if (SystemResMgr == null)
                SystemResMgr = new ResourceManager("System.Runtime.Remoting", typeof(CoreChannel).Module.Assembly);
			return SystemResMgr;
        }

        // Looks up the resource string value for key.
        // 
        internal static String GetResourceString(String key)
        {
            if (SystemResMgr == null)
                InitResourceManager();
            String s = SystemResMgr.GetString(key, null);
            Debug.Assert(s!=null, "Resource string lookup failed.  Resource name was: \""+key+"\"");
            return s;
        }
    
        //** Debug items ***************************************************
        [Conditional("_DEBUG")]
        internal static void DebugOut(String s)
        {
                InternalRemotingServices.DebugOutChnl(s);
        }

        [Conditional("_DEBUG")]
        internal static void DebugOutXMLStream(Stream stm, String tag)
        {
            /*
            This can't be done when using networked streams.
            long oldpos = stm.Position;
            stm.Position=0;
            StreamReader sr = new StreamReader(stm, Encoding.UTF8);
            String line;
            InternalRemotingServices.DebugOutChnl("\n   -----------" + tag + " OPEN-------------\n") ;
            while ((line = sr.ReadLine()) != null)
            {
                InternalRemotingServices.DebugOutChnl(line);
            }
            InternalRemotingServices.DebugOutChnl("\n   -----------" + tag + " CLOSE------------\n") ;

            stm.Position = oldpos;
            */
        }

        [Conditional("_DEBUG")]
        internal static void DebugMessage(IMessage msg)
        {
            /*
              if (msg is IMethodCallMessage)
                InternalRemotingServices.RemotingTrace("IMethodCallMessage");
                
              if (msg is IMethodReturnMessage)
                InternalRemotingServices.RemotingTrace("IMethodReturnMessage");
        
              if (msg == null)
                InternalRemotingServices.RemotingTrace("***** IMessage is null");
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here");
              IDictionary d = msg.Properties;
              if (d == null)
                InternalRemotingServices.RemotingTrace("***** Properties is null");
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here0");      
              if (d.Count == 0)
              {
                  InternalRemotingServices.RemotingTrace("Zero Properties");
                  return;
              }
        
              InternalRemotingServices.RemotingTrace("DebugMessage Here1");
              IDictionaryEnumerator e = (IDictionaryEnumerator) d.GetEnumerator();
              InternalRemotingServices.RemotingTrace("DebugMessage Here1");
        
              while (e.MoveNext())
              {
                InternalRemotingServices.RemotingTrace("DebugMessage Here2");
        
                Object key = e.Key;
                
                InternalRemotingServices.RemotingTrace("DebugMessage Here3");
        
                String keyName = key.ToString();
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here4");
        
                Object value = e.Value;
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here5");
        
                InternalRemotingServices.RemotingTrace(keyName + ":" + e.Value);
        
                InternalRemotingServices.RemotingTrace("DebugMessage Here6");
        
                if (String.Compare(keyName, "__CallContext", StringComparison.OrdinalIgnoreCase) == 0)
                {
                }
                
                InternalRemotingServices.RemotingTrace("DebugMessage Here7");
              }
              */
        }

        [Conditional("_DEBUG")]
        internal static void DebugException(String name, Exception e)
        {
            InternalRemotingServices.RemotingTrace("****************************************************\r\n");
            InternalRemotingServices.RemotingTrace("EXCEPTION THROWN!!!!!! - " + name);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.Message);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.GetType().FullName);
            InternalRemotingServices.RemotingTrace("\r\n");

            InternalRemotingServices.RemotingTrace(e.StackTrace);
            InternalRemotingServices.RemotingTrace("\r\n");
            InternalRemotingServices.RemotingTrace("****************************************************\r\n");
        }

        [Conditional("_DEBUG")]
        internal static void DebugStream(Stream stm)
        {
            /*
              try
              {
                long Position = stm.Position;
        
                MemoryStream memStm = (MemoryStream)stm;
                byte[] byteArray = memStm.ToArray();
                int byteArrayLength = byteArray.Length;
                String streamString = Encoding.ASCII.GetString(byteArray,0, byteArrayLength);
                InternalRemotingServices.RemotingTrace(streamString);
                stm.Position = Position;
              }
              catch(Exception e)
              {
                DebugException("DebugStream", e);
              }
              */
        }

    } // class CoreChannel


#if !FEATURE_PAL
    internal static class IisHelper
    {
        private static bool _bIsSslRequired = false;
        private static String _iisAppUrl = null;
        
        internal static void Initialize()
        {
            // NOTE!!!: This is only called from the synchronized initialization
            //   stage in HTTP remoting handler.
            try
            {
                HttpRequest request = HttpContext.Current.Request;
                String mdPath = request.ServerVariables["APPL_MD_PATH"];
    
                bool bSslRequired = false;
                if (mdPath.StartsWith("/LM/", StringComparison.Ordinal))
                {
                    mdPath = "IIS://localhost/" + mdPath.Substring(4);  
                    DirectoryEntry dir = new DirectoryEntry(mdPath);
                    bSslRequired = (bool)dir.Properties["AccessSSL"][0];    
                }

                _bIsSslRequired = bSslRequired;
            }
            catch
            {
                // If initialization failed, we just assume that ssl isn't required.
                // This just means that we'll have to manually Microsoft the channel data
                // everytime.         
            }
        } // Initialize
    
        internal static bool IsSslRequired
        {
            get { return _bIsSslRequired; }
        } // IsSslRequired

        internal static String ApplicationUrl
        {
            get { return _iisAppUrl; }

            set { _iisAppUrl = value; }
        } // ApplicationUrl        
        
    } // class IisHelper
#endif // !FEATURE_PAL   
    
}

