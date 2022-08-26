// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Remoting.Services
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.IO;
    using System.Reflection;
    using System.Net;
    using System.Runtime.Remoting;
    using System.Runtime.Remoting.Channels;
    using System.Runtime.Remoting.Channels.Http;
    using System.Runtime.Remoting.Messaging;
    using System.Runtime.InteropServices;

    [ComVisible(true)]
    public abstract class RemotingClientProxy : Component
    {
        protected Type _type;
        protected Object _tp;
        protected String _url;

        protected void ConfigureProxy(Type type, String url)
        {
            lock(this)
            {               
                // Initial URL Address embedded during codegen i.e. SUDSGenerator
                // User use in stockQuote.Url = "http://............." which reconnects the tp
                _type = type;
                this.Url = url;
            }
        }

        protected void ConnectProxy()
        {
            lock(this)
            {
                _tp = null;
                _tp = Activator.GetObject(_type, _url);
            }
        }

        //[DefaultValue(false), Description("Enable automatic handling of server redirects.")]
        public bool AllowAutoRedirect
        {
            get { return(bool)ChannelServices.GetChannelSinkProperties(_tp)["allowautoredirect"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["allowautoredirect"] = value;}
        }

        //[Browsable(false), Persistable(PersistableSupport.None), Description("The cookies received from the server that will be sent back on requests that match the cookie's path.  EnableCookies must be true.")]
        //public CookieCollection Cookies
        public Object Cookies
        {
            get { return null; }
        }

        //[DefaultValue(true), Description("Enables handling of cookies received from the server.")]
        public bool EnableCookies
        {
            get { return false; }
            set { throw new NotSupportedException(); }
        }

        //[DefaultValue(false), Description("Enables pre authentication of the request.")]
        public bool PreAuthenticate
        {
            get { return(bool)ChannelServices.GetChannelSinkProperties(_tp)["preauthenticate"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["preauthenticate"] = value;}
        }

        //[DefaultValue(""), RecommendedAsConfigurable(true), Description("The base URL to the server to use for requests.")]
        public String Path
        {
            get { return Url; }
            set { Url = value; }
        }

        //[DefaultValue(-1), RecommendedAsConfigurable(true), Description("Sets the timeout in milliseconds to be used for synchronous calls.  The default of -1 means infinite.")]
        public int Timeout
        {
            get { return (int)ChannelServices.GetChannelSinkProperties(_tp)["timeout"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["timeout"] = value;}
        }

//
//<

        public String Url
        {
            get
            {
                //return (String)ChannelServices.GetChannelSinkProperties(_tp)["Url"];
                return _url;
            }
            set
            {
                lock(this)
                {
                    _url = value;
                }
                ConnectProxy();
                ChannelServices.GetChannelSinkProperties(_tp)["url"] = value;
            }
        }

        //[Description("Sets the user agent http header for the request.")]
        public String UserAgent
        {
            get { return HttpClientTransportSink.UserAgent;}
            set { throw  new NotSupportedException(); }
        }

        //[DefaultValue(""), Description("The user name to be sent for basic and digest authentication.")]
        public String Username
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["username"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["username"] = value;}
        }

        //[DefaultValue(""), Description("The password to be used for basic and digest authentication.")]
        public String Password
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["password"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["password"] = value;}
        }

        //[DefaultValue(""), Description("The domain to be used for basic and digest authentication.")]
        public String Domain
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["domain"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["domain"] = value;}
        }

        //[DefaultValue(""), Description("The name of the proxy server to use for requests.")]
        public String ProxyName
        {
            get { return(String)ChannelServices.GetChannelSinkProperties(_tp)["proxyname"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["Proxyname"] = value;}
        }

        //[DefaultValue(80), Description("The port number of the proxy server to use for requests.")]
        public int ProxyPort {
            get { return(int)ChannelServices.GetChannelSinkProperties(_tp)["proxyport"];}
            set { ChannelServices.GetChannelSinkProperties(_tp)["proxyport"] = value;}
        }
    }
}
