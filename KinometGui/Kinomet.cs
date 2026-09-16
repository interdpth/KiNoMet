using KinometGui.Properties;
using Microsoft.WindowsAPICodePack.Shell;
using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Web;

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


        public int GetFrames(string videoFile)
        {
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -map 0:v:0 -c copy -f null -" };
            var P = Process.Start(PSI);
            P.WaitForExit();

            string read = P.StandardOutput.ReadToEnd() + P.StandardError.ReadToEnd();

            int idx = read.IndexOf("frame=");
            string news = read.Substring(idx + 6);
            string val = news.Split(' ')[0];
            int ret = Convert.ToInt32(val);
            return ret;

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
            int targetFps = KinoSettings.FPS;
            float fps = (float)((float)fr / (float)targetFps);
            int numframes = GetFrames(videoFile);
             if (fps > 100.0 || fps < 0.5)
            {
                Console.WriteLine("Range is bad, clamping value");
                if (fps > 100.0f) fps = 100.0f;
                if (fps < 0.5f) fps = 0.5f;
            }

            ////  Do some intial conversions.
            ///
            Thread vid = new Thread(() =>
            {
                var PSI2 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -crf 10 -s 240x160 {Processing}\\{tmpVideo}" };
                var P2 = Process.Start(PSI2);
                P2.WaitForExit();
            });

           
                string outPutAud = "wav";

                if (audiov == 3)
                {
                    outPutAud = "mp3";

                }
                string audioSrcForRender = $"{Processing}\\audio_outputmain.{outPutAud}";
             Thread aud = new Thread(() =>
            {    string temp = "";
                if (fps != 1)
                {
                    temp = $"-filter:a \"atempo = {fps}\"";
                }
                if (File.Exists($"{Processing}\\audio_outputmain.{outPutAud}")) File.Delete($"{Processing}\\audio_outputmain.{outPutAud}");
                var PSI1 = new ProcessStartInfo();
                if (audiov == 3)
                {
                    PSI1 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} -ac 1 -ar {KinoSettings.SampleRate} -codec:a libmp3lame -b:a 32k {Processing}\\audio_outputmain.{outPutAud}" };

                }
                else
                {
                    PSI1 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} -ac 1 -ar {KinoSettings.SampleRate} {Processing}\\audio_outputmain.{outPutAud}" };

                }
                var P1 = Process.Start(PSI1);
                P1.WaitForExit();
                if (!File.Exists($"{Processing}\\audio_outputmain.{outPutAud}"))
                {
                    Thread.Sleep(2000);
                }

                
                //if (audiov == 3)
                //{
                //    // MP3 (audiov==3): re-encode the extracted wav as a real, compressed MP3
                //    // instead of feeding raw/RLE'd PCM into RenderAudio -- this is what actually
                //    // shrinks the audio down. MP3 only supports a fixed set of sample rates, so
                //    // this targets RenderAudio.mp3freq (11025Hz) rather than KinoSettings.SampleRate;
                //    // ffmpeg resamples from the wav's rate automatically.
                //    string mp3Path = $"{Processing}\\audio_outputmain.{outPutAud}";
                //    if (File.Exists(mp3Path)) File.Delete(mp3Path);
                //    PSI1 = new ProcessStartInfo
                //    {
                //        FileName = "ffmpeg.exe",
                //        UseShellExecute = true,
                //        CreateNoWindow = true,
                //        Arguments = $"-i {audioSrcForRender} -ac 1 -ar {RenderAudio.mp3freq} -codec:a libmp3lame -b:a {RenderAudio.mp3Bitrate}k {mp3Path}"
                //    };
                //    P1 = Process.Start(PSI1);
                //    P1.WaitForExit();
                //    if (!File.Exists(mp3Path))
                //    {
                //        Thread.Sleep(2000);
                //    }
                //    audioSrcForRender = mp3Path;
                //}
            }
            );
            aud.Start();
          vid.Start();

           vid.Join();
            aud.Join();

          

            (new RenderAudio(audiov, $"{OutputFolder}", audioSrcForRender, targetFps, (int)numframes)).Render();

            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-fflags discardcorrupt -i {Processing}\\{tmpVideo} -vf mpdecimate -c:v cinepak -max_strips 5 -q 30 -s 240x160 -ar {KinoSettings.SampleRate} -an {Processing}\\{fn}_final.avi" };
            var P = Process.Start(PSI);
            P.WaitForExit();
            ROM.MakeSource("VideoFile", File.ReadAllBytes($"{Processing}\\{fn}_final.avi"), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }
    }
}
