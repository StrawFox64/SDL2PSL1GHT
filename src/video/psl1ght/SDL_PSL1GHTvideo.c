/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2010 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* PSL1GHT SDL video driver implementation (for PS3). Based on Dummy driver.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_PSL1GHTvideo.h"
#include "SDL_PSL1GHTevents_c.h"
#include "SDL_PSL1GHTrender_c.h"


#include <malloc.h>
#include <assert.h>

#include <rsx/reality.h>

#define PSL1GHTVID_DRIVER_NAME "psl1ght"

/* Initialization/Query functions */
static int PSL1GHT_VideoInit(_THIS);
static int PSL1GHT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void PSL1GHT_VideoQuit(_THIS);

/* PS3GUI init functions : */
static void initializeGPU(SDL_DeviceData * devdata);
static void setupScreenMode(SDL_DisplayData * dispdata);

/* PSL1GHT driver bootstrap functions */

static int
PSL1GHT_Available(void)
{
    return (1);
}

static void
PSL1GHT_DeleteDevice(SDL_VideoDevice * device)
{
    printf( "PSL1GHT_DeleteDevice( %16X)\n", device);
    SDL_free(device);
}

static SDL_VideoDevice *
PSL1GHT_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;
    printf( "PSL1GHT_CreateDevice( %16X)\n", devindex);

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (device) {
        SDL_memset(device, 0, (sizeof *device));
    }
    else {
        SDL_OutOfMemory();
        SDL_free(device);
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = PSL1GHT_VideoInit;
    device->VideoQuit = PSL1GHT_VideoQuit;
    device->SetDisplayMode = PSL1GHT_SetDisplayMode;
    device->PumpEvents = PSL1GHT_PumpEvents;

    device->free = PSL1GHT_DeleteDevice;

    return device;
}

VideoBootStrap PSL1GHT_bootstrap = {
    PSL1GHTVID_DRIVER_NAME, "SDL psl1ght video driver",
    PSL1GHT_Available, PSL1GHT_CreateDevice
};

int
PSL1GHT_VideoInit(_THIS)
{
    SDL_DisplayMode mode;
    SDL_DeviceData *devdata = NULL;
    SDL_DisplayData *didata = NULL;

    devdata = (SDL_DeviceData*) SDL_calloc(1, sizeof(SDL_DeviceData));
    if (devdata == NULL) { 
        /* memory allocation problem */  
        SDL_OutOfMemory();
        return -1;
    } 

    _this->driverdata = devdata;

    didata = (SDL_DisplayData *) SDL_calloc(1, sizeof(SDL_DisplayData));
    if (didata == NULL) { 
        /* memory allocation problem */  
        SDL_OutOfMemory();
        return -1;
    } 

    initializeGPU(devdata);
    setupScreenMode(didata);

    /* Replace by setScreenMode data  */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = didata->_resolution.width;
    mode.h = didata->_resolution.height;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }
    SDL_AddRenderDriver(&_this->displays[0], &SDL_PSL1GHT_RenderDriver);

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);
    _this->displays[0].driverdata = didata;

    /* We're done! */
    return 0;
}

static int
PSL1GHT_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    printf( "PSL1GHT_SetDisplayMode( )\n");
    return 0;
}

void
PSL1GHT_VideoQuit(_THIS)
{
    printf("PSL1GHT_VideoQuit()\n");
}

void initializeGPU( SDL_DeviceData * devdata)
{
    printf("initializeGPU()\n");
   // Allocate a 1Mb buffer, alligned to a 1Mb boundary to be our shared IO memory with the RSX.
    void *host_addr = memalign(1024*1024, 1024*1024);
    assert(host_addr != NULL);

    // Initilise Reality, which sets up the command buffer and shared IO memory
    devdata->_CommandBuffer = realityInit(0x10000, 1024*1024, host_addr);
    assert(devdata->_CommandBuffer != NULL);
}

void setupScreenMode( SDL_DisplayData * dispdata)
{
    printf("setupScreenMode\n");
    VideoState state;
    assert(videoGetState(0, 0, &state) == 0); // Get the state of the display
    assert(state.state == 0); // Make sure display is enabled

    // Get the current resolution
    assert(videoGetResolution(state.displayMode.resolution, &dispdata->_resolution) == 0);

    // Configure the buffer format to xRGB
    VideoConfiguration vconfig;
    memset(&vconfig, 0, sizeof(VideoConfiguration));
    vconfig.resolution = state.displayMode.resolution;
    vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
    vconfig.pitch = dispdata->_resolution.width * 4;

    assert(videoConfigure(0, &vconfig, NULL, 0) == 0);
    assert(videoGetState(0, 0, &state) == 0);

    gcmSetFlipMode(GCM_FLIP_VSYNC); // Wait for VSYNC to flip
}


/* vi: set ts=4 sw=4 expandtab: */
