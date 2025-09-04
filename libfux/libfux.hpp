// FUX UI Library - Creator Morteza Mansory
// Version 0.2.28
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


namespace ui {

    class App;
    class Widget;
    class WidgetBody;
    class IRenderer;

    struct Color { uint8_t r, g, b, a = 255; };
    struct TextStyle { int fontSize = 16; Color color = { 0, 0, 0 }; std::string fontFile = "arial.ttf"; };
    struct EdgeInsets { int top = 0, right = 0, bottom = 0, left = 0; };
    struct Border { Color color = { 0,0,0,0 }; int width = 0; double radius = 0.0; };

    struct Style {
        Color backgroundColor = { 0, 0, 0, 0 };
        Border border = {};
        TextStyle textStyle = {};
        EdgeInsets padding = {};
    };

    enum class SnackBarPosition { Bottom, Top };

    struct SnackBarConfig {
        std::string message = "";
        Style style = {};
        SnackBarPosition position = SnackBarPosition::Bottom;
    };

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

    class ContextBuilderBase {
    public:
        virtual ~ContextBuilderBase() = default;
        virtual void rebuild() = 0;
        virtual void addDependency(void* state, size_t type_hash) = 0;
    };

    // =======================================================
    // Reactivity System
    // =======================================================
    template<typename T>
    class State {
    public:
        State(T initialValue) : value(initialValue) {}

        const T& get() const {
            if (currentlyBuilding) currentlyBuilding->addDependency((void*)this, typeid(T).hash_code());
            return value;
        }

        void set(T newValue) {
            value = newValue;
            listeners.erase(std::remove_if(listeners.begin(), listeners.end(),
                [](const std::weak_ptr<ContextBuilderBase>& weak_listener) {
                    if (auto shared_listener = weak_listener.lock()) {
                        shared_listener->rebuild();
                        return false; // Keep
                    }
                    return true; // Remove
                }), listeners.end());
        }

        void subscribe(std::shared_ptr<ContextBuilderBase> listener) {
            listeners.push_back(listener);
        }

        static inline ContextBuilderBase* currentlyBuilding = nullptr;
    private:
        T value;
        std::vector<std::weak_ptr<ContextBuilderBase>> listeners;
    };

    // =======================================================
    // Global AppContext for simplified actions
    // =======================================================

    class AppContext {
    public:
        static AppContext& instance() {
            static AppContext inst;
            return inst;
        }
        State<bool> isSnackBarVisible{ false };
        State<SnackBarConfig> snackBarConfig{ SnackBarConfig{} };

        void showSnackBar(const std::string& message) {
            Style defaultStyle;
            defaultStyle.backgroundColor = Colors::darkGrey;
            defaultStyle.textStyle = { 16, Colors::white };
            defaultStyle.padding = { 12, 18, 12, 18 };
            defaultStyle.border.radius = 6.0;

            show({ message, defaultStyle, SnackBarPosition::Bottom });
        }

void showSnackBar(const std::string& message, const Style& style, SnackBarPosition position = SnackBarPosition::Bottom) {
            show({ message, style, position });
        }

    private:
        void show(const SnackBarConfig& config) {
            snackBarConfig.set(config);
            isSnackBarVisible.set(true);

            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::seconds(3));
                this->isSnackBarVisible.set(false);
                }).detach();
        }

        AppContext() = default;
        ~AppContext() = default;
        AppContext(const AppContext&) = delete;
        AppContext& operator=(const AppContext&) = delete;
    };
    // =======================================================
    // Handle-Body Architecture
    // =======================================================
    class IRenderer {
    public:
        virtual ~IRenderer() = default;
        virtual bool init(SDL_Window* window) = 0;
        virtual void clear(Color color) = 0;
        virtual void present() = 0;
        virtual void drawRect(const SDL_Rect& rect, Color color, double radius) = 0;
        virtual void drawText(const std::string& text, const TextStyle& style, int x, int y) = 0;
        virtual SDL_Point getTextSize(const std::string& text, const TextStyle& style) = 0;
    };

    class WidgetBody : public std::enable_shared_from_this<WidgetBody> {
    public:
        virtual ~WidgetBody() = default;
        virtual void render(App* app, IRenderer* renderer, SDL_Rect allocatedSize) = 0;
        virtual void handleEvent(App* app, SDL_Event* event, SDL_Rect allocatedSize) {}
        virtual SDL_Point getPreferredSize(IRenderer* renderer, SDL_Point constraints) = 0;
        virtual std::any getContext(size_t type_hash) {
            return (parent) ? parent->getContext(type_hash) : std::any{};
        }

        WidgetBody* parent = nullptr;
        int flex = 0;
    };

    class Widget {
    private:
        std::shared_ptr<WidgetBody> p_impl;
    public:
        Widget(std::shared_ptr<WidgetBody> impl = nullptr) : p_impl(impl) {}
        WidgetBody* operator->() const { return p_impl.get(); }
        WidgetBody* get() const { return p_impl.get(); }
        std::shared_ptr<WidgetBody> getImpl() const { return p_impl; }
        explicit operator bool() const { return p_impl != nullptr; }
    };

    class SDLRenderer : public IRenderer {
    private:
        SDL_Renderer* m_renderer = nullptr;
        std::map<std::string, TTF_Font*> fontCache;

        TTF_Font* getFont(const std::string& fontFile, int size) {
            std::string key = fontFile + std::to_string(size);
            if (fontCache.find(key) == fontCache.end()) {
                TTF_Font* font = TTF_OpenFont(fontFile.c_str(), size);
                if (!font) {
                    std::cerr << "CRITICAL: Failed to load font '" << fontFile << "'. Ensure it is next to the executable. | SDL_ttf Error: " << TTF_GetError() << std::endl;
                    font = TTF_OpenFont("C:/Windows/Fonts/Arial.ttf", size);
                    if (!font) {
                        std::cerr << "CRITICAL: Could not load fallback font Arial from Windows/Fonts." << std::endl;
                        return nullptr;
                    }
                }
                fontCache[key] = font;
            }
            return fontCache[key];
        }

    public:
        ~SDLRenderer() {
            for (auto const& [key, val] : fontCache) { if (val) TTF_CloseFont(val); }
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

        void drawRect(const SDL_Rect& rect, Color color, double radius) override {
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            if (radius <= 1.0) {
                SDL_RenderFillRect(m_renderer, &rect);
            }
            else {
                int r = static_cast<int>(radius);
                int x = rect.x, y = rect.y, w = rect.w, h = rect.h;
                if (r > w / 2) r = w / 2;
                if (r > h / 2) r = h / 2;

                SDL_Rect centerRect = { x + r, y, w - 2 * r, h };
                SDL_RenderFillRect(m_renderer, &centerRect);
                SDL_Rect leftRect = { x, y + r, r, h - 2 * r };
                SDL_RenderFillRect(m_renderer, &leftRect);
                SDL_Rect rightRect = { x + w - r, y + r, r, h - 2 * r };
                SDL_RenderFillRect(m_renderer, &rightRect);

                for (int i = 0; i < r; i++) {
                    for (int j = 0; j < r; j++) {
                        if (i * i + j * j < r * r) {
                            SDL_RenderDrawPoint(m_renderer, x + r - i, y + r - j);
                            SDL_RenderDrawPoint(m_renderer, x + w - r - 1 + i, y + r - j);
                            SDL_RenderDrawPoint(m_renderer, x + r - i, y + h - r - 1 + j);
                            SDL_RenderDrawPoint(m_renderer, x + w - r - 1 + i, y + h - r - 1 + j);
                        }
                    }
                }
            }
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
            if (!font) return { 0, 0 };
            int w, h;
            TTF_SizeText(font, text.c_str(), &w, &h);
            return { w, h };
        }
    };

    // =======================================================
    // App Class
    // =======================================================
    class App {
    public:
        App(Widget root) : m_root_handle(root) {}
        ~App() {
            m_renderer.reset();
            if (m_window) SDL_DestroyWindow(m_window);
            TTF_Quit();
            SDL_Quit();
        }

        void run(const std::string& title = "FUI App",
            SDL_Rect size = { SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600 },
            bool resizable = false,
            Color backgroundColor = Colors::white)
        {
            SDL_Init(SDL_INIT_VIDEO);
            TTF_Init();
            Uint32 window_flags = SDL_WINDOW_SHOWN | (resizable ? SDL_WINDOW_RESIZABLE : 0);

            m_renderer = std::make_unique<SDLRenderer>();

            m_window = SDL_CreateWindow(title.c_str(), size.x, size.y, size.w, size.h, window_flags);
            if (!m_window || !m_renderer->init(m_window)) { return; }

            m_root_body = m_root_handle.getImpl();
            if (m_root_body) m_root_body->parent = nullptr;

            bool running = true;
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) running = false;
                    if (m_root_body) {
                        int w, h;
                        SDL_GetWindowSize(m_window, &w, &h);
                        m_root_body->handleEvent(this, &event, { 0, 0, w, h });
                    }
                }

                m_renderer->clear(backgroundColor);
                if (m_root_body) {
                    int w, h;
                    SDL_GetWindowSize(m_window, &w, &h);
                    m_root_body->render(this, m_renderer.get(), { 0, 0, w, h });
                }
                m_renderer->present();
                SDL_Delay(16);
            }
        }
    private:
        Widget m_root_handle;
        std::shared_ptr<WidgetBody> m_root_body;
        SDL_Window* m_window = nullptr;
        std::unique_ptr<IRenderer> m_renderer;
    };

    // =======================================================
    // Context & State Management Widgets
    // =======================================================
    template<typename T>
    class ContextProviderImpl : public WidgetBody {
    public:
        State<T>& state_ref;
        std::shared_ptr<WidgetBody> child;

        ContextProviderImpl(State<T>& state, Widget childWidget) : state_ref(state) {
            child = childWidget.getImpl();
            if (child) child->parent = this;
        }

        std::any getContext(size_t type_hash) override {
            return (type_hash == typeid(T).hash_code()) ? std::any(&state_ref) : WidgetBody::getContext(type_hash);
        }

        void render(App* app, IRenderer* r, SDL_Rect size) override { if (child) child->render(app, r, size); }
        void handleEvent(App* app, SDL_Event* e, SDL_Rect s) override { if (child) child->handleEvent(app, e, s); }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override { return child ? child->getPreferredSize(r, c) : SDL_Point{ 0,0 }; }
    };

    template<typename T>
    class ContextProvider : public Widget {
    public:
        ContextProvider(State<T>& state, Widget child)
            : Widget(std::make_shared<ContextProviderImpl<T>>(state, child)) {
        }
    };

    template<typename T>
    class ContextBuilderImpl : public WidgetBody, public ContextBuilderBase {
    public:
        std::function<Widget(T)> builder;
        std::shared_ptr<WidgetBody> child;
        State<T>* state_ptr = nullptr;
        std::set<void*> dependencies;

        ContextBuilderImpl(std::function<Widget(T)> builder) : builder(builder) {}

        void buildChild() {
            if (!state_ptr) return;
            State<T>::currentlyBuilding = this;
            Widget new_widget = builder(state_ptr->get());
            State<T>::currentlyBuilding = nullptr;
            child = new_widget.getImpl();
            if (child) child->parent = this;
        }

        void rebuild() override { buildChild(); }

        void addDependency(void* state_void, size_t type_hash) override {
            if (type_hash == typeid(T).hash_code() && dependencies.find(state_void) == dependencies.end()) {
                if (auto self = std::dynamic_pointer_cast<ContextBuilderBase>(shared_from_this())) {
                    static_cast<State<T>*>(state_void)->subscribe(self);
                    dependencies.insert(state_void);
                }
            }
        }

        void ensureInitialized() {
            if (!state_ptr) {
                auto context = getContext(typeid(T).hash_code());
                if (context.has_value()) {
                    state_ptr = std::any_cast<State<T>*>(context);
                    buildChild();
                }
            }
        }

        void render(App* app, IRenderer* r, SDL_Rect size) override {
            ensureInitialized();
            if (child) child->render(app, r, size);
        }
        void handleEvent(App* app, SDL_Event* e, SDL_Rect s) override {
            ensureInitialized();
            if (child) child->handleEvent(app, e, s);
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            ensureInitialized();
            return child ? child->getPreferredSize(r, c) : SDL_Point{ 0, 0 };
        }
    };

    template<typename T>
    class ContextBuilder : public Widget {
    public:
        ContextBuilder(std::function<Widget(T)> builder)
            : Widget(std::make_shared<ContextBuilderImpl<T>>(builder)) {
        }
    };

    // =======================================================
    // Basic Layout Widgets
    // =======================================================
    class TextImpl : public WidgetBody {
    public:
        std::string text;
        TextStyle style;
        TextImpl(std::string text, TextStyle style) : text(text), style(style) {}
        void render(App* app, IRenderer* r, SDL_Rect s) override {
            if (r) r->drawText(text, style, s.x, s.y);
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            if (r) return r->getTextSize(text, style);
            return { (int)text.length() * (style.fontSize / 2), style.fontSize };
        }
    };
    class Text : public Widget {
    public:
        Text(const std::string& text = "", TextStyle style = {})
            : Widget(std::make_shared<TextImpl>(text, style)) {
        }
    };

    class ContainerImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child;
        Style style;
        ContainerImpl(Widget childWidget, Style s) : style(s) {
            child = childWidget.getImpl();
            if (child) child->parent = this;
        }

        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            if (!child) return { style.padding.left + style.padding.right, style.padding.top + style.padding.bottom };
            SDL_Point childConstraints = { c.x - (style.padding.left + style.padding.right), c.y - (style.padding.top + style.padding.bottom) };
            SDL_Point childSize = child->getPreferredSize(r, childConstraints);
            return { childSize.x + style.padding.left + style.padding.right, childSize.y + style.padding.top + style.padding.bottom };
        }

        void render(App* app, IRenderer* renderer, SDL_Rect s) override {
            if (style.backgroundColor.a > 0) {
                renderer->drawRect(s, style.backgroundColor, style.border.radius);
            }
            if (child) {
                SDL_Rect childRect = { s.x + style.padding.left, s.y + style.padding.top, s.w - (style.padding.left + style.padding.right), s.h - (style.padding.top + style.padding.bottom) };
                child->render(app, renderer, childRect);
            }
        }

        void handleEvent(App* app, SDL_Event* event, SDL_Rect s) override {
            if (child) {
                SDL_Rect childRect = { s.x + style.padding.left, s.y + style.padding.top, s.w - (style.padding.left + style.padding.right), s.h - (style.padding.top + style.padding.bottom) };
                child->handleEvent(app, event, childRect);
            }
        }
    };
    class Container : public Widget {
    public:
        Container(Widget child, Style s = {}) : Widget(std::make_shared<ContainerImpl>(child, s)) {}
    };

    class MultiChildLayoutImpl : public WidgetBody {
    public:
        std::vector<std::shared_ptr<WidgetBody>> children;
        MultiChildLayoutImpl(std::initializer_list<Widget> childrenWidgets) {
            for (const auto& handle : childrenWidgets) {
                if (auto impl = handle.getImpl()) {
                    children.push_back(impl);
                    impl->parent = this;
                }
            }
        }
    };

    class ColumnImpl : public MultiChildLayoutImpl {
    public:
        int spacing;
        ColumnImpl(std::initializer_list<Widget> c, int s) : MultiChildLayoutImpl(c), spacing(s) {}

        void render(App* a, IRenderer* r, SDL_Rect s) override {
            int total_flex = 0;
            int fixed_h = 0;
            for (const auto& c : children) {
                if (c->flex > 0) total_flex += c->flex;
                else fixed_h += c->getPreferredSize(r, { s.w, 0 }).y;
            }
            fixed_h += std::max(0, (int)children.size() - 1) * spacing;
            int remaining_h = s.h > fixed_h ? s.h - fixed_h : 0;
            int current_y = s.y;
            for (const auto& c : children) {
                int child_h = (c->flex > 0) ? ((total_flex > 0) ? (remaining_h * c->flex / total_flex) : 0) : c->getPreferredSize(r, { s.w, 0 }).y;
                c->render(a, r, { s.x, current_y, s.w, child_h });
                current_y += child_h + spacing;
            }
        }
        void handleEvent(App* a, SDL_Event* e, SDL_Rect s) override {
            int total_flex = 0;
            int fixed_h = 0;
            for (const auto& c : children) {
                if (c->flex > 0) total_flex += c->flex;
                else fixed_h += c->getPreferredSize(nullptr, { s.w, 0 }).y;
            }
            fixed_h += std::max(0, (int)children.size() - 1) * spacing;
            int remaining_h = s.h > fixed_h ? s.h - fixed_h : 0;
            int current_y = s.y;
            for (const auto& c : children) {
                int child_h = (c->flex > 0) ? ((total_flex > 0) ? (remaining_h * c->flex / total_flex) : 0) : c->getPreferredSize(nullptr, { s.w, 0 }).y;
                SDL_Rect childRect = { s.x, current_y, s.w, child_h };
                c->handleEvent(a, e, childRect);
                current_y += child_h + spacing;
            }
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            int total_h = 0, max_w = 0;
            for (size_t i = 0; i < children.size(); ++i) {
                auto size = children[i]->getPreferredSize(r, c);
                if (size.x > max_w) max_w = size.x;
                total_h += size.y;
                if (i < children.size() - 1) total_h += spacing;
            }
            return { max_w, total_h };
        }
    };
    class Column : public Widget {
    public:
        Column(std::initializer_list<Widget> c, int s = 0) : Widget(std::make_shared<ColumnImpl>(c, s)) {}
    };

    class RowImpl : public MultiChildLayoutImpl {
    public:
        int spacing;
        RowImpl(std::initializer_list<Widget> c, int s) : MultiChildLayoutImpl(c), spacing(s) {}

        void render(App* a, IRenderer* r, SDL_Rect s) override {
            int total_flex = 0;
            int fixed_w = 0;
            for (const auto& c : children) {
                if (c->flex > 0) total_flex += c->flex;
                else fixed_w += c->getPreferredSize(r, { 0, s.h }).x;
            }
            fixed_w += std::max(0, (int)children.size() - 1) * spacing;
            int remaining_w = s.w > fixed_w ? s.w - fixed_w : 0;
            int current_x = s.x;
            for (const auto& c : children) {
                int child_w = (c->flex > 0) ? ((total_flex > 0) ? (remaining_w * c->flex / total_flex) : 0) : c->getPreferredSize(r, { 0, s.h }).x;
                c->render(a, r, { current_x, s.y, child_w, s.h });
                current_x += child_w + spacing;
            }
        }

        void handleEvent(App* a, SDL_Event* e, SDL_Rect s) override {
            int total_flex = 0;
            int fixed_w = 0;
            for (const auto& c : children) {
                if (c->flex > 0) total_flex += c->flex;
                else fixed_w += c->getPreferredSize(nullptr, { 0, s.h }).x;
            }
            fixed_w += std::max(0, (int)children.size() - 1) * spacing;
            int remaining_w = s.w > fixed_w ? s.w - fixed_w : 0;
            int current_x = s.x;
            for (const auto& c : children) {
                int child_w = (c->flex > 0) ? ((total_flex > 0) ? (remaining_w * c->flex / total_flex) : 0) : c->getPreferredSize(nullptr, { 0, s.h }).x;
                SDL_Rect childRect = { current_x, s.y, child_w, s.h };
                c->handleEvent(a, e, childRect);
                current_x += child_w + spacing;
            }
        }

        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            int total_w = 0, max_h = 0;
            for (size_t i = 0; i < children.size(); ++i) {
                auto size = children[i]->getPreferredSize(r, c);
                if (size.y > max_h) max_h = size.y;
                total_w += size.x;
                if (i < children.size() - 1) total_w += spacing;
            }
            return { total_w, max_h };
        }
    };
    class Row : public Widget {
    public:
        Row(std::initializer_list<Widget> c, int s = 0) : Widget(std::make_shared<RowImpl>(c, s)) {}
    };

    class StackImpl : public MultiChildLayoutImpl {
    public:
        StackImpl(std::initializer_list<Widget> c) : MultiChildLayoutImpl(c) {}

        void render(App* a, IRenderer* r, SDL_Rect s) override {
            for (const auto& child : children) {
                if (child) child->render(a, r, s);
            }
        }
        void handleEvent(App* a, SDL_Event* e, SDL_Rect s) override {
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (*it) (*it)->handleEvent(a, e, s);
            }
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            int max_w = 0, max_h = 0;
            for (const auto& child : children) {
                if (child) {
                    auto size = child->getPreferredSize(r, c);
                    if (size.x > max_w) max_w = size.x;
                    if (size.y > max_h) max_h = size.y;
                }
            }
            return { max_w, max_h };
        }
    };
    class Stack : public Widget {
    public:
        Stack(std::initializer_list<Widget> c) : Widget(std::make_shared<StackImpl>(c)) {}
    };

    // =======================================================
    // Interactive Widgets
    // =======================================================
    class ButtonImpl : public WidgetBody {
    public:
        std::shared_ptr<WidgetBody> child;
        std::function<void()> onPressed;
        Style style;
        bool isHovered = false;

        ButtonImpl(Widget childWidget, std::function<void()> onPressed, Style style)
            : onPressed(onPressed), style(style) {
            child = childWidget.getImpl();
            if (child) child->parent = this;
        }

        void render(App* a, IRenderer* r, SDL_Rect s) override {
            Color bgColor = style.backgroundColor;
            if (isHovered) {
                bgColor.r = std::max(0, bgColor.r - 20);
                bgColor.g = std::max(0, bgColor.g - 20);
                bgColor.b = std::max(0, bgColor.b - 20);
            }
            r->drawRect(s, bgColor, style.border.radius);
            if (child) {
                auto childSize = child->getPreferredSize(r, { s.w, s.h });
                int cX = s.x + (s.w - childSize.x) / 2;
                int cY = s.y + (s.h - childSize.y) / 2;
                child->render(a, r, { cX, cY, childSize.x, childSize.y });
            }
        }

        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            if (!child) return { 20, 20 };
            auto childSize = child->getPreferredSize(r, c);
            return { childSize.x + style.padding.left + style.padding.right, childSize.y + style.padding.top + style.padding.bottom };
        }

        void handleEvent(App*, SDL_Event* event, SDL_Rect s) override {
            if (event->type == SDL_MOUSEMOTION || event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
                int mX, mY; SDL_GetMouseState(&mX, &mY);
                SDL_Point p = { mX, mY };
                isHovered = SDL_PointInRect(&p, &s);
                if (event->type == SDL_MOUSEBUTTONDOWN && isHovered && onPressed) {
                    onPressed();
                }
            }
        }
    };
    class Button : public Widget {
    public:
        Button(Widget child, std::function<void()> onPressed, Style s = {})
            : Widget(std::make_shared<ButtonImpl>(child, onPressed, s)) {
        }
    };

    class TextButton : public Widget {
    public:
        TextButton(const std::string& text, std::function<void()> onPressed, Style s = {})
            : Widget(std::make_shared<ButtonImpl>(Text(text, s.textStyle), onPressed, s)) {
        }
    };

    class TextBoxImpl : public WidgetBody {
        State<std::string>& state_ref;
        std::string hintText;
        Style style;
        bool isFocused = false;
    public:
        TextBoxImpl(State<std::string>& state, std::string hint, Style s)
            : state_ref(state), hintText(hint), style(s) {
        }

        void handleEvent(App*, SDL_Event* event, SDL_Rect s) override {
            if (event->type == SDL_MOUSEBUTTONDOWN) {
                int mX, mY; SDL_GetMouseState(&mX, &mY);
                SDL_Point p = { mX, mY };
                isFocused = SDL_PointInRect(&p, &s);
                if (isFocused) SDL_StartTextInput(); else SDL_StopTextInput();
            }
            if (isFocused) {
                std::string currentText = state_ref.get();
                bool changed = false;
                if (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_BACKSPACE && !currentText.empty()) {
                    currentText.pop_back();
                    changed = true;
                }
                else if (event->type == SDL_TEXTINPUT) {
                    currentText += event->text.text;
                    changed = true;
                }
                if (changed) state_ref.set(currentText);
            }
        }
        void render(App*, IRenderer* r, SDL_Rect s) override {
            r->drawRect(s, style.backgroundColor, style.border.radius);
            std::string text = state_ref.get();
            std::string displayText = text.empty() ? hintText : text;
            TextStyle currentTextStyle = style.textStyle;
            if (text.empty()) currentTextStyle.color = Colors::grey;

            r->drawText(displayText, currentTextStyle, s.x + style.padding.left, s.y + (s.h - r->getTextSize(displayText, currentTextStyle).y) / 2);

            if (isFocused && (SDL_GetTicks() / 500) % 2) {
                auto textSize = r->getTextSize(text, currentTextStyle);
                SDL_Rect cursor = { s.x + style.padding.left + textSize.x, s.y + style.padding.top, 2, s.h - (style.padding.top + style.padding.bottom) };
                r->drawRect(cursor, currentTextStyle.color, 0);
            }
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            int textHeight = style.textStyle.fontSize;
            if (r) {
                textHeight = r->getTextSize("Gg", style.textStyle).y;
            }
            return { c.x > 0 ? c.x : 200, textHeight + style.padding.top + style.padding.bottom };
        }
    };
    class TextBox : public Widget {
    public:
        TextBox(State<std::string>& state, std::string hint = "Enter text...", Style s = {})
            : Widget(std::make_shared<TextBoxImpl>(state, hint, s)) {
        }
    };

    class DialogBoxImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
    public:
        DialogBoxImpl(Widget childWidget) {
            child = childWidget.getImpl();
            if (child) child->parent = this;
        }
        void handleEvent(App* a, SDL_Event* e, SDL_Rect s) override {
            if (child) {
                SDL_Point constraints = { s.w > 40 ? s.w - 40 : 0, s.h > 40 ? s.h - 40 : 0 };
                auto childSize = child->getPreferredSize(nullptr, constraints);
                int cX = s.x + (s.w - childSize.x) / 2;
                int cY = s.y + (s.h - childSize.y) / 2;
                SDL_Rect childRect = { cX, cY, childSize.x, childSize.y };
                child->handleEvent(a, e, childRect);
            }

            if (e->type == SDL_MOUSEBUTTONDOWN) {
                e->type = SDL_USEREVENT;
            }
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override { return c; }
        void render(App* a, IRenderer* r, SDL_Rect s) override {
            r->drawRect(s, { 0,0,0,128 }, 0); 
            if (child) {
                SDL_Point constraints = { s.w > 40 ? s.w - 40 : 0, s.h > 40 ? s.h - 40 : 0 };
                auto childSize = child->getPreferredSize(r, constraints);
                int cX = s.x + (s.w - childSize.x) / 2;
                int cY = s.y + (s.h - childSize.y) / 2;
                child->render(a, r, { cX, cY, childSize.x, childSize.y });
            }
        }
    };
    class DialogBox : public Widget {
    public:
        DialogBox(Widget child) : Widget(std::make_shared<DialogBoxImpl>(child)) {}
    };

    class SnackBarImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> child;
        SnackBarPosition position;
    public:
        SnackBarImpl(Widget childWidget, SnackBarPosition pos) : position(pos) {
            child = childWidget.getImpl();
            if (child) child->parent = this;
        }

        void handleEvent(App* a, SDL_Event* e, SDL_Rect s) override { /* SnackBar consumes no events */ }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override { return c; }

        void render(App* a, IRenderer* r, SDL_Rect s) override {
            if (child) {
                SDL_Point constraints = { s.w > 40 ? s.w - 40 : 0, 50 };
                auto childSize = child->getPreferredSize(r, constraints);
                int cX = s.x + (s.w - childSize.x) / 2;

                int cY = (position == SnackBarPosition::Bottom)
                    ? s.y + s.h - childSize.y - 20
                    : s.y + 20;

                child->render(a, r, { cX, cY, childSize.x, childSize.y });
            }
        }
    };
    class SnackBar : public Widget {
    public:
        SnackBar(Widget child, SnackBarPosition position = SnackBarPosition::Bottom)
            : Widget(std::make_shared<SnackBarImpl>(child, position)) {
        }
    };

    class IfImpl : public WidgetBody {
        std::shared_ptr<WidgetBody> internal_widget;
    public:
        IfImpl(State<bool>& condition, std::function<Widget()> builder) {
            auto context_builder = ui::ContextBuilder<bool>([builder](bool show) -> ui::Widget {
                if (show) return builder();
                return ui::Widget();
                });
            auto context_provider = ui::ContextProvider<bool>(condition, context_builder);
            internal_widget = context_provider.getImpl();
            if (internal_widget) internal_widget->parent = this;
        }

        void render(App* app, IRenderer* r, SDL_Rect s) override {
            if (internal_widget) internal_widget->render(app, r, s);
        }
        void handleEvent(App* app, SDL_Event* e, SDL_Rect s) override {
            if (internal_widget) internal_widget->handleEvent(app, e, s);
        }
        SDL_Point getPreferredSize(IRenderer* r, SDL_Point c) override {
            return internal_widget ? internal_widget->getPreferredSize(r, c) : SDL_Point{ 0,0 };
        }
    };
    class If : public Widget {
    public:
        If(State<bool>& condition, std::function<Widget()> builder)
            : Widget(std::make_shared<IfImpl>(condition, builder)) {
        }
    };

} // namespace ui
