using Microsoft.WindowsAPICodePack.Shell;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace KinometGui
{
    public class Kinomet
    {
        public string Processing { get; set; }
        public string OutputFolder { get; set; }
        private string videoFile;
        private int audiov;//0 == raw 1 == split files.
        public Kinomet(string file, string processing, string output, int audiotype)
        {
            videoFile = file;
            Processing = processing;
            OutputFolder = output;
            audiov = audiotype;
        }
        AllSight sight;
        /// <summary>
        /// Convert the format.
        /// </summary>
        public void Kinometize()
        {
            FileInfo fi = new FileInfo(videoFile);
            //Get actiual FPS of video.
            string fn = fi.Name.Split('.')[0];
            string tmpVideo = $"{fn}.mp4";
            var vidInfoProp = ShellFile.FromFilePath(videoFile).Properties;
            var vidInfo = vidInfoProp.System.Video;
            long mInfo = (long)vidInfoProp.System.Media.Duration.Value;

            TimeSpan ts = TimeSpan.FromTicks(mInfo);
            double minutesFromTs = ts.TotalMinutes;
            uint fr = (vidInfo.FrameRate.Value == null ? 0 : vidInfo.FrameRate.Value.Value) / 1000;
            int targetFps = (int)23;

            ////  Do some intial conversions.
            ///
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -crf 25 -s 240x160 {Processing}\\{tmpVideo}" };
            var P = Process.Start(PSI);
            P.WaitForExit();

            float fps = (float)((float)fr / (float)targetFps);
            if (fps > 100.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 100.0f) fps = 100.0f;
                if (fps < 0.5f) fps = 0.5f;
            }
          
            uint numframes = (uint)(mInfo / 1000) / (uint)fps ;
            string temp = "";
            if (fps != 1)
            {
                temp = $"-filter:a \"atempo = {fps}\"";
            }
            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} -ac 1 {Processing}\\audio_outputmain.wav" };

            if (audiov == 1)
            {

                //Break audio file into n-chunks where n = fps*(audiofilezie/numvideoframes);
                //Test compression
                //for each
                //store pointer
                //write data



                string stamp = $"00:00:0{(float)(1.0 / fps)}";
                P = Process.Start(PSI);
                P.WaitForExit();
                RenderAudioV1($"{Processing}\\audio_outputmain.wav", targetFps, (int)numframes);

            }
            else
            {
                P = Process.Start(PSI);
                P.WaitForExit();

                RenderAudio_old($"{Processing}\\audio_outputmain.wav", targetFps);
            }
            //   

            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:v fps=fps={targetFps} -c:v cinepak -max_strips 5 -q 30 -s 240x160 -an {Processing}\\{fn}_final.avi" };
            P = Process.Start(PSI);
            P.WaitForExit();
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }


        private void RenderAudio_old(string srcFile, int fps)
        {
            int mffreq = 10512;
            int zmfreq = 13379;
            int freq = mffreq;

            freq = mffreq;

            FileInfo srcAudio = new FileInfo(srcFile);

            //we need to be 8bit and mono channel, apply desired frequency.
            var outFormat = new WaveFormat(freq, 8, 1);

            //Find out level of decode 
            WaveStream srcStream = null;

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

            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }
            try
            {
                //Convert either source to wave.
                using (WaveFormatConversionStream conversionStream = new WaveFormatConversionStream(outFormat, srcStream))
                {
                    using (RawSourceWaveStream raw = new RawSourceWaveStream(conversionStream, outFormat))
                    {
                        //Convert to signed 8bit.
                        raw.Seek(0, SeekOrigin.Begin);
                        int len = 0;
                        List<sbyte> data = new List<sbyte>();
                        for (; len < raw.Length; len++)
                        {
                            sbyte n = Convert.ToSByte(raw.ReadByte() - 128);
                            data.Add(n);

                        }                    //Generate 
                                             //Write it
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
                            for (len = 0; len < data.Count; len++)
                            {
                                bw.Write(data[len]);
                            }
                            bw.Close();
                        }

                        ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
                        ROM.Write(OutputFolder, fn);

                    }
                }
            }
            catch (Exception e)
            {
                using (RawSourceWaveStream raw = new RawSourceWaveStream(srcStream, outFormat))
                {
                    //Convert to signed 8bit.
                    raw.Seek(0, SeekOrigin.Begin);
                    int len = 0;
                    List<sbyte> data = new List<sbyte>();
                    for (; len < raw.Length; len++)
                    {
                        byte[] tmp = new byte[12];
                        raw.Read(tmp, 0, 12);
                        len += 12;
                        for (int i = 0; i < 12; i++)
                        {
                            sbyte n = Convert.ToSByte(tmp[i] - 128);
                            data.Add(n);
                        }


                    }                    //Generate 
                                         //Write it
                    string fn = srcAudio.Name.Replace(".wav", "");
                    using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
                    using (BinaryWriter bw = new BinaryWriter(fs))
                    {
                        for (len = 0; len < data.Count; len++)
                        {
                            bw.Write(data[len]);
                        }
                        bw.Close();
                    }

                    ROM.MakeSource(fn, data.ToArray(), OutputFolder);
                    ROM.Write(OutputFolder, fn);

                }
            }
        }

        private string RenderAudioV1(string srcFile, int fps, int numframes)
        {
            int mffreq = 10512;
            int zmfreq = 13379;
            int freq = zmfreq;
            string fn = "";
            freq = mffreq;

            FileInfo srcAudio = new FileInfo(srcFile);

            //we need to be 8bit and mono channel, apply desired frequency.
            var outFormat = new WaveFormat(freq, 8, 1);

            //Find out level of decode 
            WaveStream srcStream = null;

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

            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return null;
            }
            try
            {
                //Convert either source to wave.
                using (WaveFormatConversionStream conversionStream = new WaveFormatConversionStream(outFormat, srcStream))
                {
                    using (RawSourceWaveStream raw = new RawSourceWaveStream(conversionStream, outFormat))
                    {
                        //Convert to signed 8bit.
                        raw.Seek(0, SeekOrigin.Begin);
                        int len = 0;
                        List<sbyte> data = new List<sbyte>();
                        for (; len < raw.Length; len++)
                        {
                            sbyte n = Convert.ToSByte(raw.ReadByte() - 128);
                            data.Add(n);

                        }                    //Generate 
                                             //Write it
                        fn = srcAudio.Name.Replace(srcAudio.Extension, "");
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
                                    int bufsize = (int)raw.Length / numframes;
                                    int fnsize = ((int)raw.Length / bufsize);
                                    int sz = fps * bufsize;
                                    int ezCount = 0;

                                    for (int i = 0; i < raw.Length/ sz; i ++)
                                    {
                                        //buffer is fps*size;
                                        byte[] buf = new byte[sz];
                                        Buffer.BlockCopy(data.ToArray(), i*sz, buf, 0, sz);

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
                    return "holyshit.dmp";
                }
            }
            catch (Exception e)
            {
                using (RawSourceWaveStream raw = new RawSourceWaveStream(srcStream, outFormat))
                {
                    //Convert to signed 8bit.
                    raw.Seek(0, SeekOrigin.Begin);
                    int len = 0;
                    List<sbyte> data = new List<sbyte>();
                    for (; len < raw.Length; len++)
                    {
                        byte[] tmp = new byte[12];
                        raw.Read(tmp, 0, 12);
                        len += 12;
                        for (int i = 0; i < 12; i++)
                        {
                            sbyte n = Convert.ToSByte(tmp[i] - 128);
                            data.Add(n);
                        }


                    }                    //Generate 
                                         //Write it
                    fn = srcAudio.Name.Replace(".wav", "");
                    using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
                    using (BinaryWriter bw = new BinaryWriter(fs))
                    {
                        for (len = 0; len < data.Count; len++)
                        {
                            bw.Write(data[len]);
                        }
                        bw.Close();
                    }

                    ROM.MakeSource(fn, data.ToArray(), OutputFolder);
                    ROM.Write(OutputFolder, fn);

                }
            }
            return fn;
        }
    }
}
