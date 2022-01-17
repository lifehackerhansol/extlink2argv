/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include "main.h"
#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dswnifi_lib.h"
#include "keypadTGDS.h"
#include "TGDSLogoLZSSCompressed.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "biosTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dldi.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "consoleTGDS.h"
#include "soundTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"
#include "click_raw.h"
#include "ima_adpcm.h"
#include "linkerTGDS.h"
#include "dldi.h"
#include "utils.twl.h"

// Includes
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"
#include "WoopsiTemplate.h"

#include "limitsTGDS.h"

#include <cstdio>
#include <string>

#include "extlink.h"

#ifndef TGDSPROJECTNAME
#define TGDSPROJECTNAME "extlink2argv"
#endif

int fail(std::string debug){
	printf("extlink2argv boot fail:");
	printf(debug.c_str());
	printf("Press START to power off.");
	while(1) {
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
		scanKeys();
		int pressed = keysDown();
		if(pressed & KEY_START) break;
	}
	return 0;
}
//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool stopSoundStreamUser() {
	return stopSoundStream(_FileHandleVideo, _FileHandleAudio, &internalCodecType);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void closeSoundUser() {
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];
int TGDSProjectReturnFromLinkedModule() {
	return -1;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	printf("              ");
	printf("              ");
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();

	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else {
		return fail("FS Init error: " + ret);
	}
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	// actual code that is me
	TExtLinkBody extlink;
	FILE* f = fopen("0:/moonshl2/extlink.dat", "rb+");
	if (f == NULL) return fail("Extlink does not exist.");
	memset(&extlink, 0, sizeof(TExtLinkBody));
	fread(&extlink, 1, sizeof(TExtLinkBody), f);
	if(extlink.ID != ExtLinkBody_ID) {
		fclose(f);
		return fail("Extlink ID mismatch.");
	}
	fseek(f,0,SEEK_SET);
	fwrite("____", 1, 4, f);
	fflush(f);
	fclose(f);
	char target[768];
	ucs2tombs((unsigned char*)target, extlink.NDSFullPathFilenameUnicode, 768);
	std::string txtpath = "0:";
	for(u32 i = 0; i < strlen(target); i++){
		txtpath.push_back(target[i]);
	}
	txtpath = txtpath.erase(txtpath.size() - 4) + ".txt";
	printf(txtpath.c_str());
	f = fopen(txtpath.c_str(), "r");
	if (!f) return fail("Path text file does not exist.");
	ucs2tombs((unsigned char*)target, extlink.DataFullPathFilenameUnicode, 768);
	char bootstraptarget[770] = "0:";
	fgets(bootstraptarget + 2, 768, f);
	char *argarray[] {
		bootstraptarget,
		target
	};
	if(!(FileExists(argarray[0]))) return fail("Bootstrap target does not exist.");
	char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
	memset(thisArgv, 0, sizeof(thisArgv));
	strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
	strcpy(&thisArgv[1][0], argarray[0]);		//Arg1:	NDS Binary reloaded
	strcpy(&thisArgv[2][0], argarray[1]);		//Arg2: NDS Binary ARG0
	addARGV(3, (char*)&thisArgv);
	if(TGDSMultibootRunNDSPayload(argarray[0]) == false) return fail("NDS Launch fail."); //should never reach here, nor even return true. Should fail it returns false
}

