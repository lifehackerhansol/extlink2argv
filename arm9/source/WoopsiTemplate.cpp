// Includes
#include "WoopsiTemplate.h"
#include "woopsiheaders.h"
#include "bitmapwrapper.h"
#include "bitmap.h"
#include "graphics.h"
#include "rect.h"
#include "gadgetstyle.h"
#include "fonts/newtopaz.h"
#include "woopsistring.h"
#include "colourpicker.h"
#include "filerequester.h"
#include "soundTGDS.h"
#include "main.h"
#include "posixHandleTGDS.h"
#include "keypadTGDS.h"

__attribute__((section(".dtcm")))
WoopsiTemplate * WoopsiTemplateProc = NULL;

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::startup(int argc, char **argv) {
	
	Rect rect;

	/** SuperBitmap preparation **/
	// Create bitmap for superbitmap
	Bitmap* superBitmapBitmap = new Bitmap(164, 191);

	// Get a graphics object from the bitmap so that we can modify it
	Graphics* gfx = superBitmapBitmap->newGraphics();

	// Clean up
	delete gfx;

	// Create screens
	_controlsScreen = new AmigaScreen(TGDSPROJECTNAME, Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_controlsScreen);
	_controlsScreen->setPermeable(true);

	// Add child windows
	_controlWindow = new AmigaWindow(0, 13, 256, 33, "Controls", Gadget::GADGET_DRAGGABLE, AmigaWindow::AMIGA_WINDOW_SHOW_DEPTH);
	_controlsScreen->addGadget(_controlWindow);

	// Controls
	_controlWindow->getClientRect(rect);

	_Index = new Button(rect.x, rect.y, 41, 16, "Index");	//_Index->disable();
	_Index->setRefcon(2);
	_controlWindow->addGadget(_Index);
	_Index->addGadgetEventHandler(this);
	
	_lastFile = new Button(rect.x + 41, rect.y, 17, 16, "<");
	_lastFile->setRefcon(3);
	_controlWindow->addGadget(_lastFile);
	_lastFile->addGadgetEventHandler(this);
	
	_nextFile = new Button(rect.x + 41 + 17, rect.y, 17, 16, ">");
	_nextFile->setRefcon(4);
	_controlWindow->addGadget(_nextFile);
	_nextFile->addGadgetEventHandler(this);
	
	_play = new Button(rect.x + 41 + 17 + 17, rect.y, 40, 16, "Play");
	_play->setRefcon(5);
	_controlWindow->addGadget(_play);
	_play->addGadgetEventHandler(this);
	
	_stop = new Button(rect.x + 41 + 17 + 17 + 40, rect.y, 40, 16, "Stop");
	_stop->setRefcon(6);
	_controlWindow->addGadget(_stop);
	_stop->addGadgetEventHandler(this);
	
	_upVolume = new Button(rect.x + 41 + 17 + 17 + 40 + 40, rect.y, 40, 16, "Vol.+");
	_upVolume->setRefcon(7);
	_controlWindow->addGadget(_upVolume);
	_upVolume->addGadgetEventHandler(this);
	
	_downVolume = new Button(rect.x + 41 + 17 + 17 + 40 + 40 + 40, rect.y, 40, 16, "Vol.-");
	_downVolume->setRefcon(8);
	_controlWindow->addGadget(_downVolume);
	_downVolume->addGadgetEventHandler(this);
	
	// Add File listing screen
	_fileScreen = new AmigaScreen("File List", Gadget::GADGET_DRAGGABLE, AmigaScreen::AMIGA_SCREEN_SHOW_DEPTH | AmigaScreen::AMIGA_SCREEN_SHOW_FLIP);
	woopsiApplication->addGadget(_fileScreen);
	_fileScreen->setPermeable(true);
	_fileScreen->flipToTopScreen();
	// Add screen background
	_fileScreen->insertGadget(new Gradient(0, SCREEN_TITLE_HEIGHT, 256, 192 - SCREEN_TITLE_HEIGHT, woopsiRGB(0, 31, 0), woopsiRGB(0, 0, 31)));
	
	// Create FileRequester
	_fileReq = new FileRequester(7, 10, 250, 150, "Files", "/", GADGET_DRAGGABLE | GADGET_DOUBLE_CLICKABLE);
	_fileReq->setRefcon(1);
	_fileScreen->addGadget(_fileReq);
	_fileReq->addGadgetEventHandler(this);
	currentFileRequesterIndex = 0;
	
	_MultiLineTextBoxLogger = NULL;	//destroyable TextBox
	
	ReportAvailableMem();
	
	enableDrawing();	// Ensure Woopsi can now draw itself
	redraw();			// Draw initial state
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::ReportAvailableMem() {
	Rect rect;
	_controlsScreen->getClientRect(rect);
	_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y + 60, 200, 70, "DS Hardware status\n...", Gadget::GADGET_DRAGGABLE, 5);	// y + 60 px = move the rectangle vertically from parent obj
	_controlsScreen->addGadget(_MultiLineTextBoxLogger);
	
	_MultiLineTextBoxLogger->removeText(0);
	_MultiLineTextBoxLogger->moveCursorToPosition(0);
	_MultiLineTextBoxLogger->appendText("Memory Status: ");
	_MultiLineTextBoxLogger->appendText("\n");
	
	char arrBuild[256+1];
	sprintf(arrBuild, "Available heap memory: %d", TGDSARM9MallocFreeMemory());
	_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::shutdown() {
	Woopsi::shutdown();
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::waitForAOrTouchScreenButtonMessage(MultiLineTextBox* thisLineTextBox, const WoopsiString& thisText) {
	thisLineTextBox->appendText(thisText);
	scanKeys();
	while((!(keysDown() & KEY_A)) && (!(keysDown() & KEY_TOUCH))){
		scanKeys();
	}
	scanKeys();
	while((keysDown() & KEY_A) && (keysDown() & KEY_TOUCH)){
		scanKeys();
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleValueChangeEvent(const GadgetEventArgs& e) {

	// Did a gadget fire this event?
	if (e.getSource() != NULL) {
	
		// Is the gadget the file requester?
		if (e.getSource()->getRefcon() == 1) {
			
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = ((FileRequester*)e.getSource())->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			
			//Boot .NDS file! (homebrew only)
			char tmpName[256];
			char ext[256];
			strcpy(tmpName, currentFileChosen);
			separateExtension(tmpName, ext);
			strlwr(ext);
			if(strncmp(ext,".bin", 4) == 0){
				int argCount = 2;	
				strcpy(&args[0][0], TGDSPROJECTNAME);	//Arg0: Parent TGDS Project name
				strcpy(&args[1][0], currentFileChosen);	//Arg1: self TGDS-LinkedModule filename
				
				int i = 0;
				for(i = 0; i < argCount; i++){
					argvs[i] = (char*)&args[i][0];
				}
				
				TGDSProjectRunLinkedModule(currentFileChosen, argCount, argvs, TGDSPROJECTNAME);
			}
			else{
				char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
				memset(thisArgv, 0, sizeof(thisArgv));
				strcpy(&thisArgv[0][0], currentFileChosen);	//Arg0:	NDS Binary loaded
				strcpy(&thisArgv[1][0], "");				//Arg1: ARGV0
				addARGV(2, (char*)&thisArgv);
				if(TGDSMultibootRunNDSPayload(currentFileChosen) == false){  //Should fail it returns false. (Audio track)
					pendPlay = 1;
				}
			}
			
			/* 
			//Destroyable Textbox implementation init
			Rect rect;
			_fileScreen->getClientRect(rect);
			_MultiLineTextBoxLogger = new MultiLineTextBox(rect.x, rect.y, 262, 170, "Loading\n...", Gadget::GADGET_DRAGGABLE, 5);
			_fileScreen->addGadget(_MultiLineTextBoxLogger);
			
			_MultiLineTextBoxLogger->removeText(0);
			_MultiLineTextBoxLogger->moveCursorToPosition(0);
			_MultiLineTextBoxLogger->appendText("File open OK: ");
			_MultiLineTextBoxLogger->appendText(strObj);
			_MultiLineTextBoxLogger->appendText("\n");
			_MultiLineTextBoxLogger->appendText("Please wait calculating CRC32... \n");
			
			char arrBuild[256+1];
			sprintf(arrBuild, "%s%x\n", "Invalid file: crc32 = 0x", crc32);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			sprintf(arrBuild, "%s%x\n", "Expected: crc32 = 0x", 0x5F35977E);
			_MultiLineTextBoxLogger->appendText(WoopsiString(arrBuild));
			
			waitForAOrTouchScreenButtonMessage(_MultiLineTextBoxLogger, "Press (A) or tap touchscreen to continue. \n");
			
			_MultiLineTextBoxLogger->invalidateVisibleRectCache();
			_fileScreen->eraseGadget(_MultiLineTextBoxLogger);
			_MultiLineTextBoxLogger->destroy();	//same as delete _MultiLineTextBoxLogger;
			//Destroyable Textbox implementation end
			*/
		}
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleLidClosed() {
	// Lid has just been closed
	_lidClosed = true;

	// Run lid closed on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidClose();
		i++;
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleLidOpen() {
	// Lid has just been opened
	_lidClosed = false;

	// Run lid opened on all gadgets
	s32 i = 0;
	while (i < _gadgets.size()) {
		_gadgets[i]->lidOpen();
		i++;
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void WoopsiTemplate::handleClickEvent(const GadgetEventArgs& e) {
	switch (e.getSource()->getRefcon()) {
		//_Index Event
		case 2:{
			//Get fileRequester size, if > 0, set the first element selected
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(0);
			}
			currentFileRequesterIndex = 0;
		}	
		break;
		
		//_lastFile Event
		case 3:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex > 0){
				currentFileRequesterIndex--;
			}
			if(freqListBox->getOptionCount() > 0){
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_nextFile Event
		case 4:{
			FileRequester * freqInst = _fileReq;
			FileListBox* freqListBox = freqInst->getInternalListBoxObject();
			if(currentFileRequesterIndex < (freqListBox->getOptionCount() - 1) ){
				currentFileRequesterIndex++;
				freqListBox->setSelectedIndex(currentFileRequesterIndex);
			}
		}	
		break;
		
		//_play Event
		case 5:{
			//Play WAV/ADPCM if selected from the FileRequester
			WoopsiString strObj = _fileReq->getSelectedOption()->getText();
			memset(currentFileChosen, 0, sizeof(currentFileChosen));
			strObj.copyToCharArray(currentFileChosen);
			pendPlay = 1;
		}	
		break;
		
		//_stop Event
		case 6:{
			pendPlay = 2;
		}	
		break;
		
		//_upVolume Event
		case 7:{
			struct touchPosition touch;
			XYReadScrPosUser(&touch);
			volumeUp(touch.px, touch.py);
			
		}	
		break;
		
		//_downVolume Event
		case 8:{
			struct touchPosition touch;
			XYReadScrPosUser(&touch);
			volumeDown(touch.px, touch.py);
		}	
		break;
		
	}
}

__attribute__((section(".dtcm")))
u32 pendPlay = 0;

char currentFileChosen[256+1];

//Called once Woopsi events are ended: TGDS Main Loop
__attribute__((section(".itcm")))
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void Woopsi::ApplicationMainLoop() {
	//Earlier.. main from Woopsi SDK.
	
	//Handle TGDS stuff...
	
	
	
	switch(pendPlay){
		case(1):{
			internalCodecType = playSoundStream(currentFileChosen, _FileHandleVideo, _FileHandleAudio);
			if(internalCodecType == SRC_NONE){
				//stop right now
				pendPlay = 2;
			}
			else{
				pendPlay = 0;
			}
			WoopsiTemplateProc->ReportAvailableMem();
		}
		break;
		case(2):{
			stopSoundStreamUser();
			pendPlay = 0;
		}
		break;
	}
}