//---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation.  All rights reserved.
// 
// File: ColumnPropertiesGroup.cs
//
// Description: Column properties group. 
//
// History:  
//  05/05/2003 : Microsoft - created.
//  07/18/2003 : Microsoft - converted to new Property Engine.
//
//---------------------------------------------------------------------------

using System;
using System.Diagnostics;
using System.Windows;
using System.Windows.Media;
using System.Windows.Documents;
using MS.Internal.Text;

namespace MS.Internal.PtsHost
{
    // ----------------------------------------------------------------------
    // Column properties group.
    // ----------------------------------------------------------------------
    internal sealed class ColumnPropertiesGroup
    {
        // ------------------------------------------------------------------
        // Constructor.
        // Remarks - the pageWidth parameter can be used to limit column
        // properties if the element is a FlowDocument.
        // ------------------------------------------------------------------
        internal ColumnPropertiesGroup(DependencyObject o)
        {
            _columnWidth = (double)o.GetValue(FlowDocument.ColumnWidthProperty);
            _columnGap = (double)o.GetValue(FlowDocument.ColumnGapProperty);
            _columnRuleWidth = (double)o.GetValue(FlowDocument.ColumnRuleWidthProperty);
            _columnRuleBrush = (Brush)o.GetValue(FlowDocument.ColumnRuleBrushProperty);
            _isColumnWidthFlexible = (bool)o.GetValue(FlowDocument.IsColumnWidthFlexibleProperty);
        }

        // ------------------------------------------------------------------
        // Column width.
        // ------------------------------------------------------------------
        internal double ColumnWidth { get { Debug.Assert(!double.IsNaN(_columnWidth)); return _columnWidth; } }
        private double _columnWidth;

        // ------------------------------------------------------------------
        // Flexible column width.
        // ------------------------------------------------------------------
        internal bool IsColumnWidthFlexible { get { return _isColumnWidthFlexible; } }
        private bool _isColumnWidthFlexible;

        // ------------------------------------------------------------------
        // Column space distribution.
        // ------------------------------------------------------------------
        internal ColumnSpaceDistribution ColumnSpaceDistribution { get { return ColumnSpaceDistribution.Between; } }
        
        // ------------------------------------------------------------------
        // Column gap.
        // ------------------------------------------------------------------
        internal double ColumnGap 
        { 
            get 
            { 
                Invariant.Assert(!double.IsNaN(_columnGap)); 
                return _columnGap; 
            } 
        }
        private double _columnGap;

        // ------------------------------------------------------------------
        // Column rule brush.
        // ------------------------------------------------------------------
        internal Brush ColumnRuleBrush { get { return _columnRuleBrush; } }
        private Brush _columnRuleBrush;

        // ------------------------------------------------------------------
        // Column rule width.
        // ------------------------------------------------------------------
        internal double ColumnRuleWidth { get { return _columnRuleWidth; } }
        private double _columnRuleWidth;

        // ------------------------------------------------------------------
        // Column width is set?
        // ------------------------------------------------------------------
        internal bool ColumnWidthAuto { get { return DoubleUtil.IsNaN(_columnWidth); } }

        // ------------------------------------------------------------------
        // Column gap is set?
        // ------------------------------------------------------------------
        internal bool ColumnGapAuto
        {
            get
            {
                return DoubleUtil.IsNaN(_columnGap);
            }
        }
    }
}
