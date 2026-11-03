#pragma once
#include <tracktion_engine/tracktion_engine.h>
namespace orbit::automation
{
// Retain the parameter only while its Edit is alive; detach before replacement.
// The touched set also survives a native transport stop's automatic punch-out.
class AutomationTouchListener final : public tracktion::engine::AutomatableParameter::Listener
{
public:
    AutomationTouchListener(tracktion::engine::AutomatableParameter::Ptr p, std::function<void(float)> f)
        : parameter(std::move(p)), changed(std::move(f)) { parameter->addListener(this); }
    ~AutomationTouchListener() override { parameter->removeListener(this); }
    void curveHasChanged(tracktion::engine::AutomatableParameter&) override {}
    void parameterChanged(tracktion::engine::AutomatableParameter& p, float value) override { if(changed)changed(p.valueRange.convertTo0to1(value)); }
private:
    tracktion::engine::AutomatableParameter::Ptr parameter;
    std::function<void(float)> changed;
};
}
