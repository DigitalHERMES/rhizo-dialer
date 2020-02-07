/*
 * Tablet call app
 *
 * Copyright (C) 2019  Ondřej Jirman <megous@megous.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/time.h>
#include <math.h>

#include <sound/asound.h>
#include <sound/tlv.h>

void syscall_error(int is_err, const char* fmt, ...)
{
	va_list ap;

	if (!is_err)
		return;

	printf("ERROR: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(": %s\n", strerror(errno));

	exit(1);
}

void error(const char* fmt, ...)
{
	va_list ap;

	printf("ERROR: ");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");

	exit(1);
}

struct audio_control_state {
	char name[128];
	union {
		int64_t i[4];
		const char* e[4];
	} vals;
};

static bool audio_restore_state(struct audio_control_state* controls, int n_controls)
{
	int fd;
	int ret;

	fd = open("/dev/snd/controlC0", O_CLOEXEC | O_NONBLOCK);
	if (fd < 0)
		error("failed to open card\n");

	struct snd_ctl_elem_list el = {
		.offset = 0,
		.space = 0,
	};
	ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &el);
	syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_LIST failed");

	struct snd_ctl_elem_id ids[el.count];
	el.pids = ids;
	el.space = el.count;
	ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, &el);
	syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_LIST failed");

	for (int i = 0; i < el.used; i++) {
		struct snd_ctl_elem_info inf = {
			.id = ids[i],
		};

		ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, &inf);
		syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_INFO failed");

		if ((inf.access & SNDRV_CTL_ELEM_ACCESS_READ) && (inf.access & SNDRV_CTL_ELEM_ACCESS_WRITE)) {
			struct snd_ctl_elem_value val = {
				.id = ids[i],
			};
			int64_t cval = 0;

			ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_READ, &val);
			syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_READ failed");

			struct audio_control_state* cs = NULL;
			for (int j = 0; j < n_controls; j++) {
				if (!strcmp(controls[j].name, ids[i].name)) {
					cs = &controls[j];
					break;
				}
			}

			if (!cs)
				continue;

			// check if value needs changing

			switch (inf.type) {
			case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
			case SNDRV_CTL_ELEM_TYPE_INTEGER:
				for (int j = 0; j < inf.count; j++) {
					if (cs->vals.i[j] != val.value.integer.value[j]) {
						// update
						printf("%s <=[%d]= %"PRIi64"\n", ids[i].name, j, cs->vals.i[j]);

						val.value.integer.value[j] = cs->vals.i[j];
						ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &val);
						syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_WRITE failed");
					}
				}

				break;
			case SNDRV_CTL_ELEM_TYPE_INTEGER64:
				for (int j = 0; j < inf.count; j++) {
					if (cs->vals.i[j] != val.value.integer64.value[j]) {
						// update
						printf("%s <=[%d]= %"PRIi64"\n", ids[i].name, j, cs->vals.i[j]);

						val.value.integer64.value[j] = cs->vals.i[j];
						ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &val);
						syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_WRITE failed");
					}
				}

				break;

			case SNDRV_CTL_ELEM_TYPE_ENUMERATED: {
				for (int k = 0; k < inf.count; k++) {
					int eval = -1;
					for (int j = 0; j < inf.value.enumerated.items; j++) {
						inf.value.enumerated.item = j;

						ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_INFO, &inf);
						syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_INFO failed");

						if (!strcmp(cs->vals.e[k], inf.value.enumerated.name)) {
							eval = j;
							break;
						}
					}

					if (eval < 0)
						error("enum value %s not found\n", cs->vals.e[k]);

					if (eval != val.value.enumerated.item[k]) {
						// update
						printf("%s <=%d= %s\n", ids[i].name, k, cs->vals.e[k]);

						val.value.enumerated.item[k] = eval;
						ret = ioctl(fd, SNDRV_CTL_IOCTL_ELEM_WRITE, &val);
						syscall_error(ret < 0, "SNDRV_CTL_IOCTL_ELEM_WRITE failed");
					}
				}

				break;
			}
			}
		}
	}

	close(fd);
	return true;
}

struct audio_setup {
	bool mic_on;
	bool mic_modem_en;
	bool spk_on;
	bool hp_on;
	bool ear_on;

	// when sending audio to modem from AIF1 R, also play that back
	// to me locally (just like AIF1 L plays just to me)
	bool play_me_aif1_to_modem;

	// keep this off untill the call starts, then turn it on
	bool dai2_en;

	// todo
	int spk_vol;
	int ear_vol;
	int mic_gain;
};

static void audio_set_controls(struct audio_setup* s)
{
	struct audio_control_state controls[] = {
		{ .name = "AIF1 AD0 Capture Volume",                        .vals.i = { 180, 180 } },
		{ .name = "AIF1 DA0 Playback Volume",                       .vals.i = { 180, 180 } },
		{ .name = "AIF2 ADC Capture Volume",                        .vals.i = { 181, 181 } },
		{ .name = "AIF2 DAC Playback Volume",                       .vals.i = { 177, 177 } },
		{ .name = "ADC Capture Volume",                             .vals.i = { 184, 184 } },
		{ .name = "DAC Playback Volume",                            .vals.i = { 177, 177 } },
		{ .name = "Headphone Playback Volume",                      .vals.i = { 51 } },
		{ .name = "Headphone Playback Switch",                      .vals.i = { !!s->hp_on, !!s->hp_on } },
		{ .name = "Mic1 Playback Volume",                           .vals.i = { 0 } },
		{ .name = "Mic1 Boost Volume",                              .vals.i = { 4 } },
		{ .name = "Mic2 Playback Volume",                           .vals.i = { 0 } },
		{ .name = "Mic2 Boost Volume",                              .vals.i = { 0 } },
		{ .name = "ADC Gain Capture Volume",                        .vals.i = { 5 } },
		{ .name = "Line In Playback Volume",                        .vals.i = { 0 } },
		{ .name = "Line Out Playback Volume",                       .vals.i = { 23 } },
		{ .name = "Line Out Playback Switch",                       .vals.i = { !!s->spk_on, !!s->spk_on } },
		{ .name = "Earpiece Playback Volume",                       .vals.i = { 23 } },
		{ .name = "Earpiece Playback Switch",                       .vals.i = { !!s->ear_on } },
		{ .name = "AIF1 Loopback Switch",                           .vals.i = { 0 } },
		{ .name = "AIF2 Loopback Switch",                           .vals.i = { 0 } },
		{ .name = "AIF1 AD0 Stereo Capture Route",                  .vals.e = { "Stereo", "Stereo" } },
		{ .name = "AIF2 ADC Stereo Capture Route",                  .vals.e = { "Sum Mono", "Sum Mono" } },
		{ .name = "AIF1 AD0 Mixer AIF1 DA0 Capture Switch",         .vals.i = { 0, 0 } },
		{ .name = "AIF1 AD0 Mixer AIF2 DAC Capture Switch",         .vals.i = { 0, 1 } },
		{ .name = "AIF1 AD0 Mixer ADC Capture Switch",              .vals.i = { 1, 0 } },
		{ .name = "AIF1 AD0 Mixer AIF2 DAC Rev Capture Switch",     .vals.i = { 0, 0 } },
		{ .name = "AIF2 ADC Mixer AIF1 DA0 Capture Switch",         .vals.i = { 0, 1 } },
		{ .name = "AIF2 ADC Mixer AIF2 DAC Rev Capture Switch",     .vals.i = { 0, 0 } },
		{ .name = "AIF2 ADC Mixer ADC Capture Switch",              .vals.i = { !!s->mic_modem_en && !!s->dai2_en, !!s->mic_modem_en && !!s->dai2_en } },
		{ .name = "AIF1 DA0 Stereo Playback Route",                 .vals.e = { "Stereo", "Stereo" } },
		{ .name = "AIF2 DAC Stereo Playback Route",                 .vals.e = { "Sum Mono", "Sum Mono" } },
		{ .name = "DAC Mixer AIF1 DA0 Playback Switch",             .vals.i = { 1, !!s->play_me_aif1_to_modem } },
		{ .name = "DAC Mixer AIF2 DAC Playback Switch",             .vals.i = { 0, !!s->dai2_en } },
		{ .name = "DAC Mixer ADC Playback Switch",                  .vals.i = { 0, 0 } },
		{ .name = "Headphone Source Playback Route",                .vals.e = { "Mixer", "Mixer" } },
		{ .name = "Line Out Source Playback Route",                 .vals.e = { "Mono Differential", "Mono Differential" } },
		{ .name = "Earpiece Source Playback Route",                 .vals.e = { "Left Mixer" } },
		{ .name = "DAC Playback Switch",                            .vals.i = { 1, 1 } },
		{ .name = "DAC Reversed Playback Switch",                   .vals.i = { 1, 1 } },
		{ .name = "Line In Playback Switch",                        .vals.i = { 0, 0 } },
		{ .name = "Mic1 Playback Switch",                           .vals.i = { 0, 0 } },
		{ .name = "Mic2 Playback Switch",                           .vals.i = { 0, 0 } },
		{ .name = "Mixer Capture Switch",                           .vals.i = { 0, 0 } },
		{ .name = "Mixer Reversed Capture Switch",                  .vals.i = { 0, 0 } },
		{ .name = "Line In Capture Switch",                         .vals.i = { 0, 0 } },
		{ .name = "Mic1 Capture Switch",                            .vals.i = { !!s->mic_on, !!s->mic_on } },
		{ .name = "Mic2 Capture Switch",                            .vals.i = { 0, 0 } },
	};

	audio_restore_state(controls, sizeof(controls) / sizeof(controls[0]));
}

static struct audio_setup audio_setup = {
	.mic_on = false,
	.mic_modem_en = true,
	.spk_on = false,
	.hp_on = false,
	.ear_on = false,
	.play_me_aif1_to_modem = false,
	.dai2_en = false,

	//.spk_vol = 50,
	//.ear_vol = 50,
	//.mic_gain = 10,
};

int main(int ac, char* av[])
{
	int opt;

	while ((opt = getopt(ac, av, "smhe2")) != -1) {
		switch (opt) {
		case 's':
			audio_setup.spk_on = 1;
			break;
		case 'm':
			audio_setup.mic_on = 1;
			break;
		case 'h':
			audio_setup.hp_on = 1;
			break;
		case 'e':
			audio_setup.ear_on = 1;
			break;
		case '2':
			audio_setup.dai2_en = 1;
			break;
		default: /* '?' */
			fprintf(stderr, "Usage: %s [-s] [-m] [-h] [-e] [-2]\n", av[0]);
			exit(EXIT_FAILURE);
		}
	}

	audio_set_controls(&audio_setup);
	return 0;
}
