#include "../include/graph_renderer.hpp"
#include <algorithm>
#include <numeric>
#include <string>

GraphRenderer::GraphRenderer(HWND hwnd, const std::string& title, const std::string& unit)
    : hWnd(hwnd), title(title), unit(unit) {}

void GraphRenderer::setData(const std::vector<double>& newData) {
    data = newData;
}

void GraphRenderer::render(HDC hdc, RECT clientRect) {
    if (data.empty()) return;
    
    // Заливаем фон
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &clientRect, hBrush);
    DeleteObject(hBrush);
    
    int graphWidth = clientRect.right - clientRect.left - 2 * margin;
    int graphHeight = clientRect.bottom - clientRect.top - 2 * margin;
    
    if (graphWidth <= 0 || graphHeight <= 0) return;
    
    double minVal = *std::min_element(data.begin(), data.end());
    double maxVal = *std::max_element(data.begin(), data.end());
    double range = maxVal - minVal;
    if (range == 0) range = 1;
    
    // Сетка
    HPEN hGridPen = CreatePen(PS_DOT, 1, gridColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);
    
    for (int i = 0; i <= 10; i++) {
        int x = margin + i * graphWidth / 10;
        MoveToEx(hdc, x, margin, NULL);
        LineTo(hdc, x, clientRect.bottom - margin);
    }
    
    for (int i = 0; i <= 8; i++) {
        int y = margin + i * graphHeight / 8;
        MoveToEx(hdc, margin, y, NULL);
        LineTo(hdc, clientRect.right - margin, y);
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hGridPen);
    
    // Линия графика
    HPEN hLinePen = CreatePen(PS_SOLID, 2, lineColor);
    SelectObject(hdc, hLinePen);
    
    for (size_t i = 1; i < data.size(); i++) {
        int x1 = margin + (int)((double)(i-1) / (data.size()-1) * graphWidth);
        int y1 = clientRect.bottom - margin - 
                (int)((data[i-1] - minVal) / range * graphHeight);
        
        int x2 = margin + (int)((double)i / (data.size()-1) * graphWidth);
        int y2 = clientRect.bottom - margin - 
                (int)((data[i] - minVal) / range * graphHeight);
        
        MoveToEx(hdc, x1, y1, NULL);
        LineTo(hdc, x2, y2);
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hLinePen);
    
    // Точки
    HBRUSH hPointBrush = CreateSolidBrush(pointColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hPointBrush);
    
    for (size_t i = 0; i < data.size(); i++) {
        int x = margin + (int)((double)i / (data.size()-1) * graphWidth);
        int y = clientRect.bottom - margin - 
               (int)((data[i] - minVal) / range * graphHeight);
        
        Ellipse(hdc, x-3, y-3, x+3, y+3);
    }
    
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPointBrush);
    
    // Текст
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    
    HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH, "Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    std::string fullTitle = title + " (" + unit + ")";
    TextOut(hdc, margin, 10, fullTitle.c_str(), (int)fullTitle.length());
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    
    // Статистика
    char stats[100];
    double avg = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    sprintf_s(stats, "Points: %d | Min: %.2f | Max: %.2f | Avg: %.2f", 
             (int)data.size(), minVal, maxVal, avg);
    
    TextOut(hdc, margin, clientRect.bottom - 30, stats, (int)strlen(stats));
}

void GraphRenderer::clear() {
    data.clear();
}