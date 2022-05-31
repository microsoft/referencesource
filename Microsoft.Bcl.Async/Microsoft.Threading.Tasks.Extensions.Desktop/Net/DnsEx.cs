using System;
using System.Threading.Tasks;

namespace System.Net
{
    /// <summary>Provides asynchronous wrappers for the <see cref="Dns"/> class.</summary>
    public static class DnsEx
    {
        /// <summary>Asynchronously returns the Internet Protocol (IP) addresses for the specified host.</summary>
        /// <param name="hostNameOrAddress">The host name or IP address to resolve.</param>
        /// <returns>An array of type System.Net.IPAddress that holds the IP addresses for the host specified.</returns>
        public static Task<IPAddress[]> GetHostAddressesAsync(string hostNameOrAddress)
        {
            return Task<IPAddress[]>.Factory.FromAsync(Dns.BeginGetHostAddresses, Dns.EndGetHostAddresses, hostNameOrAddress, null);
        }

        /// <summary>Asynchronously resolves an IP address to an System.Net.IPHostEntry instance.</summary>
        /// <param name="address">The IP address to resolve.</param>
        /// <returns>An System.Net.IPHostEntry instance that contains address information about the host.</returns>
        public static Task<IPHostEntry> GetHostEntryAsync(IPAddress address)
        {
            return Task<IPHostEntry>.Factory.FromAsync(Dns.BeginGetHostEntry, Dns.EndGetHostEntry, address, null);
        }

        /// <summary>Asynchronously resolves an IP address to an System.Net.IPHostEntry instance.</summary>
        /// <param name="hostNameOrAddress">The host name or IP address to resolve.</param>
        /// <returns>An System.Net.IPHostEntry instance that contains address information about the host.</returns>
        public static Task<IPHostEntry> GetHostEntryAsync(string hostNameOrAddress)
        {
            return Task<IPHostEntry>.Factory.FromAsync(Dns.BeginGetHostEntry, Dns.EndGetHostEntry, hostNameOrAddress, null);
        }
    }
}
