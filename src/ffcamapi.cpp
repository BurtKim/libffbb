/* Copyright (c) 2012 Martin M Reed
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ffcamapi.h"

#include <fcntl.h>
#include <sys/stat.h>

void* encoding_thread(void* arg);

void ffcamera_init(ffcamera_context *ffc_context)
{
    ffc_context->fd = 0;
    ffc_context->width = 0;
    ffc_context->height = 0;
    ffc_context->codec_context = NULL;
    ffc_context->frame_count = 0;
    ffc_context->running = true;
    ffc_context->frames.clear();
    pthread_mutex_init(&ffc_context->reading_mutex, 0);
    pthread_cond_init(&ffc_context->read_cond, 0);
}

/**
 * codec_context MUST already be set
 */
ffcamera_error ffcamera_open(ffcamera_context *ffc_context)
{
    if (!ffc_context->codec_context) return FFCAMERA_NO_CODEC_SPECIFIED;
    return ffcamera_open(ffc_context, CODEC_ID_NONE);
}

/**
 * enum CodecID codec_id = CODEC_ID_MPEG2VIDEO;
 */
ffcamera_error ffcamera_open(ffcamera_context *ffc_context, enum CodecID codec_id)
{
    if (ffc_context->running) return FFCAMERA_ALREADY_RUNNING;

    int width = ffc_context->width;
    int height = ffc_context->height;
    if (width <= 0 || height <= 0) return FFCAMERA_INVALID_DIMENSIONS;

    if (ffc_context->fd <= 0) return FFCAMERA_INVALID_FILE_DESCRIPTOR;

    AVCodecContext *codec_context = ffc_context->codec_context;

    // codec id was not specified and a context has not been set yet
    if (codec_id == CODEC_ID_NONE && !codec_context) return FFCAMERA_NO_CODEC_SPECIFIED;

    // codec context is already set and matches the codec id
    if (codec_context && codec_context->codec_id == codec_id) return FFCAMERA_OK;

    if (codec_context)
    {
        avcodec_close(codec_context);
        av_free(codec_context);
        codec_context = ffc_context->codec_context = NULL;
    }

    AVCodec *codec = avcodec_find_encoder(codec_id);

    if (!codec)
    {
        av_register_all();
        codec = avcodec_find_encoder(codec_id);
        if (!codec) return FFCAMERA_CODEC_NOT_FOUND;
    }

    codec_context = avcodec_alloc_context3(codec);
    codec_context->pix_fmt = PIX_FMT_YUV420P;
    codec_context->width = width;
    codec_context->height = height;
    codec_context->bit_rate = 400000;
    codec_context->time_base.num = 1;
    codec_context->time_base.den = 30;
    codec_context->ticks_per_frame = 2;
    codec_context->gop_size = 15;
    codec_context->colorspace = AVCOL_SPC_SMPTE170M;
    codec_context->thread_count = 2;

    int codec_open = avcodec_open2(codec_context, codec, NULL);
    if (codec_open < 0) return FFCAMERA_COULD_NOT_OPEN_CODEC;

    ffc_context->codec_context = codec_context;

    return FFCAMERA_OK;
}

ffcamera_error ffcamera_close(ffcamera_context *ffc_context)
{
    AVCodecContext *codec_context = ffc_context->codec_context;

    if (codec_context)
    {
        avcodec_close(codec_context);
        av_free(codec_context);
        codec_context = ffc_context->codec_context = NULL;
    }

    pthread_mutex_destroy(&ffc_context->reading_mutex);
    pthread_cond_destroy(&ffc_context->read_cond);

    return FFCAMERA_OK;
}

ffcamera_error ffcamera_start(ffcamera_context *ffc_context)
{
    if (ffc_context->running) return FFCAMERA_ALREADY_RUNNING;
    if (!ffc_context->codec_context) return FFCAMERA_NO_CODEC_SPECIFIED;

    ffc_context->frame_count = 0;

    pthread_t pthread;
    pthread_create(&pthread, 0, &encoding_thread, ffc_context);

    ffc_context->running = true;

    return FFCAMERA_OK;
}

ffcamera_error ffcamera_stop(ffcamera_context *ffc_context)
{
    if (ffc_context->running) return FFCAMERA_ALREADY_STOPPED;

    ffc_context->running = false;
    pthread_cond_signal(&ffc_context->read_cond);

    return FFCAMERA_OK;
}

ssize_t write_all(int fd, uint8_t *buf, int size)
{
    ssize_t i = 0;
    do
    {
        ssize_t j = write(fd, buf + i, size - i);
        if (j < 0) return j;
        i += j;
    }
    while (i < size);
    return i;
}

void* encoding_thread(void* arg)
{
    ffcamera_context* ffc_context = (ffcamera_context*) arg;

    int fd = ffc_context->fd;
    AVCodecContext *codec_context = ffc_context->codec_context;
    std::deque<camera_buffer_t*> frames = ffc_context->frames;

    int encode_buffer_len = 10000000;
    uint8_t *encode_buffer = (uint8_t *) av_malloc(encode_buffer_len);

    AVFrame *frame = avcodec_alloc_frame();

    AVPacket packet;
    int got_packet;
    int success;

    while (ffc_context->running || !frames.empty())
    {
        if (frames.empty())
        {
            pthread_mutex_lock(&ffc_context->reading_mutex);
            pthread_cond_wait(&ffc_context->read_cond, &ffc_context->reading_mutex);
            pthread_mutex_unlock(&ffc_context->reading_mutex);
            continue;
        }

        camera_buffer_t *buf = frames.front();
        frames.pop_front();

        int frame_position = ffc_context->frame_count++;

        int64_t uv_offset = buf->framedesc.nv12.uv_offset;
        uint32_t height = buf->framedesc.nv12.height;
        uint32_t width = buf->framedesc.nv12.width;

        frame->pts = frame_position;

        frame->linesize[0] = width;
        frame->linesize[1] = width / 2;
        frame->linesize[2] = width / 2;

        frame->data[0] = buf->framebuf;
        frame->data[1] = &buf->framebuf[uv_offset];
        frame->data[2] = &buf->framebuf[uv_offset + ((width * height) / 4)];

        // reset the AVPacket
        av_init_packet(&packet);
        packet.data = encode_buffer;
        packet.size = encode_buffer_len;

        got_packet = 0;
        success = avcodec_encode_video2(codec_context, &packet, frame, &got_packet);

        if (success == 0 && got_packet > 0)
        {
            int size = write_all(fd, packet.data, packet.size);
            if (size == packet.size) printf("write frame %d (size=%5d)\n", frame_position, packet.size);
            else printf("FAILED write frame %d (size=%5d)\n", frame_position, packet.size);
        }
        else printf("skipped write frame: %d\n", frame_position);
        fflush(stdout);

        free(buf->framebuf);
        free(buf);
        buf = NULL;
    }

    av_free(frame);
    frame = NULL;

    do
    {
        int frame_position = ffc_context->frame_count++;

        // reset the AVPacket
        av_init_packet(&packet);
        packet.data = encode_buffer;
        packet.size = encode_buffer_len;

        got_packet = 0;
        success = avcodec_encode_video2(codec_context, &packet, NULL, &got_packet);

        if (success == 0 && got_packet > 0)
        {
            int size = write_all(fd, packet.data, packet.size);
            if (size == packet.size) printf("write (delayed) frame %d (size=%5d)\n", frame_position, packet.size);
            else printf("FAILED write (delayed) frame %d (size=%5d)\n", frame_position, packet.size);
            fflush(stdout);
        }
    }
    while (got_packet > 0);

    av_free(encode_buffer);
    encode_buffer = NULL;
    encode_buffer_len = 0;

    return 0;
}
