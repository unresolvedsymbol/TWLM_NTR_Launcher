/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <nds.h>
#include <fat.h>
#include <nds/fifocommon.h>
#include <nds/arm9/dldi.h>

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <list>

#include "inifile.h"
#include "nds_card.h"
#include "launch_engine.h"
#include "crc.h"

sNDSHeader ndsHeader;

u8 cheatData[0x8000] = {0};

off_t getFileSize(const char *fileName)
{
    FILE* fp = fopen(fileName, "rb");
    off_t fsize = 0;
    if (fp) {
        fseek(fp, 0, SEEK_END);
        fsize = ftell(fp);			// Get source file's size
		fseek(fp, 0, SEEK_SET);
	}
	fclose(fp);

	return fsize;
}


bool consoleOn = false;

int main() {

	bool consoleInited = false;
	bool scfgUnlock = false;
	bool TWLMODE = false;
	bool TWLCLK = false;	// false == NTR, true == TWL
	bool TWLVRAM = false;
	bool soundFreq = false;
	bool runCardEngine = false;
	bool EnableSD = false;
	int language = -1;

	// If slot is powered off, tell Arm7 slot power on is required.
	if (isDSiMode()) {
		if(REG_SCFG_MC == 0x11) { fifoSendValue32(FIFO_USER_02, 1); }
		if(REG_SCFG_MC == 0x10) { fifoSendValue32(FIFO_USER_02, 1); }
	}
	
	if (isDSiMode()) {
		if (fatInitDefault()) {
			CIniFile settingsini("/_nds/NTR_Launcher.ini");

			TWLMODE = settingsini.GetInt("NTRLAUNCHER","TWLMODE",0);
			TWLCLK = settingsini.GetInt("NTRLAUNCHER","TWLCLOCK",0);
			TWLVRAM = settingsini.GetInt("NTRLAUNCHER","TWLVRAM",0);
			soundFreq = settingsini.GetInt("NTRLAUNCHER","SOUNDFREQ",0);
			runCardEngine = settingsini.GetInt("NTRLAUNCHER","CARDENGINE",1);
			EnableSD = settingsini.GetInt("NTRLAUNCHER","SDACCESS",0);

			//if(settingsini.GetInt("NTRLAUNCHER","DEBUGMODE",0) == 1) {
			//	consoleOn = true;
			//	consoleDemoInit();
			//}

			if(!TWLCLK && !TWLMODE) {
				//if(settingsini.GetInt("NTRLAUNCHER","DEBUGMODE",0) == 1) {
				//	printf("TWL_CLOCK ON\n");		
				//}
				fifoSendValue32(FIFO_USER_04, 1);
				// Disabled for now. Doesn't result in correct SCFG_CLK configuration during testing. Will go back to old method.
				// setCpuClock(false);
				REG_SCFG_CLK = 0x80;
				swiWaitForVBlank();
			}

			//if(EnableSD) {
				//if(settingsini.GetInt("NTRLAUNCHER","DEBUGMODE",0) == 1) {
				//	printf("SD access ON\n");		
				//}
			//}

			scfgUnlock = settingsini.GetInt("NTRLAUNCHER","SCFGUNLOCK",0);

			if(settingsini.GetInt("NTRLAUNCHER","RESETSLOT1",1) == 1) {
				fifoSendValue32(FIFO_USER_02, 1);
				fifoSendValue32(FIFO_USER_07, 1);
			}

			language = settingsini.GetInt("NTRLAUNCHER", "LANGUAGE", -1);

			scanKeys();
			if (keysHeld() & KEY_A) {
				runCardEngine = !runCardEngine;
			}

		} else {
			fifoSendValue32(FIFO_USER_02, 1);
			fifoSendValue32(FIFO_USER_07, 1);
		}

		// Tell Arm7 it's ready for card reset (if card reset is nessecery)
		fifoSendValue32(FIFO_USER_01, 1);
		// Waits for Arm7 to finish card reset (if nessecery)
		fifoWaitValue32(FIFO_USER_03);

		// Wait for card to stablize before continuing
		for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }

		sysSetCardOwner (BUS_OWNER_ARM9);

		cardReadHeader((uint8*)&ndsHeader);

		for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
	} else {
		sysSetCardOwner (BUS_OWNER_ARM9);

		if (*(u32*)((u8*)io_dldi_data+0x64) & FEATURE_SLOT_NDS) {
			consoleDemoInit();
			consoleInited = true;
			printf ("Please remove your flashcard.\n");
			do {
				swiWaitForVBlank();
				cardReadHeader((uint8*)&ndsHeader);
			} while ((u32)ndsHeader.gameTitle != 0xffffffff);
			for (int i = 0; i < 60; i++) {
				swiWaitForVBlank();
			}
		}

		cardReadHeader((uint8*)&ndsHeader);

		if ((u32)ndsHeader.gameTitle == 0xffffffff) {
			if (!consoleInited) {
				consoleDemoInit();
				consoleInited = true;
			} else {
				consoleClear();
			}
			printf ("Insert a DS game.\n");
			do {
				swiWaitForVBlank();
				cardReadHeader((uint8*)&ndsHeader);
			} while ((u32)ndsHeader.gameTitle == 0xffffffff);
		}

		// Delay half a second for the DS card to stabilise
		for (int i = 0; i < 30; i++) {
			swiWaitForVBlank();
		}
	}

	while(1) {
		if(REG_SCFG_MC == 0x11) {
		break; } else {
			cheatData[3] = 0xCF;
			off_t wideCheatSize = getFileSize("sd:/_nds/nds-bootstrap/wideCheatData.bin");
			if (wideCheatSize > 0) {
				FILE* wideCheatFile = fopen("sd:/_nds/nds-bootstrap/wideCheatData.bin", "rb");
				fread(cheatData, 1, wideCheatSize, wideCheatFile);
				fclose(wideCheatFile);
				cheatData[wideCheatSize+3] = 0xCF;
			}
			memcpy((void*)0x023F0000, cheatData, 0x8000);
			runLaunchEngine (EnableSD, language, scfgUnlock, TWLMODE, TWLCLK, TWLVRAM, soundFreq, runCardEngine);
		}
	}
	return 0;
}

