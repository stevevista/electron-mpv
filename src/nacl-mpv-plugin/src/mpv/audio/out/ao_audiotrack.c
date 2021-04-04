/*
 * Android AudioTrack audio output driver.
 * Copyright (C) 2018 Aman Gupta <aman@tmm1.net>
 * Copyright (C) 2012-2015 VLC authors and VideoLAN, VideoLabs
 * Authors: Thomas Guillem <thomas@gllm.fr>
 *          Ming Hu <tewilove@gmail.com>
 *
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ao.h"
#include "internal.h"
#include "common/msg.h"
#include "audio/format.h"
#include "options/m_option.h"
#include "osdep/threads.h"
#include "osdep/timer.h"
#include "misc/jni.h"

struct priv {
    jobject audiotrack;
    jint samplerate;
    jint channel_config;
    jint format;
    jint size;

    jobject timestamp;
    int64_t timestamp_fetched;
    bool timestamp_set;
    int timestamp_stable;

    uint32_t written_frames; /* requires uint32_t rollover semantics */
    uint32_t playhead_pos;
    uint32_t playhead_offset;
    bool reset_pending;

    void *chunk;
    int chunksize;
    jbyteArray bytearray;
    jshortArray shortarray;
    jfloatArray floatarray;
    jobject bbuf;

    int cfg_pcm_float;
    int cfg_session_id;

    bool needs_timestamp_offset;
    int64_t timestamp_offset;

    bool thread_terminate;
    bool thread_created;
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t wakeup;
};

struct JNIByteBuffer {
    jclass clazz;
    jmethodID clear;
    struct MPJniField mapping[];
} ByteBuffer = {.mapping = {
    #define OFFSET(member) offsetof(struct JNIByteBuffer, member)
    {"java/nio/ByteBuffer", NULL, NULL, MP_JNI_CLASS, OFFSET(clazz), 1},
    {"java/nio/ByteBuffer", "clear", "()Ljava/nio/Buffer;", MP_JNI_METHOD, OFFSET(clear), 1},
    {0},
    #undef OFFSET
}};

struct JNIAudioTrack {
    jclass clazz;
    jmethodID ctor;
    jmethodID release;
    jmethodID getState;
    jmethodID getPlayState;
    jmethodID play;
    jmethodID stop;
    jmethodID flush;
    jmethodID pause;
    jmethodID write;
    jmethodID writeFloat;
    jmethodID writeV23;
    jmethodID writeShortV23;
    jmethodID writeBufferV21;
    jmethodID getPlaybackHeadPosition;
    jmethodID getTimestamp;
    jmethodID getLatency;
    jmethodID getMinBufferSize;
    jmethodID getNativeOutputSampleRate;
    jint STATE_INITIALIZED;
    jint PLAYSTATE_STOPPED;
    jint PLAYSTATE_PAUSED;
    jint PLAYSTATE_PLAYING;
    jint MODE_STREAM;
    jint ERROR;
    jint ERROR_BAD_VALUE;
    jint ERROR_INVALID_OPERATION;
    jint WRITE_BLOCKING;
    jint WRITE_NON_BLOCKING;
    struct MPJniField mapping[];
} AudioTrack = {.mapping = {
    #define OFFSET(member) offsetof(struct JNIAudioTrack, member)
    {"android/media/AudioTrack", NULL, NULL, MP_JNI_CLASS, OFFSET(clazz), 1},
    {"android/media/AudioTrack", "<init>", "(IIIIIII)V", MP_JNI_METHOD, OFFSET(ctor), 1},
    {"android/media/AudioTrack", "release", "()V", MP_JNI_METHOD, OFFSET(release), 1},
    {"android/media/AudioTrack", "getState", "()I", MP_JNI_METHOD, OFFSET(getState), 1},
    {"android/media/AudioTrack", "getPlayState", "()I", MP_JNI_METHOD, OFFSET(getPlayState), 1},
    {"android/media/AudioTrack", "play", "()V", MP_JNI_METHOD, OFFSET(play), 1},
    {"android/media/AudioTrack", "stop", "()V", MP_JNI_METHOD, OFFSET(stop), 1},
    {"android/media/AudioTrack", "flush", "()V", MP_JNI_METHOD, OFFSET(flush), 1},
    {"android/media/AudioTrack", "pause", "()V", MP_JNI_METHOD, OFFSET(pause), 1},
    {"android/media/AudioTrack", "write", "([BII)I", MP_JNI_METHOD, OFFSET(write), 1},
    {"android/media/AudioTrack", "write", "([FIII)I", MP_JNI_METHOD, OFFSET(writeFloat), 1},
    {"android/media/AudioTrack", "write", "([BIII)I", MP_JNI_METHOD, OFFSET(writeV23), 0},
    {"android/media/AudioTrack", "write", "([SIII)I", MP_JNI_METHOD, OFFSET(writeShortV23), 0},
    {"android/media/AudioTrack", "write", "(Ljava/nio/ByteBuffer;II)I", MP_JNI_METHOD, OFFSET(writeBufferV21), 1},
    {"android/media/AudioTrack", "getTimestamp", "(Landroid/media/AudioTimestamp;)Z", MP_JNI_METHOD, OFFSET(getTimestamp), 1},
    {"android/media/AudioTrack", "getPlaybackHeadPosition", "()I", MP_JNI_METHOD, OFFSET(getPlaybackHeadPosition), 1},
    {"android/media/AudioTrack", "getLatency", "()I", MP_JNI_METHOD, OFFSET(getLatency), 1},
    {"android/media/AudioTrack", "getMinBufferSize", "(III)I", MP_JNI_STATIC_METHOD, OFFSET(getMinBufferSize), 1},
    {"android/media/AudioTrack", "getNativeOutputSampleRate", "(I)I", MP_JNI_STATIC_METHOD, OFFSET(getNativeOutputSampleRate), 1},
    {"android/media/AudioTrack", "WRITE_BLOCKING", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(WRITE_BLOCKING), 0},
    {"android/media/AudioTrack", "WRITE_NON_BLOCKING", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(WRITE_NON_BLOCKING), 0},
    {"android/media/AudioTrack", "STATE_INITIALIZED", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(STATE_INITIALIZED), 1},
    {"android/media/AudioTrack", "PLAYSTATE_STOPPED", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(PLAYSTATE_STOPPED), 1},
    {"android/media/AudioTrack", "PLAYSTATE_PAUSED", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(PLAYSTATE_PAUSED), 1},
    {"android/media/AudioTrack", "PLAYSTATE_PLAYING", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(PLAYSTATE_PLAYING), 1},
    {"android/media/AudioTrack", "MODE_STREAM", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(MODE_STREAM), 1},
    {"android/media/AudioTrack", "ERROR", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ERROR), 1},
    {"android/media/AudioTrack", "ERROR_BAD_VALUE", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ERROR_BAD_VALUE), 1},
    {"android/media/AudioTrack", "ERROR_INVALID_OPERATION", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ERROR_INVALID_OPERATION), 1},
    {0}
    #undef OFFSET
}};

struct JNIAudioFormat {
    jclass clazz;
    jint ENCODING_PCM_8BIT;
    jint ENCODING_PCM_16BIT;
    jint ENCODING_PCM_FLOAT;
    jint ENCODING_IEC61937;
    jint ENCODING_AC3;
    jint CHANNEL_OUT_MONO;
    jint CHANNEL_OUT_STEREO;
    jint CHANNEL_OUT_FRONT_LEFT;
    jint CHANNEL_OUT_FRONT_RIGHT;
    jint CHANNEL_OUT_BACK_LEFT;
    jint CHANNEL_OUT_BACK_RIGHT;
    jint CHANNEL_OUT_FRONT_CENTER;
    jint CHANNEL_OUT_LOW_FREQUENCY;
    jint CHANNEL_OUT_BACK_CENTER;
    jint CHANNEL_OUT_5POINT1;
    jint CHANNEL_OUT_SIDE_LEFT;
    jint CHANNEL_OUT_SIDE_RIGHT;
    struct MPJniField mapping[];
} AudioFormat = {.mapping = {
    #define OFFSET(member) offsetof(struct JNIAudioFormat, member)
    {"android/media/AudioFormat", NULL, NULL, MP_JNI_CLASS, OFFSET(clazz), 1},
    {"android/media/AudioFormat", "ENCODING_PCM_8BIT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ENCODING_PCM_8BIT), 1},
    {"android/media/AudioFormat", "ENCODING_PCM_16BIT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ENCODING_PCM_16BIT), 1},
    {"android/media/AudioFormat", "ENCODING_PCM_FLOAT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ENCODING_PCM_FLOAT), 1},
    {"android/media/AudioFormat", "ENCODING_AC3", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ENCODING_AC3), 0},
    {"android/media/AudioFormat", "ENCODING_IEC61937", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ENCODING_IEC61937), 0},
    {"android/media/AudioFormat", "CHANNEL_OUT_MONO", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_MONO), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_STEREO", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_STEREO), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_5POINT1", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_5POINT1), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_FRONT_LEFT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_FRONT_LEFT), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_FRONT_RIGHT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_FRONT_RIGHT), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_FRONT_CENTER", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_FRONT_CENTER), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_LOW_FREQUENCY", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_LOW_FREQUENCY), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_BACK_LEFT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_BACK_LEFT), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_BACK_RIGHT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_BACK_RIGHT), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_BACK_CENTER", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_BACK_CENTER), 1},
    {"android/media/AudioFormat", "CHANNEL_OUT_SIDE_LEFT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_SIDE_LEFT), 0},
    {"android/media/AudioFormat", "CHANNEL_OUT_SIDE_RIGHT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(CHANNEL_OUT_SIDE_RIGHT), 0},
    {0}
    #undef OFFSET
}};

struct JNIAudioManager {
    jclass clazz;
    jint ERROR_DEAD_OBJECT;
    jint STREAM_MUSIC;
    struct MPJniField mapping[];
} AudioManager = {.mapping = {
    #define OFFSET(member) offsetof(struct JNIAudioManager, member)
    {"android/media/AudioManager", NULL, NULL, MP_JNI_CLASS, OFFSET(clazz), 1},
    {"android/media/AudioManager", "STREAM_MUSIC", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(STREAM_MUSIC), 1},
    {"android/media/AudioManager", "ERROR_DEAD_OBJECT", "I", MP_JNI_STATIC_FIELD_AS_INT, OFFSET(ERROR_DEAD_OBJECT), 0},
    {0}
    #undef OFFSET
}};

struct JNIAudioTimestamp {
    jclass clazz;
    jmethodID ctor;
    jfieldID framePosition;
    jfieldID nanoTime;
    struct MPJniField mapping[];
} AudioTimestamp = {.mapping = {
    #define OFFSET(member) offsetof(struct JNIAudioTimestamp, member)
    {"android/media/AudioTimestamp", NULL, NULL, MP_JNI_CLASS, OFFSET(clazz), 1},
    {"android/media/AudioTimestamp", "<init>", "()V", MP_JNI_METHOD, OFFSET(ctor), 1},
    {"android/media/AudioTimestamp", "framePosition", "J", MP_JNI_FIELD, OFFSET(framePosition), 1},
    {"android/media/AudioTimestamp", "nanoTime", "J", MP_JNI_FIELD, OFFSET(nanoTime), 1},
    {0}
    #undef OFFSET
}};

static int AudioTrack_New(struct ao *ao)
{
    struct priv *p = ao->priv;
    JNIEnv *env = MP_JNI_GET_ENV(ao);

    jobject audiotrack = MP_JNI_NEW(
        AudioTrack.clazz,
        AudioTrack.ctor,
        AudioManager.STREAM_MUSIC,
        p->samplerate,
        p->channel_config,
        p->format,
        p->size,
        AudioTrack.MODE_STREAM,
        p->cfg_session_id
    );
    if (!audiotrack || MP_JNI_EXCEPTION_LOG(ao) < 0) {
        MP_FATAL(ao, "AudioTrack Init failed\n");
        return -1;
    }

    if (MP_JNI_CALL_INT(audiotrack, AudioTrack.getState) != AudioTrack.STATE_INITIALIZED) {
        MP_JNI_CALL_VOID(audiotrack, AudioTrack.release);
        MP_JNI_EXCEPTION_LOG(ao);
        (*env)->DeleteLocalRef(env, audiotrack);
        MP_ERR(ao, "AudioTrack.getState failed\n");
        return -1;
    }

    p->audiotrack = (*env)->NewGlobalRef(env, audiotrack);
    (*env)->DeleteLocalRef(env, audiotrack);
    if (!p->audiotrack)
        return -1;

    return 0;
}

static int AudioTrack_Recreate(struct ao *ao)
{
    struct priv *p = ao->priv;
    JNIEnv *env = MP_JNI_GET_ENV(ao);

    MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.release);
    MP_JNI_EXCEPTION_LOG(ao);
    (*env)->DeleteGlobalRef(env, p->audiotrack);
    p->audiotrack = NULL;
    return AudioTrack_New(ao);
}

static uint32_t AudioTrack_getPlaybackHeadPosition(struct ao *ao)
{
    struct priv *p = ao->priv;
    if (!p->audiotrack)
        return 0;
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    uint32_t pos = 0;
    int64_t now = mp_raw_time_us();
    int state = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.getPlayState);

    int stable_count = 20;
    int64_t wait = p->timestamp_stable < stable_count ? 50000 : 3000000;

    if (state == AudioTrack.PLAYSTATE_PLAYING && p->format != AudioFormat.ENCODING_IEC61937 &&
        (p->timestamp_fetched == 0 || now - p->timestamp_fetched >= wait)) {
        if (!p->timestamp_fetched)
            p->timestamp_stable = 0;

        int64_t utime1 = MP_JNI_GET_LONG(p->timestamp, AudioTimestamp.nanoTime) / 1000;
        if (MP_JNI_CALL_BOOL(p->audiotrack, AudioTrack.getTimestamp, p->timestamp)) {
            p->timestamp_set = true;
            p->timestamp_fetched = now;
            if (p->timestamp_stable < stable_count) {
                uint32_t fpos = 0xFFFFFFFFL & MP_JNI_GET_LONG(p->timestamp, AudioTimestamp.framePosition);
                int64_t utime2 = MP_JNI_GET_LONG(p->timestamp, AudioTimestamp.nanoTime) / 1000;
                //MP_VERBOSE(ao, "getTimestamp: fpos= %u / time= %"PRId64" / now= %"PRId64" / stable= %d\n", fpos, utime2, now, p->timestamp_stable);
                if (utime1 != utime2 && utime2 != 0 && fpos != 0) {
                    p->timestamp_stable++;
                }
            }
        }
    }

    /* AudioTrack's framePosition and playbackHeadPosition return a signed integer,
     * but documentation states it should be interpreted as a 32-bit unsigned integer.
     */
    if (p->timestamp_set) {
        pos = 0xFFFFFFFFL & MP_JNI_GET_LONG(p->timestamp, AudioTimestamp.framePosition);
        uint32_t fpos = pos;
        int64_t utime = MP_JNI_GET_LONG(p->timestamp, AudioTimestamp.nanoTime) / 1000;
        if (utime == 0)
            fpos = pos = 0;
        if (p->needs_timestamp_offset) {
            if (utime != 0 && !p->timestamp_offset)
                p->timestamp_offset = now - utime;
            utime += p->timestamp_offset;
        }
        if (fpos != 0 && utime != 0 && state == AudioTrack.PLAYSTATE_PLAYING) {
            double diff = (double)(now - utime) / 1e6;
            pos += diff * ao->samplerate;
        }
        //MP_VERBOSE(ao, "position = %u via getTimestamp (state = %d / fpos= %u / time= %"PRId64")\n", pos, state, fpos, utime);
    } else {
        pos = 0xFFFFFFFFL & MP_JNI_CALL_INT(p->audiotrack, AudioTrack.getPlaybackHeadPosition);
        //MP_VERBOSE(ao, "playbackHeadPosition = %u (reset_pending=%d)\n", pos, p->reset_pending);
    }


    if (p->format == AudioFormat.ENCODING_IEC61937) {
        if (p->reset_pending) {
            // after a flush(), playbackHeadPosition will not reset to 0 right away.
            // sometimes, it will never reset at all.
            // save the initial offset after the reset, to subtract it going forward.
            if (p->playhead_offset == 0)
                p->playhead_offset = pos;
            p->reset_pending = false;
            MP_VERBOSE(ao, "IEC/playbackHead offset = %d\n", pos);
        }

        // usually shortly after a flush(), playbackHeadPosition will reset to 0.
        // clear out the position and offset to avoid regular "rollover" below
        if (pos == 0 && p->playhead_offset != 0) {
            MP_VERBOSE(ao, "IEC/playbackHeadPosition %d -> %d (flush)\n", p->playhead_pos, pos);
            p->playhead_offset = 0;
            p->playhead_pos = 0;
        }

        // sometimes on a new AudioTrack instance, playbackHeadPosition will reset
        // to 0 shortly after playback starts for no reason.
        if (pos == 0 && p->playhead_pos != 0) {
            MP_VERBOSE(ao, "IEC/playbackHeadPosition %d -> %d (reset)\n", p->playhead_pos, pos);
            p->playhead_offset = 0;
            p->playhead_pos = 0;
            p->written_frames = 0;
        }
    }

    p->playhead_pos = pos;
    return p->playhead_pos - p->playhead_offset;
}

static double AudioTrack_getLatency(struct ao *ao)
{
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    struct priv *p = ao->priv;
    if (!p->audiotrack)
        return 0;

    uint32_t playhead = AudioTrack_getPlaybackHeadPosition(ao);
    uint32_t diff = p->written_frames - playhead;
    double delay = diff / (double)(ao->samplerate);
    if (!p->timestamp_set &&
        p->format != AudioFormat.ENCODING_IEC61937)
        delay += (double)MP_JNI_CALL_INT(p->audiotrack, AudioTrack.getLatency)/1000.0;
    if (delay > 1.0) {
        //MP_WARN(ao, "getLatency: written=%u playhead=%u diff=%u delay=%f\n", p->written_frames, playhead, diff, delay);
        p->timestamp_fetched = 0;
        return 0;
    }
    return MPCLAMP(delay, 0.0, 1.0);
}

static int AudioTrack_write(struct ao *ao, int len)
{
    struct priv *p = ao->priv;
    if (!p->audiotrack)
        return -1;
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    void *buf = p->chunk;

    jint ret;
    if (p->format == AudioFormat.ENCODING_IEC61937) {
        (*env)->SetShortArrayRegion(env, p->shortarray, 0, len / 2, buf);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        ret = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.writeShortV23, p->shortarray, 0, len / 2, AudioTrack.WRITE_BLOCKING);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        if (ret > 0) ret *= 2;

    } else if (p->format == AudioFormat.ENCODING_PCM_FLOAT) {
        (*env)->SetFloatArrayRegion(env, p->floatarray, 0, len / sizeof(float), buf);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        ret = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.writeFloat, p->floatarray, 0, len / sizeof(float), AudioTrack.WRITE_BLOCKING);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        if (ret > 0) ret *= sizeof(float);

    } else if (AudioTrack.writeBufferV21) {
        jobject bbuf = MP_JNI_CALL_OBJECT(p->bbuf, ByteBuffer.clear);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        (*env)->DeleteLocalRef(env, bbuf);
        ret = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.writeBufferV21, p->bbuf, len, AudioTrack.WRITE_BLOCKING);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;

    } else {
        (*env)->SetByteArrayRegion(env, p->bytearray, 0, len, buf);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
        ret = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.write, p->bytearray, 0, len);
        if (MP_JNI_EXCEPTION_LOG(ao) < 0) return -1;
    }

    return ret;
}

static void uninit_jni(struct ao *ao)
{
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    mp_jni_reset_jfields(env, &AudioTrack, AudioTrack.mapping, 1, ao->log);
    mp_jni_reset_jfields(env, &AudioTimestamp, AudioTimestamp.mapping, 1, ao->log);
    mp_jni_reset_jfields(env, &AudioManager, AudioManager.mapping, 1, ao->log);
    mp_jni_reset_jfields(env, &AudioFormat, AudioFormat.mapping, 1, ao->log);
    mp_jni_reset_jfields(env, &ByteBuffer, ByteBuffer.mapping, 1, ao->log);
}

static int init_jni(struct ao *ao)
{
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    if (mp_jni_init_jfields(env, &AudioTrack, AudioTrack.mapping, 1, ao->log) < 0 ||
        mp_jni_init_jfields(env, &ByteBuffer, ByteBuffer.mapping, 1, ao->log) < 0 ||
        mp_jni_init_jfields(env, &AudioTimestamp, AudioTimestamp.mapping, 1, ao->log) < 0 ||
        mp_jni_init_jfields(env, &AudioManager, AudioManager.mapping, 1, ao->log) < 0 ||
        mp_jni_init_jfields(env, &AudioFormat, AudioFormat.mapping, 1, ao->log) < 0) {
            uninit_jni(ao);
            return -1;
    }

    return 0;
}

static void *playthread(void *arg)
{
    struct ao *ao = arg;
    struct priv *p = ao->priv;
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    mpthread_set_name("audiotrack");
    pthread_mutex_lock(&p->lock);
    while (!p->thread_terminate) {
        int state = AudioTrack.PLAYSTATE_PAUSED;
        if (p->audiotrack) {
            state = MP_JNI_CALL_INT(p->audiotrack, AudioTrack.getPlayState);
        }
        if (state == AudioTrack.PLAYSTATE_PLAYING) {
            int read_samples = p->chunksize / ao->sstride;
            int64_t ts = mp_time_us();
            ts += (read_samples / (double)(ao->samplerate)) * 1e6;
            ts += AudioTrack_getLatency(ao) * 1e6;
            int samples = ao_read_data(ao, &p->chunk, read_samples, ts);
            int write_samples = read_samples;
            int ret = AudioTrack_write(ao, write_samples * ao->sstride);
            if (ret >= 0) {
                p->written_frames += ret / ao->sstride;
            } else if (ret == AudioManager.ERROR_DEAD_OBJECT) {
                MP_WARN(ao, "AudioTrack.write failed with ERROR_DEAD_OBJECT. Recreating AudioTrack...\n");
                if (AudioTrack_Recreate(ao) < 0) {
                    MP_ERR(ao, "AudioTrack_Recreate failed\n");
                }
            } else {
                MP_ERR(ao, "AudioTrack.write failed with %d\n", ret);
            }
        } else {
            struct timespec wait = mp_rel_time_to_timespec(0.300);
            pthread_cond_timedwait(&p->wakeup, &p->lock, &wait);
        }
    }
    pthread_mutex_unlock(&p->lock);
    return NULL;
}

static void uninit(struct ao *ao)
{
    struct priv *p = ao->priv;
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    if (p->audiotrack) {
        MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.stop);
        MP_JNI_EXCEPTION_LOG(ao);
        MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.flush);
        MP_JNI_EXCEPTION_LOG(ao);
    }

    pthread_mutex_lock(&p->lock);
    p->thread_terminate = true;
    pthread_cond_signal(&p->wakeup);
    pthread_mutex_unlock(&p->lock);

    if (p->thread_created)
        pthread_join(p->thread, NULL);

    if (p->audiotrack) {
        MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.release);
        MP_JNI_EXCEPTION_LOG(ao);
        (*env)->DeleteGlobalRef(env, p->audiotrack);
        p->audiotrack = NULL;
    }

    if (p->bytearray) {
        (*env)->DeleteGlobalRef(env, p->bytearray);
        p->bytearray = NULL;
    }

    if (p->shortarray) {
        (*env)->DeleteGlobalRef(env, p->shortarray);
        p->shortarray = NULL;
    }

    if (p->floatarray) {
        (*env)->DeleteGlobalRef(env, p->floatarray);
        p->floatarray = NULL;
    }

    if (p->bbuf) {
        (*env)->DeleteGlobalRef(env, p->bbuf);
        p->bbuf = NULL;
    }

    if (p->timestamp) {
        (*env)->DeleteGlobalRef(env, p->timestamp);
        p->timestamp = NULL;
    }

    if (p->chunk) {
        free(p->chunk);
        p->chunk = NULL;
    }

    pthread_cond_destroy(&p->wakeup);
    pthread_mutex_destroy(&p->lock);

    uninit_jni(ao);
}

static int init(struct ao *ao)
{
    struct priv *p = ao->priv;
    JNIEnv *env = MP_JNI_GET_ENV(ao);
    if (!env)
        return -1;

    pthread_mutex_init(&p->lock, NULL);
    pthread_cond_init(&p->wakeup, NULL);

    if (init_jni(ao) < 0)
        return -1;

    if (af_fmt_is_spdif(ao->format)) {
        p->format = AudioFormat.ENCODING_IEC61937;
    } else if (ao->format == AF_FORMAT_U8) {
        p->format = AudioFormat.ENCODING_PCM_8BIT;
    } else if (p->cfg_pcm_float && (ao->format == AF_FORMAT_FLOAT || ao->format == AF_FORMAT_FLOATP)) {
        ao->format = AF_FORMAT_FLOAT;
        p->format = AudioFormat.ENCODING_PCM_FLOAT;
    } else {
        ao->format = AF_FORMAT_S16;
        p->format = AudioFormat.ENCODING_PCM_16BIT;
    }

    if (AudioTrack.getNativeOutputSampleRate) {
        jint samplerate = MP_JNI_CALL_STATIC_INT(
            AudioTrack.clazz,
            AudioTrack.getNativeOutputSampleRate,
            AudioManager.STREAM_MUSIC
        );
        if (MP_JNI_EXCEPTION_LOG(ao) == 0) {
            ao->samplerate = samplerate;
            MP_VERBOSE(ao, "AudioTrack.nativeOutputSampleRate = %d\n", samplerate);
        }
    }
    p->samplerate = ao->samplerate;

    if (p->format == AudioFormat.ENCODING_IEC61937) {
        p->channel_config = AudioFormat.CHANNEL_OUT_STEREO;
    } else if (ao->channels.num == 1) {
        p->channel_config = AudioFormat.CHANNEL_OUT_MONO;
    } else if (ao->channels.num == 6) {
        p->channel_config = AudioFormat.CHANNEL_OUT_5POINT1;
        ao->channels = (struct mp_chmap)MP_CHMAP6(FL, FR, FC, LFE, BL, BR);
    } else {
        p->channel_config = AudioFormat.CHANNEL_OUT_STEREO;
        ao->channels = (struct mp_chmap)MP_CHMAP_INIT_STEREO;
    }

    jint buffer_size = MP_JNI_CALL_STATIC_INT(
        AudioTrack.clazz,
        AudioTrack.getMinBufferSize,
        p->samplerate,
        p->channel_config,
        p->format
    );
    if (buffer_size <= 0 || MP_JNI_EXCEPTION_LOG(ao) < 0) {
        MP_FATAL(ao, "AudioTrack.getMinBufferSize returned an invalid size: %d", buffer_size);
        return -1;
    }
    p->chunksize = buffer_size;
    p->chunk = malloc(buffer_size);

    int min = 0.200 * p->samplerate * af_fmt_to_bytes(ao->format);
    int max = min * 3 / 2;
    p->size = MPCLAMP(buffer_size * 2, min, max);
    MP_VERBOSE(ao, "Setting bufferSize = %d (driver=%d, min=%d, max=%d)\n", p->size, buffer_size, min, max);
    ao->device_buffer = p->size / af_fmt_to_bytes(ao->format);

    jobject timestamp = MP_JNI_NEW(AudioTimestamp.clazz, AudioTimestamp.ctor);
    if (!timestamp || MP_JNI_EXCEPTION_LOG(ao) < 0) {
        MP_FATAL(ao, "AudioTimestamp could not be created\n");
        return -1;
    }
    p->timestamp = (*env)->NewGlobalRef(env, timestamp);
    (*env)->DeleteLocalRef(env, timestamp);

    if (p->format == AudioFormat.ENCODING_IEC61937) {
        jshortArray shortarray = (*env)->NewShortArray(env, p->chunksize / 2);
        p->shortarray = (*env)->NewGlobalRef(env, shortarray);
        (*env)->DeleteLocalRef(env, shortarray);
    } else if (p->format == AudioFormat.ENCODING_PCM_FLOAT) {
        jfloatArray floatarray = (*env)->NewFloatArray(env, p->chunksize / sizeof(float));
        p->floatarray = (*env)->NewGlobalRef(env, floatarray);
        (*env)->DeleteLocalRef(env, floatarray);
    } else if (AudioTrack.writeBufferV21) {
        jobject bbuf = (*env)->NewDirectByteBuffer(env, p->chunk, p->chunksize);
        p->bbuf = (*env)->NewGlobalRef(env, bbuf);
        (*env)->DeleteLocalRef(env, bbuf);
    } else {
        jbyteArray bytearray = (*env)->NewByteArray(env, p->chunksize);
        p->bytearray = (*env)->NewGlobalRef(env, bytearray);
        (*env)->DeleteLocalRef(env, bytearray);
    }

    /* create AudioTrack object */
    if (AudioTrack_New(ao) != 0) {
        MP_FATAL(ao, "Failed to create AudioTrack\n");
        goto error;
    }

    if (pthread_create(&p->thread, NULL, playthread, ao)) {
        MP_ERR(ao, "pthread creation failed\n");
        goto error;
    }
    p->thread_created = true;

    return 1;

error:
    uninit(ao);
    return -1;
}

static void stop(struct ao *ao)
{
    struct priv *p = ao->priv;
    if (!p->audiotrack) {
        MP_ERR(ao, "AudioTrack does not exist to stop!\n");
        return;
    }

    JNIEnv *env = MP_JNI_GET_ENV(ao);
    MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.pause);
    MP_JNI_EXCEPTION_LOG(ao);
    MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.flush);
    MP_JNI_EXCEPTION_LOG(ao);

    p->playhead_offset = 0;
    p->reset_pending = true;
    p->written_frames = 0;
    p->timestamp_fetched = 0;
    p->timestamp_set = false;
    p->timestamp_offset = 0;
}

static void start(struct ao *ao)
{
    struct priv *p = ao->priv;
    if (!p->audiotrack) {
        MP_ERR(ao, "AudioTrack does not exist to start!\n");
        return;
    }

    JNIEnv *env = MP_JNI_GET_ENV(ao);
    MP_JNI_CALL_VOID(p->audiotrack, AudioTrack.play);
    MP_JNI_EXCEPTION_LOG(ao);

    pthread_cond_signal(&p->wakeup);
}

#define OPT_BASE_STRUCT struct priv

const struct ao_driver audio_out_audiotrack = {
    .description = "Android AudioTrack audio output",
    .name      = "audiotrack",
    .init      = init,
    .uninit    = uninit,
    .reset     = stop,
    .start     = start,
    .priv_size = sizeof(struct priv),
    .options   = (const struct m_option[]) {
        {"pcm-float", OPT_FLAG(cfg_pcm_float)},
        {"session-id", OPT_INT(cfg_session_id)},
        {0}
    },
    .options_prefix = "audiotrack",
};
