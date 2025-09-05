/* 
This is the lasted example of LibFux demo, witch is using widgets like flutter and having state management like GetX!
Yeah you hear right, we bring the GetX statemanagement soon in this lib too, 
Keep updated, you can see lasted updates to this lib syntax just from this main.cpp file.
Regular updates will be applied , if i have some amount of free times..
Creator - mortza mansory
*/    
#include "libfux.hpp"

ui::State<std::string> textBoxContent("");


void onShowDialogPressed() {
    ui::Style buttonStyle;
    buttonStyle.backgroundColor = ui::Colors::lightBlue;
    buttonStyle.textStyle.color = ui::Colors::white;
    buttonStyle.padding = { 10, 10, 10, 10 };
    buttonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style dialogContainerStyle;
    dialogContainerStyle.backgroundColor = ui::Colors::white;
    dialogContainerStyle.padding = { 20, 20, 20, 20 };
    dialogContainerStyle.border.radius.topLeft = 20.0;
    dialogContainerStyle.border.radius.bottomRight = 20.0;

    ui::showDialog(
        ui::Container(
            ui::Column({
                ui::Text("This is a Dialog"),
                ui::Text("Click OK to close."),
                ui::TextButton("OK", [] {
                    if (ui::App::instance()) ui::App::instance()->popOverlay();
                }, buttonStyle)
                }, 10),
            dialogContainerStyle
        )
    );
}

void onShowSnackBarPressed() {
    ui::AppContext::instance().showSnackBar("This is a simple SnackBar!");
}

int main(int argc, char* argv[]) {
    ui::AppContext::instance();

    ui::Style primaryButtonStyle;
    primaryButtonStyle.backgroundColor = ui::Colors::blue;
    primaryButtonStyle.textStyle.color = ui::Colors::white;
    primaryButtonStyle.padding = { 10, 20, 10, 20 };
    primaryButtonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style successButtonStyle;
    successButtonStyle.backgroundColor = ui::Colors::green;
    successButtonStyle.textStyle.color = ui::Colors::white;
    successButtonStyle.padding = { 10, 20, 10, 20 };
    successButtonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style mainScaffoldStyle;
    mainScaffoldStyle.padding = { 20, 20, 20, 20 };

    ui::Style textBoxStyle;
    textBoxStyle.backgroundColor = { 230, 230, 230 };
    textBoxStyle.padding = { 10, 10, 10, 10 };
    textBoxStyle.border.radius = ui::BorderRadius::all(4.0);

    ui::App app(
        ui::Scaffold(
            ui::Column({
                ui::Text("FUX Demo", {24}),
                ui::TextBox(textBoxContent, "Type something here...", textBoxStyle),

                // Using the super-simple Obx widget for reactivity!
                ui::Obx([]() -> ui::Widget {
                    std::string text = textBoxContent.get(); 
                    if (text.empty()) {
                        return ui::Widget(); // Return an empty widget
                    }
                    return ui::Text("You wrote: " + text);
                }),

                ui::Row({
                    ui::TextButton("Show Dialog", onShowDialogPressed, primaryButtonStyle),
                    ui::TextButton("Show SnackBar", onShowSnackBarPressed, successButtonStyle)
                }, 10)
                }, 15),
            mainScaffoldStyle
        )
    );

    app.run("FUX Demo 3.0", true, { 600, 450 });

    return 0;
}
