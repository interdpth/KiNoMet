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
            int targetFps = (int)30;



            ////assuming 25 fps

            //int bitrate = 10512;
            //var b = File.ReadAllBytes($"{Processing}\\VideoFileAudio.mp3");
            ////assuming 8bit audio
            //double fileLen = 3170260;
            //double TotalSeconds = 198.12; // 3:18.12
            //double frameTime = TotalSeconds / 25;
            //double TotalFrames = TotalSeconds * 25;
            //double FrameSize = fileLen / TotalFrames;

            //int tmpIndex = 11;
            //double audioFrame = tmpIndex * FrameSize;

            //for (int i = 0; i < TotalSeconds; i++)
            //{
            //    byte[] tmp = new byte[(int)bitrate];
            //    audioFrame = i * FrameSize;
            //    Array.Copy(b, (int)audioFrame, tmp, 0, (int)bitrate);
            //    IWaveProvider provider = new RawSourceWaveStream(
            //                             new MemoryStream(tmp), new WaveFormat(bitrate, 1));

            //    var _waveOut = new WaveOut();
            //    _waveOut.Init(provider);

            //    _waveOut.Play();

            //   // Console.WriteLine(audioFrame.ToString());
            //}
            //audioframe = 

            ////  Do some intial conversions.
            ///
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -crf 32 -s 240x160 {Processing}\\{tmpVideo}" };
            var P = Process.Start(PSI);
            P.WaitForExit();

            float fps = (float)((float)fr / (float)targetFps);
            if (fps > 2.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 2.0f) fps = 2.0f;
                if (fps < 0.5f) fps = 0.5f;
            }
            string temp = "";
            if (fps != 1)
            {
                temp = $"-filter:a \"atempo = {fps}\"";
            }
            //////////4953 frames 
            //////////792 audio files 
            if (audiov == 1)
            {


                string stamp = $"00:00:0{(float)(1.0 / targetFps)}";
                PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:a \"atempo = {fps}\" -f segment -segment_time {stamp} {Processing}\\audio_output%09d.wav" };
                P = Process.Start(PSI);
                P.WaitForExit();
                RenderAudio($"{Processing}", targetFps);
                ROM.MakeSource("audio_outputmain", File.ReadAllBytes($"{Processing}\\audiopointer.dat"), $"{OutputFolder}");
                ROM.Write(OutputFolder, "audio_outputmain");
            }
            else
            {
                PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} {Processing}\\audio_outputmain.wav" };
                P = Process.Start(PSI);
                P.WaitForExit();

                RenderAudio_old($"{Processing}\\audio_outputmain.wav", targetFps);
            }
            //   

            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:v fps=fps={targetFps} -c:v cinepak -max_strips 4 -s 240x160 -q 30 -an {Processing}\\{fn}_final.avi" };
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

        private void RenderAudio(string directory, int fps)
        {
            int mffreq = 10512;
            int zmfreq = 13379;
            int freq = zmfreq;
            int coutner = 0;
            freq = mffreq;
            List<int> offsets = new List<int>();
            IOStream fileOut = new IOStream(0);
            var files = Directory.GetFiles(directory).Where(x => x.Contains("audio_output")).OrderBy(x => x).ToList();
            Console.WriteLine($"Now converging {files.Count}, grab a drink and wait :)");
            foreach (string srcFile in files)
            {

                FileInfo srcAudio = new FileInfo(srcFile);
                string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
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
                            byte[] fixedDat = new byte[raw.Length];
                            for (int i = 0; i < data.Count; i++) fixedDat[i] = (byte)data[i];


                            MimirData dat = new MimirData(new IOStream(fixedDat));

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

                            offsets.Add((int)fileOut.Position);
                            fileOut.Write8((byte)compType);
                            //fileOut.Write32(best);
                            fileOut.Write(stream.Data, best);


                        }
                    }
                }
                catch (Exception e)
                {



                }
                coutner++;
            }
            IOStream outf = new IOStream((int)(offsets.Count * 4 + fileOut.Length));
            //write offsets
            offsets.ForEach(x => outf.Write32(x));
            //slow but whaetever
            fileOut.Data.ToList().ForEach(x => outf.Write8(x));
            File.WriteAllBytes($"{directory}\\audiopointer.dat", outf.Data);
        }
    }
}
