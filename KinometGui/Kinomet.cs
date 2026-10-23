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
            var PSI = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = false, RedirectStandardError = true, RedirectStandardOutput = true, CreateNoWindow = true, Arguments = $"-hide_banner -i \"{videoFile}\" -map 0:v:0 -c copy -f null -" };
            string read;
            using (var P = Process.Start(PSI))
            {
                // Read both pipes before waiting, or ffmpeg can block on a full stderr pipe.
                var stdoutTask = P.StandardOutput.ReadToEndAsync();
                string stderr = P.StandardError.ReadToEnd();
                P.WaitForExit();
                read = stdoutTask.Result + stderr;
            }

            // ffmpeg pads the count ("frame=  2700 fps=...") and prints many progress
            // updates; the last one is the final total.
            var matches = System.Text.RegularExpressions.Regex.Matches(read, @"frame=\s*(\d+)");
            if (matches.Count == 0)
                throw new InvalidOperationException($"ffmpeg reported no frame count for {videoFile}:\n{read}");
            return int.Parse(matches[matches.Count - 1].Groups[1].Value);

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
            File.Copy($"{Processing}\\{fn}_final.avi", $"{OutputFolder}\\VideoFile.bin", true);
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

            var PSI2 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {videoFile} -filter:v fps=fps={targetFps} -s 240x160 {Processing}\\{tmpVideo}" };
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
            var PSI1 = new ProcessStartInfo { FileName = "ffmpeg.exe", UseShellExecute = true, CreateNoWindow = true, Arguments = $"-i {Processing}\\{tmpVideo} {temp} -ac 1 -ar {KinoSettings.SampleRate} {audioSrcForRender}" };
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
            string encoderScript = FindKvc1Encoder();
            if (encoderScript == null)
            {
                Debug.WriteLine($"Could not find kvc1_tools\\kvc1_encoder.py next to {AppDomain.CurrentDomain.BaseDirectory} or in any parent folder.");
                return;
            }
            string encoderArgs = $"\"{encoderScript}\" \"{Processing}\\{tmpVideo}\" \"{kvc1Path}\" --fps {targetFps}";
            int encoderExit = RunPython(encoderArgs);
            if (encoderExit != 0 || !File.Exists(kvc1Path))
            {
                Debug.WriteLine($"kvc1_encoder.py failed (exit code {encoderExit}); its console window showed the error. " +
                                "It needs Python 3 with numpy and scikit-learn: py -3 -m pip install numpy scikit-learn");
                return;
            }

            // Raw binary: GBA links data/VideoFile.bin, Windows loads it at runtime.
            File.Copy(kvc1Path, $"{OutputFolder}\\VideoFile.bin", true);
            Debug.WriteLine($"Wrote {OutputFolder}\\VideoFile.bin and audio_outputmain.bin -- copy both into KinoMetGBA\\data (and next to KinometWindows.exe).");
        }

        /// <summary>
        /// Locates kvc1_tools\kvc1_encoder.py. The build doesn't copy the scripts
        /// next to the exe, so walk up from bin\Debug until the source tree's
        /// KinoMetGUI\kvc1_tools folder is found.
        /// </summary>
        private static string FindKvc1Encoder()
        {
            var dir = new DirectoryInfo(AppDomain.CurrentDomain.BaseDirectory);
            while (dir != null)
            {
                foreach (var sub in new[] { "kvc1_tools", Path.Combine("KinoMetGUI", "kvc1_tools") })
                {
                    string candidate = Path.Combine(dir.FullName, sub, "kvc1_encoder.py");
                    if (File.Exists(candidate)) return candidate;
                }
                dir = dir.Parent;
            }
            return null;
        }

        /// <summary>
        /// Runs a Python 3 script in its own console window so its progress is
        /// visible live. On success the window lingers a few seconds so the final
        /// summary (mode mix, file size) can be read; on failure it stays open with
        /// the error until a key is pressed. Returns the script's exit code, or -1
        /// if no working Python 3 interpreter was found.
        /// </summary>
        private static int RunPython(string args)
        {
            string[] interp = FindPython();
            if (interp == null)
            {
                Debug.WriteLine("No Python 3 interpreter found (tried py -3, python, python3).");
                return -1;
            }
            Debug.WriteLine($"Running: {interp[0]} {interp[1]}-u {args}");

            // cmd /s strips only the outer quotes, so the inner quoted paths survive.
            // /v:on enables !rc! so the exit code is read after the script finishes.
            string cmdLine =
                $"\"\"{interp[0]}\" {interp[1]}-u {args}" +
                " & set \"rc=!errorlevel!\"" +
                " & if !rc!==0 (echo. & echo KVC1 encode finished. & timeout /t 5 & exit /b 0)" +
                " else (echo. & echo KVC1 encode FAILED with exit code !rc! -- press any key to close. & pause >nul & exit /b !rc!)\"";
            var psi = new ProcessStartInfo
            {
                FileName = "cmd.exe",
                Arguments = "/v:on /s /c " + cmdLine,
                UseShellExecute = false,
                CreateNoWindow = false,
            };
            using (var p = Process.Start(psi))
            {
                p.WaitForExit();
                Debug.WriteLine($"kvc1_encoder.py exited with code {p.ExitCode}");
                return p.ExitCode;
            }
        }

        /// <summary>
        /// Returns { exe, argPrefix } for the first working Python 3: the "py -3"
        /// launcher, then "python", then "python3" (on Windows "python3" is often
        /// just the Microsoft Store stub, which exits 9009 without running anything).
        /// </summary>
        private static string[] FindPython()
        {
            var interpreters = new[] { new[] { "py", "-3 " }, new[] { "python", "" }, new[] { "python3", "" } };
            foreach (var interp in interpreters)
            {
                try
                {
                    var psi = new ProcessStartInfo
                    {
                        FileName = interp[0],
                        Arguments = interp[1] + "--version",
                        UseShellExecute = false,
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        CreateNoWindow = true,
                    };
                    using (var p = Process.Start(psi))
                    {
                        var outTask = p.StandardOutput.ReadToEndAsync();
                        string err = p.StandardError.ReadToEnd();
                        p.WaitForExit();
                        string ver = (outTask.Result + err).Trim();
                        if (p.ExitCode == 0 && ver.StartsWith("Python 3"))
                            return interp;
                        Debug.WriteLine($"{interp[0]}: not a working Python 3 ({ver})");
                    }
                }
                catch (System.ComponentModel.Win32Exception ex)
                {
                    Debug.WriteLine($"{interp[0]}: {ex.Message}");
                }
            }
            return null;
        }
    }
}
