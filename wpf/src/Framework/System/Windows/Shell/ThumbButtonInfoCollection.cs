/**************************************************************************\
    Copyright Microsoft Corporation. All Rights Reserved.
\**************************************************************************/

namespace System.Windows.Shell
{
    using System.Windows;

    public class ThumbButtonInfoCollection : FreezableCollection<ThumbButtonInfo>
    {
        protected override Freezable CreateInstanceCore()
        {
            return new ThumbButtonInfoCollection();
        }

        /// <summary>
        /// A frozen empty ThumbButtonInfoCollection.
        /// </summary>
        internal static ThumbButtonInfoCollection Empty
        {
            get
            {
                if (s_empty == null)
                {
                    var collection = new ThumbButtonInfoCollection();
                    collection.Freeze();
                    s_empty = collection;
                }

                return s_empty;
            }
        }

        private static ThumbButtonInfoCollection s_empty;
    }
}
