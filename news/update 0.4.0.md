What's New in the 0.4.0 Version?

The new version of the code is much simpler, cleaner, and easier to understand.
Here are the main improvements compared to the old version:

1. ✅ No More ContextProvider / ContextBuilder

Before: You had to wrap widgets with ContextProvider and use ContextBuilder to access shared data.
Now: You can access everything directly using AppContext::instance().

➡️ Much easier to read and write.

2. ✅ Dialogs Made Super Simple

Before: You had to manage dialog state manually with a State<bool> and use If(...) to show or hide it.
Now: Just call ui::showDialog(...) and you're done. To close, call popOverlay().

➡️ No more state management for dialogs.

3. ✅ Easy Reactivity with Obx

Before: Reactivity required context setup and manual updates.
Now: Use Obx([] { ... }) to automatically update widgets when state changes.

➡️ Simple, clean, and reactive UI with one line.

4. ✅ One-Line SnackBar

Before: Showing a SnackBar needed a custom config, context, and layout.
Now: Just call:

ui::AppContext::instance().showSnackBar("This is a simple SnackBar!");


➡️ No extra setup, works out of the box.

5. ✅ Cleaner UI Structure

Before: Lots of nested widgets like Stack, If, and multiple providers.
Now: Just use Scaffold, Column, Row, and it's super readable.

➡️ Looks like how you think about UI layout.