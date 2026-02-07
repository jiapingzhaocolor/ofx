// SplitTone_v2.ofx — OpenFX port of "Split Tone_v2.dctl"
// Notes:
//  - This effect does NOT convert between log/linear color spaces. The "Input Color Space" choice
//    only selects a middle-gray reference value (exactly as in the DCTL).
//  - Processing is per-channel (RGB) with a 3-zone curve (shadows / preserve mids / highlights),
//    plus an optional on-screen curve overlay.

#include "ofxsImageEffect.h"
#include "ofxsMultiThread.h"
#include "ofxsProcessing.H"

#include <algorithm>
#include <cmath>
#include <memory>

#define kPluginIdentifier "com.jpzhao.SplitToneV2"
#define kPluginName       "Split Tone v2 (DCTL Port)"
#define kPluginGrouping   "Color"
#define kPluginVersionMajor 1
#define kPluginVersionMinor 0

static inline float clampf(float v, float lo, float hi) {
  return std::max(lo, std::min(hi, v));
}

static inline float getMiddleGray(int preset) {
  static const float mg[20] = {
    0.180f, // Linear
    0.413f, // ACEScc
    0.413f, // ACEScct
    0.391f, // ARRI LogC3
    0.278f, // ARRI LogC4
    0.383f, // BMD Film Gen5
    0.312f, // Canon Log
    0.387f, // Canon Log2
    0.330f, // Canon Log3
    0.336f, // DaVinci Intermediate
    0.398f, // DJI D-Log
    0.459f, // Fujifilm F-Log
    0.391f, // Fujifilm F-Log2
    0.458f, // Gamma 2.2
    0.489f, // Gamma 2.4
    0.363f, // Nikon N-Log
    0.423f, // Panasonic V-Log
    0.333f, // RED Log3G10
    0.410f, // Sony S-Log3
    0.488f  // Apple Log
  };
  preset = std::max(0, std::min(19, preset));
  return mg[preset];
}

static inline float applyCurve(float x,
                               float shadowEnd,
                               float highlightStart,
                               float pShadow,
                               float pHighlight) {
  // Match DCTL behavior: clamp only to >= 0
  x = std::max(0.0f, x);

  // Zone 1: Shadows
  if (x <= shadowEnd) {
    if (shadowEnd > 0.0f) {
      float ratio = x / shadowEnd;
      ratio = clampf(ratio, 0.0f, 1.0f);
      return shadowEnd * std::pow(ratio, pShadow);
    }
    return x;
  }
  // Zone 2: Preserve mids (linear passthrough)
  if (x <= highlightStart) {
    return x;
  }
  // Zone 3: Highlights (up to 1.0)
  if (x <= 1.0f) {
    float range = 1.0f - highlightStart;
    if (range > 0.0f) {
      float ratio = (x - highlightStart) / range;
      ratio = clampf(ratio, 0.0f, 1.0f);
      return highlightStart + range * std::pow(ratio, pHighlight);
    }
    return x;
  }
  // Zone 4: Above 1.0
  return x;
}

struct ParamsSnapshot {
  int preset = 9;               // default matches DCTL (DaVinci Intermediate)
  float preserveMidgray = 0.0f; // 0..1
  float pShadow[3] = {1,1,1};   // R,G,B
  float pHighlight[3] = {1,1,1};
  bool showCurve = false;
};

static inline ParamsSnapshot getParamsAtTime(OFX::ChoiceParam* preset,
                                             OFX::DoubleParam* preserve,
                                             OFX::DoubleParam* p1,
                                             OFX::DoubleParam* p2,
                                             OFX::DoubleParam* p3,
                                             OFX::DoubleParam* p4,
                                             OFX::DoubleParam* p5,
                                             OFX::DoubleParam* p6,
                                             OFX::BooleanParam* showCurve,
                                             double time) {
  ParamsSnapshot s;
  preset->getValueAtTime(time, s.preset);

  double v = 0;
  preserve->getValueAtTime(time, v);
  s.preserveMidgray = (float)v;

  p1->getValueAtTime(time, v); s.pShadow[0] = (float)v;
  p2->getValueAtTime(time, v); s.pShadow[1] = (float)v;
  p3->getValueAtTime(time, v); s.pShadow[2] = (float)v;

  p4->getValueAtTime(time, v); s.pHighlight[0] = (float)v;
  p5->getValueAtTime(time, v); s.pHighlight[1] = (float)v;
  p6->getValueAtTime(time, v); s.pHighlight[2] = (float)v;

  bool b = false;
  showCurve->getValueAtTime(time, b);
  s.showCurve = b;

  return s;
}

// Float RGBA processor
class SplitToneProcessor : public OFX::ImageProcessor {
public:
  SplitToneProcessor(OFX::ImageEffect &instance)
  : OFX::ImageProcessor(instance) {}

  void setSrcImg(const OFX::Image *src) { _src = src; }
  void setParams(const ParamsSnapshot& p) { _p = p; }

  void multiThreadProcessImages(OfxRectI procWindow) override {
    const OFX::Image* src = _src;
    OFX::Image* dst = _dstImg;
    if (!src || !dst) return;

    const int w = dst->getBounds().x2 - dst->getBounds().x1;
    const int h = dst->getBounds().y2 - dst->getBounds().y1;

    // Compute boundaries (same as DCTL)
    const float midGray = getMiddleGray(_p.preset);
    const float gapDist = midGray * _p.preserveMidgray;
    float shadowEnd = std::max(0.0f, midGray - gapDist);
    float highlightStart = std::min(1.0f, midGray + gapDist);

    for (int y = procWindow.y1; y < procWindow.y2; ++y) {
      for (int x = procWindow.x1; x < procWindow.x2; ++x) {
        const float* srcPix = (const float*)src->getPixelAddress(x, y);
        float* dstPix = (float*)dst->getPixelAddress(x, y);
        if (!srcPix || !dstPix) continue;

        float r = srcPix[0];
        float g = srcPix[1];
        float b = srcPix[2];
        float a = srcPix[3];

        float rOut = applyCurve(r, shadowEnd, highlightStart, _p.pShadow[0], _p.pHighlight[0]);
        float gOut = applyCurve(g, shadowEnd, highlightStart, _p.pShadow[1], _p.pHighlight[1]);
        float bOut = applyCurve(b, shadowEnd, highlightStart, _p.pShadow[2], _p.pHighlight[2]);

        if (_p.showCurve && w > 0 && h > 0) {
          // DCTL: x_norm = X/Width; y_norm = 1 - Y/Height
          // In OFX, image bounds may not start at (0,0), so normalize relative to bounds.
          const OfxRectI bnd = dst->getBounds();
          const float xNorm = (float)(x - bnd.x1) / (float)w;
          const float yNorm = 1.0f - ((float)(y - bnd.y1) / (float)h);

          const float curveR = applyCurve(xNorm, shadowEnd, highlightStart, _p.pShadow[0], _p.pHighlight[0]);
          const float curveG = applyCurve(xNorm, shadowEnd, highlightStart, _p.pShadow[1], _p.pHighlight[1]);
          const float curveB = applyCurve(xNorm, shadowEnd, highlightStart, _p.pShadow[2], _p.pHighlight[2]);

          const float lineThickness = 2.5f / (float)h;

          // RGB curves
          if (std::fabs(yNorm - curveR) < lineThickness) {
            rOut = 1.0f; gOut = 0.0f; bOut = 0.0f;
          } else if (std::fabs(yNorm - curveG) < lineThickness) {
            rOut = 0.0f; gOut = 1.0f; bOut = 0.0f;
          } else if (std::fabs(yNorm - curveB) < lineThickness) {
            rOut = 0.3f; gOut = 0.5f; bOut = 1.0f;
          } else if (std::fabs(yNorm - xNorm) < lineThickness * 0.6f) {
            // Diagonal reference line
            rOut = rOut * 0.4f + 0.6f;
            gOut = gOut * 0.4f + 0.6f;
            bOut = bOut * 0.4f + 0.6f;
          }

          // Shadow end line (cyan)
          if (std::fabs(xNorm - shadowEnd) < lineThickness * 0.6f) {
            rOut = 0.0f; gOut = 1.0f; bOut = 1.0f;
          }

          // Middle gray line (yellow) — both vertical and horizontal
          if (std::fabs(xNorm - midGray) < lineThickness * 0.6f ||
              std::fabs(yNorm - midGray) < lineThickness * 0.6f) {
            rOut = 1.0f; gOut = 1.0f; bOut = 0.0f;
          }

          // Highlight start line (magenta)
          if (std::fabs(xNorm - highlightStart) < lineThickness * 0.6f) {
            rOut = 1.0f; gOut = 0.0f; bOut = 1.0f;
          }
        }

        dstPix[0] = rOut;
        dstPix[1] = gOut;
        dstPix[2] = bOut;
        dstPix[3] = a;
      }
    }
  }

private:
  const OFX::Image* _src = nullptr;
  ParamsSnapshot _p;
};

class SplitToneEffect : public OFX::ImageEffect {
public:
  SplitToneEffect(OfxImageEffectHandle handle)
  : ImageEffect(handle)
  , _srcClip(fetchClip("Source"))
  , _dstClip(fetchClip("Output"))
  , _preset(fetchChoiceParam("inputColorSpace"))
  , _preserve(fetchDoubleParam("preserveMidgray"))
  , _p1(fetchDoubleParam("shadowR"))
  , _p2(fetchDoubleParam("shadowG"))
  , _p3(fetchDoubleParam("shadowB"))
  , _p4(fetchDoubleParam("highlightR"))
  , _p5(fetchDoubleParam("highlightG"))
  , _p6(fetchDoubleParam("highlightB"))
  , _showCurve(fetchBooleanParam("showCurve"))
  {}

  void render(const OFX::RenderArguments &args) override {
    std::unique_ptr<OFX::Image> dst(_dstClip->fetchImage(args.time));
    std::unique_ptr<const OFX::Image> src(_srcClip->fetchImage(args.time));

    if (!dst || !src) {
      OFX::throwSuiteStatusException(kOfxStatFailed);
    }

    // Expect float RGBA
    if (dst->getPixelDepth() != OFX::eBitDepthFloat || src->getPixelDepth() != OFX::eBitDepthFloat ||
        dst->getPixelComponents() != OFX::ePixelComponentRGBA || src->getPixelComponents() != OFX::ePixelComponentRGBA) {
      OFX::throwSuiteStatusException(kOfxStatErrUnsupported);
    }

    ParamsSnapshot p = getParamsAtTime(_preset, _preserve, _p1, _p2, _p3, _p4, _p5, _p6, _showCurve, args.time);

    SplitToneProcessor proc(*this);
    proc.setDstImg(dst.get());
    proc.setSrcImg(src.get());
    proc.setParams(p);

    proc.setRenderWindow(args.renderWindow);
    proc.process();
  }

  bool isIdentity(const OFX::IsIdentityArguments &args, OFX::Clip * &identityClip, double &identityTime) override {
    ParamsSnapshot p = getParamsAtTime(_preset, _preserve, _p1, _p2, _p3, _p4, _p5, _p6, _showCurve, args.time);

    const bool curveOff = !p.showCurve;
    const bool preserveOff = std::fabs(p.preserveMidgray) < 1e-8f;
    const bool allOnes =
      std::fabs(p.pShadow[0] - 1.0f) < 1e-8f &&
      std::fabs(p.pShadow[1] - 1.0f) < 1e-8f &&
      std::fabs(p.pShadow[2] - 1.0f) < 1e-8f &&
      std::fabs(p.pHighlight[0] - 1.0f) < 1e-8f &&
      std::fabs(p.pHighlight[1] - 1.0f) < 1e-8f &&
      std::fabs(p.pHighlight[2] - 1.0f) < 1e-8f;

    if (curveOff && preserveOff && allOnes) {
      identityClip = _srcClip;
      identityTime = args.time;
      return true;
    }
    return false;
  }

private:
  OFX::Clip *_srcClip = nullptr;
  OFX::Clip *_dstClip = nullptr;

  OFX::ChoiceParam* _preset = nullptr;
  OFX::DoubleParam* _preserve = nullptr;

  OFX::DoubleParam* _p1 = nullptr;
  OFX::DoubleParam* _p2 = nullptr;
  OFX::DoubleParam* _p3 = nullptr;
  OFX::DoubleParam* _p4 = nullptr;
  OFX::DoubleParam* _p5 = nullptr;
  OFX::DoubleParam* _p6 = nullptr;

  OFX::BooleanParam* _showCurve = nullptr;
};

class SplitTonePluginFactory : public OFX::PluginFactoryHelper<SplitTonePluginFactory> {
public:
  SplitTonePluginFactory()
  : OFX::PluginFactoryHelper<SplitTonePluginFactory>(kPluginIdentifier, kPluginVersionMajor, kPluginVersionMinor)
  {}

  void describe(OFX::ImageEffectDescriptor &desc) override {
    desc.setLabels(kPluginName, kPluginName, kPluginName);
    desc.setPluginGrouping(kPluginGrouping);

    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);

    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    desc.setSupportsMultipleClipDepths(false);
  }

  void describeInContext(OFX::ImageEffectDescriptor &desc, OFX::ContextEnum /*context*/) override {
    // Clips
    OFX::ClipDescriptor *srcClip = desc.defineClip("Source");
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);

    OFX::ClipDescriptor *dstClip = desc.defineClip("Output");
    dstClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    dstClip->setSupportsTiles(true);

    // Params
    OFX::PageParamDescriptor *page = desc.definePageParam("Controls");

    // Choice: Input Color Space
    OFX::ChoiceParamDescriptor *preset = desc.defineChoiceParam("inputColorSpace");
    preset->setLabel("Input Color Space");
    preset->setHint("Selects a middle-gray reference (no actual color-space transform).");

    const char* labels[20] = {
      "Linear","ACEScc","ACEScct","ARRI LogC3","ARRI LogC4","BMD Film Gen5",
      "Canon Log","Canon Log2","Canon Log3","DaVinci Intermediate","DJI D-Log",
      "Fujifilm F-Log","Fujifilm F-Log2","Gamma 2.2","Gamma 2.4","Nikon N-Log",
      "Panasonic V-Log","RED Log3G10","Sony S-Log3","Apple Log"
    };
    for (int i=0;i<20;++i) preset->appendOption(labels[i]);
    preset->setDefault(9); // DCTL default
    page->addChild(*preset);

    // Preserve Midgray
    OFX::DoubleParamDescriptor *preserve = desc.defineDoubleParam("preserveMidgray");
    preserve->setLabel("Preserve Midgray");
    preserve->setRange(0.0, 1.0);
    preserve->setDisplayRange(0.0, 1.0);
    preserve->setDefault(0.0);
    preserve->setIncrement(0.01);
    page->addChild(*preserve);

    // Shadow/Highlight RGB sliders
    auto makeSlider = [&](const char* name, const char* label, double defVal) -> OFX::DoubleParamDescriptor* {
      OFX::DoubleParamDescriptor *p = desc.defineDoubleParam(name);
      p->setLabel(label);
      p->setRange(0.2, 2.0);
      p->setDisplayRange(0.2, 2.0);
      p->setDefault(defVal);
      p->setIncrement(0.01);
      return p;
    };

    page->addChild(*makeSlider("shadowR","Shadow Red",1.0));
    page->addChild(*makeSlider("shadowG","Shadow Green",1.0));
    page->addChild(*makeSlider("shadowB","Shadow Blue",1.0));

    page->addChild(*makeSlider("highlightR","Highlight Red",1.0));
    page->addChild(*makeSlider("highlightG","Highlight Green",1.0));
    page->addChild(*makeSlider("highlightB","Highlight Blue",1.0));

    // Show curve
    OFX::BooleanParamDescriptor* showCurve = desc.defineBooleanParam("showCurve");
    showCurve->setLabel("Show Curve");
    showCurve->setDefault(false);
    page->addChild(*showCurve);
  }

  OFX::ImageEffect* createInstance(OfxImageEffectHandle handle, OFX::ContextEnum /*context*/) override {
    return new SplitToneEffect(handle);
  }
};

// IMPORTANT:
// Do NOT export OfxGetPlugin / OfxGetNumberOfPlugins / OfxSetHost yourself when
// compiling the Support Library sources into your plugin.
// The Support Library provides those symbols and calls this function instead.
namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray &ids) {
  static SplitTonePluginFactory p;
  ids.push_back(&p);
}
} // namespace Plugin
} // namespace OFX
