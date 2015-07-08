#include "BackLog.h"
#include "WickedMath.h"
#include "ResourceManager.h"
#include "ImageEffects.h"
#include "Renderer.h"

deque<string> wiBackLog::stream;
deque<string> wiBackLog::history;
mutex wiBackLog::logMutex;
wiBackLog::State wiBackLog::state;
const float wiBackLog::speed=50.0f;
unsigned int wiBackLog::deletefromline = 100;
float wiBackLog::pos;
int wiBackLog::scroll;
stringstream wiBackLog::inputArea;
int wiBackLog::historyPos=0;


void wiBackLog::Initialize(){
	//stream.resize(0);
	wiResourceManager::add("images/logBG.png");
	pos = wiRenderer::RENDERHEIGHT;
	scroll=0;
	state=DISABLED;
	deletefromline=100;
	inputArea=stringstream("");
	//post("wiBackLog Created");
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
	if(state==DEACTIVATING) pos+=speed;
	else if(state==ACTIVATING) pos-=speed;
	if (pos >= wiRenderer::RENDERHEIGHT) { state = DISABLED; pos = wiRenderer::RENDERHEIGHT; }
	else if(pos<=0) {state=IDLE; pos=0;}
}
void wiBackLog::Draw(){
	if(state!=DISABLED){
		wiImage::BatchBegin();
		wiImageEffects fx = wiImageEffects(wiRenderer::RENDERWIDTH, wiRenderer::RENDERHEIGHT);
		fx.pos=XMFLOAT3(0,pos,0);
		fx.opacity = wiMath::Lerp(0, 1, pos / wiRenderer::RENDERHEIGHT);
		wiImage::Draw((wiRenderer::TextureView)(wiResourceManager::get("images/logBG.png")->data),fx);
		wiFont::Draw(wiBackLog::getText(), "01", XMFLOAT4(5, pos - wiRenderer::RENDERHEIGHT + 75 + scroll, 0, -8), "left", "bottom");
		wiFont::Draw(inputArea.str().c_str(), "01", XMFLOAT4(5, -wiRenderer::RENDERHEIGHT + 10, 0, -8), "left", "bottom");
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
	inputArea.str("");

#ifdef GAMECOMPONENTS
	vector<string> command(0);
	while(!commandStream.eof()){
		string a = "";
		commandStream>>a;
		command.push_back(a);
	}
	command.pop_back();
	GameComponents::consoleCommands.clear();
	GameComponents::consoleCommands=vector<string>(command.begin(),command.end());
	GameComponents::wakeConsoleCommand=true;
#endif
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
		if(historyPos<history.size()-1)
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