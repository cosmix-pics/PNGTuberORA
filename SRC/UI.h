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

#endif