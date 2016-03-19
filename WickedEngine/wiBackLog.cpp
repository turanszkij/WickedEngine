#include "wiBackLog.h"
#include "wiMath.h"
#include "wiResourceManager.h"
#include "wiImageEffects.h"
#include "wiRenderer.h"
#include "wiTextureHelper.h"

deque<string> wiBackLog::stream;
deque<string> wiBackLog::history;
mutex wiBackLog::logMutex;
wiBackLog::State wiBackLog::state;
const float wiBackLog::speed=50.0f;
unsigned int wiBackLog::deletefromline = 500;
float wiBackLog::pos;
int wiBackLog::scroll;
stringstream wiBackLog::inputArea;
int wiBackLog::historyPos=0;
Texture2D* wiBackLog::backgroundTex = nullptr;
wiFont wiBackLog::font;

void wiBackLog::Initialize(){
	pos = (float)wiRenderer::GetScreenHeight();
	scroll=0;
	state=DISABLED;
	deletefromline=500;
	inputArea=stringstream("");
	const unsigned char colorData[] = { 0, 0, 43, 200, 43, 31, 141, 223 };
	wiTextureHelper::CreateTexture(backgroundTex, colorData, 1, 2, 4);
	font = wiFont(L"", wiFontProps(5, 0, 0, WIFALIGN_LEFT, WIFALIGN_BOTTOM, -8));
}
void wiBackLog::CleanUp(){
	stream.clear();
}
void wiBackLog::Toggle(){
	switch(state){
	case IDLE:
		state=DEACTIVATING;
		break;
	case DISABLED:
		state=ACTIVATING;
		break;
	default:break;
	};
}
void wiBackLog::Scroll(int dir){
	scroll+=dir;
}
void wiBackLog::Update(){
	if(state==DEACTIVATING) 
		pos+=speed;
	else if(state==ACTIVATING) 
		pos-=speed;
	if (pos >= wiRenderer::GetScreenHeight()) 
	{ 
		state = DISABLED; 
		pos = (float)wiRenderer::GetScreenHeight(); 
	}
	else if(pos<0) 
	{
		state=IDLE; 
		pos=0;
	}
}
void wiBackLog::Draw(){
	if(state!=DISABLED){
		//wiImage::BatchBegin();
		wiImageEffects fx = wiImageEffects((float)wiRenderer::GetScreenWidth(), (float)wiRenderer::GetScreenHeight());
		fx.pos=XMFLOAT3(0,pos,0);
		fx.opacity = wiMath::Lerp(1, 0, pos / wiRenderer::GetScreenHeight());
		wiImage::Draw(backgroundTex, fx);
		font.SetText(getText());
		font.props.posY = pos - wiRenderer::GetScreenHeight() + 75 + scroll;
		font.Draw();
		wiFont(inputArea.str().c_str(), wiFontProps(5, -(float)wiRenderer::GetScreenHeight() + 10, 0, WIFALIGN_LEFT, WIFALIGN_BOTTOM, -8)).Draw();
		//wiFont::Draw(wiBackLog::getText(), "01", XMFLOAT4(5, pos - wiRenderer::RENDERHEIGHT + 75 + scroll, 0, -8), "left", "bottom");
		//wiFont::Draw(inputArea.str().c_str(), "01", XMFLOAT4(5, -(float)wiRenderer::RENDERHEIGHT + 10, 0, -8), "left", "bottom");
	}
}


string wiBackLog::getText(){
	logMutex.lock();
	stringstream ss("");
	for(unsigned int i=0;i<stream.size();++i)
		ss<<stream[i];
	logMutex.unlock();
	return ss.str();
}
void wiBackLog::clear(){
	logMutex.lock();
	stream.clear();
	logMutex.unlock();
}
void wiBackLog::post(const char* input){
	logMutex.lock();
	stringstream ss("");
	ss<<input<<endl;
	stream.push_back(ss.str().c_str());
	if(stream.size()>deletefromline){
		stream.pop_front();
	}
	logMutex.unlock();
}
void wiBackLog::input(const char& input){
	inputArea<<input;
}
void wiBackLog::acceptInput(){
	historyPos=0;
	stringstream commandStream("");
	commandStream<<inputArea.str();
	post(inputArea.str().c_str());
	history.push_back(inputArea.str());
	if(history.size()>deletefromline){
		history.pop_front();
	}
	wiLua::GetGlobal()->RunText(inputArea.str());
	inputArea.str("");
}
void wiBackLog::deletefromInput(){
	stringstream ss(inputArea.str().substr(0,inputArea.str().length()-1));
	inputArea.str("");
	inputArea<<ss.str();
}
void wiBackLog::save(ofstream& file){
	for(deque<string>::iterator iter=stream.begin(); iter!=stream.end(); ++iter)
		file<<iter->c_str();
	file.close();
}

void wiBackLog::historyPrev(){
	if(!history.empty()){
		inputArea.str("");
		inputArea<<history[history.size()-1-historyPos];
		if((size_t)historyPos<history.size()-1)
			historyPos++;
	}
}
void wiBackLog::historyNext(){
	if(!history.empty()){
		if(historyPos>0)
			historyPos--;
		inputArea.str("");
		inputArea<<history[history.size()-1-historyPos];
	}
}

void wiBackLog::setBackground(Texture2D* texture)
{
	backgroundTex = texture;
}


