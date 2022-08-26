//------------------------------------------------------------------------------
//  Microsoft Windows Presentation Foudnation
//  Copyright (c) Microsoft Corporation, 2009
//
// Description:
//      Definition of the ICyclicBrush interface used to interact with Brush
//	objects whose content can point back into the Visual tree.
//
//------------------------------------------------------------------------------

using System;
using System.Windows.Media;
using System.Windows.Media.Composition;

namespace System.Windows.Media
{
    internal interface ICyclicBrush
    {
        void FireOnChanged();

        void RenderForCyclicBrush(DUCE.Channel channel, bool skipChannelCheck);
    }
}
