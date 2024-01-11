using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static Microsoft.WindowsAPICodePack.Shell.PropertySystem.SystemProperties.System;

namespace KinometGui
{
    /// <summary>
    /// Mirrors AudioHandler.h except datPtr
    /// </summary>
    public class AudioHeader
    {
        public UInt32 hdr;
        public UInt32 compressedlength;
        public UInt32 fileLength;
        public UInt16 type;
        public UInt16 fps;
        public UInt32 freq;
        public static int GetHdrSize()
        {
            return sizeof(UInt32) * 4 + sizeof(UInt16) * 2;
        }
        public AudioHeader(uint hdr, uint compressedlength, uint fileLength, ushort type, ushort fps, uint freq)
        {
            this.hdr = hdr;
            this.compressedlength = compressedlength;
            this.fileLength = fileLength;
            this.type = type;
            this.fps = fps;
            this.freq = freq;
        }

        public void Write(BinaryWriter binaryWriter)
        {
            binaryWriter.Seek(0, SeekOrigin.Begin);

            binaryWriter.Write(this.hdr);//AUV1
            binaryWriter.Write(this.compressedlength);
            binaryWriter.Write(this.fileLength);
            

            UInt16 atype = (UInt16)this.type;

            byte[] hdr = BitConverter.GetBytes(atype);
            binaryWriter.Write(hdr);

            atype = (UInt16)fps;

            hdr = BitConverter.GetBytes(atype);
            binaryWriter.Write(hdr);

            var freqenc = freq;

            hdr = BitConverter.GetBytes(freqenc);
            binaryWriter.Write(hdr);

        }
    }

}
