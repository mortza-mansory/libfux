#include "libfux.hpp"

// Global state for the TextBox.
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
   dialogContainerStyle.border.radius = ui::BorderRadius::all(12.0);

   ui::showDialog(
       ui::Container(
           ui::Column({
               ui::Text("This is a Dialog"),
               ui::Text("Click OK to close."),
               ui::TextButton("OK", [] {
                   ui::popOverlay();
               }, buttonStyle)
               }, 10),
           dialogContainerStyle
       )
   );
}

void onShowSnackBarPressed() {
   ui::showSnackBar("This is a simple SnackBar!");
}

int main(int argc, char* argv[]) {
   ui::Style primaryButtonStyle;
   primaryButtonStyle.backgroundColor = ui::Colors::blue;
   primaryButtonStyle.textStyle.color = ui::Colors::white;
   primaryButtonStyle.padding = { 0, 0, 0, 0 };
   primaryButtonStyle.border.radius = ui::BorderRadius::all(100.0);

   ui::Style successButtonStyle;
   successButtonStyle.backgroundColor = ui::Colors::green;
   successButtonStyle.textStyle.color = ui::Colors::white;
   successButtonStyle.padding = { 0, 0, 0, 0 };
   successButtonStyle.border.radius = ui::BorderRadius::all(8.0);

   ui::Style mainScaffoldStyle;
   mainScaffoldStyle.padding = { 0, 0, 0, 0 };

   ui::Style textBoxStyle;
   textBoxStyle.backgroundColor = { 230, 230, 230 };
   textBoxStyle.padding = { 0, 0, 0, 0 };
   textBoxStyle.border.radius = ui::BorderRadius::all(4);

   ui::App app(
       ui::Scaffold(
           ui::Column({
               ui::Text("FUX Library Demo", {24}),
               ui::TextBox(textBoxContent, "Type something here...", textBoxStyle),

               ui::Obx([] {
                   std::string text = textBoxContent.get();
                   if (text.empty()) {
                       return ui::Widget();
                   }
                   return ui::Widget(ui::Text("You wrote: " + text));
               }),

               ui::Row({
                   ui::SizedBox(
                       ui::TextButton("Show Dialog", onShowDialogPressed, primaryButtonStyle),
                       {/*width*/ 200, /*height*/ 50}
                   ),
                   ui::TextButton("Show SnackBar", onShowSnackBarPressed, successButtonStyle)
               }, 10)

               }, 15),
           mainScaffoldStyle
       )
   );

   app.run("FUX Demo 3.0", true, { 600, 450 });

   return 0;
}