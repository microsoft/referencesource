//------------------------------------------------------------------------------
//  Microsoft Avalon
//  Copyright (c) Microsoft Corporation, 2003
//
//  File:       GeometryHitTestResult
//------------------------------------------------------------------------------

using System;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;

using System.Collections;
using System.Diagnostics;
using MS.Internal;

namespace System.Windows.Media 
{
    /// <summary>
    /// This class returns the point and visual hit during a hit test pass.
    /// </summary>
    public class GeometryHitTestResult : HitTestResult
    {
        private IntersectionDetail _intersectionDetail;

        /// <summary>
        /// This constructor takes a visual and point respresenting a hit.
        /// </summary>
        public GeometryHitTestResult(
            Visual visualHit, 
            IntersectionDetail intersectionDetail) : base(visualHit)
        {
            _intersectionDetail = intersectionDetail;
        }
        
        /// <summary>
        /// The intersection detail with how geometry intersected with scene.
        /// </summary>
        public IntersectionDetail IntersectionDetail
        {
            get
            {
                return _intersectionDetail;
            }
        }

        /// <summary>
        ///     Re-expose Visual property strongly typed to 2D Visual.
        /// </summary>
        public new Visual VisualHit 
        { 
            get
            {
                return (Visual) base.VisualHit;
            }
        }
    }
}

