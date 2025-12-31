#ifndef UI_H
#define UI_H

// Trans Flag Colors
#define TRANS_BLUE      nvgRGBA(91, 206, 250, 255)
#define TRANS_PINK      nvgRGBA(245, 169, 184, 255)
#define TRANS_WHITE     nvgRGBA(255, 255, 255, 255)

// UI Element Colors
#define UI_BUTTON_NORMAL TRANS_PINK
#define UI_BUTTON_HOVER  TRANS_WHITE
#define UI_BUTTON_BORDER TRANS_WHITE
#define UI_TEXT_COLOR    TRANS_WHITE
#define UI_SLIDER_TRACK  nvgRGBA(80, 80, 80, 255)
#define UI_SLIDER_KNOB   TRANS_PINK
#define UI_PROGRESS_BG   nvgRGBA(60, 60, 65, 255)
#define UI_PROGRESS_FILL nvgRGBA(100, 255, 100, 255)
#define UI_TEXTBOX_BG    nvgRGBA(60, 60, 65, 255)
#define UI_BUTTON_ACTIVE TRANS_BLUE

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
        nvgFillColor(vg, UI_BUTTON_HOVER);
    else
        nvgFillColor(vg, UI_BUTTON_NORMAL);
    nvgFill(vg);
    
    nvgBeginPath(vg);
    nvgStrokeColor(vg, UI_BUTTON_BORDER);
    nvgStrokeWidth(vg, 2.0f);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 5.0f);
    nvgStroke(vg);

    // Text color logic: Black on White/Light Pink, White on Dark Blue
    // Since Hover is White and Normal is Pink (Light), Black text is safer for contrast?
    // Pink (245,169,184) is quite light. White text on it might be hard to read.
    // Let's use Dark Grey/Black for text on buttons.
    nvgFillColor(vg, nvgRGBA(40, 40, 40, 255)); 
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
    nvgFillColor(vg, UI_SLIDER_TRACK);
    nvgFill(vg);

    // Knob
    float knobX = s->x + (s->w * s->value);
    // Clamp knob position visuals
    if (knobX < s->x) knobX = s->x;
    if (knobX > s->x + s->w) knobX = s->x + s->w;

    nvgBeginPath(vg);
    nvgCircle(vg, knobX, cy, kr);
    nvgFillColor(vg, UI_SLIDER_KNOB);
    nvgFill(vg);
    nvgStrokeColor(vg, UI_BUTTON_BORDER);
    nvgStrokeWidth(vg, 2.0f);
    nvgStroke(vg);
}

void drawLabel(NVGcontext* vg, const char* text, float x, float y) {
    if (!vg) return;
    nvgFillColor(vg, UI_TEXT_COLOR);
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
    nvgFillColor(vg, UI_PROGRESS_BG);
    nvgFill(vg);

    // Fill
    float fillW = p->w * p->value;
    if (fillW < 0) fillW = 0;
    if (fillW > p->w) fillW = p->w;
    if (fillW > 0) {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, p->x, p->y, fillW, p->h, 3.0f);
        nvgFillColor(vg, UI_PROGRESS_FILL);
        nvgFill(vg);
    }
}

void drawTextDisplay(NVGcontext* vg, TextDisplay* t) {
    if (!vg) return;
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, t->x, t->y, t->w, t->h, 3.0f);
    nvgFillColor(vg, UI_TEXTBOX_BG);
    nvgFill(vg);

    // Text (clipped to box)
    nvgSave(vg);
    nvgScissor(vg, t->x + 4, t->y, t->w - 8, t->h);
    nvgFillColor(vg, UI_TEXT_COLOR);
    nvgFontSize(vg, 16.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    nvgText(vg, t->x + 6, t->y + t->h / 2, t->text, NULL);
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
        nvgFillColor(vg, UI_BUTTON_ACTIVE);
    else if (hk->hovered)
        nvgFillColor(vg, UI_BUTTON_HOVER);
    else
        nvgFillColor(vg, UI_TEXTBOX_BG);
    nvgFill(vg);

    // Border
    nvgBeginPath(vg);
    nvgStrokeColor(vg, UI_BUTTON_BORDER);
    nvgStrokeWidth(vg, 1.0f);
    nvgRoundedRect(vg, hk->x, hk->y, hk->w, hk->h, 3.0f);
    nvgStroke(vg);

    // Text
    const char* keyText = hk->waitingForKey ? "..." : GetKeyNameForDisplay(hk->keyCode);
    nvgFillColor(vg, hk->waitingForKey ? nvgRGBA(40, 40, 40, 255) : UI_TEXT_COLOR);
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
        nvgFillColor(vg, UI_BUTTON_ACTIVE);
    else if (b->hovered)
        nvgFillColor(vg, UI_BUTTON_HOVER);
    else
        nvgFillColor(vg, UI_TEXTBOX_BG);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgStrokeColor(vg, UI_BUTTON_BORDER);
    nvgStrokeWidth(vg, 1.0f);
    nvgRoundedRect(vg, b->x, b->y, b->w, b->h, 3.0f);
    nvgStroke(vg);

    nvgFillColor(vg, (active || b->hovered) ? nvgRGBA(40, 40, 40, 255) : UI_TEXT_COLOR);
    nvgFontSize(vg, 14.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
    nvgText(vg, b->x + b->w / 2, b->y + b->h / 2, b->text, NULL);
}

#endif