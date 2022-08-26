using System;
using System.Collections.Generic;
using System.Text;

namespace System.Windows.Media.Effects
{
    internal struct BitmapEffectInputData
    {
        public BitmapEffect BitmapEffect;
        public BitmapEffectInput BitmapEffectInput;

        public BitmapEffectInputData(BitmapEffect bitmapEffect,
                                    BitmapEffectInput bitmapEffectInput)
        {
            BitmapEffect = bitmapEffect;
            BitmapEffectInput = bitmapEffectInput;
        }
    }
}
