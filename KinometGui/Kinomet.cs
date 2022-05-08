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
            int targetFps = (int)fr;




            ////  Do some intial conversions.
            ///
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -q 32 -s 240x160 {Processing}\\{tmpVideo}" };
            var P = Process.Start(PSI);
            P.WaitForExit();

            float fps = (float)((float)fr / (float)targetFps);
            if (fps > 2.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 2.0f) fps = 2.0f;
                if (fps < 0.5f) fps = 0.5f;
            }
            uint numframes = (uint)(mInfo / 1000) / (uint)targetFps;
            string temp = "";
            if (fps != 1)
            {
                temp = $"-filter:a \"atempo = {fps}\"";
            }
            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} {Processing}\\audio_outputmain.wav" };

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

            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:v fps=fps={targetFps} -c:v cinepak -max_strips 10 -s 240x160 -q 30 -an {Processing}\\{fn}_final.avi" };
            P = Process.Start(PSI);
            P.WaitForExit();
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }


        private void RenderAudio_old(string srcFile, int fps)
        {
            int mffreq = 10512;
            int zmfreq = 13379;
            int freq = zmfreq;

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
                        using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
                        using (BinaryWriter bw = new BinaryWriter(fs))
                        {
                            UInt16 atype = (UInt16)audiov;

                            byte[] hdr = BitConverter.GetBytes(atype);
                            bw.Write(hdr);

                            atype = (UInt16)fps;

                            hdr = BitConverter.GetBytes(atype);
                            bw.Write(hdr);

                            var atype2 = freq;

                            hdr = BitConverter.GetBytes(atype2);
                            bw.Write(hdr);
                            for (len = 0; len < raw.Length; len++)
                            {
                                bw.Write(data[len]);
                            }
                            bw.Close();
                        }

                        ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder) ;
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

        private string RenderAudioV1(string srcFile, int fps,int numframes)
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
                            for (len = 0; len < raw.Length; len++)
                            {
                                bw.Write(data[len]);
                            }
                            bw.Close();
                        }
                        

                        var fdat = File.ReadAllBytes($"{OutputFolder}\\{fn}.raw");
                        int hdrLen = 4 + 2 + 2 + 4;
                        int bufsize = (fdat.Length-hdrLen) / numframes;
                        //Now math everything out. 

                        IOStream outfile = new IOStream(8);
                        IOStream infile = new IOStream(8);
                        //write old file header
                        outfile.Write32(0xFFFFFFFF);//Index Pointers
                        outfile.Write32(0xFFFFFFFF);//Data 

                        IOStream pointerTable = new IOStream();
                        IOStream dataTable = new IOStream();

                        int fnsize = (fdat.Length  / bufsize);
                        int sz = fps * bufsize;
                        int ezCount = 0;
                        for (int i = 0; i < fnsize; i += sz)
                        {
                            //buffer is fps*size;
                            byte[] buf = new byte[sz];
                            Buffer.BlockCopy(fdat, i, buf, 0, sz);

                            MimirData dat = new MimirData(new IOStream(buf));

                            ChariotWheels compType = ChariotWheels.Raw;
                            IOStream stream = dat.srcDat;
                            int best = (int)dat.srcDat.Length;
                            //if (dat.lz.Length < best)
                            //{
                            //    compType = ChariotWheels.LZ;
                            //    stream = dat.lz;
                            //    best = (int)stream.Length;
                            //}

                            //if (dat.rle.Length < best)
                            //{
                            //    compType = ChariotWheels.RLE;
                            //    stream = dat.rle;
                            //    best = (int)stream.Length;
                            //}

                            pointerTable.Write32((int)dataTable.Position);
                            dataTable.Write32(0xDEADBEEF);
                            dataTable.Write32(ezCount);//wtf
                            dataTable.Write8((byte)compType);
                            dataTable.Write16((ushort)best);
                            dataTable.Write(stream.Data, best);
                        }
                        long pointerTableoff = outfile.Position;
                        outfile.Write(pointerTable.Data, pointerTable.Data.Length);
                        outfile.Write32(0x53455054);
                        long dataOffTable = outfile.Position;
                        outfile.Write(dataTable.Data, dataTable.Data.Length);
                        outfile.Position = 4;
                        outfile.Write32((int)pointerTableoff);
                        outfile.Write32((int)dataOffTable);

                        File.WriteAllBytes("holyshit.dmp", outfile.Data);
                        ROM.MakeSource(fn, outfile.Data, OutputFolder);
                        ROM.Write(OutputFolder, fn);

                    }
                }
                return "holyshit.dmp";
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
