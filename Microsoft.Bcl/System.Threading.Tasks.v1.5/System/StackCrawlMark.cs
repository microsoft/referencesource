using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Threading
{
    /// <summary>
    /// A dummy replacement for the .NET internal class StackCrawlMark.
    /// </summary>
    internal struct StackCrawlMark
    {
        internal static StackCrawlMark LookForMyCaller
        {
            get { return new StackCrawlMark(); }
        }
    }
}
