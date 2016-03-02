using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace System.Diagnostics.Contracts
{
    internal class Contract
    {
        public static void Assert(bool condition, string message = null)
        {
            Debug.Assert(condition, message);
        }

        public static void Requires(bool condition, string message = null)
        {
        }

        public static void Ensures(bool condition)
        {
        }

        public static void EndContractBlock()
        {
        }
    }
}
