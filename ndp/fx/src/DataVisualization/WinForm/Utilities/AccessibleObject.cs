//-------------------------------------------------------------
// <copyright company=’Microsoft Corporation’>
//   Copyright © Microsoft Corporation. All Rights Reserved.
// </copyright>
//-------------------------------------------------------------
// @owner=alexgor

//=================================================================
//  File:		AccessibleObject.cs
//
//  Namespace:	System.Windows.Forms.DataVisualization.Charting.Utilities
//
//	Classes:	ChartAccessibleObject
//
//  Purpose:	Chart control accessible object.
//
//	Reviewed:	
//
//===================================================================

#if WINFORMS_CONTROL

#region Used namespaces

using System;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Globalization;

#endregion // Used namespaces

namespace System.Windows.Forms.DataVisualization.Charting.Utilities
{
    internal class ChartAccessibleObject : Control.ControlAccessibleObject
    {
        #region Fields

        // Reference to the chart control
        private Chart _chart = null;

        // List of chart accessible objects
        private List<AccessibleObject> _chartAccessibleObjectList = null;

        // Position of the chart in screen coordinates (optianl can be set to empty)
        private Point _chartScreenPosition = Point.Empty;

        // Chart scaleView transformation matrix
        private PointF _chartScale = new PointF(1f, 1f);

        #endregion // Fields

        #region Constructors

        /// <summary>
        /// Object constructor.
        /// </summary>
        /// <param name="chart">Reference to the chart control.</param>
        public ChartAccessibleObject(Chart chart) : base(chart)
        {
            this._chart = chart;
        }

        #endregion // Constructors

        #region Properties

        /// <summary>
        /// Position of the chart in screen coordinates (optianl can be set to empty)
        /// </summary>
        public Point ChartScreenPosition
        {
            get
            {
                return this._chartScreenPosition;
            }
        }
        
        /// <summary>
        /// Gets the role for the Chart. This is used by accessibility programs.
        /// </summary>
        public override AccessibleRole Role
        {
            get
            {
                return AccessibleRole.Chart;
            }
        }

        #endregion // Properties

        #region Methods

        /// <summary>
        /// Indicates if chart child accessibility objects should be reset
        /// </summary>
        public void ResetChildren()
        {
            this._chartAccessibleObjectList = null;
        }

        /// <summary>
        /// Chart child count.
        /// </summary>
        /// <returns>Number of chart child eleements.</returns>
        public override int GetChildCount()
        {
            // Fill list of chart accessible child elements
            if (this._chartAccessibleObjectList == null)
            {
                this.FillChartAccessibleObjectList();
            }
            return _chartAccessibleObjectList.Count;
        }

        /// <summary>
        /// Get chart child element by index.
        /// </summary>
        /// <param name="index">Index of the chart child element.</param>
        /// <returns>Chart element accessibility object.</returns>
        public override AccessibleObject GetChild(int index)
        {
            // Fill list of chart accessible child elements
            if (this._chartAccessibleObjectList == null)
            {
                this.FillChartAccessibleObjectList();
            }

            // Return accessible object by index
            if (index >= 0 && index < this._chartAccessibleObjectList.Count)
            {
                return this._chartAccessibleObjectList[index];
            }                
            return null;
        }

        /// <summary>
        /// Creates a list of chart accessible child elements.
        /// </summary>
        /// <returns>List of chart accessible child elements.</returns>
        private void FillChartAccessibleObjectList()
        {
            // Create new list
            this._chartAccessibleObjectList = new List<AccessibleObject>();

            // Chart reference must set first
            if (this._chart != null)
            {
                // Add all Titles into the list
                foreach (Title title in this._chart.Titles)
                {
                    this._chartAccessibleObjectList.Add(new ChartChildAccessibleObject(
                        this, 
                        this,
                        title, 
                        ChartElementType.Title, 
                        SR.AccessibilityTitleName(title.Name), 
                        title.Text, 
                        AccessibleRole.StaticText));
                }

                // Add all Legends into the list
                foreach (Legend legend in this._chart.Legends)
                {
                    this._chartAccessibleObjectList.Add(new ChartChildLegendAccessibleObject(this, legend));
                }

                // Add all Chart Areas into the list
                foreach (ChartArea chartArea in this._chart.ChartAreas)
                {
                    this._chartAccessibleObjectList.Add(new ChartChildChartAreaAccessibleObject(this, chartArea));
                }

                // Add all annotations into the list
                foreach (Annotation annotation in this._chart.Annotations)
                {
                    TextAnnotation textAnnotation = annotation as TextAnnotation;
                    if (textAnnotation != null)
                    {
                        this._chartAccessibleObjectList.Add(new ChartChildAccessibleObject(
                            this, 
                            this,
                            annotation, 
                            ChartElementType.Annotation,
                            SR.AccessibilityAnnotationName(annotation.Name), 
                            textAnnotation.Text, 
                            AccessibleRole.StaticText));
                    }
                    else
                    {
                        this._chartAccessibleObjectList.Add(new ChartChildAccessibleObject(
                            this, 
                            this,
                            annotation, 
                            ChartElementType.Annotation,
                            SR.AccessibilityAnnotationName(annotation.Name), 
                            string.Empty, 
                            AccessibleRole.Graphic));
                    }
                }
            }
        }

        /// <summary>
        /// Navigates from specified child into specified direction.
        /// </summary>
        /// <param name="chartChildElement">Chart child element.</param>
        /// <param name="chartElementType">Chart child element type.</param>
        /// <param name="direction">Navigation direction.</param>
        /// <returns>Accessibility object we just navigated to.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "direction"), 
        System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartElementType"), 
        System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartChildElement")]
        public AccessibleObject NavigateFromChild(object chartChildElement, ChartElementType chartElementType, AccessibleNavigation direction)
        {
            // TODO: Not Implemented. Requires Selection Manager code changes. Remove CodeAnalysis.SuppressMessageAttributes
            return null;
        }

        /// <summary>
        /// Selects child chart element.
        /// </summary>
        /// <param name="chartChildElement">Chart child element.</param>
        /// <param name="chartElementType">Chart child element type.</param>
        /// <param name="selection">Selection actin.</param>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "selection"), 
        System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartElementType"), 
        System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartChildElement")]
        public void SelectChild(object chartChildElement, ChartElementType chartElementType, AccessibleSelection selection)
        {
            // TODO: Not Implemented. Requires Selection Manager code changes. Remove CodeAnalysis.SuppressMessageAttributes
        }

        /// <summary>
        /// Checks if specified chart child element is selected.
        /// </summary>
        /// <param name="chartChildElement">Chart child element.</param>
        /// <param name="chartElementType">Chart child element type.</param>
        /// <returns>True if child is selected.</returns>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartElementType"),
        System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Usage", "CA1801:ReviewUnusedParameters", MessageId = "chartChildElement")]
        public bool IsChildSelected(object chartChildElement, ChartElementType chartElementType)
        {
            // TODO: Not Implemented. Requires Selection Manager code changes. Remove CodeAnalysis.SuppressMessageAttributes
            return false;
        }

        /// <summary>
        /// Gets chart child element bounds position in screen coordinates
        /// </summary>
        /// <param name="chartElement">Chart child element.</param>
        /// <param name="chartElementType">Chart child element type.</param>
        /// <param name="seriesName">Series name.</param>
        /// <param name="pointIndex">Series data point index.</param>
        /// <returns>Element boundary in screen coordinates.</returns>
        public Rectangle GetChildBounds(object chartElement, ChartElementType chartElementType, string seriesName, int pointIndex)
        {
            // Make sure we have a valid reference on the chart control
            Rectangle result = Rectangle.Empty;
            if (this._chart != null &&
                this._chart.chartPicture != null &&
                this._chart.chartPicture.Common != null &&
                this._chart.chartPicture.Common.HotRegionsList != null)
            {
                // Execute chart hit test to initialize list of chart element positions
                if (this._chart.chartPicture.Common.HotRegionsList.List == null ||
                    this._chart.chartPicture.Common.HotRegionsList.List.Count == 0)
                {
                    this._chart.HitTest(0, 0);
                }

                // Find specified chart element in the list
                foreach (HotRegion hotRegion in this._chart.chartPicture.Common.HotRegionsList.List)
                {
                    if (hotRegion.Type == chartElementType)
                    {
                        // Determine if region should be processed
                        bool processRegion = false;
                        if (chartElementType == ChartElementType.DataPoint || chartElementType == ChartElementType.DataPointLabel)
                        {
                            // In case of data point and data point label their series name and label should match
                            if (hotRegion.SeriesName == seriesName && hotRegion.PointIndex == pointIndex)
                            {
                                processRegion = true;
                            }

                        }
                        else if (hotRegion.SelectedObject == chartElement || hotRegion.SelectedSubObject == chartElement)
                        {
                            processRegion = true;
                        }

                        if (processRegion)
                        {
                            RectangleF bounds = hotRegion.BoundingRectangle;



                            // Conver chart relative coordinates to chart absolute (pixel) coordinates
                            if (hotRegion.RelativeCoordinates)
                            {
                                RectangleF absolute = RectangleF.Empty;
                                absolute.X = bounds.X * (this._chart.Width - 1) / 100F;
                                absolute.Y = bounds.Y * (this._chart.Height - 1) / 100F;
                                absolute.Width = bounds.Width * (this._chart.Width - 1) / 100F;
                                absolute.Height = bounds.Height * (this._chart.Height - 1) / 100F;
                                bounds = absolute;
                            }

                            // Check if chart should be scaled
                            Rectangle rect = Rectangle.Round(bounds);
                            if (this._chartScale.X != 1f || this._chartScale.Y != 1f)
                            {
                                SizeF rectSize = rect.Size;
                                rect.X = (int)(rect.X * this._chartScale.X);
                                rect.Y = (int)(rect.Y * this._chartScale.Y);

                                rectSize.Width *= this._chartScale.X;
                                rectSize.Height *= this._chartScale.Y;
                                rect.Size = Size.Round(rectSize);
                            }

                            // Convert to screen coordinates
                            if (!this.ChartScreenPosition.IsEmpty)
                            {
                                rect.Offset(this.ChartScreenPosition);
                            }
                            else
                            {
                                rect = this._chart.RectangleToScreen(rect);
                            }

                            // If elementd is not gridlines just return the rectangle
                            if (chartElementType != ChartElementType.Gridlines)
                            {
                                return rect;
                            }

                            // For gridlines continue accumulation all gridlines positions
                            if (result.IsEmpty)
                            {
                                result = rect;
                            }
                            else
                            {
                                result = Rectangle.Union(result, rect);
                            }
                        }

                    }
                }
            }
            return result;
        }

        #endregion // Methods
    }

    /// <summary>
    /// Chart child element accessible object
    /// </summary>
    internal class ChartChildAccessibleObject : AccessibleObject
    {
        #region Fields

        // Chart element presented by this accessibility object
        internal object chartChildObject = null;

        // Chart child object type
        internal ChartElementType chartChildObjectType = ChartElementType.Nothing;

        // Chart accessibility object
        internal ChartAccessibleObject chartAccessibleObject = null;

        // Chart accessibility object
        internal AccessibleObject chartAccessibleParentObject = null;

        // Accessible object role
        internal AccessibleRole role = AccessibleRole.StaticText;

        // Accessible object value
        internal string name = string.Empty;

        // Accessible object name
        internal string objectValue = string.Empty;

        // Series name
        protected string seriesName = string.Empty;

        // Data point index
        internal int dataPointIndex = -1;

        #endregion // Fields

        #region Constructors

        /// <summary>
        /// Initializes chart element accessibility object with specified title.
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessibility object.</param>
        /// <param name="chartAccessibleParentObject">The chart accessible parent object.</param>
        /// <param name="chartChildObject">Chart child object.</param>
        /// <param name="chartChildObjectType">Chart child object type.</param>
        /// <param name="name">Chart child object name.</param>
        /// <param name="objectValue">Chart child object value.</param>
        /// <param name="role">Chart child object role.</param>
        public ChartChildAccessibleObject(
            ChartAccessibleObject chartAccessibleObject,
            AccessibleObject chartAccessibleParentObject, 
            object chartChildObject, 
            ChartElementType chartChildObjectType,
            string name,
            string objectValue,
            AccessibleRole role)
        {
            this.chartAccessibleObject = chartAccessibleObject;
            this.chartAccessibleParentObject = chartAccessibleParentObject;
            this.chartChildObject = chartChildObject;
            this.chartChildObjectType = chartChildObjectType;
            this.name = name;
            this.role = role;
            this.objectValue = objectValue;
        }

        /// <summary>
        /// Initializes chart element accessibility object with specified title.
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessibility object.</param>
        /// <param name="chartAccessibleParentObject">The chart accessible parent object.</param>
        /// <param name="chartChildObject">Chart child object.</param>
        /// <param name="chartChildObjectType">Chart child object type.</param>
        /// <param name="name">Chart child object name.</param>
        /// <param name="objectValue">Chart child object value.</param>
        /// <param name="role">Chart child object role.</param>
        /// <param name="seriesName">Chart series name.</param>
        /// <param name="pointIndex">Chart data point index.</param>
        public ChartChildAccessibleObject(
            ChartAccessibleObject chartAccessibleObject,
            AccessibleObject chartAccessibleParentObject, 
            object chartChildObject,
            ChartElementType chartChildObjectType,
            string name,
            string objectValue,
            AccessibleRole role,
            string seriesName,
            int pointIndex)
        {
            this.chartAccessibleObject = chartAccessibleObject;
            this.chartAccessibleParentObject = chartAccessibleParentObject;
            this.chartChildObject = chartChildObject;
            this.chartChildObjectType = chartChildObjectType;
            this.name = name;
            this.role = role;
            this.objectValue = objectValue;
            this.seriesName = seriesName;
            this.dataPointIndex = pointIndex;
        }

        #endregion // Constructors

        #region Properties

        /// <summary>
        /// Gets the Bounds for the accessible object.
        /// </summary>
        public override Rectangle Bounds
        {
            get
            {
                return this.chartAccessibleObject.GetChildBounds(this.chartChildObject, this.chartChildObjectType, this.seriesName, this.dataPointIndex);
            }
        }

        /// <summary>
        /// Gets parent accessible object
        /// </summary>
        public override AccessibleObject Parent
        {
            get
            {
                if (this.chartAccessibleParentObject != null)
                {
                    return this.chartAccessibleParentObject;
                }
                return this.chartAccessibleObject;
            }
        }

        /// <summary>
        /// Object value
        /// </summary>
        public override string Value
        {
            get
            {
                return this.objectValue;
            }
            set
            {
                this.objectValue = value;
            }
        }

        /// <summary>
        /// Gets accessible object role
        /// </summary>
        public override AccessibleRole Role
        {
            get
            {
                return this.role;
            }
        }

        /// <summary>
        /// Gets accessible object state.
        /// </summary>
        public override AccessibleStates State
        {
            get
            {
                AccessibleStates state = AccessibleStates.Selectable;
                if (this.chartAccessibleObject.IsChildSelected(this.chartChildObject, this.chartChildObjectType))
                {
                    state |= AccessibleStates.Selected;
                }
                return state;
            }
        }

        /// <summary>
        /// Gets the name of the accessibility object.
        /// </summary>
        public override string Name
        {
            get
            {
                return this.name;
            }
            set
            {
                this.name = value;
            }
        }

        #endregion // Properties

        #region Methods

        /// <summary>
        /// Navigate through chart child objects.
        /// </summary>
        /// <param name="direction">Navigation direction.</param>
        /// <returns>Accessibility object to navigate to.</returns>
        public override AccessibleObject Navigate(AccessibleNavigation direction)
        {
            return this.chartAccessibleObject.NavigateFromChild(this.chartChildObject, this.chartChildObjectType, direction);
        }

        /// <summary>
        /// Selects chart child element.
        /// </summary>
        /// <param name="selection">Element to select.</param>
        public override void Select(AccessibleSelection selection)
        {
            this.chartAccessibleObject.SelectChild(this.chartChildObject, this.chartChildObjectType, selection);
        }

        #endregion // Methods
    }

    /// <summary>
    /// Chart legend element accessible object
    /// </summary>
    internal class ChartChildLegendAccessibleObject : ChartChildAccessibleObject
    {
        #region Fields

        // List of child accessible objects
        private List<ChartChildAccessibleObject> _childList = new List<ChartChildAccessibleObject>();

        #endregion // Fields

        #region Constructor

        /// <summary>
        /// Object constructor.
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessible object.</param>
        /// <param name="legend">Chart legend object.</param>
        public ChartChildLegendAccessibleObject(ChartAccessibleObject chartAccessibleObject, Legend legend) 
            : base(
                chartAccessibleObject,
                chartAccessibleObject,
                legend, ChartElementType.LegendArea,
                SR.AccessibilityLegendName(legend.Name), 
                string.Empty, 
                AccessibleRole.StaticText)
        {
            // Add legend title as a child element
            if (legend.Title.Length > 0)
            {
                this._childList.Add(new ChartChildAccessibleObject(
                            chartAccessibleObject, 
                            this,
                            legend, ChartElementType.LegendTitle,
                            SR.AccessibilityLegendTitleName(legend.Name), 
                            legend.Title, 
                            AccessibleRole.StaticText));
            }

            // NOTE: Legend items are dynamically generated and curently are not part of the list
        }

        #endregion // Constructor

        #region Methods

        /// <summary>
        /// Gets child accessible object.
        /// </summary>
        /// <param name="index">Index of the child object to get.</param>
        /// <returns>Chart child accessible object.</returns>
        public override AccessibleObject GetChild(int index)
        {
            if (index >= 0 && index < this._childList.Count)
            {
                return this._childList[index];
            }
            return null;
        }

        /// <summary>
        /// Get number of chart accessible objects.
        /// </summary>
        /// <returns>Number of chart accessible objects.</returns>
        public override int GetChildCount()
        {
            return this._childList.Count;
        }

        #endregion // Methods
    }

    /// <summary>
    /// Chart area element accessible object
    /// </summary>
    internal class ChartChildChartAreaAccessibleObject : ChartChildAccessibleObject
    {
        #region Fields

        // List of child accessible objects
        private List<ChartChildAccessibleObject> _childList = new List<ChartChildAccessibleObject>();

        #endregion // Fields

        #region Constructor

        /// <summary>
        /// Object constructor.
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessible object.</param>
        /// <param name="chartArea">Chart area object.</param>
        public ChartChildChartAreaAccessibleObject(ChartAccessibleObject chartAccessibleObject, ChartArea chartArea)
            : base(
                chartAccessibleObject,
                chartAccessibleObject,
                chartArea, ChartElementType.PlottingArea,
                SR.AccessibilityChartAreaName(chartArea.Name), 
                string.Empty, 
                AccessibleRole.Graphic)
        {
            // Add all series shown in the chart area
            List<Series> areaSeries = chartArea.GetSeries();
            foreach (Series series in areaSeries)
            {
                this._childList.Add(new ChartChildSeriesAccessibleObject(chartAccessibleObject, this, series));
            }

            // Add all axes
            this.AddAxisAccessibilityObjects(chartAccessibleObject, chartArea.AxisX);
            this.AddAxisAccessibilityObjects(chartAccessibleObject, chartArea.AxisY);
            this.AddAxisAccessibilityObjects(chartAccessibleObject, chartArea.AxisX2);
            this.AddAxisAccessibilityObjects(chartAccessibleObject, chartArea.AxisY2);
        }

        #endregion // Constructor

        #region Methods

        /// <summary>
        /// Helper method which adds all accessibility objects for specified axis
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessibility object.</param>
        /// <param name="axis">Axis to add accessibility for.</param>
        private void AddAxisAccessibilityObjects(ChartAccessibleObject chartAccessibleObject, Axis axis)
        {
            // Y axis plus title
            if (axis.enabled)
            {
                this._childList.Add(new ChartChildAccessibleObject(
                    chartAccessibleObject,
                    this,
                    axis,
                    ChartElementType.Axis,
                    axis.Name,
                    string.Empty,
                    AccessibleRole.Graphic));

                if (axis.Title.Length > 0)
                {
                    this._childList.Add(new ChartChildAccessibleObject(
                        chartAccessibleObject,
                        this,
                        axis,
                        ChartElementType.AxisTitle,
                        SR.AccessibilityChartAxisTitleName(axis.Name),
                        axis.Title,
                        AccessibleRole.StaticText));
                }


                if (axis.MajorGrid.Enabled)
                {
                    this._childList.Add(new ChartChildAccessibleObject(
                        chartAccessibleObject,
                        this,
                        axis.MajorGrid,
                        ChartElementType.Gridlines,
                        SR.AccessibilityChartAxisMajorGridlinesName(axis.Name),
                        string.Empty,
                        AccessibleRole.Graphic));
                }

                if (axis.MinorGrid.Enabled)
                {
                    this._childList.Add(new ChartChildAccessibleObject(
                        chartAccessibleObject,
                        this,
                        axis.MinorGrid,
                        ChartElementType.Gridlines,
                        SR.AccessibilityChartAxisMinorGridlinesName(axis.Name),
                        string.Empty,
                        AccessibleRole.Graphic));
                }
            }
        }

        /// <summary>
        /// Gets child accessible object.
        /// </summary>
        /// <param name="index">Index of the child object to get.</param>
        /// <returns>Chart child accessible object.</returns>
        public override AccessibleObject GetChild(int index)
        {
            if (index >= 0 && index < this._childList.Count)
            {
                return this._childList[index];
            }
            return null;
        }

        /// <summary>
        /// Get number of chart accessible objects.
        /// </summary>
        /// <returns>Number of chart accessible objects.</returns>
        public override int GetChildCount()
        {
            return this._childList.Count;
        }

        #endregion // Methods
    }

    /// <summary>
    /// Chart series element accessible object
    /// </summary>
    internal class ChartChildSeriesAccessibleObject : ChartChildAccessibleObject
    {
        #region Fields

        // List of child accessible objects
        private List<ChartChildAccessibleObject> _childList = new List<ChartChildAccessibleObject>();

        #endregion // Fields

        #region Constructor

        /// <summary>
        /// Object constructor.
        /// </summary>
        /// <param name="chartAccessibleObject">Chart accessible object.</param>
        /// <param name="series">Chart series object.</param>
        public ChartChildSeriesAccessibleObject(ChartAccessibleObject chartAccessibleObject, AccessibleObject parent, Series series)
            : base(
                chartAccessibleObject,
                parent,
                series, ChartElementType.DataPoint,
                SR.AccessibilitySeriesName(series.Name),
                string.Empty,
                AccessibleRole.Graphic)
        {
            // Add all series data points
            int index = 1;
            foreach (DataPoint point in series.Points)
            {
                this._childList.Add(new ChartChildAccessibleObject(
                    chartAccessibleObject,
                    this,
                    point,
                    ChartElementType.DataPoint,
                    SR.AccessibilityDataPointName(index),
                    string.Empty,
                    AccessibleRole.Graphic,
                    series.Name,
                    index - 1));

                if (point.Label.Length > 0 || point.IsValueShownAsLabel)
                {
                    this._childList.Add(new ChartChildAccessibleObject(
                        chartAccessibleObject,
                        this,
                        point,
                        ChartElementType.DataPointLabel,
                        SR.AccessibilityDataPointLabelName(index),
                        !String.IsNullOrEmpty(point._lastLabelText) ? point._lastLabelText : point.Label,
                        AccessibleRole.Text,
                        series.Name,
                        index - 1));
                }

                ++index;
            }
        }

        #endregion // Constructor

        #region Methods

        /// <summary>
        /// Gets child accessible object.
        /// </summary>
        /// <param name="index">Index of the child object to get.</param>
        /// <returns>Chart child accessible object.</returns>
        public override AccessibleObject GetChild(int index)
        {
            if (index >= 0 && index < this._childList.Count)
            {
                return this._childList[index];
            }
            return null;
        }

        /// <summary>
        /// Get number of chart accessible objects.
        /// </summary>
        /// <returns>Number of chart accessible objects.</returns>
        public override int GetChildCount()
        {
            return this._childList.Count;
        }

        #endregion // Methods
    }
}
#endif // WINFORMS_CONTROL
