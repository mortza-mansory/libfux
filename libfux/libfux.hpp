#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <initializer_list>

namespace ui {

// === Core Types ===
struct Color { uint8_t r, g, b, a = 255; };
struct EdgeInsets { int left = 0, top = 0, right = 0, bottom = 0; };
struct TextStyle {
    int fontSize = 14;
    Color color = {0, 0, 0};
    Color backgroundColor = {255, 255, 255, 0};
    std::string fontFamily = "arial.ttf";
};

// === Widget Interface ===
class Widget {
public:
    virtual ~Widget() = default;
    virtual void render(SDL_Renderer* renderer, int x, int y) = 0;
    virtual void handleEvent(SDL_Event* event, int x, int y) = 0;
    virtual SDL_Rect getLayout(int parentX, int parentY) = 0;
    
    EdgeInsets padding = {0, 0, 0, 0};
    Color backgroundColor = {255, 255, 255, 0};
};

// === Space (Root Container) ===
class Space : public Widget {
public:
    Space(Widget* child, Color bgColor = {240, 240, 240}) 
        : child(child), backgroundColor(bgColor) {}

    void render(SDL_Renderer* renderer, int x, int y) override {
        // Draw background
        SDL_Rect windowRect = {0, 0, 360, 480}; // Assuming a window size of 360x480
        SDL_SetRenderDrawColor(renderer, 
            backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRect(renderer, &windowRect);
        
        // Draw child
        if (child) {
            child->render(renderer, x + padding.left, y + padding.top);
        }
    }
    
    void handleEvent(SDL_Event* event, int x, int y) override {
        if (child) {
            child->handleEvent(event, x + padding.left, y + padding.top);
        }
    }
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        if (child) {
            SDL_Rect childLayout = child->getLayout(0, 0);
            return {
                parentX,
                parentY,
                childLayout.w + padding.left + padding.right,
                childLayout.h + padding.top + padding.bottom
            };
        }
        return {parentX, parentY, 0, 0};
    }

private:
    std::unique_ptr<Widget> child;
    Color backgroundColor;
};



// === Basic Widgets ===
class Text : public Widget {
public:
    Text(std::string text, TextStyle style = {}) 
        : text(text), style(style) {}

    void render(SDL_Renderer* renderer, int x, int y) override {
        TTF_Font* font = TTF_OpenFont(style.fontFamily.c_str(), style.fontSize);
        if (!font) {
            printf("Failed to load font: %s\n", TTF_GetError());
            return; 
        }

        SDL_Color sdlColor = {style.color.r, style.color.g, style.color.b};
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), sdlColor);
        if (!surface) {
            printf("Failed to create surface: %s\n", TTF_GetError());
            TTF_CloseFont(font);
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect rect = getLayout(x, y);
        
        // Draw background if specified
        if (style.backgroundColor.a > 0) {
            SDL_SetRenderDrawColor(renderer, 
                style.backgroundColor.r, style.backgroundColor.g, 
                style.backgroundColor.b, style.backgroundColor.a);
            SDL_RenderFillRect(renderer, &rect);
        }
        
        SDL_RenderCopy(renderer, texture, nullptr, &rect);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
        TTF_CloseFont(font);
    }
    
    void handleEvent(SDL_Event*, int, int) override {}
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        TTF_Font* font = TTF_OpenFont(style.fontFamily.c_str(), style.fontSize);
        if (font) {
            SDL_Color sdlColor = {style.color.r, style.color.g, style.color.b};
            SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), sdlColor);
            if (surface) {
                layout = {
                    parentX + padding.left,
                    parentY + padding.top,
                    surface->w,
                    surface->h
                };
                SDL_FreeSurface(surface);
            }
            TTF_CloseFont(font);
        }
        return layout;
    }

private:
    std::string text;
    TextStyle style;
    SDL_Rect layout;
};

class Button : public Widget {
public:
    Button(std::string text, std::function<void()> onPressed = nullptr, TextStyle style = {})
        : child(new Text(text, style)), onPressed(onPressed), textStyle(style) {}

    void render(SDL_Renderer* renderer, int x, int y) override {
        SDL_Rect rect = getLayout(x, y);
        
        // Draw background with hover effect
        Color bg = isHovered ? Color{200, 200, 200} : textStyle.backgroundColor;
        SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw child
        child->render(renderer, rect.x + padding.left, rect.y + padding.top);
    }
    
    void handleEvent(SDL_Event* event, int x, int y) override {
        SDL_Rect rect = getLayout(x, y);
        int mouseX = event->button.x;
        int mouseY = event->button.y;
        
        if (event->type == SDL_MOUSEMOTION) {
            isHovered = (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
                         mouseY >= rect.y && mouseY <= rect.y + rect.h);
        }
        else if (event->type == SDL_MOUSEBUTTONDOWN && isHovered && onPressed) {
            onPressed();
        }
    }
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        SDL_Rect childLayout = child->getLayout(0, 0);
        return {
            parentX + padding.left,
            parentY + padding.top,
            childLayout.w + padding.left + padding.right,
            childLayout.h + padding.top + padding.bottom
        };
    }

    std::function<void()> onPressed;

private:
    std::unique_ptr<Widget> child;
    TextStyle textStyle;
    bool isHovered = false;
};

// === Layout Widgets ===
class Column : public Widget {
public:
    Column(std::initializer_list<Widget*> children, int spacing = 10, 
           EdgeInsets padding = {0, 0, 0, 0}, Color bgColor = {255, 255, 255, 0}) 
        : spacing(spacing) {
        this->padding = padding;
        this->backgroundColor = bgColor;
        for (Widget* child : children) {
            this->children.emplace_back(child);
        }
    }
    
    void render(SDL_Renderer* renderer, int x, int y) override {
        SDL_Rect rect = getLayout(x, y);
        
        // Draw background
        SDL_SetRenderDrawColor(renderer, 
            backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw children
        int currentY = y + padding.top;
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(x + padding.left, currentY);
            child->render(renderer, childLayout.x, childLayout.y);
            currentY += childLayout.h + spacing;
        }
    }
    
    void handleEvent(SDL_Event* event, int x, int y) override {
        int currentY = y + padding.top;
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(x + padding.left, currentY);
            child->handleEvent(event, childLayout.x, childLayout.y);
            currentY += childLayout.h + spacing;
        }
    }
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        int totalHeight = padding.top + padding.bottom;
        int maxWidth = 0;
        
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(0, 0);
            totalHeight += childLayout.h + spacing;
            if (childLayout.w > maxWidth) maxWidth = childLayout.w;
        }
        
        if (!children.empty()) totalHeight -= spacing;
        
        return {
            parentX,
            parentY,
            maxWidth + padding.left + padding.right,
            totalHeight
        };
    }

private:
    std::vector<std::unique_ptr<Widget>> children;
    int spacing;
};

class SpaceBox : public Widget {
public:
    SpaceBox(int width, int height, Color bgColor = {255, 255, 255, 0}) 
        : width(width), height(height) {
        this->backgroundColor = bgColor;
    }

    void render(SDL_Renderer* renderer, int x, int y) override {
        SDL_Rect rect = getLayout(x, y);
        
        // Draw background
        SDL_SetRenderDrawColor(renderer, 
            backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRect(renderer, &rect);
    }
    
    void handleEvent(SDL_Event*, int, int) override {}
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        return {
            parentX + padding.left,
            parentY + padding.top,
            width + padding.left + padding.right,
            height + padding.top + padding.bottom
        };
    }

private:
    int width, height;
};

class Row : public Widget {
public:
    Row(std::initializer_list<Widget*> children, int spacing = 10, 
        EdgeInsets padding = {0, 0, 0, 0}, Color bgColor = {255, 255, 255, 0})
        : spacing(spacing) {
        this->padding = padding;
        this->backgroundColor = bgColor;
        for (Widget* child : children) {
            this->children.emplace_back(child);
        }
    }
    
    void render(SDL_Renderer* renderer, int x, int y) override {
        SDL_Rect rect = getLayout(x, y);
        
        // Draw background
        SDL_SetRenderDrawColor(renderer, 
            backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRect(renderer, &rect);
        
        // Draw children
        int currentX = x + padding.left;
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(currentX, y + padding.top);
            child->render(renderer, childLayout.x, childLayout.y);
            currentX += childLayout.w + spacing;
        }
    }
    
    void handleEvent(SDL_Event* event, int x, int y) override {
        int currentX = x + padding.left;
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(currentX, y + padding.top);
            child->handleEvent(event, childLayout.x, childLayout.y);
            currentX += childLayout.w + spacing;
        }
    }
    
    SDL_Rect getLayout(int parentX, int parentY) override {
        int totalWidth = padding.left + padding.right;
        int maxHeight = 0;
        
        for (auto& child : children) {
            SDL_Rect childLayout = child->getLayout(0, 0);
            totalWidth += childLayout.w + spacing;
            if (childLayout.h > maxHeight) maxHeight = childLayout.h;
        }
        
        if (!children.empty()) totalWidth -= spacing;
        
        return {
            parentX,
            parentY,
            totalWidth,
            maxHeight + padding.top + padding.bottom
        };
    }

private:
    std::vector<std::unique_ptr<Widget>> children;
    int spacing;
};

// === App Runner ===
class App {
public:
    App(Widget& root, const std::string& title = "SDLUI", 
        int width = 0, int height = 0, bool resizable = false)
        : root(root) {
        SDL_Init(SDL_INIT_VIDEO);
        TTF_Init();

        SDL_Rect rootLayout = root.getLayout(0, 0);
        int windowWidth = width > 0 ? width : rootLayout.w;
        int windowHeight = height > 0 ? height : rootLayout.h;

        Uint32 flags = SDL_WINDOW_SHOWN;
        if (resizable) flags |= SDL_WINDOW_RESIZABLE;

        window = SDL_CreateWindow(title.c_str(), 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            windowWidth, windowHeight, flags);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }

    ~App() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }

    void run() {
        bool running = true;
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                root.handleEvent(&event, 0, 0);
            }

            // Use the background color from the root widget
            SDL_SetRenderDrawColor(renderer, 
                root.backgroundColor.r, root.backgroundColor.g, 
                root.backgroundColor.b, root.backgroundColor.a);
            SDL_RenderClear(renderer);
            root.render(renderer, 0, 0);
            SDL_RenderPresent(renderer);
        }
    }

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    Widget& root;
};


// Helper function to create a space
Space space(std::function<Widget*()> builder, Color bgColor = {240, 240, 240}) {
    return Space(builder(), bgColor);
}
} // namespace ui