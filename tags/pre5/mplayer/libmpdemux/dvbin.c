/*

dvbstream
(C) Dave Chapman <dave@dchapman.com> 2001, 2002.

The latest version can be found at http://www.linuxstb.org/dvbstream

Copyright notice:

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "stream.h"
#include "demuxer.h"
#include "help_mp.h"
#include "../m_option.h"
#include "../m_struct.h"

#include "dvbin.h"


#define MAX_CHANNELS 8
#define CHANNEL_LINE_LEN 256
#define min(a, b) ((a) <= (b) ? (a) : (b))


//TODO: CAMBIARE list_ptr e da globale a per_priv


static struct stream_priv_s
{
	char *prog;
	int card;
	char *type;
	int vid, aid;
	char *file;
}
stream_defaults =
{
	"", 1, "", 0, 0, NULL
};

#define ST_OFF(f) M_ST_OFF(struct stream_priv_s, f)

/// URL definition
static m_option_t stream_params[] = {
	{"prog", ST_OFF(prog), CONF_TYPE_STRING, 0, 0 ,0, NULL},
	{"card", ST_OFF(card), CONF_TYPE_INT, M_OPT_RANGE, 1, 4, NULL},
	{"type", ST_OFF(type), CONF_TYPE_STRING, 0, 0 ,0, NULL},
	{"vid",  ST_OFF(vid),  CONF_TYPE_INT, 0, 0 ,0, NULL},
	{"aid",  ST_OFF(aid),  CONF_TYPE_INT, 0, 0 ,0, NULL},
	{"file", ST_OFF(file), CONF_TYPE_STRING, 0, 0 ,0, NULL},

	{"hostname", 	ST_OFF(prog), CONF_TYPE_STRING, 0, 0, 0, NULL },
	{"username", 	ST_OFF(card), CONF_TYPE_INT, M_OPT_RANGE, 1, 4, NULL},
	{NULL, NULL, 0, 0, 0, 0, NULL}
};

static struct m_struct_st stream_opts = {
	"dvbin",
	sizeof(struct stream_priv_s),
	&stream_defaults,
	stream_params
};



m_option_t dvbin_opts_conf[] = {
	{"prog", &stream_defaults.prog, CONF_TYPE_STRING, 0, 0 ,0, NULL},
	{"card", &stream_defaults.card, CONF_TYPE_INT, M_OPT_RANGE, 1, 4, NULL},
	{"type", "DVB card type is autodetected and can't be overridden\n", CONF_TYPE_PRINT, CONF_NOCFG, 0 ,0, NULL},
	{"vid",  &stream_defaults.vid,  CONF_TYPE_INT, 0, 0 ,0, NULL},
	{"aid",  &stream_defaults.aid,  CONF_TYPE_INT, 0, 0 ,0, NULL},
	{"file", &stream_defaults.file, CONF_TYPE_STRING, 0, 0 ,0, NULL},

	{NULL, NULL, 0, 0, 0, 0, NULL}
};




extern int dvb_set_ts_filt(int fd, uint16_t pid, dmx_pes_type_t pestype);
extern int dvb_demux_stop(int fd);
extern int dvb_get_tuner_type(int fd);
int dvb_open_devices(dvb_priv_t *priv, int n, int demux_cnt, int *pids);
int dvb_fix_demuxes(dvb_priv_t *priv, int cnt, int *pids);

extern int dvb_tune(dvb_priv_t *priv, int freq, char pol, int srate, int diseqc, int tone,
		fe_spectral_inversion_t specInv, fe_modulation_t modulation, fe_guard_interval_t guardInterval,
		fe_transmit_mode_t TransmissionMode, fe_bandwidth_t bandWidth, fe_code_rate_t HP_CodeRate);
extern char *dvb_dvrdev[4], *dvb_demuxdev[4], *dvb_frontenddev[4];

static dvb_config_t *dvb_config = NULL;


static dvb_channels_list *dvb_get_channels(char *filename, int type)
{
	dvb_channels_list  *list;
	FILE *f;
	uint8_t line[CHANNEL_LINE_LEN];

	int fields, cnt, pcnt;
	dvb_channel_t *ptr, *tmp, chn;
	char *tmp_lcr, *tmp_hier, *inv, *bw, *cr, *mod, *transm, *gi, *vpid_str, *apid_str;
	const char *cbl_conf = "%a[^:]:%d:%a[^:]:%d:%a[^:]:%a[^:]:%a[^:]:%a[^:]\n";
	const char *sat_conf = "%a[^:]:%d:%c:%d:%d:%a[^:]:%a[^:]\n";
	const char *ter_conf = "%a[^:]:%d:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]:%a[^:]\n";
	
	mp_msg(MSGT_DEMUX, MSGL_V, "CONFIG_READ FILE: %s, type: %d\n", filename, type);
	if((f=fopen(filename, "r"))==NULL)
	{
		mp_msg(MSGT_DEMUX, MSGL_FATAL, "CAN'T READ CONFIG FILE %s\n", filename);
		return NULL;
	}

	list = malloc(sizeof(dvb_channels_list));
	if(list == NULL)
	{
		mp_msg(MSGT_DEMUX, MSGL_V, "DVB_GET_CHANNELS: couldn't malloc enough memory\n");
		return NULL;
	}

	ptr = &chn;
	list->NUM_CHANNELS = 0;
	list->channels = NULL;
	while(! feof(f))
	{
		if( fgets(line, CHANNEL_LINE_LEN, f) == NULL )
			continue;

		if((line[0] == '#') || (strlen(line) == 0))
			continue;


		apid_str = vpid_str = NULL;
		ptr->pids_cnt = 0;
		if(type == TUNER_TER)
		{
			fields = sscanf(line, ter_conf,
				&ptr->name, &ptr->freq, &inv, &bw, &cr, &tmp_lcr, &mod,
				&transm, &gi, &tmp_hier, &vpid_str, &apid_str);
			mp_msg(MSGT_DEMUX, MSGL_V,
				"TER, NUM: %d, NUM_FIELDS: %d, NAME: %s, FREQ: %d",
				list->NUM_CHANNELS, fields, ptr->name, ptr->freq);
		}
		else if(type == TUNER_CBL)
		{
			fields = sscanf(line, cbl_conf,
				&ptr->name, &ptr->freq, &inv, &ptr->srate,
				&cr, &mod, &vpid_str, &apid_str);
			mp_msg(MSGT_DEMUX, MSGL_V,
				"CBL, NUM: %d, NUM_FIELDS: %d, NAME: %s, FREQ: %d, SRATE: %d",
				list->NUM_CHANNELS, fields, ptr->name, ptr->freq, ptr->srate);
		}
		else //SATELLITE
		{
			fields = sscanf(line, sat_conf,
				&ptr->name, &ptr->freq, &ptr->pol, &ptr->diseqc, &ptr->srate, &vpid_str, &apid_str);
			ptr->pol = toupper(ptr->pol);
			ptr->freq *=  1000UL;
			ptr->srate *=  1000UL;
			ptr->tone = -1;
			ptr->inv = INVERSION_AUTO;
			ptr->cr = FEC_AUTO;
			if((ptr->diseqc > 4) || (ptr->diseqc < 0))
			    continue;
			if(ptr->diseqc > 0)
			    ptr->diseqc--;
			mp_msg(MSGT_DEMUX, MSGL_V,
				"SAT, NUM: %d, NUM_FIELDS: %d, NAME: %s, FREQ: %d, SRATE: %d, POL: %c, DISEQC: %d",
				list->NUM_CHANNELS, fields, ptr->name, ptr->freq, ptr->srate, ptr->pol, ptr->diseqc);
		}

		if(vpid_str != NULL)
		{
			pcnt = sscanf(vpid_str, "%d+%d+%d+%d+%d+%d+%d", &ptr->pids[0], &ptr->pids[1], &ptr->pids[2], &ptr->pids[3],
				&ptr->pids[4], &ptr->pids[5], &ptr->pids[6]);
			if(pcnt > 0)
			{
				ptr->pids_cnt = pcnt;
				fields++;
			}
		}
		
		if(apid_str != NULL)
		{
			cnt = ptr->pids_cnt;
			pcnt = sscanf(apid_str, "%d+%d+%d+%d+%d+%d+%d+%d", &ptr->pids[cnt], &ptr->pids[cnt+1], &ptr->pids[cnt+2],
				&ptr->pids[cnt+3], &ptr->pids[cnt+4], &ptr->pids[cnt+5], &ptr->pids[cnt+6], &ptr->pids[cnt+7]);
			if(pcnt > 0)
			{
				ptr->pids_cnt += pcnt;
				fields++;
			}
		}

		if((fields < 3) || (ptr->pids_cnt <= 0) || (ptr->freq == 0) || (strlen(ptr->name) == 0))
			continue;


		ptr->pids[ptr->pids_cnt] = 0;	//PID 0 is the PAT
		ptr->pids_cnt++;
		mp_msg(MSGT_DEMUX, MSGL_V, " PIDS: ");
		for(cnt = 0; cnt < ptr->pids_cnt; cnt++)
			mp_msg(MSGT_DEMUX, MSGL_V, " %d ", ptr->pids[cnt]);
		mp_msg(MSGT_DEMUX, MSGL_V, "\n");
		
		if((type == TUNER_TER) || (type == TUNER_CBL))
		{
			if(! strcmp(inv, "INVERSION_ON"))
				ptr->inv = INVERSION_ON;
			else if(! strcmp(inv, "INVERSION_OFF"))
				ptr->inv = INVERSION_OFF;
			else
				ptr->inv = INVERSION_AUTO;


			if(! strcmp(cr, "FEC_1_2"))
				ptr->cr =FEC_1_2;
			else if(! strcmp(cr, "FEC_2_3"))
				ptr->cr =FEC_2_3;
			else if(! strcmp(cr, "FEC_3_4"))
				ptr->cr =FEC_3_4;
#ifdef HAVE_DVB_HEAD
			else if(! strcmp(cr, "FEC_4_5"))
				ptr->cr =FEC_4_5;
			else if(! strcmp(cr, "FEC_6_7"))
				ptr->cr =FEC_6_7;
			else if(! strcmp(cr, "FEC_8_9"))
				ptr->cr =FEC_8_9;
#endif
			else if(! strcmp(cr, "FEC_5_6"))
				ptr->cr =FEC_5_6;
			else if(! strcmp(cr, "FEC_7_8"))
				ptr->cr =FEC_7_8;
			else if(! strcmp(cr, "FEC_NONE"))
				ptr->cr =FEC_NONE;
			else ptr->cr =FEC_AUTO;

			if(! strcmp(mod, "QAM_128"))
				ptr->mod = QAM_128;
			else if(! strcmp(mod, "QAM_256"))
				ptr->mod = QAM_256;
			else if(! strcmp(mod, "QAM_64"))
				ptr->mod = QAM_64;
			else if(! strcmp(mod, "QAM_32"))
				ptr->mod = QAM_32;
			else if(! strcmp(mod, "QAM_16"))
				ptr->mod = QAM_16;
			//else ptr->mod = QPSK;
		}


		if(type == TUNER_TER)
		{
			if(! strcmp(bw, "BANDWIDTH_6_MHZ"))
				ptr->bw = BANDWIDTH_6_MHZ;
			else if(! strcmp(bw, "BANDWIDTH_7_MHZ"))
				ptr->bw = BANDWIDTH_7_MHZ;
			else if(! strcmp(bw, "BANDWIDTH_8_MHZ"))
				ptr->bw = BANDWIDTH_8_MHZ;


			if(! strcmp(transm, "TRANSMISSION_MODE_2K"))
				ptr->trans = TRANSMISSION_MODE_2K;
			else if(! strcmp(transm, "TRANSMISSION_MODE_8K"))
				ptr->trans = TRANSMISSION_MODE_8K;


			if(! strcmp(gi, "GUARD_INTERVAL_1_32"))
				ptr->gi = GUARD_INTERVAL_1_32;
			else if(! strcmp(gi, "GUARD_INTERVAL_1_16"))
				ptr->gi = GUARD_INTERVAL_1_16;
			else if(! strcmp(gi, "GUARD_INTERVAL_1_8"))
				ptr->gi = GUARD_INTERVAL_1_8;
			else ptr->gi = GUARD_INTERVAL_1_4;
		}

		tmp = (dvb_channel_t*)realloc(list->channels, sizeof(dvb_channel_t) * (list->NUM_CHANNELS + 1));
		if(tmp == NULL)
			break;

		list->channels = tmp;
		memcpy(&(list->channels[list->NUM_CHANNELS]), ptr, sizeof(dvb_channel_t));
		list->NUM_CHANNELS++;
	}

	fclose(f);
	if(list->NUM_CHANNELS == 0)
	{
		if(list->channels != NULL)
			free(list->channels);
		free(list);
		return NULL;
	}

	list->current = 0;
	return list;
}



static int dvb_streaming_read(stream_t *stream, char *buffer, int size)
{
	struct pollfd pfds[1];
	int pos=0, tries, rk, fd;
	dvb_priv_t *priv  = (dvb_priv_t *) stream->priv;

	mp_msg(MSGT_DEMUX, MSGL_DBG3, "dvb_streaming_read(%d)\n", size);

	tries = priv->retry + 1;
	
	fd = stream->fd;
	while(pos < size)
	{
		pfds[0].fd = fd;
		pfds[0].events = POLLIN | POLLPRI;

		poll(pfds, 1, 500);
		rk = size - pos;
		if((rk = read(fd, &buffer[pos], rk)) > 0)
		{
			pos += rk;
			mp_msg(MSGT_DEMUX, MSGL_DBG3, "ret (%d) bytes\n", pos);
		}
		else
		{
			errno = 0;
			mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_streaming_read, attempt N. %d failed with errno %d when reading %d bytes\n", tries, errno, size-pos);
			if(--tries > 0)
				continue;
			else
				break;
			}
		}

	if(! pos)
		mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_streaming_read, return %d bytes\n", pos);

	return pos;
}

static void dvbin_close(stream_t *stream);

int dvb_set_channel(dvb_priv_t *priv, int card, int n)
{
	dvb_channels_list *new_list;
	dvb_channel_t *channel;
	int do_tuning;
	stream_t *stream  = (stream_t*) priv->stream;
	char buf[4096];
	dvb_config_t *conf = (dvb_config_t *) priv->config;
	int devno;
	int i;

	if((card < 0) || (card > conf->count))
	{
		mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_set_channel: INVALID CARD NUMBER: %d vs %d, abort\n", card, conf->count);
		return 0;
	}
	
	devno = conf->cards[card].devno;
	new_list = conf->cards[card].list;
	if((n > new_list->NUM_CHANNELS) || (n < 0))
	{
		mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_set_channel: INVALID CHANNEL NUMBER: %d, for card %d, abort\n", n, card);
		return 0;
	}
	channel = &(new_list->channels[n]);
	
	if(priv->is_on)	//the fds are already open and we have to stop the demuxers
	{
		for(i = 0; i < priv->demux_fds_cnt; i++)
			dvb_demux_stop(priv->demux_fds[i]);
			
		priv->retry = 0;
		while(dvb_streaming_read(stream, buf, 4096) > 0);	//empty both the stream's and driver's buffer
		if(priv->card != card)
		{
			dvbin_close(stream);
			if(! dvb_open_devices(priv, devno, channel->pids_cnt, channel->pids))
			{
				mp_msg(MSGT_DEMUX, MSGL_ERR, "DVB_SET_CHANNEL, COULDN'T OPEN DEVICES OF CARD: %d, EXIT\n", card);
				return 0;
			}
			strcpy(priv->prev_tuning, "");
		}
		else	//close all demux_fds with pos > pids required for the new channel or open other demux_fds if we have too few
		{	
			if(! dvb_fix_demuxes(priv, channel->pids_cnt, channel->pids))
				return 0;
		}
	}
	else
	{
		if(! dvb_open_devices(priv, devno, channel->pids_cnt, channel->pids))
		{
			mp_msg(MSGT_DEMUX, MSGL_ERR, "DVB_SET_CHANNEL2, COULDN'T OPEN DEVICES OF CARD: %d, EXIT\n", card);
			return 0;
		}
		strcpy(priv->prev_tuning, "");
	}

	dvb_config->priv = priv;
	priv->card = card;
	priv->list = new_list;
	priv->retry = 5;
	new_list->current = n;
	stream->fd = priv->dvr_fd;
	mp_msg(MSGT_DEMUX, MSGL_V, "DVB_SET_CHANNEL: new channel name=%s, card: %d, channel %d\n", channel->name, card, n);

	switch(priv->tuner_type)
	{
		case TUNER_SAT:
			sprintf(priv->new_tuning, "%d|%09d|%09d|%d|%c", priv->card, channel->freq, channel->srate, channel->diseqc, channel->pol);
			break;

		case TUNER_TER:
			sprintf(priv->new_tuning, "%d|%09d|%d|%d|%d|%d|%d|%d", priv->card, channel->freq, channel->inv,
				channel->bw, channel->cr, channel->mod, channel->trans, channel->gi);
		  break;

		case TUNER_CBL:
			sprintf(priv->new_tuning, "%d|%09d|%d|%d|%d|%d", priv->card, channel->freq, channel->inv, channel->srate,
				channel->cr, channel->mod);
		break;
	}



	if(strcmp(priv->prev_tuning, priv->new_tuning))
	{
		mp_msg(MSGT_DEMUX, MSGL_V, "DIFFERENT TUNING THAN THE PREVIOUS: %s  -> %s\n", priv->prev_tuning, priv->new_tuning);
		strcpy(priv->prev_tuning, priv->new_tuning);
		do_tuning = 1;
	}
	else
	{
		mp_msg(MSGT_DEMUX, MSGL_V, "SAME TUNING PARAMETERS, NO TUNING\n");
		do_tuning = 0;
	}

	stream->eof=1;
	stream_reset(stream);


	if(do_tuning)
		if (! dvb_tune(priv, channel->freq, channel->pol, channel->srate, channel->diseqc, channel->tone,
			channel->inv, channel->mod, channel->gi, channel->trans, channel->bw, channel->cr))
			return 0;


	priv->is_on = 1;

	//sets demux filters and restart the stream
	for(i = 0; i < channel->pids_cnt; i++)
	{
		if(! dvb_set_ts_filt(priv->demux_fds[i], channel->pids[i], DMX_PES_OTHER))
			return 0;
	}
	
	return 1;
}



int dvb_step_channel(dvb_priv_t *priv, int dir)
{
	int new_current;
	dvb_channels_list *list;

	mp_msg(MSGT_DEMUX, MSGL_V, "DVB_STEP_CHANNEL dir %d\n", dir);

	if(priv == NULL)
	{
		mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_step_channel: NULL priv_ptr, quit\n");
		return 0;
	}

	list = priv->list;
	if(list == NULL)
	{
		mp_msg(MSGT_DEMUX, MSGL_ERR, "dvb_step_channel: NULL list_ptr, quit\n");
		return 0;
	}


	if(dir == DVB_CHANNEL_HIGHER)
	{
		if(list->current == list->NUM_CHANNELS-1)
			return 0;

		new_current = list->current + 1;
	}
	else
	{
		if(list->current == 0)
			return 0;

		new_current = list->current - 1;
	}

	return dvb_set_channel(priv, priv->card, new_current);
}




extern char *get_path(char *);

static void dvbin_close(stream_t *stream)
{
	int i;
	dvb_priv_t *priv  = (dvb_priv_t *) stream->priv;

	for(i = priv->demux_fds_cnt-1; i >= 0; i--)
	{
		priv->demux_fds_cnt--;
		mp_msg(MSGT_DEMUX, MSGL_V, "DVBIN_CLOSE, close(%d), fd=%d, COUNT=%d\n", i, priv->demux_fds[i], priv->demux_fds_cnt);
		close(priv->demux_fds[i]);
	}
	close(priv->dvr_fd);

	close(priv->fe_fd);
#ifdef HAVE_DVB
	close(priv->sec_fd);
#endif

	priv->is_on = 0;
	dvb_config->priv = NULL;
}


static int dvb_streaming_start(dvb_priv_t *priv, struct stream_priv_s *opts, int tuner_type, char *progname)
{
	int i;
	dvb_channel_t *channel = NULL;
	stream_t *stream  = (stream_t*) priv->stream;


	mp_msg(MSGT_DEMUX, MSGL_INFO, "code taken from dvbstream for mplayer v0.4pre1 - (C) Dave Chapman 2001\n");
	mp_msg(MSGT_DEMUX, MSGL_INFO, "Released under the GPL.\n");
	mp_msg(MSGT_DEMUX, MSGL_INFO, "Latest version available from http://www.linuxstb.org/\n");
	mp_msg(MSGT_DEMUX, MSGL_V, 	  "PROG: %s, CARD: %d, VID: %d, AID: %d, TYPE: %s, FILE: %s\n",
	    opts->prog, opts->card, opts->vid, opts->aid,  opts->type, opts->file);

	priv->is_on = 0;

			i = 0;
			while((channel == NULL) && i < priv->list->NUM_CHANNELS)
			{
				if(! strcmp(priv->list->channels[i].name, progname))
					channel = &(priv->list->channels[i]);

				i++;
			}

			if(channel != NULL)
			{
				priv->list->current = i-1;
				mp_msg(MSGT_DEMUX, MSGL_V, "PROGRAM NUMBER %d: name=%s, freq=%u\n", i-1, channel->name, channel->freq);
			}
		else
		{
				mp_msg(MSGT_DEMUX, MSGL_ERR, "\n\nDVBIN: no such channel \"%s\"\n\n", progname);
	  return 0;
	}


	strcpy(priv->prev_tuning, "");
	if(!dvb_set_channel(priv, priv->card, priv->list->current))
	{
		mp_msg(MSGT_DEMUX, MSGL_ERR, "ERROR, COULDN'T SET CHANNEL  %i: ", priv->list->current);
		dvbin_close(stream);
		return 0;
	}

	mp_msg(MSGT_DEMUX, MSGL_V,  "SUCCESSFUL EXIT from dvb_streaming_start\n");

	return 1;
}




static int dvb_open(stream_t *stream, int mode, void *opts, int *file_format)
{
	// I don't force  the file format bacause, although it's almost always TS,
	// there are some providers that stream an IP multicast with M$ Mpeg4 inside
	struct stream_priv_s* p = (struct stream_priv_s*)opts;
	dvb_priv_t *priv;
	char *progname;
	int tuner_type = 0;


	if(mode != STREAM_READ)
		return STREAM_UNSUPORTED;

	stream->priv = (dvb_priv_t*) malloc(sizeof(dvb_priv_t));
	if(stream->priv ==  NULL)
		return STREAM_ERROR;

	priv = (dvb_priv_t *)stream->priv;
	priv->stream = stream;
	dvb_config = dvb_get_config();
	if(dvb_config == NULL)
	{
		free(priv);
		mp_msg(MSGT_DEMUX, MSGL_ERR, "DVB CONFIGURATION IS EMPTY, exit\n");
		return STREAM_ERROR;
	}
	dvb_config->priv = priv;
	priv->config = dvb_config;

	if(p->card < 1 || p->card > priv->config->count)
 	{
		free(priv);
		mp_msg(MSGT_DEMUX, MSGL_ERR, "NO CONFIGURATION FOUND FOR CARD N. %d, exit\n", p->card);
 		return STREAM_ERROR;
 	}
	priv->card = p->card - 1;
	
	tuner_type = priv->config->cards[priv->card].type;

	if(tuner_type == 0)
	{
		free(priv);
		mp_msg(MSGT_DEMUX, MSGL_V, "OPEN_DVB: UNKNOWN OR UNDETECTABLE TUNER TYPE, EXIT\n");
		return STREAM_ERROR;
	}


	priv->tuner_type = tuner_type;

	mp_msg(MSGT_DEMUX, MSGL_V, "OPEN_DVB: prog=%s, card=%d, type=%d, vid=%d, aid=%d\n",
		p->prog, priv->card+1, priv->tuner_type, p->vid, p->aid);

	priv->list = priv->config->cards[priv->card].list;
	
	if((! strcmp(p->prog, "")) && (priv->list != NULL))
		progname = priv->list->channels[0].name;
	else
		progname = p->prog;


	if(! dvb_streaming_start(priv, p, tuner_type, progname))
	{
		free(stream->priv);
		stream->priv = NULL;
		return STREAM_ERROR;
	}

	stream->type = STREAMTYPE_DVB;
	stream->fill_buffer = dvb_streaming_read;
	stream->close = dvbin_close;
	m_struct_free(&stream_opts, opts);

    return STREAM_OK;
}

#define MAX_CARDS 4
dvb_config_t *dvb_get_config()
{
	int i, fd, type, size;
	char filename[30], *conf_file, *name;
	dvb_channels_list *list;
	dvb_card_config_t *cards = NULL;
	dvb_config_t *conf = NULL;
	
	if(dvb_config != NULL)
		return dvb_config;
			
	conf = (dvb_config_t*) malloc(sizeof(dvb_config_t));
	if(conf == NULL)
		return NULL;

	conf->priv = NULL;	
	conf->count = 0;
	conf->cards = NULL;
	for(i=0; i<MAX_CARDS; i++)
	{
		sprintf(filename, "/dev/dvb/adapter%d/frontend0", i);
		fd = open(filename, O_RDWR | O_NONBLOCK);
		if(fd < 0)
		{
			mp_msg(MSGT_DEMUX, MSGL_V, "DVB_CONFIG, can't open device %s, skipping\n", filename);
			continue;
		}
			
		type = dvb_get_tuner_type(fd);
		close(fd);
		if(type != TUNER_SAT && type != TUNER_TER && type != TUNER_CBL)
		{
			mp_msg(MSGT_DEMUX, MSGL_V, "DVB_CONFIG, can't detect tuner type of card %d, skipping\n", i);
			continue;
		}
		
		switch(type)
			{
				case TUNER_TER:
				conf_file = get_path("channels.conf.ter");
					break;
				case TUNER_CBL:
				conf_file = get_path("channels.conf.cbl");
					break;
				case TUNER_SAT:
				conf_file = get_path("channels.conf.sat");
					break;
			}
		
		if((access(conf_file, F_OK | R_OK) != 0))
			conf_file = get_path("channels.conf");

		list = dvb_get_channels(conf_file, type);
		if(list == NULL)
			continue;
		
		size = sizeof(dvb_card_config_t) * (conf->count + 1);
		cards = realloc(conf->cards, size);

		if(cards == NULL)
	{
			fprintf(stderr, "DVB_CONFIG, can't realloc %d bytes, skipping\n", size);
			continue;
	}

		name = (char*) malloc(20);
		if(name==NULL)
	{
			fprintf(stderr, "DVB_CONFIG, can't realloc 20 bytes, skipping\n");
			continue;
	}

		conf->cards = cards;
		conf->cards[conf->count].devno = i;
		conf->cards[conf->count].list = list;
		conf->cards[conf->count].type = type;
		sprintf(name, "DVB-%c card n. %d", type==TUNER_TER ? 'T' : (type==TUNER_CBL ? 'C' : 'S'), conf->count+1);
		conf->cards[conf->count].name = name;
		conf->count++;
	}

	if(conf->count == 0)
	{
		free(conf);
		conf = NULL;
	}

	dvb_config = conf;
	return conf;
}



stream_info_t stream_info_dvb = {
	"Dvb Input",
	"dvbin",
	"Nico",
	"based on the code from ??? (probably Arpi)",
	dvb_open, 			
	{ "dvb", NULL },
	&stream_opts,
	1 				// Urls are an option string
};




