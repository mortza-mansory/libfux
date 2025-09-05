//#include "libfux.hpp"
//
//// 1. State برنامه شما به صورت متغیرهای ساده تعریف می‌شود
//ui::State<int> counter(0);
//
//// 2. توابع شما فقط State را دستکاری می‌کنند
//void onFabPressed() {
//    counter.set(counter.get() + 1);
//}
//
//void onResetPressed() {
//    counter.set(0);
//}
//
//int main(int argc, char* argv[]) {
//    ui::App app(
//        new ui::Scaffold(
//            /* appBar: */ new ui::AppBar(
//                new ui::Text("Optimized Context", { 22, {255, 255, 255} })
//            ),
//            /* body: */ new ui::Center(
//                // 3. Provider را فقط دور قسمتی قرار می‌دهیم که به State نیاز دارد
//                new ui::ContextProvider<int>(counter,
//                    new ui::Column({
//                        new ui::Text("You have pushed the button this many times:", {18, {100, 100, 100}}),
//
//                        // 4. Builder به طور خودکار به Provider گوش داده و UI را بازسازی می‌کند
//                        new ui::ContextBuilder<int>(
//                            [](int currentCount) -> ui::Widget* {
//                                if (currentCount >= 5) {
//                                    return new ui::Button("Reset", onResetPressed, {24, {255, 255, 255}, {0, 150, 136}});
//                                }
// else {
//                                    // توجه: ما می‌توانیم متن را به صورت مستقیم هم به State وابسته کنیم
//                                    // اما استفاده از Builder برای تعویض ویجت قدرتمندتر است.
//                                    return new ui::Text(std::to_string(currentCount), {48, {0, 0, 0}});
//                                }
//                            }
//                        )
//                        }, 10)
//                )
//            ),
//            /* floatingActionButton: */ new ui::FloatingActionButton(
//                
//                new ui::Text("+", { 32, {255, 255, 255} }),
//                onFabPressed
//            )
//        )
//    );
//
//    app.run();
//
//    return 0;
//}
//
//// To hide the console, define this BEFORE including the library
//#define HIDE_CONSOLE
//
//#include "libfux.hpp"
//
//// State and event handlers
//// TODO: Adding String double and other data types.
//ui::State<int> counter(0);
//ui::State<bool> pressed(true);
////ui::State<string> form("bool");
////ui::State<double> 
//// .. other data types
//
//void onFabPressed() {
//    counter.set(counter.get() + 1);
//}
//
//void onResetPressed() {
//    counter.set(0);
//}
//
//void onButtonPressed()
//{
//    counter.set(4);
//    pressed.set(true);
//}
//
//int main(int argc, char* argv[]) {
//    // We can now define styles and decorations separately for cleaner code
//    ui::TextStyle buttonTextStyle = { 20, ui::Colors::white };
//    ui::BoxDecoration resetButtonDecoration = { ui::Colors::green, 12.0 }; // Green background, 12px rounded corners!
//
//    ui::App app(
//        new ui::Scaffold(
//            /* appBar: */ new ui::AppBar(
//                new ui::Text("FUI Example", { 22, ui::Colors::white })
//                // The AppBar now has a default blue background
//            ),
//            /* body: */ new ui::Center(
//                // The ContextProvider is scoped as tightly as possible around the Column
//                new ui::ContextProvider<int>(counter,
//                    new ui::Column({
//                        new ui::Text("You have pushed the button this many times:", {18, ui::Colors::darkGrey}),
//
//                        // This builder reacts to changes in the 'counter' state
//                        new ui::ContextBuilder<int>(
//                            [&](int currentCount) -> ui::Widget* {
//                                if (currentCount >= 5) {
//                                    // When the count is 5 or more, show a styled "Reset" button
//                                    return new ui::Button("Reset", onResetPressed, buttonTextStyle, resetButtonDecoration);
//                                }
// else {
//                                    // Otherwise, show the count
//                                    return new ui::Text(std::to_string(currentCount), {48, ui::Colors::black});
//                                }
//                            }
//                        ),
//                          new ui::ContextProvider<bool>(pressed,
//        new ui::ContextBuilder<bool>([&](bool isPressed) -> ui::Widget* {
//            if (isPressed) {
//                return new ui::Text("You pressed reset button!");
//            }
// else {
//                // You must return a widget in all cases.
//                // An empty Text widget works well as a placeholder.
//                return new ui::Text("");
//            }
//        }) // This parenthesis closes ContextBuilder
//    ),
//                        new ui::Column({
//                           new ui::Button("Click me to reset the number!",onButtonPressed)
//                            })
//                        }, 15) 
//                )
//            ),
//            /* floatingActionButton: */ new ui::FloatingActionButton(
//                new ui::Text("+", { 32, ui::Colors::white }),
//                onFabPressed
//              )
//        )
//    );
//
//    app.run();
//
//    return 0;
//}
//

// // =======================================================
// // Description: مثال برای تست کتابخانه بازطراحی شده libfux
// // =======================================================

// // #define USE_VULKAN // برای تست با Vulkan این خط را فعال کنید
// #include "libfux.hpp"

// int main(int argc, char* argv[]) {

//     // ساختار UI برنامه بدون استفاده از new
//     // تمام ویجت ها به صورت مقداری (by value) ساخته و کپی می شوند
//     ui::App app(
//         ui::Scaffold(
//             /* body: */
//             ui::Column({
//                 // 1. بخش Body که تمام فضای باقی مانده را می گیرد
//                 ui::Expanded(
//                     ui::Padding(
//                         ui::Column({
//                             ui::Text("Welcome to the new libfux!", {24, ui::Colors::black}),
//                             ui::Text("This demonstrates the Handle-Body pattern.", {16, ui::Colors::grey}),
//                             ui::Spacer(1) // فضای خالی انعطاف پذیر
//                         }, 10),
//                         {16, 16, 16, 16} // Padding
//                     )
//                 ),
//                 // 2. فوتر ثابت در پایین
//                 ui::Container(
//                     ui::Padding(
//                         ui::Row({
//                            ui::Text("Status: OK", {14, ui::Colors::white}),
//                            ui::Spacer(), // دکمه ها را به سمت راست هل می دهد
//                            ui::Text("Button 1", {14, ui::Colors::white}),
//                            ui::Text("Button 2", {14, ui::Colors::white})
//                         }, 20),
//                         {8, 16, 8, 16} // Padding
//                     ),
//                     { ui::Colors::blue } // BoxDecoration
//                 )
//             }),

//             /* appBar: */
//             ui::Container(
//                 ui::Padding(
//                     ui::Text("My App Title", {20, ui::Colors::white}),
//                     {0, 16, 0, 16} // Padding
//                 ),
//                 { ui::Colors::blue } // BoxDecoration
//             )
//         )
//     );

//     // اجرای برنامه با تنظیمات دلخواه در متد run()
//     app.run(
//         "Modern FUI App",                     // عنوان پنجره
//         { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 800 }, // اندازه
//         true,                                 // قابلیت تغییر سایز
//         ui::Colors::white                     // رنگ پس زمینه
//     );

//     return 0;
// }

//#include "libfux.hpp"
//
//// Global alias for easy access to the AppContext
//auto& context = ui::AppContext::instance();
//
//// States for the main UI
//ui::State<bool> showDialogState(false);
//ui::State<std::string> textBoxContent("");
//
//// =======================================================
//// Event Handlers
//// =======================================================
//void onShowDialogPressed() {
//    showDialogState.set(true);
//}
//
//void onDialogOkPressed() {
//    showDialogState.set(false);
//}
//
//// Global styles for our custom snackbars, defined by the user
//ui::Style successStyle;
//ui::Style errorStyle;
//
//void onShowSuccessPressed() {
//    std::string content = textBoxContent.get();
//    std::string message = "Success! You wrote: " + (content.empty() ? "[nothing]" : content);
//    // Call the powerful overload with our custom style
//    context.showSnackBar(message, successStyle, ui::SnackBarPosition::Top);
//}
//
//void onShowErrorPressed() {
//    // Call the powerful overload with our custom style
//    context.showSnackBar("This is a custom error message!", errorStyle, ui::SnackBarPosition::Top);
//}
//
//
//int main(int argc, char* argv[]) {
//
//    // =======================================================
//    // Style Definitions (User can define any style they want)
//    // =======================================================
//    successStyle.backgroundColor = ui::Colors::green;
//    successStyle.textStyle = { 16, ui::Colors::white };
//    successStyle.padding = { 12, 18, 12, 18 };
//    successStyle.border.radius = 8.0;
//
//    errorStyle.backgroundColor = ui::Colors::red;
//    errorStyle.textStyle = { 16, ui::Colors::white };
//    errorStyle.padding = { 12, 18, 12, 18 };
//    errorStyle.border.radius = 8.0;
//
//    // Other styles...
//    ui::Style primaryButtonStyle;
//    primaryButtonStyle.backgroundColor = ui::Colors::blue;
//    primaryButtonStyle.textStyle = { 16, ui::Colors::white };
//    primaryButtonStyle.padding = { 10, 15, 10, 15 };
//    primaryButtonStyle.border.radius = 8.0;
//
//    ui::Style dialogContainerStyle;
//    dialogContainerStyle.backgroundColor = ui::Colors::white;
//    dialogContainerStyle.padding = { 20, 20, 20, 20 };
//    dialogContainerStyle.border.radius = 12.0;
//
//    ui::Style textBoxStyle;
//    textBoxStyle.backgroundColor = { 230, 230, 230 };
//    textBoxStyle.padding = { 8, 12, 8, 12 };
//    textBoxStyle.border.radius = 8.0;
//
//    ui::Style mainContainerStyle;
//    mainContainerStyle.padding = { 20, 20, 20, 20 };
//
//    ui::Style okButtonStyle;
//    okButtonStyle.backgroundColor = ui::Colors::transparent;
//    okButtonStyle.textStyle = { 16, ui::Colors::blue };
//
//
//    // =======================================================
//    // App Structure
//    // =======================================================
//    ui::App app(
//        // IMPORTANT FIX: Provide the SnackBarConfig state to the entire UI tree
//        ui::ContextProvider<ui::SnackBarConfig>(context.snackBarConfig,
//            ui::ContextProvider<std::string>(textBoxContent,
//                ui::ContextProvider<bool>(showDialogState,
//                    ui::Stack({
//
//                        // Layer 1: The main app content
//                        ui::Container(
//                            ui::Column({
//                                ui::Text("FUX Library Demo", {24, ui::Colors::black}),
//                                ui::TextBox(textBoxContent, "Type something here...", textBoxStyle),
//                                ui::Row({
//                                    ui::TextButton("Show Dialog", onShowDialogPressed, primaryButtonStyle),
//                                    ui::TextButton("Show Success", onShowSuccessPressed, primaryButtonStyle),
//                                    ui::TextButton("Show Error", onShowErrorPressed, primaryButtonStyle)
//                                }, 10)
//                            }, 15),
//                            mainContainerStyle
//                        ),
//
//                            // Layer 2: The Dialog (conditional)
//                            ui::If(showDialogState, [=]() {
//                                return ui::DialogBox(
//                                    ui::Container(
//                                        ui::Column({
//                                            ui::Text("This is a Dialog", {20, ui::Colors::black}),
//                                            ui::Text("Click OK to close.", {14, ui::Colors::grey}),
//                                            ui::Container({}, {{10,0,0,0}}),
//                                            ui::TextButton("OK", onDialogOkPressed, okButtonStyle)
//                                        }, 15),
//                                        dialogContainerStyle
//                                    )
//                                );
//                            }),
//
//                            // Layer 3: The SnackBar (now correctly connected and managed)
//                            ui::If(context.isSnackBarVisible, []() {
//                                return ui::ContextBuilder<ui::SnackBarConfig>([](ui::SnackBarConfig config) -> ui::Widget {
//                                    return ui::SnackBar(
//                                        ui::Container(
//                                            ui::Text(config.message, config.style.textStyle),
//                                            config.style
//                                        ),
//                                        config.position
//                                    );
//                                });
//                            })
//                        })
//                )
//            )
//        )
//    );
//
//    app.run("FUX Simplified Demo", { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400 });
//
//    return 0;
//}


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
                ui::Text("FUX Library Demo", {24}),
                ui::TextBox(textBoxContent, "Type something here...", textBoxStyle),

                // UPDATED: Using the super-simple Obx widget for reactivity!
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



