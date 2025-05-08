#include "PaintTool.h"
#include "../PluginProcessor.h"

void PaintTool::setViewBounds(int _x, int _y, int _w, int _h)
{
    winx = _x;
    winy = _y;
    winw = _w;
    winh = _h;
}


void PaintTool::draw(Graphics& g)
{
    if ((!audioProcessor.showPaintWidget || audioProcessor.isPaintMode()) && !dragging) 
        return;

    g.setColour(Colours::red);
    auto bounds = getBounds(true);
    g.drawRect(bounds);
    g.fillEllipse(bounds.getTopLeft().x - 2.f, bounds.getTopLeft().y - 2.f, 4.f, 4.f);
    g.fillEllipse(bounds.getTopRight().x - 2.f, bounds.getTopRight().y - 2.f, 4.f, 4.f);
    g.fillEllipse(bounds.getBottomLeft().x - 2.f, bounds.getBottomLeft().y - 2.f, 4.f, 4.f);
    g.fillEllipse(bounds.getBottomRight().x - 2.f, bounds.getBottomRight().y - 2.f, 4.f, 4.f);
}

void PaintTool::mouseMove(const MouseEvent& e)
{
	mousePos = e.getPosition();
}

void PaintTool::mouseDrag(const MouseEvent& e)
{
}

void PaintTool::mouseDown(const MouseEvent& e)
{

}

void PaintTool::apply()
{

}

Rectangle<int> PaintTool::getBounds(bool snap)
{
    int x = mousePos.x - (invertx ? paintW : 0);
    int y = mousePos.y - (inverty ? 0 : paintH);
    int r = x + paintW;
    int b = y + paintH;

    if (x < winx) {
        if (!invertx) r += winx - x;
        x = winx;
    }
    if (r > winx + winw) {
        if (invertx) x += winx + winw - r;
        r = winx + winw;
    }
    if (y < winy) {
        if (inverty) b += winy - y;
        y = winy;
    }
    if (b > winy + winh) {
        if (!inverty) y += winy + winh - b;
        b = winy + winh;
    }

    if (x > r) x = r;
    if (y > b) y = b;

    if (true) {
        auto snapToGrid = [](int coord, int offset, double gridsize) {
            return (int)std::round((offset + std::round((coord - offset) / gridsize) * gridsize));
        };
        double grid = (double)audioProcessor.getCurrentGrid();
        double gridx = winw / grid;
        double gridy = winh / grid;

        int w = snapToGrid(r-x, 0, gridx);
        int h = snapToGrid(b-y, 0, gridy);
        int xx = snapToGrid(invertx ? r : x, winx, gridx);
        int yy = snapToGrid(inverty ? y : b, winy, gridy);

        x = invertx ? xx - w : xx;
        y = inverty ? yy : yy - h;
        x = jlimit(winx, winx+winw, x);
        y = jlimit(winy, winy+winh, y);
        r = x+w;
        b = y+h;
    }

    x = jlimit(winx, winx+winw, x);
    y = jlimit(winy, winy+winh, y);
    r = jlimit(winx, winx+winw, r);
    b = jlimit(winy, winy+winh, b);
    return Rectangle<int>(x,y,r-x,b-y);
}

bool PaintTool::isSnapping(const MouseEvent& e) {
    bool snap = audioProcessor.params.getRawParameterValue("snap")->load() == 1.0f;
    return (snap && !e.mods.isCtrlDown()) || (!snap && e.mods.isCtrlDown());
}