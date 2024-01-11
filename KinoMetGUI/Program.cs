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
                (new Kinomet(args[0], "E:\\processing\\", "E:\\Output", 0)).Kinometize();
            }
        }
    }
}
