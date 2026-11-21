#pragma once

void Backdrop() {
    const int width=int(ImGui::GetIO().DisplaySize.x), height=int(ImGui::GetIO().DisplaySize.y);
    glBindFramebuffer(GL_FRAMEBUFFER,0); glViewport(0,0,width,height);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glEnable(GL_SCISSOR_TEST);
    for (int x=0;x<width;x+=6) {
        const float offset=(x/6)%2 ? .18f : 0.f;
        glScissor(x,0,6,height/2); glClearColor(.24f+offset,.15f+offset,.08f+offset,1); glClear(GL_COLOR_BUFFER_BIT);
        glScissor(x,height/2,6,height-height/2); glClearColor(.12f+offset,.24f+offset,.35f+offset,1); glClear(GL_COLOR_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);
}
std::vector<unsigned char> ReadBlur(lite_blur::Texture texture) {
    GLint read=0,draw=0; glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read); glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);
    GLuint framebuffer=0; glGenFramebuffers(1,&framebuffer); glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture.id,0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    std::vector<unsigned char> pixels(std::size_t(texture.width)*texture.height*4);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,0); glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,texture.width,texture.height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER,read); glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);
    glDeleteFramebuffers(1,&framebuffer);
    assert(glGetError()==GL_NO_ERROR);
    return pixels;
}
GLuint GameProgram() {
    const char* vertex="#version 300 es\nvoid main(){gl_Position=vec4(0,0,0,1);}";
    const char* fragment="#version 300 es\nprecision mediump float;out vec4 color;void main(){color=vec4(1);}";
    const auto vs=glCreateShader(GL_VERTEX_SHADER), fs=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vs,1,&vertex,nullptr); glCompileShader(vs);
    glShaderSource(fs,1,&fragment,nullptr); glCompileShader(fs);
    const auto program=glCreateProgram(); glAttachShader(program,vs); glAttachShader(program,fs); glLinkProgram(program);
    GLint linked=0; glGetProgramiv(program,GL_LINK_STATUS,&linked); assert(linked);
    glDeleteShader(vs); glDeleteShader(fs);
    return program;
}
void ValidateBlur() {
    auto& io=ImGui::GetIO(); io.DisplaySize={1280,720};
    lite_blur::Release();
    assert(!lite_blur::Capture({0,0,0,500},1280,720,0).id);
    assert(!lite_blur::Capture({FLT_MAX,0,FLT_MAX,500},1280,720,0).id);
    assert(lite_blur::GetStatistics().textureBytes==0);
    Backdrop();
    const auto program=GameProgram(); glUseProgram(program);
    GLuint textures[3],framebuffer,sampler,vao,unpack;
    glGenTextures(3,textures); glGenFramebuffers(1,&framebuffer); glGenSamplers(1,&sampler); glGenVertexArrays(1,&vao); glGenBuffers(1,&unpack);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,textures[2]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,8,8,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glBindFramebuffer(GL_READ_FRAMEBUFFER,framebuffer);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,textures[2],0);
    glBindTexture(GL_TEXTURE_2D,textures[0]); glBindSampler(0,sampler);
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBindVertexArray(vao); glBindBuffer(GL_PIXEL_UNPACK_BUFFER,unpack); glBufferData(GL_PIXEL_UNPACK_BUFFER,16,nullptr,GL_STATIC_DRAW);
    glPixelStorei(GL_UNPACK_ALIGNMENT,8); glPixelStorei(GL_UNPACK_ROW_LENGTH,7);
    glViewport(9,11,300,200); glScissor(21,22,90,91);
    glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);
    for (auto capability : {GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_RASTERIZER_DISCARD}) glEnable(capability);
    const auto texture=lite_blur::Capture({200,100,200,500},1280,720,0);
    assert(texture.id && texture.width<=64 && texture.height<=192);
    GLint actual[4];
    glGetIntegerv(GL_CURRENT_PROGRAM,actual); assert(actual[0]==GLint(program));
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,actual); assert(actual[0]==GLint(framebuffer));
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,actual); assert(actual[0]==0);
    glGetIntegerv(GL_VIEWPORT,actual); assert(actual[0]==9 && actual[1]==11 && actual[2]==300 && actual[3]==200);
    glGetIntegerv(GL_SCISSOR_BOX,actual); assert(actual[0]==21 && actual[1]==22 && actual[2]==90 && actual[3]==91);
    glGetIntegerv(GL_ACTIVE_TEXTURE,actual); assert(actual[0]==GL_TEXTURE3);
    glGetIntegerv(GL_TEXTURE_BINDING_2D,actual); assert(actual[0]==GLint(textures[1]));
    glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_TEXTURE_BINDING_2D,actual); assert(actual[0]==GLint(textures[0]));
    glGetIntegerv(GL_SAMPLER_BINDING,actual); assert(actual[0]==GLint(sampler)); glActiveTexture(GL_TEXTURE3);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING,actual); assert(actual[0]==GLint(vao));
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,actual); assert(actual[0]==GLint(unpack));
    glGetIntegerv(GL_UNPACK_ALIGNMENT,actual); assert(actual[0]==8);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,actual); assert(actual[0]==7);
    GLboolean color[4]; glGetBooleanv(GL_COLOR_WRITEMASK,color); assert(!color[0] && color[1] && !color[2] && color[3]);
    for (auto capability : {GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,GL_RASTERIZER_DISCARD}) {
        assert(glIsEnabled(capability)); glDisable(capability);
    }
    assert(glGetError()==GL_NO_ERROR);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0); glPixelStorei(GL_UNPACK_ALIGNMENT,4); glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glBindVertexArray(0); glActiveTexture(GL_TEXTURE0); glBindSampler(0,0);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    const auto pixels=ReadBlur(texture);
    double difference=0.; int count=0;
    for (int y=10;y<texture.height-10;++y) for (int x=3;x<texture.width-3;++x) {
        const auto at=(std::size_t(y)*texture.width+x)*4;
        difference+=std::abs(int(pixels[at])-int(pixels[at-4])); ++count;
        assert(pixels[at+3]==255);
    }
    assert(difference/count<3.);
    const auto top=(std::size_t(texture.height-12)*texture.width+texture.width/2)*4;
    const auto bottom=(std::size_t(12)*texture.width+texture.width/2)*4;
    assert(pixels[top+2]>pixels[top] && pixels[bottom]>pixels[bottom+2]);
    const auto bytes=lite_blur::GetStatistics().textureBytes;
    assert(bytes==std::size_t(texture.width)*texture.height*8 && bytes<=98304);
    assert(lite_blur::GetStatistics().captures==1);
    for (int index=1;index<100;++index)
        assert(lite_blur::Capture({200,100,200,500},1280,720,index*.0004).id==texture.id);
    assert(lite_blur::GetStatistics().captures==1);
    assert(lite_blur::Capture({200,100,200,500},1280,720,.051).id);
    assert(lite_blur::GetStatistics().captures==2);
    const auto resized=lite_blur::Capture({200,50,180,600},1280,720,.052);
    assert(resized.id==texture.id && resized.height<=192 && lite_blur::GetStatistics().captures==3);
    GLuint msaa=0,multi=0; glGenFramebuffers(1,&msaa); glGenRenderbuffers(1,&multi);
    glBindRenderbuffer(GL_RENDERBUFFER,multi); glRenderbufferStorageMultisample(GL_RENDERBUFFER,2,GL_RGBA8,128,128);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,msaa); glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,multi);
    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE);
    assert(!lite_blur::Capture({0,0,128,128},128,128,.2).id);
    assert(lite_blur::GetStatistics().captures==3 && glGetError()==GL_NO_ERROR);
    glBindFramebuffer(GL_FRAMEBUFFER,0); glBindRenderbuffer(GL_RENDERBUFFER,0);
    glDeleteFramebuffers(1,&msaa); glDeleteRenderbuffers(1,&multi);
    const auto id=resized.id; lite_blur::Release();
    assert(!glIsTexture(id) && lite_blur::GetStatistics().textureBytes==0);
    glUseProgram(0); glDeleteProgram(program);
    glDeleteTextures(3,textures); glDeleteFramebuffers(1,&framebuffer); glDeleteSamplers(1,&sampler); glDeleteVertexArrays(1,&vao); glDeleteBuffers(1,&unpack);
    assert(glGetError()==GL_NO_ERROR);
    std::printf("Sidebar blur: real low-pass filtering (adjacent delta %.3f), orientation, full GL state preservation, 20-Hz cache, resize, MSAA fallback and release; %zu texture bytes\n",difference/count,bytes);
}
void PrepareSidebar() {
    view.sidebarTexture=allowedTexture=nullptr;
    if (!view.ready || state.minimized) return;
    Backdrop();
    const auto layout=lite_ui::Fit(ImGui::GetIO().DisplaySize);
    const auto pos=(ImGui::GetIO().DisplaySize-layout.size)*.5f;
    const auto texture=lite_blur::Capture({pos.x,pos.y,layout.sidebar,layout.size.y},
        int(ImGui::GetIO().DisplaySize.x),int(ImGui::GetIO().DisplaySize.y),ImGui::GetTime());
    assert(texture.id);
    view.sidebarTexture=allowedTexture=reinterpret_cast<ImTextureID>(static_cast<std::uintptr_t>(texture.id));
}
