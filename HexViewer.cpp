#include <windows.h>
#include "WndProc.h"

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR cmdLine, _In_ int showCmd) {
	
	WNDCLASS wndCls = { 0 };
	wndCls.hInstance = hInstance;
	wndCls.lpfnWndProc = WndProc;
	wndCls.style = CS_OWNDC;
	wndCls.lpszClassName = TEXT("HexReader");
	wndCls.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wndCls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndCls.hIcon = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClass(&wndCls)) {
		MessageBox(NULL, TEXT("Unable to register class for HexReader"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	HWND hWnd = CreateWindow(wndCls.lpszClassName, TEXT("Hex Reader"), WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		MessageBox(NULL, TEXT("Unable to create a window for HexReader"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	MSG msg = { 0 };
	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}