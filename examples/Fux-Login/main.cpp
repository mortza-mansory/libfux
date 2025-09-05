
#include "libfux.hpp"

// --- 1. Global State Management ---
// State for the username text box
ui::State<std::string> username("User");
// State to track login status
ui::State<bool> isLoggedIn(false);
// State for a button's height to demonstrate reactive sizing
ui::State<int> dynamicButtonHeight(40);

// --- 2. UI Action Functions ---

// This function is called from within the dialog
void showConfirmationSnackBar() {
    ui::showSnackBar("You have been logged in!");
}

// Shows a dialog and demonstrates overlays working together
void showLoginDialog() {
    ui::Style buttonStyle;
    buttonStyle.backgroundColor = ui::Colors::green;
    buttonStyle.textStyle.color = ui::Colors::white;
    buttonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style dialogContainerStyle;
    dialogContainerStyle.backgroundColor = ui::Colors::white;
    dialogContainerStyle.padding = { 20, 20, 20, 20 };
    dialogContainerStyle.border.radius = ui::BorderRadius::all(12.0);

    ui::showDialog(
        ui::Container(
            ui::Column({
                ui::Text("Confirm Login", { 18 }),
                ui::Text("Log in as '" + username.get() + "'?"),
                ui::SizedBox(ui::Widget(), { -1, 10 }), // Vertical space
                ui::TextButton("Confirm", [] {
                    isLoggedIn.set(true);
                    ui::popOverlay(); // Close the dialog first
                    showConfirmationSnackBar(); // Then show the snackbar
                }, buttonStyle)
                }, 10),
            dialogContainerStyle
        )
    );
}


int main(int argc, char* argv[]) {
    ui::Style titleStyle;
    titleStyle.textStyle.fontSize = 28;

    ui::Style primaryButtonStyle;
    primaryButtonStyle.backgroundColor = ui::Colors::blue;
    primaryButtonStyle.textStyle.color = ui::Colors::white;
    primaryButtonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style logoutButtonStyle;
    logoutButtonStyle.backgroundColor = ui::Colors::red;
    logoutButtonStyle.textStyle.color = ui::Colors::white;
    logoutButtonStyle.border.radius = ui::BorderRadius::all(8.0);

    ui::Style secondaryButtonStyle;
    secondaryButtonStyle.backgroundColor = ui::Colors::grey;
    secondaryButtonStyle.textStyle.color = ui::Colors::white;
    secondaryButtonStyle.border.radius = ui::BorderRadius::all(20.0); // Pill shape

    ui::Style mainContainerStyle;
    mainContainerStyle.padding = { 20, 20, 20, 20 };

    ui::Style inputBoxStyle;
    inputBoxStyle.backgroundColor = { 240, 240, 240 };
    inputBoxStyle.padding = { 8, 8, 8, 8 };
    inputBoxStyle.border.radius = ui::BorderRadius::all(4);

    ui::App app(
        ui::Scaffold(
            ui::Container(
                ui::Column({
                    ui::Text("User Profile Demo", titleStyle.textStyle),
                    ui::SizedBox(ui::Widget(), { -1, 20 }), // Vertical space

                    ui::Row({
                        ui::SizedBox(ui::Text("Username:", {16}), {100, 40}),
                        ui::TextBox(username, "Enter name...", inputBoxStyle)
                    }, 10),

                    ui::SizedBox(ui::Widget(), { -1, 20 }),

                    // Reactive Welcome Message
                    ui::Obx([] {
                        if (isLoggedIn.get()) {
                            return ui::Widget(ui::Text("Welcome, " + username.get() + "!", ui::TextStyle{18, ui::Colors::green}));
                        }
                        return ui::Widget(ui::Text("Please log in.", ui::TextStyle{18, ui::Colors::red}));
                    }),

                    ui::SizedBox(ui::Widget(), { -1, 20 }),

                    ui::Row({
                        ui::Obx([&] {
                            if (isLoggedIn.get()) {
                                return ui::Widget(ui::TextButton("Logout", [] { isLoggedIn.set(false); }, logoutButtonStyle));
                            }
                            return ui::Widget(ui::TextButton("Login", showLoginDialog, primaryButtonStyle));
                        }),

                        ui::Obx([&] {
                            return ui::Widget(
                                ui::SizedBox(
                                    ui::TextButton("Change My Size", [] {
                                        dynamicButtonHeight.set(dynamicButtonHeight.get() == 40 ? 60 : 40);
                                    }, secondaryButtonStyle),
                                    { -1, dynamicButtonHeight.get() }
                                )
                            );
                        })
                    }, 10)

                    }, 15),
                mainContainerStyle
            )
        )
    );

    app.run("FUX Demo 3.0", true, { 600, 450 });

    return 0;
}

