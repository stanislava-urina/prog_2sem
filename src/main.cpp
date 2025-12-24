#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <map>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <algorithm>
#include "../include/opcua_client.hpp"

using namespace ftxui;

int main() {
    OPCUAClient client;
    // Подключаемся к серверу (убедись, что адрес верный)
    client.connectToServer("opc.tcp://127.0.0.1:4840");

    auto screen = ScreenInteractive::Fullscreen();
    
    // Хранилище историй: Имя тега -> Список значений (double для точности)
    std::map<std::string, std::vector<double>> histories;
    
    std::string input_val = "";
    std::string status = "Status: OK";
    int selected = 0;

    // Компоненты ввода
    auto input_field = Input(&input_val, "0.0");
    
    auto btn = Button(" SEND ", [&] {
        auto tags = client.getTags();
        if (selected < tags.size()) {
            try {
                double v = std::stod(input_val);
                if (client.writeValue(tags[selected].nodeId, v)) {
                    status = "Done: " + std::to_string(v);
                } else {
                    status = "Fail: Server error";
                }
            } catch (...) {
                status = "Error: Invalid input";
            }
        }
    });

    // Список имен для меню выбора
    std::vector<std::string> names = {"Temperature", "Voltage"};
    auto menu = Menu(&names, &selected);

    auto renderer = Renderer(Container::Vertical({menu, input_field, btn}), [&] {
        client.updateValues();
        auto tags = client.getTags();
        Elements charts;

        for (const auto& tag : tags) {
            // 1. Обновляем историю конкретного тега
            auto& history = histories[tag.name];
            history.push_back(tag.value);
            if (history.size() > 100) history.erase(history.begin());

            // Сохраняем данные для использования внутри лямбды
            std::string tagName = tag.name; 
            double currentVal = tag.value;

            // 2. Отрисовка графика с масштабированием
            charts.push_back(vbox({
                text(tagName + ": " + std::to_string(currentVal).substr(0, 6)) | bold | color(Color::Yellow),
                graph([&histories, tagName](int w, int h) {
                    std::vector<int> r(w, 0);
                    const auto& his = histories[tagName];
                    if (his.empty() || h == 0) return r;

                    // Находим min/max для того, чтобы график занимал всё окно
                    auto [min_it, max_it] = std::minmax_element(his.begin(), his.end());
                    double min_v = *min_it;
                    double max_v = *max_it;

                    // Если значения не меняются, создаем искусственный диапазон
                    if (max_v == min_v) {
                        max_v += 1.0;
                        min_v -= 1.0;
                    }

                    for (int i = 0; i < w && i < his.size(); ++i) {
                        double val = his[his.size() - 1 - i];
                        // Пропорция: текущее значение относительно диапазона, умноженное на высоту h
                        r[w - 1 - i] = static_cast<int>((val - min_v) * h / (max_v - min_v));
                    }
                    return r;
                }) | flex | color(Color::GreenLight) | border
            }) | flex);
        }

        // 3. Компоновка интерфейса
        return vbox({
            text(" OPC UA TUI MONITOR ") | bold | center | border | color(Color::Cyan),
            hbox({
                vbox({ 
                    text(" SELECT TAG ") | bold, 
                    menu->Render() | border 
                }) | size(WIDTH, EQUAL, 25),
                vbox({text(" WRITE VALUE ") | bold, 
                    hbox(text(" New: "), input_field->Render()) | border, 
                    btn->Render() | center,
                    text(status) | center | color(status.find("Done") != std::string::npos ? Color::Green : Color::Red)
                }) | flex
            }) | size(HEIGHT, EQUAL, 10),
            separator(),
            hbox(std::move(charts)) | flex 
        }) | border;
    });

    // Фоновый поток для авто-обновления экрана (2 раза в секунду)
    std::atomic<bool> run(true);
    std::thread ui_thread([&] {
        while(run) { 
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
            screen.PostEvent(Event::Custom); 
        }
    });

    screen.Loop(renderer);
    
    // Чистое завершение
    run = false; 
    if(ui_thread.joinable()) ui_thread.join();
    
    return 0;
}