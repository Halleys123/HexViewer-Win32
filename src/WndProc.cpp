#include "WndProc.h"
#include "HexTable.h"

#define PADDING_TOP 12
#define PADDING_BOTTOM 24
#define BYTE_SPACING_X 4
#define PADDING_LEFT 12
#define TEXT_BYTE_GAP 48

// For next thing I want that it don't reads more than 3 MB of file, either way, no need to read more than that, so I will for future use, modify it so that file is read in 3 parts
// I will have 3 buffers 1 MB each, one of them will save the whole thing being displayed on screen other two are extra's to quickly switch from current pointer to next
// My goal is to save memory, why read all when can be done in less (either its not like you need all the data at once)
// That is

// BufferA
// BufferB
// BufferC

// ReadFromPointer -> BufferB

// always read from pointer named ReadFromPointer
// When scroll bar is near start change pointer to BufferA, load more previous data in BufferB and so on
// but need to think of a mechanism to switch data properly

// Found online that there are two ways in which I can handle this: 
// 1. Same as above 3 buffer sliding window manual implementation but this is prone to Seam Bug and is difficult to handle (but will try to make it someday)

// 2. Second method is memory mapped files, rather than loading and creating a sliding window manually I can use Windows Inbuilt function made for exact same purpose
// because this sas such a common problem, windows have it inbuilt: "CreateFileMapping" and "MapViewOfFile". This is the shit that I studied in OS, for exact details:
// You tell the OS: "Map this entire 50GB file into my virtual memory space."
// The OS gives you back a single unsigned char* buffer pointer.
// The Magic : The OS does not load 50GB into your RAM.It loads 0 bytes.
// When your WM_PAINT loop tries to read buffer[0], the CPU triggers a "Page Fault." The OS pauses your app for a microsecond, grabs a tiny 4KB chunk of the file from the hard drive, puts it in RAM, and lets your app continue.

// More future ideas
// There is a function called ScrollWindowEx this function allows to reduce cpu usage by 99% as this is a hardware level function that moves pixels at hardware level,
// use it to acheive flicker free, very low memory usage version of hex editor

ATOM OpenFile(HWND hWnd, LARGE_INTEGER& fileSize, static const TCHAR* path, static unsigned char** buffer) {
	if (*buffer) {
		delete[] *buffer;
		*buffer = nullptr;
	}

	HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		MessageBox(hWnd, TEXT("No such specified file exists, closing the program"), TEXT("No File Found"), MB_OK | MB_ICONERROR);
		return 0;
	}

	TCHAR newTitleBuffer[100];
	wsprintf(newTitleBuffer, TEXT("Hex Reader - %s"), path);
	SetWindowText(hWnd, newTitleBuffer);

	if (GetFileSizeEx(file, &fileSize) == 0) {
		MessageBox(hWnd, TEXT("Can't process the file size, Unable to proceed, closing the program."), TEXT("Insufficient permissions"), MB_OK | MB_ICONERROR);
		return 0;
	}

	*buffer = new unsigned char[fileSize.QuadPart]; // 1 bytes = 1 char so it works
	DWORD actualBytesRead = 0;
	if (!ReadFile(file, *buffer, fileSize.QuadPart, &actualBytesRead, NULL)) {
		MessageBox(hWnd, TEXT("Unable to read file, internal error"), TEXT("Error Reading file"), MB_OK | MB_ICONERROR);
		return 0;
	}

	return 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	static HDC hdc;

	static unsigned char* buffer = nullptr;
	// I'll use and support microsofts 260 character limit
	static TCHAR tempPath[260];
	static DWORD tempPathIndex = 0;
	static TCHAR path[260] = TEXT("./HexViewer.cpp");
	
	// Pixel based scrolling rather than line basd.
	static DWORD contentHeight = 1, viewportHeight = 1;
	static DWORD bytesPerLines = 12;
	static DWORD totalLines;
	static DWORD charHeight = 1, charWidth = 1;
	static LARGE_INTEGER fileSize;

	static RECT clientRect;
	static TEXTMETRIC tm;
	static SCROLLINFO sf;

	// Scroll speed controls using keys
	static int speedY = 0;
	static int maxSpeedY = 32;
	static BOOL keyDownPressed = FALSE;
	static BOOL keyUpPressed = FALSE;

	switch (msg)
	{
	case WM_CREATE:
	{
		
		if (!OpenFile(hWnd, fileSize, path, &buffer)) {
			PostMessage(hWnd, WM_CLOSE, NULL, NULL);
			return 0;
		}

		hdc = GetDC(hWnd);

		// Getviewport height
		GetClientRect(hWnd, &clientRect);
		viewportHeight = clientRect.bottom - clientRect.top;

		// Getcharacter6 height
		GetTextMetrics(hdc, &tm);
		charHeight = tm.tmHeight + tm.tmExternalLeading;
		charWidth = tm.tmMaxCharWidth;

		ReleaseDC(hWnd, hdc);

		totalLines = fileSize.QuadPart/ bytesPerLines + (fileSize.QuadPart % bytesPerLines != 0);
		contentHeight = totalLines * charHeight + PADDING_TOP + PADDING_BOTTOM;

		TCHAR finalMsg[200];
		wsprintf(finalMsg, TEXT("Calculations are now complete\nContent Height: %d\ntotalLines: %d\nViewport Height: %d"), contentHeight, totalLines, viewportHeight);
		MessageBox(hWnd, finalMsg, TEXT("Info"), MB_OK | MB_ICONEXCLAMATION);

		sf.cbSize = sizeof(SCROLLINFO);
		sf.fMask = SIF_ALL;
		sf.nMin = 0;
		sf.nMax = contentHeight - 1;
		sf.nPage = viewportHeight;
		sf.nPos = 0;

		SetScrollInfo(hWnd, SB_VERT, &sf, FALSE);

		break;
	}
	case WM_SIZE:
	{
		viewportHeight = HIWORD(lparam);


		if (viewportHeight >= contentHeight) {
			EnableScrollBar(hWnd, SB_VERT, ESB_DISABLE_BOTH);
		}

		break;
	}
	case WM_PAINT:
	{
		// Personal Notes:
		// Start reading from first byte
		// print every byte
		// on every 8th byte change to next line
		// stop when line * charHeight + padding >= viewPortHeight
		PAINTSTRUCT ps;
		hdc = BeginPaint(hWnd, &ps);

		int lastLine = (sf.nPos + sf.nPage) / charHeight;
		int lettersInLastLine = 0;

		while (lastLine) {
			lastLine /= 10;
			lettersInLastLine += 1;
		}

		int startLine = sf.nPos / charHeight;
		int endLine = startLine + viewportHeight / charHeight + 2;
		if (endLine > totalLines) endLine = totalLines;

		int hexSpacing = (charWidth * 2) + BYTE_SPACING_X;

		for (int line = startLine; line < endLine; line += 1) {
			int relativeY = (line * charHeight) - sf.nPos + PADDING_TOP;

			char lineNumber[16] = { 0 };
			int actualLength = wsprintfA(lineNumber, "%d", line);

			TextOutA(hdc, PADDING_LEFT, relativeY, lineNumber, lettersInLastLine);

			for (int byte = 0; byte < bytesPerLines && byte + line * bytesPerLines < fileSize.QuadPart; byte += 1) {
				int xPos = PADDING_LEFT + (3 * charWidth) + 16 + (byte * hexSpacing);
				TextOutA(hdc, xPos, relativeY, hex_table[buffer[byte + line * bytesPerLines]], 2);
			}

			for (int byte = 0; byte < bytesPerLines && byte + line * bytesPerLines < fileSize.QuadPart; byte += 1) {
				int xPos = PADDING_LEFT + (3 * charWidth) + 16 + (bytesPerLines * hexSpacing) + TEXT_BYTE_GAP + (byte * hexSpacing / 2);
				TextOutA(hdc, xPos, relativeY, (char*)buffer + (byte + line * bytesPerLines), 1);
			}
		}
		
		EndPaint(hWnd, &ps);

		break;
	}

	case WM_VSCROLL: 
	{
		GetScrollInfo(hWnd, SB_VERT, &sf);
		
		switch(LOWORD(wparam)) {
			case SB_LINEDOWN: 
			{
				sf.nPos += 32;
				sf.fMask = SIF_POS;
				break;
			}
			case SB_LINEUP:
			{
				sf.nPos -= 32;
				sf.fMask = SIF_POS;
				break;
			}
		}

		SetScrollInfo(hWnd, SB_VERT, &sf, TRUE);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}

	case WM_KEYDOWN:
	{
		bool isCtrlHeld = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

		if (isCtrlHeld) {
			bool gridChanged = false;

			if (wparam == VK_OEM_PLUS || wparam == VK_ADD) {
				if (bytesPerLines < 64) {
					bytesPerLines += 1;
					gridChanged = true;
				}
			}
			else if (wparam == VK_OEM_MINUS || wparam == VK_SUBTRACT) {
				if (bytesPerLines > 1) {
					bytesPerLines -= 1;
					gridChanged = true;
				}
			}

			if (gridChanged && buffer != nullptr) {

				totalLines = fileSize.QuadPart / bytesPerLines + (fileSize.QuadPart % bytesPerLines != 0);
				contentHeight = totalLines * charHeight + PADDING_TOP + PADDING_BOTTOM;

				sf.nMax = contentHeight - 1;
				sf.nPage = viewportHeight;

				int maxScroll = sf.nMax - sf.nPage + 1;
				if (maxScroll < 0) maxScroll = 0;
				if (sf.nPos > maxScroll) sf.nPos = maxScroll;

				SetScrollInfo(hWnd, SB_VERT, &sf, TRUE);
				InvalidateRect(hWnd, NULL, TRUE);
			}

			return 0; 
		}

		if (wparam == VK_RETURN) {

			tempPath[tempPathIndex] = '\0';

			if (OpenFile(hWnd, fileSize, tempPath, &buffer)) {
				totalLines = fileSize.QuadPart / bytesPerLines + (fileSize.QuadPart % bytesPerLines != 0);
				contentHeight = totalLines * charHeight + PADDING_TOP + PADDING_BOTTOM;

				sf.nMax = contentHeight - 1;
				sf.nPage = viewportHeight;
				sf.nPos = 0;

				SetScrollInfo(hWnd, SB_VERT, &sf, TRUE);
				InvalidateRect(hWnd, NULL, TRUE);

				lstrcpy(path, tempPath);
				OutputDebugString(path);
			}

			tempPathIndex = 0;
			break;
		}
		else if (wparam == VK_ESCAPE) {
			tempPathIndex = 0;
			TCHAR newTitleBuffer[100] = { '\0' };
			wsprintf(newTitleBuffer, TEXT("Hex Reader - %s"), path);
			SetWindowText(hWnd, newTitleBuffer);

			break;
		}

		else if (wparam == VK_BACK) {
			if (tempPathIndex > 0) {
				tempPath[tempPathIndex--] = '\0';
			}
		}


		if (wparam == VK_DOWN) {
			speedY += 2;
		}
		else if (wparam == VK_UP) {
			speedY -= 2;
		}
		else {
			break;
		}

		if (speedY > maxSpeedY) speedY = maxSpeedY;
		if (speedY < -maxSpeedY) speedY = -maxSpeedY;

		sf.nPos += speedY;

		int maxScroll = contentHeight - viewportHeight;
		if (maxScroll < 0) maxScroll = 0;

		if (sf.nPos > maxScroll) sf.nPos = maxScroll;
		if (sf.nPos < 0) sf.nPos = 0;

		sf.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_VERT, &sf, TRUE);
		InvalidateRect(hWnd, NULL, TRUE);

		break;
	}

	case WM_CHAR:
	{
		if (wparam > 31 && tempPathIndex < 260) {

			tempPath[tempPathIndex] = (TCHAR)wparam;
			tempPathIndex += 1;

			TCHAR newTitleBuffer[100];
			wsprintf(newTitleBuffer, TEXT("Hex Reader - %s"), tempPath);
			SetWindowText(hWnd, newTitleBuffer);

		}
		return 0;
	}

	case WM_CLOSE:
		if(buffer)
			delete[] buffer;
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wparam, lparam);
}