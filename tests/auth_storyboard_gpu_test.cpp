#define IMGUI_DEFINE_MATH_OPERATORS
#include "../src/main/jni/ImGui/imgui_internal.h"
#include "../src/main/jni/System/UI/AuthenticateScene.h"
#include "../src/main/jni/System/UI/AuthenticateFlow.h"
#include "../src/main/jni/System/UI/StartupUI.h"
#include "../src/main/jni/Fonts/fonts.h"
#include "../src/main/jni/Fonts/SPECIAL.h"
#include "../src/main/jni/Fonts/Iconcpp.h"
#include <EGL/eglext.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

ImFont* F50=nullptr;
ImFont* F107=nullptr;
float menu[4]={.69f,.43f,.96f,1.f};
namespace font { ImFont* inter_semibold=nullptr; ImFont* inter_title=nullptr; ImFont* icomoon_page=nullptr; }

namespace {
constexpr int imageWidth=960,imageHeight=540;
constexpr double pi=3.14159265358979323846;
struct Image {
    std::vector<unsigned char> pixels;
    Image():pixels(imageWidth*imageHeight*4) {}
    const unsigned char* At(int x,int y) const {
        x=std::clamp(x,0,imageWidth-1);y=std::clamp(y,0,imageHeight-1);
        return pixels.data()+((imageHeight-1-y)*imageWidth+x)*4;
    }
    double Light(int x,int y) const {
        const auto* p=At(x,y);return p[0]*.2126+p[1]*.7152+p[2]*.0722;
    }
    void Read(GLuint framebuffer=0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer);
        glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
        glPixelStorei(GL_PACK_ALIGNMENT,1);
        glReadPixels(0,0,imageWidth,imageHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
        assert(glGetError()==GL_NO_ERROR);
    }
    void Save(const std::filesystem::path& filename) const {
        std::ofstream output(filename,std::ios::binary);
        output<<"P6\n"<<imageWidth<<' '<<imageHeight<<"\n255\n";
        for(int y=0;y<imageHeight;++y) for(int x=0;x<imageWidth;++x)
            output.write(reinterpret_cast<const char*>(At(x,y)),3);
        assert(output.good());
    }
    double Difference(const Image& other) const {
        double total=0.;
        for(std::size_t i=0;i<pixels.size();++i) if(i%4!=3)
            total+=std::abs(int(pixels[i])-int(other.pixels[i]));
        return total/(imageWidth*imageHeight*3.);
    }
};
struct NativeContext {
    EGLDisplay display=EGL_NO_DISPLAY;
    EGLContext context=EGL_NO_CONTEXT;
    EGLSurface surface=EGL_NO_SURFACE;
    NativeContext() {
        const auto getDisplay=reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        assert(getDisplay);
        display=getDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,nullptr);
        EGLint major,minor;assert(eglInitialize(display,&major,&minor));
        assert(eglBindAPI(EGL_OPENGL_ES_API));
        const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,
            EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
        EGLConfig config;EGLint count;assert(eglChooseConfig(display,attrs,&config,1,&count)&&count);
        const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
        context=eglCreateContext(display,config,EGL_NO_CONTEXT,ca);assert(context!=EGL_NO_CONTEXT);
        const EGLint sa[]={EGL_WIDTH,imageWidth,EGL_HEIGHT,imageHeight,EGL_NONE};
        surface=eglCreatePbufferSurface(display,config,sa);assert(surface!=EGL_NO_SURFACE);
        assert(eglMakeCurrent(display,surface,surface,context));
        std::printf("GLES: %s\n",glGetString(GL_VERSION));
        ImGui::CreateContext();auto& io=ImGui::GetIO();
        io.IniFilename=nullptr;io.DisplaySize={imageWidth,imageHeight};io.DeltaTime=1.f/60.f;
        io.MousePos={-FLT_MAX,-FLT_MAX};
        ImFontConfig fontConfig;fontConfig.FontDataOwnedByAtlas=false;fontConfig.PixelSnapH=true;
        font::inter_title=io.Fonts->AddFontFromMemoryTTF((void*)inter_semibold,sizeof(inter_semibold),28,&fontConfig);
        font::inter_semibold=io.Fonts->AddFontFromMemoryTTF((void*)inter_semibold,sizeof(inter_semibold),20,&fontConfig);
        font::icomoon_page=io.Fonts->AddFontFromMemoryTTF((void*)icomoon_page,sizeof(icomoon_page),18,&fontConfig);
        fontConfig.MergeMode=true;fontConfig.OversampleH=2.5f;fontConfig.OversampleV=2.5f;
        static const ImWchar glyphs[]={0xe000,0xf8ff,0};
        F107=io.Fonts->AddFontFromMemoryCompressedTTF((void*)font_awesome_data1,font_awesome_size1,25,&fontConfig,glyphs);
        ImFontConfig displayFont;displayFont.FontDataOwnedByAtlas=false;
        F50=io.Fonts->AddFontFromMemoryTTF((void*)F50_data,F50_size,30,&displayFont,io.Fonts->GetGlyphRangesDefault());
        io.FontDefault=font::inter_semibold;
        unsigned char* pixels;int width,height;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
        assert(F107->FindGlyphNoFallback(0xf090)&&F107->FindGlyphNoFallback(0xf090)->Visible);
        const auto* signIn=F107->FindGlyphNoFallback(0xf090);
        std::printf("Production font order: title28 body20 page18+Awesome25 F5030; F107size %.1f, glyph bounds [%.1f %.1f %.1f %.1f], advance %.1f\n",
            F107->FontSize,signIn->X0,signIn->Y0,signIn->X1,signIn->Y1,signIn->AdvanceX);
        c::ApplyTheme();c::scale=1.f;
        assert(ImGui_ImplOpenGL3_Init("#version 300 es"));
    }
    ~NativeContext() {
        auth_scene::Abort();ImGui_ImplOpenGL3_Shutdown();ImGui::DestroyContext();
        eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        eglDestroySurface(display,surface);eglDestroyContext(display,context);eglTerminate(display);
    }
};
Image RenderScene(auth_scene::Mode mode,float progress=0.f) {
    glBindFramebuffer(GL_FRAMEBUFFER,0);glViewport(0,0,imageWidth,imageHeight);
    assert(auth_scene::Begin(imageWidth,imageHeight));
    assert(auth_scene::Render(nullptr,{mode,progress,{.5f,.5f}}));
    Image image;image.Read();return image;
}
double FrontVariation(const Image& frame,const Image& baseline) {
    std::vector<double> radii;
    for(int ray=0;ray<64;++ray) {
        const double angle=2.*pi*ray/64.;
        double score=-1.,peakRadius=0.;
        for(int r=12;r<imageWidth*.65;++r) {
            const int x=std::lround(imageWidth*.5+r*std::cos(angle));
            const int y=std::lround(imageHeight*.5+r*std::sin(angle));
            if(x<1||y<1||x>=imageWidth-1||y>=imageHeight-1) break;
            const double light=frame.Light(x,y),difference=light-baseline.Light(x,y);
            if(light>110.&&difference>score) { score=difference;peakRadius=r; }
        }
        if(score>35.) radii.push_back(peakRadius);
    }
    assert(radii.size()>=12);
    return *std::max_element(radii.begin(),radii.end())-*std::min_element(radii.begin(),radii.end());
}
void CheckShader(const std::filesystem::path& output) {
    const auto baseline=RenderScene(auth_scene::Mode::Empty);
    baseline.Save(output/"E5-calm-baseline.ppm");
    const auto calm=RenderScene(auth_scene::Mode::Explosion,1.f);
    assert(calm.pixels==baseline.pixels);
    const auto empty=RenderScene(auth_scene::Mode::Empty,.5f);
    assert(empty.pixels==baseline.pixels);
    RenderScene(auth_scene::Mode::Explosion,0.f).Save(output/"explosion-zero.ppm");
    RenderScene(auth_scene::Mode::Explosion,1.f/108.f).Save(output/"explosion-first-frame.ppm");
    for(float p : {.12f,.22f,.36f,.50f}) {
        const auto explosion=RenderScene(auth_scene::Mode::Explosion,p);
        const double variation=FrontVariation(explosion,baseline);
        std::printf("front p=%.2f variation=%.1f pixels, scene delta=%.3f\n",p,variation,explosion.Difference(baseline));
        assert(variation>imageHeight*.025);
        char name[64];std::snprintf(name,sizeof(name),"explosion-%03d.ppm",int(p*1000.f));
        explosion.Save(output/name);
    }
    for(float p : {.8f,.9f,.97f}) {
        const auto explosion=RenderScene(auth_scene::Mode::Explosion,p);
        std::printf("decay p=%.2f delta=%.5f\n",p,explosion.Difference(baseline));
    }
    assert(auth_scene::TextureBytes()==imageWidth*imageHeight*8u);
    assert(glGetError()==GL_NO_ERROR);
}
void CheckGraphicsState() {
    ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
    auto* draw=ImGui::GetForegroundDrawList();
    draw->AddRectFilled({300,180},{660,360},IM_COL32(180,90,230,210),24);
    draw->AddText({320,210},IM_COL32_WHITE,"Offscreen state restoration");
    ImGui::Render();
    GLuint textures[3],framebuffers[2],samplers[3],unpack,vertexArray;
    glGenTextures(3,textures);glGenFramebuffers(2,framebuffers);glGenSamplers(3,samplers);
    for(int i=0;i<2;++i) {
        glBindTexture(GL_TEXTURE_2D,textures[i]);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,imageWidth,imageHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER,framebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[i],0);
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    }
    glGenBuffers(1,&unpack);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,16,nullptr,GL_STATIC_DRAW);
    glGenVertexArrays(1,&vertexArray);glBindVertexArray(vertexArray);
    const char* vertex="#version 300 es\nvoid main(){gl_Position=vec4(0,0,0,1);}";
    const char* fragment="#version 300 es\nprecision highp float;out vec4 color;void main(){color=vec4(1);}";
    const auto program=clay::Program(vertex,fragment);assert(program);glUseProgram(program);
    const auto configure=[&] {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,framebuffers[0]);
        glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffers[1]);
        glBindVertexArray(vertexArray);glUseProgram(program);
        for(int i=0;i<3;++i) {
            glActiveTexture(GL_TEXTURE0+(i==2?5:i));glBindTexture(GL_TEXTURE_2D,textures[i]);
            glBindSampler(i==2?5:i,samplers[i]);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack);
        glViewport(7,9,imageWidth-15,imageHeight-17);glScissor(3,5,13,17);
        glClearColor(.125f,.25f,.5f,.75f);glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_FALSE);
        glEnable(GL_SCISSOR_TEST);glEnable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
        glEnable(GL_STENCIL_TEST);glEnable(GL_RASTERIZER_DISCARD);glEnable(GL_DITHER);
        glEnable(GL_SAMPLE_COVERAGE);glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glBlendEquationSeparate(GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT);
        glBlendFuncSeparate(GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA,GL_ZERO,GL_ONE);
    };
    for(auto mode : {auth_scene::Mode::Card,auth_scene::Mode::Suck,auth_scene::Mode::Explosion,auth_scene::Mode::Empty}) {
        configure();const auth_scene::SavedState before;
        GLint equationRGB,equationAlpha,sourceRGB,destinationRGB,sourceAlpha,destinationAlpha;
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&equationRGB);glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&equationAlpha);
        glGetIntegerv(GL_BLEND_SRC_RGB,&sourceRGB);glGetIntegerv(GL_BLEND_DST_RGB,&destinationRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&sourceAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&destinationAlpha);
        assert(auth_scene::Begin(imageWidth,imageHeight));
        assert(auth_scene::Render(ImGui::GetDrawData(),{mode,.35f,{.5f,.5f}}));
        const auth_scene::SavedState after;
        assert(before.read==after.read&&before.write==after.write&&before.active==after.active);
        assert(before.texture==after.texture&&before.texture0==after.texture0&&before.texture1==after.texture1);
        assert(before.sampler0==after.sampler0&&before.sampler1==after.sampler1);
        assert(before.unpack==after.unpack&&before.program==after.program&&before.vao==after.vao);
        for(int i=0;i<4;++i) {
            assert(before.viewport[i]==after.viewport[i]&&before.scissorBox[i]==after.scissorBox[i]);
            assert(before.clear[i]==after.clear[i]&&before.mask[i]==after.mask[i]);
        }
        assert(before.scissor==after.scissor&&before.blend==after.blend&&before.depth==after.depth);
        assert(before.cull==after.cull&&before.stencil==after.stencil&&before.discard==after.discard);
        assert(before.dither==after.dither&&before.coverage==after.coverage&&before.alphaCoverage==after.alphaCoverage);
        GLint actual;
        glGetIntegerv(GL_BLEND_EQUATION_RGB,&actual);assert(actual==equationRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA,&actual);assert(actual==equationAlpha);
        glGetIntegerv(GL_BLEND_SRC_RGB,&actual);assert(actual==sourceRGB);
        glGetIntegerv(GL_BLEND_DST_RGB,&actual);assert(actual==destinationRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&actual);assert(actual==sourceAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA,&actual);assert(actual==destinationAlpha);
        glGetIntegerv(GL_SAMPLER_BINDING,&actual);assert(actual==static_cast<GLint>(samplers[2]));
        assert(glGetError()==GL_NO_ERROR);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,0);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glUseProgram(0);glBindVertexArray(0);
    for(int i : {0,1,5}) { glActiveTexture(GL_TEXTURE0+i);glBindSampler(i,0);glBindTexture(GL_TEXTURE_2D,0); }
    glActiveTexture(GL_TEXTURE0);glDeleteTextures(3,textures);glDeleteFramebuffers(2,framebuffers);
    glDeleteSamplers(3,samplers);glDeleteBuffers(1,&unpack);glDeleteVertexArrays(1,&vertexArray);glDeleteProgram(program);
    glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);glDisable(GL_RASTERIZER_DISCARD);glDisable(GL_SAMPLE_COVERAGE);glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glViewport(0,0,imageWidth,imageHeight);
    std::puts("Host framebuffer, samplers, texture units, program, VAO, raster and blend state restoration passed");
}
void CheckTransmissionAndRefraction(const std::filesystem::path& output) {
    const auto baseline=RenderScene(auth_scene::Mode::Empty);
    const auto explosion=RenderScene(auth_scene::Mode::Explosion,.36f);
    double sx=0.,sy=0.,sxx=0.,syy=0.,sxy=0.,count=0.;
    for(int y=imageHeight/2-70;y<imageHeight/2+70;++y) for(int x=imageWidth/2-100;x<imageWidth/2+100;++x) {
        const double a=baseline.Light(x,y),b=explosion.Light(x,y);
        sx+=a;sy+=b;sxx+=a*a;syy+=b*b;sxy+=a*b;count+=1.;
    }
    const double correlation=(sxy-sx*sy/count)/std::sqrt((sxx-sx*sx/count)*(syy-sy*sy/count));
    assert(correlation>.50&&std::abs(sy/count-sx/count)<22.);
    int bentEdges=0,edges=0;
    double largestBend=0.;
    for(int y=imageHeight/2-60;y<=imageHeight/2+60;y+=15)
        for(int x=imageWidth/2-130;x<imageWidth/2+130;++x) {
            const double original=baseline.Light(x+1,y)-baseline.Light(x-1,y);
            if(std::abs(original)<15.) continue;
            ++edges;double strongest=0.;int shifted=x;
            for(int probe=x-10;probe<=x+10;++probe) {
                const double gradient=(explosion.Light(probe+1,y)-explosion.Light(probe-1,y))*(original>0.?1.:-1.);
                if(gradient>strongest) { strongest=gradient;shifted=probe; }
            }
            if(strongest>8.&&std::abs(shifted-x)>=2) ++bentEdges;
            largestBend=std::max(largestBend,double(std::abs(shifted-x)));
        }
    assert(edges>=12&&bentEdges>=8&&largestBend>=3.);
    std::vector<unsigned char> ramp(imageWidth*imageHeight*4);
    std::array<Image,2> responses;
    for(int direction=0;direction<2;++direction) {
        assert(auth_scene::Begin(imageWidth,imageHeight));
        for(int y=0;y<imageHeight;++y) for(int x=0;x<imageWidth;++x) {
            const int index=(y*imageWidth+x)*4;
            const auto value=static_cast<unsigned char>(64+(direction?imageWidth-1-x:x)*128/imageWidth);
            ramp[index]=ramp[index+1]=ramp[index+2]=value;ramp[index+3]=255;
        }
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,auth_scene::baseTexture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexSubImage2D(GL_TEXTURE_2D,0,0,0,imageWidth,imageHeight,GL_RGBA,GL_UNSIGNED_BYTE,ramp.data());
        assert(auth_scene::Render(nullptr,{auth_scene::Mode::Explosion,.22f,{.5f,.5f}}));
        responses[direction].Read();
    }
    int splitPixels=0,maximumSplit=0;
    for(int y=0;y<imageHeight;++y) for(int x=0;x<imageWidth;++x) {
        const auto* a=responses[0].At(x,y);const auto* b=responses[1].At(x,y);
        bool unclipped=true;for(int channel=0;channel<3;++channel) unclipped&=a[channel]>3&&a[channel]<240&&b[channel]>3&&b[channel]<240;
        if(!unclipped) continue;
        const int deltaR=int(a[0])-int(b[0]),deltaG=int(a[1])-int(b[1]),deltaB=int(a[2])-int(b[2]);
        const int split=std::max({deltaR,deltaG,deltaB})-std::min({deltaR,deltaG,deltaB});
        maximumSplit=std::max(maximumSplit,split);if(split>=3) ++splitPixels;
    }
    assert(splitPixels>=50&&maximumSplit>=3);
    responses[0].Save(output/"rgb-source-response.ppm");
    glBindTexture(GL_TEXTURE_2D,auth_scene::baseTexture);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,imageWidth,imageHeight,GL_RGBA,GL_UNSIGNED_BYTE,baseline.pixels.data());
    std::printf("Transparent interior correlation %.4f, checker edges bent %d/%d (max %.0fpx), chromatic source separation %d pixels (max %d levels)\n",
        correlation,bentEdges,edges,largestBend,splitPixels,maximumSplit);
}
const char* StageName(startup_ui::AuthStage stage) {
    using Stage=startup_ui::AuthStage;
    switch(stage) {
        case Stage::Idle:return "A1-idle";
        case Stage::LabelFade:return "A2-label-fade";
        case Stage::IconMove:return "A3-icon-move";
        case Stage::Spin:return "A4-spin";
        case Stage::Droplet:return "A5-morph";
        case Stage::Hover:return "A5-hover";
        case Stage::Fall:return "A6-fall";
        case Stage::Splash:return "A7-splash";
        case Stage::Wave:return "A8-wave";
        case Stage::Check:return "A9-check";
        case Stage::Suck:return "B1-suck";
        case Stage::Singularity:return "B2-singularity";
        case Stage::Flash:return "B3-flash";
        case Stage::Explosion:return "B4-explosion";
        case Stage::Calm:return "B5-calm";
        case Stage::Reveal:return "B6-reveal";
        case Stage::Loading:return "B7-loading";
    }
    return "unknown";
}
Image RenderFlow(startup_ui::LoginTransition& transition,startup_ui::LoadingTimeline& loading,double elapsed) {
    startup_ui::loginTransition=transition;
    const auto frame=transition.At(elapsed);
    assert(auth_scene::Begin(imageWidth,imageHeight));
    ImGui_ImplOpenGL3_NewFrame();ImGui::NewFrame();
    clay::Capture(imageWidth,imageHeight,frame.loginVisible||frame.loadingVisible,false,elapsed);
    if(frame.loginVisible) {
        char key[64]="FREETRIAL";bool keyboard=false;
        const auto actions=startup_ui::RenderLogin(key,sizeof(key),!transition.succeeded,"",true,keyboard,
            [](const char* id,char* key,std::size_t,ImVec2 size) {
                const auto lo=ImGui::GetCursorScreenPos();
                ImGui::InvisibleButton(id,size);
                startup_ui::Text(ImGui::GetWindowDrawList(),startup_ui::Body(),24.f,lo+ImVec2(10,12),IM_COL32(245,240,255,255),key);
            },&frame);
        assert(!actions.submit&&!actions.paste&&!actions.load);
        const auto* login=ImGui::FindWindowByName("Login Menu");
        assert(ImLengthSqr(login->Pos+login->Size*.5f-ImGui::GetMainViewport()->GetCenter())<=2.f);
    }
    if(frame.loadingVisible) {
        if(!loading.active) loading.Start(elapsed,7.);
        const float scale=startup_ui::Fit({680,600});
        const ImVec2 size=ImVec2(680,600)*scale;
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter()-(size+ImVec2(20,20)*scale)*.5f,ImGuiCond_Always);
        ImGui::SetNextWindowSize(size+ImVec2(20,20)*scale,ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
        ImGui::Begin("Storyboard loading",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoMove);
        const startup_ui::LoadingReveal reveal{frame.loadingReveal};
        startup_ui::LoadingCard(ImGui::GetWindowDrawList(),ImGui::GetWindowPos()+ImVec2(10,10)*scale,size,scale,
            loading.Progress(elapsed),static_cast<float>(elapsed-loading.started),&reveal);
        ImGui::End();ImGui::PopStyleVar();
    } else assert(!loading.active);
    ImGui::Render();
    if(frame.stage==startup_ui::AuthStage::Calm) assert(ImGui::GetDrawData()->TotalVtxCount==0);
    assert(auth_scene::Render(ImGui::GetDrawData(),startup_ui::SceneFrame(frame)));
    assert(!auth_scene::Active()&&glGetError()==GL_NO_ERROR);
    Image image;image.Read();return image;
}
void CheckButtonClipping() {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    RenderFlow(transition,loading,3.60);
    const auto baseline=RenderFlow(transition,loading,3.60);
    const auto scale=startup_ui::Fit({720,500});
    const auto windowSize=ImVec2(740,520)*scale;
    const auto lo=ImGui::GetMainViewport()->GetCenter()-windowSize*.5f+ImVec2(70,368)*scale;
    const auto hi=lo+ImVec2(600,66)*scale;
    const auto center=(lo+hi)*.5f;
    const float radius=(hi.y-lo.y)*.5f;
    int testedPixels=0;
    for(double time : {2.75,2.80,3.0,3.15,3.35,3.60,3.80,4.0,4.39}) {
        const auto image=RenderFlow(transition,loading,time);
        for(int y=int(lo.y)-4;y<=int(hi.y)+4;++y) for(int x=int(lo.x)-4;x<=int(hi.x)+4;++x) {
            const float qx=std::max(0.f,std::abs(x+.5f-center.x)-(hi.x-lo.x)*.5f+radius);
            const float qy=std::abs(y+.5f-center.y);
            if(std::sqrt(qx*qx+qy*qy)-radius<=1.5f) continue;
            const auto* a=image.At(x,y);const auto* b=baseline.At(x,y);
            for(int channel=0;channel<3;++channel) assert(std::abs(int(a[channel])-int(b[channel]))<=1);
            ++testedPixels;
        }
    }
    std::printf("Fluid capsule clipping: %d exterior pixel comparisons unchanged through droplet, fall, splash and wave\n",testedPixels);
}
void CaptureGlyphCalibration(const std::filesystem::path& output) {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    RenderFlow(transition,loading,0.).Save(output/"production-font-idle.ppm");
    RenderFlow(transition,loading,.45).Save(output/"production-font-spin-start.ppm");
    RenderFlow(transition,loading,.90).Save(output/"production-font-spin-diagonal.ppm");
}
void CheckSuckContainment(const std::filesystem::path& output) {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    const auto baseline=RenderScene(auth_scene::Mode::Empty);
    for(float progress : {.2f,.357f,.5f,.79f}) {
        const double now=(startup_ui::AuthTiming::suckStartMs+startup_ui::AuthTiming::suckMs*progress)/1000.;
        const auto image=RenderFlow(transition,loading,now);
        int clippedPixels=0;
        const auto changed=[&](int x,int y) {
            const auto* a=image.At(x,y);const auto* b=baseline.At(x,y);
            return std::max({std::abs(int(a[0])-int(b[0])),std::abs(int(a[1])-int(b[1])),std::abs(int(a[2])-int(b[2]))})>8;
        };
        for(int x=0;x<imageWidth;++x) clippedPixels+=changed(x,0)+changed(x,imageHeight-1);
        for(int y=1;y<imageHeight-1;++y) clippedPixels+=changed(0,y)+changed(imageWidth-1,y);
        char name[48];std::snprintf(name,sizeof(name),"suck-%03d.ppm",int(std::lround(progress*1000)));
        image.Save(output/name);
        std::printf("Suck p=%.3f viewport edge pixels exceeding8-level background delta: %d\n",progress,clippedPixels);
        assert(clippedPixels==0);
    }
}
void RenderSequence(const std::filesystem::path& output) {
    using Timing=startup_ui::AuthTiming;
    startup_ui::LoginTransition transition;
    startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    const auto baseline=RenderScene(auth_scene::Mode::Empty);
    const std::vector<double> captures={0.,75.,150.,300.,450.,1450.,2433.333,2450.,2575.,2700.,2750.,2800.,2975.,3150.,3375.,3600.,3800.,4000.,4383.333,
        4400.,4575.,4750.,5200.,5250.,5500.,5800.,5933.333,5950.,6025.,6100.,6175.,6250.,6266.667,6430.,6700.,7150.,7600.,8033.333,8050.,8066.667,8225.,8383.333,
        8400.,8450.,8500.,8600.,8750.,8833.333,8850.,9600.};
    std::ofstream metadata(output/"frames.csv");metadata<<"requested_ms,actual_ms,stage,file\n";
    std::ofstream movie(output/"sequence.rgb",std::ios::binary);
    std::ofstream counts(output/"sequence.json");
    int movieFrames=0,calmFrames=0,loadingFrames=0;
    double firstLoading=-1.,firstSettled=-1.;
    const auto wallStarted=std::chrono::steady_clock::now();
    for(int index=0;index<=std::lround(Timing::reviewEndMs*.06);++index) {
        const double elapsed=index/60.;
        const auto frame=transition.At(elapsed);
        if(!frame.loadingVisible) assert(startup_ui::media::uploadCount[1]==0);
        const auto image=RenderFlow(transition,loading,elapsed);
        if(frame.stage==startup_ui::AuthStage::Calm) {
            assert(image.pixels==baseline.pixels);++calmFrames;
        }
        if(frame.loadingVisible) {
            ++loadingFrames;if(firstLoading<0.) firstLoading=elapsed;
            if(frame.ready&&firstSettled<0.) firstSettled=elapsed;
            if(frame.loadingReveal==0.f) assert(image.pixels==baseline.pixels);
        }
        for(double requested : captures) if(index==std::lround(requested*.06)) {
            const auto stage=index==0?"A1-idle-press":StageName(frame.stage);
            char name[96];std::snprintf(name,sizeof(name),"%06d-%s.ppm",int(std::lround(elapsed*1000.)),stage);
            image.Save(output/name);
            metadata<<requested<<','<<elapsed*1000.<<','<<stage<<','<<name<<'\n';
            assert(std::abs(elapsed*1000.-requested)<=1000./60.+.001);
        }
        if(index%2==0&&elapsed*1000.<Timing::reviewEndMs-.001) {
            for(int y=0;y<imageHeight/2;++y) for(int x=0;x<imageWidth/2;++x)
                movie.write(reinterpret_cast<const char*>(image.At(x*2,y*2)),3);
            ++movieFrames;
        }
    }
    const auto wallSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-wallStarted).count();
    counts<<"{\"width\":"<<imageWidth/2<<",\"height\":"<<imageHeight/2<<",\"fps\":30,\"frames\":"<<movieFrames<<"}\n";
    assert(calmFrames==std::lround(Timing::calmMs*.06));
    assert(std::abs(firstLoading-Timing::revealStartMs/1000.)<1./60.+.0001);
    assert(std::abs(firstSettled-Timing::revealEndMs/1000.)<1./60.+.0001);
    std::printf("Sequence: %d calmframes exactly checker-only, loading first %.3fs, settled %.3fs, %d loadingframes\n",calmFrames,firstLoading,firstSettled,loadingFrames);
    std::printf("Software Mesa render and readback wall time %.3fs for %.3fs animation; not an Android FPS measurement\n",wallSeconds,Timing::reviewEndMs/1000.);
}
void CheckLoadingScale() {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    std::array<ImVec2,2> sizes;
    for(int sample=0;sample<2;++sample) {
        const double progress=sample?1.:.55;
        const double time=(startup_ui::AuthTiming::revealStartMs+startup_ui::AuthTiming::revealMs*progress)/1000.;
        RenderFlow(transition,loading,time);
        Image ui;ui.Read(auth_scene::uiFramebuffer);
        int loX=imageWidth,hiX=-1,loY=imageHeight,hiY=-1;
        for(int y=0;y<imageHeight;++y) for(int x=0;x<imageWidth;++x) if(ui.At(x,y)[3]>96) {
            loX=std::min(loX,x);hiX=std::max(hiX,x);loY=std::min(loY,y);hiY=std::max(hiY,y);
        }
        assert(hiX>loX&&hiY>loY);
        sizes[sample]={float(hiX-loX+1),float(hiY-loY+1)};
    }
    const auto expected=startup_ui::LoadingReveal{.55f}.Scale();
    std::printf("Loading body alpha bounds early %.0fx%.0f, settled %.0fx%.0f\n",sizes[0].x,sizes[0].y,sizes[1].x,sizes[1].y);
    assert(sizes[0].x<sizes[1].x&&sizes[0].y<sizes[1].y);
    assert(std::abs(sizes[0].x/sizes[1].x-expected)<.012f);
    assert(std::abs(sizes[0].y/sizes[1].y-expected)<.012f);
    std::printf("Loading fullscreen UI alpha bounds scale %.4fx%.4f, expected %.4f at reveal55%%\n",sizes[0].x/sizes[1].x,sizes[0].y/sizes[1].y,expected);
}
void CheckReducedMotion(const std::filesystem::path& output) {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.,true));transition.Resolve(.1,true);
    const auto check=transition.At(.1);
    assert(check.stage==startup_ui::AuthStage::Check&&check.button.check==1.f&&check.button.wave==1.f);
    assert(!check.loadingVisible&&check.inputBlocked&&!check.ready);
    RenderFlow(transition,loading,.1).Save(output/"reduced-motion-check.ppm");
    assert(!loading.active);
    transition.Presented(check);
    const auto revealed=transition.At(.1+1./60.);
    assert(revealed.stage==startup_ui::AuthStage::Loading&&revealed.loadingReveal==1.f&&revealed.ready);
    RenderFlow(transition,loading,.1+1./60.).Save(output/"reduced-motion-loading.ppm");
    assert(loading.active&&!auth_scene::Active());
    std::puts("Reduced motion: one presented full check then next-frame settled loading, no transition effects");
}
void CheckTipBoundaries(const std::filesystem::path& output) {
    startup_ui::LoginTransition transition;startup_ui::LoadingTimeline loading;
    assert(transition.Start(0.));transition.Resolve(.1,true);
    const double reveal=startup_ui::AuthTiming::revealStartMs/1000.;
    RenderFlow(transition,loading,reveal);
    for(double offset : {2.5-1./60.,2.5,2.5+1./60.,5.-1./60.,5.,5.+1./60.}) {
        const auto image=RenderFlow(transition,loading,reveal+offset);
        if(offset==2.5) image.Save(output/"tip-opacity-boundary.ppm");
        assert(glGetError()==GL_NO_ERROR);
    }
    std::puts("Native loading tip opacity-zero boundaries and adjacent frames render without invalid clip commands");
}
}
int main(int argc,char** argv) {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    const std::filesystem::path output=argc>1?argv[1]:"/tmp/npc-auth-storyboard";
    std::filesystem::create_directories(output);
    NativeContext context;
    CheckShader(output);
    CheckGraphicsState();
    CheckTransmissionAndRefraction(output);
    CaptureGlyphCalibration(output);
    CheckSuckContainment(output);
    CheckButtonClipping();
    if(argc<3||std::string(argv[2])!="--probes-only") RenderSequence(output);
    CheckLoadingScale();
    CheckReducedMotion(output);
    CheckTipBoundaries(output);
    std::puts("Storyboard shader front irregularity, full decay and fixed checkerboard passed");
}
