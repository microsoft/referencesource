using System;
using System.Diagnostics.CodeAnalysis;

namespace System.Windows.Forms {
    /// <include file='doc\SearchDirectionHint.uex' path='docs/doc[@for="SearchDirectionHint"]/*' />
    /// <devdoc>
    ///
    /// </devdoc>
    [
        SuppressMessage("Microsoft.Design", "CA1008:EnumsShouldHaveZeroValue")  // Maps to native enum.
    ]
    public enum SearchDirectionHint {
        /// <include file='doc\SearchDirectionHint.uex' path='docs/doc[@for="SearchDirectionHint.Up"]/*' />
        /// <devdoc>
        ///
        /// </devdoc>
        Up = NativeMethods.VK_UP,
        /// <include file='doc\SearchDirectionHint.uex' path='docs/doc[@for="SearchDirectionHint.Down"]/*' />
        /// <devdoc>
        ///
        /// </devdoc>
        Down = NativeMethods.VK_DOWN,
        /// <include file='doc\SearchDirectionHint.uex' path='docs/doc[@for="SearchDirectionHint.Left"]/*' />
        /// <devdoc>
        ///
        /// </devdoc>
        Left = NativeMethods.VK_LEFT,
        /// <include file='doc\SearchDirectionHint.uex' path='docs/doc[@for="SearchDirectionHint.Right"]/*' />
        /// <devdoc>
        ///
        /// </devdoc>
        Right = NativeMethods.VK_RIGHT
    }
}
