// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace System.Runtime.Remoting.Services 
{
    using System.Diagnostics;
    using System.Web;
    using System.ComponentModel;
    using System.Web.SessionState;
    using System.Security.Principal;
    using System.Runtime.Remoting.Channels;

    public class RemotingService : Component 
	{
        public HttpApplicationState Application 
		{
            get {
                return Context.Application;
            }
        }

        public HttpContext Context 
		{
            get {            
                HttpContext context = HttpContext.Current;
                if (context == null)
                    throw new RemotingException(CoreChannel.GetResourceString("Remoting_HttpContextNotAvailable"));
                return context;
            }
        }

        public HttpSessionState Session 
		{
            get {
                return Context.Session;
            }
        }

        public HttpServerUtility Server 
		{
            get {
                return Context.Server;
            }
        }       

        public IPrincipal User 
		{
            get {
                return Context.User;
            }
        }
    }
}
