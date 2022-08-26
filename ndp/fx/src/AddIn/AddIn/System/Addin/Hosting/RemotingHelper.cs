using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.Remoting.Channels;
using System.Runtime.Remoting.Channels.Ipc;

namespace System.AddIn.Hosting
{

    internal static class RemotingHelper
    {
        // runtime initializes this to false;
        static bool CreatedInAD;
        static readonly Object s_lock = new Object();
        internal static readonly String s_emptyGuid = Guid.Empty.ToString();

    
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="BinaryServerFormatterSinkProvider..ctor()" />
        // <SatisfiesLinkDemand Name="BinaryServerFormatterSinkProvider.set_TypeFilterLevel(System.Runtime.Serialization.Formatters.TypeFilterLevel):System.Void" />
        // <SatisfiesLinkDemand Name="BinaryClientFormatterSinkProvider..ctor()" />
        // <SatisfiesLinkDemand Name="IpcChannel..ctor(System.Collections.IDictionary,System.Runtime.Remoting.Channels.IClientChannelSinkProvider,System.Runtime.Remoting.Channels.IServerChannelSinkProvider)" />
        // <Asserts Name="Declarative: [SecurityPermission(SecurityAction.Assert, Flags = SecurityPermissionFlag.RemotingConfiguration)]" />
        // </SecurityKernel>
        [System.Security.SecuritySafeCritical]
        [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Assert, 
                Flags = System.Security.Permissions.SecurityPermissionFlag.RemotingConfiguration | System.Security.Permissions.SecurityPermissionFlag.Infrastructure)]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security","CA2116:AptcaMethodsShouldOnlyCallAptcaMethods")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2129:SecurityTransparentCodeShouldNotReferenceNonpublicSecurityCriticalCode", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2128:SecurityTransparentCodeShouldNotAssert", Justification = "This is a SecurityRules.Level1 assembly, in which this rule is being incorrectly applied")]
        internal static void InitializeClientChannel()
        {
            lock(s_lock)
            {
                if (CreatedInAD)                    // only one client channel per AD
                    return;
                CreatedInAD = true;                 // make sure this hasn't been created already in this AD

                // 
                BinaryServerFormatterSinkProvider serverProvider = new BinaryServerFormatterSinkProvider();
                serverProvider.TypeFilterLevel = System.Runtime.Serialization.Formatters.TypeFilterLevel.Full;
                BinaryClientFormatterSinkProvider clientProvider = new BinaryClientFormatterSinkProvider();
                System.Collections.IDictionary props = new System.Collections.Hashtable();
                props["name"] = "ClientChannel";
                props["portName"] = Guid.NewGuid().ToString();
                props["typeFilterLevel"] = "Full";

                // When communicating between application domains on the same computer, the ipc channel is much faster than the tcp
                IChannel ichannel = new AddInIpcChannel(props, clientProvider, serverProvider);

                // (
                ChannelServices.RegisterChannel(ichannel, false);
            }
        }

        // AddinServer is a singleton per process. It is created at process creation.
        // The AddinServer is the Marshaled class used to hook up the Client and Server remoting channel.
        // <SecurityKernel Critical="True" Ring="0">
        // <SatisfiesLinkDemand Name="Activator.GetObject(System.Type,System.String):System.Object" />
        // </SecurityKernel>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2122:DoNotIndirectlyExposeMethodsWithLinkDemands", Justification = "Reviewed")]
        [System.Security.SecuritySafeCritical]
        internal static AddInServer GetAddInServer(string guid)
        {
            System.Diagnostics.Contracts.Contract.Requires(guid != null && guid != s_emptyGuid);

            return (AddInServer)Activator.GetObject(Type.GetType(@"System.AddIn.Hosting.AddInServer")
                    , @"ipc://" + guid + @"/AddInServer");
        }   
    }   

}
