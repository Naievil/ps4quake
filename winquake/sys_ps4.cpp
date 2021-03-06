/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_null.h -- null system driver to aid porting efforts

#include <orbis/libkernel.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "quakedef.h"
#include "errno.h"

qboolean			isDedicated;

char	*newargv[256];

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES             10
FILE    *sys_handles[MAX_HANDLES];

int             findhandle (void)
{
	int             i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
		if (!sys_handles[i])
			return i;
	Sys_Error ((char *)"out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (FILE *f)
{
	int             pos;
	int             end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	FILE    *f;
	int             i;
	
	i = findhandle ();

	f = fopen(path, "rb");
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	FILE    *f;
	int             i;
	
	i = findhandle ();

	f = fopen(path, "wb");
	if (!f)
		Sys_Error ((char *)"Error opening %s: %s", path,strerror(errno));
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, int position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{
	va_list         argptr;

	printf ("Sys_Error: ");   
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	printf("Setting Sys_Quit!\n");
	exit (0);
}

double Sys_FloatTime (void)
{
	double timer = (double)sceKernelGetProcessTimeCounter();
	return (timer/(1000*1000*1000));
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_SendKeyEvents (void)
{
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}


#ifdef __cplusplus
} // extern "C"
#endif

void Sleep (int ms)
{
	sceKernelUsleep(ms*1000);
}

int main (int argc, char **argv)
{

	quakeparms_t	parms;
	double			time, oldtime;
	static	char	cwd[1024];

	// naievil -- No buffering
    setvbuf(stdout, NULL, _IONBF, 0);

	printf ("Quake v%4.2f\n", VERSION);

	memset (&parms, 0, sizeof(parms));

	parms.memsize = 16*1024*1024;
	parms.membase = malloc (parms.memsize);


	parms.basedir = strdup("/app0/");
	printf("Basedir set to %s\n", parms.basedir);

	COM_InitArgv (argc, argv);

	// dedicated server ONLY!
	if (!COM_CheckParm ("-dedicated"))
	{
		memcpy (newargv, argv, argc*4);
		newargv[argc] = "-dedicated";
		argc++;
		argv = newargv;
		COM_InitArgv (argc, argv);
	}

	parms.argc = argc;
	parms.argv = argv;

	Host_Init (&parms);

	oldtime = Sys_FloatTime() -0.1;

    // main window message loop //
	while (1)
	{
		time = Sys_FloatTime();
		/*
		if (time - oldtime < sys_ticrate.value )
		{
			Sleep(1);
			continue;
		}
		*/

		Host_Frame ( time - oldtime );
		oldtime = time;
	}

	printf("!!! PLEASE CLOSE APPLICATION !!!\n");
    while (1) {}
    return 0;
}