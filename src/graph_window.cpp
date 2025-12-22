#include <windows.h>
#include <string>
#include "../include/opcua_client.hpp"
#include "../include/graph_renderer.hpp"

extern OPCUAClient g_client;
GraphRenderer* g_pGraphRenderer = nullptr;
std::string g_currentTagName;

LRESULT CALLBACK GraphWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            auto tagPtr = g_client.getTagByName(g_currentTagName);
            if (tagPtr) {
                g_pGraphRenderer = new GraphRenderer(hWnd, tagPtr->name, tagPtr->unit);
                
                auto history = g_client.getTagHistory(g_currentTagName);
                if (history) {
                    g_pGraphRenderer->setData(history->values);
                }
            }
            break;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rect;
            GetClientRect(hWnd, &rect);
            
            if (g_pGraphRenderer) {
                g_pGraphRenderer->render(hdc, rect);
            } else {
                FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
                TextOut(hdc, 10, 10, "No data to display", 18);
            }
            
            EndPaint(hWnd, &ps);
            break;
        }
        
        case WM_SIZE:
            InvalidateRect(hWnd, NULL, TRUE);
            break;
            
        case WM_TIMER:
            if (wParam == 1) {
                auto history = g_client.getTagHistory(g_currentTagName);
                if (history && g_pGraphRenderer) {
                    g_pGraphRenderer->setData(history->values);
                    InvalidateRect(hWnd, NULL, TRUE);
                }
            }
            break;
            
        case WM_DESTROY:
            if (g_pGraphRenderer) {
                delete g_pGraphRenderer;
                g_pGraphRenderer = nullptr;
            }
            KillTimer(hWnd, 1);
            break;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void CreateGraphWindow(HWND hParent, const std::string& tagName, const std::string& unit) {
    g_currentTagName = tagName;
    
    WNDCLASS wc = {0};
    wc.lpfnWndProc = GraphWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszClassName = "OPCUAGraphClass";
    
    RegisterClass(&wc);
    
    HWND hGraphWnd = CreateWindow(
        "OPCUAGraphClass",
        ("Graph: " + tagName).c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        hParent, NULL, GetModuleHandle(NULL), NULL
    );
    
    SetTimer(hGraphWnd, 1, 1000, NULL);
    
    ShowWindow(hGraphWnd, SW_SHOW);
    UpdateWindow(hGraphWnd);
}