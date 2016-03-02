using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Threading.Tasks
{
    /// <summary>
    /// An interface similar to the one added in .NET 4.0.
    /// </summary>
    internal interface IThreadPoolWorkItem
    {
        void ExecuteWorkItem();
    }
}
