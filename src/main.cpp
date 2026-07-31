#include <iostream>
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool create_window(HWND hwnd){
    HINSTANCE hInstance = GetModuleHandleA(0);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "vulkan";
    if(!RegisterClass(&wc)){
        MessageBox(hwnd, "failed to register window class", "Error", MB_OK);
        return false;
    }
    
    hwnd = CreateWindow("vulkan", "snake", WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0, 800, 600, NULL, NULL, hInstance, NULL);
    if(!hwnd){
        MessageBox(hwnd, "failed to create window", "Error", MB_OK);
        return false;
    }
    ShowWindow(hwnd, SW_SHOW);

    return true;
}

void update_window(HWND window){
    MSG msg;
    while(GetMessage(&msg, window, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}


int main() {
    HWND window = 0;
    create_window(window);
    update_window(window);
    return 0;
}