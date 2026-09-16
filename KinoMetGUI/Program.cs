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
                // Optional 2nd arg selects the audio codec: 0/1/2 = raw/RLE variants (default,
                // unchanged), 3 = MP3 (see RenderAudio.RenderAudioV3 / KiNomet/AudioV3.cpp).
                int audioType = 0;
                if (args.Length > 1) int.TryParse(args[1], out audioType);
                (new Kinomet(args[0], "E:\\processing\\", "E:\\Output", 3)).Kinometize();
            }
        }
    }
}
