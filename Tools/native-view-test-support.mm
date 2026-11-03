#import <Cocoa/Cocoa.h>
#import <ImageIO/ImageIO.h>
#import <QuartzCore/QuartzCore.h>
#include <dlfcn.h>
#include <JuceHeader.h>

bool saveNativePeer(juce::Component& component,const juce::File& file)
{
    auto* peer=component.getPeer();if(peer==nullptr)return false;
    auto* view=(NSView*)peer->getNativeHandle();[[view window] displayIfNeeded];
    // Test-only compatibility capture; newer SDKs remove the direct declaration.
    using Capture=CGImageRef (*)(CGRect,CGWindowListOption,CGWindowID,CGWindowImageOption);
    auto capture=reinterpret_cast<Capture>(dlsym(RTLD_DEFAULT,"CGWindowListCreateImage"));
    auto image=capture!=nullptr?capture(CGRectNull,kCGWindowListOptionIncludingWindow,(CGWindowID)[[view window] windowNumber],kCGWindowImageBoundsIgnoreFraming):nullptr;
    if(image==nullptr)return false;
    auto* url=[NSURL fileURLWithPath:[NSString stringWithUTF8String:file.getFullPathName().toRawUTF8()]];
    auto destination=CGImageDestinationCreateWithURL((CFURLRef)url,CFSTR("public.png"),1,nullptr);
    if(destination==nullptr){CGImageRelease(image);return false;}
    CGImageDestinationAddImage(destination,image,nullptr);const bool ok=CGImageDestinationFinalize(destination);
    CFRelease(destination);CGImageRelease(image);return ok;
}
bool nativeHitIsJuce(juce::Component& component,juce::Point<int> point)
{
    auto* peer=component.getPeer();if(peer==nullptr)return false;
    auto* root=(NSView*)peer->getNativeHandle();const auto local=peer->getComponent().getLocalPoint(&component,point);
    const auto inSuper=[root convertPoint:NSMakePoint(local.x,local.y) toView:[root superview]];
    return [root hitTest:inSuper]==root;
}

static juce::NSViewComponent* findNative(juce::Component& component)
{
    if(auto* native=dynamic_cast<juce::NSViewComponent*>(&component))return native;
    for(auto* child:component.getChildren())if(auto* native=findNative(*child))return native;
    return nullptr;
}
bool nativeGeometryMatchesPeer(juce::Component& editor)
{
    auto* native=findNative(editor);if(native==nullptr || native->getPeer()==nullptr)return false;
    const auto expected=native->getPeer()->getAreaCoveredBy(*native);auto* view=(NSView*)native->getView();
    return view!=nil && NSEqualRects([view frame],NSMakeRect(expected.getX(),expected.getY(),expected.getWidth(),expected.getHeight()));
}
bool nativeMaskStable(juce::Component& editor,const std::function<void()>& action)
{
    auto* native=findNative(editor);if(native==nullptr)return false;
    auto* layer=[[(NSView*)native->getView() superview] layer];auto* mask=[layer mask];
    if(![mask isKindOfClass:[CAShapeLayer class]])return false;
    [mask retain];auto path=CGPathRetain([(CAShapeLayer*)mask path]);action();
    const bool same=[layer mask]==mask && [(CAShapeLayer*)mask path]==path;
    CGPathRelease(path);[mask release];return same;
}

void postNativeSpace(juce::Component& component,bool repeat=false,NSUInteger modifiers=0)
{
    auto* peer=component.getPeer();if(peer==nullptr)return;
    NSWindow* window=[(NSView*)peer->getNativeHandle() window];
    for(const auto type:{NSEventTypeKeyDown,NSEventTypeKeyUp})
    {
        NSEvent* event=[NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:modifiers timestamp:[NSProcessInfo processInfo].systemUptime windowNumber:[window windowNumber] context:nil characters:@" " charactersIgnoringModifiers:@" " isARepeat:repeat keyCode:49];
        [NSApp postEvent:event atStart:NO];
    }
}
bool focusNativeEditor(juce::Component& editor)
{
    auto* native=findNative(editor);if(native==nullptr || native->getView()==nullptr)return false;
    auto* view=(NSView*)native->getView();return [[view window] makeFirstResponder:view];
}
void activateNativeWindow(juce::Component& component)
{
    if(auto* peer=component.getPeer())
    {
        [NSApp activateIgnoringOtherApps:YES];NSWindow* window=[(NSView*)peer->getNativeHandle() window];[window makeKeyAndOrderFront:nil];[window makeMainWindow];
    }
}
static NSTextView* transportTextFixture=nil;
void installNativeTextInput(juce::Component& component)
{
    auto* peer=component.getPeer();if(peer==nullptr)return;NSView* root=(NSView*)peer->getNativeHandle();
    transportTextFixture=[[NSTextView alloc] initWithFrame:NSMakeRect(30,400,300,40)];[transportTextFixture setString:@"Text"];[root addSubview:transportTextFixture];[[root window] makeFirstResponder:transportTextFixture];[transportTextFixture setSelectedRange:NSMakeRange(4,0)];
}
juce::String nativeTextInputValue(){return transportTextFixture!=nil?juce::String([[transportTextFixture string] UTF8String]):juce::String{};}
void removeNativeTextInput(juce::Component& component)
{
    if(auto* peer=component.getPeer())[[((NSView*)peer->getNativeHandle()) window] makeFirstResponder:(NSView*)peer->getNativeHandle()];
    [transportTextFixture removeFromSuperview];[transportTextFixture release];transportTextFixture=nil;
}
