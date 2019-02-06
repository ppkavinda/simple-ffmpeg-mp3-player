#include <iostream>
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
}

#define INPUT_FILE "sample.mp3"
#define MAX_AUDIO_FRAME_SIZE 192000

#define OUTPUT_PCM 1
#define USE_SDL 1

using namespace std;

static Uint8 *audio_chunk;
static Uint32 audio_len;
static Uint8 *audio_pos;

void fill_audio(void *udata, Uint8 *stream, int len)
{
    //SDL 2.0
    SDL_memset(stream, 0, len);
    if (audio_len == 0)
        return;

    len = (len > audio_len ? audio_len : len); /*  Mix  as  much  data  as  possible  */

    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    audio_len -= len;

}

int main() {
    AVFormatContext *pFormatCtx;
    int i, audioStream;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *packet;
    uint8_t *out_buffer;
    AVFrame *pFrame;
    SDL_AudioSpec wanted_spec;
    int ret;
    uint32_t len = 0;
    int got_picture;
    int index = 0;
    int64_t in_channel_layout;
    struct SwrContext *au_convert_ctx;

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, INPUT_FILE, nullptr, nullptr) != 0) {
        cout << "Couldn't open input stream." << endl;
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        cout << "Couldn't find stream information." << endl;
        return -1;
    }

//    av_dump_format(pFormatCtx, 0, INPUT_FILE, false);

    audioStream = -1;

    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            break;
        }
    }

    if (audioStream == -1) {
        cout << "Didn't find a audio stream." << endl;
        return -1;
    }

    pCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audioStream]->codecpar);

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == nullptr) {
        cout << "Unsupported codec." << endl;
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        cout << "Couldn't open codec." << endl;
        return -1;
    }

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    av_init_packet(packet);

    uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
    int out_nb_samples = pCodecCtx->frame_size;
    cout << "nb samples " << pCodecCtx->frame_size << endl;
    AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = 44100;
    int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    int out_buffer_size = av_samples_get_buffer_size(
            nullptr, out_channels, out_nb_samples,
            out_sample_fmt, 1);
    out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
    pFrame = av_frame_alloc();

    cout << "out channels:" << out_channels << "out nb samples:"
    << out_nb_samples << "out sample fmt: " << out_sample_fmt << endl;


#if USE_SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        cout << "Couldn't initialize SDL " << SDL_GetError() << endl;
        return -1;
    }

    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = out_channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = out_nb_samples;
    wanted_spec.callback = fill_audio;
    wanted_spec.userdata = pCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, nullptr) < 0) {
        cout << "Can't open audio." << endl;
        return -1;
    }
#endif

    in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);

    au_convert_ctx = swr_alloc();
    au_convert_ctx = swr_alloc_set_opts(
            au_convert_ctx, out_channel_layout, out_sample_fmt,
            out_sample_rate, in_channel_layout, pCodecCtx->sample_fmt,
            pCodecCtx->sample_rate, 0, nullptr);

    swr_init(au_convert_ctx);

    SDL_PauseAudio(0);

    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audioStream) {
            ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0) {
                cout << "Error in decoding audio frame." << endl;
                return -1;
            }
            if (got_picture > 0) {
                swr_convert(
                        au_convert_ctx, &out_buffer,
                        MAX_AUDIO_FRAME_SIZE,
                        (const uint8_t **)pFrame->data,
                        pFrame->nb_samples);
//                cout << "Index: " << index << " Pts: " << packet->pts << " Packet size : " << packet->size << endl;
//                return -1;

                index++;
            }

#if USE_SDL
            cout << "audio_len" << out_buffer_size << endl;

            while (audio_len > 0) {
                SDL_Delay(1);
            }

            audio_chunk = (Uint8 *) out_buffer;
            audio_len = out_buffer_size;
            audio_pos = audio_chunk;
#endif
        }
        av_free_packet(packet);
    }
    swr_free(&au_convert_ctx);

#if USE_SDL
    SDL_CloseAudio();
    SDL_Quit();
#endif
    av_free(out_buffer);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}
