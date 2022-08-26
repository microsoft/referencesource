using System;
using MS.Internal.WindowsBase;

namespace MS.Internal.KnownBoxes
{
    [FriendAccessAllowed] // Built into Base, also used by Core and Framework.
    internal static class BooleanBoxes
    {
        internal static object TrueBox = true;
        internal static object FalseBox = false;

        internal static object Box(bool value)
        {
            if (value)
            {
                return TrueBox;
            }
            else
            {
                return FalseBox;
            }
        }
    }

}
