#include "../include/opcua_client.hpp"
#include "../include/graph_renderer.hpp"
#include <windows.h>
#include <commctrl.h>
#include "../resource.h"
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

// Глобальные переменные
OPCUAClient g_client;
HWND g_hList = NULL;

// Прототипы функций
void CreateGraphWindow(HWND hParent, const std::string& tagName, const std::string& unit);
void UpdateTagList(HWND hMainWnd);

// Диалог подключения
INT_PTR CALLBACK ConnectDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // Центрируем диалог
            RECT rcOwner, rcDlg;
            HWND hwndOwner = GetParent(hDlg);
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            
            int x = rcOwner.left + (rcOwner.right - rcOwner.left) / 2 - (rcDlg.right - rcDlg.left) / 2;
            int y = rcOwner.top + (rcOwner.bottom - rcOwner.top) / 2 - (rcDlg.bottom - rcDlg.top) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // Устанавливаем текст по умолчанию
            SetDlgItemText(hDlg, IDC_SERVER_URL, "opc.tcp://localhost:4840");
            
            // Устанавливаем фокус на поле ввода
            SetFocus(GetDlgItem(hDlg, IDC_SERVER_URL));
            return FALSE;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                // Получаем URL сервера
                char serverUrl[256];
                GetDlgItemText(hDlg, IDC_SERVER_URL, serverUrl, 256);
                
                // Пробуем подключиться
                if (strlen(serverUrl) > 0) {
                    bool success = g_client.connect(serverUrl);
                    
                    if (success) {
                        MessageBox(hDlg, "Connected successfully!", "Success", 
                                  MB_OK | MB_ICONINFORMATION);
                        EndDialog(hDlg, IDOK);
                        
                        // Обновляем список тегов после подключения
                        HWND hParent = GetParent(hDlg);
                        UpdateTagList(hParent);
                    } else {
                        MessageBox(hDlg, "Failed to connect to server!", "Error", 
                                  MB_OK | MB_ICONERROR);
                    }
                } else {
                    MessageBox(hDlg, "Please enter server URL!", "Warning", 
                              MB_OK | MB_ICONWARNING);
                }
            } 
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
            }
            break;
        }
        
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

// Диалог записи
INT_PTR CALLBACK WriteDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // Центрируем диалог
            RECT rcOwner, rcDlg;
            HWND hwndOwner = GetParent(hDlg);
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            
            int x = rcOwner.left + (rcOwner.right - rcOwner.left) / 2 - (rcDlg.right - rcDlg.left) / 2;
            int y = rcOwner.top + (rcOwner.bottom - rcOwner.top) / 2 - (rcDlg.bottom - rcDlg.top) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // Заполняем комбобокс тегами
            HWND hCombo = GetDlgItem(hDlg, IDC_TAG_COMBO);
            auto tags = g_client.getTags();
            
            for (const auto& tag : tags) {
                SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)tag.name.c_str());
            }
            
            if (!tags.empty()) {
                SendMessage(hCombo, CB_SETCURSEL, 0, 0);
                
                // Показываем текущее значение первого тега
                char valueStr[50];
                sprintf_s(valueStr, "%.2f", tags[0].value);
                SetDlgItemText(hDlg, IDC_CURRENT_VALUE, valueStr);
                
                // Показываем единицы измерения
                SetDlgItemText(hDlg, IDC_TAG_UNIT, tags[0].unit.c_str());
                
                // Показываем статус
                SetDlgItemText(hDlg, IDC_STATUS, tags[0].is_written ? "WRITTEN" : "AUTO");
            }
            
            // Устанавливаем фокус на поле ввода нового значения
            SetFocus(GetDlgItem(hDlg, IDC_NEW_VALUE));
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDC_TAG_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                // Пользователь выбрал другой тег
                HWND hCombo = GetDlgItem(hDlg, IDC_TAG_COMBO);
                int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                
                if (index != CB_ERR) {
                    char tagName[256];
                    SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)tagName);
                    
                    auto tagPtr = g_client.getTagByName(tagName);
                    if (tagPtr) {
                        char valueStr[50];
                        sprintf_s(valueStr, "%.2f", tagPtr->value);
                        SetDlgItemText(hDlg, IDC_CURRENT_VALUE, valueStr);
                        SetDlgItemText(hDlg, IDC_TAG_UNIT, tagPtr->unit.c_str());
                        SetDlgItemText(hDlg, IDC_STATUS, tagPtr->is_written ? "WRITTEN" : "AUTO");
                        
                        // Очищаем поле ввода нового значения
                        SetDlgItemText(hDlg, IDC_NEW_VALUE, "");
                    }
                }
            }
            else if (LOWORD(wParam) == IDC_WRITE_BUTTON) {
                HWND hCombo = GetDlgItem(hDlg, IDC_TAG_COMBO);
                int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                
                if (index != CB_ERR) {
                    char tagName[256];
                    SendMessage(hCombo, CB_GETLBTEXT, index, (LPARAM)tagName);
                    
                    // Получаем новое значение
                    char newValueStr[256];
                    GetDlgItemText(hDlg, IDC_NEW_VALUE, newValueStr, 256);
                    
                    // Если поле пустое, используем текущее значение
                    if (strlen(newValueStr) == 0) {
                        GetDlgItemText(hDlg, IDC_CURRENT_VALUE, newValueStr, 256);
                    }
                    
                    // Преобразуем в число
                    double value = atof(newValueStr);
                    
                    // Записываем значение
                    if (g_client.writeTagByName(tagName, value)) {
                        // Обновляем текущее значение в диалоге
                        auto tagPtr = g_client.getTagByName(tagName);
                        if (tagPtr) {
                            char valueStr[50];
                            sprintf_s(valueStr, "%.2f", tagPtr->value);
                            SetDlgItemText(hDlg, IDC_CURRENT_VALUE, valueStr);
                            SetDlgItemText(hDlg, IDC_STATUS, "WRITTEN");
                        }
                        
                        MessageBox(hDlg, "Value written successfully!", "Success", 
                                  MB_OK | MB_ICONINFORMATION);
                        // НЕ закрываем диалог
                    } else {
                        MessageBox(hDlg, "Failed to write value!", "Error", 
                                  MB_OK | MB_ICONERROR);
                    }
                }
            } 
            else if (LOWORD(wParam) == IDOK) {
                EndDialog(hDlg, IDOK);
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
            }
            break;
        }
        
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

// Диалог сброса
INT_PTR CALLBACK ResetDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hListBox = NULL;
    
    switch (msg) {
        case WM_INITDIALOG: {
            // Центрируем диалог
            RECT rcOwner, rcDlg;
            HWND hwndOwner = GetParent(hDlg);
            GetWindowRect(hwndOwner, &rcOwner);
            GetWindowRect(hDlg, &rcDlg);
            
            int x = rcOwner.left + (rcOwner.right - rcOwner.left) / 2 - (rcDlg.right - rcDlg.left) / 2;
            int y = rcOwner.top + (rcOwner.bottom - rcOwner.top) / 2 - (rcDlg.bottom - rcDlg.top) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            // Получаем ListBox
            hListBox = GetDlgItem(hDlg, IDC_RESET_LIST);
            
            // Заполняем только WRITTEN теги
            auto tags = g_client.getTags();
            int writtenCount = 0;
            
            for (const auto& tag : tags) {
                if (tag.is_written) {
                    char itemText[100];
                    sprintf_s(itemText, "%s = %.2f %s", 
                            tag.name.c_str(), tag.value, tag.unit.c_str());
                    int index = (int)SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)itemText);
                    // Сохраняем имя тега в данных элемента
                    SendMessage(hListBox, LB_SETITEMDATA, index, (LPARAM)tag.name.c_str());
                    writtenCount++;
                }
            }
            
            // Обновляем статус
            char status[100];
            sprintf_s(status, "%d WRITTEN tag(s) found", writtenCount);
            SetDlgItemText(hDlg, IDC_STATUS_TEXT, status);
            
            if (writtenCount == 0) {
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE); // Отключаем кнопку Reset
            }
            
            return TRUE;
        }
        
        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK) {
                int selectedCount = SendMessage(hListBox, LB_GETSELCOUNT, 0, 0);
                
                if (selectedCount > 0) {
                    int* selectedItems = new int[selectedCount];
                    SendMessage(hListBox, LB_GETSELITEMS, selectedCount, (LPARAM)selectedItems);
                    
                    int resetCount = 0;
                    
                    for (int i = 0; i < selectedCount; i++) {
                        int index = selectedItems[i];
                        char* tagName = (char*)SendMessage(hListBox, LB_GETITEMDATA, index, 0);
                        
                        if (tagName && g_client.resetTagToAuto(tagName)) {
                            resetCount++;
                        }
                    }
                    
                    delete[] selectedItems;
                    
                    if (resetCount > 0) {
                        // Обновляем главное окно
                        HWND hParent = GetParent(hDlg);
                        UpdateTagList(hParent);
                        
                        char finalMsg[256];
                        sprintf_s(finalMsg, "Successfully reset %d tag(s)", resetCount);
                        MessageBox(hDlg, finalMsg, "Reset Complete", MB_OK | MB_ICONINFORMATION);
                        EndDialog(hDlg, IDOK);
                    } else {
                        MessageBox(hDlg, "No tags were reset", "Info", MB_OK | MB_ICONINFORMATION);
                    }
                } else {
                    MessageBox(hDlg, "Please select at least one tag!", "Warning", MB_OK | MB_ICONWARNING);
                }
            } 
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
            }
            break;
        }
        
        case WM_CLOSE:
            EndDialog(hDlg, IDCANCEL);
            break;
    }
    return FALSE;
}

// Обновление списка тегов
void UpdateTagList(HWND hMainWnd) {
    if (!g_hList) return;
    
    ListView_DeleteAllItems(g_hList);
    
    auto tags = g_client.getTags();
    
    if (hMainWnd) {
        char title[256];
        const char* status = g_client.isConnected() ? "CONNECTED" : "DISCONNECTED";
        sprintf_s(title, "OPC UA Monitor - %s - %d tags", status, (int)tags.size());
        SetWindowText(hMainWnd, title);
    }

    for (size_t i = 0; i < tags.size(); i++) {
        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = (int)i;
        lvi.iSubItem = 0;
        
        // Выделяем WRITTEN теги
        if (tags[i].is_written) {
            lvi.mask |= LVIF_STATE;
            lvi.state = LVIS_SELECTED;
            lvi.stateMask = LVIS_SELECTED;
        }
        
        lvi.pszText = (LPSTR)tags[i].name.c_str();
        ListView_InsertItem(g_hList, &lvi);
        
        // Значение
        char valueStr[50];
        sprintf_s(valueStr, "%.2f", tags[i].value);
        ListView_SetItemText(g_hList, i, 1, valueStr);
        
        // Единицы измерения
        ListView_SetItemText(g_hList, i, 2, (LPSTR)tags[i].unit.c_str());
        
        // Статус (WRITTEN или AUTO)
        char status[20];
        strcpy_s(status, tags[i].is_written ? "WRITTEN" : "AUTO");
        ListView_SetItemText(g_hList, i, 3, status);
        
        // Время
        ListView_SetItemText(g_hList, i, 4, (LPSTR)tags[i].timestamp.c_str());
        
        // Качество
        ListView_SetItemText(g_hList, i, 5, (LPSTR)tags[i].quality.c_str());
    }
}

// Обработчик главного окна
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // Инициализация
            INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
            InitCommonControlsEx(&icex);
            
            // Кнопки
            CreateWindow("BUTTON", "Connect",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 10, 100, 30, hWnd, (HMENU)5, NULL, NULL);

            CreateWindow("BUTTON", "Write Value",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        120, 10, 100, 30, hWnd, (HMENU)1, NULL, NULL);
            
            CreateWindow("BUTTON", "Reset to AUTO",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        230, 10, 100, 30, hWnd, (HMENU)2, NULL, NULL);
            
            CreateWindow("BUTTON", "Refresh",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        340, 10, 100, 30, hWnd, (HMENU)3, NULL, NULL);
            
            CreateWindow("BUTTON", "Auto Update",
                        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                        450, 10, 100, 30, hWnd, (HMENU)4, NULL, NULL);
            
            // Таблица тегов
            g_hList = CreateWindow(WC_LISTVIEW, "",
                                  WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS,
                                  10, 50, 760, 300, hWnd, NULL, NULL, NULL);
            
            // Устанавливаем расширенный стиль
            ListView_SetExtendedListViewStyle(g_hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            
            // Колонки таблицы
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
            lvc.fmt = LVCFMT_LEFT;
            
            // Колонка 0: Имя тега
            lvc.pszText = "TAG NAME";
            lvc.cx = 150;
            ListView_InsertColumn(g_hList, 0, &lvc);
            
            // Колонка 1: Значение
            lvc.pszText = "VALUE";
            lvc.cx = 100;
            ListView_InsertColumn(g_hList, 1, &lvc);
            
            // Колонка 2: Единицы измерения
            lvc.pszText = "UNIT";
            lvc.cx = 80;
            ListView_InsertColumn(g_hList, 2, &lvc);
            
            // Колонка 3: Статус
            lvc.pszText = "STATUS";
            lvc.cx = 80;
            ListView_InsertColumn(g_hList, 3, &lvc);
            
            // Колонка 4: Время
            lvc.pszText = "TIME";
            lvc.cx = 100;
            ListView_InsertColumn(g_hList, 4, &lvc);
            
            // Колонка 5: Качество
            lvc.pszText = "QUALITY";
            lvc.cx = 80;
            ListView_InsertColumn(g_hList, 5, &lvc);
            
            // Обновляем список (начальное состояние)
            UpdateTagList(hWnd);
            
            break;
        }
        
        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            
            if (nmhdr->hwndFrom == g_hList && nmhdr->code == NM_DBLCLK) {
                // Двойной клик по тегу
                LPNMITEMACTIVATE item = (LPNMITEMACTIVATE)lParam;
                if (item->iItem >= 0) {
                    // Получаем имя тега
                    char tagName[256];
                    ListView_GetItemText(g_hList, item->iItem, 0, tagName, 256);
                    
                    // Получаем единицы измерения
                    char unit[50];
                    ListView_GetItemText(g_hList, item->iItem, 2, unit, 50);
                    
                    // Создаём окно с графиком
                    CreateGraphWindow(hWnd, tagName, unit);
                }
            }
            break;
        }

        case WM_USER + 100: // Сигнал обновить UI после подключения
            UpdateTagList(hWnd);
            break;

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case 1: // Write Value
                    DialogBoxParam(GetModuleHandle(NULL),
                                   MAKEINTRESOURCE(IDD_WRITE_DIALOG),
                                   hWnd,
                                   WriteDialogProc,
                                   0);
                    break;
                    
                case 2: // Reset to AUTO
                    DialogBoxParam(GetModuleHandle(NULL),
                                   MAKEINTRESOURCE(IDD_RESET_DIALOG),
                                   hWnd,
                                   ResetDialogProc,
                                   0);
                    // Обновляем таблицу после сброса
                    UpdateTagList(hWnd);
                    break;
                    
                case 3: // Refresh
                    g_client.updateValues();
                    UpdateTagList(hWnd);
                    break;
                    
                case 4: { // Auto Update (вкл/выкл)
                    static bool autoUpdate = false;
                    autoUpdate = !autoUpdate;
                    
                    if (autoUpdate) {
                        SetTimer(hWnd, 1, 2000, NULL); // Каждые 2 секунды
                        MessageBox(hWnd, "Auto-update enabled (2 sec)", "Info", MB_OK);
                    } else {
                        KillTimer(hWnd, 1);
                        MessageBox(hWnd, "Auto-update disabled", "Info", MB_OK);
                    }
                    break;
                }
                
                case 5: // Connect
                    DialogBoxParam(GetModuleHandle(NULL),
                                MAKEINTRESOURCE(IDD_CONNECT_DIALOG),
                                hWnd,
                                ConnectDialogProc,
                                0);
                    break;
            }
            break;
        }
        
        case WM_TIMER: // Автообновление
            if (wParam == 1) {
                g_client.updateValues();
                UpdateTagList(hWnd);
            }
            break;
            
        case WM_SIZE:
            if (g_hList) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                MoveWindow(g_hList, 10, 50, rc.right - 20, rc.bottom - 60, TRUE);
            }
            break;
            
        case WM_DESTROY:
            KillTimer(hWnd, 1);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszClassName = "SimpleOPCUA";
    
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Window class registration failed", "Error", MB_ICONERROR);
        return 0;
    }
    
    HWND hWnd = CreateWindow("SimpleOPCUA", "OPC UA Monitor - DISCONNECTED",
                            WS_OVERLAPPEDWINDOW, 100, 100, 800, 400,
                            NULL, NULL, hInstance, NULL);
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}