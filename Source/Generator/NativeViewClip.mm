#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include "NativeViewClip.h"
#include <algorithm>

@interface OrbitGeneratorClipView : NSView
{
    NSArray<NSValue*>* occluded;
    NSRect bodyArea;
}
- (void)setOccluded:(NSArray<NSValue*>*)rectangles;
- (void)setBodyArea:(NSRect)rectangle;
@end
@implementation OrbitGeneratorClipView
- (BOOL)isFlipped {return YES;}
- (void)dealloc {[occluded release];[super dealloc];}
- (void)setOccluded:(NSArray<NSValue*>*)rectangles
{[rectangles retain];[occluded release];occluded=rectangles;}
- (void)setBodyArea:(NSRect)rectangle {bodyArea=rectangle;}
- (NSView*)hitTest:(NSPoint)point
{
    const auto local=[self convertPoint:point fromView:[self superview]];
    if(!NSPointInRect(local,bodyArea))return nil;
    for(NSValue* value in occluded)if(NSPointInRect(local,[value rectValue]))return nil;
    return [super hitTest:point];
}
@end

namespace orbit::generator
{
struct NativeViewClip::Impl
{
    struct Entry
    {
        juce::Component::SafePointer<juce::NSViewComponent> component;
        NSView* view=nil;
        OrbitGeneratorClipView* clip=nil;
        CAShapeLayer* mask=nil; // Owned by clip's layer.
        juce::Rectangle<int> lastBody;
        NSRect lastClipBounds=NSZeroRect;
        std::vector<juce::Rectangle<int>> lastHoles;
        ~Entry()
        {
            if(component!=nullptr && component->getView()==view)
            {
                if(auto* peer=component->getPeer())
                {
                    auto* root=(NSView*)peer->getNativeHandle();
                    const auto area=peer->getAreaCoveredBy(*component);
                    [view removeFromSuperview];[root addSubview:view];
                    [view setFrame:NSMakeRect(area.getX(),area.getY(),area.getWidth(),area.getHeight())];
                }
            }
            else [view removeFromSuperview];
            [clip removeFromSuperview];[clip release];[view release];
        }
    };
    std::vector<std::unique_ptr<Entry>> entries;
    bool updating=false;
    void visit(juce::Component& component,juce::Component& body,const std::vector<juce::Rectangle<int>>& blocked)
    {
        if(auto* native=dynamic_cast<juce::NSViewComponent*>(&component))
        {
            auto* peer=native->getPeer();auto* view=(NSView*)native->getView();
            if(peer!=nullptr && view!=nil)
            {
                Entry* entry=nullptr;
                for(auto& item:entries)if(item->view==view){entry=item.get();break;}
                if(entry==nullptr)
                {
                    auto item=std::make_unique<Entry>();entry=item.get();
                    entry->component=native;entry->view=[view retain];
                    entry->clip=[[OrbitGeneratorClipView alloc] initWithFrame:NSZeroRect];
                    [entry->clip setWantsLayer:YES];[[entry->clip layer] setMasksToBounds:YES];
                    entry->mask=[CAShapeLayer layer];[entry->mask setFillColor:[[NSColor whiteColor] CGColor]];
                    [[entry->clip layer] setMask:entry->mask];
                    entries.push_back(std::move(item));
                }
                auto* root=(NSView*)peer->getNativeHandle();
                auto clipArea=peer->getAreaCoveredBy(body);
                for(auto* parent=body.getParentComponent();parent!=nullptr;parent=parent->getParentComponent())
                    clipArea=clipArea.getIntersection(peer->getAreaCoveredBy(*parent));
                if([entry->clip superview]!=root){[entry->clip removeFromSuperview];[root addSubview:entry->clip];}
                if([view superview]!=entry->clip){[view removeFromSuperview];[entry->clip addSubview:view];}
                const bool hidden=!native->isShowing() || clipArea.isEmpty();
                if([entry->clip isHidden]!=hidden)[entry->clip setHidden:hidden];
                // Keep the host at the peer origin. JUCE's native attachment
                // writes peer-relative frames during Component movement; an
                // offset clip parent makes those writes visibly jump twice.
                if(!NSEqualRects([entry->clip frame],[root bounds]))[entry->clip setFrame:[root bounds]];
                const auto nativeArea=peer->getAreaCoveredBy(*native);
                const auto frame=NSMakeRect(nativeArea.getX(),nativeArea.getY(),nativeArea.getWidth(),nativeArea.getHeight());
                if(!NSEqualRects([view frame],frame))[view setFrame:frame];
                // Component transforms change the peer frame, but Cocoa needs
                // the untransformed logical bounds to scale native children too.
                const auto logicalSize=NSMakeSize(native->getWidth(),native->getHeight());
                if(!NSEqualSizes([view bounds].size,logicalSize))[view setBoundsSize:logicalSize];

                std::vector<juce::Rectangle<int>> peerHoles;
                for(const auto& rect:blocked)
                {
                    const auto peerRect=peer->getComponent().getLocalArea(&body,rect).getIntersection(clipArea);
                    if(!peerRect.isEmpty())peerHoles.push_back(peerRect);
                }
                const auto clipBounds=[entry->clip bounds];
                if(entry->lastBody!=clipArea || entry->lastHoles!=peerHoles || !NSEqualRects(entry->lastClipBounds,clipBounds))
                {
                    juce::RectangleList<int> visible(clipArea);auto* holes=[NSMutableArray array];
                    for(const auto& rect:peerHoles)
                    {
                        visible.subtract(rect);[holes addObject:[NSValue valueWithRect:NSMakeRect(rect.getX(),rect.getY(),rect.getWidth(),rect.getHeight())]];
                    }
                    [entry->clip setBodyArea:NSMakeRect(clipArea.getX(),clipArea.getY(),clipArea.getWidth(),clipArea.getHeight())];
                    [entry->clip setOccluded:holes];[entry->mask setFrame:clipBounds];
                    auto path=CGPathCreateMutable();
                    for(const auto& rect:visible)
                        CGPathAddRect(path,nullptr,CGRectMake(rect.getX(),rect.getY(),rect.getWidth(),rect.getHeight()));
                    [entry->mask setPath:path];CGPathRelease(path);
                    entry->lastBody=clipArea;entry->lastHoles=std::move(peerHoles);entry->lastClipBounds=clipBounds;
                }
            }
        }
        for(auto* child:component.getChildren())visit(*child,body,blocked);
    }
};
NativeViewClip::NativeViewClip():impl(std::make_unique<Impl>()){}
NativeViewClip::~NativeViewClip()=default;
void NativeViewClip::clear(){impl->entries.clear();}
int NativeViewClip::attachedCount() const{return int(impl->entries.size());}
void NativeViewClip::update(juce::Component& editor,juce::Component& body,const std::vector<juce::Rectangle<int>>& blocked)
{
    if(impl->updating)return;
    const juce::ScopedValueSetter<bool> guard(impl->updating,true);
    if(!body.isShowing()){clear();return;}
    std::erase_if(impl->entries,[](const auto& item){return item->component==nullptr || item->component->getView()!=item->view;});
    [CATransaction begin];[CATransaction setDisableActions:YES];
    impl->visit(editor,body,blocked);
    [CATransaction commit];
}
}
