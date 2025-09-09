#include "libfux.hpp"

using namespace ui;

Widget Title(const std::string& text) {
    return Text(text, { 20, Colors::darkGrey });
}

int main(int argc, char* argv[]) {
    State<std::string> textValue("You can edit this!");
    State<bool> isChecked(true);
    State<double> sliderValue(75.0);
    State<int> counter(0);

    auto widgetGallery = Scaffold(
        Container(
            ScrollView(
                Column({
                    // Foundational Widgets
                    Title("Foundational Widgets"),
                    Text("This is a simple Text widget."),
                    SizedBox({}, {-1, 5}),
                    Text("This Text has a different font size.", 24),
                    SizedBox({}, {-1, 10}),
                    Container(
                        Text("This Text is inside a blue Container with padding.", {16, Colors::white}),
                        Style{
                            .backgroundColor = Colors::lightBlue,
                            .border = {.radius = BorderRadius::all(8.0) },
                            .padding = {10, 15, 10, 15}
                        }
                    ),
                    SizedBox({}, {-1, 20}),
                    Divider(),
                    SizedBox({}, {-1, 20}),

                    // Layout Widgets
                    Title("Layout Widgets"),
                    Row({
                        Container(Text("Item 1"), {
                            .backgroundColor = {220, 220, 220},
                            .padding = {8, 8, 8, 8}
                        }),
                        Container(Text("Item 2"), {
                            .backgroundColor = {220, 220, 220},
                            .padding = {8, 8, 8, 8}
                        }),
                        Container(Text("Item 3"), {
                            .backgroundColor = {220, 220, 220},
                            .padding = {8, 8, 8, 8}
                        })
                    }, 10),
                    SizedBox({}, {-1, 10}),
                    Center(Text("This text is Centered.")),
                    SizedBox({}, {-1, 10}),
                    Stack({
                        SizedBox(Widget(), {120, 120}),
                        Container(Widget(), {.backgroundColor = Colors::red }),
                        Positioned(Container(Widget(), {.backgroundColor = {0, 0, 255, 150} }), 10, 10, 10, 10),
                        Positioned(Text("Positioned", {16, Colors::white}), 50, 20)
                    }),
                    SizedBox({}, {-1, 20}),
                    Divider(),
                    SizedBox({}, {-1, 20}),

                    // Interactive Widgets
                    Title("Interactive Widgets"),
                    TextBox(textValue, "Enter text here...", {
                        .backgroundColor = {240, 240, 240},
                        .padding = {8, 8, 8, 8}
                    }),
                    SizedBox({}, {-1, 10}),
                    Row({
                        Checkbox(isChecked),
                        Text("Enable Feature")
                    }, 10),
                    SizedBox({}, {-1, 10}),
                    Slider(sliderValue, 0.0, 100.0),
                    SizedBox({}, {-1, 10}),
                    TextButton("Increment Counter", [&]() {
                        counter.set(counter.get() + 1);
                    }),
                    SizedBox({}, {-1, 10}),
                    // IconButton (اختیاری فعال شود)
                    // IconButton("icon.png", [&]() {
                    //     showSnackBar("Icon button pressed!");
                    // }, {.padding = {5, 5, 5, 5}}),
                    SizedBox({}, {-1, 20}),
                    Divider(),
                    SizedBox({}, {-1, 20}),

                    // Reactive Widgets (Obx)
                    Title("Reactive Widgets (Obx)"),
                    Obx([&]() {
                        return Text("Button pressed " + std::to_string(counter.get()) + " times.");
                    }),
                    Obx([&]() {
                        return ProgressBar(sliderValue.get() / 100.0);
                    }),
                    Obx([&]() {
                        if (isChecked.get()) {
                            return Text("The feature is currently ENABLED.", {16, Colors::green});
                        }
 else {
  return Text("The feature is currently DISABLED.", {16, Colors::red});
}
}),
SizedBox({}, {-1, 20}),
Divider(),
SizedBox({}, {-1, 20}),

Title("Overlays"),
Row({
    TextButton("Show Dialog", []() {
        showDialog(
            Container(
                Column({
                    Text("This is a Dialog Box"),
                    SizedBox({}, {-1, 20}),
                    TextButton("Close", []() {
                        popOverlay();
                    }, {
                        .backgroundColor = Colors::blue,
                        .textStyle = {16, Colors::white}
                    })
                }, 10),
                Style{
                    .backgroundColor = Colors::white,
                    .border = {.radius = BorderRadius::all(10.0) },
                    .padding = {20, 20, 20, 20}
                }
            )
        );
    }),
    TextButton("Show SnackBar", []() {
        showSnackBar("This is a SnackBar message!");
    })
}, 10)
                    }, 15)
            ),
            Style{
                .backgroundColor = {245, 245, 245},
                .padding = {20, 20, 20, 20}
            }
        )
    );

    App myApp(widgetGallery);
    myApp.run("LibFux Widget Gallery", true, { 600, 800 });

    return 0;
}
