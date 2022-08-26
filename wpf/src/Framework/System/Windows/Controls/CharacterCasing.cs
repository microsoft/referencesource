//---------------------------------------------------------------------------
//
// File: CharacterCasing.cs
//
// Description: Enum for TextBoxBase.CharacterCasing type.
//
//---------------------------------------------------------------------------

namespace System.Windows.Controls
{
    /// <summary>
    /// Specifies the case of characters in a TextBox control when
    /// the text is typed.
    /// </summary>
    public enum CharacterCasing
    {
        /// <summary>
        /// Don't convert the typed character's case.
        /// </summary>
        Normal = 0,

        /// <summary>
        /// Convert typed character to the lower case.
        /// </summary>
        Lower = 1,

        /// <summary>
        /// Convert typed character to the upper case.
        /// </summary>
        Upper = 2
    }
}
