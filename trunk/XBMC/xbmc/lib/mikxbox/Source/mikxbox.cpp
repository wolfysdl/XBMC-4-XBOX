/**********************************************************************************************************************************************************	
*	MikMod sound library
*	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
*	complete list.
*
*	This library is free software; you can redistribute it and/or modify
*	it under the terms of the GNU Library General Public License as
*	published by the Free Software Foundation; either version 2 of
*	the License, or (at your option) any later version.
* 
*	This program is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU Library General Public License for more details.
* 
*	You should have received a copy of the GNU Library General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
*	02111-1307, USA.
*
*	mikxbox unofficial mikmod port. / XBOX PORT
*
***********************************************************************************************************************************************************
*   Last Revision : Rev 2.1 6/21/2003
*   PeteGabe <Peter Knepley> hacked this to work on XBOX
*
*   Rev 2.0 01/06/2002
*		Modified functions prototypes.
*		Added SFX management using a ring buffer.
*		Added compilation defines for all kind of supported tracker formats. (see mikmod.h)
*		Fixed bugs in libmikmod (probs with effects channels reinitialisation when module was wrapping)
*		Now modules are wrapping by default.
*		Ozzy <ozzy@orkysquad.org> -> http://www.orkysquad.org
*
*   Rev 1.2 23/08/2001
*		Modified DirectSound buffer management from double buffering to a 50Hz playing system.
*		This change prevent some rastertime overheads during the MikMod_update()..
*		Replay calls are now much more stable in this way! :)
*		Ozzy <ozzy@orkysquad.org> -> http://www.orkysquad.org
*		
*	Rev 1.1 15/02/1999	
*		first & nice DirectSound Double-buffered implementation.
*		J�rg Mensmann <joerg.mensmann@gmx.net>	
*
*
**TABULATION 4*******RESOLUTION : 1280*1024 ***************************************************************************************************************/
#include "xbsection_start.h"

#include <mikmod.h>
#include "mikmod_internals.h"
#include "mikwin.h"


/********************************************************************************************************************************************************** 

																	... mikxboxInit(...) ...

***********************************************************************************************************************************************************/
BOOL mikxboxInit()
{
	char *pInfos;
	BOOL initialised;

	/* register all the drivers */
	MikMod_RegisterDriver(&drv_xbox);

    /* register all the module loaders */
	MikMod_RegisterAllLoaders();

	/* initialize the library */
	md_mode = DMODE_HQMIXER | DMODE_INTERP | DMODE_STEREO;
    
	pInfos = MikMod_InfoDriver();
	initialised = !MikMod_Init(pInfos);

	if (initialised){
		MikMod_SetNumVoices(127,0);          
		MikMod_EnableOutput();
	}
	return initialised;
}
/********************************************************************************************************************************************************** 

																		... mikxboxExit(...) ...

***********************************************************************************************************************************************************/
void mikxboxExit()
{
	MikMod_Exit();
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetErrno(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetErrno(int errno)
{
	MikMod_errno = errno;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetErrno(...) ...

***********************************************************************************************************************************************************/
int		mikxboxGetErrno(void)
{
	return MikMod_errno;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetMasterVolume(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetMasterVolume(UBYTE vol)
{
	md_volume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetMasterVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikxboxGetMasterVolume(void)
{
	return md_volume;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetMusicVolume(...) ...

  note: when playing music + sfx that's better to use 75% volume onto music coz sfx volume is a bit low by default.

***********************************************************************************************************************************************************/
void	mikxboxSetMusicVolume(UBYTE vol)
{
	md_musicvolume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetMusicVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikxboxGetMusicVolume(void)
{
	return md_musicvolume;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetSfxVolume(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetSfxVolume(UBYTE vol)
{
	md_sndfxvolume = vol;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetSfxVolume(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikxboxGetSfxVolume(void)
{
	return md_sndfxvolume;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetMasterReverb(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetMasterReverb(UBYTE rev)
{
	md_reverb = rev;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetMasterReverb(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikxboxGetMasterReverb(void)
{
	return md_reverb;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetPanning(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetPanning(UBYTE pan)
{
	md_pansep = pan;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetPanning(...) ...

***********************************************************************************************************************************************************/
UBYTE	mikxboxGetPanning(void)
{
	return md_pansep;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetMasterDevice(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetMasterDevice(UWORD dev)
{
	md_device = dev;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetMasterDevice(...) ...

***********************************************************************************************************************************************************/
UWORD	mikxboxGetMasterDevice(void)
{
	return md_device;
}
/********************************************************************************************************************************************************** 

																	... mikxboxSetMixFrequency(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetMixFrequency(UWORD freq)
{
	md_mixfreq = freq;
}
/********************************************************************************************************************************************************** 

																	... mikxboxGetMixFrequency(...) ...

***********************************************************************************************************************************************************/
UWORD	mikxboxGetMixFrequency(void)
{
	return md_mixfreq;
}
/********************************************************************************************************************************************************** 

																		... mikxboxSetMode(...) ...

***********************************************************************************************************************************************************/
void	mikxboxSetMode(UWORD mode)
{
	md_mode = mode;
}
/********************************************************************************************************************************************************** 

																		... mikxboxGetMode(...) ...

***********************************************************************************************************************************************************/
UWORD	mikxboxGetMode(void)
{
	return md_mode;
}


__int64 mikxboxGetPTS()
{
	extern __int64 XB_GetPTS();
	return XB_GetPTS();
}

void mikxboxSetCallback(void (*p)(unsigned char*, int))
{
}