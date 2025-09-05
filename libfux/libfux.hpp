// FUX UI Library - Creator Morteza Mansory
// Version 0.4.2
// Still in development version keep your ears up, for the new version.
//=----------------------------------------------=
// Whats New in this Version:

// Reactivity System Has Changed
// The new code uses a simpler reactivity system with Obx and g_currentlyBuildingWidget to track widgets.

// Layout Handling is Different
// In the new version, widget sizes are explicitly calculated using a performLayout function before rendering.

// Overlay Management Added
// The new code introduces m_overlayStack to show dialogs and snackbars, allowing multiple overlapping windows.

// Smarter Event Handling
// The new version uses hitTest to detect which widget should receive a click event (usually the top one).

// Improved Snackbar Timer
// In the new code, snackbar timers are managed centrally in the app’s main loop (App::run).

//  Global Helper Functions for Cleaner API
// In this new thing , i added some inline void function , to not using App::instance() anymore, so it would help you write cleaner and faster code.
// And re coding all project means from 900 lines its decrease to ~620 lines! don't know should i celebrate or ...anyway.
//=----------------------------------------------=
#pragma once

#ifdef HIDE_CONSOLE
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <algorithm>
#include <any>
#include <set>
#include <typeinfo>
#include <chrono>
#include <thread>
#include <utility>
#include <optional>

namespace ui {

    class App;
    class Widget;
    class WidgetBody;
    class IRenderer;
    class SDLRenderer;
    class RebuildRequester;
    class DialogBox;
    class SnackBar;

    struct Color { uint8_t r, g, b, a = 255; };
    struct TextStyle { int fontSize = 16; Color color = { 0, 0, 0 }; std::string fontFile; };
    struct EdgeInsets { int top = 0, right = 0, bottom = 0, left = 0; };

    struct BorderRadius {
        double topLeft = 0.0, topRight = 0.0, bottomLeft = 0.0, bottomRight = 0.0;
        static BorderRadius all(double radius) {
            return { radius, radius, radius, radius };
        }
    };

    struct Border { Color color = { 0,0,0,0 }; int width = 0; BorderRadius radius = {}; };

    struct Style {
        Color backgroundColor = { 0, 0, 0, 0 };
        Border border = {};
        TextStyle textStyle = {};
        EdgeInsets padding = {};
    };

    enum class SnackBarPosition { Bottom, Top };

    namespace Colors {
        constexpr Color transparent = { 0, 0, 0, 0 };
        constexpr Color black = { 0, 0, 0 };
        constexpr Color white = { 255, 255, 255 };
        constexpr Color blue = { 33, 150, 243 };
        constexpr Color lightBlue = { 66, 165, 245 };
        constexpr Color green = { 76, 175, 80 };
        constexpr Color red = { 244, 67, 54 };
        constexpr Color grey = { 158, 158, 158 };
        constexpr Color darkGrey = { 40, 40, 40 };
    }

    class RebuildRequester {
    public:
        virtual ~RebuildRequester() = default;
        virtual void rebuild() = 0;
    };

    inline std::weak_ptr<RebuildRequester> g_currentlyBuildingWidget;

    template<typename T>
    class State {
    public:
        State(T initialValue) : m_value(initialValue) {}

        const T& get() const {
            if (auto listener = g_currentlyBuildingWidget.lock()) {
                m_listeners.push_back(listener);
            }
            return m_value;
        }

        void set(T newValue) {
            if (m_value == newValue) return;
            m_value = newValue;

            std::set<std::shared_ptr<RebuildRequester>> unique_listeners;
            m_listeners.erase(std::remove_if(m_listeners.begin(), m_listeners.end(),
                [&](const std::weak_ptr<RebuildRequester>& weak_ptr) {
                    if (auto shared_ptr = weak_ptr.lock()) {
                        unique_listeners.insert(shared_ptr);
                        return false; 
                    }
                    return true; 
                }), m_listeners.end());

            for (const auto& listener : unique_listeners) {
                listener->rebuild();
            }
        }

    private:
        T m_value;
        mutable std::vector<std::weak_ptr<RebuildRequester>> m_listeners;
    };

    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        virtual bool init(SDL_Window* window) = 0;
        virtual void clear(Color color) = 0;
        virtual void present() = 0;
        virtual void drawRect(const SDL_Rect& rect, Color color, const BorderRadius& radius) = 0;
        virtual void drawText(const std::string& text, const TextStyle& style, int x, int y) = 0;
        virtual SDL_Point getTextSize(const std::string& text, const TextStyle& style) = 0;
    };

    class WidgetBody : public std::enable_shared_from_this<WidgetBody> {
    public:
        SDL_Rect m_allocatedSize = { 0, 0, 0, 0 };
        WidgetBody* parent = nullptr;
        virtual ~WidgetBody() = default;
        virtual void performLayout(IRenderer* renderer, SDL_Rect constraints) = 0;
        virtual void render(App* app, IRenderer* renderer) = 0;
        virtual WidgetBody* hitTest(SDL_Point point) {
            return SDL_PointInRect(&point, &m_allocatedSize) ? this : nullptr;
        }
        virtual void handleEvent(App* app, SDL_Event* event) {}
    };

    class Widget {
    protected:
        std::shared_ptr<WidgetBody> p_impl;
    public:
        Widget(std::shared_ptr<WidgetBody> impl = nullptr) : p_impl(impl) {}
        WidgetBody* operator->() const { return p_impl.get(); }
        std::shared_ptr<WidgetBody> getImpl() const { return p_impl; }
        explicit operator bool() const { return p_impl != nullptr; }
    };

    class App {
    public:
        App(Widget root);
        ~App();

        void run(const std::string& title = "FUX App", bool resizable = false, SDL_Point size = { 800, 600 });

        static App* instance() { return s_instance; }

        void pushOverlay(Widget widget);
        void popOverlay();
        void addTimer(unsigned int ms, std::function<void()> callback);

        void requestFocus(WidgetBody* widget) { m_focusedWidget = widget; }
        void releaseFocus(WidgetBody* widget) { if (m_focusedWidget == widget) m_focusedWidget = nullptr; }
        void markNeedsLayoutUpdate() { m_needs_layout_update = true; }
    private:
        void internal_run(const std::string& title, SDL_Rect size, bool resizable, Color backgroundColor, const std::string& defaultFont);
        struct Timer { std::chrono::steady_clock::time_point expiryTime; std::function<void()> callback; };
        Widget m_root_handle;
        std::shared_ptr<WidgetBody> m_root_body;
        SDL_Window* m_window = nullptr;
        std::unique_ptr<IRenderer> m_renderer;
        std::vector<std::shared_ptr<WidgetBody>> m_overlayStack;
        static App* s_instance;
        std::vector<Timer> m_timers;
        WidgetBody* m_focusedWidget = nullptr;
        bool m_needs_layout_update = true;
    };
    inline App* App::s_instance = nullptr;

    class SDLRenderer : public IRenderer {
    private:
        SDL_Renderer* m_renderer = nullptr;
        std::map<std::string, TTF_Font*> m_fontCache;
        std::string m_defaultFontFile;
        TTF_Font* getFont(const std::string& fontFile, int size) {
            std::string actualFontFile = fontFile.empty() ? m_defaultFontFile : fontFile;
            if (actualFontFile.empty()) return nullptr;
            std::string key = actualFontFile + std::to_string(size);
            if (m_fontCache.find(key) == m_fontCache.end()) {
                TTF_Font* font = TTF_OpenFont(actualFontFile.c_str(), size);
                if (!font) {
                    std::cerr << "WARN: Failed to load font '" << actualFontFile << "'. Trying fallback." << std::endl;
                    font = TTF_OpenFont("C:/Windows/Fonts/Arial.ttf", size);
                    if (!font) { std::cerr << "CRITICAL: Could not load ANY font." << std::endl; return nullptr; }
                }
                m_fontCache[key] = font;
            }
            return m_fontCache[key];
        }
    public:
        SDLRenderer(std::string defaultFont) : m_defaultFontFile(std::move(defaultFont)) {}
        ~SDLRenderer() {
            for (auto const& [key, val] : m_fontCache) { if (val) TTF_CloseFont(val); }
            if (m_renderer) SDL_DestroyRenderer(m_renderer);
        }
        bool init(SDL_Window* window) override {
            m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!m_renderer) return false;
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            return true;
        }
        void clear(Color color) override {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            SDL_RenderClear(m_renderer);
        }
        void present() override { SDL_RenderPresent(m_renderer); }
        void drawRect(const SDL_Rect& rect, Color color, const BorderRadius& radius) override {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(m_renderer, &rect);
        }
        void drawText(const std::string& text, const TextStyle& style, int x, int y) override {
            if (text.empty()) return;
            TTF_Font* font = getFont(style.fontFile, style.fontSize); if (!font) return;
            SDL_Color c = { style.color.r, style.color.g, style.color.b, style.color.a };
            SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), c); if (!surface) return;
            SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
            SDL_Rect dstRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(m_renderer, texture, nullptr, &dstRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }
        SDL_Point getTextSize(const std::string& text, const TextStyle& style) override {
            if (text.empty()) return { 0, style.fontSize };
            TTF_Font* font = getFont(style.fontFile, style.fontSize);
            if (!font) return { 0, style.fontSize };
            int w, h;
            TTF_SizeText(font, text.c_str(), &w, &h);
            return { w, h };
        }
    };

    inline App::App(Widget root) : m_root_handle(root) { s_instance = this; }
    inline App::~App() {
        m_renderer.reset();
        if (m_window) SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        s_instance = nullptr;
    }
    inline void App::pushOverlay(Widget widget) {
        m_overlayStack.push_back(widget.getImpl());
        markNeedsLayoutUpdate();
    }
    inline void App::popOverlay() {
        if (!m_overlayStack.empty()) {
            m_overlayStack.pop_back();
            markNeedsLayoutUpdate();
        }
    }
    inline void App::addTimer(unsigned int ms, std::function<void()> callback) {
        m_timers.push_back({ std::chrono::steady_clock::now() + std::chrono::milliseconds(ms), std::move(callback) });
    }
    inline void App::run(const std::string& title, bool resizable, SDL_Point size) {
        SDL_Rect window_rect = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.x, size.y };
        internal_run(title, window_rect, resizable, Colors::white, "arial.ttf");
    }
    inline void App::internal_run(const std::string& title, SDL_Rect size, bool resizable, Color backgroundColor, const std::string& defaultFont) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) return;
        if (TTF_Init() == -1) return;

        Uint32 window_flags = SDL_WINDOW_SHOWN | (resizable ? SDL_WINDOW_RESIZABLE : 0);
        m_window = SDL_CreateWindow(title.c_str(), size.x, size.y, size.w, size.h, window_flags);
        m_renderer = std::make_unique<SDLRenderer>(defaultFont);
        if (!m_window || !m_renderer->init(m_window)) { return; }
        m_root_body = m_root_handle.getImpl();

        bool running = true;
        while (running) {
            auto now = std::chrono::steady_clock::now();
            std::vector<std::function<void()>> callbacks_to_run;

            m_timers.erase(std::remove_if(m_timers.begin(), m_timers.end(),
                [&](Timer& timer) {
                    if (now >= timer.expiryTime) {
                        callbacks_to_run.push_back(std::move(timer.callback));
                        return true;
                    }
                    return false;
                }), m_timers.end());

            for (const auto& callback : callbacks_to_run) {
                callback();
            }

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                if (m_needs_layout_update) continue;

                SDL_Point mousePos = { event.motion.x, event.motion.y };
                WidgetBody* target = nullptr;

                if (!m_overlayStack.empty()) {
                    for (auto it = m_overlayStack.rbegin(); it != m_overlayStack.rend(); ++it) {
                        target = (*it)->hitTest(mousePos);
                        if (target) break;
                    }
                }

                if (!target && m_root_body) {
                    target = m_root_body->hitTest(mousePos);
                }

                if (target) {
                    target->handleEvent(this, &event);
                }
                else if (m_focusedWidget) {
                    m_focusedWidget->handleEvent(this, &event);
                }
            }

            if (m_needs_layout_update) {
                int w, h; SDL_GetWindowSize(m_window, &w, &h); SDL_Rect windowRect = { 0, 0, w, h };
                if (m_root_body) m_root_body->performLayout(m_renderer.get(), windowRect);
                for (const auto& overlay : m_overlayStack) { overlay->performLayout(m_renderer.get(), windowRect); }
                m_needs_layout_update = false;
            }

            m_renderer->clear(backgroundColor);
            if (m_root_body) m_root_body->render(this, m_renderer.get());
            for (const auto& overlay : m_overlayStack) { overlay->render(this, m_renderer.get()); }
            m_renderer->present();

            SDL_Delay(16);
        }
    }

    // === All Widgets ===

    class TextImpl : public WidgetBody {
    public:
        std::string text; TextStyle style;
        TextImpl(std::string t, TextStyle s) : text(std::move(t)), style(std::move(s)) {}
        void performLayout(IRenderer* r, SDL_Rect c) override { SDL_Point s = r->getTextSize(text, style); m_allocatedSize = { c.x, c.y, s.x, s.y }; }
        void render(App* a, IRenderer* r) override { r->drawText(text, style, m_allocatedSize.x, m_allocatedSize.y); }
        WidgetBody* hitTest(SDL_Point p) override { return nullptr; }
    };
    class Text : public Widget {
    public:
        Text(const std::string& text = "", TextStyle style = {}) : Widget(std::make_shared<TextImpl>(text, style)) {}
        Text(const std::string& text, int fontSize) : Widget(std::make_shared<TextImpl>(text, TextStyle{ fontSize })) {}
    };

    class ContainerImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child; Style style;
        ContainerImpl(Widget c, Style s) : style(std::move(s)) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (child) child->performLayout(r, { c.x + style.padding.left, c.y + style.padding.top, c.w - (style.padding.left + style.padding.right), c.h - (style.padding.top + style.padding.bottom) }); }
        void render(App* a, IRenderer* r) override { if (style.backgroundColor.a > 0) r->drawRect(m_allocatedSize, style.backgroundColor, style.border.radius); if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override { if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr; if (child) return child->hitTest(p); return this; }
    };
    class Container : public Widget {
    public:
        Container(Widget child, Style s = {}) : Widget(std::make_shared<ContainerImpl>(child, std::move(s))) {}
    };

    class ObxImpl : public WidgetBody, public RebuildRequester {
        std::function<Widget()> m_builder;
        std::shared_ptr<WidgetBody> m_child;
    public:
        ObxImpl(std::function<Widget()> builder) : m_builder(std::move(builder)) {}
        void initialize() { buildChild(); }
        void buildChild() {
            auto self_as_derived = std::static_pointer_cast<ObxImpl>(shared_from_this());
            g_currentlyBuildingWidget = self_as_derived;
            Widget new_widget = m_builder();
            g_currentlyBuildingWidget.reset();
            m_child = new_widget.getImpl();
            if (m_child) m_child->parent = this;
            if (App::instance()) App::instance()->markNeedsLayoutUpdate();
        }
        void rebuild() override { buildChild(); }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (m_child) m_child->performLayout(r, c); }
        void render(App* a, IRenderer* r) override { if (m_child) m_child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override { if (m_child) return m_child->hitTest(p); return nullptr; }
        void handleEvent(App* a, SDL_Event* e) override { if (m_child) m_child->handleEvent(a, e); }
    };
    class Obx : public Widget {
    public:
        Obx(std::function<Widget()> builder) {
            auto impl = std::make_shared<ObxImpl>(std::move(builder));
            impl->initialize();
            p_impl = impl;
        }
    };
    // Deduction guide was removed for compatibility. User must specify lambda return type.

    class ColumnImpl : public WidgetBody {
    public:
        std::vector<std::shared_ptr<WidgetBody>> children; int spacing;
        ColumnImpl(std::initializer_list<Widget> c, int s) : spacing(s) { for (const auto& w : c) if (auto i = w.getImpl()) { children.push_back(i); i->parent = this; } }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            int y = c.y;
            for (const auto& ch : children) {
                if (!ch) continue;
                int h = 20;
                if (auto t = std::dynamic_pointer_cast<TextImpl>(ch)) {
                    h = r->getTextSize(t->text, t->style).y;
                }
                ch->performLayout(r, { c.x, y, c.w, h });
                y += ch->m_allocatedSize.h + spacing;
            }
        }
        void render(App* a, IRenderer* r) override { for (const auto& c : children) if (c) c->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            for (const auto& child : children) {
                if (child) {
                    if (WidgetBody* target = child->hitTest(p)) {
                        return target;
                    }
                }
            }
            return nullptr;
        }
    };
    class Column : public Widget {
    public:
        Column(std::initializer_list<Widget> c, int s = 0) : Widget(std::make_shared<ColumnImpl>(c, s)) {}
    };

    class RowImpl : public WidgetBody {
    public:
        std::vector<std::shared_ptr<WidgetBody>> children; int spacing;
        RowImpl(std::initializer_list<Widget> c, int s) : spacing(s) { for (const auto& w : c) if (auto i = w.getImpl()) { children.push_back(i); i->parent = this; } }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; int x = c.x; int w = children.empty() ? 0 : (c.w - (static_cast<int>(children.size()) - 1) * spacing) / static_cast<int>(children.size()); for (const auto& ch : children) { if (!ch) continue; ch->performLayout(r, { x, c.y, w, c.h }); x += w + spacing; } }
        void render(App* a, IRenderer* r) override { for (const auto& c : children) if (c) c->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            for (const auto& child : children) {
                if (child) {
                    if (WidgetBody* target = child->hitTest(p)) {
                        return target;
                    }
                }
            }
            return nullptr;
        }
    };
    class Row : public Widget {
    public:
        Row(std::initializer_list<Widget> c, int s = 0) : Widget(std::make_shared<RowImpl>(c, s)) {}
    };

    class ButtonImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child; std::function<void()> onPressed; Style style; bool isHovered = false;
        ButtonImpl(Widget c, std::function<void()> o, Style s) : onPressed(std::move(o)), style(std::move(s)) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (child) { SDL_Point childSize = { 0,0 }; if (auto text_impl = std::dynamic_pointer_cast<TextImpl>(child)) { childSize = r->getTextSize(text_impl->text, text_impl->style); } int cX = c.x + (c.w - childSize.x) / 2; int cY = c.y + (c.h - childSize.y) / 2; child->performLayout(r, { cX, cY, childSize.x, childSize.y }); } }
        void render(App* a, IRenderer* r) override { Color bg = style.backgroundColor; if (isHovered) { bg.r = (uint8_t)std::max(0, (int)bg.r - 20); bg.g = (uint8_t)std::max(0, (int)bg.g - 20); bg.b = (uint8_t)std::max(0, (int)bg.b - 20); } r->drawRect(m_allocatedSize, bg, style.border.radius); if (child) child->render(a, r); }
        void handleEvent(App*, SDL_Event* e) override {
            if (e->type == SDL_MOUSEMOTION) {
                SDL_Point mousePos = { e->motion.x, e->motion.y };
                isHovered = SDL_PointInRect(&mousePos, &m_allocatedSize);
            }
            if (e->type == SDL_MOUSEBUTTONDOWN && isHovered && onPressed) {
                onPressed();
            }
        }
    };
    class TextButton : public Widget {
    public:
        TextButton(const std::string& t, std::function<void()> o, Style s = {}) : Widget(std::make_shared<ButtonImpl>(Text(t, s.textStyle), std::move(o), std::move(s))) {}
    };

    class TextBoxImpl : public WidgetBody {
        State<std::string>& state_ref;
        std::string m_localText;
        std::string hintText;
        Style style;
        bool isFocused = false;
    public:
        TextBoxImpl(State<std::string>& s, std::string h, Style st)
            : state_ref(s), m_localText(s.get()), hintText(std::move(h)), style(std::move(st)) {
        }
        ~TextBoxImpl() {
            if (isFocused) {
                SDL_StopTextInput();
                if (App::instance()) App::instance()->releaseFocus(this);
            }
        }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            int h = r->getTextSize("Gg", style.textStyle).y;
            m_allocatedSize = { c.x, c.y, c.w, h + style.padding.top + style.padding.bottom };
        }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePos = { e->button.x, e->button.y };
                bool currentlyFocused = SDL_PointInRect(&mousePos, &m_allocatedSize);
                if (currentlyFocused && !isFocused) {
                    isFocused = true;
                    SDL_StartTextInput();
                    a->requestFocus(this);
                }
                else if (!currentlyFocused && isFocused) {
                    isFocused = false;
                    SDL_StopTextInput();
                    a->releaseFocus(this);
                }
            }
            if (isFocused) {
                bool changed = false;
                if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_BACKSPACE && !m_localText.empty()) {
                    m_localText.pop_back();
                    changed = true;
                }
                else if (e->type == SDL_TEXTINPUT) {
                    m_localText += e->text.text;
                    changed = true;
                }
                if (changed) {
                    state_ref.set(m_localText);
                }
            }
        }
        void render(App* a, IRenderer* r) override {
            m_localText = state_ref.get();
            r->drawRect(m_allocatedSize, style.backgroundColor, style.border.radius);
            std::string displayText = m_localText.empty() ? hintText : m_localText;
            TextStyle ts = style.textStyle;
            if (m_localText.empty()) ts.color = Colors::grey;
            SDL_Point tsz = r->getTextSize(displayText, ts);
            r->drawText(displayText, ts, m_allocatedSize.x + style.padding.left, m_allocatedSize.y + (m_allocatedSize.h - tsz.y) / 2);
            if (isFocused && (SDL_GetTicks() / 500) % 2) {
                SDL_Point trs = r->getTextSize(m_localText, style.textStyle);
                SDL_Rect cursor = { m_allocatedSize.x + style.padding.left + trs.x, m_allocatedSize.y + style.padding.top, 2, m_allocatedSize.h - (style.padding.top + style.padding.bottom) };
                r->drawRect(cursor, ts.color, {});
            }
        }
    };
    class TextBox : public Widget {
    public:
        TextBox(State<std::string>& s, std::string h = "...", Style st = {})
            : Widget(std::make_shared<TextBoxImpl>(s, std::move(h), std::move(st))) {
        }
    };

    class DialogBoxImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
    public:
        DialogBoxImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (child) { int cw = 300, ch = 150; child->performLayout(r, { c.x + (c.w - cw) / 2, c.y + (c.h - ch) / 2, cw, ch }); } }
        void render(App* a, IRenderer* r) override { r->drawRect(m_allocatedSize, { 0,0,0,128 }, {}); if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            if (child) {
                WidgetBody* t = child->hitTest(p);
                if (t) return t;
            }
            return this;
        }
        void handleEvent(App* a, SDL_Event* e) override {
            if (child) child->handleEvent(a, e);
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                e->type = SDL_USEREVENT;
            }
        }
    };
    class DialogBox : public Widget {
    public:
        DialogBox(Widget child) : Widget(std::make_shared<DialogBoxImpl>(child)) {}
    };

    class SnackBarImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child; SnackBarPosition position;
    public:
        SnackBarImpl(Widget c, SnackBarPosition p) : position(p) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (child) { int cw = 400, ch = 50; int cx = c.x + (c.w - cw) / 2; int cy = (position == SnackBarPosition::Bottom) ? c.y + c.h - ch - 20 : c.y + 20; child->performLayout(r, { cx, cy, cw, ch }); } }
        void render(App* a, IRenderer* r) override { if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override { return nullptr; }
    };
    class SnackBar : public Widget {
    public:
        SnackBar(Widget child, SnackBarPosition p = SnackBarPosition::Bottom) : Widget(std::make_shared<SnackBarImpl>(child, p)) {}
    };

    class ScaffoldImpl : public ContainerImpl {
    public:
        ScaffoldImpl(Widget child, Style style) : ContainerImpl(child, std::move(style)) {}
    };
    class Scaffold : public Widget {
    public:
        Scaffold(Widget child, Style s = {}) : Widget(std::make_shared<ScaffoldImpl>(child, std::move(s))) {}
    };

    // === Global Helper Functions for Cleaner API ===
    inline void popOverlay() {
        if (App::instance()) App::instance()->popOverlay();
    }

    inline void showDialog(Widget dialogContent) {
        if (App::instance()) App::instance()->pushOverlay(DialogBox(dialogContent));
    }

    inline void showSnackBar(const std::string& message) {
        if (!App::instance()) return;

        Style defaultStyle;
        defaultStyle.backgroundColor = Colors::darkGrey;
        defaultStyle.textStyle.color = Colors::white;
        defaultStyle.padding = { 12, 18, 12, 18 };
        defaultStyle.border.radius = BorderRadius::all(6.0);

        auto snackBarWidget = SnackBar(Container(Text(message, defaultStyle.textStyle), defaultStyle));
        App::instance()->pushOverlay(snackBarWidget);
        App::instance()->addTimer(3000, [] { popOverlay(); });
    }

} // namespace ui
