using System;
using System.Windows.Forms;

namespace KinometGui
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new Form1());
            }
            else
            {
                // Optional 2nd arg selects the audio codec: 0/1/2 = raw/RLE variants, 3 = MP3
                // (see RenderAudio.RenderAudioV3 / KiNomet/AudioV3.cpp -- silent on real GBA
                // hardware today, its decoder is still a stub), 4 = ADPCM (default; see
                // RenderAudio.RenderAudioV4 / KiNomet/AudioV4.cpp -- actually plays on GBA).
                int audioType = 4;
                if (args.Length > 1) int.TryParse(args[1], out audioType);
                // KinometizeKvc1(), not Kinometize(): bakes video with KVC1-C (see
                // KiNomet/kvc1_load.cpp) instead of Cinepak.
                (new Kinomet(args[0], "E:\\processing\\", "E:\\Output", audioType)).KinometizeKvc1();
            }
        }
    }
}
