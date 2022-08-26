//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

namespace Microsoft.Tools.ServiceModel.WsatConfig
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Data;
    using System.Drawing;
    using System.Text;
    using System.Windows.Forms;
    using System.Diagnostics;

    partial class TraceOptionsForm : Form
    {
        WsatConfiguration config;

        internal TraceOptionsForm(WsatConfiguration config)
        {
            if (config == null)
            {
                throw new ArgumentNullException("config");
            }

            this.config = config;
            InitializeComponent();

            if (config.IsLocalMachine)
            {
                bool isRegError = false;
                try
                {
                    textLogFileSize.Text = ETWWsatTraceSession.GetMaxTraceFileSizeFromReg().ToString(System.Globalization.CultureInfo.InvariantCulture);
                }
                catch (WsatAdminException ex)
                {
                    isRegError = true;
                    ShowErrorDialog(ex);
                }

                bool isSessionExist = ETWWsatTraceSession.IsSessionExist();

                buttonNewSession.Enabled = !isSessionExist && !isRegError;
                buttonStopSession.Enabled = isSessionExist && !isRegError;
                buttonFlushData.Enabled = isSessionExist && !isRegError;
                textLogFileSize.Enabled = !isSessionExist && !isRegError;
            }
            else
            {
                textLogFileSize.Text = ETWWsatTraceSession.DefaultLogFileSize.ToString(System.Globalization.CultureInfo.InvariantCulture);
                groupBoxLoggingSession.Enabled = false;
            }
        }

        void buttonOK_Click(object sender, EventArgs e)
        {
            if (config.IsLocalMachine)
            {
                uint fileSize = 0;

                bool ret = UInt32.TryParse(textLogFileSize.Text, out fileSize);
                if (!ret || fileSize > ETWWsatTraceSession.MaxLogFileSize || fileSize == 0)
                {
                    textLogFileSize.Focus();
                    ShowErrorDialog(SR.GetString(SR.ErrorSessionLogFileSize));
                    return;
                }

                try
                {
                    ETWWsatTraceSession.SaveMaxTraceFileSizeToReg(fileSize);
                }
                catch (WsatAdminException ex)
                {
                    ShowErrorDialog(ex);
                    return;
                }
            }

            if (comboBoxTraceLevel.SelectedItem != null)
            {
                string traceLevelString = comboBoxTraceLevel.SelectedItem.ToString();
                SourceLevels traceLevel;

                if (!Utilities.ParseSourceLevel(traceLevelString, out traceLevel))
                {
                    traceLevel = WsatConfiguration.DefaultTraceLevel;
                }

                config.TraceLevel = traceLevel;
                config.ActivityTracing = checkBoxActivityTracing.Checked;
                config.ActivityPropagation = checkBoxActivityPropagation.Checked;
                config.TracePii = checkBoxTracePii.Checked;
            }
            DialogResult = DialogResult.OK;
        }

        void TraceOptionsForm_Load(object sender, EventArgs e)
        {
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingCritical);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingError);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingWarning);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingInformation);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingVerbose);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingAll);
            comboBoxTraceLevel.Items.Add(CommandLineOption.TracingOff);

            SourceLevels configuredLevel = config.TraceLevel & ~SourceLevels.ActivityTracing; // remove the activityTracing bit

            foreach (object item in comboBoxTraceLevel.Items)
            {
                SourceLevels level;                
                if (Utilities.ParseSourceLevel(item.ToString(), out level))
                {
                    if ((level & ~SourceLevels.ActivityTracing) == configuredLevel)
                    {
                        comboBoxTraceLevel.SelectedItem = item;
                        break;
                    }
                }
            }

            checkBoxActivityTracing.Checked = config.ActivityTracing;
            checkBoxActivityPropagation.Checked = config.ActivityPropagation;
            checkBoxTracePii.Checked = config.TracePii;
        }

        void buttonNewSession_Click(object sender, EventArgs e)
        {
            uint fileSize = 0;

            bool ret = UInt32.TryParse(textLogFileSize.Text, out fileSize);
            if (!ret || fileSize > ETWWsatTraceSession.MaxLogFileSize || fileSize == 0)
            {
                textLogFileSize.Focus();
                ShowErrorDialog(SR.GetString(SR.ErrorSessionLogFileSize));
                return;
            }

            try
            {
                ETWWsatTraceSession.StartSession(fileSize);

                buttonNewSession.Enabled = false;
                buttonStopSession.Enabled = true;
                buttonFlushData.Enabled = true;
                textLogFileSize.Enabled = false;
            }
            catch (WsatAdminException ex)
            {
                ShowErrorDialog(ex);
            }
        }

        void buttonStopSession_Click(object sender, EventArgs e)
        {
            try
            {
                ETWWsatTraceSession.StopSession();

                buttonNewSession.Enabled = true;
                buttonStopSession.Enabled = false;
                buttonFlushData.Enabled = false;
                textLogFileSize.Enabled = true;
            }
            catch (WsatAdminException ex)
            {
                ShowErrorDialog(ex);
            }
        }

        void buttonFlushData_Click(object sender, EventArgs e)
        {
            try
            {
                ETWWsatTraceSession.FlushData();
            }
            catch (WsatAdminException ex)
            {
                ShowErrorDialog(ex);
            }
        }

        void ShowErrorDialog(string msg)
        {
            MessageBox.Show(msg, SR.GetString(SR.ErrorMessageBoxTitle),
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error,
                            MessageBoxDefaultButton.Button1,
                            (MessageBoxOptions)0);
        }

        void ShowErrorDialog(WsatAdminException ex)
        {
            ShowErrorDialog(ex.Message);
        }
    }
}