//
//  ffmpeg.c
//  MacRecorder
//
//  Created by Li Sheng on 2020/6/9.
//  Copyright © 2020 LiLi Kazine. All rights reserved.
//

#include "ffmpeg.h"


#define ADTS_HEADER_LEN  7;

static int get_audio_obj_type(int aactype) {
    //AAC HE V2 = AAC LC + SBR + PS
    //AAV HE = AAC LC + SBR
    //所以无论是 AAC_HEv2 还是 AAC_HE 都是 AAC_LC
    switch(aactype){
        case 0:
        case 2:
        case 3:
            return aactype+1;
        case 1:
        case 4:
        case 28:
            return 2;
        default:
            return 2;
            
    }
}

static int get_sample_rate_index(int freq, int aactype) {
    
    int i = 0;
    int freq_arr[13] = {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    
    //如果是 AAC HEv2 或 AAC HE, 则频率减半
    if(aactype == 28 || aactype == 4){
        freq /= 2;
    }
    
    for(i=0; i< 13; i++){
        if(freq == freq_arr[i]){
            return i;
        }
    }
    return 4;//默认是44100
}

static int get_channel_config(int channels, int aactype) {
    //如果是 AAC HEv2 通道数减半
    if(aactype == 28){
        return (channels / 2);
    }
    return channels;
}

static void adts_header(char *szAdtsHeader, int dataLen, int aactype, int frequency, int channels) {
    
    int audio_object_type = get_audio_obj_type(aactype);
    int sampling_frequency_index = get_sample_rate_index(frequency, aactype);
    int channel_config = get_channel_config(channels, aactype);
    
    //    printf("aot=%d, freq_index=%d, channel=%d\n", audio_object_type, sampling_frequency_index, channel_config);
    
    int adtsLen = dataLen + 7;
    
    szAdtsHeader[0] = 0xff;         //syncword:0xfff                          高8bits
    szAdtsHeader[1] = 0xf0;         //syncword:0xfff                          低4bits
    szAdtsHeader[1] |= (0 << 3);    //MPEG Version:0 for MPEG-4,1 for MPEG-2  1bit
    szAdtsHeader[1] |= (0 << 1);    //Layer:0                                 2bits
    szAdtsHeader[1] |= 1;           //protection absent:1                     1bit
    
    szAdtsHeader[2] = (audio_object_type - 1)<<6;            //profile:audio_object_type - 1                      2bits
    szAdtsHeader[2] |= (sampling_frequency_index & 0x0f)<<2; //sampling frequency index:sampling_frequency_index  4bits
    szAdtsHeader[2] |= (0 << 1);                             //private bit:0                                      1bit
    szAdtsHeader[2] |= (channel_config & 0x04)>>2;           //channel configuration:channel_config               高1bit
    
    szAdtsHeader[3] = (channel_config & 0x03)<<6;     //channel configuration:channel_config      低2bits
    szAdtsHeader[3] |= (0 << 5);                      //original：0                               1bit
    szAdtsHeader[3] |= (0 << 4);                      //home：0                                   1bit
    szAdtsHeader[3] |= (0 << 3);                      //copyright id bit：0                       1bit
    szAdtsHeader[3] |= (0 << 2);                      //copyright id start：0                     1bit
    szAdtsHeader[3] |= ((adtsLen & 0x1800) >> 11);           //frame length：value   高2bits
    
    szAdtsHeader[4] = (uint8_t)((adtsLen & 0x7f8) >> 3);     //frame length:value    中间8bits
    szAdtsHeader[5] = (uint8_t)((adtsLen & 0x7) << 5);       //frame length:value    低3bits
    szAdtsHeader[5] |= 0x1f;                                 //buffer fullness:0x7ff 高5bits
    szAdtsHeader[6] = 0xfc;
}

void set_log_level(int level) {
    av_log_set_level(level);
}

void output(int level, const char *fmt) {
    av_log(NULL, level, "%s", fmt);
}

int delete_file(const char *url) {
    int ret = avpriv_io_delete(url);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

int move_file(const char *src, const char *dst) {
    int ret = avpriv_io_move(src, dst);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        return -1;
    }
    return 0;
}

void read_dir(const char *url) {
    AVIODirContext *ctx = NULL;
    AVIODirEntry *entry = NULL;
    int ret = 0;
    
    ret = avio_open_dir(&ctx, url, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open dir: %s\n", av_err2str(ret));
        return;
    }
    
    while (1) {
        ret = avio_read_dir(ctx, &entry);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot read dir: %s\n", av_err2str(ret));
            break;
        }
        
        if (!entry) {
            break;
        }
        av_log(NULL, AV_LOG_INFO, "%12"PRId64" %s \n", entry->size, entry->name);
        avio_free_directory_entry(&entry);
    }
    
    avio_close_dir(&ctx);
}

void dump_meta(const char *url) {
    //    av_register_all();  /* deprecated */
    int ret = 0;
    AVFormatContext *ctx = NULL;
    ret = avformat_open_input(&ctx, url, NULL, NULL);
    if (!ctx || ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file: %s\n", av_err2str(ret));
        return;
    }
    av_dump_format(ctx, 0, url, 0);
    avformat_close_input(&ctx);
}

void extract_audio(const char *src, const char *dst) {
    int ret = 0;
    int audio_index;
    unsigned long len;
    AVFormatContext *ctx = NULL;
    AVPacket pkt;
    FILE *audio_file = NULL;
    ret = avformat_open_input(&ctx, src, NULL, NULL);
    if (!ctx || ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open file: %s\n", av_err2str(ret));
        return;
    }
    audio_file = fopen(dst, "wb");
    if (!audio_file) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open output file.");
        return;
    }
    ret = av_find_best_stream(ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find best stream: %s\n", av_err2str(ret));
        goto __ERROR;
    } else {
        audio_index = ret;
    }
    
    if (audio_index < 0) {
        av_log(NULL, AV_LOG_DEBUG, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_AUDIO),
               src);
        goto __ERROR;
    }
    
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    
    int aac_type = ctx->streams[1]->codecpar->profile;
    int channels = ctx->streams[1]->codecpar->channels;
    int sample_rate = ctx->streams[1]->codecpar->sample_rate;
    
    if (ctx->streams[1]->codecpar->codec_id != AV_CODEC_ID_AAC) {
        av_log(NULL, AV_LOG_ERROR, "the audio type is not AAC.\n");
        goto __ERROR;
    } else {
        av_log(NULL, AV_LOG_INFO, "the audio type is AAC!\n");
    }
    
    while (ret >= 0) {
        ret = av_read_frame(ctx, &pkt);
        if (pkt.stream_index == audio_index) {
            char adts_header_buf[7];
            adts_header(adts_header_buf, pkt.size, aac_type, sample_rate, channels);
            fwrite(adts_header_buf, 1, 7, audio_file);
            len = fwrite(pkt.data, 1, pkt.size, audio_file);
            if (len != pkt.size) {
                av_log(NULL, AV_LOG_WARNING, "Warning, length of data is not equal to the size of packet: %lu, %d", len, pkt.size);
            }
        }
        av_packet_unref(&pkt);
    }
__ERROR:
    
    avformat_close_input(&ctx);
    if (audio_file) {
        fclose(audio_file);
    }
}

#ifndef AV_WB32
#   define AV_WB32(p, val) do { \
uint32_t d = (val); \
((uint8_t*)(p))[3] = (d);   \
((uint8_t*)(p))[2] = (d)>>8;    \
((uint8_t*)(p))[1] = (d)>>16;   \
((uint8_t*)(p))[0] = (d)>>24;   \
} while(0)
#endif

#ifndef AV_RB16
#   define AV_RB16(x)   \
((((const uint8_t*)(x))[0] << 8) |   \
((const uint8_t*)(x))[1])
#endif

static int alloc_and_copy(AVPacket *out,
                          const uint8_t *sps_pps, uint32_t sps_pps_size,
                          const uint8_t *in, uint32_t in_size) {
    uint32_t offset = out->size;
    uint8_t nal_header_size = offset ? 3 : 4;
    int err;
    
    err = av_grow_packet(out, sps_pps_size + in_size + nal_header_size);
    if (err < 0) {
        return err;
    }
    if (sps_pps) {
        memcpy(out->data + offset, sps_pps, sps_pps_size);
    }
    memcpy(out->data + sps_pps_size + nal_header_size + offset, in, in_size);
    if (!offset) {
        AV_WB32(out->data + sps_pps_size, 1);
    } else {
        (out->data + offset + sps_pps_size)[0] =
        (out->data + offset + sps_pps_size)[1] = 0;
        (out->data + offset + sps_pps_size)[2] = 1;
    }
    return 0;
}

int h264_extradata_to_annexb(const uint8_t *codec_extradata,
                             const int codec_extradata_size,
                             AVPacket *out_extradata,
                             int padding) {
    uint16_t unit_size;
    uint64_t total_size = 0;
    uint8_t *out = NULL,
    unit_nb,
    sps_done = 0,
    sps_seen = 0,
    pps_seen = 0,
    sps_offset = 0,
    pps_offset = 0;
    const uint8_t *extradata = codec_extradata + 4;
    static const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
    int length_size = (*extradata++ & 0x3) + 1;
    
    sps_offset = pps_offset = -1;
    
    unit_nb = *extradata ++ & 0x1f;
    if (!unit_nb) {
        goto pps;
    } else {
        sps_offset = 0;
        sps_seen = 1;
    }
    
    while (unit_nb--) {
        int err;
        unit_size = AV_RB16(extradata);
        total_size += unit_size + 4;
        if (total_size > INT_MAX - padding) {
            av_log(NULL, AV_LOG_ERROR,
                   "Too big extradata size, corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if (extradata + 2 + unit_size > codec_extradata + codec_extradata_size) {
            av_log(NULL, AV_LOG_ERROR, "Packet header is not container in global extradata,"
                   "corrupted stream or invalid MP4/AVCC bitstream\n");
            av_free(out);
            return AVERROR(EINVAL);
        }
        if ((err = av_reallocp(&out, total_size + padding)) < 0) {
            return err;
        }
        memcpy(out + total_size - unit_size - 4, nalu_header, 4);
        memcpy(out + total_size - unit_size, extradata + 2, unit_size);
        extradata += 2 + unit_size;
    pps:
        if (!unit_nb && !sps_done++) {
            unit_nb = *extradata++;
            if (unit_nb) {
                pps_offset = total_size;
                pps_seen = 1;
            }
        }
    }
    
    if (out) {
        memset(out + total_size, 0, padding);
    }
    
    if (!sps_seen) {
        av_log(NULL, AV_LOG_WARNING,
               "Warning: SPS NALU missing or invalid, "
               "The resulting stream may not play.\n");
    }
    
    if (!pps_seen) {
        av_log(NULL, AV_LOG_WARNING,
               "Warning: PPS NALU missing or invalid. "
               "The resulting stream may not play.\n");
    }
    
    out_extradata->data = out;
    out_extradata->size = total_size;
    
    return length_size;
}

int h264_mp4toannexb(AVFormatContext *fmt_ctx, AVPacket *pkt, FILE *video_file) {
    
    AVPacket *out = NULL;
    AVPacket spspps_pkt;
    
    int len;
    uint8_t unit_type;
    int32_t nal_size;
    uint32_t cumul_size = 0;
    const uint8_t *buf;
    const uint8_t *buf_end;
    int buf_size;
    int ret = 0, i;
    
    out = av_packet_alloc();
    buf = pkt->data;
    buf_size = pkt->size;
    buf_end = pkt->data + pkt->size;
    
    do {
        ret = AVERROR(EINVAL);
        if (buf + 4 > buf_end) {
            goto fail;
        }
        for (nal_size = 0, i = 0; i < 4; i++) {
            nal_size = (nal_size << 8) | buf[i];
        }
        buf += 4;
        unit_type = *buf & 0x1f;
        if (nal_size > buf_end - buf || nal_size < 0) {
            goto fail;
        }
        if (unit_type == 5) {
            //关键帧 需要sps pps
            h264_extradata_to_annexb(fmt_ctx->streams[pkt->stream_index]->codecpar->extradata,
                                     fmt_ctx->streams[pkt->stream_index]->codecpar->extradata_size,
                                     &spspps_pkt,
                                     AV_INPUT_BUFFER_PADDING_SIZE);
            if ((ret = alloc_and_copy(out,
                                      spspps_pkt.data, spspps_pkt.size,
                                      buf, nal_size)) < 0) {
                goto fail;
            }
        } else {
            if ((ret = alloc_and_copy(out, NULL, 0, buf, nal_size)) < 0) {
                goto fail;
            }
        }
        len = fwrite(out->data, 1, out->size, video_file);
        if (len != out->size) {
            av_log(NULL, AV_LOG_DEBUG, "warning, length of writed data isn't equal pkt.size(%d, %d)\n",
                   len,
                   out->size);
        }
        fflush(video_file);
        
    next_nal:
        buf += nal_size;
        cumul_size += nal_size + 4; //s->length_size
    } while (cumul_size < buf_size);
fail:
    av_packet_free(&out);
    return ret;
}

void extract_video(const char *src, const char *dst) {

    int err_code = 0;
    char errors[1024];
    int video_stream_index = 0;
    AVFormatContext *fmt_ctx = NULL;
    AVPacket pkt;
    
    FILE *video_file = NULL;
    
    video_file = fopen(dst, "wb");
    if (!video_file) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create file %s\n", dst);
        return;
    }
    
    err_code = avformat_open_input(&fmt_ctx, src, NULL, NULL);
    if (err_code < 0) {
        av_strerror(err_code, errors, 1024);
        av_log(NULL, AV_LOG_ERROR, "Could not open souece file: %s, %d(%s)\n",
               src,
               err_code,
               errors);
        goto __ERROR;
    }
    
    /*
     err_code = avformat_find_stream_info(fmt_ctx, NULL);
     if (err_code < 0) {
     av_strerror(err_code, errors, 1024);
     av_log(NULL, AV_LOG_ERROR, "failed to find stream infomation: %s, %d(%s)\n",
     src,
     err_code,
     errors);
     goto __ERROR;
     }
     */
    av_dump_format(fmt_ctx, 0, src, 0);
    
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not find %s stream in input file %s\n",
               av_get_media_type_string(AVMEDIA_TYPE_VIDEO),
               src);
        goto __ERROR;
    }
    
    while (err_code >= 0) {
        err_code = av_read_frame(fmt_ctx, &pkt);
        if (pkt.stream_index == video_stream_index) {
            h264_mp4toannexb(fmt_ctx, &pkt, video_file);
        }
        av_packet_unref(&pkt);
    }
    
__ERROR:
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    if (video_file) {
        fclose(video_file);
    }
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}


void mp4_2_flv(const char *in_filename, const char *out_filename) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;
    
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s'", in_filename);
        goto end;
    }
    
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not retrieve input stream information");
        goto end;
    }
    
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    
    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }
    
    ofmt = ofmt_ctx->oformat;
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream,
        *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;
        
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            stream_mapping[i] = -1;
            continue;
        }
        
        stream_mapping[i] = stream_index++;
        
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        
        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        goto end;
    }
    while (1) {
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) {
            break;
        }
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }
        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        log_packet(ifmt_ctx, &pkt, "in");
        
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q_rnd(pkt.duration, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
        pkt.pos = -1;
        log_packet(ofmt_ctx, &pkt, "out");
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error muxing packet\n");
            break;
        }
        av_packet_unref(&pkt);
    }
    av_write_trailer(ofmt_ctx);
end:
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
    av_freep(&stream_mapping);
    
    if (ret < 0 && ret != AVERROR_EOF) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));
    }
}

void cut_video(double from_seconds,
              double end_seconds,
              const char *in_filename,
              const char *out_filename) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL,
    *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    
    ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0);
    
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s', %s",
               in_filename,
               av_err2str(ret));
        goto end;
    }
    
    ret = avformat_find_stream_info(ifmt_ctx, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to retrieve input stream information, %s", av_err2str(ret));
        goto end;
    }
    
    av_dump_format(ifmt_ctx, 0, in_filename, 0);
    
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx || ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context, %s\n", av_err2str(ret));
        goto end;
    }
    
    ofmt = ofmt_ctx->oformat;
    
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodec *in_codec = avcodec_find_decoder(in_stream->codecpar->codec_id);
        
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_codec);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate output stream.\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        
        ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy parameters from input to output, %s", av_err2str(ret));
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s', %s",
                   out_filename,
                   av_err2str(ret));
            goto end;
        }
    }
    
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file, %s\n", av_err2str(ret));
        goto end;
    }
    
    ret = av_seek_frame(ifmt_ctx, -1, from_seconds*AV_TIME_BASE, AVSEEK_FLAG_ANY);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error seek, %s\n", av_err2str(ret));
        goto end;
    }
    
    int64_t *dts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
    memset(dts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
    int64_t *pts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
    memset(pts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
    
    while (1) {
        AVStream *in_stream, *out_stream;
        
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "Read frame failed, %s\n", av_err2str(ret));
            break;
        }
        
        in_stream = ifmt_ctx->streams[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        
//        log_packet(ifmt_ctx, &pkt, "in");
        
        if (av_q2d(in_stream->time_base) * pkt.pts > end_seconds) {
            av_packet_unref(&pkt);
            break;
        }
        pkt.pts = av_rescale_q_rnd(pkt.pts - pts_start_from[pkt.stream_index], in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts - dts_start_from[pkt.stream_index], in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        if (pkt.pts < 0) {
            pkt.pts = 0;
        }
        if (pkt.dts < 0) {
            pkt.dts = 0;
        }
        pkt.duration = (int)av_rescale_q((int64_t)pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
//        log_packet(ofmt_ctx, &pkt, "out");
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Error muxing packet, %s\n", av_err2str(ret));
            break;
        }
        av_packet_unref(&pkt);
    }
    free(dts_start_from);
    free(pts_start_from);
    
    av_write_trailer(ofmt_ctx);
end:
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt_ctx->pb);
    }
    avformat_free_context(ofmt_ctx);
    
    if (ret < 0 && ret != AVERROR_EOF) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred, %s\n", av_err2str(ret));
    }
}


