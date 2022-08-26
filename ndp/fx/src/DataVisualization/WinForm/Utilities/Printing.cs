//-------------------------------------------------------------
// <copyright company=’Microsoft Corporation’>
//   Copyright © Microsoft Corporation. All Rights Reserved.
// </copyright>
//-------------------------------------------------------------
// @owner=alexgor, deliant
//=================================================================
//  File:		Printing.cs
//
//  Namespace:	DataVisualization.Charting.Utilities
//
//	Classes:	PrintingManager
//
//  Purpose:	Utility class that conatins properties and methods
//				for chart printing.
//
//	Reviewed:	
//
//===================================================================


using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Drawing.Printing;
using System.Windows.Forms;



#if WINFORMS_CONTROL
using System.Windows.Forms.DataVisualization.Charting;

namespace System.Windows.Forms.DataVisualization.Charting
{
	/// <summary>
	/// Chart printing class.
	/// </summary>
	public class PrintingManager : IDisposable
	{
		#region Private fields

		// Reference to the service container
		private IServiceContainer			_serviceContainer;

		// Reference to the chart image object
		private ChartImage					_chartImage;

		// Chart printing document
		private PrintDocument				_printDocument;

		#endregion

		#region Constructors and Service Provider methods

		/// <summary>
		/// Public constructor is unavailable
		/// </summary>
		private PrintingManager()
		{
		}

		/// <summary>
		/// Public constructor
		/// </summary>
		/// <param name="container">Service container reference.</param>
		public PrintingManager(IServiceContainer container)
		{
			if(container == null)
			{
				throw(new ArgumentNullException(SR.ExceptionInvalidServiceContainer));
			}
			_serviceContainer = container;
		}

		/// <summary>
		/// Returns Printing Manager service object
		/// </summary>
		/// <param name="serviceType">Requested service type.</param>
		/// <returns>Printing Manager sevice object.</returns>
		internal object GetService(Type serviceType)
		{
			if(serviceType == typeof(PrintingManager))
			{
				return this;
			}
			throw (new ArgumentException( SR.ExceptionChartSerializerUnsupportedType( serviceType.ToString() ) ) );
		}
		#endregion

		#region Printing properties
		/// <summary>
		/// Chart printing document.
		/// </summary>
		[
		Bindable(false),
		SRDescription("DescriptionAttributePrintingManager_PrintDocument"),
		Browsable(false),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        Utilities.SerializationVisibilityAttribute(Utilities.SerializationVisibility.Hidden)
		]
		public PrintDocument PrintDocument
		{
			set
			{
				_printDocument = value;
			}
			get
			{
				if(_printDocument == null)
				{
					// Create new object
					_printDocument = new PrintDocument();

					// Hook up to the PrintPage event of the document
					this.PrintDocument.PrintPage += new PrintPageEventHandler(pd_PrintPage);
				}
				return _printDocument;
			}
		}

		#endregion

		#region Printing methods

		/// <summary>
		/// Draws chart on the printer graphics.
		/// </summary>
        /// <param name="graphics">Printer graphics.</param>
		/// <param name="position">Position to draw in the graphics.</param>
        public void PrintPaint(Graphics graphics, Rectangle position)
		{
			// Get a reference to the chart image object
			if(_chartImage == null && _serviceContainer != null)
			{
				_chartImage = (ChartImage)_serviceContainer.GetService(typeof(ChartImage));
			}

			// Draw chart
			if(_chartImage != null)
			{
				// Change chart size to fit the new position
				int oldWidth = _chartImage.Width;
				int oldHeight = _chartImage.Height;
                _chartImage.Width = position.Width;
                _chartImage.Height = position.Height;

				// Save graphics state.
				GraphicsState transState = graphics.Save();

				// Set required transformation
				graphics.TranslateTransform(position.X, position.Y);

				// Set printing indicator
				_chartImage.isPrinting = true;

				// Draw chart
				_chartImage.Paint(graphics, false);

				// Clear printing indicator
				_chartImage.isPrinting = false;

				// Restore graphics state.
				graphics.Restore(transState);

				// Restore old chart position
				_chartImage.Width = oldWidth;
				_chartImage.Height = oldHeight;
			}
		}

		/// <summary>
		/// Shows Page Setup dialog.
		/// </summary>
		public void PageSetup()
		{
			// Create print preview dialog
			PageSetupDialog	pageSetupDialog = new PageSetupDialog();

			// Initialize printing document
			pageSetupDialog.Document = this.PrintDocument;

			// Show page setup dialog
			pageSetupDialog.ShowDialog();
		}


		/// <summary>
		/// Print preview the chart.
		/// </summary>
		public void PrintPreview()
		{
			// Create print preview dialog
			PrintPreviewDialog	printPreviewDialog = new PrintPreviewDialog();

			// Initialize printing document
			printPreviewDialog.Document = this.PrintDocument;

			// Show print preview
			printPreviewDialog.ShowDialog();
		}

		/// <summary>
		/// Prints chart.
		/// </summary>
		/// <param name="showPrintDialog">Indicates if printing dialog should be shown.</param>
		public void Print(bool showPrintDialog)
		{
            // Show Print dialog
			if(showPrintDialog)
			{
				// Create and show Print dialog
				PrintDialog printDialog = new PrintDialog();
                printDialog.UseEXDialog = true;
				printDialog.Document = this.PrintDocument;
				DialogResult dialogResult = printDialog.ShowDialog();

				// Do not proceed with printing if OK button was not pressed
				if(dialogResult != DialogResult.OK && dialogResult != DialogResult.Yes)
				{
					return;
				}
			}

			// Print chart
			this.PrintDocument.Print();
		}

		/// <summary>
		/// Handles PrintPage event of the document.
		/// </summary>
		/// <param name="sender">Sender object.</param>
		/// <param name="ev">Event parameters.</param>
		private void pd_PrintPage(object sender, PrintPageEventArgs ev) 
		{
			// Get a reference to the chart image object
			if(_chartImage == null && _serviceContainer != null)
			{
				_chartImage = (ChartImage)_serviceContainer.GetService(typeof(ChartImage));
			}

			if(_chartImage != null)
			{
                // Save graphics state.
                GraphicsState transState = ev.Graphics.Save();
                try
                {
                    Rectangle marginPixel = ev.MarginBounds;
                    // Display units mean different thing depending if chart is rendered on the display or printed.
                    // Typically pixels for video displays, and 1/100 inch for printers.
                    if (ev.Graphics.PageUnit != GraphicsUnit.Pixel)
                    {
                        ev.Graphics.PageUnit = GraphicsUnit.Pixel;
                        marginPixel.X = (int)(marginPixel.X * (ev.Graphics.DpiX / 100.0f));
                        marginPixel.Y = (int)(marginPixel.Y * (ev.Graphics.DpiY / 100.0f));
                        marginPixel.Width = (int)(marginPixel.Width * (ev.Graphics.DpiX / 100.0f));
                        marginPixel.Height = (int)(marginPixel.Height * (ev.Graphics.DpiY / 100.0f));
                    }
                    // Calculate chart position rectangle
                    Rectangle chartPosition = new Rectangle(marginPixel.X, marginPixel.Y, _chartImage.Width, _chartImage.Height);

                    // Make sure chart corretly fits the margin area
                    float chartWidthScale = ((float)marginPixel.Width) / ((float)chartPosition.Width);
                    float chartHeightScale = ((float)marginPixel.Height) / ((float)chartPosition.Height);
                    chartPosition.Width = (int)(chartPosition.Width * Math.Min(chartWidthScale, chartHeightScale));
                    chartPosition.Height = (int)(chartPosition.Height * Math.Min(chartWidthScale, chartHeightScale));

                    // Calculate top left position so that chart is aligned in the center   
                    chartPosition.X += (marginPixel.Width - chartPosition.Width) / 2;
                    chartPosition.Y += (marginPixel.Height - chartPosition.Height) / 2;

                    // Draw chart on the printer graphisc
                    this.PrintPaint(ev.Graphics, chartPosition);
                }
                finally
                {
                    // Restore graphics state.
                    ev.Graphics.Restore(transState);
                }
            }
		}

		#endregion

        #region IDisposable Members

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources
        /// </summary>
        /// <param name="disposing"><c>true</c> to release both managed and unmanaged resources; <c>false</c> to release only unmanaged resources.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                //Free managed resources
                if (_printDocument != null)
                {
                    _printDocument.Dispose();
                    _printDocument = null;
                }
            }
        }

        /// <summary>
        /// Performs freeing, releasing, or resetting managed resources.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion
	}
}

#endif	//#if WINFORMS_CONTROL