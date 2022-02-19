using Microsoft.WindowsAPICodePack.Shell;
using NAudio.Wave;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;

namespace KiNoMetSharp
{
    class Program
    {
        static string Processing = "F:\\Processing";
        static string OutputFolder = "F:\\Output";
        public static void RenderAudio(string srcFile)
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



        static void Main(string[] args)
        {

            if (!Directory.Exists(Processing))
            {
                Directory.CreateDirectory(Processing);
            }
            else
            {
                List<string> killme = Directory.GetFiles(Processing).ToList();
                foreach (string sz in killme)
                {
                    File.Delete(sz);
                }
            }

            if (!Directory.Exists(OutputFolder))
            {
                Directory.CreateDirectory(OutputFolder);
            }
            else
            {
                List<string> killme = Directory.GetFiles(OutputFolder).ToList();
                foreach (string sz in killme)
                {
                    File.Delete(sz);
                }
            }

            string videoFile = "Alie.mp4";
            FileInfo fi = new FileInfo(videoFile);
            string fn = fi.Name.Split(".")[0];
            string tmpVideo = $"{fn}.mp4";
            uint fr = (ShellFile.FromFilePath(videoFile).Properties.System.Video.FrameRate.Value == null ? 0 : ShellFile.FromFilePath(videoFile).Properties.System.Video.FrameRate.Value.Value) / 1000;
            int targetFps = (int)20;

            //The shittier the large image the better it looks on gba.
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -crf 31 -filter:v fps=fps={targetFps} -s 240x160 {Processing}\\{tmpVideo}" };

            var P = Process.Start(PSI);

            P.WaitForExit();


            float fps = (float)((float)fr / (float)targetFps);
            if (fps > 2.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 2.0f) fps = 2.0f;
                if (fps < 0.5f) fps = 0.5f;

            }
            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -af atempo={fps} {Processing}\\VideoFileAudio.wav" };
            P = Process.Start(PSI);
            P.WaitForExit();


            PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} -c:v cinepak -max_strips 1 -an -q 31 -s 240x160 {Processing}\\{fn}_final.avi" };
            P = Process.Start(PSI);
            P.WaitForExit();

            RenderAudio($"{Processing}\\VideoFileAudio.wav");
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }
    }
}
