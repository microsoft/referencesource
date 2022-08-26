using System;

namespace System.AddIn.MiniReflection.MetadataReader
{
    internal struct AssemblyInfo
    {
        private Version m_Version;
        private UInt32 m_Flags;
        private String m_SimpleName;
        private String m_Culture;          // Never an empty string, always null for non-empty
        private byte[] m_PublicKey;
        private String m_FullName;
        private String m_PublicKeyToken;

        /*
        public String SimpleName
        {
            get { return m_SimpleName; }
        }

        public String Culture
        {
            get { return m_Culture; }
        }
         */

        public AssemblyInfo(System.Reflection.AssemblyName AN)
        {
            m_Version = AN.Version;
            m_Flags = (UInt32) AN.Flags;
            byte[] Token;
            if ((m_Flags & (UInt32) System.Reflection.AssemblyNameFlags.PublicKey) == 0)
                Token = AN.GetPublicKeyToken();
            else Token = AN.GetPublicKey();
            if ((Token != null) && (Token.Length==0))
                m_PublicKey = null;
            else m_PublicKey = Token;
            m_SimpleName = AN.Name;
            m_Culture = AN.CultureInfo.Name;
            if (m_Culture.Length == 0) m_Culture = null;
            m_FullName = null;
            m_PublicKeyToken = null;
        }

        internal AssemblyInfo(Version V, UInt32 Flags, byte[] PublicKeyOrToken, 
                              String Name, String Culture)
        {
            m_Version = V;
            m_Flags = Flags;
            m_PublicKey = PublicKeyOrToken;
            m_SimpleName = Name;
            m_Culture = Culture;
            m_FullName = null;
            m_PublicKeyToken = null;
        }
        
        public static implicit operator AssemblyInfo(System.Reflection.AssemblyName AN)
        {
            return new AssemblyInfo(AN);
        }

        public String PublicKeyToken
        {
            get
            {
                if (m_PublicKeyToken == null)
                {
                    if (m_PublicKey == null) return null;
                    byte[] Useful = new byte[8];
                    if ((m_Flags & (UInt32) System.Reflection.AssemblyNameFlags.PublicKey) != 0)
                    {
                        using (System.Security.Cryptography.SHA1 Hasher = System.Security.Cryptography.SHA1.Create()) {
                            byte[] Hashed = Hasher.ComputeHash(m_PublicKey);
                            for (int i = 0, j = Hashed.Length - 1; i < 8; i++, j--)
                                Useful[i] = Hashed[j];
                        }
                    }
                    else 
                        Useful = m_PublicKey;
                    m_PublicKeyToken = PEFileReader.BytesToHexString(Useful);
                }
                return m_PublicKeyToken;
            }
        }

        /*
        public System.Reflection.ProcessorArchitecture ProcessorArchitecture
        {
            get
            { // According to inc\corhdr.h the architecture is encoded in the flags,
                // as xxxxxxPx where P is a byte of the form 0yyy with yyy being the value
                // we want to return.
                return (System.Reflection.ProcessorArchitecture)((m_Flags & 0x70) >> 4);
            }
        }

        public static AssemblyInfo[] FromArray(System.Reflection.AssemblyName[] ANs)
        {
            AssemblyInfo[] Result = new AssemblyInfo[ANs.Length];
            for (int i = 0; i < ANs.Length; i++) Result[i] = new AssemblyInfo(ANs[i]);
            return Result;
        }
         */

        public Version Version
        {
            get { return m_Version; }
        }

        public override String ToString()
        {
            if (m_FullName == null)
            {
                m_FullName = m_SimpleName + ", Version=" + Version.Major + "." +
                             Version.Minor + "." + Version.Build + "." + Version.Revision;
                m_FullName += ", Culture=" + ((m_Culture == null) ? "neutral" : m_Culture);
                String PKT = PublicKeyToken;
                m_FullName += ", PublicKeyToken=" + ((PKT == null) ? "null" : PKT);
            }
            return m_FullName;
        }

        /*
        public String FullName
        {
            // Note: always returns PublicKeyToken, never full public key
            get
            {
                return this.ToString();
            }
        }
         */
    }
}
