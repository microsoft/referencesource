// IKeyFrameAnimation.cs

using System.Collections;

namespace System.Windows.Media.Animation
{
    /// <summary>
    /// This interface should be implemented by all key frame animations to
    /// provide untyped access to the key frame collection.
    /// </summary>
    public interface IKeyFrameAnimation
    {
        /// <summary>
        /// The key frame collection associated with the key frame animation.
        /// </summary>
        /// <value></value>
        IList KeyFrames { get; set; }
    }
}

