#include "libfux/libfux.hpp"

int main() {
    auto myApp = ui::space([]() {
        ui::TextStyle titleStyle{34, {0, 0, 0}};
        ui::TextStyle buttonStyle{18, {0, 0, 0}, {255, 255, 255}}; 

        return new ui::Column({
            new ui::Text("Welcome to SDLUI", titleStyle),
            new ui::SpaceBox(200, 20, {255, 255, 255}), 
            new ui::Row({
                new ui::Button("Cancel", []() { printf("Cancel pressed!\n"); }, buttonStyle),
                new ui::Button("OK", []() { printf("OK pressed!\n"); }, buttonStyle) }, 20),
            new ui::Column({
                new ui::Button("Option 1", []() { printf("Option 1\n"); }, buttonStyle),
                new ui::Button("Option 2", []() { printf("Option 2\n"); }, buttonStyle)
            }, 10)
        });
    }, {60, 70, 10}); 

    ui::App app(myApp, "SDLUI Demo", 360, 480, false);
    app.run();

    return 0;
}