#ifndef UI_H
#define UI_H

// UI Color Globals
static NVGcolor ui_bg_color;
static NVGcolor ui_button_normal;
static NVGcolor ui_button_hover;
static NVGcolor ui_button_active;
static NVGcolor ui_button_border;
static NVGcolor ui_text_color;
static NVGcolor ui_text_inv_color; // For buttons with light bg
static NVGcolor ui_slider_track;
static NVGcolor ui_slider_knob;
static NVGcolor ui_progress_bg;
static NVGcolor ui_progress_fill;
static NVGcolor ui_textbox_bg;
static float ui_clear_color[4]; // RGBA for glClearColor

// Theme definitions
#define THEME_TRANS 0
#define THEME_DARK  1
#define THEME_WHITE 2

static void SetTheme(int theme) {
    if (theme == THEME_TRANS) {
        // Trans Theme: Blue Background, Darker Pink Buttons, White Text
        ui_bg_color       = nvgRGBA(30, 60, 100, 255);
        ui_button_normal  = nvgRGBA(210, 80, 120, 255);  // Darker Pink
        ui_button_hover   = nvgRGBA(255, 255, 255, 255); // White
        ui_button_active  = nvgRGBA(100, 200, 255, 255); // Cyan/Blue
        ui_button_border  = nvgRGBA(255, 255, 255, 255);
        ui_text_color     = nvgRGBA(255, 255, 255, 255);
        ui_text_inv_color = nvgRGBA(255, 255, 255, 255); // White text on buttons
        ui_slider_track   = nvgRGBA(20, 40, 70, 255);
        ui_slider_knob    = nvgRGBA(210, 80, 120, 255);  // Darker Pink
        ui_progress_bg    = nvgRGBA(20, 40, 70, 255);
        ui_progress_fill  = nvgRGBA(255, 255, 255, 255); // White fill for contrast
        ui_textbox_bg     = nvgRGBA(20, 40, 70, 255);
        
        ui_clear_color[0] = 0.12f; ui_clear_color[1] = 0.24f; ui_clear_color[2] = 0.39f; ui_clear_color[3] = 1.0f;
    } 
    else if (theme == THEME_DARK) {
        // Dark Mode: Grays, Dark Blue accent, Red Viseme Bars
        ui_bg_color       = nvgRGBA(30, 30, 30, 255);
        ui_button_normal  = nvgRGBA(60, 60, 60, 255);
        ui_button_hover   = nvgRGBA(80, 80, 80, 255);
        ui_button_active  = nvgRGBA(100, 100, 100, 255);
        ui_button_border  = nvgRGBA(100, 100, 100, 255);
        ui_text_color     = nvgRGBA(220, 220, 220, 255);
        ui_text_inv_color = nvgRGBA(220, 220, 220, 255);
        ui_slider_track   = nvgRGBA(40, 40, 40, 255);
        ui_slider_knob    = nvgRGBA(150, 150, 150, 255);
        ui_progress_bg    = nvgRGBA(40, 40, 40, 255);
        ui_progress_fill  = nvgRGBA(200, 50, 50, 255);   // Red
        ui_textbox_bg     = nvgRGBA(40, 40, 40, 255);
        
        ui_clear_color[0] = 0.12f; ui_clear_color[1] = 0.12f; ui_clear_color[2] = 0.12f; ui_clear_color[3] = 1.0f;
    }
    else if (theme == THEME_WHITE) {
        // White Mode: Light grays, Black text
        ui_bg_color       = nvgRGBA(240, 240, 240, 255);
        ui_button_normal  = nvgRGBA(220, 220, 220, 255);
        ui_button_hover   = nvgRGBA(200, 200, 200, 255);
        ui_button_active  = nvgRGBA(180, 180, 180, 255);
        ui_button_border  = nvgRGBA(150, 150, 150, 255);
        ui_text_color     = nvgRGBA(20, 20, 20, 255);
        ui_text_inv_color = nvgRGBA(20, 20, 20, 255);
        ui_slider_track   = nvgRGBA(200, 200, 200, 255);
        ui_slider_knob    = nvgRGBA(100, 100, 100, 255);
        ui_progress_bg    = nvgRGBA(200, 200, 200, 255);
        ui_progress_fill  = nvgRGBA(100, 180, 100, 255);
        ui_textbox_bg     = nvgRGBA(255, 255, 255, 255);
        
        ui_clear_color[0] = 0.94f; ui_clear_color[1] = 0.94f; ui_clear_color[2] = 0.94f; ui_clear_color[3] = 1.0f;
    }
}

typedef struct {
    float x, y, w, h;
    const char* text;
    int hovered;
} Button;

typedef struct {
    float x, y, w, h;
    float value; // 0.0 to 1.0
    int dragging;
} Slider;

typedef struct {
    float x, y, w, h;
    float value; // 0.0 to 1.0
} ProgressBar;

typedef struct {
    float x, y, w, h;
    char text[512];
} TextDisplay;

typedef struct {
    float x, y, w, h;
    int keyCode;
    int waitingForKey;
    int hovered;
} HotkeyButton;

int isPointInRect(float x, float y, float rx, float ry, float rw, float rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

void drawButton(NVGcontext* vg, Button* b) {
    if (!vg) return;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 5.0f);
    if (b->hovered)
        nvgFillColor(vg, ui_button_hover);
    else
        nvgFillColor(vg, ui_button_normal);
    nvgFill(vg);
    
    nvgBeginPath(vg);
    nvgStrokeColor(vg, ui_button_border);
    nvgStrokeWidth(vg, 2.0f);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 5.0f);
    nvgStroke(vg);

    nvgFillColor(vg, ui_text_inv_color); 
    nvgFontSize(vg, 24.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, b->x + b->w/2, b->y + b->h/2, b->text, NULL);
}

void drawSlider(NVGcontext* vg, Slider* s) {
    if (!vg) return;
    float cy = s->y + s->h * 0.5f;
    float kr = s->h * 0.5f; // Knob radius based on height

    // Track
    nvgBeginPath(vg);
    nvgRoundedRect(vg, s->x, cy - 2, s->w, 4, 2);
    nvgFillColor(vg, ui_slider_track);
    nvgFill(vg);

    // Knob
    float knobX = s->x + (s->w * s->value);
    // Clamp knob position visuals
    if (knobX < s->x) knobX = s->x;
    if (knobX > s->x + s->w) knobX = s->x + s->w;

    nvgBeginPath(vg);
    nvgCircle(vg, knobX, cy, kr);
    nvgFillColor(vg, ui_slider_knob);
    nvgFill(vg);
    nvgStrokeColor(vg, ui_button_border);
    nvgStrokeWidth(vg, 2.0f);
    nvgStroke(vg);
}

void drawLabel(NVGcontext* vg, const char* text, float x, float y) {
    if (!vg) return;
    nvgFillColor(vg, ui_text_color);
    nvgFontSize(vg, 20.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    nvgText(vg, x, y, text, NULL);
}

void drawProgressBar(NVGcontext* vg, ProgressBar* p) {
    if (!vg) return;
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, p->x, p->y, p->w, p->h, 3.0f);
    nvgFillColor(vg, ui_progress_bg);
    nvgFill(vg);

    // Fill
    float fillW = p->w * p->value;
    if (fillW < 0) fillW = 0;
    if (fillW > p->w) fillW = p->w;
    if (fillW > 0) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, p->x, p->y, fillW, p->h, 3.0f);
        nvgFillColor(vg, ui_progress_fill);
        nvgFill(vg);
    }
}

void drawTextDisplay(NVGcontext* vg, TextDisplay* t) {
    if (!vg) return;
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, t->x, t->y, t->w, t->h, 3.0f);
    nvgFillColor(vg, ui_textbox_bg);
    nvgFill(vg);

    // Text (clipped to box)
    nvgSave(vg);
    nvgScissor(vg, t->x + 4, t->y, t->w - 8, t->h);
    nvgFillColor(vg, ui_text_color);
    nvgFontSize(vg, 16.0f);
    nvgFontFace(vg, "sans");

    float bounds[4];
    float textWidth = nvgTextBounds(vg, 0, 0, t->text, NULL, bounds);
    float boxWidth = t->w - 12;

    if (textWidth > boxWidth) {
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(vg, t->x + t->w - 6, t->y + t->h / 2, t->text, NULL);
    } else {
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, t->x + 6, t->y + t->h / 2, t->text, NULL);
    }
    
    nvgRestore(vg);
}

static const char* GetKeyNameForDisplay(int vk) {
    static char buf[16];
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        buf[0] = (char)vk;
        buf[1] = '\0';
        return buf;
    }
    if (vk >= 0x70 && vk <= 0x7B) {
        sprintf(buf, "F%d", vk - 0x70 + 1);
        return buf;
    }
    sprintf(buf, "0x%02X", vk);
    return buf;
}

void drawHotkeyButton(NVGcontext* vg, HotkeyButton* hk) {
    if (!vg) return;
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, hk->x, hk->y, hk->w, hk->h, 3.0f);
    if (hk->waitingForKey)
        nvgFillColor(vg, ui_button_active);
    else if (hk->hovered)
        nvgFillColor(vg, ui_button_hover);
    else
        nvgFillColor(vg, ui_textbox_bg);
    nvgFill(vg);

    // Border
    nvgBeginPath(vg);
    nvgStrokeColor(vg, ui_button_border);
    nvgStrokeWidth(vg, 1.0f);
    nvgRoundedRect(vg, hk->x, hk->y, hk->w, hk->h, 3.0f);
    nvgStroke(vg);

    // Text
    const char* keyText = hk->waitingForKey ? "..." : GetKeyNameForDisplay(hk->keyCode);
    nvgFillColor(vg, hk->waitingForKey ? ui_text_inv_color : ui_text_color);
    nvgFontSize(vg, 16.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, hk->x + hk->w / 2, hk->y + hk->h / 2, keyText, NULL);
}

void drawSmallButton(NVGcontext* vg, Button* b, int active) {
    if (!vg) return;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 3.0f);
    if (active)
        nvgFillColor(vg, ui_button_active);
    else if (b->hovered)
        nvgFillColor(vg, ui_button_hover);
    else
        nvgFillColor(vg, ui_textbox_bg);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgStrokeColor(vg, ui_button_border);
    nvgStrokeWidth(vg, 1.0f);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 3.0f);
    nvgStroke(vg);

    nvgFillColor(vg, (active || b->hovered) ? ui_text_inv_color : ui_text_color);
    nvgFontSize(vg, 14.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, b->x + b->w / 2, b->y + b->h / 2, b->text, NULL);
}

#endif