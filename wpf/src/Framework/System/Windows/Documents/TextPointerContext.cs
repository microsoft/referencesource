//---------------------------------------------------------------------------
//
// File=TextPointerContext.cs
//
// Description: A block of text stored in a TextContainer.
//
//---------------------------------------------------------------------------

namespace System.Windows.Documents
{
    /// <summary>
    /// Specifies the category of content to one side of a TextPointer.
    /// </summary>
    public enum TextPointerContext
    {
        /// <summary>
        /// Adjacent to the beginning or end of content.
        /// </summary>
        None,
        /// <summary>
        /// Adjacent to text.
        /// </summary>
        Text,
        /// <summary>
        /// Adjacent to an embedded element, a UIElement or ContentElement.
        /// </summary>
        EmbeddedElement,
        /// <summary>
        /// Adjacent to the start edge of a TextElement.
        /// </summary>
        ElementStart,
        /// <summary>
        /// Adjacent to the end edge of a TextElement.
        /// </summary>
        ElementEnd,
    };
}
