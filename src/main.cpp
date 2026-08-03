#include <iostream>
#include <windows.h>
#include "renderer.h"

bool running = true;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg){
        case WM_CLOSE:
            running = false;
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool create_window(HWND *hwnd){
    HINSTANCE hInstance = GetModuleHandleA(0);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "vulkan";
    if(!RegisterClass(&wc)){
        MessageBox(*hwnd, "failed to register window class", "Error", MB_OK);
        return false;
    }
    
    *hwnd = CreateWindow("vulkan", "snake", WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 800, 400, 800, 600, NULL, NULL, hInstance, NULL);
    if(!*hwnd){
        MessageBox(0, "failed to create window", "Error", MB_OK);
        return false;
    }
    ShowWindow(*hwnd, SW_SHOW);

    return true;
}

void update_window(HWND* window){
    MSG msg;
    while(PeekMessageA(&msg, *window, 0,0,PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}


int main() {
    VkContext vkcontext = {};
    HWND window = 0;
    create_window(&window);
    if(!vk_init(&vkcontext,(void*)window)){
        return -1;
    }
    while(running){
        update_window(&window);
        render(&vkcontext);
    }
    return 0;
}
