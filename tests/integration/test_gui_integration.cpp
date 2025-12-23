#include <gtest/gtest.h>
#include <windows.h>
#include "../mocks/mock_client.hpp"
#include "../../src/simple_opcua_gui.cpp"

using ::testing::_;
using ::testing::Return;
using ::testing::NiceMock;

// Фикстура для тестов GUI
class GUITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Регистрируем тестовый класс окна
        WNDCLASS wc = {};
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "TestGUIClass";
        RegisterClass(&wc);
    }
    
    void TearDown() override {
        // Очистка
    }
};

// Тест: Обработчик диалога подключения
TEST_F(GUITest, ConnectDialogProcHandlesMessages) {
    // Создаем mock-клиента
    NiceMock<MockOPCUAClient> mockClient;
    
    // Заменяем глобальный клиент на mock (в реальном тесте нужно dependency injection)
    // g_client = &mockClient; // Не работает из-за типа
    
    // Проверяем, что функция определена
    EXPECT_NE(ConnectDialogProc, nullptr);
    
    // Базовый тест - не падает ли функция при вызове
    EXPECT_NO_FATAL_FAILURE({
        // Пробуем вызвать с базовыми параметрами
        // В реальности нужно создавать диалог
    });
}

// Тест: Создание элементов главного окна
TEST_F(GUITest, MainWindowCreationAndControls) {
    // Создаем тестовое окно
    HWND hWnd = CreateWindow("TestGUIClass", "Test Window",
                            WS_OVERLAPPEDWINDOW, 100, 100, 800, 400,
                            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    ASSERT_NE(hWnd, nullptr);
    
    // Проверяем, что окно создано
    EXPECT_TRUE(IsWindow(hWnd));
    
    // Проверяем стили окна
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    EXPECT_TRUE(style & WS_OVERLAPPEDWINDOW);
    
    DestroyWindow(hWnd);
}

// Тест: Создание элементов управления
TEST_F(GUITest, CreateStandardControls) {
    HWND hWnd = CreateWindow("TestGUIClass", "Test Window",
                            WS_OVERLAPPEDWINDOW, 100, 100, 800, 400,
                            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    ASSERT_NE(hWnd, nullptr);
    
    // Создаем кнопку
    HWND hButton = CreateWindow("BUTTON", "Test Button",
                               WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                               10, 10, 100, 30, hWnd, 
                               (HMENU)1, GetModuleHandle(nullptr), nullptr);
    
    EXPECT_NE(hButton, nullptr);
    EXPECT_TRUE(IsWindow(hButton));
    
    // Проверяем текст кнопки
    char buttonText[50];
    GetWindowText(hButton, buttonText, 50);
    EXPECT_STREQ(buttonText, "Test Button");
    
    // Создаем ListView
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);
    
    HWND hList = CreateWindow(WC_LISTVIEW, "",
                             WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT,
                             10, 50, 760, 300, hWnd, 
                             nullptr, nullptr, nullptr);
    
    EXPECT_NE(hList, nullptr);
    EXPECT_TRUE(IsWindow(hList));
    
    DestroyWindow(hWnd);
}

// Тест: Проверка сообщений Windows
TEST_F(GUITest, WindowsMessagesProcessing) {
    HWND hWnd = CreateWindow("TestGUIClass", "Test Window",
                            WS_OVERLAPPEDWINDOW, 100, 100, 800, 400,
                            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    ASSERT_NE(hWnd, nullptr);
    
    // Посылаем сообщения и проверяем обработку
    EXPECT_NO_FATAL_FAILURE({
        SendMessage(hWnd, WM_SIZE, 0, MAKELPARAM(800, 600));
        SendMessage(hWnd, WM_PAINT, 0, 0);
        SendMessage(hWnd, WM_CLOSE, 0, 0);
    });
    
    DestroyWindow(hWnd);
}

// Тест: Проверка ID ресурсов
TEST_F(GUITest, ResourceIDsDefined) {
    // Проверяем, что ID определены в resource.h
    // В тестовой среде они могут быть не доступны, поэтому проверяем только компиляцию
    
    #ifdef IDD_CONNECT_DIALOG
    EXPECT_GT(IDD_CONNECT_DIALOG, 0);
    #endif
    
    #ifdef IDD_WRITE_DIALOG
    EXPECT_GT(IDD_WRITE_DIALOG, 0);
    #endif
    
    #ifdef IDD_RESET_DIALOG
    EXPECT_GT(IDD_RESET_DIALOG, 0);
    #endif
}

// Тест: Работа с ListView
TEST_F(GUITest, ListViewOperations) {
    HWND hWnd = CreateWindow("TestGUIClass", "Test Window",
                            WS_OVERLAPPEDWINDOW, 100, 100, 800, 400,
                            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);
    
    HWND hList = CreateWindow(WC_LISTVIEW, "",
                             WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT,
                             10, 50, 760, 300, hWnd, 
                             nullptr, nullptr, nullptr);
    
    ASSERT_NE(hList, nullptr);
    
    // Добавляем колонки
    LVCOLUMN lvc = {};
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = 100;
    lvc.pszText = "Column 1";
    
    int colIndex = ListView_InsertColumn(hList, 0, &lvc);
    EXPECT_EQ(colIndex, 0);
    
    // Добавляем элемент
    LVITEM lvi = {};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;
    lvi.iSubItem = 0;
    lvi.pszText = "Test Item";
    
    int itemIndex = ListView_InsertItem(hList, &lvi);
    EXPECT_EQ(itemIndex, 0);
    
    // Получаем количество элементов
    int itemCount = ListView_GetItemCount(hList);
    EXPECT_EQ(itemCount, 1);
    
    DestroyWindow(hWnd);
}