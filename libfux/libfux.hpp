// LibFux - Creator Morteza Mansory
// Version 0.5.0
// This version expands the widget library and adds core new features.
//=----------------------------------------------=
// Whats New in this Version:

// Rich Widget Expansion: Introduced a dozen new widgets including Image, Stack, ScrollView, Slider, and Checkbox.

// Image Support Added: Integrated the SDL_image library for native image rendering via new Image and IconButton widgets.

// Advanced Layout Control: Added powerful layout widgets like Stack, Positioned, and Center for more complex, layered UIs.

// Enhanced Robustness: Implemented detailed error logging during initialization and added more cross-platform font fallbacks.

// Interactive Element Overhaul: Added new interactive elements like Slider, Checkbox, and ScrollView for richer user input.
//=----------------------------------------------=
#pragma once

#ifdef HIDE_CONSOLE
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL_image.h>
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
    class PositionedImpl; // Forward declaration

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

    struct Size {
        int width = -1;
        int height = -1;
    };

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
        void clear(Color color);
        void present();
        virtual void drawRect(const SDL_Rect& rect, Color color, const BorderRadius& radius) = 0;
        virtual void drawLine(int x1, int y1, int x2, int y2, Color color) = 0;
        virtual void drawText(const std::string& text, const TextStyle& style, int x, int y) = 0;
        virtual SDL_Point getTextSize(const std::string& text, const TextStyle& style) = 0;
        virtual SDL_Texture* loadImage(const std::string& path) = 0;
        virtual void drawImage(SDL_Texture* texture, const SDL_Rect& dstRect) = 0;
        virtual SDL_Point getImageSize(SDL_Texture* texture) = 0;
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
        std::unique_ptr<SDLRenderer> m_renderer;
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
        std::map<std::string, SDL_Texture*> m_imageCache;
        std::string m_defaultFontFile;

        void fillCircle(int x, int y, int radius, Color color) {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            for (int w = 0; w < radius * 2; w++) {
                for (int h = 0; h < radius * 2; h++) {
                    int dx = radius - w;
                    int dy = radius - h;
                    if ((dx * dx + dy * dy) <= (radius * radius)) {
                        SDL_RenderDrawPoint(m_renderer, x + dx, y + dy);
                    }
                }
            }
        }

        TTF_Font* getFont(const std::string& fontFile, int size) {
            std::string actualFontFile = fontFile.empty() ? m_defaultFontFile : fontFile;
            if (actualFontFile.empty()) return nullptr;
            std::string key = actualFontFile + std::to_string(size);
            if (m_fontCache.find(key) == m_fontCache.end()) {
                TTF_Font* font = TTF_OpenFont(actualFontFile.c_str(), size);
                if (!font) {
                    std::cerr << "WARN: Failed to load font '" << actualFontFile << "'. Trying fallbacks." << std::endl;
                    // Windows
                    font = TTF_OpenFont("C:/Windows/Fonts/Arial.ttf", size);
                    // macOS
                    if (!font) font = TTF_OpenFont("/System/Library/Fonts/Supplemental/Arial.ttf", size);
                    // Linux
                    if (!font) font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", size);
                    if (!font) { std::cerr << "CRITICAL: Could not load ANY font." << std::endl; return nullptr; }
                }
                m_fontCache[key] = font;
            }
            return m_fontCache[key];
        }
    public:
        SDL_Renderer* getSDLRenderer() { return m_renderer; }
        SDLRenderer(std::string defaultFont) : m_defaultFontFile(std::move(defaultFont)) {}
        ~SDLRenderer() {
            for (auto const& [key, val] : m_fontCache) { if (val) TTF_CloseFont(val); }
            for (auto const& [key, val] : m_imageCache) { if (val) SDL_DestroyTexture(val); }
            if (m_renderer) SDL_DestroyRenderer(m_renderer);
        }
        bool init(SDL_Window* window) override {
            m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!m_renderer) return false;
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            return true;
        }
        void clear(Color color) {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            SDL_RenderClear(m_renderer);
        }
        void present() { SDL_RenderPresent(m_renderer); }

        void drawRect(const SDL_Rect& rect, Color color, const BorderRadius& radius) override {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            if (radius.topLeft <= 0 && radius.topRight <= 0 && radius.bottomLeft <= 0 && radius.bottomRight <= 0) {
                SDL_RenderFillRect(m_renderer, &rect);
                return;
            }
            int r = static_cast<int>(std::max({ radius.topLeft, radius.topRight, radius.bottomLeft, radius.bottomRight }));
            if (r > rect.w / 2) r = rect.w / 2;
            if (r > rect.h / 2) r = rect.h / 2;
            if (r < 1) { SDL_RenderFillRect(m_renderer, &rect); return; }
            int x = rect.x, y = rect.y, w = rect.w, h = rect.h;
            fillCircle(x + r, y + r, r, color);
            fillCircle(x + w - r, y + r, r, color);
            fillCircle(x + r, y + h - r, r, color);
            fillCircle(x + w - r, y + h - r, r, color);
            SDL_Rect top_rect = { x + r, y, w - 2 * r, r };
            SDL_Rect bottom_rect = { x + r, y + h - r, w - 2 * r, r };
            SDL_Rect middle_rect = { x, y + r, w, h - 2 * r };
            SDL_RenderFillRect(m_renderer, &top_rect);
            SDL_RenderFillRect(m_renderer, &bottom_rect);
            SDL_RenderFillRect(m_renderer, &middle_rect);
        }

        void drawLine(int x1, int y1, int x2, int y2, Color color) override {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            SDL_RenderDrawLine(m_renderer, x1, y1, x2, y2);
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

        SDL_Texture* loadImage(const std::string& path) override {
            if (m_imageCache.count(path)) {
                return m_imageCache[path];
            }
            SDL_Texture* texture = IMG_LoadTexture(m_renderer, path.c_str());
            if (!texture) {
                std::cerr << "Error: Failed to load image " << path << " - " << IMG_GetError() << std::endl;
                return nullptr;
            }
            m_imageCache[path] = texture;
            return texture;
        }

        void drawImage(SDL_Texture* texture, const SDL_Rect& dstRect) override {
            if (texture) {
                SDL_RenderCopy(m_renderer, texture, nullptr, &dstRect);
            }
        }

        SDL_Point getImageSize(SDL_Texture* texture) override {
            if (!texture) return { 0, 0 };
            int w, h;
            SDL_QueryTexture(texture, NULL, NULL, &w, &h);
            return { w, h };
        }
    };

    inline App::App(Widget root) : m_root_handle(root) { s_instance = this; }
    inline App::~App() {
        m_renderer.reset();
        if (m_window) SDL_DestroyWindow(m_window);
        IMG_Quit(); 
        TTF_Quit();
        SDL_Quit();
        s_instance = nullptr;
    }
    inline void App::pushOverlay(Widget widget) { m_overlayStack.push_back(widget.getImpl()); markNeedsLayoutUpdate(); }
    inline void App::popOverlay() { if (!m_overlayStack.empty()) { m_overlayStack.pop_back(); markNeedsLayoutUpdate(); } }
    inline void App::addTimer(unsigned int ms, std::function<void()> callback) { m_timers.push_back({ std::chrono::steady_clock::now() + std::chrono::milliseconds(ms), std::move(callback) }); }
    inline void App::run(const std::string& title, bool resizable, SDL_Point size) { internal_run(title, { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.x, size.y }, resizable, Colors::white, ""); }
    inline void App::internal_run(const std::string& title, SDL_Rect size, bool resizable, Color backgroundColor, const std::string& defaultFont) {
        // --- Robust Initialization with Error Logging ---
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "FATAL ERROR: SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }

        if (TTF_Init() == -1) {
            std::cerr << "FATAL ERROR: TTF_Init failed: " << TTF_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG))) {
            std::cerr << "FATAL ERROR: IMG_Init failed: " << IMG_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return;
        }

        m_window = SDL_CreateWindow(title.c_str(), size.x, size.y, size.w, size.h, SDL_WINDOW_SHOWN | (resizable ? SDL_WINDOW_RESIZABLE : 0));
        if (!m_window) {
            std::cerr << "FATAL ERROR: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            IMG_Quit();
            TTF_Quit();
            SDL_Quit();
            return;
        }

        m_renderer = std::make_unique<SDLRenderer>(defaultFont);
        if (!m_renderer->init(m_window)) {
            std::cerr << "FATAL ERROR: m_renderer->init failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            IMG_Quit();
            TTF_Quit();
            SDL_Quit();
            return;
        }

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

            for (const auto& callback : callbacks_to_run) { callback(); }

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                // --- START: DIAGNOSTIC TEST FOR IMMEDIATE QUIT ---
                if (event.type == SDL_QUIT) {
                    std::cout << "DEBUG: SDL_QUIT event received!" << std::endl;
                    // The next line is commented out ON PURPOSE for this test.
                    // running = false; 
                    continue; // Skip the rest of the loop to ignore the QUIT event.
                }
                // --- END: DIAGNOSTIC TEST ---

                if (m_needs_layout_update) continue;

                WidgetBody* target = nullptr;
                SDL_Point mousePos = { 0, 0 };

                if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL) {
                    if (event.type == SDL_MOUSEMOTION) {
                        mousePos = { event.motion.x, event.motion.y };
                    }
                    else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
                        mousePos = { event.button.x, event.button.y };
                    }
                    else {
                        SDL_GetMouseState(&mousePos.x, &mousePos.y);
                    }

                    if (!m_overlayStack.empty()) {
                        for (auto it = m_overlayStack.rbegin(); it != m_overlayStack.rend(); ++it) {
                            target = (*it)->hitTest(mousePos);
                            if (target) break;
                        }
                    }
                    if (!target && m_root_body) {
                        target = m_root_body->hitTest(mousePos);
                    }
                }

                if (target) {
                    target->handleEvent(this, &event);
                }
                else if (m_focusedWidget) {
                    m_focusedWidget->handleEvent(this, &event);
                }
            }

            if (m_needs_layout_update) {
                int w, h;
                SDL_GetWindowSize(m_window, &w, &h);
                SDL_Rect windowRect = { 0, 0, w, h };
                if (m_root_body) m_root_body->performLayout(m_renderer.get(), windowRect);
                for (const auto& overlay : m_overlayStack) {
                    overlay->performLayout(m_renderer.get(), windowRect);
                }
                m_needs_layout_update = false;
            }

            m_renderer->clear(backgroundColor);
            if (m_root_body) m_root_body->render(this, m_renderer.get());
            for (const auto& overlay : m_overlayStack) {
                overlay->render(this, m_renderer.get());
            }
            m_renderer->present();

            SDL_Delay(16);
        }
    }
    // === All Widgets ===

    // --- Foundational Widgets ---

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
        WidgetBody* hitTest(SDL_Point p) override { if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr; return child ? child->hitTest(p) : this; }
    };
    class Container : public Widget {
    public:
        Container(Widget child, Style s = {}) : Widget(std::make_shared<ContainerImpl>(child, std::move(s))) {}
    };

    class ObxImpl : public WidgetBody, public RebuildRequester {
        std::function<Widget()> m_builder;
        std::shared_ptr<WidgetBody> m_child;
    public:
        template<typename Func>
        ObxImpl(Func&& builder) : m_builder(std::forward<Func>(builder)) {}
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
        WidgetBody* hitTest(SDL_Point p) override { return m_child ? m_child->hitTest(p) : nullptr; }
        void handleEvent(App* a, SDL_Event* e) override { if (m_child) m_child->handleEvent(a, e); }
    };
    class Obx : public Widget {
    public:
        template<typename Func>
        Obx(Func&& builder) {
            auto impl = std::make_shared<ObxImpl>(std::forward<Func>(builder));
            impl->initialize();
            p_impl = impl;
        }
    };

    // --- Layout Widgets ---

    class ColumnImpl : public WidgetBody {
    public:
        std::vector<std::shared_ptr<WidgetBody>> children; int spacing;
        ColumnImpl(std::initializer_list<Widget> c, int s) : spacing(s) { for (const auto& w : c) if (auto i = w.getImpl()) { children.push_back(i); i->parent = this; } }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            int y = c.y;
            for (const auto& ch : children) {
                if (!ch) continue;
                ch->performLayout(r, { c.x, y, c.w, c.h - (y - c.y) }); // Give remaining space
                y += ch->m_allocatedSize.h + spacing;
            }
        }
        void render(App* a, IRenderer* r) override { for (const auto& c : children) if (c) c->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            for (const auto& child : children) {
                if (child) if (WidgetBody* target = child->hitTest(p)) return target;
            }
            return this;
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
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            int x = c.x;
            if (children.empty()) return;
            int num_children = static_cast<int>(children.size());
            int total_spacing = spacing * (num_children - 1);
            int child_width = (c.w - total_spacing) / num_children;
            for (const auto& ch : children) {
                if (!ch) continue;
                ch->performLayout(r, { x, c.y, child_width, c.h });
                x += child_width + spacing;
            }
        }
        void render(App* a, IRenderer* r) override { for (const auto& c : children) if (c) c->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            for (const auto& child : children) {
                if (child) if (WidgetBody* target = child->hitTest(p)) return target;
            }
            return this;
        }
    };
    class Row : public Widget {
    public:
        Row(std::initializer_list<Widget> c, int s = 0) : Widget(std::make_shared<RowImpl>(c, s)) {}
    };

    class CenterImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
    public:
        CenterImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            if (child) {
                child->performLayout(r, c); // Let child determine its own size first
                int child_w = child->m_allocatedSize.w;
                int child_h = child->m_allocatedSize.h;
                int child_x = c.x + (c.w - child_w) / 2;
                int child_y = c.y + (c.h - child_h) / 2;
                child->m_allocatedSize = { child_x, child_y, child_w, child_h };
            }
        }
        void render(App* a, IRenderer* r) override { if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            return child ? child->hitTest(p) : this;
        }
    };
    class Center : public Widget {
    public:
        Center(Widget child) : Widget(std::make_shared<CenterImpl>(child)) {}
    };

    class StackImpl : public WidgetBody {
    public:
        std::vector<std::shared_ptr<WidgetBody>> children;
        StackImpl(std::initializer_list<Widget> c) { for (const auto& w : c) if (auto i = w.getImpl()) { children.push_back(i); i->parent = this; } }
        void performLayout(IRenderer* r, SDL_Rect c) override; // Defined after PositionedImpl
        void render(App* a, IRenderer* r) override { for (const auto& ch : children) if (ch) ch->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (*it) if (WidgetBody* target = (*it)->hitTest(p)) return target;
            }
            return this;
        }
    };
    class Stack : public Widget {
    public:
        Stack(std::initializer_list<Widget> children) : Widget(std::make_shared<StackImpl>(children)) {}
    };

    class PositionedImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child;
        std::optional<int> top, left, right, bottom;
        PositionedImpl(Widget c, std::optional<int> t, std::optional<int> l, std::optional<int> r, std::optional<int> b)
            : child(c.getImpl()), top(t), left(l), right(r), bottom(b) {
            if (child) child->parent = this;
        }

        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c; // Will be set by StackImpl
            if (child) child->performLayout(r, m_allocatedSize);
        }
        void render(App* a, IRenderer* r) override { if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            return child ? child->hitTest(p) : this;
        }
    };
    class Positioned : public Widget {
    public:
        Positioned(Widget child, std::optional<int> top = {}, std::optional<int> left = {}, std::optional<int> right = {}, std::optional<int> bottom = {})
            : Widget(std::make_shared<PositionedImpl>(child, top, left, right, bottom)) {
        }
    };

    inline void StackImpl::performLayout(IRenderer* r, SDL_Rect c) {
        m_allocatedSize = c;
        for (const auto& ch : children) {
            if (!ch) continue;
            if (auto pos_impl = std::dynamic_pointer_cast<PositionedImpl>(ch)) {
                int x = c.x + (pos_impl->left.has_value() ? *pos_impl->left : 0);
                int y = c.y + (pos_impl->top.has_value() ? *pos_impl->top : 0);
                int w = c.w - (pos_impl->left.has_value() ? *pos_impl->left : 0) - (pos_impl->right.has_value() ? *pos_impl->right : 0);
                int h = c.h - (pos_impl->top.has_value() ? *pos_impl->top : 0) - (pos_impl->bottom.has_value() ? *pos_impl->bottom : 0);
                ch->performLayout(r, { x, y, w, h });
            }
            else {
                ch->performLayout(r, c); // Non-positioned children take full space
            }
        }
    }

    class ScrollViewImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
        int scrollY = 0;
        int contentHeight = 0;
    public:
        ScrollViewImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            if (child) {
                // Let the child determine its full height, constrained by our width
                child->performLayout(r, { c.x, c.y, c.w, 9999 });
                contentHeight = child->m_allocatedSize.h;
            }
        }
        void handleEvent(App* a, SDL_Event* e) override {
            // Only scroll if the mouse is over the scroll view
            SDL_Point mousePos;
            SDL_GetMouseState(&mousePos.x, &mousePos.y);
            if (SDL_PointInRect(&mousePos, &m_allocatedSize)) {
                if (e->type == SDL_MOUSEWHEEL) {
                    scrollY += e->wheel.y * -20; // Adjust scroll sensitivity here
                    scrollY = std::max(0, scrollY);
                    scrollY = std::min(scrollY, std::max(0, contentHeight - m_allocatedSize.h));
                }
                if (child) {
                    child->handleEvent(a, e);
                }
            }
        }
        void render(App* a, IRenderer* r) override {
            if (!child) return;

            // Use the new public getter method to access the renderer
            SDL_RenderSetClipRect(static_cast<SDLRenderer*>(r)->getSDLRenderer(), &m_allocatedSize);

            auto original_pos = child->m_allocatedSize;
            child->m_allocatedSize.y = m_allocatedSize.y - scrollY;
            child->render(a, r);
            child->m_allocatedSize = original_pos; // Restore position

            SDL_RenderSetClipRect(static_cast<SDLRenderer*>(r)->getSDLRenderer(), NULL);
        }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            // Adjust the hit test point for scrolling, but don't check the child if it's out of view
            SDL_Point adjusted_p = p;
            adjusted_p.y += scrollY;

            if (child) {
                WidgetBody* target = child->hitTest(adjusted_p);
                // Only return a valid target if its visible area is within the scroll view's bounds
                if (target && target->m_allocatedSize.y + target->m_allocatedSize.h > m_allocatedSize.y && target->m_allocatedSize.y < m_allocatedSize.y + m_allocatedSize.h) {
                    return target;
                }
            }
            // If no child is hit, the scroll view itself is the target
            return this;
        }
    };

    // --- Visual Widgets ---

    class ImageImpl : public WidgetBody {
        std::string path;
        SDL_Texture* texture = nullptr;
    public:
        ImageImpl(std::string p) : path(std::move(p)) {}
        void performLayout(IRenderer* r, SDL_Rect c) override {
            if (!texture) texture = r->loadImage(path);
            SDL_Point size = r->getImageSize(texture);
            m_allocatedSize = { c.x, c.y, size.x, size.y };
        }
        void render(App* a, IRenderer* r) override {
            if (!texture) texture = r->loadImage(path);
            if (texture) r->drawImage(texture, m_allocatedSize);
        }
    };
    class Image : public Widget {
    public:
        Image(const std::string& path) : Widget(std::make_shared<ImageImpl>(path)) {}
    };

    class DividerImpl : public WidgetBody {
        Color color;
        int thickness;
    public:
        DividerImpl(Color c, int t) : color(c), thickness(t) {}
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = { c.x, c.y, c.w, thickness };
        }
        void render(App* a, IRenderer* r) override {
            r->drawRect(m_allocatedSize, color, {});
        }
    };
    class Divider : public Widget {
    public:
        Divider(Color color = Colors::grey, int thickness = 1) : Widget(std::make_shared<DividerImpl>(color, thickness)) {}
    };

    // --- Interactive Widgets ---
    class ButtonImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child; std::function<void()> onPressed; Style style; bool isHovered = false;
        ButtonImpl(Widget c, std::function<void()> o, Style s) : onPressed(std::move(o)), style(std::move(s)) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            if (child) {
                child->performLayout(r, c);
                SDL_Point childSize = { child->m_allocatedSize.w, child->m_allocatedSize.h };
                int cX = c.x + (c.w - childSize.x) / 2;
                int cY = c.y + (c.h - childSize.y) / 2;
                child->m_allocatedSize = { cX, cY, childSize.x, childSize.y };
            }
        }
        void render(App* a, IRenderer* r) override { Color bg = style.backgroundColor; if (isHovered) { bg.r = (uint8_t)std::max(0, (int)bg.r - 20); bg.g = (uint8_t)std::max(0, (int)bg.g - 20); bg.b = (uint8_t)std::max(0, (int)bg.b - 20); } r->drawRect(m_allocatedSize, bg, style.border.radius); if (child) child->render(a, r); }
        void handleEvent(App*, SDL_Event* e) override {
            if (e->type == SDL_MOUSEMOTION) {
                SDL_Point mousePos = { e->motion.x, e->motion.y };
                isHovered = SDL_PointInRect(&mousePos, &m_allocatedSize);
            }
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePos = { e->button.x, e->button.y };
                if (isHovered && onPressed && SDL_PointInRect(&mousePos, &m_allocatedSize)) {
                    onPressed();
                }
            }
        }
    };
    class TextButton : public Widget {
    public:
        TextButton(const std::string& t, std::function<void()> o, Style s = {}) : Widget(std::make_shared<ButtonImpl>(Text(t, s.textStyle), std::move(o), std::move(s))) {}
    };
    class IconButton : public Widget {
    public:
        IconButton(const std::string& imagePath, std::function<void()> o, Style s = {}) : Widget(std::make_shared<ButtonImpl>(Image(imagePath), std::move(o), std::move(s))) {}
    };

    class TextBoxImpl : public WidgetBody {
        State<std::string>& state_ref;
        std::string m_localText;
        std::string hintText;
        Style style;
        bool isFocused = false;
    public:
        TextBoxImpl(State<std::string>& s, std::string h, Style st) : state_ref(s), m_localText(s.get()), hintText(std::move(h)), style(std::move(st)) {}
        ~TextBoxImpl() { if (isFocused) { SDL_StopTextInput(); if (App::instance()) App::instance()->releaseFocus(this); } }
        void performLayout(IRenderer* r, SDL_Rect c) override { int h = r->getTextSize("Gg", style.textStyle).y; m_allocatedSize = { c.x, c.y, c.w, h + style.padding.top + style.padding.bottom }; }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePos = { e->button.x, e->button.y };
                bool currentlyFocused = SDL_PointInRect(&mousePos, &m_allocatedSize);
                if (currentlyFocused && !isFocused) { isFocused = true; SDL_StartTextInput(); a->requestFocus(this); }
                else if (!currentlyFocused && isFocused) { isFocused = false; SDL_StopTextInput(); a->releaseFocus(this); }
            }
            if (isFocused) {
                bool changed = false;
                if (e->type == SDL_KEYDOWN && e->key.keysym.sym == SDLK_BACKSPACE && !m_localText.empty()) { m_localText.pop_back(); changed = true; }
                else if (e->type == SDL_TEXTINPUT) { m_localText += e->text.text; changed = true; }
                if (changed) { state_ref.set(m_localText); }
            }
        }
        void render(App* a, IRenderer* r) override {
            m_localText = state_ref.get(); // Sync with state
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
        TextBox(State<std::string>& s, std::string h = "...", Style st = {}) : Widget(std::make_shared<TextBoxImpl>(s, std::move(h), std::move(st))) {}
    };

    class CheckboxImpl : public WidgetBody {
        State<bool>& state_ref;
        bool isHovered = false;
    public:
        CheckboxImpl(State<bool>& s) : state_ref(s) {}
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = { c.x, c.y, 20, 20 }; }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEMOTION) {
                SDL_Point mousePos = { e->motion.x, e->motion.y };
                isHovered = SDL_PointInRect(&mousePos, &m_allocatedSize);
            }
            if (e->type == SDL_MOUSEBUTTONDOWN && isHovered) {
                state_ref.set(!state_ref.get());
            }
        }
        void render(App* a, IRenderer* r) override {
            Color boxColor = isHovered ? Colors::lightBlue : Colors::grey;
            r->drawRect(m_allocatedSize, { 0,0,0,0 }, {}); // Transparent bg
            r->drawLine(m_allocatedSize.x, m_allocatedSize.y, m_allocatedSize.x + m_allocatedSize.w, m_allocatedSize.y, boxColor);
            r->drawLine(m_allocatedSize.x + m_allocatedSize.w, m_allocatedSize.y, m_allocatedSize.x + m_allocatedSize.w, m_allocatedSize.y + m_allocatedSize.h, boxColor);
            r->drawLine(m_allocatedSize.x + m_allocatedSize.w, m_allocatedSize.y + m_allocatedSize.h, m_allocatedSize.x, m_allocatedSize.y + m_allocatedSize.h, boxColor);
            r->drawLine(m_allocatedSize.x, m_allocatedSize.y + m_allocatedSize.h, m_allocatedSize.x, m_allocatedSize.y, boxColor);

            if (state_ref.get()) {
                r->drawLine(m_allocatedSize.x + 4, m_allocatedSize.y + 10, m_allocatedSize.x + 8, m_allocatedSize.y + 14, Colors::blue);
                r->drawLine(m_allocatedSize.x + 8, m_allocatedSize.y + 14, m_allocatedSize.x + 16, m_allocatedSize.y + 6, Colors::blue);
            }
        }
    };
    class Checkbox : public Widget {
    public:
        Checkbox(State<bool>& state) : Widget(std::make_shared<CheckboxImpl>(state)) {}
    };

    class SliderImpl : public WidgetBody {
        State<double>& state_ref;
        double min_val, max_val;
        bool isDragging = false;
    public:
        SliderImpl(State<double>& s, double min, double max) : state_ref(s), min_val(min), max_val(max) {}
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = { c.x, c.y, c.w, 20 }; }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePos = { e->button.x, e->button.y };
                if (SDL_PointInRect(&mousePos, &m_allocatedSize)) {
                    isDragging = true;
                }
            }
            if (e->type == SDL_MOUSEBUTTONUP) {
                isDragging = false;
            }
            if (e->type == SDL_MOUSEMOTION && isDragging) {
                double ratio = static_cast<double>(e->motion.x - m_allocatedSize.x) / m_allocatedSize.w;
                ratio = std::max(0.0, std::min(1.0, ratio));
                double newValue = min_val + ratio * (max_val - min_val);
                state_ref.set(newValue);
            }
        }
        void render(App* a, IRenderer* r) override {
            int trackY = m_allocatedSize.y + m_allocatedSize.h / 2 - 2;
            SDL_Rect track = { m_allocatedSize.x, trackY, m_allocatedSize.w, 4 };
            r->drawRect(track, Colors::grey, BorderRadius::all(2));

            double current_ratio = (max_val - min_val == 0) ? 0 : (state_ref.get() - min_val) / (max_val - min_val);
            int thumbX = m_allocatedSize.x + static_cast<int>(m_allocatedSize.w * current_ratio);
            SDL_Rect thumb = { thumbX - 8, m_allocatedSize.y, 16, 16 };
            r->drawRect(thumb, Colors::blue, BorderRadius::all(8));
        }
    };
    class Slider : public Widget {
    public:
        Slider(State<double>& state, double min = 0.0, double max = 100.0) : Widget(std::make_shared<SliderImpl>(state, min, max)) {}
    };

    class ProgressBarImpl : public WidgetBody {
        State<double>& state_ref; // Value between 0.0 and 1.0
    public:
        ProgressBarImpl(State<double>& s) : state_ref(s) {}
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = { c.x, c.y, c.w, 10 }; }
        void render(App* a, IRenderer* r) override {
            r->drawRect(m_allocatedSize, Colors::grey, BorderRadius::all(5));
            double progress = std::max(0.0, std::min(1.0, state_ref.get()));
            SDL_Rect progressRect = { m_allocatedSize.x, m_allocatedSize.y, static_cast<int>(m_allocatedSize.w * progress), m_allocatedSize.h };
            r->drawRect(progressRect, Colors::green, BorderRadius::all(5));
        }
    };
    class ProgressBar : public Widget {
    public:
        ProgressBar(State<double>& state) : Widget(std::make_shared<ProgressBarImpl>(state)) {}
    };

    // --- Overlays and Scaffolding ---

    class DialogBoxImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
    public:
        DialogBoxImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = c; if (child) { int cw = 300, ch = 150; child->performLayout(r, { c.x + (c.w - cw) / 2, c.y + (c.h - ch) / 2, cw, ch }); } }
        void render(App* a, IRenderer* r) override { r->drawRect(m_allocatedSize, { 0,0,0,128 }, {}); if (child) child->render(a, r); }
        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            if (child) { WidgetBody* t = child->hitTest(p); if (t) return t; }
            return this;
        }
        void handleEvent(App* a, SDL_Event* e) override {
            if (child) child->handleEvent(a, e);
            if (e->type == SDL_MOUSEBUTTONDOWN) { e->type = SDL_USEREVENT; }
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
    class SizedBoxImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child;
        Size size;

        SizedBoxImpl(Widget c, Size s) : child(c.getImpl()), size(s) {
            if (child) child->parent = this;
        }

        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize.x = c.x;
            m_allocatedSize.y = c.y;
            m_allocatedSize.w = (size.width != -1) ? size.width : c.w;
            m_allocatedSize.h = (size.height != -1) ? size.height : c.h;

            if (child) {
                child->performLayout(r, m_allocatedSize);
            }
        }

        void render(App* a, IRenderer* r) override {
            if (child) child->render(a, r);
        }

        WidgetBody* hitTest(SDL_Point p) override {
            if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr;
            return child ? child->hitTest(p) : this;
        }
    };

    class SizedBox : public Widget {
    public:
        SizedBox(Widget child, Size s) : Widget(std::make_shared<SizedBoxImpl>(child, s)) {}
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
