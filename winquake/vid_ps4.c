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
// vid_null.c -- null video driver to aid porting efforts

#include "quakedef.h"
#include "d_local.h"
#include <orbis/VideoOut.h>

viddef_t	vid;				// global video state

#define	BASEWIDTH	1280
#define	BASEHEIGHT	720
#define FRAMEDEPTH     4

byte	vid_buffer[BASEWIDTH*BASEHEIGHT];
short	zbuffer[BASEWIDTH*BASEHEIGHT];
byte	surfcache[16*1024*1024];

unsigned short	d_8to16table[256];
unsigned	d_8to24table[256];



/// ps4 start

int ps4video;
void *ps4videoMem;
OrbisKernelEqueue flipQueue;
size_t directMemAllocationSize;
off_t directMemOff;
uintptr_t videoMemSP;
uint32_t **frameBuffers;
int frameBufferSize;
OrbisVideoOutBufferAttribute attr;
int activeFrameBufferIdx;
int ps4frameID;

uint32_t RGB8_to_FBCOLOR(char r, char g, char b) {
	// Encode to 24-bit color
	return 0x80000000 + (r << 16) + (g << 8) + (b);
}

uint32_t *allocateDisplayMem(size_t size)
{
	// Essentially just bump allocation
	uint32_t *allocatedPtr = (uint32_t *)videoMemSP;
	videoMemSP += size;

	return allocatedPtr;
}

int allocateFrameBuffers(int num)
{
	// Allocate frame buffers array
	frameBuffers = malloc(sizeof(uint32_t *) * num);
	
	// Set the display buffers
	for(int i = 0; i < num; i++)
		frameBuffers[i] = allocateDisplayMem(frameBufferSize);

	// Set SRGB pixel format
	sceVideoOutSetBufferAttribute(&attr, 0x80000000, 1, 0, BASEWIDTH, BASEHEIGHT, BASEWIDTH);
	
	// Register the buffers to the video handle
	return (sceVideoOutRegisterBuffers(ps4video, 0, (void **)frameBuffers, num, &attr) == 0);
}

int allocateVideoMem(size_t size, int alignment)
{
	int rc;
	
	// Align the allocation size
	directMemAllocationSize = (size + alignment - 1) / alignment * alignment;
	
	// Allocate memory for display buffer
	rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), directMemAllocationSize, alignment, 3, &directMemOff);
	
	if(rc < 0)
	{
		directMemAllocationSize = 0;
		return false;
	}
	
	// Map the direct memory
	rc = sceKernelMapDirectMemory(&ps4videoMem, directMemAllocationSize, 0x33, 0, directMemOff, alignment);
	
	if(rc < 0)
	{
		sceKernelReleaseDirectMemory(directMemOff, directMemAllocationSize);
		
		directMemOff = 0;
		directMemAllocationSize = 0;
		
		return false;
	}
	
	// Set the stack pointer to the beginning of the buffer
	videoMemSP = (uintptr_t)ps4videoMem;
	return true;
}

int initFlipQueue()
{
	int rc = sceKernelCreateEqueue(&flipQueue, "homebrew flip queue");
	
	if(rc < 0)
		return 0;
		
	sceVideoOutAddFlipEvent(flipQueue, ps4video, 0);
	return 1;
}

void FrameBufferSwap()
{
	// Swap the frame buffer for some perf
	activeFrameBufferIdx = (activeFrameBufferIdx + 1) % 2;
}

void PS4DrawPixel(int x, int y, int r, int g, int b)
{
	// Get pixel location based on pitch
	int pixel = (y * BASEWIDTH) + x;
	
	// Encode to 24-bit color
	uint32_t encodedColor = RGB8_to_FBCOLOR(r, g, b);
	
	// Draw to the frame buffer
	((uint32_t *)frameBuffers[activeFrameBufferIdx])[pixel] = encodedColor;
}

void PS4DrawRectangle(int x, int y, int w, int h, int r, int g, int b)
{
	int xPos, yPos;
	
	// Draw row-by-row, column-by-column
	for(yPos = y; yPos < y + h; yPos++)
	{
		for(xPos = x; xPos < x + w; xPos++)
		{
			PS4DrawPixel(xPos, yPos, r, g, b);
		}
	}
}

int VID_Init_PS4(void) {
	int rc;
	
	ps4video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
	ps4videoMem = NULL;
	frameBufferSize = BASEWIDTH * BASEHEIGHT * FRAMEDEPTH;

	if (ps4video < 0)
	{
		printf("Failed to open a video out handle: %s\n", strerror(errno));
		return 0;
	}
	
	if(!initFlipQueue())
	{
		printf("Failed to initialize flip queue: %s\n", strerror(errno));
		return 0;
	}
	
	if(!allocateVideoMem(0xC000000, 0x200000))
	{
		printf("Failed to allocate video memory: %s\n", strerror(errno));
		return 0;
	}
	
	if(!allocateFrameBuffers(2))
	{
		printf("Failed to allocate frame buffers: %s\n", strerror(errno));
		return 0;
	}
	
	sceVideoOutSetFlipRate(ps4video, 0);
	return 1;
}

void SubmitFlip(int frameID)
{
	sceVideoOutSubmitFlip(ps4video, activeFrameBufferIdx, ORBIS_VIDEO_OUT_FLIP_VSYNC, frameID);
}

void FrameWait(int frameID)
{
	OrbisKernelEvent evt;
	int count;
	
	// If the video handle is not initialized, bail out. This is mostly a failsafe, this should never happen.
	if(ps4video == 0)
		return;
		
	for(;;)
	{
		OrbisVideoOutFlipStatus flipStatus;
		
		// Get the flip status and check the arg for the given frame ID
		sceVideoOutGetFlipStatus(ps4video, &flipStatus);
		
		if(flipStatus.flipArg == frameID)
			break;
			
		// Wait on next flip event
		if(sceKernelWaitEqueue(flipQueue, &evt, 1, &count, 0) != 0)
			break;
	}
}

void VID_Update_PS4(int frameID) {
    // Submit the frame buffer
    SubmitFlip(frameID);
    FrameWait(frameID);

    // Swap to the next buffer
    FrameBufferSwap();
}

// ps4 end




void	VID_SetPalette (unsigned char *palette)
{
	int i;
	unsigned char *pal = palette;
	unsigned *table = d_8to24table;
	unsigned r, g, b;
	for(i=0; i<256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		table[0] = RGB8_to_FBCOLOR(r, g, b);
		table++;
		pal += 3;
	}
}

void	VID_ShiftPalette (unsigned char *palette)
{
	VID_SetPalette(palette);
}

void	VID_Init (unsigned char *palette)
{
	printf("VID_Init\n");
	if (VID_Init_PS4() != 1) {
		Sys_Error("FAILED TO INITIALIZE PS4 VIDEO BUFFERS\n");
	} else {
		printf("VID_Init_PS4 successful!\n");
	}
	vid.maxwarpwidth = vid.width = vid.conwidth = BASEWIDTH;
	vid.maxwarpheight = vid.height = vid.conheight = BASEHEIGHT;
	vid.aspect = 1.0;
	vid.numpages = 1;
	vid.colormap = host_colormap;
	vid.fullbright = 256 - LittleLong (*((int *)vid.colormap + 2048));
	vid.buffer = vid.conbuffer = vid_buffer;
	vid.rowbytes = vid.conrowbytes = BASEWIDTH;
	VID_SetPalette(palette);

	d_pzbuffer = zbuffer;
	D_InitCaches (surfcache, sizeof(surfcache));

	ps4frameID = 0;
}

void	VID_Shutdown (void)
{
}

void	VID_Update (vrect_t *rects)
{
	int x,y;
	for(x = rects->x; x < rects->width; x++){
		for(y = rects->x; y < rects->height; y++){
			frameBuffers[activeFrameBufferIdx][((y * BASEWIDTH) + x)] = d_8to24table[vid_buffer[y*BASEWIDTH + x]];
		}
	}

	//PS4DrawRectangle(32, 32, 200, 200, 128, 128, 128);
	VID_Update_PS4(ps4frameID);
}

/*
================
D_BeginDirectRect
================
*/
void D_BeginDirectRect (int x, int y, byte *pbitmap, int width, int height)
{
}


/*
================
D_EndDirectRect
================
*/
void D_EndDirectRect (int x, int y, int width, int height)
{
}


