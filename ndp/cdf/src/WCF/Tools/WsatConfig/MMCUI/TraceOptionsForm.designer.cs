//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    partial class TraceOptionsForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TraceOptionsForm));
            this.flowLayoutPanelButtons = new System.Windows.Forms.FlowLayoutPanel();
            this.buttonOK = new System.Windows.Forms.Button();
            this.buttonCancel = new System.Windows.Forms.Button();
            this.groupBoxTraceOutput = new System.Windows.Forms.GroupBox();
            this.traceOutputPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.labelTraceInfo = new System.Windows.Forms.Label();
            this.traceLevelPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.labelTraceLevel = new System.Windows.Forms.Label();
            this.panelTraceLevel = new System.Windows.Forms.Panel();
            this.checkBoxActivityTracing = new System.Windows.Forms.CheckBox();
            this.checkBoxActivityPropagation = new System.Windows.Forms.CheckBox();
            this.checkBoxTracePii = new System.Windows.Forms.CheckBox();
            this.comboBoxTraceLevel = new System.Windows.Forms.ComboBox();
            this.groupBoxLoggingSession = new System.Windows.Forms.GroupBox();
            this.flowLayoutPanel2 = new System.Windows.Forms.FlowLayoutPanel();
            this.label3 = new System.Windows.Forms.Label();
            this.flowLayoutPanelSessionControlButtons = new System.Windows.Forms.FlowLayoutPanel();
            this.buttonNewSession = new System.Windows.Forms.Button();
            this.buttonStopSession = new System.Windows.Forms.Button();
            this.buttonFlushData = new System.Windows.Forms.Button();
            this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
            this.label5 = new System.Windows.Forms.Label();
            this.textLogFileSize = new System.Windows.Forms.TextBox();
            this.label4 = new System.Windows.Forms.Label();
            this.flowLayoutPanelButtons.SuspendLayout();
            this.groupBoxTraceOutput.SuspendLayout();
            this.traceOutputPanel.SuspendLayout();
            this.traceLevelPanel.SuspendLayout();
            this.panelTraceLevel.SuspendLayout();
            this.groupBoxLoggingSession.SuspendLayout();
            this.flowLayoutPanel2.SuspendLayout();
            this.flowLayoutPanelSessionControlButtons.SuspendLayout();
            this.flowLayoutPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // flowLayoutPanelButtons
            // 
            resources.ApplyResources(this.flowLayoutPanelButtons, "flowLayoutPanelButtons");
            this.flowLayoutPanelButtons.Controls.Add(this.buttonOK);
            this.flowLayoutPanelButtons.Controls.Add(this.buttonCancel);
            this.flowLayoutPanelButtons.Name = "flowLayoutPanelButtons";
            // 
            // buttonOK
            // 
            resources.ApplyResources(this.buttonOK, "buttonOK");
            this.buttonOK.Name = "buttonOK";
            this.buttonOK.UseVisualStyleBackColor = true;
            this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
            // 
            // buttonCancel
            // 
            resources.ApplyResources(this.buttonCancel, "buttonCancel");
            this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.buttonCancel.Name = "buttonCancel";
            this.buttonCancel.UseVisualStyleBackColor = true;
            // 
            // groupBoxTraceOutput
            // 
            resources.ApplyResources(this.groupBoxTraceOutput, "groupBoxTraceOutput");
            this.groupBoxTraceOutput.Controls.Add(this.traceOutputPanel);
            this.groupBoxTraceOutput.Name = "groupBoxTraceOutput";
            this.groupBoxTraceOutput.TabStop = false;
            // 
            // traceOutputPanel
            // 
            resources.ApplyResources(this.traceOutputPanel, "traceOutputPanel");
            this.traceOutputPanel.Controls.Add(this.labelTraceInfo);
            this.traceOutputPanel.Controls.Add(this.traceLevelPanel);
            this.traceOutputPanel.Name = "traceOutputPanel";
            // 
            // labelTraceInfo
            // 
            resources.ApplyResources(this.labelTraceInfo, "labelTraceInfo");
            this.labelTraceInfo.MaximumSize = new System.Drawing.Size(300, 50);
            this.labelTraceInfo.Name = "labelTraceInfo";
            // 
            // traceLevelPanel
            // 
            resources.ApplyResources(this.traceLevelPanel, "traceLevelPanel");
            this.traceLevelPanel.Controls.Add(this.labelTraceLevel);
            this.traceLevelPanel.Controls.Add(this.panelTraceLevel);
            this.traceLevelPanel.Name = "traceLevelPanel";
            // 
            // labelTraceLevel
            // 
            resources.ApplyResources(this.labelTraceLevel, "labelTraceLevel");
            this.labelTraceLevel.Name = "labelTraceLevel";
            // 
            // panelTraceLevel
            // 
            resources.ApplyResources(this.panelTraceLevel, "panelTraceLevel");
            this.panelTraceLevel.Controls.Add(this.checkBoxActivityTracing);
            this.panelTraceLevel.Controls.Add(this.checkBoxActivityPropagation);
            this.panelTraceLevel.Controls.Add(this.checkBoxTracePii);
            this.panelTraceLevel.Controls.Add(this.comboBoxTraceLevel);
            this.panelTraceLevel.Name = "panelTraceLevel";
            // 
            // checkBoxActivityTracing
            // 
            resources.ApplyResources(this.checkBoxActivityTracing, "checkBoxActivityTracing");
            this.checkBoxActivityTracing.Name = "checkBoxActivityTracing";
            this.checkBoxActivityTracing.UseVisualStyleBackColor = true;
            // 
            // checkBoxActivityPropagation
            // 
            resources.ApplyResources(this.checkBoxActivityPropagation, "checkBoxActivityPropagation");
            this.checkBoxActivityPropagation.Name = "checkBoxActivityPropagation";
            this.checkBoxActivityPropagation.UseVisualStyleBackColor = true;
            // 
            // checkBoxTracePii
            // 
            resources.ApplyResources(this.checkBoxTracePii, "checkBoxTracePii");
            this.checkBoxTracePii.Name = "checkBoxTracePii";
            this.checkBoxTracePii.UseVisualStyleBackColor = true;
            // 
            // comboBoxTraceLevel
            // 
            this.comboBoxTraceLevel.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxTraceLevel.FormattingEnabled = true;
            resources.ApplyResources(this.comboBoxTraceLevel, "comboBoxTraceLevel");
            this.comboBoxTraceLevel.Name = "comboBoxTraceLevel";
            // 
            // groupBoxLoggingSession
            // 
            resources.ApplyResources(this.groupBoxLoggingSession, "groupBoxLoggingSession");
            this.groupBoxLoggingSession.Controls.Add(this.flowLayoutPanel2);
            this.groupBoxLoggingSession.Name = "groupBoxLoggingSession";
            this.groupBoxLoggingSession.TabStop = false;
            // 
            // flowLayoutPanel2
            // 
            resources.ApplyResources(this.flowLayoutPanel2, "flowLayoutPanel2");
            this.flowLayoutPanel2.Controls.Add(this.label3);
            this.flowLayoutPanel2.Controls.Add(this.flowLayoutPanelSessionControlButtons);
            this.flowLayoutPanel2.Controls.Add(this.flowLayoutPanel1);
            this.flowLayoutPanel2.Name = "flowLayoutPanel2";
            // 
            // label3
            // 
            resources.ApplyResources(this.label3, "label3");
            this.label3.MaximumSize = new System.Drawing.Size(300, 34);
            this.label3.Name = "label3";
            // 
            // flowLayoutPanelSessionControlButtons
            // 
            resources.ApplyResources(this.flowLayoutPanelSessionControlButtons, "flowLayoutPanelSessionControlButtons");
            this.flowLayoutPanelSessionControlButtons.Controls.Add(this.buttonNewSession);
            this.flowLayoutPanelSessionControlButtons.Controls.Add(this.buttonStopSession);
            this.flowLayoutPanelSessionControlButtons.Controls.Add(this.buttonFlushData);
            this.flowLayoutPanelSessionControlButtons.Name = "flowLayoutPanelSessionControlButtons";
            // 
            // buttonNewSession
            // 
            resources.ApplyResources(this.buttonNewSession, "buttonNewSession");
            this.buttonNewSession.Name = "buttonNewSession";
            this.buttonNewSession.UseVisualStyleBackColor = true;
            this.buttonNewSession.Click += new System.EventHandler(this.buttonNewSession_Click);
            // 
            // buttonStopSession
            // 
            resources.ApplyResources(this.buttonStopSession, "buttonStopSession");
            this.buttonStopSession.Name = "buttonStopSession";
            this.buttonStopSession.UseVisualStyleBackColor = true;
            this.buttonStopSession.Click += new System.EventHandler(this.buttonStopSession_Click);
            // 
            // buttonFlushData
            // 
            resources.ApplyResources(this.buttonFlushData, "buttonFlushData");
            this.buttonFlushData.Name = "buttonFlushData";
            this.buttonFlushData.UseVisualStyleBackColor = true;
            this.buttonFlushData.Click += new System.EventHandler(this.buttonFlushData_Click);
            // 
            // flowLayoutPanel1
            // 
            this.flowLayoutPanel1.Controls.Add(this.label5);
            this.flowLayoutPanel1.Controls.Add(this.textLogFileSize);
            this.flowLayoutPanel1.Controls.Add(this.label4);
            resources.ApplyResources(this.flowLayoutPanel1, "flowLayoutPanel1");
            this.flowLayoutPanel1.Name = "flowLayoutPanel1";
            // 
            // label5
            // 
            resources.ApplyResources(this.label5, "label5");
            this.label5.Name = "label5";
            // 
            // textLogFileSize
            // 
            resources.ApplyResources(this.textLogFileSize, "textLogFileSize");
            this.textLogFileSize.Name = "textLogFileSize";
            // 
            // label4
            // 
            resources.ApplyResources(this.label4, "label4");
            this.label4.Name = "label4";
            // 
            // TraceOptionsForm
            // 
            this.AcceptButton = this.buttonOK;
            resources.ApplyResources(this, "$this");
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.buttonCancel;
            this.Controls.Add(this.groupBoxLoggingSession);
            this.Controls.Add(this.groupBoxTraceOutput);
            this.Controls.Add(this.flowLayoutPanelButtons);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "TraceOptionsForm";
            this.ShowIcon = false;
            this.Load += new System.EventHandler(this.TraceOptionsForm_Load);
            this.flowLayoutPanelButtons.ResumeLayout(false);
            this.flowLayoutPanelButtons.PerformLayout();
            this.groupBoxTraceOutput.ResumeLayout(false);
            this.traceOutputPanel.ResumeLayout(false);
            this.traceOutputPanel.PerformLayout();
            this.traceLevelPanel.ResumeLayout(false);
            this.traceLevelPanel.PerformLayout();
            this.panelTraceLevel.ResumeLayout(false);
            this.panelTraceLevel.PerformLayout();
            this.groupBoxLoggingSession.ResumeLayout(false);
            this.flowLayoutPanel2.ResumeLayout(false);
            this.flowLayoutPanel2.PerformLayout();
            this.flowLayoutPanelSessionControlButtons.ResumeLayout(false);
            this.flowLayoutPanel1.ResumeLayout(false);
            this.flowLayoutPanel1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanelButtons;
        private System.Windows.Forms.Button buttonOK;
        private System.Windows.Forms.Button buttonCancel;
        private System.Windows.Forms.GroupBox groupBoxTraceOutput;
        private System.Windows.Forms.Label labelTraceLevel;
        private System.Windows.Forms.ComboBox comboBoxTraceLevel;
        private System.Windows.Forms.Label labelTraceInfo;
        private System.Windows.Forms.GroupBox groupBoxLoggingSession;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Button buttonFlushData;
        private System.Windows.Forms.Button buttonStopSession;
        private System.Windows.Forms.Button buttonNewSession;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanelSessionControlButtons;
        private System.Windows.Forms.FlowLayoutPanel traceOutputPanel;
        private System.Windows.Forms.FlowLayoutPanel traceLevelPanel;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel2;
        private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox textLogFileSize;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Panel panelTraceLevel;
        private System.Windows.Forms.CheckBox checkBoxActivityTracing;
        private System.Windows.Forms.CheckBox checkBoxActivityPropagation;
        private System.Windows.Forms.CheckBox checkBoxTracePii;
    }
}
