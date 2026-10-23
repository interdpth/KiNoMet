using KinometGui.Properties;
using Microsoft.WindowsAPICodePack.Shell;
using System;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
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
            // Quote the path (spaces in it otherwise split the argument and ffmpeg
            // fails to open the file), and drain stderr before WaitForExit so a
            // long log can't fill the pipe and deadlock.
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true, CreateNoWindow = true, Arguments = $"-hide_banner -i \"{videoFile}\" -map 0:v:0 -c copy -f null -" };
            var P = Process.Start(PSI);
            var stdoutTask = P.StandardOutput.ReadToEndAsync();
            string err = P.StandardError.ReadToEnd();
            P.WaitForExit();
            string read = stdoutTask.Result + err;

            // ffmpeg pads the count ("frame=  299") and rewrites the progress line
            // with \r, so take the LAST frame= value and allow whitespace.
            var matches = Regex.Matches(read, @"frame=\s*(\d+)");
            if (matches.Count == 0)
            {
                throw new InvalidOperationException(
                    $"ffmpeg did not report a frame count for \"{videoFile}\" (exit code {P.ExitCode}). ffmpeg output:\n{read}");
            }
            return Convert.ToInt32(matches[matches.Count - 1].Groups[1].Value);
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

        /// <summary>
        /// Same pipeline as Kinometize(), but bakes video with KVC1-C (see
        /// KiNomet/kvc1_load.cpp) instead of ffmpeg's Cinepak encoder, and
        /// audio defaults to AudioV4/ADPCM (see RenderAudio.RenderAudioV4)
        /// instead of MP3 -- AudioV3's real GBA decoder is still a stub
        /// (mp3_decoder_gba_stub.c), so an MP3 track is silent on real
        /// hardware today, while ADPCM actually plays. Pass audiotype=3 to
        /// the constructor if a specific build genuinely wants the (silent
        /// on GBA, working on Windows) MP3 path instead.
        /// </summary>
        public void KinometizeKvc1()
        {
            FileInfo fi = new FileInfo(videoFile);
            string fn = fi.Name.Split('.')[0];
            string tmpVideo = $"{fn}.mp4";
            var vidInfoProp = ShellFile.FromFilePath(videoFile).Properties;
            var vidInfo = vidInfoProp.System.Video;
            long mInfo = (long)vidInfoProp.System.Media.Duration.Value;

            TimeSpan ts = TimeSpan.FromTicks(mInfo);
            uint fr = (vidInfo.FrameRate.Value == null ? 0 : vidInfo.FrameRate.Value.Value) / 1000;
            int targetFps = KinoSettings.FPS;
            float fps = (float)((float)fr / (float)targetFps);
            int numframes = GetFrames(videoFile);
            if (fps > 100.0 || fps < 0.5)
            {
                if (fps > 100.0f) fps = 100.0f;
                if (fps < 0.5f) fps = 0.5f;
            }

            var PSI2 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-y -i \"{videoFile}\" -filter:v fps=fps={targetFps} -s 240x160 \"{Processing}\\{tmpVideo}\"" };
            var P2 = Process.Start(PSI2);
            P2.WaitForExit();

            string outPutAud = "wav";
            string audioSrcForRender = $"{Processing}\\audio_outputmain.{outPutAud}";
            string temp = "";
            if (fps != 1)
            {
                temp = $"-filter:a \"atempo = {fps}\"";
            }
            if (File.Exists(audioSrcForRender)) File.Delete(audioSrcForRender);
            var PSI1 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-y -i \"{Processing}\\{tmpVideo}\" {temp} -ac 1 -ar {KinoSettings.SampleRate} \"{audioSrcForRender}\"" };
            var P1 = Process.Start(PSI1);
            P1.WaitForExit();
            if (!File.Exists(audioSrcForRender))
            {
                Thread.Sleep(2000);
            }

            (new RenderAudio(audiov, $"{OutputFolder}", audioSrcForRender, targetFps, (int)numframes)).Render();

            // Hand the resized clip to the Python encoder (KinoMetGUI/kvc1_tools)
            // instead of ffmpeg's Cinepak path. Requires python3 with numpy and
            // scikit-learn on PATH (see that tool's own requirements).
            string kvc1Path = $"{Processing}\\{fn}.kvc1";
            if (File.Exists(kvc1Path)) File.Delete(kvc1Path);
            string toolsDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "kvc1_tools");
            var PSI3 = new ProcessStartInfo
            {
                FileName = "python3",
                UseShellExecute = true,
                CreateNoWindow = true,
                Arguments = $"\"{Path.Combine(toolsDir, "kvc1_encoder.py")}\" \"{Processing}\\{tmpVideo}\" \"{kvc1Path}\" --fps {targetFps}"
            };
            var P3 = Process.Start(PSI3);
            P3.WaitForExit();
            if (!File.Exists(kvc1Path))
            {
                Console.WriteLine("kvc1_encoder.py did not produce an output file -- check it ran and python3/numpy/scikit-learn are installed.");
                return;
            }

            ROM.MakeSource("VideoFile", File.ReadAllBytes(kvc1Path), $"{OutputFolder}");
            ROM.Write(OutputFolder, "VideoFile");
        }
    }
}
