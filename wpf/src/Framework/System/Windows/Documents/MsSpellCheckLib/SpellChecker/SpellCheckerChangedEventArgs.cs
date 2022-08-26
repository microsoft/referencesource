//---------------------------------------------------------------------------
//
// File: SpellCheckerChangedEventArgs.cs
//
// Description: EventArgs class that supports SpellCheckerChangedEventHandler.
//
//---------------------------------------------------------------------------

using System;

namespace System.Windows.Documents
{
    namespace MsSpellCheckLib
    {
        internal partial class SpellChecker
        {
            /// <summary>
            /// This EventArgs type supports SpellCheckerChangedEventHandler. 
            /// </summary>
            internal class SpellCheckerChangedEventArgs : EventArgs
            {
                internal SpellCheckerChangedEventArgs(SpellChecker spellChecker)
                {
                    SpellChecker = spellChecker;
                }

                internal SpellChecker SpellChecker { get; private set; }
            }
        }
    }
}
