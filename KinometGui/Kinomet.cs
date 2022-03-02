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
        public Kinomet(string file, string processing, string output)
        {
            videoFile = file;
            Processing = processing;
            OutputFolder = output;
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

            //  Do some intial conversions.
            //var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -s 240x160 {Processing}\\{tmpVideo}" };
            //var P = Process.Start(PSI);
            //P.WaitForExit();

            float fps = (float)((float)fr / (float)targetFps);
            if (fps > 100.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 100.0f) fps = 100.0f;
                if (fps < 0.5f) fps = 0.5f;
            }

            ////4953 frames 
            ////792 audio files 
            string stamp = $"00:00:0{(float)(1.0 / 25)}";
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:a \"atempo = {fps}\" -b:a 10512 -f segment -segment_time {stamp} {Processing}\\audio_output%09d.wav" };
            var P = Process.Start(PSI);
            P.WaitForExit();


           PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -filter:v fps=fps={targetFps} -c:v cinepak -max_strips 2  -q 31 -s 240x160 -an {Processing}\\{fn}_final.avi" };
           P = Process.Start(PSI);
            P.WaitForExit();

             RenderAudio($"{Processing}");
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
            ROM.MakeSource("VideoFileAudio", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFileAudio");
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
                            for (len = 0; len < raw.Length; len++)
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
                        for(int i = 0; i < 12;i++)
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
        
        private void RenderAudio(string directory)
        {
            int mffreq = 10512;
            int zmfreq = 13379;
            int freq = zmfreq;
       
            freq = mffreq;
            AllSight odin = new AllSight();
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


                            odin.AddFrame(fixedDat);
                           

                        }
                    }
                }
                catch (Exception e)
                {
                    //using (RawSourceWaveStream raw = new RawSourceWaveStream(srcStream, outFormat))
                    //{
                    //    //Convert to signed 8bit.
                    //    raw.Seek(0, SeekOrigin.Begin);
                    //    int len = 0;
                    //    List<sbyte> data = new List<sbyte>();
                    //    for (; len < raw.Length; len++)
                    //    {
                    //        byte[] tmp = new byte[12];
                    //        raw.Read(tmp, 0, 12);
                    //        len += 12;
                    //        for (int i = 0; i < 12; i++)
                    //        {
                    //            sbyte n = Convert.ToSByte(tmp[i] - 128);
                    //            data.Add(n);
                    //        }


                    //    }                    //Generate 
                    //                         //Write it
                    //    string fn = srcAudio.Name.Replace(".wav", "");
                    //    using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
                    //    using (BinaryWriter bw = new BinaryWriter(fs))
                    //    {
                    //        for (len = 0; len < data.Count; len++)
                    //        {
                    //            bw.Write(data[len]);
                    //        }
                    //        bw.Close();
                    //    }

                    //    ROM.MakeSource(fn, data.ToArray(), OutputFolder);
                      
                    //}


                
                }
              
              //  ROM.Write(OutputFolder, fn);

            }
            odin.WriteToFile(OutputFolder);
        }
    }
}
