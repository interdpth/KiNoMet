using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;

namespace KinometGui
{

    //Reading
    //Read byte for ChariotWheels
    //if(Pointer)

    public enum ChariotWheels
    {
        Raw,
        RLE,
        LZ,
        Pointer, //Always chec kfor pointer, then size difference.         
        END = 0xFD
    }


    public class MimirData
    {
        public IOStream srcDat;

        public IOStream rle = new IOStream(0);
        public IOStream lz = new IOStream(0);


        public MimirData(IOStream dat)
        {
            srcDat = dat;
            Compression.CompRLE(dat, (int)dat.Length, rle);
            Compression.CompLZ77(dat, (int)dat.Length, lz);
        }
    }
    public class OdinsEyes
    {

        public int Length { get { return nextStrip - lastStrip; } }
        private int lastStrip;
        private int nextStrip; //THis wil get set when the next eye comes into play. 

        private Guid custom;
        public string HashCode;
        public int id;
        public MimirData data;
        public OdinsEyes(int lastStrip, int length, string hash, IOStream src, int idn)
        {
            //3 versions
            nextStrip = lastStrip + length;
            data = new MimirData(src);
            HashCode = hash;
            id = idn;
            //Hash on original data
        }
    }



    public class AllSight
    {
        IOStream file;
        List<OdinsEyes> frames;
        Dictionary<int, int> hashlookup;
        public AllSight()
        {
            hashlookup = new Dictionary<int, int>();
            frames = new List<OdinsEyes>();
        }

        public void AddFrame(byte[] data, int frameno)
        {
            IOStream dat = new IOStream(data);
            OdinsEyes eyeball = new OdinsEyes(0, data.Length, GetCheckSum(data), dat, frameno);
            frames.Add(eyeball);
        }
        public static string GetCheckSum(byte[] data)
        {
            try
            {
                using (var md5Instance = MD5.Create())
                {
                    using (var stream = new MemoryStream(data))
                    {
                        var hashResult = md5Instance.ComputeHash(stream);

                        return BitConverter.ToString(hashResult).Replace("-", "").ToLowerInvariant();
                    }
                }
            }
            catch (Exception)
            {

            }
            return ""; ;
        }
        //List<List<byte>> SplitBytes(IOStream data)
        //{
        //    var SplitFile = new List<List<byte>>();
        //    List<byte> curBuffer = new List<byte>();
        //    for (int i = 0; i < data.Length; i++)
        //    {
        //        curBuffer.Add(data.Data[i]);
        //        if (curBuffer.Count == 512)
        //        {
        //            SplitFile.Add(curBuffer);
        //            curBuffer.Clear();
        //        }
        //    }
        //    if (curBuffer.Count != 0) SplitFile.Add(curBuffer);//Get the rest.
        //    return SplitFile;
        //}
        public void WriteToFile(string directory, int fps)
        {
            IOStream outFile = new IOStream(frames.Count * 0x1c0);
            bool dumpData = true;
            Dictionary<string, int> intHashLookup = new Dictionary<string, int>();
            foreach (OdinsEyes eyeball in frames.OrderBy(x => x.id))
            {
                //Do we exist? 
                if (intHashLookup.ContainsKey(eyeball.HashCode))  //Does full frame exist
                {
                    outFile.Write8((byte)ChariotWheels.Pointer);
                    outFile.WriteU32((uint)intHashLookup[eyeball.HashCode]);
                    continue;
                }

                outFile.WriteU32((uint)eyeball.data.srcDat.Length);

                //We will break the frame up into 512byte chunks.

                var dat = eyeball.data.srcDat.Data;


                //go through 
                //foreach (var ourBytes in frameBytes)
                //{
                //Check if this data exists as a full frame.

                string chkSUm = GetCheckSum(dat);

                //Do we exist? 
                if (intHashLookup.ContainsKey(eyeball.HashCode))
                {
                    outFile.Write8((byte)ChariotWheels.Pointer);
                    outFile.WriteU32((uint)intHashLookup[eyeball.HashCode]);
                    continue;
                }

                ChariotWheels compType = ChariotWheels.Raw;
                IOStream stream = new IOStream(dat);
                int best = (int)dat.Length;
                if (eyeball.data.lz.Length < best)
                {
                    compType = ChariotWheels.LZ;
                    stream = eyeball.data.lz;
                    best = (int)stream.Length;
                }

                if (eyeball.data.rle.Length < best)
                {
                    compType = ChariotWheels.RLE;
                    stream = eyeball.data.rle;
                    best = (int)stream.Length;
                }

                intHashLookup[eyeball.HashCode] = (int)outFile.Position;
                outFile.Write8((byte)compType);
                outFile.Write32(best);
                outFile.Write(stream.Data, best);

                if (dumpData)
                {
                    Debug.WriteLine($"First file: {outFile.Position.ToString("X")} {compType.ToString()} {best.ToString("X")}");
                    dumpData = false;
                }
            }

            outFile.Write8((byte)ChariotWheels.END);



            //Real 
            IOStream outputFIle = new IOStream((int)(intHashLookup.Count * 8 + outFile.Length));//"Audi, two pointers, pointer table, data;
            outputFIle.WriteASCII("BRAG");
            outputFIle.Write8((byte)fps);
            int pointerOffset = (int)outputFIle.Position;
            outputFIle.Write32((int)0x8123456);
            outputFIle.Write32((int)0x8123456);

            int lookupTable = (int)outputFIle.Position;
            outputFIle.Write32(intHashLookup.Count);
            outputFIle.Write32(0xDEADBEE5);
            foreach (var luts in intHashLookup)
            {
                outputFIle.Write32(luts.Value);
            }
            outputFIle.Write32(0xDEADBEEF);
            int outputPointer = (int)outputFIle.Position;
            //    outputFIle.Write(outFile.Data, (int)outFile.Position);
            int pos = (int)outFile.Position;
            outputFIle.Write32(0xBEEFBEEF);
            outFile.Position = 0;
            for (int i = 0; i < pos; i++)
            {
                outputFIle.WriteByte(outFile.Read8());
            }
            outputFIle.Write32(0xBEEFDEAD);
            outputFIle.Position = pointerOffset;
            outputFIle.Write32(lookupTable);
            outputFIle.Write32(outputPointer);
            Debug.WriteLine($"FPS: {fps}");
            Debug.WriteLine($"Output Pointer(Where relative data lives.): {outputPointer.ToString("X")} ");
            Debug.WriteLine($"Table Lookup and Output Pointers: {pointerOffset.ToString("X")} Compiled: {(outputPointer + pointerOffset).ToString("X")}");
            Debug.WriteLine($"Lookup Table: Table Values:{intHashLookup.Count}:{lookupTable.ToString("X")} Compiled:  {(outputPointer + lookupTable).ToString("X")} ");

            var firstRame = frames.Where(x => x.id == 0).FirstOrDefault();
            Debug.WriteLine($"First frame id: {firstRame.id.ToString("X")}");

            Debug.WriteLine($"First Frame size: {firstRame.Length.ToString("X")}");
            File.WriteAllBytes($"{directory}\\VideoAudio.Bragi", outputFIle.Data);
        }
    }
}
