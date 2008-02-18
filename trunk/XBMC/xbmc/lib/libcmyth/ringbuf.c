/*
 *  Copyright (C) 2004-2006, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * ringbuf.c -    functions to handle operations on MythTV ringbuffers.  A
 *                MythTV Ringbuffer is the part of the backend that handles
 *                recording of live-tv for streaming to a MythTV frontend.
 *                This allows the watcher to do things like pause, rewind
 *                and so forth on live-tv.
 */
#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <mvp_refmem.h>
#include <cmyth.h>
#include <cmyth_local.h>
#include <string.h>

/*
 * cmyth_ringbuf_destroy(cmyth_ringbuf_t rb)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Clean up and free a ring buffer structure.  This should only be done
 * by the ref_release() code.  Everyone else should call
 * ref_release() because ring buffer structures are reference
 * counted.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_ringbuf_destroy(cmyth_ringbuf_t rb)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!rb) {
		return;
	}

	if (rb->ringbuf_url) {
		ref_release(rb->ringbuf_url);
	}
	if (rb->ringbuf_hostname) {
		ref_release(rb->ringbuf_hostname);
	}
}

/*
 * cmyth_ringbuf_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Allocate and initialize a ring buffer structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_ringbuf_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_ringbuf_t
 */
cmyth_ringbuf_t
cmyth_ringbuf_create(void)
{
	cmyth_ringbuf_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return NULL;
	}

	ret->conn_data = NULL;
	ret->ringbuf_url = NULL;
	ret->ringbuf_size = 0;
	ret->ringbuf_fill = 0;
	ret->file_pos = 0;
	ret->file_id = 0;
	ret->ringbuf_hostname = NULL;
	ret->ringbuf_port = 0;
	ref_set_destroy(ret, (ref_destroy_t)cmyth_ringbuf_destroy);
	return ret;
}

/*
 * cmyth_ringbuf_setup(cmyth_recorder_t old_rec)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Set up the ring buffer inside a recorder for use in playing live
 * tv.  The recorder is supplied.  This will be duplicated and
 * released, so the caller can re-use the same variable to hold the
 * return.  The new copy of the recorder will have a ringbuffer set up
 * within it.
 *
 * Return Value:
 *
 * Success: A pointer to a new recorder structure with a ringbuffer
 *
 * Faiure: NULL
 */
cmyth_recorder_t
cmyth_ringbuf_setup(cmyth_recorder_t rec)
{
	static const char service[]="rbuf://";
	cmyth_recorder_t new_rec = NULL;
	char *host = NULL;
	char *port = NULL;
	char *path = NULL;
	char tmp;

	int err, count;
	int r;
	long long size, fill;
	char msg[256];
	char url[1024];
	char buf[32];
	cmyth_conn_t control;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no recorder connection\n",
			  __FUNCTION__);
		return NULL;
	}

	control = rec->rec_conn;

	pthread_mutex_lock(&mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %u[]:[]SETUP_RING_BUFFER[]:[]0",
		 rec->rec_id);

	if ((err=cmyth_send_message(control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto out;
	}

	count = cmyth_rcv_length(control);

	if (control->conn_version >= 16) {
		r = cmyth_rcv_string(control, &err, buf, sizeof(buf)-1, count);
		count -= r;
	}
	r = cmyth_rcv_string(control, &err, url, sizeof(url)-1, count); 
	count -= r;

	if ((r=cmyth_rcv_long_long(control, &err, &size, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		goto out;
	}
	count -= r;

	if ((r=cmyth_rcv_long_long(control, &err, &fill, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		goto out;
	}

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s: url is: '%s'\n",
		  __FUNCTION__, url);
	path = url;
	if (strncmp(url, service, sizeof(service) - 1) == 0) {
		/*
		 * The URL starts with rbuf://.  The rest looks like
		 * <host>:<port>/<filename>.
		 */
		host = url + strlen(service);
		port = strchr(host, ':');
		if (!port) {
			/*
			 * This does not seem to be a proper URL, so just
			 * assume it is a filename, and get out.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: 1 port %s, host = %s\n",
				  __FUNCTION__, port, host);
			goto out;
		}
		port = port + 1;
		path = strchr(port, '/');
		if (!path) {
			/*
			 * This does not seem to be a proper URL, so just
			 * assume it is a filename, and get out.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s: no path\n",
				  __FUNCTION__);
			goto out;
		}
	}

	new_rec = cmyth_recorder_dup(rec);
	if (new_rec == NULL) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: cannot create recorder\n",
			  __FUNCTION__);
		goto out;
	}
	ref_release(rec);
        new_rec->rec_ring = cmyth_ringbuf_create();
        
	tmp = *(port - 1);
	*(port - 1) = '\0';
	new_rec->rec_ring->ringbuf_hostname = ref_strdup(host);
	*(port - 1) = tmp;
	tmp = *(path);
	*(path) = '\0';
	new_rec->rec_ring->ringbuf_port = atoi(port);
	*(path) = tmp;
	new_rec->rec_ring->ringbuf_url = ref_strdup(url);
	new_rec->rec_ring->ringbuf_size = size;
	new_rec->rec_ring->ringbuf_fill = fill;

    out:
	pthread_mutex_unlock(&mutex);

	return new_rec;
}

char *
cmyth_ringbuf_pathname(cmyth_recorder_t rec)
{
        return ref_hold(rec->rec_ring->ringbuf_url);
}

/*
 * cmyth_ringbuf_get_block(cmyth_recorder_t rec, char *buf, unsigned long len)
 * Scope: PUBLIC
 * Description
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 * Sucess: number of bytes read into buf
 * Failure: -1
 */
int
cmyth_ringbuf_get_block(cmyth_recorder_t rec, char *buf, unsigned long len)
{
	struct timeval tv;
	fd_set fds;

	if (rec == NULL)
		return -EINVAL;

	tv.tv_sec = 10;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(rec->rec_ring->conn_data->conn_fd, &fds);
	if (select((int)rec->rec_ring->conn_data->conn_fd+1,
		   NULL, &fds, NULL, &tv) == 0) {
		rec->rec_ring->conn_data->conn_hang = 1;
		return 0;
	} else {
		rec->rec_ring->conn_data->conn_hang = 0;
	}
	return recv(rec->rec_ring->conn_data->conn_fd, buf, len, 0);
}

int
cmyth_ringbuf_select(cmyth_recorder_t rec, struct timeval *timeout)
{
	fd_set fds;
	int ret;
	cmyth_socket_t fd;
	if (rec == NULL)
		return -EINVAL;

	fd = rec->rec_ring->conn_data->conn_fd;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	ret = select((int)fd+1, &fds, NULL, NULL, timeout);

	if (ret == 0)
		rec->rec_ring->conn_data->conn_hang = 1;
	else
		rec->rec_ring->conn_data->conn_hang = 0;

	return ret;
}

/*
 * cmyth_ringbuf_request_block(cmyth_ringbuf_t file, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int
cmyth_ringbuf_request_block(cmyth_recorder_t rec, unsigned long len)
{
	int err, count;
	int r;
	long c, ret;
	char msg[256];

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	pthread_mutex_lock(&mutex);

	if(len > (unsigned int)rec->rec_conn->conn_tcp_rcvbuf)
		len = (unsigned int)rec->rec_conn->conn_tcp_rcvbuf;

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %u[]:[]REQUEST_BLOCK_RINGBUF[]:[]%ld",
		 rec->rec_id, len);

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);
	if ((r=cmyth_rcv_long(rec->rec_conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	rec->rec_ring->file_pos += c;
	ret = c;

    out:
	pthread_mutex_unlock(&mutex);

	return ret;
}

/*
 * cmyth_ringbuf_seek(
 *                    cmyth_ringbuf_t file, long long offset, int whence)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	SEEK_SET
 *		The offset is set to offset bytes.
 *	SEEK_CUR
 *		The offset is set to the current position plus offset bytes.
 *	SEEK_END
 *		The offset is set to the size of the file minus offset bytes.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
long long
cmyth_ringbuf_seek(cmyth_recorder_t rec,
		   long long offset, int whence)
{
	char msg[128];
	int err;
	int count;
	long long c;
	long r;
	long long ret;
	cmyth_ringbuf_t ring;

	if (rec == NULL)
		return -EINVAL;

	ring = rec->rec_ring;

	if ((offset == 0) && (whence == SEEK_CUR))
		return ring->file_pos;

	pthread_mutex_lock(&mutex);

	snprintf(msg, sizeof(msg),
		 "QUERY_RECORDER %u[]:[]SEEK_RINGBUF[]:[]%ld[]:[]%ld[]:[]%d[]:[]%ld[]:[]%ld",
		 rec->rec_id,
		 (long)(offset >> 32),
		 (long)(offset & 0xffffffff),
		 whence,
		 (long)(ring->file_pos >> 32),
		 (long)(ring->file_pos & 0xffffffff));

	if ((err = cmyth_send_message(rec->rec_conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	count = cmyth_rcv_length(rec->rec_conn);
	if ((r=cmyth_rcv_long_long(rec->rec_conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto out;
	}

	switch (whence) {
	case SEEK_SET:
		ring->file_pos = offset;
		break;
	case SEEK_CUR:
		ring->file_pos += offset;
		break;
	case SEEK_END:
		ring->file_pos = ring->file_length - offset;
		break;
	}

	ret = ring->file_pos;

    out:
	pthread_mutex_unlock(&mutex);
	
	return ret;
}
