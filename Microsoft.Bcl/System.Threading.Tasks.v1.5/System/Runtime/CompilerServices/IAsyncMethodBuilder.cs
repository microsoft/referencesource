using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Runtime.CompilerServices
{
    /// <summary>Represents an asynchronous method builder.</summary>
    internal interface IAsyncMethodBuilder
    {
        void PreBoxInitialization();
    }
}
