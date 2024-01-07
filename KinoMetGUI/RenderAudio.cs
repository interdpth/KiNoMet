using NAudio.Wave;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Security;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using static Microsoft.WindowsAPICodePack.Shell.PropertySystem.SystemProperties.System;

namespace KinometGui.Properties
{

    public class Tmp
    {
        public int count;
        public int len;
        public Tmp(int c, int l)
        {
               count = c; 
              len = l;
        }
    }
    public class RenderAudio
    {
        public static int mffreq = 10512;
        public static int zmfreq = 13379;
        public static int freq = mffreq;
        private string OutputFolder { get; set; }
        private string srcFile { get; set; }
        private int fps { get; set; }
        private int numframes { get; set; }
        private int audiov { get; set; }
        public RenderAudio(int type, string outFolder, string sf, int framps, int noframes)
        {
            audiov = type;
            OutputFolder = outFolder;
            srcFile = sf;
            fps = framps;
            numframes = noframes;
        }
        public void Render()
        {
            switch (audiov)
            {
                case 0:
                    RenderAudio_old();
                    break;
                case 1:
                    RenderAudioV1();
                    break;
                case 2:
                    RenderAudioV2();
                    break;
            }
        }
        private List<sbyte> DecodeFile(FileInfo srcAudio)
        {
            List<sbyte> data = null;
            WaveStream srcStream = null;
            //we need to be 8bit and mono channel, apply desired frequency.
            var outFormat = new WaveFormat(freq, 8, 1);

            if (srcAudio.Extension.ToLower() == ".mp3")
            {
                Console.WriteLine("Decoding mp3.");
                srcStream = new Mp3FileReader(srcFile);
            }

            if (srcAudio.Extension.ToLower() == ".wav")
            {
                Console.WriteLine("Decoding wav.");
                srcStream = new WaveFileReader(srcFile);
            }
            using (WaveFormatConversionStream conversionStream = new WaveFormatConversionStream(outFormat, srcStream))
            {
                using (RawSourceWaveStream raw = new RawSourceWaveStream(conversionStream, outFormat))
                {
                    //Convert to signed 8bit.
                    raw.Seek(0, SeekOrigin.Begin);
                    int len = 0;
                    data = new List<sbyte>();
                    for (; len < raw.Length; len++)
                    {
                        sbyte n = Convert.ToSByte(raw.ReadByte() - 128);
                        data.Add(n);

                    }
                }
            }

            return data;
        }
        private void RenderAudio_old()
        {
            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }
            //Convert to signed 8bit.

            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            if (File.Exists($"{OutputFolder}\\{fn}.raw")) File.Delete($"{OutputFolder}\\{fn}.raw");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                bw.Write(0x41555630);//AUV0
                UInt16 atype = (UInt16)audiov;

                byte[] hdr = BitConverter.GetBytes(atype);
                bw.Write(hdr);

                atype = (UInt16)fps;

                hdr = BitConverter.GetBytes(atype);
                bw.Write(hdr);

                var atype2 = freq;

                hdr = BitConverter.GetBytes(atype2);
                bw.Write(hdr);
                for (int len = 0; len < srcStream.Count; len++)
                {
                    bw.Write(srcStream[len]);
                }
                bw.Close();
            }

            ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
            ROM.Write(OutputFolder, fn);

        }

        private void RenderAudioV1()
        {

            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }


            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                using (MemoryStream pointerTable = new MemoryStream())
                {
                    using (MemoryStream dataTable = new MemoryStream())
                    {

                        bw.Write(0x41555631);//AUV1

                        UInt16 atype = (UInt16)audiov;

                        byte[] hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);

                        atype = (UInt16)fps;

                        hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);

                        var atype2 = freq;

                        hdr = BitConverter.GetBytes(atype2);
                        bw.Write(hdr);

                        int hdrOffset = (int)bw.BaseStream.Position;
                        bw.Write(0xFFFFFFFF);//Count
                        bw.Write(0xFFFFFFFF);//Index Pointers
                        bw.Write(0xFFFFFFFF);//Data 
                        int streamLength = srcStream.Count;
                        int bufsize = (int)streamLength / numframes;
                        int fnsize = ((int)streamLength / bufsize);
                        int sz = fps * bufsize;
                        int ezCount = 0;
                        Array astream = srcStream.ToArray();
                        for (int i = 0; i < streamLength / sz; i++)
                        {
                            //buffer is fps*size;
                            byte[] buf = new byte[sz];
                            Buffer.BlockCopy(astream, i * sz, buf, 0, sz);

                            MimirData dat = new MimirData(new IOStream(buf));

                            ChariotWheels compType = ChariotWheels.Raw;
                            IOStream stream = dat.srcDat;
                            int best = (int)dat.srcDat.Length;
                            if (dat.lz.Length < best)
                            {
                                compType = ChariotWheels.LZ;
                                stream = dat.lz;
                                best = (int)stream.Length;
                            }

                            if (dat.rle.Length < best)
                            {
                                compType = ChariotWheels.RLE;
                                stream = dat.rle;
                                best = (int)stream.Length;
                            }

                            pointerTable.Write(BitConverter.GetBytes(dataTable.Position), 0, 4);
                            //dataTable.Write(BitConverter.GetBytes(0xDEADBEEF), 0, 4);
                            //dataTable.Write(BitConverter.GetBytes(ezCount), 0, 4);//wtf
                            dataTable.WriteByte((byte)compType);
                            dataTable.Write(BitConverter.GetBytes((ushort)best), 0, 2);
                            dataTable.Write(stream.Data, 0, best);
                            ezCount++;
                        }




                        int pointerTableIndex = (int)bw.BaseStream.Position;
                        bw.Write(pointerTable.ToArray(), 0, (int)pointerTable.Length);

                        int dataTableOffset = (int)bw.BaseStream.Position;

                        bw.Write(dataTable.ToArray(), 0, (int)dataTable.Length);
                        bw.BaseStream.Position = hdrOffset;
                        bw.Write(ezCount);
                        bw.Write(pointerTableIndex);
                        bw.Write(dataTableOffset);
                        bw.Close();

                    }
                }


                ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
                ROM.Write(OutputFolder, fn);

            }
        }

        private void RenderAudioV2()
        {
            Dictionary<string, Tmp> found = new Dictionary<string, Tmp>();
            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }
            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                using (MemoryStream pointerTable = new MemoryStream())
                {
                    using (MemoryStream dataTable = new MemoryStream())
                    {

                        bw.Write(0x41555632);//AUV1

                        UInt16 atype = (UInt16)audiov;

                        byte[] hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);

                        atype = (UInt16)fps;

                        hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);

                        var atype2 = freq;

                        hdr = BitConverter.GetBytes(atype2);
                        bw.Write(hdr);

                        int hdrOffset = (int)bw.BaseStream.Position;
                        bw.Write(0xFFFFFFFF);//Count
                        bw.Write(0xFFFFFFFF);//Index Pointers
                        bw.Write(0xFFFFFFFF);//Data 
                        int bufsize = 64;




                        int fnsize = ((int)srcStream.Count / bufsize);
                        // int sz = fps * bufsize;
                        int ezCount = 0;
                        int audioSamples = (fnsize);

                        //How can we sync, we don't :) 
                        for (int i = 0; i < audioSamples; i++)
                        {
                            //buffer is fps*size;
                            byte[] buf = new byte[bufsize];
                            Buffer.BlockCopy(srcStream.ToArray(), i * bufsize, buf, 0, bufsize);
                            byte[] hashBytes;

                            using (SHA256 sha256Hash = SHA256.Create())
                            {
                                hashBytes = sha256Hash.ComputeHash(buf);
                            }

                            string hashString = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();

                            Tmp curCount = null;
                            bool existed = false;
                            if (found.ContainsKey(hashString))
                            {                                
                                existed = true;
                            }
                            else
                            {
                                found.Add(hashString, new Tmp(0, buf.Length));
                            }
                            curCount = found[hashString];
                            curCount.count++;
                            found[hashString] = curCount;
                            MimirData dat = new MimirData(new IOStream(buf));

                            ChariotWheels compType = ChariotWheels.Raw;
                            IOStream stream = dat.srcDat;
                            int best = (int)dat.srcDat.Length;
                            if (dat.lz.Length < best)
                            {
                                compType = ChariotWheels.LZ;
                                stream = dat.lz;
                                best = (int)stream.Length;
                            }

                            if (dat.rle.Length < best)
                            {
                                compType = ChariotWheels.RLE;
                                stream = dat.rle;
                                best = (int)stream.Length;
                            }

                            //HELLO DOES THE DATA EXIST?


              



                            pointerTable.Write(BitConverter.GetBytes(dataTable.Position), 0, 4);
                            dataTable.WriteByte((byte)compType);
                            dataTable.Write(BitConverter.GetBytes((ushort)best), 0, 2);
                            dataTable.Write(stream.Data, 0, best);
                            ezCount++;
                        }




                        int pointerTableIndex = (int)bw.BaseStream.Position;
                        bw.Write(pointerTable.ToArray(), 0, (int)pointerTable.Length);

                        int dataTableOffset = (int)bw.BaseStream.Position;

                        bw.Write(dataTable.ToArray(), 0, (int)dataTable.Length);
                        bw.BaseStream.Position = hdrOffset;
                        bw.Write(ezCount);
                        bw.Write(pointerTableIndex);
                        bw.Write(dataTableOffset);
                        bw.Close();

                    }
                }



                //lets see all the data we could have saved!

                found.Keys.ToList().ForEach(key => 
                {
                    Console.WriteLine($"{key} found {found[key].count} times with a size of {found[key].len}");                    
                });
                ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
                ROM.Write(OutputFolder, fn);

                return;
            }


            //private void EmergencyRender()
            //{
            //    using (RawSourceWaveStream raw = new RawSourceWaveStream(srcStream, outFormat))
            //    {
            //        //Convert to signed 8bit.
            //        raw.Seek(0, SeekOrigin.Begin);
            //        int len = 0;
            //        List<sbyte> data = new List<sbyte>();
            //        for (; len < raw.Length; len++)
            //        {
            //            byte[] tmp = new byte[12];
            //            raw.Read(tmp, 0, 12);
            //            len += 12;
            //            for (int i = 0; i < 12; i++)
            //            {
            //                sbyte n = Convert.ToSByte(tmp[i] - 128);
            //                data.Add(n);
            //            }


            //        }                    //Generate 
            //                             //Write it
            //        string fn = srcAudio.Name.Replace(".wav", "");
            //        using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            //        using (BinaryWriter bw = new BinaryWriter(fs))
            //        {
            //            for (len = 0; len < data.Count; len++)
            //            {
            //                bw.Write(data[len]);
            //            }
            //            bw.Close();
            //        }

            //        ROM.MakeSource(fn, data.ToArray(), OutputFolder);
            //        ROM.Write(OutputFolder, fn);

            //    }
            //}

        }
    }
}
