//
//  api.c
//  Imitator
//
//  Created by Li Sheng on 2020/6/29.
//  Copyright © 2020 lilikazine. All rights reserved.
//

#include "api.h"

#define INBUF_SIZE 4096

void dump(AVFormatContext* fmt_ctx,
          int index,
          const char* url,
          int is_output) {
    int ret = 0;
    if (!fmt_ctx) {
        if ((ret = avformat_open_input(&fmt_ctx, url, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
            return;
        }
        ret = 1;
    }
    av_dump_format(fmt_ctx, index, url, is_output);
    if (ret > 0) {
        avformat_close_input(&fmt_ctx);
    }
}

void media_legth(AVFormatContext* fmt_ctx,
                 const char* url,
                 struct AudioInfo** audio_info,
                 struct VideoInfo** video_info) {
    int ret = 0;
    if (!fmt_ctx) {
        if ((ret = avformat_open_input(&fmt_ctx, url, NULL, NULL)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "%s", av_err2str(ret));
            return;
        }
        ret = 1;
    }
    for (int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            *audio_info = (struct AudioInfo*)malloc(sizeof(struct AudioInfo));
            (*audio_info)->metadata = stream->metadata;
        }
        
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            *video_info = (struct VideoInfo*)malloc(sizeof(struct VideoInfo));
            (*video_info)->duration = stream->duration * av_q2d(stream->time_base);
            (*video_info)->metadata = stream->metadata;
        }
    }
    if (ret > 0) {
        avformat_close_input(&fmt_ctx);
    }
}

void dump_filters() {
    const AVBitStreamFilter* bsf = NULL;
    void* state = NULL;
    while ((bsf = av_bsf_iterate(&state))) {
        av_log(NULL, AV_LOG_INFO, "%s\n", bsf->name);
    }
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename) {
    FILE *f;
    int i;
    f = fopen(filename, "w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i<ysize; i++) {
        fwrite(buf + i * wrap, 1, xsize, f);
    }
    fclose(f);
}

static void png_save(AVCodecContext* decoder_ctx, AVFrame* frame, const char* filename) {
    int ret = 0;
    AVCodec* encoder = NULL;
    AVCodecContext* encoder_ctx = NULL;
    AVPacket *pkt = NULL;
    encoder = avcodec_find_encoder(AV_CODEC_ID_JPEG2000);
    if (!encoder) {
        av_log(NULL, AV_LOG_ERROR, "Can't find encoder for jpg.\n");
        goto fail;
    }
    encoder_ctx = avcodec_alloc_context3(encoder);
    if (!encoder_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc encoder context.\n");
        goto fail;
    }
    
    encoder_ctx->pix_fmt = decoder_ctx->pix_fmt;
    encoder_ctx->height = frame->height;
    encoder_ctx->width = frame->width;
    encoder_ctx->time_base = decoder_ctx->time_base;
    
    pkt = av_packet_alloc();
    if (!pkt) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc packet.\n");
        goto fail;
    }
    ret = avcodec_open2(encoder_ctx, encoder, NULL);
    if (ret < 0) {
        goto fail;
    }
    ret = avcodec_send_frame(encoder_ctx, frame);
    if (ret < 0) {
        goto fail;
    }
    FILE* f;
    f = fopen(filename, "wb");
    if (!f) {
        av_log(NULL, AV_LOG_ERROR, "Can't open file.\n");
        goto fail;
    }
    int count = 0;
    while ((ret = avcodec_receive_packet(encoder_ctx, pkt)) >= 0) {
        printf("%d\n",count);
        count++;
        fwrite(pkt->data, 1, pkt->size, f);
    }
    fclose(f);
    
fail:
    if (pkt) {
        av_packet_free(&pkt);
    }
    if (encoder_ctx) {
        avcodec_close(encoder_ctx);
    }
    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
}

static void decode(AVCodecContext *codec_ctx, AVFrame* frame, AVPacket* pkt, const char* dir) {
    char buf[1024];
    int ret = 0;
    
    ret = avcodec_send_packet(codec_ctx, pkt);
    if (ret < 0) {
        goto fail;
    }
    
    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            break;
        }
        
        if (frame->key_frame != 1) {
            av_log(NULL, AV_LOG_INFO, "not key frame\n");
            continue;
        }
        
        printf("saving frame %3d\n", codec_ctx->frame_number);
        fflush(stdout);
        snprintf(buf, sizeof(buf), "%s/%s-%d.jpg", dir, "filename", codec_ctx->frame_number);
//        pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
        png_save(codec_ctx, frame, buf);
        
    }
    
fail:
    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
}

void frame_2_pic(const char* src, const char* dir) {
    
    int ret = 0;
    
//    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
//
//    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    
    AVFormatContext* fmt_ctx = NULL;
//    enum AVCodecID codecID;
    
    AVCodec* codec = NULL;
    
//    AVCodecParserContext* parser_ctx = NULL;
    
    AVCodecContext* codec_ctx = NULL;
    
    AVFrame* frame = NULL;
    
    int istream = 0;
    
    ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    istream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    
//    codecID = fmt_ctx->video_codec_id;
    
//    codec = avcodec_find_decoder(codecID);
    if (!codec) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder for video.\n");
        goto fail;
    }
//    parser_ctx = av_parser_init(codecID);
//    if (!parser_ctx) {
//        av_log(NULL, AV_LOG_ERROR, "Can't find parser for decoder.\n");
//        goto fail;
//    }
    
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc context for decoder.\n");
        goto fail;
    }

    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    frame = av_frame_alloc();
    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc video frame.\n");
        goto fail;
    }
    
    AVPacket pkt;
    
    while((ret = av_read_frame(fmt_ctx, &pkt)) == 0) {
//        AVPacket* opkt = av_packet_alloc();
//        if (!opkt) {
//            av_log(NULL, AV_LOG_ERROR, "Can't alloc output packet.\n");
//            goto fail;
//        }
        
//        ret = av_parser_parse2(parser_ctx, codec_ctx, &opkt->data, &opkt->size, pkt.data, pkt.size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
//
//        if (ret < 0) {
//            av_packet_free(&opkt);
//            goto fail;
//        }
//        decode(codec_ctx, frame, opkt);
        
//        av_packet_free(&opkt);
        if (pkt.stream_index != istream ) {
            av_packet_unref(&pkt);
            continue;
        }
        
        decode(codec_ctx, frame, &pkt, dir);
        av_packet_unref(&pkt);
    }
    
fail:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    
    if (codec_ctx) {
        avcodec_free_context(&codec_ctx);
    }
    
//    if (parser_ctx) {
//        av_parser_close(parser_ctx);
//    }
//
    if (frame) {
        av_frame_free(&frame);
    }
}

uint8_t* h264_2_data(uint8_t** data, int** length, const char* src) {
    int ret = 0;
    AVFormatContext* fmt_ctx = NULL;
    AVCodec* decoder = NULL;
    AVCodecContext* decoder_ctx = NULL;
  
    AVFrame* frame = NULL;
    AVPacket pkt;
    
    int istream = 0;
    
    ret = avformat_open_input(&fmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        goto fail;
    }
    istream = ret;
    
    if (!decoder) {
        av_log(NULL, AV_LOG_ERROR, "Can't find decoder for video.\n");
        goto fail;
    }
    
    decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc context for decoder.\n");
        goto fail;
    }
    
    ret = avcodec_open2(decoder_ctx, decoder, NULL);
    if (ret < 0) {
        goto fail;
    }

    frame = av_frame_alloc();
    if (!frame) {
        av_log(NULL, AV_LOG_ERROR, "Can't alloc video frame.\n");
        goto fail;
    }
    
    while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0) {
        if (pkt.stream_index != istream) {
            av_packet_unref(&pkt);
            continue;
        }
        
        ret = avcodec_send_packet(decoder_ctx, &pkt);
        if (ret < 0) {
            break;
        }
        while (ret >= 0) {
            ret = avcodec_receive_frame(decoder_ctx, frame);
            if (ret < 0) {
                break;
            }
            if (frame->key_frame != 1) {
                continue;
            }
            printf("saving frame %3d\n", decoder_ctx->frame_number);
            fflush(stdout);
            return frame->data;
            
        }
        av_packet_unref(&pkt);
    }
    
fail:
    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }
    if (decoder_ctx) {
        avcodec_close(decoder_ctx);
        avcodec_free_context(&decoder_ctx);
    }
  
    if (frame) {
        av_frame_free(&frame);
    }
   
    return NULL;
}

void extract_video(const char* src,
                   const char* dst,
                   int64_t begin,
                   int64_t end) {
    int ret = 0;
    
    AVFormatContext* ifmt_ctx = NULL;
    AVFormatContext* ofmt_ctx = NULL;
    AVBSFContext* bsf_ctx = NULL;
    AVPacket pkt;
    
    ret = avformat_open_input(&ifmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, dst);
    if (ret < 0) {
        goto fail;
    }
    int istream = 0;
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* stream = ifmt_ctx->streams[i];
        if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        istream = i;
        AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        AVStream* new_stream = avformat_new_stream(ofmt_ctx, decoder);
        ret = avcodec_parameters_copy(new_stream->codecpar, stream->codecpar);
        if (ret < 0) {
            goto fail;
        }
        new_stream->codecpar->codec_tag = 0;
    }
    
    ret = avio_open(&ofmt_ctx->pb, dst, AVIO_FLAG_WRITE);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    const AVBitStreamFilter* bsf_filter = av_bsf_get_by_name("h264_mp4toannexb");
    if (!bsf_filter) {
        goto fail;
    }
    
    ret = av_bsf_alloc(bsf_filter, &bsf_ctx);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[istream]->codecpar);
    if (ret < 0) {
        goto fail;
    }
    
    bsf_ctx->time_base_in = ifmt_ctx->streams[istream]->time_base;
    
    ret = av_bsf_init(bsf_ctx);
    if (ret < 0) {
        goto fail;
    }
    
    int64_t begin_time = begin / av_q2d(ifmt_ctx->streams[istream]->time_base);

    ret = avformat_seek_file(ifmt_ctx, istream, INT64_MIN, begin_time, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        goto fail;
    }
    
    int64_t dts_start = -1;
    int64_t pts_start = -1;
    
    while ((ret = av_read_frame(ifmt_ctx, &pkt)) == 0) {
        if (pkt.stream_index != istream) {
            continue;
        }
        
        if (pkt.pts * av_q2d(ifmt_ctx->streams[istream]->time_base) > end) {
            av_packet_unref(&pkt);
            break;
        }
        
        ret = av_bsf_send_packet(bsf_ctx, &pkt);
        
        if (ret < 0) {
            goto fail;
        }
        
        while ((ret = av_bsf_receive_packet(bsf_ctx, &pkt)) == 0) {
            
            if (dts_start < 0) {
                dts_start = pkt.dts;
            }
            if (pts_start < 0) {
                pts_start = pkt.pts;
            }
            
            pkt.pts = av_rescale_q_rnd(pkt.pts-pts_start, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            pkt.dts = av_rescale_q_rnd(pkt.dts-dts_start, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
            if (pkt.pts < 0) {
                pkt.pts = 0;
            }
            
            if (pkt.dts < 0) {
                pkt.dts = 0;
            }
            
            if (pkt.pts < pkt.dts) {
                pkt.pts = pkt.dts;
            }
            
            pkt.duration = av_rescale_q(pkt.duration, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base);
            pkt.pos = -1;
            
            ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
            
            av_packet_unref(&pkt);
            
        }
        
        if (ret == AVERROR(EAGAIN)) {
            av_packet_unref(&pkt);
            continue;
        }
        
        if (ret == AVERROR_EOF) {
            break;
        }
        
        if (ret < 0) {
            goto fail;
        }
        
    }
    
    ret = av_bsf_send_packet(bsf_ctx, NULL);
    while ((ret = av_bsf_receive_packet(bsf_ctx, &pkt)) == 0) {
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (dts_start < 0) {
            dts_start = pkt.dts;
        }
        if (pts_start < 0) {
            pts_start = pkt.pts;
        }
        
        pkt.pts = av_rescale_q_rnd(pkt.pts-pts_start, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts-dts_start, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        if (pkt.pts < 0) {
            pkt.pts = 0;
        }
        
        if (pkt.dts < 0) {
            pkt.dts = 0;
        }
        
        if (pkt.pts < pkt.dts) {
            pkt.pts = pkt.dts;
        }
        
        pkt.duration = av_rescale_q(pkt.duration, ifmt_ctx->streams[istream]->time_base, ofmt_ctx->streams[istream]->time_base);
        pkt.pos = -1;
        av_packet_unref(&pkt);
    }
    
    av_write_trailer(ofmt_ctx);
    
fail:
    av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    
    av_packet_unref(&pkt);
    
    if (bsf_ctx) {
        av_bsf_free(&bsf_ctx);
    }
    
    if (ifmt_ctx) {
        avformat_close_input(&ifmt_ctx);
    }
    
    if (ofmt_ctx) {
        avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        
    }
}

void cut_video(const char* src,
               const char* dst,
               int64_t begin,
               int64_t end,
               AVCodec* encoder) {
    int ret = 0;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    
    ret = avformat_open_input(&ifmt_ctx, src, NULL, NULL);
    if (ret < 0) {
        goto fail;
    }
    ret = avformat_find_stream_info(ifmt_ctx, NULL);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, dst);
    if (ret < 0) {
        goto fail;
    }
    
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream* istream = ifmt_ctx->streams[i];
        AVCodec* decoder = avcodec_find_decoder(istream->codecpar->codec_id);
        AVStream* ostream = avformat_new_stream(ofmt_ctx, decoder);
        avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
        ostream->codecpar->codec_tag = 0;
        
    }
    
    int i_vstream = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
//
//    AVStream* istream = ifmt_ctx->streams[i_vstream];
//    AVStream* ostream = avformat_new_stream(ofmt_ctx, decoder);
//
//    avcodec_parameters_copy(ostream->codecpar, istream->codecpar);
//
    av_dump_format(ofmt_ctx, 0, dst, 1);

    
    ret = avio_open(&ofmt_ctx->pb, dst, AVIO_FLAG_WRITE);
    if (ret < 0) {
        goto fail;
    }
    
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        goto fail;
    }
    int64_t begin_time = begin / av_q2d(ifmt_ctx->streams[i_vstream]->time_base);
    
    av_log(NULL, AV_LOG_INFO, "begin time: %lld", begin_time);
    
    ret = avformat_seek_file(ifmt_ctx, i_vstream, INT64_MIN, begin_time, INT64_MAX, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        goto fail;
    }
    
    int64_t *dts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
    memset(dts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
    int64_t *pts_start_from = malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
    memset(pts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
    
    AVPacket pkt;
    while (1) {
        AVStream *istream, *ostream;
        
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) {
            goto fail;
        }
        istream = ifmt_ctx->streams[pkt.stream_index];
        ostream = ofmt_ctx->streams[pkt.stream_index];
        
        if (pkt.pts * av_q2d(istream->time_base) > end) {
            av_packet_unref(&pkt);
            break;
        }
        
        // recode
//        enum AVMediaType itype = istream->codecpar->codec_type;
//        if (itype == AVMEDIA_TYPE_AUDIO ||
//            itype == AVMEDIA_TYPE_VIDEO ||
//            itype == AVMEDIA_TYPE_SUBTITLE) {
//            
//            
//            
//        } else {
//            av_packet_unref(&pkt);
//            continue;
//        }
        
        if (dts_start_from[pkt.stream_index] == 0) {
            dts_start_from[pkt.stream_index] = pkt.dts;
            printf("dts_start_from: %s\n", av_ts2str(dts_start_from[pkt.stream_index]));
        }
        if (pts_start_from[pkt.stream_index] == 0) {
            pts_start_from[pkt.stream_index] = pkt.pts;
            printf("pts_start_from: %s\n", av_ts2str(pts_start_from[pkt.stream_index]));
        }
        
        pkt.pts = av_rescale_q_rnd(pkt.pts - pts_start_from[pkt.stream_index], istream->time_base, ostream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts - dts_start_from[pkt.stream_index], istream->time_base, ostream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        if (pkt.pts < 0) {
            pkt.pts = 0;
        }
        if (pkt.dts < 0) {
            pkt.dts = 0;
        }
        if (pkt.pts < pkt.dts) {
            pkt.pts = pkt.dts;
        }
        pkt.duration = av_rescale_q(pkt.duration, istream->time_base, ostream->time_base);
        pkt.pos = -1;
        
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        av_packet_unref(&pkt);
    }
    
    av_write_trailer(ofmt_ctx);
    
fail:
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
    }
    if (ifmt_ctx) {
        avformat_close_input(&ifmt_ctx);
    }
    
    if (ofmt_ctx) {
        avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        
    }
    
}
