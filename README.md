# KiNoMet


Reimplementation of Meteo
Why KinoMet? 
In reference to cinema, Kino is a shortening of the German kinematograph, meaning “motion-picture projector” and related to English's own cinematography.
The No is capitalized because it overrides Meteo. Then then Met is for meteo lol. Meteo is fantastic software by the way and while it is abandonware I hope the devs are doing awesome things. 

Support: 
How many frames? Idk see how many you can get. I had issues upscaling from 24 to 30 with ffmpeg. 
Supports GBA mode 3 and mode 5(soon)
When audio? When video is faster.


Compile Guide:
Forgot all the libraries, so please just file a bug if you can't get it to work.
Requires:
Windows(Mono?)
Visual Studio(can be converted to CMake I guess but that's beyond my scope)
DevkitPro, just the GBA kit.
After syncing the repo, make your own branch. Unless you just got a zip....then...unzip.
Open the SLN. 
ffmpeg binary. 
TileShopPro https://github.com/stevemonaco/ImageMagitek/releases
Build the projects in Windows, Run KiNoMetGui on your video file.
Once it outputs VideoFile.cpp and VideoFile.h toss that in KinoMetWindows folder and build the project and let it run. After it's done 
Use tileshop pro to verify the frames are good by opening them up using the PSX16bpp filter, and dimension 240x160 or the ones for mode 5
after which, toss the VideoFile.cpp and VideoFile.h into the KinoMetGba folder. 
Next hit make in the Kinomet folder so it builds for GBA. Then hit make in the KinometGba folder. Check your video and change settings accordingly(soon)



Random notes from Development:
.\ffmpeg.exe -i Alie.avi -c:v cinepak -an -q 25 -s 240x160 -c:a pcm_s8 Alie2.avi

meteo uses cinepak, by limiting fps to 20


    make a slow version first that translates the whole frame from 24 to 16bit on console. 
unlike meteo that uses windows media library Converters, we will just take input video
set audio into one file
add video to another 
remux manually

make player

port to C

find best decoder 

best decoder = 1 buffer, 2 max.
meteo runs some of the decoding in oam so probably use that for buffering 
it uses 0x600000+240*160*2 as the starting point for that area.


https://github.com/BeWorld2018/rawgl/blob/a3bfd1b66fe031eac5b2bf70724d0590f01d6393/tools/convert_3do/cinepak.cpp

https://github.com/sdelmas/SDesk/blob/b9e00bc57c97c719cc8ac3a07abc00dc4b570bc1/vlc-0.8.1/modules/codec/cinepak.c

https://github.com/nemomobile-apps/scummvm/blob/092a2bbbe33425c6725c3a3a23db33fae8c7ce3f/video/codecs/cinepak.h
https://github.com/enriquesomolinos/Another-World-Vita/blob/68b2e0a2b263ce669ad33452130ac01ed7683e5a/tools/convert_3do/cinepak.cpp
https://github.com/maemo-leste-extras/scummvm/blob/db460aa52a738e37d50dfcff2683b0ce5357c235/image/codecs/cinepak.hs
"JUST USE FFMPEG" 


Wild shit
//            string cmd = @"
//geq=
//r = 'bitor(bitor((r(X,Y) /8), (g(X,Y) /8) * 32),(b(X,Y) /8)*1024)': 
//g = 'bitand(r(X,Y),255)':
//r = 'bitand(r(X,Y),65280)/100':
//b = 'g(X,Y)'
//";
//            cmd = "\"" + cmd + "\"";
//            PSI = new ProcessStartInfo
//            {
//                FileName = "ffmpeg.exe",
//                UseShellExecute = true,
//                CreateNoWindow = true,
//                Arguments = $"-i {Processing}\\{tmpVideo} -vf {cmd} -c:v cinepak -max_strips 3 -an -q 31 -s 240x160 {Processing}\\{fn}_final.avi"
//            };



instead of applying yuv math and keeping lots of memory around, meteo just encodes the rgb15 value while reading the codebook which allows for massive gains. should allow for 2 more strips.
