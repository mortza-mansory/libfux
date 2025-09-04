# libfux
<< Summery >>LibFux its my personal _flutter like_ gui maker for my cpp projects.
Its simplify the process of creating ui without anything unrelated to the ui you can make easily UI's free fast as possible!

Currently in dev,..( I will update it as soon as i have free time, contact me or pull request is available. )


What's New in FUX UI

1. Easy App-Wide Actions with context
There is a new global object named context. You can use it from anywhere in your code to perform common actions without managing complex states.

Its main use is to show notifications like SnackBars with one simple line of code.

How to Use It:
You can now show different kinds of messages easily. The context object handles all the style and state logic for you.

C++
```
// Show a simple, default message at the bottom
context.showSnackBar("Your profile has been updated.");

// Define your own style
ui::Style successStyle;
successStyle.backgroundColor = ui-::Colors::green;
successStyle.textStyle = { 16, ui::Colors::white };

// Show a custom "success" message at the top
context.showSnackBar("Login successful!", successStyle, ui::SnackBarPosition::Top);
```
2. Simple Conditional UI with ui::If
Showing or hiding a widget based on a state is now very simple. You no longer need to write complex ContextProvider and ContextBuilder code. Just use the new ui::If widget.

Example:
This code will only show the DialogBox when the showDialogState variable is true.
```
// In your main UI structure
ui::Stack({
    // ... your main content ...

    // This DialogBox is now conditional
    ui::If(showDialogState, [=]() {
        return ui::DialogBox(
            ui::Container(...)
        );
    }),

    // ... other overlays ...
})
```
3. Major Stability Fixes
No More Crashes: We fixed a major bug that caused the app to crash when showing a SnackBar for the second time. The state management system is now much safer.

Improved Reliability: Several other compiler and runtime errors have been fixed. The library is now more stable and reliable.


Example: The main.cpp is your example only put it on your visual studio, config the SDL lib and enjoy power of simplify.

Creator: Morteza Mansory
Current Version: 0.28.2 { Currently In Dev }
