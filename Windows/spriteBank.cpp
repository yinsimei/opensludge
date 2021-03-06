#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "winterfa.h"
#include "moreio.h"
#include "messbox.h"
#include "wincomp.h"
#include "Interface.h"
//#include "registry.h"
#include "splitter.hpp"
#include "sprites.h"
//#include "fontify.h"

HWND				mainWin = NULL;
HINSTANCE			inst;
HMENU				myMenu;
HCURSOR				grabCursor, normalCursor, busyCursor;

char				* loader;
extern int			mouseX, mouseY;
int currentSpriteNum = 0;
BOOL				changed = FALSE, addColours = FALSE, dragging = FALSE;

static OPENFILENAME ofn;
static char path[MAX_PATH];
char * loadedFile;

spriteBank	loadedSprites;
int			SPRITE_AREA_MX, SPRITE_AREA_MY;

/* Messaging macros */
#define MESS(id,m,w,l) SendDlgItemMessage(mainWin,id,m,(WPARAM)w,(LPARAM)l)

void fixExtension (char * buff, char * ext);

bool addComment(int e, char const * a, char const * b, char const * c) {
    return false;
}

void setChanged (BOOL on) {
	EnableWindow (GetDlgItem (mainWin, ID_PROJECT_SAVE), on);
	EnableMenuItem (myMenu, ID_PROJECT_SAVE, on ? MF_ENABLED : MF_GRAYED);
	changed = on;
}

void updateSpriteNum () {
	BOOL allowButtons = currentSpriteNum < loadedSprites.total;
	BOOL allowSaveSprite = FALSE;

	SetDlgItemInt (mainWin, ID_SPRITENUM, currentSpriteNum, TRUE);

	if (allowButtons) {
		char buff[20];	// 11 should be big enough, but in case of errors...
		sprintf (buff, "%i x %i", loadedSprites.sprites[currentSpriteNum].width, loadedSprites.sprites[currentSpriteNum].height);
		SetDlgItemText (mainWin, ID_SPRITESIZE, buff);
		sprintf (buff, "(%i, %i)", loadedSprites.sprites[currentSpriteNum].xhot, loadedSprites.sprites[currentSpriteNum].yhot);
		SetDlgItemText (mainWin, ID_SPRITECENTRE, buff);
		allowSaveSprite = (loadedSprites.sprites[currentSpriteNum].width) && (loadedSprites.sprites[currentSpriteNum].height);
	} else {
		SetDlgItemText (mainWin, ID_SPRITESIZE, "No sprite");
		SetDlgItemText (mainWin, ID_SPRITECENTRE, "No sprite");
	}
	EnableWindow (GetDlgItem (mainWin, ID_SPRITELAST), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITEFORWARD), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITEDELETE), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITESAVE), allowSaveSprite);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITELOAD), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_MOVEHOTSPOT), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITETRIM), allowButtons);
//	EnableWindow (GetDlgItem (mainWin, ID_FIXAXIS), allowButtons);
	EnableMenuItem (myMenu, ID_SPRITESAVE, allowSaveSprite ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem (myMenu, ID_SPRITELOAD, allowButtons ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem (myMenu, ID_SPRITEDELETE, allowButtons ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem (myMenu, ID_MOVEHOTSPOT, allowButtons ? MF_ENABLED : MF_GRAYED);
	EnableMenuItem (myMenu, ID_SPRITETRIM, allowButtons ? MF_ENABLED : MF_GRAYED);

	allowButtons = (currentSpriteNum > 0);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITEFIRST), allowButtons);
	EnableWindow (GetDlgItem (mainWin, ID_SPRITEBACK), allowButtons);
}

void updateData () {
	SetDlgItemText (mainWin, ID_LOADED_FILE, loader ? loader : "New sprite bank");
	SetDlgItemInt (mainWin, ID_NUM_SPRITES, loadedSprites.total, FALSE);
//	SetDlgItemInt (mainWin, ID_NUM_COLOURS, loadedSprites.myPaletteNum, FALSE);
	updateSpriteNum ();
}

/*
BOOL saveProject (BOOL overwrite) {
	if (loader && overwrite) {
		if (saveSpriteBank (loader)) {
			setChanged (FALSE);
			return TRUE;
		}
		return FALSE;
	}

	char file[MAX_PATH]="";
	ofn.lpstrFilter = "SLUDGE sprite banks (*.DUC)\0*.duc\0\0";
	ofn.lpstrFile = file;
	ofn.lpstrTitle = "Save a SLUDGE sprite bank...";
	if (GetSaveFileName(&ofn)) {
		memcpy(path,file,ofn.nFileOffset);
		path[ofn.nFileOffset-1]=0;
		fixExtension (file, "duc");
		if (saveSpriteBank (file)) {
			delete loader;
			loader = joinStrings (file, "");
			updateData ();
			setChanged (FALSE);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL continueEvenIfChanged () {
	if (changed) {
		switch (MessageBox (mainWin, "Do you want to save your changes to this sprite bank first?", "SLUDGE Sprite Bank Editor", MB_YESNOCANCEL)) {
			case IDYES:
			return saveProject (TRUE);

			case IDCANCEL:
			return FALSE;
		}
	}
	return TRUE;
}

LRESULT CALLBACK paletteDialogFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case WM_INITDIALOG:
		SetDlgItemInt (hDlg, ID_NUM_COLOURS, loadedSprites.myPaletteNum, FALSE);
		SetDlgItemInt (hDlg, ID_COLOUR_OFFSET, loadedSprites.myPaletteStart, FALSE);
		return TRUE;

		case WM_PAINT:
		paintPal (hDlg);
		break;

		case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
			case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return TRUE;

			case ID_SORTBRIGHT:
			SetCursor (busyCursor);
			sortColours (hDlg);
			updateData ();
			setChanged (TRUE);
			SetCursor (normalCursor);
			return TRUE;

			case ID_REMOVEUNUSED:
			optimizePalette ();
			SetDlgItemInt (hDlg, ID_NUM_COLOURS, loadedSprites.myPaletteNum, FALSE);
			SetDlgItemInt (hDlg, ID_COLOUR_OFFSET, loadedSprites.myPaletteStart, FALSE);
			RedrawWindow (hDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
			paintPal (hDlg);
			updateData ();
			setChanged (TRUE);
			return TRUE;
		}
	}

    return FALSE;
}

LRESULT CALLBACK hotspotDialogFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
        case WM_INITDIALOG:
		SetDlgItemInt (hDlg, ID_X, loadedSprites.sprites[currentSpriteNum].xhot, FALSE);
		SetDlgItemInt (hDlg, ID_Y, loadedSprites.sprites[currentSpriteNum].yhot, FALSE);
		return TRUE;

		case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
			{
				BOOL worked;
				int newX = GetDlgItemInt (hDlg, ID_X, & worked, TRUE);
				if (worked) {
					int newY = GetDlgItemInt (hDlg, ID_Y, & worked, TRUE);
					if (worked) {
						loadedSprites.sprites[currentSpriteNum].xhot = newX;
						loadedSprites.sprites[currentSpriteNum].yhot = newY;
						pasteSprite (currentSpriteNum);
						paintSpriteArea ();
						updateSpriteNum ();
						setChanged (TRUE);
						return TRUE;
					}
				}
				errorBox ("Can't apply changes", "Values must be integers in the range 0 to 255.");
			}
			return TRUE;

			case ID_HOT_CENTRE:
			loadedSprites.sprites[currentSpriteNum].xhot = loadedSprites.sprites[currentSpriteNum].width >> 1;
			loadedSprites.sprites[currentSpriteNum].yhot = loadedSprites.sprites[currentSpriteNum].height >> 1;
			pasteSprite (currentSpriteNum);
			paintSpriteArea ();
			updateSpriteNum ();
			SetDlgItemInt (hDlg, ID_X, loadedSprites.sprites[currentSpriteNum].xhot, FALSE);
			SetDlgItemInt (hDlg, ID_Y, loadedSprites.sprites[currentSpriteNum].yhot, FALSE);
			setChanged (TRUE);
			return TRUE;

			case ID_HOT_BASE:
			loadedSprites.sprites[currentSpriteNum].xhot = loadedSprites.sprites[currentSpriteNum].width >> 1;
			loadedSprites.sprites[currentSpriteNum].yhot = loadedSprites.sprites[currentSpriteNum].height ? loadedSprites.sprites[currentSpriteNum].height - 1 : 0;
			pasteSprite (currentSpriteNum);
			paintSpriteArea ();
			updateSpriteNum ();
			SetDlgItemInt (hDlg, ID_X, loadedSprites.sprites[currentSpriteNum].xhot, FALSE);
			SetDlgItemInt (hDlg, ID_Y, loadedSprites.sprites[currentSpriteNum].yhot, FALSE);
			setChanged (TRUE);
			return TRUE;

			case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return TRUE;
		}
	}

    return FALSE;
}

void centreMouseOverArea () {
	POINT w;

	w.x = SPRITE_AREA_X + (SPRITE_AREA_W / 2);
	w.y = SPRITE_AREA_Y + (SPRITE_AREA_H / 2);

	if (ClientToScreen (mainWin, & w)) SetCursorPos (w.x, w.y);
}
*/
BOOL APIENTRY dialogproc (HWND h, UINT m, WPARAM w, LPARAM lParam) {
	switch (m) {

		case WM_COMMAND:
		switch (LOWORD(w)) {
			case ID_VIEWPAL:
			//DialogBox(inst, MAKEINTRESOURCE(600), h, (DLGPROC) paletteDialogFunc);
			break;

			case ID_MOVEHOTSPOT:
			//DialogBox(inst, MAKEINTRESOURCE(700), h, (DLGPROC) hotspotDialogFunc);
			break;

			case ID_SPRITEINSERTEMPTY:
			//insertSprite (currentSpriteNum);
			break;
/*
			case ID_SPRITEINSERT:
			case ID_SPRITELOAD:
			{
				char file[MAX_PATH]="";
				ofn.lpstrFilter="TGA image files (*.TGA)\0*.tga\0\0";
				ofn.lpstrFile=file;
				ofn.lpstrTitle = "Choose an image to load...";
				if (GetOpenFileName(&ofn)) {
					memcpy(path,file,ofn.nFileOffset);
					path[ofn.nFileOffset-1]=0;
					if (LOWORD(w) == ID_SPRITEINSERT) {
						insertSprite (currentSpriteNum);
						if (loadSpriteFromTGA (file, currentSpriteNum)) {
							pasteSprite (currentSpriteNum);
							paintSpriteArea ();
							updateData ();
							setChanged (TRUE);
							DialogBox(inst, MAKEINTRESOURCE(700), h, (DLGPROC) hotspotDialogFunc);
						} else {
							deleteSprite (currentSpriteNum);
						}
					} else {
						if (loadSpriteFromTGA (file, currentSpriteNum)) {
							pasteSprite (currentSpriteNum);
							paintSpriteArea ();
							updateData ();
							setChanged (TRUE);
							DialogBox(inst, MAKEINTRESOURCE(700), h, (DLGPROC) hotspotDialogFunc);
						}
					}
				}
			}
			break;

			case ID_SPRITESAVE:
			{
				char file[MAX_PATH]="";
				ofn.lpstrFilter="TGA image files (*.TGA)\0*.tga\0\0";
				ofn.lpstrFile=file;
				ofn.lpstrTitle = "Save a sprite to an image file...";
				if (GetSaveFileName(&ofn)) {
					memcpy(path,file,ofn.nFileOffset);
					path[ofn.nFileOffset-1]=0;
					fixExtension (file, "tga");
					saveSpriteToTGA (file, currentSpriteNum);
				}
			}
			break;
*/
			case IDCANCEL:
			/*if (continueEvenIfChanged ()) */PostQuitMessage (0);
			break;
/*
			case ID_ADDCOLOURS:
			addColours = IsDlgButtonChecked (mainWin, ID_ADDCOLOURS) != BST_CHECKED;
			CheckDlgButton (mainWin, ID_ADDCOLOURS, addColours ? BST_CHECKED : BST_UNCHECKED);
			break;

			case ID_PROJECT_NEW:
			if (continueEvenIfChanged ()) {
				forgetSpriteBank ();
				delete loader;
				loader = NULL;
				currentSpriteNum = 0;
				updateData ();
				pasteSprite (currentSpriteNum);
				paintSpriteArea ();
				setChanged (FALSE);
			}
			break;

			case ID_FONTIFY:
			if (continueEvenIfChanged ()) {
				char file[MAX_PATH]="";
				ofn.lpstrFilter="TGA image files (*.TGA)\0*.tga\0\0";
				ofn.lpstrFile=file;
				ofn.lpstrTitle = "Choose an image to fontify...";
				if (GetOpenFileName(&ofn)) {
					memcpy(path,file,ofn.nFileOffset);
					path[ofn.nFileOffset-1]=0;
					forgetSpriteBank ();
					delete loader;
					loader = NULL;
					currentSpriteNum = 0;
					updateData ();
					pasteSprite (currentSpriteNum);
					paintSpriteArea ();
					fontify (file, inst);
				}
			}
			break;

			case ID_SPRITEFORWARD:
			if (currentSpriteNum < loadedSprites.total) {
				pasteSprite (++ currentSpriteNum);
				paintSpriteArea ();
				updateSpriteNum ();
			}
			break;

			case ID_SPRITEBACK:
			if (currentSpriteNum) {
				pasteSprite (-- currentSpriteNum);
				paintSpriteArea ();
				updateSpriteNum ();
			}
			break;

			case ID_SPRITEFIRST:
			currentSpriteNum = 0;
			pasteSprite (0);
			paintSpriteArea ();
			updateSpriteNum ();
			break;

			case ID_SPRITELAST:
			currentSpriteNum = loadedSprites.total;
			pasteSprite (currentSpriteNum);
			paintSpriteArea ();
			updateSpriteNum ();
			break;

			case ID_SPRITEDELETE:
			if (currentSpriteNum < loadedSprites.total) {
				deleteSprite (currentSpriteNum);
				pasteSprite (currentSpriteNum);
				paintSpriteArea ();
				updateData ();
				setChanged (TRUE);
			}
			break;

			case ID_BANKTRIM:
			{
				int trimmedTotal = FALSE;
				for (int a = 0; a < loadedSprites.total; a ++) {
					trimmedTotal += trimSprite (a);
				}
				if (trimmedTotal) {
					char comment[100];
					sprintf (comment, "Trimmed %i sprites.", trimmedTotal);
					messageBox (er, comment);
					pasteSprite (currentSpriteNum);
					paintSpriteArea ();
					updateData ();
					setChanged (TRUE);
				} else {
					messageBox (er, "No sprites require trimming.");
				}
			}
			break;

			case ID_SPRITETRIM:
			if (currentSpriteNum < loadedSprites.total) {
				if (trimSprite (currentSpriteNum)) {
					pasteSprite (currentSpriteNum);
					paintSpriteArea ();
					updateData ();
					setChanged (TRUE);
				}
			}
			break;

			case ID_FIXAXIS:
			spriteFit (currentSpriteNum);
			pasteSprite (currentSpriteNum);
			paintSpriteArea ();
			break;

			case ID_PROJECT_LOAD:
			if (continueEvenIfChanged ()) {
				char file[MAX_PATH]="";
				ofn.lpstrFilter = "SLUDGE sprite banks (*.DUC)\0*.duc\0\0";
				ofn.lpstrFile = file;
				ofn.lpstrTitle = "Load a SLUDGE sprite bank...";
				if (GetOpenFileName(&ofn)) {
					memcpy(path,file,ofn.nFileOffset);
					path[ofn.nFileOffset-1]=0;
					delete loader;
					if (loadSpriteBank (file)) {
						loader = joinStrings (file, "");
					} else {
						loader = NULL;
					}
					currentSpriteNum = 0;
					updateData ();
					pasteSprite (currentSpriteNum);
					paintSpriteArea ();
					setChanged (FALSE);
				}
			}
			break;

			case ID_PROJECT_SAVE:
			saveProject (TRUE);
			break;

			case ID_PROJECT_SAVEAS:
			saveProject (FALSE);
			break;
			*/
		}
		break;

		case WM_INITDIALOG:
		myMenu = LoadMenu (inst, MAKEINTRESOURCE(1));
		SetMenu (h, myMenu);
		mainWin = h;
		SetClassLong(h, GCL_HICON, (LONG) LoadIcon(inst, MAKEINTRESOURCE(3)));
		GetCurrentDirectory(MAX_PATH,path);
		memset (& ofn, 0, sizeof (ofn));
		ofn.lStructSize = sizeof (ofn);
		ofn.hwndOwner = h;
		ofn.hInstance = inst;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrInitialDir = path;
		ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;

/*		initSpriteBank ();
		if (loader[0]) {
			loadSpriteBank (loader);
			loader = joinStrings (loader, "");
		} else {
			loader = NULL;
		}*/
		updateData ();
//		pasteSprite (currentSpriteNum);
//		paintSpriteArea ();
		setChanged (FALSE);
		return 1;

		case WM_MOUSEMOVE:
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);
		if (dragging) {
			SPRITE_AREA_MX += mouseX - (SPRITE_AREA_X + (SPRITE_AREA_W / 2));
			SPRITE_AREA_MY += mouseY - (SPRITE_AREA_Y + (SPRITE_AREA_H / 2));
//			centreMouseOverArea ();
//			pasteSprite (currentSpriteNum);
//			paintSpriteArea ();
		}
		break;

		case WM_LBUTTONUP:
		if (dragging) {
			ShowCursor (TRUE);
			dragging = FALSE;
		}
		break;

		case WM_LBUTTONDOWN:
		if (mouseX >= SPRITE_AREA_X						&&
			mouseY >= SPRITE_AREA_Y						&&
			mouseX <  SPRITE_AREA_X + SPRITE_AREA_W		&&
			mouseY <  SPRITE_AREA_Y + SPRITE_AREA_H		&& ! dragging) {
			dragging = TRUE;
//			centreMouseOverArea ();
			ShowCursor (FALSE);
		}
		break;

		case WM_PAINT:
//		paintSpriteArea ();
		return 0;

		case WM_DESTROY:
//		WinHelp (mainWin, helpFile, HELP_QUIT, 0);
		PostQuitMessage(0);
		return 1;
	}
//	#pragma unused(l)
	return 0;
}

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	inst=hInstance;
	loader = lpCmdLine;

//	grabCursor		= LoadCursor (inst, MAKEINTRESOURCE (102));
	normalCursor	= LoadCursor (NULL, IDC_ARROW);
	busyCursor		= LoadCursor (NULL, IDC_WAIT);

	if (!CreateDialog(hInstance, MAKEINTRESOURCE(500), NULL, dialogproc)) {
		return 0;
	}

	while (1) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (!GetMessage(&msg, NULL, 0, 0))
				break;
			if (! IsDialogMessage (mainWin, &msg)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else
			WaitMessage();
	}

	return 0;
}
