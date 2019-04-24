using System;
using System.Collections.Generic;
using System.Text;

namespace System.Net.Http
{
    public enum HttpCompletionOption
    {
        ResponseContentRead = 0, // default        
        ResponseHeadersRead,
    }
}
