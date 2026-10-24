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
        // If ADPCM audio at the normal rate would use more than this share of the budget
        // (long videos), it is re-encoded at LowAudioRate instead to leave room for video.
        public static double MaxAudioShare = 0.5;
        public static int LowAudioRate = 8000;

    }
}
