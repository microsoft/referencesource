/**************************************************************************\
    Copyright Microsoft Corporation. All Rights Reserved.
\**************************************************************************/

namespace System.Windows.Shell
{
    public class JumpTask : JumpItem
    {
        public JumpTask() : base()
        {}

        public string Title { get; set; }

        public string Description { get; set; }

        public string ApplicationPath { get; set; }

        public string Arguments { get; set; }

        public string WorkingDirectory { get; set; }

        public string IconResourcePath { get; set; }

        public int IconResourceIndex { get; set; }
    }
}
