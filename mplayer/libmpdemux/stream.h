#ifndef __STREAM_H
#define __STREAM_H

#include "mp_msg.h"
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#define STREAMTYPE_DUMMY -1    // for placeholders, when the actual reading is handled in the demuxer
#define STREAMTYPE_FILE 0      // read from seekable file
#define STREAMTYPE_VCD  1      // raw mode-2 CDROM reading, 2324 bytes/sector
#define STREAMTYPE_STREAM 2    // same as FILE but no seeking (for net/stdin)
#define STREAMTYPE_DVD  3      // libdvdread
#define STREAMTYPE_MEMORY  4   // read data from memory area
#define STREAMTYPE_PLAYLIST 6  // FIXME!!! same as STREAMTYPE_FILE now
#define STREAMTYPE_DS   8      // read from a demuxer stream
#define STREAMTYPE_DVDNAV 9    // we cannot safely "seek" in this...
#define STREAMTYPE_CDDA 10     // raw audio CD reader
#define STREAMTYPE_SMB 11      // smb:// url, using libsmbclient (samba)
#define STREAMTYPE_VCDBINCUE 12      // vcd directly from bin/cue files
#define STREAMTYPE_DVB 13

#ifdef _XBOX
#define STREAM_BUFFER_SIZE 16384
#else
#define STREAM_BUFFER_SIZE 2048
#endif

#define VCD_SECTOR_SIZE 2352
#define VCD_SECTOR_OFFS 24
#define VCD_SECTOR_DATA 2324

/// atm it will always use mode == STREAM_READ
/// streams that use the new api should check the mode at open
#define STREAM_READ  0
#define STREAM_WRITE 1
/// Seek flags, if not mannualy set and s->seek isn't NULL
/// STREAM_SEEK is automaticly set
#define STREAM_SEEK_BW  2
#define STREAM_SEEK_FW  4
#define STREAM_SEEK  (STREAM_SEEK_BW|STREAM_SEEK_FW)

//////////// Open return code
/// This can't open the requested protocol (used by stream wich have a
/// * protocol when they don't know the requested protocol)
#define STREAM_UNSUPORTED -1
#define STREAM_ERROR 0
#define STREAM_OK    1

#define MAX_STREAM_PROTOCOLS 10

#define STREAM_CTRL_RESET 0

#ifdef MPLAYER_NETWORK
#include "network.h"
#endif

struct stream_st;
typedef struct stream_info_st {
  const char *info;
  const char *name;
  const char *author;
  const char *comment;
  /// mode isn't used atm (ie always READ) but it shouldn't be ignored
  /// opts is at least in it's defaults settings and may have been
  /// altered by url parsing if enabled and the options string parsing.
  int (*open)(struct stream_st* st, int mode, void* opts, int* file_format);
  char* protocols[MAX_STREAM_PROTOCOLS];
  void* opts;
  int opts_url; /* If this is 1 we will parse the url as an option string
		 * too. Otherwise options are only parsed from the
		 * options string given to open_stream_plugin */
} stream_info_t;

typedef struct stream_st {
  // Read
  int (*fill_buffer)(struct stream_st *s, char* buffer, int max_len);
  // Write
  int (*write_buffer)(struct stream_st *s, char* buffer, int len);
  // Seek
  int (*seek)(struct stream_st *s,off_t pos);
  // Control
  // Will be later used to let streams like dvd and cdda report
  // their structure (ie tracks, chapters, etc)
  int (*control)(struct stream_st *s,int cmd,void* arg);
  // Close
  void (*close)(struct stream_st *s);

  int fd;   // file descriptor, see man open(2)
  int type; // see STREAMTYPE_*
  int flags;
  int sector_size; // sector size (seek will be aligned on this size if non 0)
  unsigned int buf_pos,buf_len;
  off_t pos,start_pos,end_pos;
  int eof;
  int mode; //STREAM_READ or STREAM_WRITE
  unsigned int cache_pid;
  void* cache_data;
  void* priv; // used for DVD, TV, RTSP etc
  char* url;  // strdup() of filename/url
#ifdef MPLAYER_NETWORK
  streaming_ctrl_t *streaming_ctrl;
#endif
  unsigned char buffer[STREAM_BUFFER_SIZE>VCD_SECTOR_SIZE?STREAM_BUFFER_SIZE:VCD_SECTOR_SIZE];
} stream_t;

#ifdef USE_STREAM_CACHE
int stream_enable_cache(stream_t *stream,int size,int min,int prefill);
int cache_stream_fill_buffer(stream_t *s);
int cache_stream_seek_long(stream_t *s,off_t pos);
#else
// no cache, define wrappers:
#define cache_stream_fill_buffer(x) stream_fill_buffer(x)
#define cache_stream_seek_long(x,y) stream_seek_long(x,y)
#define stream_enable_cache(x,y,z,w) 1
#endif
void fixup_network_stream_cache(stream_t *stream);

inline static int stream_read_char(stream_t *s){
  return (s->buf_pos<s->buf_len)?s->buffer[s->buf_pos++]:
    (cache_stream_fill_buffer(s)?s->buffer[s->buf_pos++]:-256);
//  if(s->buf_pos<s->buf_len) return s->buffer[s->buf_pos++];
//  stream_fill_buffer(s);
//  if(s->buf_pos<s->buf_len) return s->buffer[s->buf_pos++];
//  return 0; // EOF
}

inline static unsigned int stream_read_word(stream_t *s){
  int x,y;
  x=stream_read_char(s);
  y=stream_read_char(s);
  return (x<<8)|y;
}

inline static unsigned int stream_read_dword(stream_t *s){
  unsigned int y;
  y=stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  return y;
}

#define stream_read_fourcc stream_read_dword_le

inline static unsigned int stream_read_word_le(stream_t *s){
  int x,y;
  x=stream_read_char(s);
  y=stream_read_char(s);
  return (y<<8)|x;
}

inline static unsigned int stream_read_dword_le(stream_t *s){
  unsigned int y;
  y=stream_read_char(s);
  y|=stream_read_char(s)<<8;
  y|=stream_read_char(s)<<16;
  y|=stream_read_char(s)<<24;
  return y;
}

inline static uint64_t stream_read_qword(stream_t *s){
  uint64_t y;
  y = stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  return y;
}

inline static uint64_t stream_read_qword_le(stream_t *s){
  uint64_t y;
  y = stream_read_char(s);
  y|=stream_read_char(s)<<8;
  y|=stream_read_char(s)<<16;
  y|=stream_read_char(s)<<24;
  y|=(uint64_t)stream_read_char(s)<<32;
  y|=(uint64_t)stream_read_char(s)<<40;
  y|=(uint64_t)stream_read_char(s)<<48;
  y|=(uint64_t)stream_read_char(s)<<56;
  return y;
}

inline static unsigned int stream_read_int24(stream_t *s){
  unsigned int y;
  y = stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  y=(y<<8)|stream_read_char(s);
  return y;
}

inline static int stream_read(stream_t *s,char* mem,int total){
  int len=total;
  while(len>0){
    int x;
    x=s->buf_len-s->buf_pos;
    if(x==0){
      if(!cache_stream_fill_buffer(s)) return total-len; // EOF
      x=s->buf_len-s->buf_pos;
    }
    if(s->buf_pos>s->buf_len) mp_msg(MSGT_DEMUX, MSGL_WARN, "stream_read: WARNING! s->buf_pos>s->buf_len\n");
    if(x>len) x=len;
    memcpy(mem,&s->buffer[s->buf_pos],x);
    s->buf_pos+=x; mem+=x; len-=x;
  }
  return total;
}

inline static unsigned char* stream_read_line(stream_t *s,unsigned char* mem, int max) {
  int len;
  unsigned char* end,*ptr = mem;;
  do {
    len = s->buf_len-s->buf_pos;
    // try to fill the buffer
    if(len <= 0 &&
       (!cache_stream_fill_buffer(s) || 
        (len = s->buf_len-s->buf_pos) <= 0)) break;
    end = (unsigned char*) memchr((void*)(s->buffer+s->buf_pos),'\n',len);
    if(end) len = end - (s->buffer+s->buf_pos) + 1;
    if(len > 0 && max > 1) {
      int l = len > max-1 ? max-1 : len;
      memcpy(ptr,s->buffer+s->buf_pos,l);
      max -= l;
      ptr += l;
    }
    s->buf_pos += len;
  } while(!end);
  if(s->eof && ptr == mem) return NULL;
  if(max > 0) ptr[0] = 0;
  return mem;
}


inline static int stream_eof(stream_t *s){
  return s->eof;
}

inline static off_t stream_tell(stream_t *s){
  return s->pos+s->buf_pos-s->buf_len;
}

inline static int stream_skip(stream_t *s,off_t len){

  if(s->buf_pos+len < s->buf_len) { //internal buffer skip
    off_t x=s->buf_pos+len;
    if(x>=0){
      s->buf_pos=x;
      return 1;
    }
  }
  
  if(len>0 && len<2*STREAM_BUFFER_SIZE){ //read linearly
    while(len>0){
      int x=s->buf_len-s->buf_pos;
      if(x==0){
        if(!cache_stream_fill_buffer(s)) return 0; // EOF
        x=s->buf_len-s->buf_pos;
      }
      if(x>len) x=len;
      s->buf_pos+=x; len-=x;
    }
    return 1;
  }

  return cache_stream_seek_long(s,len+stream_tell(s));
}

inline static int stream_seek(stream_t *s,off_t pos){

  mp_dbg(MSGT_DEMUX, MSGL_DBG3, "seek to 0x%qX\n",(long long)pos);  
  return stream_skip(s, pos-stream_tell(s));
}

void stream_reset(stream_t *s);
stream_t* new_stream(int fd,int type);
void free_stream(stream_t *s);
stream_t* new_memory_stream(unsigned char* data,int len);
stream_t* open_stream(char* filename,char** options,int* file_format);
stream_t* open_stream_full(char* filename,int mode, char** options, int* file_format);

extern int dvd_title;
extern int dvd_chapter;
extern int dvd_last_chapter;
extern int dvd_angle;

extern char * audio_stream;

#ifdef USE_DVDNAV
#include "dvdnav_stream.h"
#endif

#ifdef USE_DVDREAD

#ifdef USE_MPDVDKIT
#if (USE_MPDVDKIT == 2)
#include "../libmpdvdkit2/dvd_reader.h"
#include "../libmpdvdkit2/ifo_types.h"
#include "../libmpdvdkit2/ifo_read.h"
#include "../libmpdvdkit2/nav_read.h"
#else
#include "../libmpdvdkit/dvd_reader.h"
#include "../libmpdvdkit/ifo_types.h"
#include "../libmpdvdkit/ifo_read.h"
#include "../libmpdvdkit/nav_read.h"
#endif
#else
#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_types.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>
#endif

typedef struct {
 int id; // 0 - 31 mpeg; 128 - 159 ac3; 160 - 191 pcm
 int language; 
 int type;
 int channels;
} stream_language_t;

typedef struct {
  dvd_reader_t *dvd;
  dvd_file_t *title;
  ifo_handle_t *vmg_file;
  tt_srpt_t *tt_srpt;
  ifo_handle_t *vts_file;
  vts_ptt_srpt_t *vts_ptt_srpt;
  pgc_t *cur_pgc;
//
  int cur_cell;
  int last_cell;
  int cur_pack;
  int cell_last_pack;
// Navi:
  int packs_left;
  dsi_t dsi_pack;
  int angle_seek;
// audio datas
  int nr_of_channels;
  stream_language_t audio_streams[32];
// subtitles
  int nr_of_subtitles;
  stream_language_t subtitles[32];
} dvd_priv_t;

int dvd_number_of_subs(stream_t *stream);
int dvd_lang_from_sid(stream_t *stream, int id);
int dvd_aid_from_lang(stream_t *stream, unsigned char* lang);
int dvd_sid_from_lang(stream_t *stream, unsigned char* lang);
int dvd_chapter_from_cell(dvd_priv_t *dvd,int title,int cell);

#endif
							    
#endif // __STREAM_H
