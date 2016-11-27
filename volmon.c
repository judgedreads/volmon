#include <stdio.h>
#include <poll.h>
#include <alsa/asoundlib.h>
#include <alsa/control.h>

int subscribe(const char *card, snd_ctl_t **ctlp)
{
	int err;

	err = snd_ctl_open(ctlp, card, SND_CTL_READONLY);
	if (err < 0) {
		fprintf(stderr, "Failed to open device: %s\n", card);
		return err;
	}
	err = snd_ctl_subscribe_events(*ctlp, 1);
	if (err < 0) {
		fprintf(stderr, "Failed to subscribe to events on device: %s\n",
			card);
		snd_ctl_close(*ctlp);
		return err;
	}
	return 0;
}

int print_vol(const char *card, const char *mix)
{
	int err = 0;
	snd_hctl_t *hctl;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;

	err = snd_hctl_open(&hctl, card, 0);
	if (err != 0) {
		fprintf(stderr, "Failed to open device: %s\n", card);
		return err;
	}
	err = snd_hctl_load(hctl);
	if (err != 0) {
		fprintf(stderr, "Failed to load device: %s\n", card);
		snd_hctl_close(hctl);
		return err;
	}

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

	int len_mix_name = strlen(mix) + 17;
	char mix_name[len_mix_name];
	sprintf(mix_name, "%s Playback Volume", mix);
	snd_ctl_elem_id_set_name(id, mix_name);

	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	if (elem == NULL)
		err = -1;

	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, id);

	err = snd_hctl_elem_read(elem, control);
	if (err != 0) {
		fprintf(stderr, "Failed to read volume\n");
		snd_hctl_close(hctl);
		return err;
	}
	int vol = (int) snd_ctl_elem_value_get_integer(control, 0);
	if (vol >= 0) {
		fprintf(stdout, "%d\n", vol);
	} else {
		fprintf(stderr, "Failed to read volume\n");
		err = vol;
	}
	snd_hctl_close(hctl);
	return err;
}

int handle_event(snd_ctl_t * ctl, const char *card, const char *mix)
{
	snd_ctl_event_t *event;
	unsigned int mask;
	int err;

	snd_ctl_event_alloca(&event);
	err = snd_ctl_read(ctl, event);
	if (err < 0)
		return err;

	if (snd_ctl_event_get_type(event) != SND_CTL_EVENT_ELEM)
		return 0;

	mask = snd_ctl_event_elem_get_mask(event);
	if (!(mask & SND_CTL_EVENT_MASK_VALUE))
		return 0;

	err = print_vol(card, mix);
	if (err < 0)
		return err;

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

	const char *card = argv[1];
	const char *mix = argv[2];


	snd_ctl_t *ctl;
	int err = 0;

	err = print_vol(card, mix);
	if (err < 0)
		return err;

	err = subscribe(card, &ctl);
	if (err < 0)
		return err;

	struct pollfd fd;
	snd_ctl_poll_descriptors(ctl, &fd, 1);
	while (1) {
		err = poll(&fd, 1, -1);
		if (err <= 0) {
			break;
		}
		unsigned short revents;
		err = snd_ctl_poll_descriptors_revents(ctl, &fd, 1, &revents);
		if (err != 0)
			break;
		if (!revents & POLLIN)
			continue;
		err = handle_event(ctl, card, mix);
		if (err != 0) {
			fprintf(stderr, "Failed to handle volume event\n");
			break;
		}
	}

	snd_ctl_close(ctl);
	return err;
}
