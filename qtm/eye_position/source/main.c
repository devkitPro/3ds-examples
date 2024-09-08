#include <string.h>
#include <stdio.h>
#include <3ds.h>
#include <math.h>

void printTrackingData(const QtmTrackingData *data)
{
	printf("Flags:                 %d, %d, %d, %d      \n", data->eyesTracked, data->faceDetected, data->eyesDetected, data->clamped);
	printf("Confidence level:      %.4f         \n\n", data->confidenceLevel);

	printf("Left eye X (cam):     % .4f        \n", data->eyeCameraCoordinates[QTM_EYE_LEFT][0]);
	printf("Left eye Y (cam):     % .4f        \n", data->eyeCameraCoordinates[QTM_EYE_LEFT][1]);
	printf("Right eye X (cam):    % .4f        \n", data->eyeCameraCoordinates[QTM_EYE_RIGHT][0]);
	printf("Right eye Y (cam):    % .4f        \n\n", data->eyeCameraCoordinates[QTM_EYE_RIGHT][1]);

	// Was previously using data - previous data dPitch, but these values were changing too fast
	// making this print look weird.
	printf("Pitch deviation:      % .4f        \n", data->dPitch);
	printf("Yaw deviation:        % .4f        \n", data->dYaw);
	printf("Roll deviation:       % .4f        \n\n", data->dRoll);
	//printf("Tick:                 %lld        \n\n", data->samplingTick);

	printf("Distance (approx):     %d cm        \n", (int)roundf(qtmEstimateEyeToCameraDistance(data) / 10.0f));
	printf("Head tilt angle:      % .4f deg       \n", qtmComputeHeadTiltAngle(data) * 180.0 / M_PI);

}

// Convert natural (x, y) coords into 3DS top screen pixel coordinates, taking screen rotation into account
static inline size_t getPixelOffset(int x, int y, size_t bpp)
{
	return bpp * (240*x + (240 - y));
}

static inline void writePixel24(void *fb, u32 color, int x, int y)
{
	memcpy((u8 *)fb + getPixelOffset(x, y, 3), &color, 3);
}

static void updateEyePixelMarkers(void *fb, const QtmTrackingData *data, const QtmTrackingData *prevData)
{
	// Put origin at center of screen, with camera space being 320x240 as usual
	// Y axis goes to the top, hence the minus signs

	// Left eye
	int x0L = (int)(200 + 160 * prevData->eyeCameraCoordinates[QTM_EYE_LEFT][0]);
	int y0L = (int)(120 - 160 * prevData->eyeCameraCoordinates[QTM_EYE_LEFT][1]);

	// Right eye
	int x0R = (int)(200 + 160 * prevData->eyeCameraCoordinates[QTM_EYE_RIGHT][0]);
	int y0R = (int)(120 - 160 * prevData->eyeCameraCoordinates[QTM_EYE_RIGHT][1]);

	// Replace old markers by black
	writePixel24(fb, 0x000000, x0L, y0L);
	writePixel24(fb, 0x000000, x0R, y0R);

	if (!data)
		return;

	// Left eye
	int x1L = (int)(200 + 160 *     data->eyeCameraCoordinates[QTM_EYE_LEFT][0]);
	int y1L = (int)(120 - 160 *     data->eyeCameraCoordinates[QTM_EYE_LEFT][1]);

	// Right eye
	int x1R = (int)(200 + 160 *     data->eyeCameraCoordinates[QTM_EYE_RIGHT][0]);
	int y1R = (int)(120 - 160 *     data->eyeCameraCoordinates[QTM_EYE_RIGHT][1]);

	// New left eye pixel marker: blue
	writePixel24(fb, 0x0000FF, x1L, y1L);

	// New right eye pixel marker: red
	writePixel24(fb, 0xFF0000, x1R, y1R);
}

static void drawBorderLine(void *fb, u32 color, int startX, int len)
{
	for (int i = 0; i < len; i++)
	{
		for (int j = 0; j < 240; j++)
			writePixel24(fb, color, startX + i, 240 - j);
	}
}

static void prepareFb(void *fb)
{
	// 320x240 camera resolution, means borders at left+40-1px, right-40+1px

	memset(fb, 0, 400*240*3);
	drawBorderLine(fb, 0xFFFFFF, 35, 4);
	drawBorderLine(fb, 0xFFFFFF, 400 - 40 + 1, 4);
}

int main(void)
{
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	consoleClear();

	prepareFb(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL));
	gfxSwapBuffers();
	prepareFb(gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL));

	gfxFlushBuffers();

	Result res = 0;

	if (qtmCheckServicesRegistered())
	{
		// Use "qtm:s" to get camera luminance. Otherwise "qtm:u" would have been fine
		res = qtmInit(QTM_SERVICE_SYSTEM);
		if (R_FAILED(res))
			printf("qtmInit(QTM_SERVICE_SYSTEM) failed with result %08lx\n", res);
	}

	bool qtmAvailable = true;
	if (qtmIsInitialized())
	{
		bool blacklisted = false;
		res = QTMU_IsCurrentAppBlacklisted(&blacklisted);
		if (R_FAILED(res))
			printf("QTMU_IsCurrentAppBlacklisted failed with result %08lx\n", res);
		else if (blacklisted)
			printf("QTM has been blacklisted for this application\n");
		qtmAvailable = !blacklisted;
	}

	if (!qtmAvailable)
		printf("QTM is not available.\n\nPress START to exit.");

	QtmTrackingData prevPrevTrackingData = {0};
	QtmTrackingData prevTrackingData = {0};
	QtmTrackingData curTrackingData = {0};

	float lux = 400.0f;

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		if(qtmAvailable)
		{
			QtmTrackingData tmp;
			if (!(kHeld & KEY_B))
			{
				res = QTMU_GetTrackingData(&tmp);
				if (R_SUCCEEDED(res))
				{
					prevPrevTrackingData = prevTrackingData;
					prevTrackingData = curTrackingData;
					curTrackingData = tmp;

					// Update markers as we have new data
					// Clear t-2 instead of t-1 markers because of double buffering
					updateEyePixelMarkers(fb, &curTrackingData, &prevPrevTrackingData);
				}
				else switch (res)
				{
					case 0xC8A183EFu:
						printf("QTM is unavailable\n");
						qtmAvailable = false;
						break;
					case 0xC8A18008u:
						printf("QTM is paused because camera is in use by user\n");
						qtmAvailable = false;
					default:
						// Should not happen
						printf("QTMU_GetTrackingData returned unknown err code %08lx\n", res);
						qtmAvailable = false;
						break;
				}
			}
			else
			{
				prevPrevTrackingData = prevTrackingData;
				prevTrackingData = curTrackingData;
				updateEyePixelMarkers(fb, &curTrackingData, &prevPrevTrackingData);
			}


			if (qtmAvailable && !(kHeld & KEY_B))
			{
				// Also update camera luminance (requires "qtm:s")
				QTMS_GetCameraLuminance(&lux);
			}

			if (qtmAvailable)
			{
				// Reset cursor
				printf("\x1b[H");
				printTrackingData(&curTrackingData);
				printf("\nCam luminance (lux):   %2f        \n", lux);
			}
		}

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	// Exit services
	if (qtmIsInitialized())
		qtmExit();

	gfxExit();
	return 0;
}

