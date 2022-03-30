#pragma once
////example
///*
//struct _SDL_AudioStream
//{
//    SDL_AudioCVT cvt_before_resampling;
//    SDL_AudioCVT cvt_after_resampling;
//    SDL_DataQueue *queue;
//    SDL_bool first_run;
//    Uint8 *staging_buffer;
//    int staging_buffer_size;
//    int staging_buffer_filled;
//    Uint8 *work_buffer_base;  /* maybe unaligned pointer from SDL_realloc(). */
//int work_buffer_len;
//int src_sample_frame_size;
//SDL_AudioFormat src_format;
//Uint8 src_channels;
//int src_rate;
//int dst_sample_frame_size;
//SDL_AudioFormat dst_format;
//Uint8 dst_channels;
//int dst_rate;
//double rate_incr;
//Uint8 pre_resample_channels;
//int packetlen;
//int resampler_padding_samples;
//float* resampler_padding;
//void* resampler_state;
//SDL_ResampleAudioStreamFunc resampler_func;
//SDL_ResetAudioStreamResamplerFunc reset_resampler_func;
//SDL_CleanupAudioStreamResamplerFunc cleanup_resampler_func;
//};
//
//


struct GDL_Stream
{
	int len;
	
};

//insert audio 
int GDL_AudioStreamPut(GDL_Stream* stream, unsigned char* buff, int len); //SDL_QueueAudio(deviceid, buff, len);
int GDL_GetError();
int SDL_AudioStreamAvailable(GDL_Stream* stream);
//tell buffer to add audio

int SDL_AudioStreamGet(GDL_Stream* stream, unsigned char* buffer, int len);;