//
//  testC.c
//  mytest
//
//  Created by lichao on 2019/12/30.
//  Copyright © 2019年 lichao. All rights reserved.
//

#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>


//这里必须是const char *, 而不能是 char*
int test(const char* path){
    
    int err_code = 0;
    char errors[1024];
    
    AVFormatContext *fmt_ctx = NULL;
    
    //char *src_filename = "/Users/lichao/Documents/killer.mp4";
    const char* src_filename = path;
    
    //av_register_all();
    
    /* open input file, and allocate format context */
    if ((err_code=avformat_open_input(&fmt_ctx, src_filename, NULL, NULL)) < 0) {
        av_strerror(err_code, errors, 1024);
        fprintf(stderr, "Could not open source file[%s], %d(%s)\n", src_filename, err_code, errors);
        goto __ERROR;
    }
    
    /* retreive stream information */
    if((err_code = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_strerror(err_code, errors, 1024);
        fprintf(stderr, "Could not open source file [%s], %d(%s)\n", src_filename, err_code, errors);
        goto __CLOSE_INPUT;
    }
    
    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, src_filename, 0);

__CLOSE_INPUT:
    /* close input file */
    avformat_close_input(&fmt_ctx);
    
__ERROR:
    return err_code;
}

static int get_format_from_sample_fmt(const char **fmt,
                                      enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;
    
    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }
    
    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}


static void decode(AVCodecContext *dec_ctx,
                   AVPacket *pkt,
                   AVFrame *frame,
                   FILE *outfile)
{
    int i, ch;
    int ret, data_size;
    
    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting the packet to the decoder\n");
        exit(1);
    }
    
    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            return;
        }else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }
        
        data_size = av_get_bytes_per_sample(dec_ctx->sample_fmt);
        if (data_size < 0) {
            /* This should not occur, checking just for paranoia */
            fprintf(stderr, "Failed to calculate data size\n");
            exit(1);
        }
        
        for (i = 0; i < frame->nb_samples; i++){
            for (ch = 0; ch < dec_ctx->channels; ch++){
                fwrite(frame->data[ch] + data_size*i, 1, data_size, outfile);
            }
        }
    }
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx,
                              AVFormatContext *fmt_ctx,
                              enum AVMediaType type)
{
    int ret = 0, stream_index = 0;
    AVStream *st = NULL;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;
    
    //find stream index
    for(int i=0; i < fmt_ctx->nb_streams; i++) {
        if( fmt_ctx->streams[i]->codecpar->codec_type == type ) {
            stream_index = i;
            break;
        }
    }
    
    if(stream_index == -1)
    {
        printf("Couldn't find a video stream.\n");
        return -1;
    }

    st = fmt_ctx->streams[stream_index];
    
    /* find decoder for the stream */
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) {
        fprintf(stderr, "Failed to find %s codec, codec_id=%08x\n",
                av_get_media_type_string(type), st->codecpar->codec_id);
        return AVERROR(EINVAL);
    }
    
    
    /* Allocate a codec context for the decoder */
    *dec_ctx = avcodec_alloc_context3(dec);
    if (!*dec_ctx) {
        fprintf(stderr, "Failed to allocate the %s codec context\n",
                av_get_media_type_string(type));
        return AVERROR(ENOMEM);
    }
    
    /* Copy codec parameters from input stream to output codec context */
    if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(type));
        return ret;
    }
    
    /* Init the decoders, with or without reference counting */
    if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(type));
        return ret;
    }
    
    *stream_idx = stream_index;

    return 0;
}

int deviceInfo(){
    
    int ret = 0;
    char errors[1024];
    
    AVDictionary* options = NULL;
    
    int type  = AVMEDIA_TYPE_AUDIO;
    int stream_index = -1;
    AVFormatContext *fmt_ctx = NULL;
    
    AVFrame *decoded_frame = NULL;
    AVCodecContext * codec_ctx = NULL;
    
    int n_channels = 0;
    enum AVSampleFormat sfmt;
    const char *fmt;
    
    FILE* outfile = NULL;
    const char* outfilename = "/Users/lichao/Downloads/av_base/mytest/mytest/1.pcm";
    //const char* infilename = "/Users/lichao/Documents/killer.mp4";
    const char* infilename = ":0";
    
    int count = 0;
    AVPacket avpkt;
    av_init_packet(&avpkt);
    
    //register audio/video device
    avdevice_register_all();

    //
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    
//    av_dict_set(&options,"list_devices","true",0);
//    int ret = avformat_open_input(&pFormatCtx, "dummy", iformat, &options);
//    if(ret < 0) {
//        av_strerror(ret, errors, 1024);
//        fprintf(stderr, "Failed to open device, [%d]%s\n", ret, errors);
//    }else{
//        fprintf(stdout, "success to open device===\n");
//    }
//    av_dict_free(&options);
    
    //set audio parameters 这段代码无用
//    av_dict_set(&options, "ar", "48000", 0); //这里不能使用 r 参数
//    av_dict_set(&options, "ac","1", 0);
//    av_dict_set(&options, "sample_fmt","pcm_s16le", 0);
    
    //Avfoundation [video]:[audio]
    //if((ret = avformat_open_input(&fmt_ctx, infilename, NULL, &options)) < 0){
    if((ret = avformat_open_input(&fmt_ctx, infilename, iformat, &options)) < 0){
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open device, [%d]%s\n", ret, errors);
        goto __ERROR;
    }
    
    //find stream
    if((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_strerror(ret, errors, 1024);
        fprintf(stderr, "Failed to open device, [%d]%s\n", ret, errors);
        goto __RELEASE;
    }
    
    av_dump_format(fmt_ctx, 0, infilename, 0);
    
    //create codec context
    open_codec_context(&stream_index, &codec_ctx, fmt_ctx, type);
    
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open out file[%s]\n", outfilename);
        goto __RELEASE;
    }
    
    //alloc frame
    if (!decoded_frame) {
        if (!(decoded_frame = av_frame_alloc())) {
            fprintf(stderr, "Could not allocate audio frame\n");
            goto __RELEASE;
        }
    }
    
   // initialize packet, set data to NULL, let the demuxer fill it 
    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;

    // read frames from the file 
    while (av_read_frame(fmt_ctx, &avpkt) >= 0 && count++ < 1000) {
        fwrite(avpkt.data, 1, avpkt.size, outfile);
        fflush(outfile);
        decode(codec_ctx, &avpkt, decoded_frame, outfile);
    }
    
    //flush cached frames 
    avpkt.data = NULL;
    avpkt.size = 0;
    decode(codec_ctx, &avpkt, decoded_frame, outfile);

    // print output pcm infomations, because there have no metadata of pcm 
//    sfmt = iformat->//iformatsample_fmt;
//
//    if (av_sample_fmt_is_planar(sfmt)) {
//        const char *packed = av_get_sample_fmt_name(sfmt);
//        printf("Warning: the sample format the decoder produced is planar "
//               "(%s). This example will output the first channel only.\n",
//               packed ? packed : "?");
//        sfmt = av_get_packed_sample_fmt(sfmt);
//    }
//
//    n_channels = codec_ctx->channels;
//    if ((ret = get_format_from_sample_fmt(&fmt, sfmt)) < 0){
//        goto __RELEASE;
//    }
//    printf("Play the output audio file with the command:\n"
//           "ffplay -f %s -ac %d -ar %d %s\n",
//           fmt, n_channels, codec_ctx->sample_rate,
//           outfilename);
    
    
    fprintf(stdout, "successed to record audio");
__RELEASE:
    if(fmt_ctx){
        avformat_close_input(&fmt_ctx);
    }
    
    if(codec_ctx){
        avcodec_free_context(&codec_ctx);
    }
    
    if(decoded_frame) {
     av_frame_free(&decoded_frame);
    }
    
    av_packet_unref(&avpkt);
    
    if(outfile){
        fclose(outfile);
    }
    
__ERROR:
    return ret;
}

#if  1
int main(int argc, char* argv[]){
    
    return deviceInfo();
}
#endif
