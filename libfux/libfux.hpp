// LibFux - Creator Morteza Mansory
// Version 0.5.2 Patch 4 beta ( in development usable )
// This version expands the widget library and adds core new features.
//=----------------------------------------------=
// Whats New in this Version:

// Rich Widget Expansion: Introduced a dozen new widgets including Image, Stack, ScrollView, Slider, and Checkbox.

// Image Support Added: Integrated the SDL_image library for native image rendering via new Image and IconButton widgets.

// Advanced Layout Control: Added powerful layout widgets like Stack, Positioned, and Center for more complex, layered UIs.

// Enhanced Robustness: Implemented detailed error logging during initialization and added more cross-platform font fallbacks.

// Interactive Element Overhaul: Added new interactive elements like Slider, Checkbox, and ScrollView for richer user input.

// Patch 1: Fixing the ScrollView focus mode.
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

inline void print_rect(const std::string& name, const SDL_Rect& r) {
    std::cout << "[DEBUG] " << name << ": { x=" << r.x << ", y=" << r.y << ", w=" << r.w << ", h=" << r.h << " }" << std::endl;
}


namespace ui {

    class App;
    class Widget;
    class WidgetBody;
    class IRenderer;
    class SDLRenderer;
    class RebuildRequester;
    class DialogBox;
    class SnackBar;
    class PositionedImpl;
    class SizedBoxImpl;

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
            std::cout << "[DEBUG] State changed. Notifying listeners." << std::endl;
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
        virtual void onFocusLost() {}
        virtual std::string getTypeName() const { return typeid(*this).name(); }
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
        void requestFocus(WidgetBody* newFocus) {
            if (m_focusedWidget == newFocus) {
                return;
            }
            if (m_focusedWidget) {
                m_focusedWidget->onFocusLost();
            }

            m_focusedWidget = newFocus;
        }        void releaseFocus(WidgetBody* widget) { if (m_focusedWidget == widget) m_focusedWidget = nullptr; }
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

            if (actualFontFile.empty()) {
                actualFontFile = "Arial.ttf";
                std::cout << "[DEBUG] No font specified, trying default '" << actualFontFile << "'" << std::endl;
            }

            std::string key = actualFontFile + std::to_string(size);
            if (m_fontCache.find(key) != m_fontCache.end()) {
                return m_fontCache[key];
            }

            std::cout << "[DEBUG] Caching new font. Key: '" << key << "'" << std::endl;
            TTF_Font* font = TTF_OpenFont(actualFontFile.c_str(), size);

            if (!font) {
                std::cerr << "[ERROR] Failed to load font '" << actualFontFile << "'. SDL_ttf Error: " << TTF_GetError() << ". Trying fallbacks." << std::endl;
                const char* fallbacks[] = {
                    "C:/Windows/Fonts/Arial.ttf",
                    "/System/Library/Fonts/Supplemental/Arial.ttf",
                    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"
                };
                for (const char* fallback_path : fallbacks) {
                    std::cout << "[DEBUG] Trying fallback font: '" << fallback_path << "'" << std::endl;
                    font = TTF_OpenFont(fallback_path, size);
                    if (font) {
                        std::cout << "[INFO] Successfully loaded fallback font '" << fallback_path << "'." << std::endl;
                        break;
                    }
                    else {
                        std::cerr << "[ERROR] Fallback failed. SDL_ttf Error: " << TTF_GetError() << std::endl;
                    }
                }
            }

            if (!font) {
                std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                std::cerr << "!!! CRITICAL: COULD NOT LOAD ANY FONT. TEXT WILL NOT RENDER. !!!" << std::endl;
                std::cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
                m_fontCache[key] = nullptr;
                return nullptr;
            }

            std::cout << "[INFO] Successfully loaded and cached font '" << actualFontFile << "'." << std::endl;
            m_fontCache[key] = font;
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
            TTF_Font* font = getFont(style.fontFile, style.fontSize);
            if (!font) {
                return;
            }

            SDL_Color c = { style.color.r, style.color.g, style.color.b, style.color.a };
            SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
            if (!surface) {
                std::cerr << "[ERROR] TTF_RenderUTF8_Blended failed for text '" << text << "'. SDL_ttf Error: " << TTF_GetError() << std::endl;
                return;
            }

            SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
            if (!texture) {
                std::cerr << "[ERROR] SDL_CreateTextureFromSurface failed. SDL Error: " << SDL_GetError() << std::endl;
                SDL_FreeSurface(surface);
                return;
            }

            SDL_Rect dstRect = { x, y, surface->w, surface->h };
            SDL_RenderCopy(m_renderer, texture, nullptr, &dstRect);
            SDL_DestroyTexture(texture);
            SDL_FreeSurface(surface);
        }

        SDL_Point getTextSize(const std::string& text, const TextStyle& style) override {
            if (text.empty()) return { 0, style.fontSize };
            TTF_Font* font = getFont(style.fontFile, style.fontSize);
            if (!font) {
                std::cerr << "[ERROR] Cannot get text size for '" << text << "' because font is null." << std::endl;
                return { 0, 0 };
            }
            int w, h;
            if (TTF_SizeText(font, text.c_str(), &w, &h) != 0) {
                std::cerr << "[ERROR] TTF_SizeText failed. SDL_ttf Error: " << TTF_GetError() << std::endl;
                return { 0, 0 };
            }
            return { w, h };
        }

        SDL_Texture* loadImage(const std::string& path) override {
            if (m_imageCache.count(path)) {
                return m_imageCache[path];
            }
            SDL_Texture* texture = IMG_LoadTexture(m_renderer, path.c_str());
            if (!texture) {
                std::cerr << "[ERROR] Failed to load image " << path << " - " << IMG_GetError() << std::endl;
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
    inline void App::run(const std::string& title, bool resizable, SDL_Point size) { internal_run(title, { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.x, size.y }, resizable, Colors::white, "Arial.ttf"); }
    inline void App::internal_run(const std::string& title, SDL_Rect size, bool resizable, Color backgroundColor, const std::string& defaultFont) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "[FATAL] SDL_Init failed: " << SDL_GetError() << std::endl;
            return;
        }

        if (TTF_Init() == -1) {
            std::cerr << "[FATAL] TTF_Init failed: " << TTF_GetError() << std::endl;
            SDL_Quit();
            return;
        }

        if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG))) {
            std::cerr << "[FATAL] IMG_Init failed: " << IMG_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return;
        }
        std::cout << "[INFO] SDL, TTF, and IMG initialized successfully." << std::endl;

        m_window = SDL_CreateWindow(title.c_str(), size.x, size.y, size.w, size.h, SDL_WINDOW_SHOWN | (resizable ? SDL_WINDOW_RESIZABLE : 0));
        if (!m_window) {
            std::cerr << "[FATAL] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            IMG_Quit();
            TTF_Quit();
            SDL_Quit();
            return;
        }
        std::cout << "[INFO] Window created successfully." << std::endl;

        m_renderer = std::make_unique<SDLRenderer>(defaultFont);
        if (!m_renderer->init(m_window)) {
            std::cerr << "[FATAL] m_renderer->init failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            IMG_Quit();
            TTF_Quit();
            SDL_Quit();
            return;
        }
        std::cout << "[INFO] Renderer created successfully." << std::endl;

        m_root_body = m_root_handle.getImpl();

        bool running = true;
        std::cout << "[INFO] Entering main loop." << std::endl;
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
                if (event.type == SDL_QUIT) {
                    running = false;
                    continue;
                }

                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    std::cout << "[DEBUG] Window resized, marking for layout update." << std::endl;
                    markNeedsLayoutUpdate();
                }

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
                std::cout << "[INFO] Performing layout update..." << std::endl;
                int w, h;
                SDL_GetWindowSize(m_window, &w, &h);
                SDL_Rect windowRect = { 0, 0, w, h };
                print_rect("Window Constraints", windowRect);
                if (m_root_body) m_root_body->performLayout(m_renderer.get(), windowRect);
                for (const auto& overlay : m_overlayStack) {
                    overlay->performLayout(m_renderer.get(), windowRect);
                }
                m_needs_layout_update = false;
                std::cout << "[INFO] Layout update finished." << std::endl;
            }
            m_renderer->clear(backgroundColor);
            if (m_root_body) m_root_body->render(this, m_renderer.get());
            for (const auto& overlay : m_overlayStack) {
                overlay->render(this, m_renderer.get());
            }
            m_renderer->present();

            SDL_Delay(16);
        }
        std::cout << "[INFO] Exiting main loop." << std::endl;
    }
    // === All Widgets ===

    class TextImpl : public WidgetBody {
    public:
        std::string text; TextStyle style;
        TextImpl(std::string t, TextStyle s) : text(std::move(t)), style(std::move(s)) {}
        void performLayout(IRenderer* r, SDL_Rect c) override {
            SDL_Point s = r->getTextSize(text, style);
            m_allocatedSize = { c.x, c.y, s.x, s.y };
            std::cout << "[Layout] TextImpl ('" << text << "'): ";
            print_rect("allocated", m_allocatedSize);
        }
        void render(App* a, IRenderer* r) override {
            if (m_allocatedSize.w > 0 && m_allocatedSize.h > 0) {
                r->drawText(text, style, m_allocatedSize.x, m_allocatedSize.y);
            }
        }
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
        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;

            SDL_Rect child_constraints = {
                c.x + style.padding.left,
                c.y + style.padding.top,
                c.w - (style.padding.left + style.padding.right),
                c.h - (style.padding.top + style.padding.bottom)
            };

            if (child) {
                child->performLayout(r, child_constraints);

                bool isHeightUnbounded = (c.h >= 9999);
                if (isHeightUnbounded) {
                    m_allocatedSize.h = child->m_allocatedSize.h + style.padding.top + style.padding.bottom;
                }
            }
            else {
                bool isHeightUnbounded = (c.h >= 9999);
                if (isHeightUnbounded) {
                    m_allocatedSize.h = style.padding.top + style.padding.bottom;
                }
            }

            std::cout << "[Layout] ContainerImpl: ";
            print_rect("allocated", m_allocatedSize);
        }
        void render(App* a, IRenderer* r) override {
            if (style.backgroundColor.a > 0) {
                r->drawRect(m_allocatedSize, style.backgroundColor, style.border.radius);
            }
            if (child) child->render(a, r);
        }
        WidgetBody* hitTest(SDL_Point p) override { if (!SDL_PointInRect(&p, &m_allocatedSize)) return nullptr; return child ? child->hitTest(p) : this; }
    };
    class Container : public Widget {
    public:
        Container(Widget child, Style s = {}) : Widget(std::make_shared<ContainerImpl>(child, std::move(s))) {}
    };

    class ObxImpl : public WidgetBody, public RebuildRequester {
        std::function<Widget()> m_builder;
    public:
        std::shared_ptr<WidgetBody> m_child;
        template<typename Func>
        ObxImpl(Func&& builder) : m_builder(std::forward<Func>(builder)) {}
        void initialize() { buildChild(); }
        void buildChild() {
            std::cout << "[DEBUG] Obx is rebuilding its child." << std::endl;
            auto self_as_derived = std::static_pointer_cast<ObxImpl>(shared_from_this());
            g_currentlyBuildingWidget = self_as_derived;
            Widget new_widget = m_builder();
            g_currentlyBuildingWidget.reset();
            m_child = new_widget.getImpl();
            if (m_child) m_child->parent = this;
            if (App::instance()) App::instance()->markNeedsLayoutUpdate();
        }
        void rebuild() override { buildChild(); }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            if (m_child) {
                m_child->performLayout(r, c);
                m_allocatedSize = m_child->m_allocatedSize;
            }
            else {
                m_allocatedSize = { c.x, c.y, 0, 0 };
            }
        }        void render(App* a, IRenderer* r) override { if (m_child) m_child->render(a, r); }
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

            m_allocatedSize.x = c.x;
            m_allocatedSize.y = c.y;
            m_allocatedSize.w = c.w;

            std::cout << "[Layout] ColumnImpl: ";
            print_rect("constraints", c);

            int current_y = c.y;
            for (const auto& ch : children) {
                if (!ch) continue;

                ch->performLayout(r, { c.x, current_y, c.w, 9999 });

                current_y += ch->m_allocatedSize.h + spacing;
            }

            int total_height = current_y - c.y;
            if (!children.empty()) {
                total_height -= spacing;
            }
            m_allocatedSize.h = total_height;
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
            if (children.empty()) {
                m_allocatedSize.h = 0;
                return;
            }

            int num_children = static_cast<int>(children.size());
            int total_spacing = spacing * (num_children - 1);

            int child_width_slice = (c.w > total_spacing) ? (c.w - total_spacing) / num_children : 0;

            int max_h = 0;

            for (const auto& ch : children) {
                if (!ch) continue;

                ch->performLayout(r, { x, c.y, child_width_slice, 9999 });

                if (ch->m_allocatedSize.h > max_h) {
                    max_h = ch->m_allocatedSize.h;
                }

                x += child_width_slice + spacing;
            }

            m_allocatedSize.h = max_h;
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
    public:
        std::shared_ptr<WidgetBody> child;
        CenterImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }
        void performLayout(IRenderer* r, SDL_Rect c) override {
            std::cout << "[Layout] CenterImpl: ";
            print_rect("constraints", c);

            if (child) {
                child->performLayout(r, c);
                int child_w = child->m_allocatedSize.w;
                int child_h = child->m_allocatedSize.h;

                m_allocatedSize = { c.x, c.y, c.w, child_h };

                int child_x = c.x + (c.w - child_w) / 2;
                child->m_allocatedSize.x = child_x;
                child->m_allocatedSize.y = c.y;
            }
            else {
                m_allocatedSize = { c.x, c.y, c.w, 0 };
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
        void performLayout(IRenderer* r, SDL_Rect c) override;
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
            m_allocatedSize = c;
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
        int max_w = 0;
        int max_h = 0;

        for (const auto& ch : children) {
            if (!ch || std::dynamic_pointer_cast<PositionedImpl>(ch)) {
                continue;
            }
            ch->performLayout(r, { c.x, c.y, c.w, 0 });
            if (ch->m_allocatedSize.w > max_w) max_w = ch->m_allocatedSize.w;
            if (ch->m_allocatedSize.h > max_h) max_h = ch->m_allocatedSize.h;
        }

        m_allocatedSize = { c.x, c.y, max_w, max_h };

        for (const auto& ch : children) {
            if (!ch) continue;

            if (auto pos_impl = std::dynamic_pointer_cast<PositionedImpl>(ch)) {
                int x = m_allocatedSize.x + (pos_impl->left.has_value() ? *pos_impl->left : 0);
                int y = m_allocatedSize.y + (pos_impl->top.has_value() ? *pos_impl->top : 0);
                int w = m_allocatedSize.w - (pos_impl->left.has_value() ? *pos_impl->left : 0) - (pos_impl->right.has_value() ? *pos_impl->right : 0);
                int h = m_allocatedSize.h - (pos_impl->top.has_value() ? *pos_impl->top : 0) - (pos_impl->bottom.has_value() ? *pos_impl->bottom : 0);
                ch->performLayout(r, { x, y, w, h });
            }
            else {
                ch->m_allocatedSize.x = m_allocatedSize.x;
                ch->m_allocatedSize.y = m_allocatedSize.y;
            }
        }
    }

    class ScrollViewImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
        int scrollY = 0;
        int contentHeight = 0;
    private:
        void applyOffsetToDescendants(WidgetBody* body, int dy, std::vector<std::pair<WidgetBody*, SDL_Rect>>& originalRects);
    public:
        ScrollViewImpl(Widget c) { child = c.getImpl(); if (child) child->parent = this; }

        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;
            if (child) {
                child->performLayout(r, { c.x, c.y, c.w, 9999 });
                contentHeight = child->m_allocatedSize.h;
            }
            else {
                contentHeight = 0;
            }
        }

        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEWHEEL) {
                scrollY += e->wheel.y * -20;
                scrollY = std::max(0, scrollY);
                scrollY = std::min(scrollY, std::max(0, contentHeight - m_allocatedSize.h));
                return;
            }

            if (child) {
                SDL_Event adjusted_event = *e;
                bool is_mouse_event = false;

                if (e->type == SDL_MOUSEMOTION) {
                    adjusted_event.motion.y += scrollY;
                    is_mouse_event = true;
                }
                else if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP) {
                    adjusted_event.button.y += scrollY;
                    is_mouse_event = true;
                }
                if (is_mouse_event) {
                    SDL_Point mousePos = { e->button.x, e->button.y };
                    SDL_Point adjusted_p = mousePos;
                    adjusted_p.y += scrollY;

                    WidgetBody* target = child->hitTest(adjusted_p);
                    if (target) {
                        target->handleEvent(a, &adjusted_event);
                    }
                }
            }
        }

        void render(App* a, IRenderer* r) override {
            if (!child) return;
            SDL_RenderSetClipRect(static_cast<SDLRenderer*>(r)->getSDLRenderer(), &m_allocatedSize);
            std::vector<std::pair<WidgetBody*, SDL_Rect>> originalRects;
            applyOffsetToDescendants(child.get(), -scrollY, originalRects);
            child->render(a, r);
            for (const auto& pair : originalRects) {
                pair.first->m_allocatedSize = pair.second;
            }
            SDL_RenderSetClipRect(static_cast<SDLRenderer*>(r)->getSDLRenderer(), NULL);
        }

        WidgetBody* hitTest(SDL_Point p) override {
            if (SDL_PointInRect(&p, &m_allocatedSize)) {
                return this;
            }
            return nullptr;
        }
    };
    class ScrollView : public Widget {
    public:
        ScrollView(Widget child) : Widget(std::make_shared<ScrollViewImpl>(child)) {}
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
            if (child) {
                child->performLayout(r, c);
                SDL_Point childSize = { child->m_allocatedSize.w, child->m_allocatedSize.h };

                m_allocatedSize.w = childSize.x + style.padding.left + style.padding.right;
                m_allocatedSize.h = childSize.y + style.padding.top + style.padding.bottom;
                m_allocatedSize.x = c.x;
                m_allocatedSize.y = c.y;

                int cX = m_allocatedSize.x + style.padding.left;
                int cY = m_allocatedSize.y + style.padding.top;
                child->m_allocatedSize = { cX, cY, childSize.x, childSize.y };

            }
            else {
                m_allocatedSize = c;
            }
            std::cout << "[Layout] ButtonImpl: ";
            print_rect("allocated", m_allocatedSize);
        }
        void render(App* a, IRenderer* r) override {
            Color bg = style.backgroundColor;
            if (isHovered) {
                bg.r = (uint8_t)std::max(0, (int)bg.r - 20);
                bg.g = (uint8_t)std::max(0, (int)bg.g - 20);
                bg.b = (uint8_t)std::max(0, (int)bg.b - 20);
            }
            if (m_allocatedSize.w > 0 && m_allocatedSize.h > 0) {
                r->drawRect(m_allocatedSize, bg, style.border.radius);
            }
            if (child) child->render(a, r);
        }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEMOTION) {
                SDL_Point mousePos = { e->motion.x, e->motion.y };
                isHovered = SDL_PointInRect(&mousePos, &m_allocatedSize);
            }
            else if (e->type == SDL_MOUSEBUTTONDOWN) {
                if (onPressed) {
                    std::cout << "[EVENT] Button pressed!" << std::endl;
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
        void onFocusLost() override {
            if (isFocused) {
                isFocused = false;
                SDL_StopTextInput();
            }
        }
        void performLayout(IRenderer* r, SDL_Rect c) override { int h = r->getTextSize("Gg", style.textStyle).y; m_allocatedSize = { c.x, c.y, c.w, h + style.padding.top + style.padding.bottom }; }
        void handleEvent(App* a, SDL_Event* e) override {
            if (e->type == SDL_MOUSEBUTTONDOWN) {
                SDL_Point mousePos = { e->button.x, e->button.y };
                if (SDL_PointInRect(&mousePos, &m_allocatedSize)) {
                    a->requestFocus(this);
                    if (!isFocused) {
                        isFocused = true;
                        SDL_StartTextInput();
                    }
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
            r->drawRect(m_allocatedSize, { 0,0,0,0 }, {});
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
        double m_progress;
    public:
        ProgressBarImpl(double p) : m_progress(p) {}

        void performLayout(IRenderer* r, SDL_Rect c) override { m_allocatedSize = { c.x, c.y, c.w, 10 }; }
        void render(App* a, IRenderer* r) override {
            r->drawRect(m_allocatedSize, Colors::grey, BorderRadius::all(5));
            double progress = std::max(0.0, std::min(1.0, m_progress));

            SDL_Rect progressRect = { m_allocatedSize.x, m_allocatedSize.y, static_cast<int>(m_allocatedSize.w * progress), m_allocatedSize.h };
            r->drawRect(progressRect, Colors::green, BorderRadius::all(5));
        }
    };
    class ProgressBar : public Widget {
    public:
        ProgressBar(double progress) : Widget(std::make_shared<ProgressBarImpl>(progress)) {}
    };

    // --- Overlays and Scaffolding ---
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

            if (size.height != -1) {
                m_allocatedSize.h = size.height;
            }
            else if (child) {
                child->performLayout(r, { c.x, c.y, m_allocatedSize.w, c.h });
                m_allocatedSize.h = child->m_allocatedSize.h;
            }
            else {
                m_allocatedSize.h = 0;
            }
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

    class DialogBoxImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;

    private:
        void applyOffsetToDescendants(WidgetBody* body, int dx, int dy) {
            if (!body) return;

            body->m_allocatedSize.x += dx;
            body->m_allocatedSize.y += dy;
            
            if (auto impl = dynamic_cast<ContainerImpl*>(body)) {
                applyOffsetToDescendants(impl->child.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<ColumnImpl*>(body)) {
                for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<RowImpl*>(body)) {
                for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<StackImpl*>(body)) {
                for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<CenterImpl*>(body)) {
                applyOffsetToDescendants(impl->child.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<SizedBoxImpl*>(body)) {
                applyOffsetToDescendants(impl->child.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<ButtonImpl*>(body)) {
                applyOffsetToDescendants(impl->child.get(), dx, dy);
            }
            else if (auto impl = dynamic_cast<ObxImpl*>(body)) {
                applyOffsetToDescendants(impl->m_child.get(), dx, dy);
            }
        }

    public:
        DialogBoxImpl(Widget c) {
            child = c.getImpl();
            if (child) child->parent = this;
        }

        void performLayout(IRenderer* r, SDL_Rect c) override {
            m_allocatedSize = c;

            if (child) {
                int max_w = static_cast<int>(c.w * 0.8);
                child->performLayout(r, { 0, 0, max_w, 9999 });

                int child_w = child->m_allocatedSize.w;
                int child_h = child->m_allocatedSize.h;

                int dx = c.x + (c.w - child_w) / 2;
                int dy = c.y + (c.h - child_h) / 2;

                applyOffsetToDescendants(child.get(), dx, dy);
            }
        }

        void render(App* a, IRenderer* r) override {
            r->drawRect(m_allocatedSize, { 0, 0, 0, 128 }, {});
            if (child) child->render(a, r);
        }

        WidgetBody* hitTest(SDL_Point p) override {
            if (!child) return this;
            WidgetBody* target = child->hitTest(p);
            return target ? target : this;
        }

        void handleEvent(App* a, SDL_Event* e) override {
            if (child) {
                child->handleEvent(a, e);
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
    inline void ScrollViewImpl::applyOffsetToDescendants(WidgetBody* body, int dy, std::vector<std::pair<WidgetBody*, SDL_Rect>>& originalRects) {
        if (!body) return;

        originalRects.push_back({ body, body->m_allocatedSize });
        body->m_allocatedSize.y += dy;

        if (auto impl = dynamic_cast<ContainerImpl*>(body)) {
            applyOffsetToDescendants(impl->child.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<ColumnImpl*>(body)) {
            for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<RowImpl*>(body)) {
            for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<StackImpl*>(body)) {
            for (const auto& c : impl->children) applyOffsetToDescendants(c.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<CenterImpl*>(body)) {
            applyOffsetToDescendants(impl->child.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<SizedBoxImpl*>(body)) {
            applyOffsetToDescendants(impl->child.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<ObxImpl*>(body)) {
            applyOffsetToDescendants(impl->m_child.get(), dy, originalRects);
        }
        else if (auto impl = dynamic_cast<ButtonImpl*>(body)) {
            applyOffsetToDescendants(impl->child.get(), dy, originalRects);
        }
    }
} // namespace ui
