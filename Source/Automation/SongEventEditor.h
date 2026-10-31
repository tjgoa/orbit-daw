#pragma once
#include "../Project/ProjectDocument.h"
#include "../Editing/EditShortcuts.h"
#include "../Workspace/WorkspaceWindow.h"

namespace orbit::automation
{
using Points = juce::Array<ProjectDocument::AutomationPoint>;
inline bool samePoints(const Points& a, const Points& b)
{
    if (a.size() != b.size()) return false;
    for (int i = 0; i < a.size(); ++i)
        if (a[i].position != b[i].position || a[i].value != b[i].value) return false;
    return true;
}
struct SongTarget
{
    juce::String trackId, pluginId, pluginName, parameterId, parameterName, label;
};
inline bool sameTarget(const SongTarget& a, const SongTarget& b)
{
    return a.trackId == b.trackId && a.pluginId == b.pluginId && a.parameterId == b.parameterId;
}

class SongEventPanel final : public juce::Component
{
public:
    enum class Tool { pencil, line, erase, move };
    class ToolButton:public juce::TextButton
    {
    public:
        int kind=0;
        void paintButton(juce::Graphics& g,bool over,bool down)override
        {
            auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(getToggleState()?0xff287c9d:over||down?0xff385868:0xff203340));g.fillRoundedRectangle(r,2);g.setColour(juce::Colour(0xff91b9c8));g.drawRoundedRectangle(r,2,1);
            g.setColour(juce::Colour(0xffe3f2fa));const auto c=r.getCentre();
            if(kind==0){g.drawLine(c.x-5,c.y+5,c.x+4,c.y-5,2);g.drawLine(c.x-6,c.y+7,c.x-3,c.y+6,1);}
            else if(kind==1){g.drawLine(c.x-6,c.y+5,c.x+6,c.y-5,1.5f);g.fillEllipse(c.x-7,c.y+4,3,3);g.fillEllipse(c.x+5,c.y-6,3,3);}
            else if(kind==2){g.drawLine(c.x-4,c.y-4,c.x+4,c.y+4,1.5f);g.drawLine(c.x-4,c.y+4,c.x+4,c.y-4,1.5f);}
            else{g.drawArrow({c.x,c.y,c.x-7,c.y},1.5f,4,4);g.drawArrow({c.x,c.y,c.x+7,c.y},1.5f,4,4);}
        }
    };
    class Graph final : public juce::Component
    {
    public:
        explicit Graph(SongEventPanel& p) : panel(p) { setWantsKeyboardFocus(true); }
        juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().withTrimmedTop(24).withTrimmedBottom(5); }
        int stepAt(float x) const { return juce::jlimit(0, panel.lastStep, int(std::floor(x / panel.pixelsPerStep))); }
        float valueAt(float y) const { auto r = plot(); return juce::jlimit(0.0f, 1.0f, (r.getBottom() - y) / juce::jmax(1.0f, r.getHeight())); }
        float yAt(float v) const { auto r = plot(); return r.getBottom() - v * r.getHeight(); }
        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff16232d));
            const auto visible = g.getClipBounds();
            const auto loop = panel.loopPixelRange();
            if (loop.getLength() > 0)
            {
                g.setColour(juce::Colour(0xffffbf55).withAlpha(panel.loopEnabled ? 0.12f : 0.035f));
                g.fillRect(juce::Rectangle<float>(loop.getStart(), 24, loop.getLength(), float(getHeight() - 24)));
                g.setColour(juce::Colour(0xffffbf55).withAlpha(panel.loopEnabled ? 0.55f : 0.18f));
                g.fillRect(juce::Rectangle<float>(loop.getStart(), 0, loop.getLength(), 4));
                g.drawVerticalLine(int(loop.getStart()), 0, float(getHeight()));
                g.drawVerticalLine(int(loop.getEnd()), 0, float(getHeight()));
            }
            g.setColour(juce::Colour(0xff304955));
            for (int i = 0; i <= 4; ++i) g.drawHorizontalLine(juce::roundToInt(yAt(i / 4.0f)), float(visible.getX()), float(visible.getRight()));
            const int stride = panel.pixelsPerStep >= 8 ? 1 : 4;
            for (int step = juce::jmax(0, stepAt(float(visible.getX())) / stride * stride); step <= stepAt(float(visible.getRight())) + stride; step += stride)
            {
                const float x = step * panel.pixelsPerStep;
                const double bar = step / panel.stepsPerBar;
                const bool barLine = std::abs(bar - std::round(bar)) < 0.0001;
                g.setColour(barLine ? juce::Colour(0xff648493) : juce::Colour(0xff263c49));
                g.drawVerticalLine(int(x), 24, float(getHeight()));
            }
            const int firstBar = int(visible.getX() / (panel.stepsPerBar * panel.pixelsPerStep));
            const int labelStride = juce::jmax(1, int(std::ceil(52.0 / (panel.stepsPerBar * panel.pixelsPerStep))));
            g.setColour(juce::Colours::lightgrey);
            for (int bar = firstBar / labelStride * labelStride; bar <= int(std::ceil((panel.lastStep + 1) / panel.stepsPerBar)); bar += labelStride)
            {
                const int x = juce::roundToInt(bar * panel.stepsPerBar * panel.pixelsPerStep);
                if (x > visible.getRight()) break;
                g.drawText(juce::String(bar + 1), x + 3, 0, 48, 23, juce::Justification::centredLeft);
            }
            const auto& displayed=panel.previewActive?panel.previewPoints:panel.points;
            if (displayed.isEmpty())
            {
                g.setColour(juce::Colours::lightgrey);
                g.drawText(panel.targets.empty() ? "No automatable parameters on this track." : "Draw Song events here. No events yet.",
                    visible.withTrimmedTop(50).withHeight(30), juce::Justification::centred);
            }
            else
            {
            juce::Path path;
            path.startNewSubPath(0, yAt(displayed.getFirst().value));
            for (const auto& point : displayed) path.lineTo(point.position * panel.pixelsPerStep, yAt(point.value));
            path.lineTo(float(getWidth()), yAt(displayed.getLast().value));
            if (panel.isGradientFillEnabled())
            {
                auto fill = path; fill.lineTo(float(getWidth()), plot().getBottom());
                fill.lineTo(0, plot().getBottom()); fill.closeSubPath();
                g.setGradientFill(juce::ColourGradient(panel.gradientTop().withAlpha(0.55f), 0, plot().getY(),
                    panel.gradientBottom().withAlpha(0.18f), 0, plot().getBottom(), false));
                g.fillPath(fill);
            }
            g.setColour(juce::Colour(0xff60cee9)); g.strokePath(path, juce::PathStrokeType(2));
            for (const auto& point : displayed)
            {
                const float x = point.position * panel.pixelsPerStep;
                if (x >= visible.getX() - 3 && x <= visible.getRight() + 3) g.fillEllipse(x - 2, yAt(point.value) - 2, 4, 4);
            }
        }
            if (panel.playheadVisible)
            {
                const float x = panel.playheadPixel();
                g.setColour(juce::Colour(0xffff6d6d)); g.drawLine(x, 0, x, plot().getBottom(), 2);
                juce::Path marker; marker.addTriangle(x - 5, 5, x + 5, 5, x, 12); g.fillPath(marker);
            }
        }
        void mouseDown(const juce::MouseEvent& e) override
        {
            grabKeyboardFocus();
            if (e.mods.isPopupMenu()) { panel.showGraphMenu(); return; }
            if(e.mods.isLeftButtonDown() && e.y<24){panel.beginRuler(e);return;}
            if (!e.mods.isLeftButtonDown() || e.position.x < 0 || e.position.x >= getWidth()
                || e.position.y < plot().getY() || e.position.y > plot().getBottom() || panel.targets.empty()) return;
            panel.gesturing = true; panel.before = panel.points;
            panel.startStep = panel.previousStep = stepAt(e.position.x);
            panel.startValue = panel.previousValue = valueAt(e.position.y);
            panel.gestureTool = panel.tool;
            panel.editAt(panel.startStep, panel.startValue); panel.updateReadout(panel.startStep, panel.startValue);
        }
        void mouseDrag(const juce::MouseEvent& e) override
        {
            if(panel.rulerDragging){panel.dragRuler(e);return;}
            if (!panel.gesturing) return;
            const int step = stepAt(e.position.x); const float value = valueAt(e.position.y);
            panel.editAt(step, value); panel.updateReadout(step, value);
        }
        void mouseUp(const juce::MouseEvent& e) override { if(panel.rulerDragging)panel.endRuler(e);else panel.finishGesture(); }
        bool keyPressed(const juce::KeyPress& key) override { return panel.handleKey(key); }
    private:
        SongEventPanel& panel;
    };

    SongEventPanel() : graph(*this)
    {
        for (auto* c : std::initializer_list<juce::Component*>{&trackSelector, &parameterSelector, &zoomSelector, &readout, &transportLabel, &hint, &viewport, &settingsButton}) addAndMakeVisible(c);
        viewport.setViewedComponent(&graph, false); viewport.setScrollBarsShown(false, true);
        trackSelector.onChange = [this] { cancelGesture(); if (onTrack) onTrack(trackSelector.getSelectedId() - 1); };
        parameterSelector.onChange = [this] { cancelGesture(); if (onTarget) onTarget(parameterSelector.getSelectedId() - 1); };
        zoomSelector.addItem("Zoom 50%", 1); zoomSelector.addItem("Zoom 100%", 2); zoomSelector.addItem("Zoom 200%", 3);
        zoomSelector.setSelectedId(2, juce::dontSendNotification);
        zoomSelector.onChange = [this]
        {
            const auto start = viewport.getViewPositionX() / pixelsPerStep;
            pixelsPerStep = zoomSelector.getSelectedId() == 1 ? 4.0f : zoomSelector.getSelectedId() == 3 ? 16.0f : 8.0f;
            resized(); viewport.setViewPosition(juce::roundToInt(start * pixelsPerStep), 0);
        };
        const char* names[] = {"/", "\\", "X", "<>"};
        const char* tips[]={"Draw","Line","Erase","Move Graph"};
        for (int i = 0; i < 4; ++i)
        {
            auto& b = tools[size_t(i)]; addAndMakeVisible(b); b.kind=i;b.setButtonText(names[i]);b.setTooltip(tips[i]);
            b.onClick = [this, i] { tool = Tool(i); for (int j = 0; j < 4; ++j) tools[size_t(j)].setToggleState(j == i, juce::dontSendNotification); };
        }
        recordToggle.setButtonText("Record");recordToggle.setTooltip("Record live parameter movements into Song events. Stop to finish one Undo take.");
        recordToggle.onClick=[this]{if(onRecord)onRecord();};
        followToggle.setButtonText("Follow"); fillToggle.setButtonText("Gradient fill");
        followToggle.setTooltip("Scroll with Song playback, including loop returns. Paused while drawing.");
        fillToggle.setTooltip("Shade the area beneath the automation curve with a gradient.");
        setViewOptions(true, false);
        followToggle.onClick = [this] { followPlayhead(); if (onViewOptionsChanged) onViewOptionsChanged(); };
        fillToggle.onClick = [this] { graph.repaint(); if (onViewOptionsChanged) onViewOptionsChanged(); };
        tools[0].setToggleState(true, juce::dontSendNotification);
        tools[3].setTooltip("Move Graph");
        const char* actions[] = {"Copy", "Paste", "Clear", "Smooth", "Undo", "Redo"};
        for (int i = 0; i < 6; ++i)
        {
            auto& b = buttons[size_t(i)]; b.setButtonText(actions[i]); b.onClick = [this, i] { action(i); };
        }
        hint.setText("Song timeline: choose a track and parameter, then draw. Plays in Song mode. Right-click for Scale.", juce::dontSendNotification);
        hint.setFont(juce::Font(12));
        settingsButton.setButtonText("...");settingsButton.setTooltip("Event Editor settings, colors and LFO tool");settingsButton.onClick=[this]{showSettings();};
        setSize(960, 460);
    }
    ~SongEventPanel() override { viewport.setViewedComponent(nullptr, false); }
    std::function<void(int)> onTrack, onTarget;
    std::function<void(const Points&, const Points&)> onCommit;
    std::function<void(bool)> onUndo;
    std::function<void()> onViewOptionsChanged, onRecord, onLfo;
    std::function<void(double)> onSeek;std::function<void(int,int)> onLoop;
    Points previewPoints;bool previewActive=false;
    int gradientStyle=0;
    juce::Colour gradientTop()const{return juce::Colour(gradientStyle==1?0xfff26567:gradientStyle==2?0xffb58bfa:gradientStyle==3?0xff6be6a3:0xff60cee9);}
    juce::Colour gradientBottom()const{return juce::Colour(gradientStyle==1?0xff407cd9:gradientStyle==2?0xfff4ba60:gradientStyle==3?0xff19838f:0xff247991);}
    void setGradientStyle(int style){gradientStyle=juce::jlimit(0,3,style);graph.repaint();}
    void setPreview(Points data,bool active){previewPoints=std::move(data);previewActive=active;graph.repaint();}
    void setRecording(bool recording) { recordToggle.setToggleState(recording,juce::dontSendNotification); }
    void setViewOptions(bool follow, bool fill)
    {
        followToggle.setToggleState(follow, juce::dontSendNotification);
        fillToggle.setToggleState(fill, juce::dontSendNotification); graph.repaint();
    }
    bool isGradientFillEnabled() const { return fillToggle.getToggleState(); }
    bool isFollowing() const { return followToggle.getToggleState(); }
    float playheadPixel() const { return float(playheadStep * pixelsPerStep); }
    juce::Range<float> loopPixelRange() const { return {float(loopStartStep * pixelsPerStep), float(loopEndStep * pixelsPerStep)}; }
    int firstVisiblePixel() const { return viewport.getViewPositionX(); }
    int visibleGraphWidth() const { return viewport.getViewWidth(); }
    void setTransportState(double step, bool songMode, bool playing, double loopStart, double loopEnd, bool enabled)
    {
        step = std::isfinite(step) ? juce::jmax(0.0, step) : 0.0;
        const bool moved = step != playheadStep || songMode != playheadVisible;
        const bool changed = moved || loopStart != loopStartStep || loopEnd != loopEndStep || enabled != loopEnabled;
        playheadStep = step; playheadVisible = songMode;
        loopStartStep = juce::jlimit(0.0, 65536.0, loopStart); loopEndStep = juce::jlimit(loopStartStep, 65536.0, loopEnd);
        loopEnabled = enabled;
        if (moved && songMode && playing && !gesturing && !rulerDragging) followPlayhead();
        transportLabel.setText((songMode ? (playing ? "Playing" : "Stopped") : "Pattern mode")
            + juce::String(" | Bar ") + juce::String(int(step / stepsPerBar) + 1)
            + " | Loop " + juce::String(int(loopStartStep / stepsPerBar) + 1) + "-"
            + juce::String(int(std::ceil(loopEndStep / stepsPerBar))) + (enabled ? " On" : " Off"), juce::dontSendNotification);
        if (changed) graph.repaint();
    }
    std::function<juce::String(float)> formatValue;
    juce::String projectId, trackId;
    std::vector<SongTarget> targets;
    Points points, clipboard;
    int selectedTarget = -1;
    Graph graph;

    void setTracks(const juce::StringArray& labels, int selected)
    {
        cancelGesture(); trackSelector.clear(juce::dontSendNotification);
        for (int i = 0; i < labels.size(); ++i) trackSelector.addItem(labels[i], i + 1);
        trackSelector.setSelectedId(selected + 1, juce::dontSendNotification);
    }
    void setTargets(std::vector<SongTarget> next, int selected, Points data, double barSteps)
    {
        cancelGesture();setPreview({},false); targets = std::move(next); selectedTarget = selected;
        parameterSelector.clear(juce::dontSendNotification);
        for (int i = 0; i < int(targets.size()); ++i) parameterSelector.addItem(targets[size_t(i)].label, i + 1);
        parameterSelector.setSelectedId(selected + 1, juce::dontSendNotification);
        points = std::move(data); stepsPerBar = barSteps;
        lastStep = juce::jlimit(0, 65535, int(std::ceil(512 * stepsPerBar)) - 1);
        for (const auto& point : points) lastStep = juce::jmax(lastStep, int(std::floor(point.position)));
        resized(); updateReadout(-1, 0); repaint(); graph.repaint();
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff203340));
        g.setColour(juce::Colours::lightgrey);
        const auto area = viewport.getBounds();
        for (int i = 0; i <= 4; ++i)
        {
            const float v = i / 4.0f;
            const int y = area.getY() + juce::roundToInt(graph.yAt(v));
            g.drawText(formatValue ? formatValue(v) : juce::String(v, 2), 2, y - 9, 76, 18, juce::Justification::centredRight);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced(3);
        auto row = r.removeFromTop(28);
        for(auto& button:tools)button.setBounds(row.removeFromLeft(25).reduced(1));
        trackSelector.setBounds(row.removeFromLeft(juce::jmin(200,row.getWidth()/3)).reduced(1));parameterSelector.setBounds(row.reduced(1));
        auto bottom=r.removeFromBottom(27);settingsButton.setBounds(bottom.removeFromRight(28));zoomSelector.setBounds(bottom.removeFromRight(115));
        transportLabel.setBounds(bottom.removeFromRight(300));readout.setBounds(bottom);hint.setBounds({});
        viewport.setBounds(r.withTrimmedLeft(74));
        graph.setSize(juce::jmax(viewport.getWidth(), juce::roundToInt((lastStep + 1) * pixelsPerStep)), juce::jmax(80, viewport.getHeight() - viewport.getScrollBarThickness()));
    }
    bool keyPressed(const juce::KeyPress& key) override { return handleKey(key); }
    bool handleKey(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey && gesturing) { cancelGesture(); return true; }
        if(key.getKeyCode()==juce::KeyPress::deleteKey || key.getKeyCode()==juce::KeyPress::backspaceKey){action(2);return true;}
        const auto mods = key.getModifiers();
        if (!(mods.isCommandDown() || mods.isCtrlDown()) || mods.isAltDown()) return false;
        const auto c = editing::shortcutLetter(key);
        if (c == 'c') action(0); else if (c == 'v') action(1); else if(c=='x'){action(0);action(2);}
        else if (c == 'z') action(mods.isShiftDown() ? 5 : 4); else if (c == 'y') action(5); else return false;
        return true;
    }
    void action(int index)
    {
        cancelGesture();
        if (index == 4 || index == 5) { if (onUndo) onUndo(index == 5); return; }
        if (targets.empty()) return;
        if (index == 0) { clipboard = points; return; }
        auto next = points;
        if (index == 1) next = clipboard;
        if (index == 2) next.clear();
        if (index == 3 && points.size() > 2)
            for (int i = 1; i < points.size() - 1; ++i) next.getReference(i).value = (points[i - 1].value + 2 * points[i].value + points[i + 1].value) / 4;
        commit(next);
    }
    void cancelGesture() { if (gesturing) { points = before; gesturing = false; graph.repaint(); } }
    bool isGesturing() const { return gesturing; }
    void finishGesture()
    {
        if (!gesturing) return;
        const auto next = points; points = before; gesturing = false; commit(next);
    }
private:
    bool rulerDragging=false;float rulerX=0;int rulerMode=0,rulerFirst=0,rulerLast=1;
    void beginRuler(const juce::MouseEvent& e)
    {
        rulerDragging=true;rulerX=e.position.x;rulerFirst=int(std::round(loopStartStep/stepsPerBar));rulerLast=juce::jmax(rulerFirst+1,int(std::round(loopEndStep/stepsPerBar)));
        const auto range=loopPixelRange();rulerMode=std::abs(e.position.x-range.getStart())<8?1:std::abs(e.position.x-range.getEnd())<8?2:range.contains(e.position.x)?3:0;
    }
    void dragRuler(const juce::MouseEvent& e)
    {
        if(std::abs(e.position.x-rulerX)<4 || !onLoop)return;
        const double width=stepsPerBar*pixelsPerStep;int a=rulerFirst,b=rulerLast;const int current=int(std::floor(e.position.x/width)),delta=int(std::round((e.position.x-rulerX)/width));
        if(rulerMode==1)a=juce::jlimit(0,b-1,current);else if(rulerMode==2)b=juce::jlimit(a+1,512,current);else if(rulerMode==3){a=juce::jlimit(0,512-(b-a),rulerFirst+delta);b=a+rulerLast-rulerFirst;}else{a=juce::jlimit(0,511,juce::jmin(int(rulerX/width),current));b=juce::jlimit(a+1,512,juce::jmax(int(rulerX/width),current)+1);}
        onLoop(a+1,b-a);
    }
    void endRuler(const juce::MouseEvent& e)
    {if(std::abs(e.position.x-rulerX)<4 && onSeek)onSeek(e.position.x/pixelsPerStep);rulerDragging=false;}
    void showSettings()
    {
        juce::PopupMenu menu;menu.addItem(1,"Gradient fill",true,isGradientFillEnabled());menu.addItem(2,"Follow play cursor",true,isFollowing());menu.addSeparator();
        const char* colors[]={"Cyan / teal","Red / blue","Violet / gold","Green / teal"};for(int i=0;i<4;++i)menu.addItem(10+i,colors[i],true,gradientStyle==i);
        menu.addSeparator();menu.addItem(20,"LFO Tool...");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&settingsButton),[safe=juce::Component::SafePointer<SongEventPanel>(this)](int choice)
        {if(safe==nullptr)return;if(choice==1)safe->fillToggle.setToggleState(!safe->isGradientFillEnabled(),juce::dontSendNotification);if(choice==2)safe->followToggle.setToggleState(!safe->isFollowing(),juce::dontSendNotification);if(choice>=10 && choice<=13)safe->setGradientStyle(choice-10);if(choice==20 && safe->onLfo)safe->onLfo();safe->graph.repaint();if(choice!=0 && safe->onViewOptionsChanged)safe->onViewOptionsChanged();});
    }
    void followPlayhead()
    {
        if (!isFollowing() || !playheadVisible || gesturing) return;
        const auto x = playheadPixel(); const auto left = viewport.getViewPositionX(); const auto width = viewport.getViewWidth();
        if (x < left || x >= left + width - 24)
            viewport.setViewPosition(juce::jmax(0, juce::roundToInt(x) - 24), 0);
    }
    void commit(const Points& next)
    {
        if (samePoints(points, next) || !onCommit) return;
        const auto old = points; onCommit(old, next); // Host refreshes accepted or rejected data.
    }
    void editAt(int step, float value)
    {
        std::map<double, float> data;
        if (gestureTool == Tool::move)
        {
            points = before;
            if (!points.isEmpty())
            {
                const double delta = juce::jlimit(-points.getFirst().position, std::nextafter(double(lastStep+1),0.0) - points.getLast().position, double(step - startStep));
                for (auto& point : points) point.position += delta;
            }
        }
        else
        {
            const bool line = gestureTool == Tool::line;
            for (const auto& point : line ? before : points) data[point.position] = point.value;
            const int a = line ? startStep : previousStep, b = step;
            const float av = line ? startValue : previousValue;
            data.erase(data.lower_bound(juce::jmin(a,b)),data.lower_bound(juce::jmax(a,b)+1));
            for (int position = juce::jmin(a, b); position <= juce::jmax(a, b); ++position)
                if (gestureTool == Tool::erase) data.erase(position);
                else data[position] = a == b ? value : av + (value - av) * (float(position - a) / float(b - a));
            points.clear(); for (auto [position, v] : data) points.add({position, v});
        }
        previousStep = step; previousValue = value; graph.repaint();
    }
    void updateReadout(int step, float value)
    {
        readout.setText(step < 0 ? juce::String(points.size()) + " Song events. Pattern events remain separate."
            : "Bar " + juce::String(int(step / stepsPerBar) + 1) + " / step " + juce::String(step + 1)
                + "   " + (formatValue ? formatValue(value) : juce::String(value, 3)), juce::dontSendNotification);
    }
    void showGraphMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Copy Events"); menu.addItem(2, "Paste Events"); menu.addItem(3, "Delete Events"); menu.addItem(4, "Smooth");
        menu.addItem(20,"LFO Tool...");
        menu.addItem(10, "Scale 50%"); menu.addItem(11, "Scale 200%");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&graph), [safe = juce::Component::SafePointer<SongEventPanel>(this)](int result)
        {
            if (safe == nullptr) return;
            if(result==20 && safe->onLfo)safe->onLfo();
            if (result >= 1 && result <= 4) safe->action(result - 1);
            if (result == 10 || result == 11)
            {
                safe->cancelGesture(); auto next = safe->points;
                for (auto& point : next) point.value = juce::jlimit(0.0f, 1.0f, point.value * (result == 10 ? 0.5f : 2.0f));
                safe->commit(next);
            }
        });
    }
    juce::TextButton settingsButton;
    juce::ComboBox trackSelector, parameterSelector, zoomSelector;
    juce::Label readout, transportLabel, hint;
    juce::ToggleButton followToggle, fillToggle, recordToggle;
    double playheadStep = 0, loopStartStep = 0, loopEndStep = 0;
    bool playheadVisible = false, loopEnabled = false;
    juce::Viewport viewport;
    std::array<ToolButton, 4> tools;
    std::array<juce::TextButton, 6> buttons;
    Tool tool = Tool::pencil, gestureTool = Tool::pencil;
    Points before;
    bool gesturing = false;
    int startStep = 0, previousStep = 0, lastStep = 8191;
    float startValue = 0, previousValue = 0, pixelsPerStep = 8;
    double stepsPerBar = 16;
};

class SongEventWindow final : public orbit::workspace::Window
{
public:
    SongEventPanel panel;
    SongEventWindow() : Window("Song Event Editor", juce::Colour(0xff203340))
    {
        setUsingNativeTitleBar(false); setContentNonOwned(&panel, false);
        setResizable(true, false); setResizeLimits(520, 260, 4096, 4096);
        setBounds(40,100,1000,480);
    }
    ~SongEventWindow() override { clearContentComponent(); }
    void closeButtonPressed() override { panel.cancelGesture(); Window::closeButtonPressed(); }
};
}
