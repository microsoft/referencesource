//------------------------------------------------------------------------------
// <copyright file="ImageInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
namespace System.Drawing {
    using System.Threading;

    using System;
    using System.Diagnostics;
    using System.Drawing.Imaging;

    /// <devdoc>
    ///     Animates one or more images that have time-based frames.
    ///     This file contains the nested ImageInfo class - See ImageAnimator.cs for the definition of the outer class.
    /// </devdoc>                                   
    public sealed partial class ImageAnimator {
        /// <devdoc> 
        ///     ImageAnimator nested helper class used to store extra image state info.
        /// </devdoc>  
        private class ImageInfo {
            const int PropertyTagFrameDelay = 0x5100;

            Image image;
            int frame;
            int frameCount;
            bool frameDirty;
            bool animated;
            EventHandler onFrameChangedHandler;
            int[] frameDelay;
            int frameTimer;

            /// <devdoc> 
            /// </devdoc>  
            public ImageInfo(Image image) {
                this.image = image;
                animated = ImageAnimator.CanAnimate(image);

                if (animated) {
                    frameCount = image.GetFrameCount(FrameDimension.Time);

                    PropertyItem frameDelayItem = image.GetPropertyItem(PropertyTagFrameDelay);

                    // If the image does not have a frame delay, we just return 0.                                     
                    //
                    if (frameDelayItem != null) {
                        // Convert the frame delay from byte[] to int
                        //
                        byte[] values = frameDelayItem.Value;
                        Debug.Assert(values.Length == 4 * FrameCount, "PropertyItem has invalid value byte array");
                        frameDelay = new int[FrameCount];
                        for (int i=0; i < FrameCount; ++i) {
                            frameDelay[i] = values[i * 4] + 256 * values[i * 4 + 1] + 256 * 256 * values[i * 4 + 2] + 256 * 256 * 256 * values[i * 4 + 3];
                        }
                    }
                }
                else {
                    frameCount = 1;
                }
                if (frameDelay == null) {
                    frameDelay = new int[FrameCount];
                }
            }                                               

            /// <devdoc> 
            ///     Whether the image supports animation.
            /// </devdoc>  
            public bool Animated {
                get {
                    return animated;
                }
            }

            /// <devdoc> 
            ///     The current frame.
            /// </devdoc> 
            public int Frame {
                get {
                    return frame;
                }                
                set {
                    if (frame != value) {
                        if (value < 0 || value >= FrameCount) {
                            throw new ArgumentException(SR.GetString(SR.InvalidFrame), "value");
                        }

                        if (Animated) {
                            frame = value;
                            frameDirty = true;

                            OnFrameChanged(EventArgs.Empty);
                        }
                    }
                }
            }

            /// <devdoc> 
            ///     The current frame has not been updated.
            /// </devdoc> 
            public bool FrameDirty {
                get {
                    return frameDirty;
                }
            }

            /// <devdoc> 
            /// </devdoc> 
            public EventHandler FrameChangedHandler {
                get {
                    return onFrameChangedHandler;
                }
                set {
                    onFrameChangedHandler = value;
                }
            }

            /// <devdoc> 
            ///     The number of frames in the image.
            /// </devdoc> 
            public int FrameCount {
                get {
                    return frameCount;
                }
            }

            /// <devdoc> 
            ///     The delay associated with the frame at the specified index.
            /// </devdoc> 
            public int FrameDelay(int frame) {
                return frameDelay[frame];
            }

            /// <devdoc> 
            /// </devdoc> 
            internal int FrameTimer {
                get {
                    return frameTimer;
                }
                set {
                    frameTimer = value;
                }
            }

            /// <devdoc> 
            ///     The image this object wraps.
            /// </devdoc> 
            internal Image Image {
                get {
                    return image;                
                }
            }

            /// <devdoc> 
            ///     Selects the current frame as the active frame in the image.
            /// </devdoc> 
            internal void UpdateFrame() {
                if (frameDirty) {
                    image.SelectActiveFrame(FrameDimension.Time, Frame);
                    frameDirty = false;
                }
            }

            /// <devdoc> 
            ///     Raises the FrameChanged event.
            /// </devdoc> 
            protected void OnFrameChanged(EventArgs e) {
                if( this.onFrameChangedHandler != null ){
                    this.onFrameChangedHandler(image, e);
                }
            }
        }
    }
}
