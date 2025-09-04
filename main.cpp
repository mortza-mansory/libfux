#include "libfux.hpp"

auto& context = ui::AppContext::instance();

ui::State<bool> showDialogState(false);
ui::State<std::string> textBoxContent("");

void onShowDialogPressed() {
    showDialogState.set(true);
}

void onDialogOkPressed() {
    showDialogState.set(false);
}

ui::Style successStyle;
ui::Style errorStyle;

void onShowSuccessPressed() {
    std::string content = textBoxContent.get();
    std::string message = "Success! You wrote: " + (content.empty() ? "[nothing]" : content);
    context.showSnackBar(message, successStyle, ui::SnackBarPosition::Top);
}

void onShowErrorPressed() {
    context.showSnackBar("This is a custom error message!", errorStyle, ui::SnackBarPosition::Top);
}


int main(int argc, char* argv[]) {
    successStyle.backgroundColor = ui::Colors::green;
    successStyle.textStyle = { 16, ui::Colors::white };
    successStyle.padding = { 12, 18, 12, 18 };
    successStyle.border.radius = 8.0;

    errorStyle.backgroundColor = ui::Colors::red;
    errorStyle.textStyle = { 16, ui::Colors::white };
    errorStyle.padding = { 12, 18, 12, 18 };
    errorStyle.border.radius = 8.0;

    ui::Style primaryButtonStyle;
    primaryButtonStyle.backgroundColor = ui::Colors::blue;
    primaryButtonStyle.textStyle = { 16, ui::Colors::white };
    primaryButtonStyle.padding = { 10, 15, 10, 15 };
    primaryButtonStyle.border.radius = 8.0;

    ui::Style dialogContainerStyle;
    dialogContainerStyle.backgroundColor = ui::Colors::white;
    dialogContainerStyle.padding = { 20, 20, 20, 20 };
    dialogContainerStyle.border.radius = 12.0;

    ui::Style textBoxStyle;
    textBoxStyle.backgroundColor = { 230, 230, 230 };
    textBoxStyle.padding = { 8, 12, 8, 12 };
    textBoxStyle.border.radius = 8.0;

    ui::Style mainContainerStyle;
    mainContainerStyle.padding = { 20, 20, 20, 20 };

    ui::Style okButtonStyle;
    okButtonStyle.backgroundColor = ui::Colors::transparent;
    okButtonStyle.textStyle = { 16, ui::Colors::blue };

    ui::App app(
        ui::ContextProvider<ui::SnackBarConfig>(context.snackBarConfig,
            ui::ContextProvider<std::string>(textBoxContent,
                ui::ContextProvider<bool>(showDialogState,
                    ui::Stack({

                        ui::Container(
                            ui::Column({
                                ui::Text("FUX Library Demo", {24, ui::Colors::black}),
                                ui::TextBox(textBoxContent, "Type something here...", textBoxStyle),
                                ui::Row({
                                    ui::TextButton("Show Dialog", onShowDialogPressed, primaryButtonStyle),
                                    ui::TextButton("Show Success", onShowSuccessPressed, primaryButtonStyle),
                                    ui::TextButton("Show Error", onShowErrorPressed, primaryButtonStyle)
                                }, 10)
                            }, 15),
                            mainContainerStyle
                        ),
                            ui::If(showDialogState, [=]() {
                                return ui::DialogBox(
                                    ui::Container(
                                        ui::Column({
                                            ui::Text("This is a Dialog", {20, ui::Colors::black}),
                                            ui::Text("Click OK to close.", {14, ui::Colors::grey}),
                                            ui::Container({}, {{10,0,0,0}}),
                                            ui::TextButton("OK", onDialogOkPressed, okButtonStyle)
                                        }, 15),
                                        dialogContainerStyle
                                    )
                                );
                            }),

                            ui::If(context.isSnackBarVisible, []() {
                                return ui::ContextBuilder<ui::SnackBarConfig>([](ui::SnackBarConfig config) -> ui::Widget {
                                    return ui::SnackBar(
                                        ui::Container(
                                            ui::Text(config.message, config.style.textStyle),
                                            config.style
                                        ),
                                        config.position
                                    );
                                });
                            })
                        })
                )
            )
        )
    );

    app.run("FUX Demo", { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400 });

    return 0;
}
