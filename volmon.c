#include <alsa/asoundlib.h>
#include <stdio.h>

long min, max;

int print_vol(snd_mixer_elem_t *elem)
{
	long vol;
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &vol);
	printf("%ld%%\n", (vol * 100) / max);
	return 0;
}

int mixer_elem_cb(snd_mixer_elem_t *elem, unsigned int mask)
{
	if (mask & SND_CTL_EVENT_MASK_VALUE)
		print_vol(elem);
	return 0;
}

void usage()
{
	fprintf(stderr,
		"Usage:\n"
		"\tvolmon <card name> <mix name>\n\n"
		"Examples:\n"
		"\tvolmon default Master\n"
		"\tvolmon hw:1 PCM\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc != 3)
		usage();

	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = argv[1];
	const char *selem_name = argv[2];
	int err;

	if ((err = snd_mixer_open(&handle, 0)) < 0) {
		fprintf(stderr, "Failed to open mixer\n");
		snd_mixer_close(handle);
		return 1;
	}
	if ((err = snd_mixer_attach(handle, card)) < 0) {
		fprintf(stderr, "Failed to attach %s\n", card);
		snd_mixer_close(handle);
		return 1;
	}
	if ((err = snd_mixer_selem_register(handle, NULL, NULL)) < 0) {
		fprintf(stderr, "Failed to register mixer\n");
		snd_mixer_close(handle);
		return 1;
	}
	if ((err = snd_mixer_load(handle)) < 0) {
		fprintf(stderr, "Failed to load mixer\n");
		snd_mixer_close(handle);
		return 1;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_elem_set_callback(elem, mixer_elem_cb);

	print_vol(elem);
	while (1) {
		int res;
		res = snd_mixer_wait(handle, -1);
		if (res >= 0) {
			res = snd_mixer_handle_events(handle);
			if (res < 0) {
				fprintf(stderr, "Failed to handle events\n");
				snd_mixer_close(handle);
				return 1;
			}
		}
	}

	snd_mixer_close(handle);
	return 0;
}
