#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>	// OutputDebugString
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>      // std::ofstream
#include <map>

#include"shared/file.h"
#include"shared/output.h"
#include"shared/math.h"
#include"shared/time.h"
#include"shared/std_ext.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_glfw.h"

#define STB_IMAGE_WRITE_STATIC
#define STBI_UNUSED_SYMBOLS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "shared/stb_image.h"

enum class eDisplayMode {
	NEAREST=0,
	VORONOI=1,
	VORONOI_NORMALIZED=2,
	MASK=3
};

enum class eMaskMode {
	RANDOM=0,
	RANDOM_ANIMATE=1,
	SQUARE=2
};

inline uint32_t DistSquared(uint32_t v0,uint32_t v1) {
	if(v0==0xffffffff || v1==0xffffffff)
		return 0xffffffff;
	int16_t x0=(v0>>16)&0xffff;
	int16_t y0=v0&0xffff;
	int16_t x1=(v1>>16)&0xffff;
	int16_t y1=v1&0xffff;
	int16_t x=x1-x0;
	int16_t y=y1-y0;
	return (x*x)+(y*y);
}
inline uint32_t GetPixel(int x,int y,const uint32_t* pixels,int width,int height) {
	if(x<0 || x>width-1)
		return 0xffffffff;
	if(y<0 || y>height-1)
		return 0xffffffff;
	return pixels[y*width+x];
}

void GenerateMask(uint8_t* mask,int width,int height,eMaskMode mode) {
	memset(mask,0,width*height);
	srand(0);
	switch(mode) {
		case eMaskMode::RANDOM: {
			for(int i=0;i!=100;i++) {
				int x=rand()%width;
				int y=rand()%height;
				mask[y*width+x]=1;
			}
			break;
		}
		case eMaskMode::RANDOM_ANIMATE: {
			float scalar=(float)Time::GetTime()*100+1000;
			float fwidth=(float)width;
			float fheight=(float)height;
			for(int i=0;i!=100;i++) {
				V2 pos=V2(RandomUnitFloat(),RandomUnitFloat())*V2(fwidth,fheight);
				V2 dir=V2(RandomUnitFloat(),RandomUnitFloat());
				pos+=dir*scalar;
				pos.x=fmodf(pos.x,fwidth*2.0f);
				if(pos.x>width) {
					pos.x=fwidth-(pos.x-fwidth);
				}else
				if(pos.x<-width) {
					pos.x=-fwidth-(-pos.x-fwidth);
				}
				pos.y=fmodf(pos.y,fheight*2.0f);
				if(pos.y>height) {
					pos.y=fheight-(pos.y-fheight);
				}else
				if(pos.y<-height) {
					pos.y=-fheight-(-pos.y-fheight);
				}
				int x=CLAMP(0,(int)pos.x,width-1);
				int y=CLAMP(0,(int)pos.y,height-1);
				mask[y*width+x]=1;
			}
			break;
		}
		case eMaskMode::SQUARE: {
			int cx=width>>1;
			int cy=height>>1;
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					int dx=abs(x-cx);
					int dy=abs(y-cy);
					mask[y*width+x]=(dx>32 || dy>32) ? 0:1;
				}
			}
			break;
		}
	}
}

void CalculateNearestNoZero(uint32_t* nearestPixels,const uint8_t* src,int width,int height) {
	for(int y=0;y!=height;y++) {
		uint32_t prevPixel=0xffffffff;
		for(int x=0;x!=width;x++) {
			uint32_t curPixel=(x<<16)|y;
			if(src[y*width+x]) {
				nearestPixels[y*width+x]=curPixel;
			}else{
				uint32_t topPixel=GetPixel(x,y-1,nearestPixels,width,height);
				uint32_t topRightPixel=GetPixel(x+1,y-1,nearestPixels,width,height);
				uint32_t prevDist=DistSquared(curPixel,prevPixel);
				uint32_t topDist=DistSquared(curPixel,topPixel);
				uint32_t topRightDist=DistSquared(curPixel,topRightPixel);
				if(prevDist<topDist) {
					if(prevDist<topRightDist) {
						nearestPixels[y*width+x]=prevPixel;
					}else{
						nearestPixels[y*width+x]=topRightPixel;
					}
				}else{
					if(topDist<topRightDist) {
						nearestPixels[y*width+x]=topPixel;
					}else{
						nearestPixels[y*width+x]=topRightPixel;
					}
				}
			}
			prevPixel=nearestPixels[y*width+x];
		}
	}
	for(int y=height-1;y>=0;y--) {
		uint32_t prevPixel=0xffffffff;
		prevPixel=0xffffffff;
		for(int x=width-1;x>=0;x--) {
			uint32_t curPixel=(x<<16)|y;
			if(src[y*width+x]) {
				nearestPixels[y*width+x]=curPixel;
			}else{
				uint32_t topPixel=GetPixel(x,y+1,nearestPixels,width,height);
				uint32_t topLeftPixel=GetPixel(x-1,y+1,nearestPixels,width,height);
				uint32_t prevDist=DistSquared(curPixel,prevPixel);
				uint32_t topDist=DistSquared(curPixel,topPixel);
				uint32_t topLedtDist=DistSquared(curPixel,topLeftPixel);
				uint32_t existDist=DistSquared(curPixel,nearestPixels[y*width+x]);
				if(prevDist<topDist) {
					if(prevDist<topLedtDist) {
						if(existDist>prevDist) {
							nearestPixels[y*width+x]=prevPixel;
						}
					}else{
						if(existDist>topLedtDist) {
							nearestPixels[y*width+x]=topLeftPixel;
						}
					}
				}else{
					if(topDist<topLedtDist) {
						if(existDist>topDist) {
							nearestPixels[y*width+x]=topPixel;
						}
					}else{
						if(existDist>topLedtDist) {
							nearestPixels[y*width+x]=topLeftPixel;
						}
					}
				}
			}
			prevPixel=nearestPixels[y*width+x];
		}
	}
}
void ConvertNearestToRGB(uint8_t* dstRGB,const uint32_t* nearestPixels,const uint8_t* src,int width,int height,eDisplayMode mode) {
	if(nearestPixels[0]==0xffffffff) {			// mask all zero
		memset(dstRGB,0,width*height*3);
		return;
	}
	uint32_t* colorLookup=new uint32_t[width*height];
	memset(colorLookup,0,width*height*sizeof(*colorLookup));
	switch(mode) {
		case eDisplayMode::NEAREST: {
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					uint32_t nearestPixel=nearestPixels[y*width+x];
					uint32_t color;
					if(src[y*width+x]) {
						color=0xffffff;
					}else{
						int16_t nx=nearestPixel>>16;
						int16_t ny=nearestPixel&0xffff;
						color=colorLookup[ny*width+nx];
						if(!color) {
							color=mRandomU32()&0xffffff;
							colorLookup[ny*width+nx]=color;
						}
					}
					dstRGB[(y*width*3)+(x*3)+0]=(color>>16)&0xff;
					dstRGB[(y*width*3)+(x*3)+1]=(color>>8)&0xff;
					dstRGB[(y*width*3)+(x*3)+2]=color&0xff;
				}
			}
			break;
		}
		case eDisplayMode::VORONOI: {
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					int squaredDistance=DistSquared((x<<16)|y,nearestPixels[y*width+x]);
					float dist=sqrtf((float)squaredDistance);
					uint8_t dist8=CLAMP(0,(int)dist,255);
					dstRGB[(y*width*3)+(x*3)+0]=dist8;
					dstRGB[(y*width*3)+(x*3)+1]=dist8;
					dstRGB[(y*width*3)+(x*3)+2]=dist8;
				}
			}
			break;
		}
		case eDisplayMode::VORONOI_NORMALIZED: {
			int maxDistance=1;
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					int squaredDistance=DistSquared((x<<16)|y,nearestPixels[y*width+x]);
					maxDistance=MAX(maxDistance,squaredDistance);
				}
			}
			float scalar=255.0f/sqrtf((float)maxDistance);
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					int squaredDistance=DistSquared((x<<16)|y,nearestPixels[y*width+x]);
					float dist=sqrtf((float)squaredDistance)*scalar;
					uint8_t dist8=CLAMP(0,(int)dist,255);
					dstRGB[(y*width*3)+(x*3)+0]=dist8;
					dstRGB[(y*width*3)+(x*3)+1]=dist8;
					dstRGB[(y*width*3)+(x*3)+2]=dist8;
				}
			}
			break;
		}
		case eDisplayMode::MASK: {
			for(int y=0;y!=height;y++) {
				for(int x=0;x!=width;x++) {
					uint32_t color=src[y*width+x] ? 0xffffff:0;
					dstRGB[(y*width*3)+(x*3)+0]=(color>>16)&0xff;
					dstRGB[(y*width*3)+(x*3)+1]=(color>>8)&0xff;
					dstRGB[(y*width*3)+(x*3)+2]=color&0xff;
				}
			}
			break;
		}
	}
	delete [] colorLookup;
}


inline ImVec4 ImLoad(const V4& v){
	return ImVec4(v.x,v.y,v.z,v.w);
}
inline ImVec2 ImLoad(const V2& v){
	return ImVec2(v.x,v.y);
}
inline V2 VLoad(const ImVec2& v){
	return V2(v.x,v.y);
}

class GuiLog {
	public:
		enum PRIMITIVE_TYPE {
			NA=0,
			TEXT=1
		};
		struct Primitive {
			Primitive() : m_type(NA) {}
			Primitive(PRIMITIVE_TYPE type) : m_type(type){}
			PRIMITIVE_TYPE m_type;
			uint32_t m_color=0;
			std::string m_text;
		};
		void InitLog();
		void DrawLog();
		void AddText(const char* text);
		std::mutex m_lock;
		std::vector<Primitive> m_primitives;
		int m_maxSize=0;
		int m_tail = 0;
};

void GuiLog::AddText(const char* text) {
	const std::lock_guard<std::mutex> lock(m_lock);
	const int size = int(m_primitives.size());
	Primitive* primitive = nullptr;
	if(size < m_maxSize || m_maxSize < 1) {
		m_primitives.emplace_back();
		primitive = &m_primitives.back();
	}
	else {
		primitive = &m_primitives[m_tail];
	}
	if(m_maxSize > 0) {
		m_tail = (m_tail + 1) % m_maxSize;
	}
	primitive->m_type=TEXT;
	uint32_t color=0xc0c0c0ff;
	if(strstr(text,"ERROR")) {
		color=0xdc3545ff;
	}else
	if(strstr(text,"WARNING")) {
		color=0xffc107ff;
	}else
	if(strstr(text,"NOTIFY")) {
		color=0x007bffff;
	}
	primitive->m_color=color;
	primitive->m_text=text;
}

void GuiLog::InitLog() {
	m_primitives.reserve(m_maxSize);
}

void GuiLog::DrawLog() {
	const std::lock_guard<std::mutex> lock(m_lock);
	const int numPrimitives = (int)m_primitives.size();
	for(int i=0;i!=numPrimitives;i++) {
		const int index = (m_tail + i) % numPrimitives;
		const Primitive* primitive = &m_primitives[index];
		switch(primitive->m_type) {
			case TEXT: {
				int color=primitive->m_color;
				ImGui::TextColored(ImLoad(uint322V4(color)),"%s",primitive->m_text.c_str());
				if(primitive->m_text.rfind('\n')==std::string::npos)
					ImGui::SameLine(0,0);
				break;
			}
			default:
				FATAL("Viewer::DrawLog");
		}
	}
	if(ImGui::GetScrollY()==ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	static V2 popupPosition;
	if(ImGui::IsWindowHovered() && ImGui::IsMouseReleased(1)) {
		popupPosition=VLoad(ImGui::GetMousePos());
		ImGui::OpenPopup("LogPopup");
	}
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(4,4));
	ImGui::SetNextWindowPos(ImLoad(popupPosition));
	if(ImGui::BeginPopup("LogPopup",ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollWithMouse|ImGuiWindowFlags_NoScrollbar)) {
		if(!ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
			ImGui::CloseCurrentPopup();
		}
		if(ImGui::Button("Clear log")) {
			m_primitives.clear();
			m_tail = 0;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar(1);
}

class Viewer {
	public:
		void End();
		void Begin();
		void Run();
		void DrawProfilerDisplay(const std::vector<DisplayTimer>& profilerDisplay);
		std::atomic<bool> m_close=false;
		GuiLog m_log;
};

static void glfw_error_callback(int error,const char* description){
	uprintf("Glfw Error %d: %s\n",error,description);
}

void LineBB(ImVec2& top_left,ImVec2& bottom_right){
	ImGuiWindow* window=ImGui::GetCurrentWindow();
	ImGuiContext& g=*GImGui;
	const ImVec2 size_arg(-1.0f,0);
	ImVec2 pos=window->DC.CursorPos;
	ImVec2 sz=ImGui::CalcItemSize(size_arg,ImGui::CalcItemWidth(),g.FontSize);
	ImRect bb(pos,ImVec2(pos.x+sz.x,pos.y+sz.y));
	top_left=bb.Min;
	bottom_right=bb.Max;
}
void LineMultiRect(float* positions,ImU32* colors,int count){
	ImGuiWindow* window=ImGui::GetCurrentWindow();
	if(window->SkipItems)
		return;
	ImGuiContext& g=*GImGui;
	const ImGuiStyle& style=g.Style;
	const ImVec2 size_arg(-1.0f,0);
	ImVec2 pos=window->DC.CursorPos;
	ImVec2 sz=ImGui::CalcItemSize(size_arg,ImGui::CalcItemWidth(),g.FontSize);
	ImRect bb(pos,ImVec2(pos.x+sz.x,pos.y+sz.y));
	ImGui::ItemSize(bb,style.FramePadding.y);
	if(!ImGui::ItemAdd(bb,0))
		return;
	ImGui::RenderFrame(bb.Min,bb.Max,ImGui::GetColorU32(ImGuiCol_FrameBg),false,0.0f);
	bb.Expand(ImVec2(-style.FrameBorderSize,-style.FrameBorderSize));
	for(int i=0;i!=count;i++){
		ImRect bb1=bb;
		bb1.Min.x=ImMin(bb.Min.x+positions[i*2+0],bb.Max.x);
		bb1.Max.x=ImMin(bb.Min.x+positions[i*2+1],bb.Max.x);
		ImGui::RenderFrame(bb1.Min,bb1.Max,colors[i],false,0.0f);
	}
}
void Viewer::Begin() {
	m_log.InitLog();
}
void Viewer::End() {
}

void Viewer::DrawProfilerDisplay(const std::vector<DisplayTimer>& profilerDisplay){
	for(int i=0;i!=(int)profilerDisplay.size();i++){
		ImGui::Text("%s","");
		ImGui::SameLine(20.0f+(float)profilerDisplay[i].m_depth*20.0f);
		ImGui::Text("%s",profilerDisplay[i].m_name.c_str());
		ImGui::SameLine(270);
		ImGui::Text("%d",profilerDisplay[i].m_count);
		ImGui::SameLine(290+40);
		ImGui::Text("%.2f",profilerDisplay[i].m_time);
		ImGui::SameLine(360+40);
		ImGui::SameLine(380+40);
		ImVec2 topLeft;
		ImVec2 bottomRight;
		LineBB(topLeft,bottomRight);
		float width=bottomRight.x-topLeft.x;
		int count=0;
		float positions[64*2];
		uint32_t colors[64];
		for(int j=0;j!=(int)profilerDisplay[i].m_bars.size();j++){
			positions[count*2+0]=profilerDisplay[i].m_bars[j].m_start*width;
			positions[count*2+1]=profilerDisplay[i].m_bars[j].m_end*width;
			if(positions[count*2+0]==positions[count*2+1]){
				positions[count*2+1]+=1;
			}
			colors[count++]=profilerDisplay[i].m_bars[j].m_color;
			if(count==countof(colors))
				break;
		}
		LineMultiRect(positions,colors,count);
	}
}

void Viewer::Run(){
	glfwSetErrorCallback(glfw_error_callback);
	if(!glfwInit()){
		uprintf("glfwInit failed\n");
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API,GLFW_OPENGL_ES_API);
	const char* glsl_version="#version 300 es";//#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0);

	GLFWwindow* window=glfwCreateWindow(1600,1200,"Nearest non zero",NULL,NULL);
	if(!window){
		uprintf("Unable to create window\n");
		return;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	bool err=glewInit()!=GLEW_OK;
	if(err){
		FATAL("Failed to initialize OpenGL loader!");
		return;
	}
	uprintf("glsl_version %s\n",glsl_version);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io=ImGui::GetIO(); (void)io;
	io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags|=ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags|=ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

	ImGui::StyleColorsDark();

	ImGuiStyle& style=ImGui::GetStyle();
	if(io.ConfigFlags&ImGuiConfigFlags_ViewportsEnable){
		style.WindowRounding=0.0f;
		style.Colors[ImGuiCol_WindowBg].w=1.0f;
	}

	ImGui_ImplGlfw_InitForOpenGL(window,true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	io.Fonts->AddFontFromFileTTF(GetFileNameRemap("$(DATA)/fonts/inconsolata/InconsolataGo-Regular.ttf").c_str(),20.0f);

	std::mutex profilerDisplayLock;
	std::vector<DisplayTimer> profilerDisplay;

	std::vector<uint8_t> pixelsRgb;
	std::vector<uint8_t> pixelsMask;
	int width=512;
	int height=812;
	pixelsRgb.resize(width*height*3);
	pixelsMask.resize(width*height);

	GLuint m_textureId=0;
	glGenTextures(1,&m_textureId);
	glBindTexture(GL_TEXTURE_2D,m_textureId);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP); 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP); 
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,pixelsRgb.data());

	uint32_t* nearestPixels=new uint32_t[width*height];
	memset(nearestPixels,0,width*height*sizeof(*nearestPixels));
	int tick=0;
	eDisplayMode displayMode=eDisplayMode::NEAREST;
	eMaskMode maskMode=eMaskMode::RANDOM;
	while(!glfwWindowShouldClose(window)){
		if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS)
			glfwSetWindowShouldClose(window,true);
		glfwPollEvents();
		auto time=std::chrono::system_clock::now();

		Profiler profiler;
		profiler.m_rangeMilliseconds=100;
		START_TIMER(mainTimer,&profiler,"main",0xffff20);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiID dockspace_id=ImGui::GetID("EditDockSpace");
		if(!tick||ImGui::DockBuilderGetNode(dockspace_id)==NULL){
			ImGui::DockBuilderRemoveNode(dockspace_id);
			ImVec2 dockspace_size=ImGui::GetMainViewport()->Size;
			ImGui::DockBuilderAddNode(dockspace_id,ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspace_id,dockspace_size);
			ImGuiID dock_video_id=dockspace_id;
			ImGuiID dock_prop_id=ImGui::DockBuilderSplitNode(dock_video_id,ImGuiDir_Left,0.50f,NULL,&dock_video_id);
			ImGuiID dock_status_id=ImGui::DockBuilderSplitNode(dock_video_id,ImGuiDir_Down,0.35f,NULL,&dock_video_id);
			ImGui::DockBuilderDockWindow("Frame",dock_prop_id);
			ImGui::DockBuilderDockWindow("Profiler",dock_video_id);
			ImGui::DockBuilderDockWindow("Log",dock_status_id);
			ImGui::DockBuilderFinish(dockspace_id);
		}
		ImGuiViewport* viewport=ImGui::GetMainViewport();
		const ImGuiWindowClass* window_class=0;
		ImVec2 p=viewport->Pos;
		ImVec2 s=viewport->Size;

		ImGui::SetNextWindowPos(p);
		ImGui::SetNextWindowSize(s);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::SetNextWindowBgAlpha(0);

		ImGuiDockNodeFlags dockspace_flags=0;
		ImGuiWindowFlags host_window_flags=0;
		host_window_flags|=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_NoBackground;
		host_window_flags|=ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoNavFocus|ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0.0f,0.0f));
		ImGui::Begin("MainWindow",NULL,host_window_flags);
		ImGui::DockSpace(dockspace_id,ImVec2(0.0f,0.0f),dockspace_flags,window_class);
		ImGui::PopStyleVar(3);

		ImGui::Begin("Frame");


		START_TIMER(maskTimer,&profiler,"Generate mask",0x4f606f);
		GenerateMask(pixelsMask.data(),width,height,maskMode);
		END_TIMER(maskTimer,&profiler);
		START_TIMER(voronoiTimer,&profiler,"Nearest non zero",0x6fff20);
		CalculateNearestNoZero(nearestPixels,pixelsMask.data(),width,height);
		END_TIMER(voronoiTimer,&profiler);
		memset(pixelsRgb.data(),0,pixelsRgb.size());
		START_TIMER(rgbTimer,&profiler,"Convert to rgb",0xff2f70);
		ResetRandom();
		ConvertNearestToRGB(pixelsRgb.data(),nearestPixels,pixelsMask.data(),width,height,displayMode);
		END_TIMER(rgbTimer,&profiler);
		
		glBindTexture(GL_TEXTURE_2D,m_textureId);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,pixelsRgb.data());

		V2 mouse=VLoad(ImGui::GetMousePos());
		V2 imageSize=V2((float)width,(float)height);
		V2 imageTopLeft=VLoad(ImGui::GetCursorScreenPos());
		V2 imageMousePos=mouse-imageTopLeft;
		int px=(int)imageMousePos.x;
		int py=(int)imageMousePos.y;
		int16_t nx=0x7fff;
		int16_t ny=0x7fff;
		if(px>0 && py>0 && px<width && py<height) {
			nx=nearestPixels[py*width+px]>>16;
			ny=nearestPixels[py*width+px]&0xffff;
		}
		ImDrawList* dl=ImGui::GetWindowDrawList();
		ImGui::Dummy(ImLoad(imageSize));
		dl->AddImage((ImTextureID)(uint64_t)m_textureId,ImLoad(imageTopLeft),ImLoad(imageTopLeft+imageSize),ImVec2(0,0),ImVec2(1,1),0xffffffff);
		dl->PushClipRect(ImLoad(imageTopLeft),ImLoad(imageTopLeft+imageSize),true);
		if(nx!=0x7fff)
			dl->AddLine(ImLoad(imageTopLeft+V2((float)px,(float)py)),ImLoad(imageTopLeft+V2((float)(nx),(float)(ny))),0xffffffff);
		dl->PopClipRect();
		const char* modes[]={"Nearest non zero","Voronoi","Voronoi normalized","Mask"};
		ImGui::Combo("Display mode",(int*)&displayMode,modes,IM_ARRAYSIZE(modes));
		const char* maskModes[]={"Random","Random animate","Square"};
		ImGui::Combo("Mask mode",(int*)&maskMode,maskModes,IM_ARRAYSIZE(maskModes));
		if(nx!=0x7fff)
			ImGui::Text("mouse %.0f,%.0f nearest %d,%d",imageMousePos.x,imageMousePos.y,nx,ny);
		ImGui::End();
		ImGui::Begin("Profiler");
		DrawProfilerDisplay(profilerDisplay);
		ImGui::End();
		ImGui::Begin("Log");
		m_log.DrawLog();
		ImGui::End();

		ImGui::End();

		ImGui::Render();

		int windowWidth,windowHeight;
		glfwGetWindowSize(window,&windowWidth,&windowHeight);

		glViewport(0,0,windowWidth,windowHeight);
		glClearColor(.1,.1,.1,1);
		glClear(GL_COLOR_BUFFER_BIT);

		END_TIMER(mainTimer,&profiler);
		profilerDisplay.clear();
		profilerDisplayLock.lock();
		profiler.GetDisplayTimers(profilerDisplay);
		profilerDisplayLock.unlock();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if(io.ConfigFlags&ImGuiConfigFlags_ViewportsEnable){
			GLFWwindow* backup_current_context=glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		std::this_thread::sleep_until(time+std::chrono::microseconds(1000000/60));		//limit frame rate
		glfwSwapBuffers(window);

		tick++;
	}
	if(m_textureId)
		glDeleteTextures(1,&m_textureId);
	m_textureId=0;
	delete [] nearestPixels;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

Viewer* g_viewer=0;

void PrintCallback(const char* str) {
#ifdef _WIN32
	OutputDebugString(str);
#else
	::printf("%s",str);
#endif
	if(g_viewer)
		g_viewer->m_log.AddText(str);
}

int main(){
	SetPrintCallback(PrintCallback);
#ifdef CMAKE_SOURCE_DIR
	AddFilePathRemap("$(DATA)",std::string(CMAKE_SOURCE_DIR)+"/data");
#else
	AddFilePathRemap("$(DATA)",GetExecutablePath()+"/data");
#endif
	Viewer d;
	d.Begin();
	g_viewer=&d;
	std::thread tr=std::thread([&](){
	});
	d.Run();
	d.m_close=true;
	tr.join();
	d.End();
	g_viewer=0;
	return 0;
}

#ifdef _WIN32
#undef APIENTRY
#include<windows.h>
#include"debugapi.h"
#include<crtdbg.h>
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR lpCmdLine,INT nCmdShow){
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(167);
	return main();
}
#endif
