import Foundation
import AppKit
func load(_ path:String)->CGImage {return NSImage(contentsOfFile:path)!.cgImage(forProposedRect:nil,context:nil,hints:nil)!}
func crop(_ image:CGImage,_ x:Int,_ y:Int,_ w:Int,_ h:Int)->CGImage {return image.cropping(to:CGRect(x:x,y:y,width:w,height:h))!}
func save(_ image:CGImage,_ path:String) {let rep=NSBitmapImageRep(cgImage:image);try! rep.representation(using:.png,properties:[:])!.write(to:URL(fileURLWithPath:path))}
// Usage: swift -module-cache-path /tmp/orbit-swift-cache Tools/extract-reference-header-artwork.swift ribbon-reference.jpg silver-reference.png output-folder
let args=CommandLine.arguments
if args.count != 4 {print("Usage: extract-reference-header-artwork.swift ribbon-reference.jpg silver-reference.png output-folder");exit(1)}
let output=args[3]
try! FileManager.default.createDirectory(atPath:output,withIntermediateDirectories:true)
let source=load(args[1])
let w=source.width,h=64
let context=CGContext(data:nil,width:w,height:h,bitsPerComponent:8,bytesPerRow:w*4,space:CGColorSpaceCreateDeviceRGB(),bitmapInfo:CGImageAlphaInfo.premultipliedLast.rawValue)!
context.interpolationQuality = .high
context.draw(crop(source,0,0,w,h),in:CGRect(x:0,y:0,width:w,height:h))
func patch(_ sourceRect:CGRect,_ target:CGRect,_ radius:CGFloat=0) {
 let image=source.cropping(to:sourceRect)!
 let dest=CGRect(x:target.minX,y:CGFloat(h)-target.maxY,width:target.width,height:target.height)
 context.saveGState()
 if radius<0 {
  let mw=Int(target.width),mh=Int(target.height)
  var bytes=[UInt8](repeating:0,count:mw*mh)
  for y in 0..<mh {for x in 0..<mw {let a=max(0,min(1,min(min(Double(x)/12,Double(mw-1-x)/12),min(Double(y)/10,Double(mh-1-y)/10))));bytes[y*mw+x]=UInt8(a*255)}}
  let provider=CGDataProvider(data:Data(bytes) as CFData)!
  let mask=CGImage(width:mw,height:mh,bitsPerComponent:8,bitsPerPixel:8,bytesPerRow:mw,space:CGColorSpaceCreateDeviceGray(),bitmapInfo:CGBitmapInfo(rawValue:0),provider:provider,decode:nil,shouldInterpolate:true,intent:.defaultIntent)!
  context.clip(to:dest,mask:mask)
 }
 if radius>0 {context.addPath(CGPath(roundedRect:dest,cornerWidth:radius,cornerHeight:radius,transform:nil));context.clip()}
 context.draw(image,in:dest);context.restoreGState()
}
patch(CGRect(x:300,y:50,width:30,height:10),CGRect(x:8,y:4,width:322,height:60),-1)
patch(CGRect(x:700,y:12,width:14,height:8),CGRect(x:376,y:11,width:535,height:38),18)
patch(CGRect(x:1490,y:40,width:175,height:9),CGRect(x:1490,y:5,width:175,height:34),-1)
save(context.makeImage()!,output+"/reference-ribbon.png")
let silver=load(args[2])
save(crop(silver,1320,8,1080,56),output+"/compact-silver.png")
print("Extracted reference ribbon and unlabelled silver surface; no image generation.")
