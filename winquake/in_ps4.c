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
// in_null.c -- for systems without a mouse

#include "quakedef.h"
#include <orbis/Pad.h>
#include <orbis/UserService.h>
int ps4pad;
int ps4userID;
OrbisPadData ps4padData;
int ps4prevButtonState;
int ps4buttonState;

void IN_Init (void)
{
	// Initialize the Pad library
    if (scePadInit() != 0)
    {
        Sys_Error("Failed to initialize pad library!\n");
    }

    // Get the user ID
	OrbisUserServiceInitializeParams param;
	param.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
	sceUserServiceInitialize(&param);
	sceUserServiceGetInitialUser(&ps4userID);

    // Open a handle for the controller
    ps4pad = scePadOpen(ps4userID, 0, 0, NULL);

    if (ps4pad < 0)
    {
        Sys_Error("Failed to open pad!");
    }    
}

void IN_Shutdown (void)
{
}

void setButtonState(int state)
{
	ps4prevButtonState = ps4buttonState;
	ps4buttonState = state;
}

typedef struct
{
	int		name;
	int		keynum;
} ps4keymap_t;


ps4keymap_t ps4keynames[] =
{
	{ORBIS_PAD_BUTTON_CROSS, K_ENTER},
	{ORBIS_PAD_BUTTON_CIRCLE, K_ESCAPE},
	{ORBIS_PAD_BUTTON_TRIANGLE, K_SPACE},
	{ORBIS_PAD_BUTTON_UP, K_UPARROW},
	{ORBIS_PAD_BUTTON_DOWN, K_DOWNARROW},
	{ORBIS_PAD_BUTTON_LEFT, K_LEFTARROW},
	{ORBIS_PAD_BUTTON_RIGHT, K_RIGHTARROW},

	{ORBIS_PAD_BUTTON_R1, K_ALT},
	{ORBIS_PAD_BUTTON_L2, K_CTRL},
	{ORBIS_PAD_BUTTON_L1, K_SHIFT},

	{ORBIS_PAD_BUTTON_R2, K_MOUSE1},

	{ORBIS_PAD_BUTTON_OPTIONS, K_PAUSE}
};

void IN_Commands (void)
{
	scePadReadState(ps4pad, &ps4padData);
	setButtonState(ps4padData.buttons);

	for (ps4keymap_t* kn=ps4keynames ; kn->name ; kn++)
	{
		if (!(ps4buttonState & kn->name)) {
			Key_Event (kn->keynum, false);
		} else {
			Key_Event (kn->keynum, true);
		}
	}

}

void IN_Move (usercmd_t *cmd)
{
}

