#pragma once
#include <windows.h>
#include <vector>
#include <string>

class GraphRenderer {
private:
    HWND hWnd;
    std::vector<double> data;
    std::string title;
    std::string unit;
    
    COLORREF bgColor = RGB(30, 30, 40);
    COLORREF gridColor = RGB(60, 60, 70);
    COLORREF lineColor = RGB(0, 200, 255);
    COLORREF pointColor = RGB(255, 100, 100);
    COLORREF textColor = RGB(220, 220, 220);
    
    int margin = 50;
    
public:
    GraphRenderer(HWND hwnd, const std::string& title, const std::string& unit);
    void setData(const std::vector<double>& newData);
    void render(HDC hdc, RECT clientRect);
    void clear();
};