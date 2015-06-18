#include "BackLog.h"
//#include "GameComponents.h"

//extern ID3D11Device*			Renderer::graphicsDevice;

deque<string> BackLog::stream;
deque<string> BackLog::history;
mutex BackLog::logMutex;
BackLog::State BackLog::state;
const float BackLog::speed=50.0f;
unsigned int BackLog::deletefromline = 100;
float BackLog::pos;
int BackLog::scroll;
int BackLog::RENDERWIDTH,BackLog::RENDERHEIGHT;
stringstream BackLog::inputArea;
int BackLog::historyPos=0;


void BackLog::Initialize(int width, int height){
	//stream.resize(0);
	ResourceManager::add("images/logBG.png");
	RENDERWIDTH=width;
	RENDERHEIGHT=height;
	pos=RENDERHEIGHT;
	scroll=0;
	state=DISABLED;
	deletefromline=100;
	inputArea=stringstream("");
	//post("BackLog Created");
}
void BackLog::CleanUp(){
	stream.clear();
}
void BackLog::Toggle(){
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
void BackLog::Scroll(int dir){
	scroll+=dir;
}
void BackLog::Update(){
	if(state==DEACTIVATING) pos+=speed;
	else if(state==ACTIVATING) pos-=speed;
	if(pos>=RENDERHEIGHT) {state=DISABLED; pos=RENDERHEIGHT;}
	else if(pos<=0) {state=IDLE; pos=0;}
}
void BackLog::Draw(){
	if(state!=DISABLED){
		Image::BatchBegin();
		ImageEffects fx = ImageEffects(RENDERWIDTH,RENDERHEIGHT);
		fx.pos=XMFLOAT3(0,pos,0);
		fx.opacity=WickedMath::Lerp(0,1,pos/RENDERHEIGHT);
		Image::Draw((Renderer::TextureView)(ResourceManager::get("images/logBG.png")->data),fx);
		Font::Draw(BackLog::getText(),"01",XMFLOAT4(5,pos-RENDERHEIGHT+75+scroll,0,-8),"left","bottom");
		Font::Draw(inputArea.str().c_str(),"01",XMFLOAT4(5,-RENDERHEIGHT+10,0,-8),"left","bottom");
	}
}


string BackLog::getText(){
	logMutex.lock();
	stringstream ss("");
	for(unsigned int i=0;i<stream.size();++i)
		ss<<stream[i];
	logMutex.unlock();
	return ss.str();
}
void BackLog::clear(){
	logMutex.lock();
	stream.clear();
	logMutex.unlock();
}
void BackLog::post(const char* input){
	logMutex.lock();
	stringstream ss("");
	ss<<input<<endl;
	stream.push_back(ss.str().c_str());
	if(stream.size()>deletefromline){
		stream.pop_front();
	}
	logMutex.unlock();
}
void BackLog::input(const char& input){
	inputArea<<input;
}
void BackLog::acceptInput(){
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
void BackLog::deletefromInput(){
	stringstream ss(inputArea.str().substr(0,inputArea.str().length()-1));
	inputArea.str("");
	inputArea<<ss.str();
}
void BackLog::save(ofstream& file){
	for(deque<string>::iterator iter=stream.begin(); iter!=stream.end(); ++iter)
		file<<iter->c_str();
	file.close();
}

void BackLog::historyPrev(){
	if(!history.empty()){
		inputArea.str("");
		inputArea<<history[history.size()-1-historyPos];
		if(historyPos<history.size()-1)
			historyPos++;
	}
}
void BackLog::historyNext(){
	if(!history.empty()){
		if(historyPos>0)
			historyPos--;
		inputArea.str("");
		inputArea<<history[history.size()-1-historyPos];
	}
}