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

#ifndef FFCAMAPI_H
#define FFCAMAPI_H

#define UINT64_C uint64_t
#define INT64_C int64_t
#include <libavformat/avformat.h>

#include <camera/camera_api.h>

#include <pthread.h>
#include <deque>

typedef enum
{
    FFCAMERA_OK = 0,
    FFCAMERA_NO_CODEC_SPECIFIED,
    FFCAMERA_CODEC_NOT_FOUND,
    FFCAMERA_COULD_NOT_OPEN_CODEC,
    FFCAMERA_INVALID_DIMENSIONS,
    FFCAMERA_INVALID_FILE_DESCRIPTOR,
    FFCAMERA_ALREADY_RUNNING,
    FFCAMERA_ALREADY_STOPPED
} ffcamera_error;

typedef struct
{
    int fd;
    int width, height;
    bool running;
    AVCodecContext *codec_context;
    pthread_mutex_t reading_mutex;
    pthread_cond_t read_cond;
    int frame_count;
    std::deque<camera_buffer_t*> frames;
} ffcamera_context;

void ffcamera_init(ffcamera_context *ffc_context);

ffcamera_error ffcamera_open(ffcamera_context *ffc_context);
ffcamera_error ffcamera_open(ffcamera_context *ffc_context, enum CodecID codec_id);

ffcamera_error ffcamera_close(ffcamera_context *ffc_context);

ffcamera_error ffcamera_start(ffcamera_context *ffc_context);
ffcamera_error ffcamera_stop(ffcamera_context *ffc_context);

#endif
