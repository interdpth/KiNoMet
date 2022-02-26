﻿using Microsoft.WindowsAPICodePack.Shell;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
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

        /// <summary>
        /// Convert the format.
        /// </summary>
        public void Kinometize()
        {
            FileInfo fi = new FileInfo(videoFile);
            //Get actiual FPS of video.
            string fn = fi.Name.Split('.')[0];
            string tmpVideo = $"{fn}.mp4";
            uint fr = (ShellFile.FromFilePath(videoFile).Properties.System.Video.FrameRate.Value == null ? 0 : ShellFile.FromFilePath(videoFile).Properties.System.Video.FrameRate.Value.Value) / 1000;
            int targetFps = (int)fr;

            //Do some intial conversions.
            //var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -crf 25 -filter:v fps=fps={targetFps} -s 160x128 {Processing}\\{tmpVideo}" };
            //var P = Process.Start(PSI);
            //P.WaitForExit();

            //float fps = (float)((float)fr / (float)targetFps);
            //if (fps > 2.0 || fps < 0.5)
            //{
            //    Console.WriteLine("Range is bad, clamping value");
            //    if (fps > 2.0f) fps = 2.0f;
            //    if (fps < 0.5f) fps = 0.5f;
            //}

            //PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -af atempo={fps} {Processing}\\VideoFileAudio.wav" };
            //P = Process.Start(PSI);
            //P.WaitForExit();

            
           var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -c:v cinepak -max_strips 2 -an -q 31 -s 160x128 {Processing}\\{fn}_final.avi" };
        var    P = Process.Start(PSI);
            P.WaitForExit();

            RenderAudio($"{Processing}\\VideoFileAudio.wav");
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }


        private void RenderAudio(string srcFile)
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
                        string fn = srcAudio.Name.Replace(".wav", "");
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
                Console.WriteLine("Encoded audio needs to be a different format.");
            }
        }
    }
}
