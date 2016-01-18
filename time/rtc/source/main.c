/*
	RTC example made by Aurelio Mannara for libctru
*/

#include <3ds.h>
#include <stdio.h>
#include <time.h>


const char* months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

const char* weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


const u16 daysAtStartOfMonthLUT[12] =
{
	0	%7, //januari		31
	31	%7, //februari		28+1(leap year)
	59	%7, //maart			31
	90	%7, //april			30
	120	%7, //mei			31
	151	%7, //juni			30
	181	%7, //juli			31
	212	%7, //augustus		31
	243	%7, //september		30
	273	%7, //oktober		31
	304	%7, //november		30
	334	%7  //december		31
};

#define isLeapYear(year) (((year)%4) == 0)

uint getDayOfWeek(uint day, uint month, uint year)
{
	//http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week

	day += 2*(3-((year/100)%4));
	year %= 100;
	day += year + (year/4);
	day += daysAtStartOfMonthLUT[month] - (isLeapYear(year) && (month <= 1));
	return day % 7;
}

int main(int argc, char **argv)
{
	// Initialize services
	gfxInitDefault();

	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	consoleInit(GFX_TOP, NULL);

	printf("\x1b[29;15HPress Start to exit.");

	// Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		//Print current time
		time_t unixTime = time(NULL);
		struct tm* timeStruct = gmtime((const time_t *)&unixTime);

		int hours = timeStruct->tm_hour;
		int minutes = timeStruct->tm_min;
		int seconds = timeStruct->tm_sec;
		int day = timeStruct->tm_mday;
		int month = timeStruct->tm_mon;
		int year = timeStruct->tm_year +1900;

		printf("\x1b[0;0H%02i:%02i:%02i", hours, minutes, seconds);
		printf("\n%s %s %i %i", weekDays[getDayOfWeek(day, month, year)], months[month], day, year);

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}

	// Exit services
	gfxExit();

	return 0;
}
