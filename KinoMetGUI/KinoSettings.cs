using KinometGui.Properties;

namespace KinometGui
{
    public static class KinoSettings
    {
        public static int FPS = 15;
        public static int SampleRate = 11025;// RenderAudio.mffreq;
        // VideoFile.bin + audio_outputmain.bin together must fit in this many bytes;
        // the KVC1 encoder is given whatever the audio leaves over.
        public static long MediaBudgetBytes = 3000000;

    }
}
