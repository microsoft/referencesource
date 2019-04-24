
namespace System.Net.Http
{
    // Used with HttpClientHandler to decide if client certificates should be supplied by default
    public enum ClientCertificateOption
    {
        Manual = 0, // Default
        Automatic,
    }
}
