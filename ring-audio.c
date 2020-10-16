/*
 * Copyright (C) 2020 Rhizomatica <rafael@rhizomatica.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 * Rhizo-dialer is an experimental dialer for Maemo.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <alsa/asoundlib.h>

#include "daemonize.h"
#include "ring-audio.h"

bool ring_2tones (double seconds, double freq1, double freq2)
{
    unsigned int sampling_rate = 48000;
    int dir = 0, rc, i, j = 0;
    char * buffer;
    double x, y1, y2;
    int sample, sample1, sample2, amp = 10000;

    // Here are some alsa specific structures
    snd_pcm_t * handle; // A reference to the sound card
    snd_pcm_hw_params_t * params; // Information about hardware params
    snd_pcm_uframes_t frames = 4; // The size of the period

    // Here we open a reference to the sound card
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc < 0){
        log_message(LOG_FILE, "unable to open default device\n");
        // fprintf(stderr, "unable to open default device: %s\n", snd_strerror(rc));
        return false;
    }

    // Now we allocate memory for the parameters structure on the stack
    snd_pcm_hw_params_alloca(&params);

    // Next, we're going to set all the parameters for the sound card

    // This sets up the soundcard with some default parameters and we'll 
    // customize it a bit afterwards
    snd_pcm_hw_params_any(handle, params);

    // Set the samples for each channel to be interleaved
    snd_pcm_hw_params_set_access(handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    // This says our samples represented as signed, 16 bit integers in
    // little endian format
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(handle, params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(handle, params, &sampling_rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    // Finally, the parameters get written to the sound card
    rc = snd_pcm_hw_params(handle, params);
    if(rc < 0){
        log_message(LOG_FILE, "unable to set the hw params\n");
        exit(1);
    }

    // This allocates memory to hold our samples
    buffer = (char *) malloc(frames * 4);
    j = 0;
    for (i=0;i< seconds * sampling_rate; i++){
        // Create a sample and convert it back to an integer
        x = (double) i / (double) sampling_rate;
        y1 = sin(2.0 * 3.14159 * freq1 * x);
        y2 = sin(2.0 * 3.14159 * freq2 * x);
        sample1 = amp * y1;
        sample2 = amp * y2;
        sample = (sample1 + sample2) / 2;

        // Store the sample in our buffer using Little Endian format
        buffer[0 + 4*j] = sample & 0xff;
        buffer[1 + 4*j] = (sample & 0xff00) >> 8;
        buffer[2 + 4*j] = sample & 0xff;
        buffer[3 + 4*j] = (sample & 0xff00) >> 8;

        // If we have a buffer full of samples, write 1 period of 
        //samples to the sound card
        if(j++ == frames){
            j = snd_pcm_writei(handle, buffer, frames);

            // Check for under runs
            if (j < 0){
                snd_pcm_prepare(handle);
            }
            j = 0;
        }
    }

    // Play all remaining samples before exitting
    snd_pcm_drain(handle);

    // Close the sound card handle
    snd_pcm_close(handle);

    free(buffer);

    return true;
}

bool ring (double seconds, double freq)
{
    unsigned int sampling_rate = 48000;
    int dir = 0, rc, i, j = 0;
    char * buffer;
    double x, y;
    int sample, amp = 10000;

    // Here are some alsa specific structures
    snd_pcm_t * handle; // A reference to the sound card
    snd_pcm_hw_params_t * params; // Information about hardware params
    snd_pcm_uframes_t frames = 4; // The size of the period

    // Here we open a reference to the sound card
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc < 0){
        log_message(LOG_FILE, "unable to open default device\n");
        // fprintf(stderr, "unable to open default device: %s\n", snd_strerror(rc));
        return false;
    }

    // Now we allocate memory for the parameters structure on the stack
    snd_pcm_hw_params_alloca(&params);

    // Next, we're going to set all the parameters for the sound card

    // This sets up the soundcard with some default parameters and we'll 
    // customize it a bit afterwards
    snd_pcm_hw_params_any(handle, params);

    // Set the samples for each channel to be interleaved
    snd_pcm_hw_params_set_access(handle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    // This says our samples represented as signed, 16 bit integers in
    // little endian format
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    // We use 2 channels (left audio and right audio)
    snd_pcm_hw_params_set_channels(handle, params, 2);

    // Here we set our sampling rate.
    snd_pcm_hw_params_set_rate_near(handle, params, &sampling_rate, &dir);

    // This sets the period size
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    // Finally, the parameters get written to the sound card
    rc = snd_pcm_hw_params(handle, params);
    if(rc < 0){
        log_message(LOG_FILE, "unable to set the hw params\n");
        // fprintf(stderr, "unable to set the hw params: %s\n",snd_strerror(rc));
        exit(1);
    }

    // This allocates memory to hold our samples
    buffer = (char *) malloc(frames * 4);
    j = 0;
    for (i=0;i< seconds * sampling_rate; i++){
        // Create a sample and convert it back to an integer
        x = (double) i / (double) sampling_rate;
        y = sin(2.0 * 3.14159 * freq * x);
        sample = amp * y;

        // Store the sample in our buffer using Little Endian format
        buffer[0 + 4*j] = sample & 0xff;
        buffer[1 + 4*j] = (sample & 0xff00) >> 8;
        buffer[2 + 4*j] = sample & 0xff;
        buffer[3 + 4*j] = (sample & 0xff00) >> 8;

        // If we have a buffer full of samples, write 1 period of 
        //samples to the sound card
        if(j++ == frames){
            j = snd_pcm_writei(handle, buffer, frames);

            // Check for under runs
            if (j < 0){
                snd_pcm_prepare(handle);
            }
            j = 0;
        }
    }

    // Play all remaining samples before exitting
    snd_pcm_drain(handle);

    // Close the sound card handle
    snd_pcm_close(handle);

    free(buffer);

    return true;
}
