/*
 * FILE:    audio/utils.c
 * AUTHORS: Martin Benes     <martinbenesh@gmail.com>
 *          Lukas Hejtmanek  <xhejtman@ics.muni.cz>
 *          Petr Holub       <hopet@ics.muni.cz>
 *          Milos Liska      <xliska@fi.muni.cz>
 *          Jiri Matela      <matela@ics.muni.cz>
 *          Dalibor Matura   <255899@mail.muni.cz>
 *          Ian Wesley-Smith <iwsmith@cct.lsu.edu>
 *
 * Copyright (c) 2005-2010 CESNET z.s.p.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 
 *      This product includes software developed by CESNET z.s.p.o.
 * 
 * 4. Neither the name of CESNET nor the names of its contributors may be used 
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif // HAVE_CONFIG_H


#include "audio/audio.h"
#include "audio/codec.h"
#include "audio/utils.h" 
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef WORDS_BIGENDIAN
#error "This code will not run with a big-endian machine. Please report a bug to " PACKAGE_BUGREPORT " if you reach here."
#endif // WORDS_BIGENDIAN

using namespace std;

/**
 * @brief Creates empty audio_frame2
 */
audio_frame2::audio_frame2() :
        bps(0), sample_rate(0), codec(AC_PCM)
{
}

/**
 * @brief creates audio_frame2 from POD audio_frame
 */
audio_frame2::audio_frame2(struct audio_frame *old)
{
        init(old->ch_count, AC_PCM, old->bps, old->sample_rate);

        for (int i = 0; i < old->ch_count; i++) {
                resize(i, old->data_len / old->ch_count);
                char *data = channels[i].first.get();
                demux_channel(data, old->data, old->bps, old->data_len, old->ch_count, i);
        }
}

/**
 * @brief Initializes audio_frame2 for use. If already initialized, data are dropped.
 */
void audio_frame2::init(int nr_channels, audio_codec_t c, int b, int sr)
{
        channels.clear();
        channels.resize(nr_channels);
        bps = b;
        codec = c;
        sample_rate = sr;
}

void audio_frame2::append(audio_frame2 const &src)
{
        if (bps != src.bps || sample_rate != src.sample_rate ||
                        channels.size() != src.channels.size()) {
                throw std::logic_error("Trying to append frame with different parameters!");
        }

        for (size_t i = 0; i < channels.size(); i++) {
                unique_ptr<char []> new_data(new char[channels[i].second + src.channels[i].second]);
                copy(channels[i].first.get(), channels[i].first.get() + channels[i].second, new_data.get());
                copy(src.channels[i].first.get(), src.channels[i].first.get() + src.channels[i].second, new_data.get() + channels[i].second);
                channels[i].second += src.channels[i].second;
                channels[i].first = std::move(new_data);
        }
}

void audio_frame2::append(int channel, const char *data, size_t length)
{
        unique_ptr<char []> new_data(new char[channels[channel].second + length]);
        copy(channels[channel].first.get(), channels[channel].first.get() + channels[channel].second, new_data.get());
        copy(data, data + length, new_data.get() + channels[channel].second);
        channels[channel].second += length;
        channels[channel].first = std::move(new_data);
}

/**
 * @brief replaces portion of data of specified channel. If the size of the channel is not sufficient,
 * it is extended and old data are copied.
 */
void audio_frame2::replace(int channel, size_t offset, const char *data, size_t length)
{
        if (channels[channel].second < length + offset) {
                unique_ptr<char []> new_data(new char[length + offset]);
                copy(channels[channel].first.get(), channels[channel].first.get() +
                                channels[channel].second, new_data.get());

                channels[channel].second = length + offset;
                channels[channel].first = std::move(new_data);
        }

        copy(data, data + length, channels[channel].first.get() + offset);
}

/**
 * If the size of the specified channel is less than lenght. Channel length is extended. Otherwise,
 * no action is performed (no shrinking when requestedlength is less than current channel length).
 */
void audio_frame2::resize(int channel, size_t length)
{
        if (channels[channel].second < length) {
                unique_ptr<char []> new_data(new char[length]);
                copy(channels[channel].first.get(), channels[channel].first.get() +
                                channels[channel].second, new_data.get());

                channels[channel].second = length;
                channels[channel].first = std::move(new_data);
        }
}

/**
 * Removes all data from audio_frame2. It is equivalent to call of audio_frame2::init with current frame
 * parameters.
 */
void audio_frame2::reset()
{
        for (size_t i = 0; i < channels.size(); i++) {
                channels[i].first = unique_ptr<char []>(new char[0]);
                channels[i].second = 0;
        }
}

int audio_frame2::get_bps()
{
        return bps;
}

audio_codec_t audio_frame2::get_codec()
{
        return codec;
}

const char *audio_frame2::get_data(int channel)
{
        return channels[channel].first.get();
}

size_t audio_frame2::get_data_len(int channel)
{
        return channels[channel].second;
}

int audio_frame2::get_channel_count()
{
        return channels.size();
}

int audio_frame2::get_sample_count()
{
        if (channels.size() == 0) {
                throw logic_error("zero channels!");
        }

        // for PCM, we can deduce samples count from length of the data
        if (codec == AC_PCM) {
                return channels[0].second / channels.size() / get_bps();
        } else {
                throw logic_error("Unknown sample count for compressed audio!");
        }
}

int audio_frame2::get_sample_rate()
{
        return sample_rate;
}

bool audio_frame2::has_same_prop_as(audio_frame2 const &frame) {
        return bps == frame.bps &&
                sample_rate == frame.sample_rate &&
                codec == frame.codec &&
                channels.size() == frame.channels.size();
}

static double get_normalized(const char *in, int bps) {
        int64_t sample = 0;
        bool negative = false;

        for (int j = 0; j < bps; ++j) {
                sample = (sample | (((uint8_t)in[j]) << (8ull * j)));
        }
        if ((int8_t)(in[bps - 1] < 0))
                negative = true;
        if (negative) {
                for (int i = bps; i < 8; ++i) {
                        sample = (sample |  (255ull << (8ull * i)));
                }
        }
        return (double) sample / ((1 << (bps * 8 - 1)));
}

/**
 * @brief Calculates mean and peak RMS from audio samples
 *
 * @param[in]  frame   audio frame
 * @param[in]  channel channel index to calculate RMS to
 * @param[out] peak    peak RMS
 * @returns            mean RMS
 */
double calculate_rms(audio_frame2 *frame, int channel, double *peak)
{
        assert(frame->get_codec() == AC_PCM);
        double sum = 0;
        *peak = 0;
        int sample_count = frame->get_data_len(channel) / frame->get_bps();
        const char *channel_data = frame->get_data(channel);
        for (size_t i = 0; i < frame->get_data_len(channel); i += frame->get_bps()) {
                double val = get_normalized(channel_data + i, frame->get_bps());
                sum += val;
                if (fabs(val) > *peak) {
                        *peak = fabs(val);
                }
        }

        double average = sum / sample_count;

        double sumMeanSquare = 0.0;

        for (size_t i = 0; i < frame->get_data_len(channel); i += frame->get_bps()) {
                sumMeanSquare += pow(get_normalized(channel_data + i, frame->get_bps())
                                - average, 2.0);
        }

        double averageMeanSquare = sumMeanSquare / sample_count;
        double rootMeanSquare = sqrt(averageMeanSquare);

        return rootMeanSquare;
}

bool audio_desc_eq(struct audio_desc a1, struct audio_desc a2) {
        return a1.bps == a2.bps &&
                a1.sample_rate == a2.sample_rate &&
                a1.ch_count == a2.ch_count &&
                a1.codec == a2.codec;
}

struct audio_desc audio_desc_from_audio_frame(struct audio_frame *frame) {
        return (struct audio_desc) { .bps = frame->bps,
                .sample_rate = frame->sample_rate,
                .ch_count = frame->ch_count,
                .codec = AC_PCM
        };
}

struct audio_desc audio_desc_from_audio_frame2(audio_frame2 *frame) {
        return (struct audio_desc) { .bps = frame->get_bps(),
                .sample_rate = frame->get_sample_rate(),
                .ch_count = frame->get_channel_count(),
                .codec = frame->get_codec()
        };
}

struct audio_desc audio_desc_from_audio_channel(audio_channel *channel) {
        return (struct audio_desc) { .bps = channel->bps,
                .sample_rate = channel->sample_rate,
                .ch_count = 1,
                .codec = channel->codec
        };
}

void change_bps(char *out, int out_bps, const char *in, int in_bps, int in_len /* bytes */)
{
        int i;

        assert ((unsigned int) out_bps <= sizeof(int32_t));

        for(i = 0; i < in_len / in_bps; i++) {
                int32_t in_value = format_from_in_bps(in, in_bps);

                int32_t out_value;

                if(in_bps > out_bps) {
                        out_value = in_value >> (in_bps * 8 - out_bps * 8);
                } else {
                        out_value = in_value << (out_bps * 8 - in_bps * 8);
                }

                format_to_out_bps(out, out_bps, out_value);

                in += in_bps;
                out += out_bps;
        }
}

void copy_channel(char *out, const char *in, int bps, int in_len /* bytes */, int out_channel_count)
{
        int samples = in_len / bps;
        int i;
        
        assert(out_channel_count > 0);
        assert(bps > 0);
        assert(in_len >= 0);
        
        in += in_len;
        out += in_len * out_channel_count;
        for (i = samples; i > 0 ; --i) {
                int j;
                
                in -= bps;
                for  (j = out_channel_count + 0; j > 0; --j) {
                        out -= bps;
                        memmove(out, in, bps);
                }
        }
}

void audio_frame_multiply_channel(struct audio_frame *frame, int new_channel_count) {
        assert(frame->max_size >= (unsigned int) frame->data_len * new_channel_count / frame->ch_count);

        copy_channel(frame->data, frame->data, frame->bps, frame->data_len, new_channel_count);
}

void demux_channel(char *out, char *in, int bps, int in_len, int in_stream_channels, int pos_in_stream)
{
        int samples = in_len / (in_stream_channels * bps);
        int i;

        assert (bps <= 4);

        in += pos_in_stream * bps;

        for (i = 0; i < samples; ++i) {
                memcpy(out, in, bps);

                out += bps;
                in += in_stream_channels * bps;

        }
}

void mux_channel(char *out, const char *in, int bps, int in_len, int out_stream_channels, int pos_in_stream, double scale)
{
        int samples = in_len / bps;
        int i;
        
        assert (bps <= 4);

        out += pos_in_stream * bps;

        if(scale == 1.0) {
                for (i = 0; i < samples; ++i) {
                        memcpy(out, in, bps);

                        in += bps;
                        out += out_stream_channels * bps;

                }
        } else {
                for (i = 0; i < samples; ++i) {
                        int32_t in_value = format_from_in_bps(in, bps);

                        in_value *= scale;

                        format_to_out_bps(out, bps, in_value);

                        in += bps;
                        out += out_stream_channels * bps;
                }
        }
}

void mux_and_mix_channel(char *out, const char *in, int bps, int in_len, int out_stream_channels, int pos_in_stream, double scale)
{
        int i;

        assert (bps <= 4);

        out += pos_in_stream * bps;

        for(i = 0; i < in_len / bps; i++) {
                int32_t in_value = format_from_in_bps(in, bps);
                int32_t out_value = format_from_in_bps(out, bps);

                int32_t new_value = (double)in_value * scale + out_value;

                format_to_out_bps(out, bps, new_value);

                in += bps;
                out += out_stream_channels * bps;
        }
}

double get_avg_volume(char *data, int bps, int in_len, int stream_channels, int pos_in_stream)
{
        float average_vol = 0;
        int i;

        assert ((unsigned int) bps <= sizeof(int32_t));

        data += pos_in_stream * bps;

        for(i = 0; i < in_len / bps; i++) {
                int32_t in_value = format_from_in_bps(data, bps);

                //if(pos_in_stream) fprintf(stderr, "%d-%d ", pos_in_stream, data);

                average_vol = average_vol * (i / ((double) i + 1)) + 
                        fabs(((double) in_value / ((1 << (bps * 8 - 1)) - 1)) / (i + 1));

                data += bps * stream_channels;
        }

        return average_vol;
}

void float2int(char *out, char *in, int len)
{
        float *inf = (float *)(void *) in;
        int32_t *outi = (int32_t *)(void *) out;
        int items = len / sizeof(int32_t);

        while(items-- > 0) {
                *outi++ = *inf++ * INT_MAX;
        }
}

void int2float(char *out, char *in, int len)
{
        int32_t *ini = (int32_t *)(void *) in;
        float *outf = (float *)(void *) out;
        int items = len / sizeof(int32_t);

        while(items-- > 0) {
                *outf++ = (float) *ini++ / INT_MAX;
        }
}

void short_int2float(char *out, char *in, int in_len)
{
        int16_t *ini = (int16_t *)(void *) in;
        float *outf = (float *)(void *) out;
        int items = in_len / sizeof(int16_t);

        while(items-- > 0) {
                *outf++ = (float) *ini++ / SHRT_MAX;
        }
}

void signed2unsigned(char *out, char *in, int in_len)
{
        int8_t *inch = (int8_t *) in;
        uint8_t *outch = (uint8_t *) out;
        int items = in_len / sizeof(int8_t);

        while(items-- > 0) {
                int8_t in_value = *inch++;
                uint8_t out_value = (int) 128 + in_value;
                *outch++ = out_value;
        }
}

void audio_channel_demux(audio_frame2 *frame, int index, audio_channel *channel)
{
        channel->data = (char *) frame->get_data(index);
        channel->data_len = frame->get_data_len(index);
        channel->codec = frame->get_codec();
        channel->bps = frame->get_bps();
        channel->sample_rate = frame->get_sample_rate();
}

int32_t format_from_in_bps(const char * in, int bps) {
        int32_t in_value = 0;
        memcpy(&in_value, in, bps);

        if(in_value >> (bps * 8 - 1) && bps != 4) { //negative
                in_value |= ((1<<(32 - bps * 8)) - 1) << (bps * 8);
        }

        return in_value;
}

void format_to_out_bps(char *out, int bps, int32_t out_value) {
        uint32_t mask = ((1ll << (bps * 8)) - 1);

        // clamp
        if(out_value > (1ll << (bps * 8 - 1)) -1) {
                out_value = (1ll << (bps * 8 - 1)) -1;
        }

        if(out_value < -(1ll << (bps * 8 - 1))) {
                out_value = -(1ll << (bps * 8 - 1));
        }

        uint32_t out_value_formatted = (1 * (0x1 & (out_value >> 31))) << (bps * 8 - 1) | (out_value & mask);

        memcpy(out, &out_value_formatted, bps);
}
