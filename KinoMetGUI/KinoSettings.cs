using KinometGui.Properties;

namespace KinometGui
{
    public static class KinoSettings
    {
        // 12fps: old cartoons are animated on 2s (12 drawings/s at 24fps), so more
        // frames mostly repeat drawings while splitting the same bytes thinner.
        // Measured at Steamboat Willie's budget (~2.5KB/s of video): 12fps beats
        // 30fps by ~1.2 dB PSNR / +0.03 SSIM at full res, more at half res.
        public static int FPS = 12;
        public static int SampleRate = 11025;// RenderAudio.mffreq;
        // VideoFile.bin + audio_outputmain.bin together must fit in this many bytes;
        // the KVC1 encoder is given whatever the audio leaves over.
        public static long MediaBudgetBytes = 3000000;
        // If ADPCM audio at the normal rate would use more than this share of the budget
        // (long videos), it is re-encoded at LowAudioRate instead to leave room for video.
        public static double MaxAudioShare = 0.5;
        public static int LowAudioRate = 8000;
        // KVC1 --half-res: "auto" encodes at 120x80 (the GBA scales it 2x) when the
        // video budget is under ~1 bit/pixel/second -- long videos like Steamboat
        // Willie (0.5) look far cleaner (SSIM 0.80 vs 0.65); short/high-budget ones
        // (the ALIE trailer, 2.0) stay full res. "on"/"off" force it.
        public static string HalfRes = "auto";

    }
}
