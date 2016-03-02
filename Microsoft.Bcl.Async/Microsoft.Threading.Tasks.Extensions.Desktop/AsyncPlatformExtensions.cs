using System;
using System.Net;
using System.Net.Mail;
using System.Net.NetworkInformation;
using System.ServiceModel.Discovery;
using System.Threading.Tasks;

/// <summary>
///     Provides asynchronous wrappers for .NET Framework operations.
/// </summary>
public static partial class AsyncPlatformExtensions
{
    ///<summary>Causes an online announcement (Hello) message to be sent asynchronously with the specified endpoint discovery metadata and user-defined state. The specified  is called when the operation completes.</summary>
    ///<returns>Task instance.</returns>
    ///<param name="discoveryMetadata">The endpoint discovery metadata.</param>
    ///<param name="source">The source.</param>
    public static Task AnnounceOnlineTaskAsync(this System.ServiceModel.Discovery.AnnouncementClient source, System.ServiceModel.Discovery.EndpointDiscoveryMetadata discoveryMetadata)
    {
        return Task.Factory.FromAsync(source.BeginAnnounceOnline, source.EndAnnounceOnline, discoveryMetadata, null);
    }

    ///<summary>Causes an offline announcement (Bye) message to be sent asynchronously with the specified endpoint discovery metadata and user-defined state. The specified  is called when the operation completes.</summary>
    ///<returns>Task instance.</returns>
    ///<param name="discoveryMetadata">The endpoint discovery metadata.</param>
    ///<param name="source">The source.</param>
    public static Task AnnounceOfflineTaskAsync(this System.ServiceModel.Discovery.AnnouncementClient source, System.ServiceModel.Discovery.EndpointDiscoveryMetadata discoveryMetadata)
    {
        return Task.Factory.FromAsync(source.BeginAnnounceOffline, source.EndAnnounceOffline, discoveryMetadata, null);
    }

    ///<summary>Begins asynchronously retrieving an incoming request.</summary>
    ///<returns>Task object that indicates the status of the asynchronous operation.</returns>
    ///<exception cref="T:System.Net.HttpListenerException">A Win32 function call failed. Check the exception's  property to determine the cause of the exception.</exception>
    ///<exception cref="T:System.InvalidOperationException">This object has not been started or is currently stopped.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object is closed.</exception>
    ///<param name="source">The source.</param>
    public static Task<System.Net.HttpListenerContext> GetContextAsync(this System.Net.HttpListener source)
    {
        return Task<System.Net.HttpListenerContext>.Factory.FromAsync(source.BeginGetContext, source.EndGetContext, null);
    }

    ///<summary>Starts an asynchronous request for the client's X.509 v.3 certificate.</summary>
    ///<returns>Task that indicates the status of the operation.</returns>
    ///<param name="source">The source.</param>
    public static Task<System.Security.Cryptography.X509Certificates.X509Certificate2> GetClientCertificateAsync(this System.Net.HttpListenerRequest source)
    {
        return Task<System.Security.Cryptography.X509Certificates.X509Certificate2>.Factory.FromAsync(source.BeginGetClientCertificate, source.EndGetClientCertificate, null);
    }

    ///<summary>Called by clients to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation. </returns>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.- or -This stream was used previously to attempt authentication as the server. You cannot use the stream to retry authentication as the client.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsClientAsync(this System.Net.Security.NegotiateStream source)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsClient, source.EndAuthenticateAsClient, null);
    }

    ///<summary>Called by clients to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. The authentication process uses the specified credentials. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation. </returns>
    ///<param name="credential">The  that is used to establish the identity of the client.</param>
    ///<param name="targetName">The Service Principal Name (SPN) that uniquely identifies the server to authenticate.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null.- or -<paramref name="targetName" /> is null.</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.- or -This stream was used previously to attempt authentication as the server. You cannot use the stream to retry authentication as the client.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsClientAsync(this System.Net.Security.NegotiateStream source, System.Net.NetworkCredential credential, string targetName)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsClient, source.EndAuthenticateAsClient, credential, targetName, null);
    }

    ///<summary>Called by clients to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. The authentication process uses the specified credentials and channel binding. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation.</returns>
    ///<param name="credential">The  that is used to establish the identity of the client.</param>
    ///<param name="binding">The  that is used for extended protection.</param>
    ///<param name="targetName">The Service Principal Name (SPN) that uniquely identifies the server to authenticate.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null.- or -<paramref name="targetName" /> is null.</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.- or -This stream was used previously to attempt authentication as the server. You cannot use the stream to retry authentication as the client.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsClientAsync(this System.Net.Security.NegotiateStream source, System.Net.NetworkCredential credential, System.Security.Authentication.ExtendedProtection.ChannelBinding binding, string targetName)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsClient, source.EndAuthenticateAsClient, credential, binding, targetName, null);
    }

    ///<summary>Called by servers to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation. </returns>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<exception cref="T:System.NotSupportedException">Windows 95 and Windows 98 are not supported.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsServerAsync(this System.Net.Security.NegotiateStream source)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsServer, source.EndAuthenticateAsServer, null);
    }

    ///<summary>Called by servers to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. The authentication process uses the specified extended protection policy. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation.</returns>
    ///<param name="policy">The  that is used for extended protection.</param>
    ///<exception cref="T:System.ArgumentException">The  and <see cref="P:System.Security.Authentication.ExtendedProtection.ExtendedProtectionPolicy.CustomServiceNames" /> on the extended protection policy passed in the  parameter are both null.</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.NotSupportedException">Windows 95 and Windows 98 are not supported.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsServerAsync(this System.Net.Security.NegotiateStream source, System.Security.Authentication.ExtendedProtection.ExtendedProtectionPolicy policy)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsServer, source.EndAuthenticateAsServer, policy, null);
    }

    ///<summary>Called by servers to begin an asynchronous operation to authenticate the client, and optionally the server, in a client-server connection. The authentication process uses the specified server credentials and authentication options. This method does not block.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation. </returns>
    ///<param name="credential">The  that is used to establish the identity of the client.</param>
    ///<param name="requiredProtectionLevel">One of the  values, indicating the security services for the stream.</param>
    ///<param name="requiredImpersonationLevel">One of the  values, indicating how the server can use the client's credentials to access resources.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null.</exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException"> must be , <see cref="F:System.Security.Principal.TokenImpersonationLevel.Impersonation" />, or <see cref="F:System.Security.Principal.TokenImpersonationLevel.Delegation" />,</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.Security.Authentication.InvalidCredentialException">The authentication failed. You can use this object to retry the authentication.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.- or -This stream was used previously to attempt authentication as the client. You cannot use the stream to retry authentication as the server.</exception>
    ///<exception cref="T:System.NotSupportedException">Windows 95 and Windows 98 are not supported.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsServerAsync(this System.Net.Security.NegotiateStream source, System.Net.NetworkCredential credential, System.Net.Security.ProtectionLevel requiredProtectionLevel, System.Security.Principal.TokenImpersonationLevel requiredImpersonationLevel)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsServer, source.EndAuthenticateAsServer, credential, requiredProtectionLevel, requiredImpersonationLevel, null);
    }

    ///<summary>Called by clients to begin an asynchronous operation to authenticate the server and optionally the client.</summary>
    ///<returns>Task object that indicates the status of the asynchronous operation. </returns>
    ///<param name="targetHost">The name of the server that shares this .</param>
    ///<exception cref="T:System.ArgumentNullException"> is null.</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed and left this object in an unusable state.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.-or-Server authentication using this  was tried previously.-or- Authentication is already in progress.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsClientAsync(this System.Net.Security.SslStream source, string targetHost)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsClient, source.EndAuthenticateAsClient, targetHost, null);
    }

    ///<summary>Called by servers to begin an asynchronous operation to authenticate the client and optionally the server in a client-server connection.</summary>
    ///<returns>Task object indicating the status of the asynchronous operation. </returns>
    ///<param name="serverCertificate">The X509Certificate used to authenticate the server.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null.</exception>
    ///<exception cref="T:System.Security.Authentication.AuthenticationException">The authentication failed and left this object in an unusable state.</exception>
    ///<exception cref="T:System.InvalidOperationException">Authentication has already occurred.-or-Client authentication using this  was tried previously.-or- Authentication is already in progress.</exception>
    ///<exception cref="T:System.ObjectDisposedException">This object has been closed.</exception>
    ///<exception cref="T:System.PlatformNotSupportedException">The  method is not supported on Windows 95, Windows 98, or Windows Millennium.</exception>
    ///<param name="source">The source.</param>
    public static Task AuthenticateAsServerAsync(this System.Net.Security.SslStream source, System.Security.Cryptography.X509Certificates.X509Certificate serverCertificate)
    {
        return Task.Factory.FromAsync(source.BeginAuthenticateAsServer, source.EndAuthenticateAsServer, serverCertificate, null);
    }

    ///<summary>Starts an asynchronous request for a remote host connection. The host is specified by a host name and a port number.</summary>
    ///<returns>Task that represents the asynchronous connection.</returns>
    ///<param name="hostname">The name of the remote host.</param>
    ///<param name="port">The port number of the remote host.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The  has been closed. </exception>
    ///<exception cref="T:System.NotSupportedException">This method is valid for sockets in the  or  families.</exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException">The port number is not valid.</exception>
    ///<exception cref="T:System.InvalidOperationException">The  is ing.</exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.EnvironmentPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Diagnostics.PerformanceCounterPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Net.SocketPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task ConnectAsync(this System.Net.Sockets.TcpClient source, string hostname, int port)
    {
        return Task.Factory.FromAsync(source.BeginConnect, source.EndConnect, hostname, port, null);
    }

    ///<summary>Starts an asynchronous request for a remote host connection. The host is specified by an  and a port number.</summary>
    ///<returns>Task that represents the asynchronous connection.</returns>
    ///<param name="address">The  of the remote host.</param>
    ///<param name="port">The port number of the remote host.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.Net.Sockets.SocketException">An error occurred when attempting to access the socket. See the Remarks section for more information. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The  has been closed. </exception>
    ///<exception cref="T:System.NotSupportedException">The  is not in the socket family.</exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException">The port number is not valid.</exception>
    ///<exception cref="T:System.ArgumentException">The length of  is zero.</exception>
    ///<exception cref="T:System.InvalidOperationException">The  is ing.</exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.EnvironmentPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Diagnostics.PerformanceCounterPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Net.SocketPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task ConnectAsync(this System.Net.Sockets.TcpClient source, System.Net.IPAddress address, int port)
    {
        return Task.Factory.FromAsync(source.BeginConnect, source.EndConnect, address, port, null);
    }

    ///<summary>Starts an asynchronous request for a remote host connection. The host is specified by an  array and a port number.</summary>
    ///<returns>Task that represents the asynchronous connections.</returns>
    ///<param name="ipAddresses">At least one , designating the remote host.</param>
    ///<param name="port">The port number of the remote host.</param>
    ///<exception cref="T:System.ArgumentNullException"> is null. </exception>
    ///<exception cref="T:System.Net.Sockets.SocketException">An error occurred when attempting to access the socket. See the Remarks section for more information. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The  has been closed. </exception>
    ///<exception cref="T:System.NotSupportedException">This method is valid for sockets that use  or .</exception>
    ///<exception cref="T:System.ArgumentOutOfRangeException">The port number is not valid.</exception>
    ///<exception cref="T:System.ArgumentException">The length of  is zero.</exception>
    ///<exception cref="T:System.InvalidOperationException">The  is ing.</exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.EnvironmentPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Diagnostics.PerformanceCounterPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Net.SocketPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task ConnectAsync(this System.Net.Sockets.TcpClient source, System.Net.IPAddress[] ipAddresses, int port)
    {
        return Task.Factory.FromAsync(source.BeginConnect, source.EndConnect, ipAddresses, port, null);
    }

    ///<summary>Starts an asynchronous operation to accept an incoming connection attempt.</summary>
    ///<returns>Task that represents the asynchronous creation of the <see cref="T:System.Net.Sockets.Socket" />.</returns>
    ///<exception cref="T:System.Net.Sockets.SocketException">An error occurred while attempting to access the socket. See the Remarks section for more information. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The  has been closed. </exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.EnvironmentPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Flags="UnmanagedCode, ControlEvidence" />    <IPermission class="System.Diagnostics.PerformanceCounterPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task<System.Net.Sockets.Socket> AcceptSocketAsync(this System.Net.Sockets.TcpListener source)
    {
        return Task<System.Net.Sockets.Socket>.Factory.FromAsync(source.BeginAcceptSocket, source.EndAcceptSocket, null);
    }

    ///<summary>Starts an asynchronous operation to accept an incoming connection attempt.</summary>
    ///<returns>Task that represents the asynchronous creation of the <see cref="T:System.Net.Sockets.TcpClient" />.</returns>
    ///<exception cref="T:System.Net.Sockets.SocketException">An error occurred while attempting to access the socket. See the Remarks section for more information. </exception>
    ///<exception cref="T:System.ObjectDisposedException">The  has been closed. </exception>
    ///<PermissionSet>    <IPermission class="System.Security.Permissions.EnvironmentPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />    <IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Flags="UnmanagedCode, ControlEvidence" />    <IPermission class="System.Diagnostics.PerformanceCounterPermission, System, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Unrestricted="true" />  </PermissionSet>
    ///<param name="source">The source.</param>
    public static Task<System.Net.Sockets.TcpClient> AcceptTcpClientAsync(this System.Net.Sockets.TcpListener source)
    {
        return Task<System.Net.Sockets.TcpClient>.Factory.FromAsync(source.BeginAcceptTcpClient, source.EndAcceptTcpClient, null);
    }

    ///<summary>Sends a datagram to a destination asynchronously. The destination is specified by a .</summary>
    ///<returns>Task object that represents the asynchronous send.</returns>
    ///<param name="datagram">A  array that contains the data to be sent.</param>
    ///<param name="bytes">The number of bytes to send.</param>
    ///<param name="endPoint">The  that represents the destination for the data.</param>
    ///<param name="source">The source.</param>
    public static Task<int> SendAsync(this System.Net.Sockets.UdpClient source, byte[] datagram, int bytes, System.Net.IPEndPoint endPoint)
    {
        return Task<int>.Factory.FromAsync(source.BeginSend, source.EndSend, datagram, bytes, endPoint, null);
    }

    ///<summary>Sends a datagram to a remote host asynchronously. The destination was specified previously by a call to .</summary>
    ///<returns>Task object that represents the asynchronous send.</returns>
    ///<param name="datagram">A  array that contains the data to be sent.</param>
    ///<param name="bytes">The number of bytes to send.</param>
    ///<param name="source">The source.</param>
    public static Task<int> SendAsync(this System.Net.Sockets.UdpClient source, byte[] datagram, int bytes)
    {
        return Task<int>.Factory.FromAsync(source.BeginSend, source.EndSend, datagram, bytes, null);
    }

    ///<summary>Sends a datagram to a remote host asynchronously. The destination was specified previously by a call to .</summary>
    ///<returns>Task object that represents the asynchronous send.</returns>
    ///<param name="datagram">A  array that contains the data to be sent.</param>
    ///<param name="bytes">The number of bytes to send.</param>
    ///<param name="hostname">The host name.</param>
    ///<param name="port">The host name.</param>
    ///<param name="source">The source.</param>
    public static Task<int> SendAsync(this System.Net.Sockets.UdpClient source, byte[] datagram, int bytes, string hostname, int port)
    {
        return Task<int>.Factory.FromAsync((callback, state) => source.BeginSend(datagram, bytes, hostname, port, callback, state), source.EndSend, null);
    }

    ///<summary>Starts an asynchronous request to retrieve the stable unicast IP address table on the local computer.</summary>
    ///<returns>Task that represents the asynchronous request.</returns>
    ///<exception cref="T:System.NotImplementedException">This method is not implemented on the platform. This method uses the native NotifyStableUnicastIpAddressTable function that is supported on Windows Vista and later. </exception>
    ///<exception cref="T:System.ComponentModel.Win32Exception">The call to the native NotifyStableUnicastIpAddressTable function failed.</exception>
    ///<param name="source">The source.</param>
    public static Task<System.Net.NetworkInformation.UnicastIPAddressInformationCollection> GetUnicastAddressesAsync(this System.Net.NetworkInformation.IPGlobalProperties source)
    {
        return Task<System.Net.NetworkInformation.UnicastIPAddressInformationCollection>.Factory.FromAsync(source.BeginGetUnicastAddresses, source.EndGetUnicastAddresses, null);
    }

    /// <summary>Opens the connection asynchronously.</summary>
    /// <param name="source">The source.</param>
    /// <returns>Task that represents the asynchronous request.</returns>
    public static Task OpenAsync(this System.Data.SqlClient.SqlConnection source)
    {
        return OpenAsync(source, System.Threading.CancellationToken.None);
    }

    /// <summary>Opens the connection asynchronously.</summary>
    /// <param name="source">The source.</param>
    /// <param name="cancellationToken">The cancellation token.</param>
    ///<returns>Task that represents the asynchronous request.</returns>
    public static Task OpenAsync(this System.Data.SqlClient.SqlConnection source, System.Threading.CancellationToken cancellationToken)
    {
        return Task.Factory.StartNew(() => source.Open(), cancellationToken, TaskCreationOptions.None, TaskScheduler.Default);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this , given a callback procedure and state information.</summary>
    ///<returns>Task that can be used to poll or wait for results, or both; this value is also needed when invoking <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteNonQuery(System.IAsyncResult)" />, which returns the number of affected rows.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="source">The source.</param>
    public static Task<int> ExecuteNonQueryAsync(this System.Data.SqlClient.SqlCommand source)
    {
        return ExecuteNonQueryAsync(source, System.Threading.CancellationToken.None);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this , given a callback procedure and state information.</summary>
    ///<returns>Task that can be used to poll or wait for results, or both; this value is also needed when invoking <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteNonQuery(System.IAsyncResult)" />, which returns the number of affected rows.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="cancellationToken">The cancellation token.</param>
    ///<param name="source">The source.</param>
    public static Task<int> ExecuteNonQueryAsync(this System.Data.SqlClient.SqlCommand source, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation<int>(cancellationToken);
        return Task<int>.Factory.FromAsync(source.BeginExecuteNonQuery, source.EndExecuteNonQuery, null);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this  and returns results as an <see cref="T:System.Xml.XmlReader" /> object, using a callback procedure.</summary>
    ///<returns>Task that can be used to poll, wait for results, or both; this value is also needed when the <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteXmlReader(System.IAsyncResult)" /> is called, which returns the results of the command as XML.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="source">The source.</param>
    public static Task<System.Xml.XmlReader> ExecuteXmlReaderAsync(this System.Data.SqlClient.SqlCommand source)
    {
        return ExecuteXmlReaderAsync(source, System.Threading.CancellationToken.None);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this  and returns results as an <see cref="T:System.Xml.XmlReader" /> object, using a callback procedure.</summary>
    ///<returns>Task that can be used to poll, wait for results, or both; this value is also needed when the <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteXmlReader(System.IAsyncResult)" /> is called, which returns the results of the command as XML.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="cancellationToken">The cancellation token.</param>
    ///<param name="source">The source.</param>
    public static Task<System.Xml.XmlReader> ExecuteXmlReaderAsync(this System.Data.SqlClient.SqlCommand source, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation<System.Xml.XmlReader>(cancellationToken);
        return Task<System.Xml.XmlReader>.Factory.FromAsync(source.BeginExecuteXmlReader, source.EndExecuteXmlReader, null);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this  and retrieves one or more result sets from the server, given a callback procedure and state information.</summary>
    ///<returns>Task that can be used to poll, wait for results, or both; this value is also needed when invoking <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteReader(System.IAsyncResult)" />, which returns a <see cref="T:System.Data.SqlClient.SqlDataReader" /> instance which can be used to retrieve the returned rows.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="source">The source.</param>
    public static Task<System.Data.SqlClient.SqlDataReader> ExecuteReaderAsync(this System.Data.SqlClient.SqlCommand source)
    {
        return ExecuteReaderAsync(source, System.Threading.CancellationToken.None);
    }

    ///<summary>Initiates the asynchronous execution of the Transact-SQL statement or stored procedure that is described by this  and retrieves one or more result sets from the server, given a callback procedure and state information.</summary>
    ///<returns>Task that can be used to poll, wait for results, or both; this value is also needed when invoking <see cref="M:System.Data.SqlClient.SqlCommand.EndExecuteReader(System.IAsyncResult)" />, which returns a <see cref="T:System.Data.SqlClient.SqlDataReader" /> instance which can be used to retrieve the returned rows.</returns>
    ///<exception cref="T:System.Data.SqlClient.SqlException">Any error that occurred while executing the command text.</exception>
    ///<exception cref="T:System.InvalidOperationException">The name/value pair "Asynchronous Processing=true" was not included within the connection string defining the connection for this .</exception>
    ///<filterpriority>2</filterpriority>
    ///<param name="cancellationToken">The cancellation token.</param>
    ///<param name="source">The source.</param>
    public static Task<System.Data.SqlClient.SqlDataReader> ExecuteReaderAsync(this System.Data.SqlClient.SqlCommand source, System.Threading.CancellationToken cancellationToken)
    {
        if (cancellationToken.IsCancellationRequested)
            return TaskServices.FromCancellation<System.Data.SqlClient.SqlDataReader>(cancellationToken);
        return Task<System.Data.SqlClient.SqlDataReader>.Factory.FromAsync(source.BeginExecuteReader, source.EndExecuteReader, null);
    }

    ///<summary>Starts an asynchronous method call that returns a .</summary>
    ///<returns>The metadata.</returns>
    ///<param name="source">The source.</param>
    public static Task<System.ServiceModel.Description.MetadataSet> GetMetadataAsync(this System.ServiceModel.Description.MetadataExchangeClient source)
    {
        return Task<System.ServiceModel.Description.MetadataSet>.Factory.FromAsync(source.BeginGetMetadata, source.EndGetMetadata, null);
    }

    ///<summary>Starts an asynchronous method call that returns a  using the specified address, callback, asynchronous state, and download mechanism.</summary>
    ///<returns>The metadata obtained from the specified .</returns>
    ///<param name="address">The address of the metadata.</param>
    ///<param name="mode">The  value to use when downloading the metadata.</param>
    ///<param name="source">The source.</param>
    public static Task<System.ServiceModel.Description.MetadataSet> GetMetadataAsync(this System.ServiceModel.Description.MetadataExchangeClient source, System.Uri address, System.ServiceModel.Description.MetadataExchangeClientMode mode)
    {
        return Task<System.ServiceModel.Description.MetadataSet>.Factory.FromAsync(source.BeginGetMetadata, source.EndGetMetadata, address, mode, null);
    }

    ///<summary>Starts an asynchronous method call that returns a  using the specified address, callback, and asynchronous state.</summary>
    ///<returns>The metadata obtained from the specified .</returns>
    ///<param name="address">The address of the metadata.</param>
    ///<param name="source">The source.</param>
    public static Task<System.ServiceModel.Description.MetadataSet> GetMetadataAsync(this System.ServiceModel.Description.MetadataExchangeClient source, System.ServiceModel.EndpointAddress address)
    {
        return Task<System.ServiceModel.Description.MetadataSet>.Factory.FromAsync(source.BeginGetMetadata, source.EndGetMetadata, address, null);
    }

    /// <summary>
    /// Begins an asynchronous find operation with the specified criteria.
    /// </summary>
    /// <param name="discoveryClient">The discovery client.</param>
    /// <param name="criteria">The criteria for finding services.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<FindResponse> FindTaskAsync(
        this DiscoveryClient discoveryClient, FindCriteria criteria)
    {
        // Validate arguments
        if (discoveryClient == null) throw new ArgumentNullException("discoveryClient");

        // Create a TaskCompletionSource to represent the operation
        var tcs = new TaskCompletionSource<FindResponse>(discoveryClient);

        // Register a handler that will transfer completion results to the TCS Task
        EventHandler<FindCompletedEventArgs> completedHandler = null;
        completedHandler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => discoveryClient.FindCompleted -= completedHandler);
        discoveryClient.FindCompleted += completedHandler;

        // Start the async operation.
        try { discoveryClient.FindAsync(criteria, tcs); }
        catch
        {
            discoveryClient.FindCompleted -= completedHandler;
            throw;
        }

        // Return the task to represent the asynchronous operation
        return tcs.Task;
    }

    /// <summary>
    /// Begins an asynchronous resolve operation with the specified criteria.
    /// </summary>
    /// <param name="discoveryClient">The discovery client.</param>
    /// <param name="criteria">The criteria for matching a service endpoint.</param>
    /// <returns>A Task that represents the asynchronous operation.</returns>
    public static Task<ResolveResponse> ResolveTaskAsync(
        this DiscoveryClient discoveryClient, ResolveCriteria criteria)
    {
        // Validate arguments
        if (discoveryClient == null) throw new ArgumentNullException("discoveryClient");

        // Create a TaskCompletionSource to represent the operation
        var tcs = new TaskCompletionSource<ResolveResponse>(discoveryClient);

        // Register a handler that will transfer completion results to the TCS Task
        EventHandler<ResolveCompletedEventArgs> completedHandler = null;
        completedHandler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Result, () => discoveryClient.ResolveCompleted -= completedHandler);
        discoveryClient.ResolveCompleted += completedHandler;

        // Start the async operation.
        try { discoveryClient.ResolveAsync(criteria, tcs); }
        catch
        {
            discoveryClient.ResolveCompleted -= completedHandler;
            throw;
        }

        // Return the task to represent the asynchronous operation
        return tcs.Task;
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="address">An IPAddress that identifies the computer that is the destination for the ICMP echo message.</param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, IPAddress address)
    {
        return SendTaskAsyncCore(ping, address, tcs => ping.SendAsync(address, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="hostNameOrAddress">
    /// A String that identifies the computer that is the destination for the ICMP echo message. 
    /// The value specified for this parameter can be a host name or a string representation of an IP address.
    /// </param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, string hostNameOrAddress)
    {
        return SendTaskAsyncCore(ping, hostNameOrAddress, tcs => ping.SendAsync(hostNameOrAddress, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="address">An IPAddress that identifies the computer that is the destination for the ICMP echo message.</param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, IPAddress address, int timeout)
    {
        return SendTaskAsyncCore(ping, address, tcs => ping.SendAsync(address, timeout, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="hostNameOrAddress">
    /// A String that identifies the computer that is the destination for the ICMP echo message. 
    /// The value specified for this parameter can be a host name or a string representation of an IP address.
    /// </param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, string hostNameOrAddress, int timeout)
    {
        return SendTaskAsyncCore(ping, hostNameOrAddress, tcs => ping.SendAsync(hostNameOrAddress, timeout, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="address">An IPAddress that identifies the computer that is the destination for the ICMP echo message.</param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <param name="buffer">
    /// A Byte array that contains data to be sent with the ICMP echo message and returned 
    /// in the ICMP echo reply message. The array cannot contain more than 65,500 bytes.
    /// </param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, IPAddress address, int timeout, byte[] buffer)
    {
        return SendTaskAsyncCore(ping, address, tcs => ping.SendAsync(address, timeout, buffer, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="hostNameOrAddress">
    /// A String that identifies the computer that is the destination for the ICMP echo message. 
    /// The value specified for this parameter can be a host name or a string representation of an IP address.
    /// </param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <param name="buffer">
    /// A Byte array that contains data to be sent with the ICMP echo message and returned 
    /// in the ICMP echo reply message. The array cannot contain more than 65,500 bytes.
    /// </param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, string hostNameOrAddress, int timeout, byte[] buffer)
    {
        return SendTaskAsyncCore(ping, hostNameOrAddress, tcs => ping.SendAsync(hostNameOrAddress, timeout, buffer, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="address">An IPAddress that identifies the computer that is the destination for the ICMP echo message.</param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <param name="buffer">
    /// A Byte array that contains data to be sent with the ICMP echo message and returned 
    /// in the ICMP echo reply message. The array cannot contain more than 65,500 bytes.
    /// </param>
    /// <param name="options">A PingOptions object used to control fragmentation and Time-to-Live values for the ICMP echo message packet.</param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, IPAddress address, int timeout, byte[] buffer, PingOptions options)
    {
        return SendTaskAsyncCore(ping, address, tcs => ping.SendAsync(address, timeout, buffer, options, tcs));
    }

    /// <summary>
    /// Asynchronously attempts to send an Internet Control Message Protocol (ICMP) echo message.
    /// </summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="hostNameOrAddress">
    /// A String that identifies the computer that is the destination for the ICMP echo message. 
    /// The value specified for this parameter can be a host name or a string representation of an IP address.
    /// </param>
    /// <param name="timeout">
    /// An Int32 value that specifies the maximum number of milliseconds (after sending the echo message) 
    /// to wait for the ICMP echo reply message.
    /// </param>
    /// <param name="buffer">
    /// A Byte array that contains data to be sent with the ICMP echo message and returned 
    /// in the ICMP echo reply message. The array cannot contain more than 65,500 bytes.
    /// </param>
    /// <param name="options">A PingOptions object used to control fragmentation and Time-to-Live values for the ICMP echo message packet.</param>
    /// <returns>A task that represents the asynchronous operation.</returns>
    public static Task<PingReply> SendTaskAsync(this Ping ping, string hostNameOrAddress, int timeout, byte[] buffer, PingOptions options)
    {
        return SendTaskAsyncCore(ping, hostNameOrAddress, tcs => ping.SendAsync(hostNameOrAddress, timeout, buffer, options, tcs));
    }

    /// <summary>The core implementation of SendTaskAsync.</summary>
    /// <param name="ping">The Ping.</param>
    /// <param name="userToken">A user-defined object stored in the resulting Task.</param>
    /// <param name="sendAsync">
    /// A delegate that initiates the asynchronous send.
    /// The provided TaskCompletionSource must be passed as the user-supplied state to the actual Ping.SendAsync method.
    /// </param>
    /// <returns></returns>
    private static Task<PingReply> SendTaskAsyncCore(Ping ping, object userToken, Action<TaskCompletionSource<PingReply>> sendAsync)
    {
        // Validate arguments
        if (ping == null) throw new ArgumentNullException("ping");

        // Create a TaskCompletionSource to represent the operation
        var tcs = new TaskCompletionSource<PingReply>(userToken);

        // Register a handler that will transfer completion results to the TCS Task
        PingCompletedEventHandler handler = null;
        handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => e.Reply, () => ping.PingCompleted -= handler);
        ping.PingCompleted += handler;

        // Start the async operation.
        try { sendAsync(tcs); }
        catch
        {
            ping.PingCompleted -= handler;
            throw;
        }

        // Return the task to represent the asynchronous operation
        return tcs.Task;
    }

    /// <summary>Sends an e-mail message asynchronously.</summary>
    /// <param name="smtpClient">The client.</param>
    /// <param name="from">A String that contains the address information of the message sender.</param>
    /// <param name="recipients">A String that contains the address that the message is sent to.</param>
    /// <param name="subject">A String that contains the subject line for the message.</param>
    /// <param name="body">A String that contains the message body.</param>
    /// <returns>A Task that represents the asynchronous send.</returns>
    public static Task SendTaskAsync(this SmtpClient smtpClient, string from, string recipients, string subject, string body)
    {
        var message = new MailMessage(from, recipients, subject, body);
        return SendTaskAsync(smtpClient, message);
    }

    /// <summary>Sends an e-mail message asynchronously.</summary>
    /// <param name="smtpClient">The client.</param>
    /// <param name="message">A MailMessage that contains the message to send.</param>
    /// <returns>A Task that represents the asynchronous send.</returns>
    public static Task SendTaskAsync(this SmtpClient smtpClient, MailMessage message)
    {
        return SendTaskAsyncCore(smtpClient, message, tcs => smtpClient.SendAsync(message, tcs));
    }

    /// <summary>The core implementation of SendTaskAsync.</summary>
    /// <param name="smtpClient">The client.</param>
    /// <param name="userToken">The user-supplied state.</param>
    /// <param name="sendAsync">
    /// A delegate that initiates the asynchronous send.
    /// The provided TaskCompletionSource must be passed as the user-supplied state to the actual SmtpClient.SendAsync method.
    /// </param>
    /// <returns></returns>
    private static Task SendTaskAsyncCore(SmtpClient smtpClient, object userToken, Action<TaskCompletionSource<object>> sendAsync)
    {
        // Validate arguments
        if (smtpClient == null) throw new ArgumentNullException("smtpClient");

        // Create a TaskCompletionSource to represent the operation
        var tcs = new TaskCompletionSource<object>(userToken);

        // Register a handler that will transfer completion results to the TCS Task
        SendCompletedEventHandler handler = null;
        handler = (sender, e) => TaskServices.HandleEapCompletion(tcs, true, e, () => null, () => smtpClient.SendCompleted -= handler);
        smtpClient.SendCompleted += handler;

        // Start the async operation.
        try { sendAsync(tcs); }
        catch
        {
            smtpClient.SendCompleted -= handler;
            throw;
        }

        // Return the task to represent the asynchronous operation
        return tcs.Task;
    }
}
