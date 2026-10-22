using NAudio.Wave;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;

namespace KinometGui.Properties
{

    public class Tmp
    {
        public int count;
        public int len;
        public int offset;
        public Tmp(int c, int l,int o)
        {
            count = c;
            len = l;
            offset = o;
        }
    }
    public class RenderAudio
    {
        public static int mffreq = 10512;
        public static int zmfreq = 13379;
        public static int freq = mffreq;
        // MP3 (MPEG Layer III) only supports a fixed set of sample rates. 11025Hz (MPEG-2.5)
        // is the closest standard rate to mffreq and is what AudioV3/RenderAudioV3 use.
        public static int mp3freq = 11025;
        // Mono CBR bitrate (kbps) used when encoding AudioV3 tracks. Low enough to keep
        // files small for a GBA cart, plenty for the GBA's speaker/headphone output.
        public static int mp3Bitrate = 32;
        private string OutputFolder { get; set; }
        private string srcFile { get; set; }
        private int fps { get; set; }
        private int numframes { get; set; }
        private int audiov { get; set; }
        public static short DecompSize = 0x500;
        public RenderAudio(int type, string outFolder, string sf, int framps, int noframes)
        {
            audiov = type;
            OutputFolder = outFolder;
            srcFile = sf;
            fps = framps;
            numframes = noframes;
        }
        public void Render()
        {
            switch (audiov)
            {
                case 0:
                    RenderAudio_old();
                    break;
                case 1:
                    RenderAudioV1();
                    break;
                case 2:
                    RenderAudioV2();
                    break;
                case 3:
                    RenderAudioV3();
                    break;
                case 4:
                    RenderAudioV4();
                    break;
            }
        }
        private List<sbyte> DecodeFile(FileInfo srcAudio)
        {
            List<sbyte> data = null;
            WaveStream srcStream = null;
            //we need to be 8bit and mono channel, apply desired frequency.
            var outFormat = new WaveFormat(freq, 8, 1);

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
            using (WaveFormatConversionStream conversionStream = new WaveFormatConversionStream(outFormat, srcStream))
            {
                using (RawSourceWaveStream raw = new RawSourceWaveStream(conversionStream, outFormat))
                {
                    //Convert to signed 8bit.
                    raw.Seek(0, SeekOrigin.Begin);
                    int len = 0;
                    data = new List<sbyte>();
                    for (; len < raw.Length; len++)
                    {
                        sbyte n = Convert.ToSByte(raw.ReadByte() - 128);
                        data.Add(n);

                    }
                }
            }

            return data;
        }
        private void RenderAudio_old()
        {
            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }
            //Convert to signed 8bit.
            AudioHeader aHdr = new AudioHeader(0x41555630, int.MaxValue, (uint)srcAudio.Length, (ushort)audiov, (ushort)fps, (uint)freq);
            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            if (File.Exists($"{OutputFolder}\\{fn}.raw")) File.Delete($"{OutputFolder}\\{fn}.raw");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                //tmp
                byte[] fbuffer = new byte[AudioHeader.GetHdrSize()];
                Util.Memset(fbuffer, 0, fbuffer.Length);
                bw.Write(fbuffer);

                
                for (int len = 0; len < srcStream.Count; len++)
                {
                    bw.Write(srcStream[len]);
                }
                aHdr.compressedlength = (uint)((int)bw.BaseStream.Length - AudioHeader.GetHdrSize());
                aHdr.Write(bw);


                bw.Close();
            }

            ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
            ROM.Write(OutputFolder, fn);
            Debug.Write(JsonConvert.SerializeObject(aHdr));
        }

        private void RenderAudioV1()
        {

            Dictionary<string, Tmp> found = new Dictionary<string, Tmp>();
            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }

            AudioHeader aHdr = new AudioHeader(0x41555631, int.MaxValue, (uint) srcAudio.Length, (ushort)audiov, (ushort)fps, (uint)freq);

            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                using (MemoryStream pointerTable = new MemoryStream())
                {
                    using (MemoryStream dataTable = new MemoryStream())
                    {
                        //tmp
                        byte[] fbuffer = new byte[AudioHeader.GetHdrSize()];
                        Util.Memset(fbuffer, 0, fbuffer.Length);
                        bw.Write(fbuffer);

                        //done writing temp data
                        int encodedFileInfo = (int)bw.BaseStream.Position;




                        bw.Write(0xFFFFFFFF);//Count
                        bw.Write(0xFFFFFFFF);//Index Pointers
                        bw.Write(0xFFFFFFFF);//Data 
                        int streamLength = srcStream.Count;
                        int bufsize = 800;// KinoSettings.SampleRate;
                        int fnsize = ((int)streamLength / bufsize);
                        int sz = bufsize;
                        int ezCount = 0;
                        Array astream = srcStream.ToArray();
                        for (int i = 0; i < streamLength / sz; i++)
                        {
                            //buffer is fps*size;
                            byte[] buf = new byte[sz];
                            Buffer.BlockCopy(astream, i * sz, buf, 0, sz);
                            byte[] hashBytes;

                            using (SHA256 sha256Hash = SHA256.Create())
                            {
                                hashBytes = sha256Hash.ComputeHash(buf);
                            }

                            string hashString = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();
                            bool wasFound = false;
                            Tmp curCount = null;
                            if (found.ContainsKey(hashString))
                            {
                                wasFound = true;
                            }
                            else
                            {
                                found.Add(hashString, new Tmp(0, buf.Length, (int)dataTable.Position));
                            }
                            curCount = found[hashString];
                            curCount.count++;
                            MimirData dat = new MimirData(new IOStream(buf));

                            ChariotWheels compType = ChariotWheels.Raw;
                            IOStream stream = dat.srcDat;
                            UInt16 best = (UInt16)dat.srcDat.Length;
                            if (dat.lz.Length < best)
                            {
                                compType = ChariotWheels.LZ;
                                stream = dat.lz;
                                best = (UInt16)stream.Length;
                            }

                            if (dat.rle.Length < best)
                            {
                                compType = ChariotWheels.RLE;
                                stream = dat.rle;
                                best = (UInt16)stream.Length;
                            }

                            if(!wasFound)
                            {
                                pointerTable.Write(BitConverter.GetBytes(dataTable.Position), 0, 4);
                                // dataTable.Write(BitConverter.GetBytes(0xDEADBEEF), 0, 4);
                                //dataTable.Write(BitConverter.GetBytes(ezCount), 0, 4);//wtf
                                dataTable.WriteByte((byte)compType);
                                dataTable.Write(BitConverter.GetBytes(best), 0, 2);
                                dataTable.Write(stream.Data, 0, best);
                            }
                            else 
                            { 
                                pointerTable.Write(BitConverter.GetBytes(curCount.offset), 0, 4); 
                            }

                            ezCount++;
                        }

                        int pointerTableIndex = (int)bw.BaseStream.Position;
                        bw.Write(pointerTable.ToArray(), 0, (int)pointerTable.Length);

                        int dataTableOffset = (int)bw.BaseStream.Position;

                        bw.Write(dataTable.ToArray(), 0, (int)dataTable.Length);

                       

                        bw.BaseStream.Position = encodedFileInfo;
                        bw.Write(ezCount);
                        bw.Write(pointerTableIndex);
                        bw.Write(dataTableOffset);
                        aHdr.compressedlength =(uint)( (int)bw.BaseStream.Length - AudioHeader.GetHdrSize());
                        aHdr.Write(bw);                      

                        bw.Close();

                    }
                }
                int saved = 0;
                found.Keys.ToList().ForEach(key =>
                {
                    if (found[key].count > 1)
                    {
                        Console.WriteLine($"{key} found {found[key].count} times with a size of {found[key].len}");

                
                        saved += found[key].count * found[key].len;
                    }
                });

                Console.WriteLine($"Saved {saved} bytes thanks to pointers");
                ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
                ROM.Write(OutputFolder, fn);

            }
        }

        private void RenderAudioV2()
        {
            Dictionary<string, Tmp> found = new Dictionary<string, Tmp>();
            FileInfo srcAudio = new FileInfo(srcFile);

            //Find out level of decode 
            List<sbyte> srcStream = DecodeFile(srcAudio);


            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }
            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                using (MemoryStream pointerTable = new MemoryStream())
                {
                    using (MemoryStream dataTable = new MemoryStream())
                    {

                        bw.Write(0x41555632);//AUV1

                        UInt16 atype = (UInt16)audiov;

                        byte[] hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);
                        int compressedsizeoff = (int)bw.BaseStream.Position;
                        bw.Write(0xFFFFFFFF);
                      
                        bw.Write(srcAudio.Length);
                        atype = (UInt16)fps;

                        hdr = BitConverter.GetBytes(atype);
                        bw.Write(hdr);

                        var atype2 = freq;

                        hdr = BitConverter.GetBytes(atype2);
                        bw.Write(hdr);

                        int hdrOffset = (int)bw.BaseStream.Position;
                        bw.Write(0xFFFFFFFF);//Count
                        bw.Write(0xFFFFFFFF);//Index Pointers
                        bw.Write(0xFFFFFFFF);//Data 

                        int hdrsize = (int)bw.BaseStream.Position;
                        int bufsize = 32;




                        int fnsize = ((int)srcStream.Count / bufsize);
                        // int sz = fps * bufsize;
                        int ezCount = 0;
                        int audioSamples = (fnsize);

                        //How can we sync, we don't :) 
                        for (int i = 0; i < audioSamples; i++)
                        {
                            //buffer is fps*size;
                            byte[] buf = new byte[bufsize];
                            Buffer.BlockCopy(srcStream.ToArray(), i * bufsize, buf, 0, bufsize);
                            byte[] hashBytes;

                            using (SHA256 sha256Hash = SHA256.Create())
                            {
                                hashBytes = sha256Hash.ComputeHash(buf);
                            }

                            string hashString = BitConverter.ToString(hashBytes).Replace("-", "").ToLowerInvariant();

                            Tmp curCount = null;
                            if (found.ContainsKey(hashString))
                            {
                            }
                            else
                            {
                                found.Add(hashString, new Tmp(0, buf.Length, (int) dataTable.Position));
                            }
                            curCount = found[hashString];
                            curCount.count++;
                            found[hashString] = curCount;
                            MimirData dat = new MimirData(new IOStream(buf));

                            ChariotWheels compType = ChariotWheels.Raw;
                            IOStream stream = dat.srcDat;
                            int best = (int)dat.srcDat.Length;
                            if (dat.lz.Length < best)
                            {
                                compType = ChariotWheels.LZ;
                                stream = dat.lz;
                                best = (int)stream.Length;
                            }

                            if (dat.rle.Length < best)
                            {
                                compType = ChariotWheels.RLE;
                                stream = dat.rle;
                                best = (int)stream.Length;
                            }

                            //HELLO DOES THE DATA EXIST?

                            pointerTable.Write(BitConverter.GetBytes(dataTable.Position), 0, 4);
                            dataTable.WriteByte((byte)compType);
                            dataTable.Write(BitConverter.GetBytes((ushort)best), 0, 2);
                            dataTable.Write(stream.Data, 0, best);
                            ezCount++;
                        }




                        int pointerTableIndex = (int)bw.BaseStream.Position;
                        bw.Write(pointerTable.ToArray(), 0, (int)pointerTable.Length);

                        int dataTableOffset = (int)bw.BaseStream.Position;

                        bw.Write(dataTable.ToArray(), 0, (int)dataTable.Length);
                    
                      
                        bw.BaseStream.Position = hdrOffset;
                        bw.Write(ezCount);
                        bw.Write(pointerTableIndex);
                        bw.Write(dataTableOffset);
                        bw.Seek(0, SeekOrigin.End);
                        int lol = (int)bw.BaseStream.Position;
                        

                            bw.BaseStream.Position = compressedsizeoff;
                        bw.Write(lol-compressedsizeoff);
                        bw.Close();

                    }
                }



                //lets see all the data we could have saved!

                found.Keys.ToList().ForEach(key =>
                {
                    Console.WriteLine($"{key} found {found[key].count} times with a size of {found[key].len}");
                });
                ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
                ROM.Write(OutputFolder, fn);

                return;
            }


            //private void EmergencyRender()

            //{
            //    using (RawSourceWaveStream raw = new RawSourceWaveStream(srcStream, outFormat))
            //    {
            //        //Convert to signed 8bit.
            //        raw.Seek(0, SeekOrigin.Begin);
            //        int len = 0;
            //        List<sbyte> data = new List<sbyte>();
            //        for (; len < raw.Length; len++)
            //        {
            //            byte[] tmp = new byte[12];
            //            raw.Read(tmp, 0, 12);
            //            len += 12;
            //            for (int i = 0; i < 12; i++)
            //            {
            //                sbyte n = Convert.ToSByte(tmp[i] - 128);
            //                data.Add(n);
            //            }


            //        }                    //Generate 
            //                             //Write it
            //        string fn = srcAudio.Name.Replace(".wav", "");
            //        using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            //        using (BinaryWriter bw = new BinaryWriter(fs))
            //        {
            //            for (len = 0; len < data.Count; len++)
            //            {
            //                bw.Write(data[len]);
            //            }
            //            bw.Close();
            //        }

            //        ROM.MakeSource(fn, data.ToArray(), OutputFolder);
            //        ROM.Write(OutputFolder, fn);

            //    }
            //}

        }

        /// <summary>
        /// Stores the audio as a real MP3 file (see Kinomet.cs, which encodes
        /// audio_outputmain.wav to audio_outputmain.mp3 via ffmpeg/libmp3lame before
        /// calling Render() with audiov==3). Unlike V0/V1/V2, which store raw or
        /// lightly RLE/LZ-compressed 8-bit PCM, this keeps the compressed MP3 bytes
        /// as-is -- MP3 typically compresses 8-15x smaller than that PCM, which is
        /// the actual fix for "videos are huge due to audio". Decoding happens at
        /// playback time (see KiNomet/AudioV3.cpp).
        /// </summary>
        private void RenderAudioV3()
        {
            FileInfo srcAudio = new FileInfo(srcFile);

            if (srcAudio.Extension.ToLower() != ".mp3")
            {
                Console.WriteLine($"RenderAudioV3 expects an already-encoded .mp3 file, got {srcAudio.Extension}");
                return;
            }

            byte[] mp3Bytes = File.ReadAllBytes(srcFile);
            if (mp3Bytes == null || mp3Bytes.Length == 0)
            {
                Console.WriteLine("MP3 source is empty or missing.");
                return;
            }

            AudioHeader aHdr = new AudioHeader(0x41555633, (uint)mp3Bytes.Length, (uint)mp3Bytes.Length, (ushort)audiov, (ushort)fps, (uint)mp3freq);

            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            if (File.Exists($"{OutputFolder}\\{fn}.raw")) File.Delete($"{OutputFolder}\\{fn}.raw");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                byte[] fbuffer = new byte[AudioHeader.GetHdrSize()];
                Util.Memset(fbuffer, 0, fbuffer.Length);
                bw.Write(fbuffer);

                bw.Write(mp3Bytes);

                aHdr.compressedlength = (uint)mp3Bytes.Length;
                aHdr.Write(bw);

                bw.Close();
            }

            Console.WriteLine($"MP3 audio: {mp3Bytes.Length} bytes @ {mp3Bitrate}kbps");

            ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
            ROM.Write(OutputFolder, fn);
        }

        // Step/index tables and the encode algorithm below MUST stay bit-for-bit
        // identical to KiNomet/adpcm_decode.c's kAdpcmStepTable/kAdpcmIndexTable/
        // adpcm_decode(). Both sides were derived from, and checked bit-exact
        // against, the same Python reference prototype before either was
        // written -- see adpcm_decode.h's module comment for why this is a
        // from-scratch 8-bit-domain table rather than the standard 16-bit IMA
        // table scaled down. If you touch one side, mirror the change on the
        // other, or every AudioV4 track will decode to noise.
        private static readonly int[] AdpcmStepTable = {
              1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
             11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
             21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
             31, 32, 33, 34, 35, 36, 38, 42, 46, 51,
             57, 63, 69, 77, 85, 94,104,115,127,
        };
        private static readonly int[] AdpcmIndexTable = { -1, -1, -1, -1, 2, 4, 6, 8 };

        private static int AdpcmClamp(int v, int lo, int hi) => v < lo ? lo : (v > hi ? hi : v);

        /// <summary>
        /// Encodes 8-bit signed PCM into the 4-bit ADPCM nibble stream AudioV4 /
        /// adpcm_decode.c expect (2 samples/byte, high nibble first). valpred/index
        /// persist across the WHOLE track, matching adpcm_state_t's lifetime on the
        /// decode side (AudioV4.h) -- resetting them per chunk would re-sync against
        /// a decoder that never resets and audibly degrade quality at every frame
        /// boundary. The nibble stream itself IS flushed to a byte boundary at the
        /// end of every chunkSize-sample chunk (padding with a zero low nibble if
        /// chunkSize happens to be odd for this fps), because AudioV4::DecodeMore()
        /// also starts each GetNextFrame() call on a fresh byte. chunkSize = freq/fps
        /// is even for this project's actual fps targets (e.g. 10512/15 = 700), so
        /// that padding is a correctness guard for other fps values, not a
        /// routinely-exercised path.
        /// </summary>
        private static byte[] AdpcmEncodeTrack(List<sbyte> pcm, int targetFps)
        {
            int chunkSize = targetFps > 0 ? freq / targetFps : freq;
            if (chunkSize < 1) chunkSize = 1;

            using (MemoryStream outStream = new MemoryStream())
            {
                int valpred = 0;
                int index = 0;
                int pos = 0;
                int n = pcm.Count;

                while (pos < n)
                {
                    int take = Math.Min(chunkSize, n - pos);
                    int step = AdpcmStepTable[index];
                    int? pendingHi = null;

                    for (int i = 0; i < take; i++)
                    {
                        int sample = pcm[pos + i];
                        int diff = sample - valpred;
                        int sign = diff < 0 ? 8 : 0;
                        if (sign != 0) diff = -diff;

                        int vpdiff = step >> 3;
                        int delta = 0;
                        int d = diff;
                        int t = step;
                        if (d >= t) { delta = 4; d -= t; vpdiff += t; }
                        t >>= 1;
                        if (d >= t) { delta |= 2; d -= t; vpdiff += t; }
                        t >>= 1;
                        if (d >= t) { delta |= 1; vpdiff += t; }

                        if (sign != 0) valpred -= vpdiff; else valpred += vpdiff;
                        valpred = AdpcmClamp(valpred, -128, 127);

                        delta |= sign;
                        index = AdpcmClamp(index + AdpcmIndexTable[delta & 7], 0, AdpcmStepTable.Length - 1);
                        step = AdpcmStepTable[index];

                        if (pendingHi == null)
                        {
                            pendingHi = delta;
                        }
                        else
                        {
                            outStream.WriteByte((byte)(((pendingHi.Value << 4) & 0xF0) | (delta & 0x0F)));
                            pendingHi = null;
                        }
                    }
                    if (pendingHi != null)
                    {
                        outStream.WriteByte((byte)((pendingHi.Value << 4) & 0xF0));
                    }

                    pos += take;
                }

                return outStream.ToArray();
            }
        }

        /// <summary>
        /// Stores audio as 4-bit ADPCM (see adpcm_decode.h/.c, AudioV4.h/.cpp).
        /// Half the size of raw 8-bit PCM (AudioV0/V1/V2's uncompressed case) with
        /// much better quality than those codecs' RLE/LZ-on-raw-PCM fallback, and --
        /// unlike AudioV3/MP3, whose real GBA decoder is still a stub -- actually
        /// produces audible output on real hardware today. See AudioV4.h's class
        /// comment for the full tradeoff.
        /// </summary>
        private void RenderAudioV4()
        {
            FileInfo srcAudio = new FileInfo(srcFile);

            List<sbyte> srcStream = DecodeFile(srcAudio);
            if (srcStream == null)
            {
                Console.WriteLine($"{srcAudio.Extension} is an unsupported format");
                return;
            }

            byte[] encoded = AdpcmEncodeTrack(srcStream, fps);

            AudioHeader aHdr = new AudioHeader(0x41555634, (uint)encoded.Length, (uint)srcAudio.Length, (ushort)audiov, (ushort)fps, (uint)freq);

            string fn = srcAudio.Name.Replace(srcAudio.Extension, "");
            if (File.Exists($"{OutputFolder}\\{fn}.raw")) File.Delete($"{OutputFolder}\\{fn}.raw");
            using (FileStream fs = new FileStream($"{OutputFolder}\\{fn}.raw", FileMode.OpenOrCreate))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                byte[] fbuffer = new byte[AudioHeader.GetHdrSize()];
                Util.Memset(fbuffer, 0, fbuffer.Length);
                bw.Write(fbuffer);

                bw.Write(encoded);

                aHdr.compressedlength = (uint)encoded.Length;
                aHdr.Write(bw);

                bw.Close();
            }

            double ratio = encoded.Length > 0 ? (double)srcStream.Count / encoded.Length : 0;
            Console.WriteLine($"ADPCM audio: {srcStream.Count} PCM bytes -> {encoded.Length} bytes ({ratio:0.00}x)");

            ROM.MakeSource(fn, File.ReadAllBytes($"{OutputFolder}\\{fn}.raw"), OutputFolder);
            ROM.Write(OutputFolder, fn);
        }
    }
}
