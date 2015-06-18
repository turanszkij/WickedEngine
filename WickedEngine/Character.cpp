#include "Renderer.h"
//#include "GameComponents.h"



extern FLOAT shBias;
extern FLOAT BlackOut, BlackWhite, InvertCol;


float Character::hitFreeze;
bool Character::timestop=false;
bool Character::celShaded=true;

void Character::updateCelShading(){
	for(Object* object : e_objects){
		for(Material* mat : object->mesh->materials){
			mat->toonshading=celShaded;
		}
	}
}

Character::Character(const string& name, const string& newIdentifier, short pIndex, float pos, int costume)
{
	identifier=newIdentifier;
	Renderer::Renderer();

	playerIndex=pIndex;
	
	std::stringstream ss("");
	ss<<"characters/"<<name<<"/"<<costume<<"/";

	LoadFromDisk(ss.str(),name,identifier,armatures,materials,objects,objects_trans,objects_water
		,meshes,lights,spheres,WorldInfo(),Wind(),cameras
		,e_armatures,e_objects,transforms,decals);

	for(Object* object : e_objects){
		object->mesh->stencilRef=STENCILREF::STENCILREF_CHARACTER;
	}

	startPosition=pos;
	

	std::stringstream directory("");
	directory<<"characters/"<<name<<"/";
	cData = Data();
	cData.Load(directory.str().c_str(), name);
	Name=name;

	portrait=NULL;
	std::stringstream portraitFname("");
	portraitFname<<directory.str()<<"portrait.png";
	portraitName=portraitFname.str();
	portrait=(ID3D11ShaderResourceView*)ResourceManager::add(portraitName);

	for(Armature* armature : e_armatures){
		combatants.insert(pair<Armature*,Combatant>(armature,Combatant()));
	}

	LoadMoveList();
	CreateStateMachine();
	
	for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter)
		iter->second.ActivateState(iter->second.startState);
	
	corr=0;
	onInputChanged=false;
	noInput=true;
	ClearBuffer(specialBuffer);
	ClearBuffer(inputBuffer);
	finalInput="";
	for(short i=0;i<5;i++){
		buttonPress[0][i]=0;
		buttonPress[1][i]=0;
		if(i<4) buttons[i]=0;
	}
	directions=0;
	inputs.clear();
	commandBuffer.clear();
	currentInput=Input();
	inputTimer=0;
	combocount=combodamage=0;
	comboContinue=false;
	comboShowTime=AFTERCOMBOSHOWTIME;
	
	ground_height=e_armatures[0]->translation.y;
	
	//combatants.resize(armatures.size()); for(int i=0;i<combatants.size();++i) combatants[i]=Combatant();
	//shared.activeSpheres.resize(combatants.size());
	//shared.move.resize(combatants.size());
	Reset();


	//SetUpBaseActions();
	//if(playerIndex) 
	//	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter)
	//		TurnFacing(iter->first);

	timestopOverride=false;

	//ConstructMoveTree();

	ResourceManager::add("sound/blockeffect0.wav",ResourceManager::SOUND);

	overrideColor = XMFLOAT4(0,0,0,0);
}
void Character::CompleteLoading(){
	
}


void* Character::operator new(size_t size)
{
	void* result = _aligned_malloc(size,16);
	return result;
}
void Character::operator delete(void* p)
{
	if(p) _aligned_free(p);
}

void Character::CleanUp()
{
	Entity::CleanUp();
	
	for(map<string,MoveListItem*>::iterator it=movelist.begin();it!=movelist.end();++it){
		it->second->CleanUp();
		delete it->second;
	}
	for(map<string,State*>::iterator it=stateCollection.begin();it!=stateCollection.end();++it){
		delete it->second;
	}
	movelist.clear();
	combatants.clear();
	stateCollection.clear();

	ResourceManager::del(portraitName);
	portrait=nullptr;
	inputs.clear();
	commandBuffer.clear();
}

void Character::LoadMoveList()
{
	MoveListItem* currentMove=nullptr;
	stringstream fname("");
	fname<<"characters/"<<Name<<"/movelist.mo";
	std::ifstream file(fname.str());

	if(file.is_open()){

		while(!file.eof()){
			//Requirements requirements(0);
			string moveName = "", armName = "", actName = "";
			file>>moveName>>armName>>actName;
			stringstream identified_armatureP("");
			identified_armatureP<<armName<<identifier;
			MoveListItem* currentMove = new MoveListItem(moveName,identified_armatureP.str(),actName);
			movelist.insert(pair<string,MoveListItem*>(moveName,currentMove));


			currentMove->armature=getArmatureByName(currentMove->armatureP);
			currentMove->actionI=getActionByName(currentMove->armature, currentMove->actionP);

			if(actName.back()=='}'){
				actName = actName.substr(0,actName.length()-3);
			}
			else{
				if(actName.back()=='{'){
					actName = actName.substr(0,actName.length()-2);
				}

				string read = "";
				file>>read;
				while(read.compare("}")){
					if(!read.compare("{")){

					}
					else if(ParseRequirement(currentMove,currentMove->requirements,read,file)){
						//Could read requirement, move on
					}
					else if(read.find("active")!=string::npos){
						string sphereP;
						string frame;
						float radius=1.0;
						int damage=0,chip=0;
						string prop = "default", height="high";
						int stun = 0;
						int blockstun = 0;
						int guardeater = 0;
						int gain = 0;
						int freeze = HITFREEZETIME;
						int repeat = 1;
						bool attach=false;
						file>>sphereP>>frame;


						string::size_type p = frame.find('[');
						if(p!=string::npos){
							string rep = frame.substr(p+1,frame.length()-p-1);
							repeat=atoi(rep.c_str());
							frame = frame.substr(0,p);
						}

						string r="";
						file>>r;
						while(r.back()!='}'){
							if(r.find("radius")!=string::npos){
								file>>radius;
							}
							else if(r.find("damage")!=string::npos){
								file>>damage;
							}
							else if(r.find("chip")!=string::npos){
								file>>chip;
							}
							else if(r.find("prop")!=string::npos){
								file>>prop;
							}
							else if(r.find("height")!=string::npos){
								file>>height;
							}
							else if(r.find("blockstun")!=string::npos){
								file>>blockstun;
							}
							else if(r.find("stun")!=string::npos){
								file>>stun;
							}
							else if(r.find("guard")!=string::npos){
								file>>guardeater;
							}
							else if(r.find("gain")!=string::npos){
								file>>gain;
							}
							else if(r.find("freeze")!=string::npos){
								file>>freeze;
							}
							else if(r.find("attach")!=string::npos){
								attach=true;
							}

							file>>r;
						}

						stringstream identified_sphere("");
						identified_sphere<<sphereP<<identifier;

						//file>>radius>>damage>>prop>>stun>>blockstun>>guardeater>>gain;
						currentMove->aframes.push_back( ActiveFrame(identified_sphere.str(),frame,radius,damage,chip,prop,stun,blockstun,guardeater,gain,freeze,repeat,height,attach) );
						currentMove->aframes.back().sphere=getSphereByName(identified_sphere.str());
					}
					else if(read.find("guard")!=string::npos){
						string frame;
						stringstream height("");
						int gain = 0;
						int freeze = HITFREEZETIME;
						file>>frame;


						string r="";
						file>>r;
						while(r.back()!='}'){
							if(r.find("hei")!=string::npos){
								string h;
								file>>h;
								height<<h;
							}
							else if(r.find("gai")!=string::npos){
								file>>gain;
							}
							else if(r.find("fre")!=string::npos){
								file>>freeze;
							}
							file>>r;
						}
						//file>>radius>>damage>>prop>>stun>>blockstun>>guardeater>>gain;
						currentMove->gframes.push_back(GuardFrame(frame,height.str(),gain,freeze));
					}
					else if(read.find("inv")!=string::npos){
						string frames="";
						file>>frames;
						currentMove->invincible.push_back(Interval<int>(frames));
					}
					else if(read.find("push")!=string::npos || read.find("pull")!=string::npos){
						string frames="";
						float stren=0;
						file>>frames>>stren;
						currentMove->pushPull.push_back(PushPull(frames,stren));
					}
					else if(read.find("propel")!=string::npos){
						string frames="";
						float stren=0;
						file>>frames>>stren;
						currentMove->propel.push_back(PushPull(frames,stren));
					}
					else{
						string script=read;
						currentMove->scripts.push_back(ScriptItem(script));
						int paramCount = Script_getParamCount(script.c_str());
						if(paramCount>=0) //params read
							for(int i=0;i<paramCount;++i){
								std::string s="";
								file>>s;
								currentMove->scripts.back().params.push_back(s);
							}
						else{ //read block
							string read="";
							file>>read;
							stack<string> blocks;
							blocks.push(read);
							while(!blocks.empty()){
								file>>read;
								if(!strcmp(read.c_str(),"{")) {blocks.push(read); currentMove->scripts.back().params.push_back(read);}
								else if(!strcmp(read.c_str(),"}")) {blocks.pop(); if(!blocks.empty())currentMove->scripts.back().params.push_back(read);}
								else if(!strcmp(read.c_str(),"=")) continue;
								else {
									currentMove->scripts.back().params.push_back(read);
								}
							}
						}
					}


					read.clear();
					file>>read;
				}
			}
		}

		file.close();
	}


	//if(file){
	//	while(!file.eof()){
	//		
	//		std::string voidStr="";
	//		file>>voidStr;

	//		switch(voidStr[0]){
	//			case 'm':
	//				{
	//					std::string name="";
	//					std::string armatureP="";
	//					std::string actionP="";
	//					std::string inputMotion="";
	//					int repeatableS=-1,repeatableE=-1; //-1 for none
	//					std::string conditionIn="";
	//					std::string typeIn="";
	//					file>>name>>armatureP>>actionP>>conditionIn>>typeIn>>repeatableS;
	//					if(repeatableS>=0)
	//						file>>repeatableE;
	//					file>>inputMotion;
	//					if(!inputMotion.compare("0"))
	//						inputMotion="";

	//					stringstream identified_armatureP("");
	//					identified_armatureP<<armatureP<<identifier;
	//					movelist.insert( pair<string,MoveListItem>(
	//						name,MoveListItem(name,identified_armatureP.str(),actionP,conditionIn,typeIn,repeatableS,repeatableE,inputMotion))
	//						);
	//					currentMove=&movelist[name];
	//				}
	//				break;
	//			case 'a':
	//				{
	//					std::string sphereP;
	//					int frame0, frame1;
	//					float radius;
	//					int damage;
	//					std::string prop;
	//					int stun;
	//					int blockstun;
	//					int guardeater;
	//					int gain;
	//					file>>sphereP>>frame0>>frame1>>radius>>damage>>prop>>stun>>blockstun>>guardeater>>gain;
	//					currentMove->aframes.push_back( ActiveFrame(sphereP,frame0,frame1,radius,damage,prop,stun,blockstun,guardeater,gain) );
	//				}
	//				break;
	//			case 'i':
	//				currentMove->invincible.push_back(Frames());
	//				file>>currentMove->invincible.back().frame[0]>>currentMove->invincible.back().frame[1];
	//				break;
	//			case 'c':
	//				{
	//					if(voidStr.length()>1) 
	//						switch(voidStr[1]){
	//						case 'm':
	//							{
	//								std::string move;
	//								int frame0,frame1;
	//								file>>move>>frame0>>frame1;
	//								currentMove->cancelFrames.push_back( CancelFrame(move,frame0,frame1) );
	//							}
	//							break;
	//						case 'p':
	//							{
	//								std::string type;
	//								int frame0,frame1;
	//								file>>type>>frame0>>frame1;
	//								currentMove->cancelFrames.push_back( CancelFrame(examineType(type),frame0,frame1) );
	//							}
	//							break;
	//						default: break;
	//					}
	//					
	//					if(voidStr.length()>2) 
	//						switch(voidStr[2]){
	//						case 'o':
	//							currentMove->cancelFrames.back().onHitOnly=true;
	//							break;
	//						default:break;
	//					}
	//				}
	//				break;
	//			case 'p':
	//				{
	//					currentMove->pushPull.push_back(PushPull());
	//					float frameS = 0;
	//					file>>frameS;
	//					currentMove->pushPull.back().frames.frame[0]=frameS;
	//					if(frameS>=0)
	//						file>>currentMove->pushPull.back().frames.frame[1];
	//					file>>currentMove->pushPull.back().directionStrength;
	//				}
	//			case 's':
	//				{
	//					std::string script="";
	//					file>>script;
	//					currentMove->scripts.push_back(ScriptItem(script));
	//					int paramCount = Script_getParamCount(script.c_str());
	//					if(paramCount>=0) //params read
	//						for(int i=0;i<paramCount;++i){
	//							std::string s="";
	//							file>>s;
	//							currentMove->scripts.back().params.push_back(s);
	//						}
	//					else{ //read block
	//						string read="";
	//						file>>read;
	//						stack<string> blocks;
	//						blocks.push(read);
	//						while(!blocks.empty()){
	//							file>>read;
	//							if(!strcmp(read.c_str(),"{")) {blocks.push(read); currentMove->scripts.back().params.push_back(read);}
	//							else if(!strcmp(read.c_str(),"}")) {blocks.pop(); if(!blocks.empty())currentMove->scripts.back().params.push_back(read);}
	//							else if(!strcmp(read.c_str(),"=")) continue;
	//							else {
	//								currentMove->scripts.back().params.push_back(read);
	//							}
	//						}
	//					}
	//				}
	//				break;
	//			default: break;
	//		}
	//	}
	//}
	//file.close();
	
	////FIXATE THE INDEXERS
	//for(map<string,MoveListItem*>::iterator it=movelist.begin();it!=movelist.end();++it){
	//	MoveListItem& move = *it->second;
	//	//move.armature=getArmatureByName(move.armatureP);
	//	//it->second.armature = armatures[it->second.armatureI];
	//	//move.actionI=getActionByName(move.armature, move.actionP);
	//	for(int j=0;j<move.aframes.size();j++){
	//		move.aframes[j].sphereI=getSphereByName(move.aframes[j].sphereP);
	//	}
	//	//int frameCount = move.armature->actions[move.actionI].frameCount;
	//}
}
Character::MoveListItem* Character::getMoveByName(const string& get){
	map<string,MoveListItem*>::iterator it = movelist.find(get);
	if(it==movelist.end())
		return nullptr;
	return it->second;
}
Character::State* Character::getStateByName(const string& get){
	map<string,State*>::iterator it = stateCollection.find(get);
	if(it==stateCollection.end())
		return nullptr;
	return it->second;
}

void Character::CreateStateMachine(){

	for(map<string,MoveListItem*>::iterator iter=movelist.begin();iter!=movelist.end();++iter){
		stateCollection.insert(pair<string,State*>(iter->first,new State(iter->first,iter->second)));
	}

	stringstream fname("");
	fname<<"characters/"<<Name<<"/statemachine.sm";
	ifstream file(fname.str());
	if(file.is_open()){

		vector<pair<string,string> > rootState(0);
		State* state = nullptr;
		while(!file.eof()){
			string read = "";
			file>>read;

			if(!read.compare("start:")){ //start symbol
				string s="",a="";
				file>>s>>a;
				rootState.push_back(pair<string,string>(s,a));
			}
			else if(read[0]=='-'){ //transition
				string transName = "";
				if(read.back()=='{') //just getting the transition name without {} and -
					transName = read.substr(1,read.length()-2);
				else if(read.back()=='}')
					transName = read.substr(1,read.length()-3);
				else 
					transName = read.substr(1,read.length()-1);


				Requirements requirements(0);
				State* addState = getStateByName(transName);

				if(read.back()!='}'){
					string s="";
					file>>s;
					while(s.compare("}")){
						ParseRequirement(state!=nullptr?state->move:nullptr,requirements,s,file);
						s.clear();
						file>>s;
					}
				}

				if(addState!=nullptr && state!=nullptr)
					state->transitions.push_back(StateTransition( addState,requirements ));
				else{
#ifdef BACKLOG
					stringstream ss("");
					ss<<"[CreateStateMachine-Transition] Warning: "<<transName<<" state has no corresponding MoveListItem!("<<Name<<")";
					BackLog::post(ss.str().c_str());
#endif
				}

			}
			else{ //The state itself
				state = getStateByName(read);
#ifdef BACKLOG
				if(state==nullptr){
					stringstream ss("");
					ss<<"[CreateStateMachine-Main] Warning: "<<read<<" state has no corresponding MoveListItem!("<<Name<<")";
					BackLog::post(ss.str().c_str());
				}
#endif
			}

		}
		
		for(pair<string,string>& rs : rootState){
			stringstream identified_aname("");
			identified_aname<<rs.second<<identifier;
			combatants[getArmatureByName(identified_aname.str())].startState = getStateByName(rs.first);
		}

		file.close();
	}
	else{
#ifdef BACKLOG
		stringstream ss("");
		ss<<"Error: "<<fname.str()<<" State Machine descriptor could not be loaded!";
		BackLog::post(ss.str().c_str());
#endif
	}
}
void Character::StepStateMachine(){
	for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter){
		Combatant& combatant = iter->second;
		

		for(StateTransition& trans : combatant.state->transitions){
			/* SIMPLE VERSION (NO ADDITIVE REQUIREMENTS)
			bool valid = false;
			for(Requirement* req : trans.requirements){
				valid = req->validate(this);
				if(!valid)
					break;
			}

			if(valid){
				combatant.ActivateState(trans.state);
				break;
			}
			*/

			map<string,vector<bool> > valid;
			for(Requirement* req : trans.requirements){
				valid[req->type].push_back( req->validate() );
			}

			bool invalid = false;
			for(map<string,vector<bool> >::iterator iter= valid.begin();iter!=valid.end();++iter){
				bool vali = false;
				for(bool v : iter->second){
					if(v==true){
						vali=true;
						break;
					}
				}
				if(vali==false){
					invalid=true;
					break;
				}
			}

			if(!invalid){
				for(Requirement* req : trans.requirements){
					req->activate();
				}
				combatant.ActivateState(trans.state);
				break;
			}
		}
	}
}

bool Character::ParseRequirement(MoveListItem* move, Requirements& requirements, const string& s, ifstream& file)
{
	if(!s.compare("input")){
		string args = "";
		file>>args;
		requirements.push_back(new Req_input(this,args));
		return true;
	}
	else if(!s.compare("frame")){
		string args = "";
		file>>args;
		if(move!=nullptr)
			requirements.push_back(new Req_frame(this,args,move));
		return true;
	}
	else if(!s.compare("height")){
		string args = "";
		file>>args;
		if(move!=nullptr)
			requirements.push_back(new Req_height(this,args,move));
		return true;
	}
	else if(!s.compare("hit")){
		if(move!=nullptr)
			requirements.push_back(new Req_hit(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("onstun")){
		if(move!=nullptr)
			requirements.push_back(new Req_onstun(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("onhit")){
		if(move!=nullptr)
			requirements.push_back(new Req_onhit(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("nostun")){
		if(move!=nullptr)
			requirements.push_back(new Req_nostun(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("onblockstun")){
		if(move!=nullptr)
			requirements.push_back(new Req_onblockstun(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("noblockstun")){
		if(move!=nullptr)
			requirements.push_back(new Req_noblockstun(this,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("timeout")){
		string args = "";
		file>>args;
		if(move!=nullptr)
			requirements.push_back(new Req_timeout(this,args,combatants[move->armature]));
		return true;
	}
	else if(!s.compare("died")){
		requirements.push_back(new Req_died(this));
		return true;
	}
	else if(!s.compare("health")){
		string args = "";
		file>>args;
		requirements.push_back(new Req_health(this,args));
	}
	else if(!s.compare("meter")){
		string args = "";
		file>>args;
		requirements.push_back(new Req_meter(this,args));
	}
	else if(!s.compare("facing")){
		requirements.push_back(new Req_facing(this,combatants[getArmatureByName(move->armatureP)]));
	}
	else if(!s.compare("canblock")){
		requirements.push_back(new Req_block(this,combatants[getArmatureByName(move->armatureP)]));
	}
	else if(!s.compare("falling")){
		requirements.push_back(new Req_falling(this,combatants[getArmatureByName(move->armatureP)]));
	}
	else if(!s.compare("hitproperty")){
		string args = "";
		file>>args;
		requirements.push_back(new Req_hitproperty(this,combatants[getArmatureByName(move->armatureP)],args));
	}

	return false;
}


void Character::Reset()
{
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		iter->second.ActivateState(iter->second.startState);
		iter->second.position.x=startPosition;
		iter->second.position.y=0;
		iter->second.stun=0;
		iter->second.hitConfirmed=false;
		iter->second.velocity=XMFLOAT2(0,0);
		iter->second.hitIn=false;

		iter->first->translation_rest.x=iter->second.position.x;
		iter->first->translation_rest.y=iter->second.position.y;
		iter->first->translation_rest.z=0;
	}
	cData.Reset();
	combocount=0;
	comboContinue=0;
	combodamage=0;
	hitFreeze=0;
}
void Character::UpdateC(bool inputAllowed)
{
	//for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter)
	//	iter->second.hitConfirmed=false;

	updateCelShading();

	if(cData.hp>0){
		if(inputAllowed)
			ReadInput();
		else{
			for(short i=0;i<4;i++) buttons[i]=0;
			ClearBuffer(specialBuffer);
		}
	}
	else{
		directions=0;
		for(short i=0;i<4;i++) buttons[i]=0;
	}
	
	//SearchForInputAction();
	Action();
	StepStateMachine();

	if(!hitFreeze && !IsTimeStop()){
		if(inputTimer>=0) inputTimer+=GetGameSpeed();

		if(!comboContinue){
			comboShowTime-=GetGameSpeed();
			if(comboShowTime<=0) {comboShowTime=0; combocount=0;combodamage=0;}
			if(cData.hp<cData.hp_prev)			cData.hp_prev-=cData.hp_max*0.008f;
			else if(cData.hp>cData.hp_prev)		cData.hp_prev=cData.hp;
		}
		if(comboContinue>0) comboContinue-=GetGameSpeed();
		overrideColor = WickedMath::Lerp(XMFLOAT4(0,0,0,0),overrideColor,0.7f);
		if(overrideColor.w<0.1f)
			overrideColor.w=0;
		
		VelocityStep();

		for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter){
			iter->second.stun-=GetGameSpeed();
			iter->second.blockstun-=GetGameSpeed();
			iter->second.state->move->timer+=GetGameSpeed();
		}
		
		
		//Block(opponentShared);
		//CheckForOnHit(inputAllowed, opponentShared);
		//CheckForHitConfirm(opponentShared);
	


		
		

		//Update();
		//UpdateSpheres();

		//FadeTrails();

	}
	//ShareData();
	
}
void Character::PostUpdateC()
{

	if(!hitFreeze && !IsTimeStop()){
		
		UpdateSpheres();

		FadeTrails();
		
		for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter)
			ActivateFrames(iter->first);

	}
	
}
void Character::PreUpdateC()
{
	if(!hitFreeze && !IsTimeStop()){

		for(int i=0;i<spheres.size();++i)
			spheres[i]->Reset();
	
	}
}
void Character::ReadInput()
{
	//corr++;
	//if(corr>CORRECTIONTIME) {
	//	corr=0;
	//	for(short i=0;i<5;i++) buttons[i]=0;
	//}

	////if(!shared.stunTime){
	//	directions = GameComponents::pContext[playerIndex].input.directions;
	//	if(GameComponents::pContext[playerIndex].input.buttons[0]) buttons[0]    = GameComponents::pContext[playerIndex].input.buttons[0];
	//	if(GameComponents::pContext[playerIndex].input.buttons[1]) buttons[1]    = GameComponents::pContext[playerIndex].input.buttons[1];
	//	if(GameComponents::pContext[playerIndex].input.buttons[2]) buttons[2]    = GameComponents::pContext[playerIndex].input.buttons[2];
	//	if(GameComponents::pContext[playerIndex].input.buttons[3]) buttons[3]    = GameComponents::pContext[playerIndex].input.buttons[3];
	////}


	Input prevInput = currentInput;
	//currentInput = Input(prevInput.frame+1,GameComponents::pContext[playerIndex].input.directions,GameComponents::pContext[playerIndex].input.buttons);
	PressChange ch = currentInput.compare(prevInput);
	if(ch == PRESSCHANGE_NONE){
		int h = prevInput.held+1;
		currentInput.held=h;
		if(!commandBuffer.empty())
			commandBuffer.back().held=h;
	}

	if(!currentInput.isIdle()){
		if(currentInput.compare(prevInput)!=PRESSCHANGE_NONE )
			inputs.push_back(currentInput);
	}
	if(ch!=PRESSCHANGE_NONE){
		commandBuffer.push_back(currentInput);
		if(ch==PRESSCHANGE_BUTTON)
			commandBuffer.back().directions=D_IDLE;
		else if(ch==PRESSCHANGE_DIRECTION)
			commandBuffer.back().x=commandBuffer.back().y=commandBuffer.back().z=commandBuffer.back().w=false;
	}
	while(!commandBuffer.empty() && (currentInput.frame-commandBuffer.front().frame-commandBuffer.front().held>INPUTWINDOW)){
		commandBuffer.pop_front();
	}

	
	stringstream ss("");
	for(Input& i : commandBuffer){
		//if(i.directions!=D_IDLE)
			ss<<(i.held>=CHARGETIME?"h":"")<<GetDirection(i.directions);
		if(i.x) ss<<'x';
		if(i.y) ss<<'y';
		if(i.z) ss<<'z';
		if(i.w) ss<<'w';
		if(i.taunt) ss<<'t';
	}
	finalInput=ss.str();

	stringstream ssd("");
	//if(currentInput.directions!=D_IDLE)
		ssd<<(currentInput.held>=CHARGETIME?"h":"")<<GetDirection(currentInput.directions);
	if(currentInput.compare(prevInput)!=PRESSCHANGE_NONE)
	{
		if(currentInput.x) ssd<<'x';
		if(currentInput.y) ssd<<'y';
		if(currentInput.z) ssd<<'z';
		if(currentInput.w) ssd<<'w';
		if(currentInput.taunt) ssd<<'t';
	}
	directinput=ssd.str();


	StoreInBuffer();
}
void Character::ActivateFrames(Armature* armature){
	int activeAction = armature->activeAction;
	float cf = armature->currentFrame;
	Combatant& combatant = combatants[armature];
	MoveListItem& move = *combatant.state->move;

	combatant.activeSpheres.clear();
	combatant.hitSpheres.clear();

	for(int i=0;i<move.invincible.size();++i)
		if(move.invincible[i].contains(cf))
			for(int j=0;j<spheres.size();++j)
				if(spheres[j]->TYPE!=HitSphere::HitSphereType::ATKTYPE) spheres[j]->TYPE=HitSphere::HitSphereType::INVTYPE;

	for(int i=0;i<move.aframes.size();++i){
		if(move.aframes[i].active>0 && cf-(int)cf<=GetGameSpeed()){
			HitSphere* hs = move.aframes[i].sphere;

			if(hs!=nullptr && move.aframes[i].interval.contains(cf)){
				hs->TYPE=HitSphere::HitSphereType::ATKTYPE;
				float rad = hs->radius*move.aframes[i].radius;
				hs->radius = rad;

				combatant.activeSpheres.push_back(ASphere(hs->translation,rad));
				combatant.activeSpheres.back().damage=move.aframes[i].damage;
				combatant.activeSpheres.back().chip=move.aframes[i].chip;
				combatant.activeSpheres.back().freeze=move.aframes[i].freeze;
				combatant.activeSpheres.back().guardeater=move.aframes[i].guardeater;
				combatant.activeSpheres.back().stun=move.aframes[i].stun;
				combatant.activeSpheres.back().blockstun=move.aframes[i].blockstun;
				combatant.activeSpheres.back().prop=move.aframes[i].prop;
				combatant.activeSpheres.back().height=move.aframes[i].height;
				if(move.aframes[i].attach)
					combatant.activeSpheres.back().attach=hs;
				else
					combatant.activeSpheres.back().attach=nullptr;
			}
		}
	}

	combatant.blocking=false;
	for(GuardFrame& g : move.gframes){
		if(g.interval.contains(cf)){
			combatant.blocking=true;
			combatant.guard=g;
			break;
		}
	}


	for(HitSphere* s : spheres){
		//if(s->parentArmature==armature){
			if(s->TYPE!=HitSphere::HitSphereType::ATKTYPE && s->TYPE!=HitSphere::HitSphereType::INVTYPE){
				combatant.hitSpheres.push_back(Sphere(s->translation,s->radius));
			}
		//}
	}

}
bool Character::CheckForOnHit(bool inputAllowed, Combatant& opponent){
	for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter){
		Combatant& combatant = iter->second;
		Armature* armature = iter->first;
		MoveListItem& move = *combatant.state->move;
		combatant.hitIn=false;

		for(Sphere& x : combatant.hitSpheres)
			for(ASphere& y : opponent.activeSpheres)
				//if(x.TYPE!=ATKTYPE && x.TYPE!=INVTYPE)
				if(x.intersects(y)) {
					combatant.hitIn=true;

					string attackHeight = y.height;


					if(combatant.blocking && combatant.guard.height.find(attackHeight) != string::npos ){ //BLOCK
						//spawn image
						images.push_back( new oImage("images/block1.png","","") );
						images.back()->effects.blendFlag=BLENDMODE_ADDITIVE;
						images.back()->effects.typeFlag=WORLD;
						images.back()->effects.pivotFlag=Pivot::CENTER;
						images.back()->effects.sampleFlag=SAMPLEMODE_CLAMP;
						images.back()->effects.siz=XMFLOAT2(4,4);
						images.back()->effects.pos.x=combatant.clip.pos.x+combatant.clip.siz.x*0.5f*combatant.prevFacing;
						images.back()->effects.pos.y=combatant.clip.pos.y+combatant.clip.siz.y*0.5f;
						images.back()->anim.fad=0.05f;

						
						images.push_back( new oImage("images/block2.png","","") );
						images.back()->effects.blendFlag=BLENDMODE_ADDITIVE;
						images.back()->effects.typeFlag=WORLD;
						images.back()->effects.pivotFlag=Pivot::CENTER;
						images.back()->effects.sampleFlag=SAMPLEMODE_CLAMP;
						images.back()->effects.siz=XMFLOAT2(4,4);
						images.back()->effects.pos.x=combatant.clip.pos.x+combatant.clip.siz.x*0.5f*combatant.prevFacing;
						images.back()->effects.pos.y=combatant.clip.pos.y+combatant.clip.siz.y*0.5f;
						images.back()->anim.fad=0.05f;
						images.back()->anim.scaleX=-0.08f;
						images.back()->anim.scaleY=-0.08f;

						
						images.push_back( new oImage("images/block3.png","","") );
						images.back()->effects.blendFlag=BLENDMODE_ADDITIVE;
						images.back()->effects.typeFlag=WORLD;
						images.back()->effects.pivotFlag=Pivot::CENTER;
						images.back()->effects.sampleFlag=SAMPLEMODE_CLAMP;
						images.back()->effects.siz=XMFLOAT2(3,3);
						images.back()->effects.pos.x=combatant.clip.pos.x+combatant.clip.siz.x*0.5f*combatant.prevFacing;
						images.back()->effects.pos.y=combatant.clip.pos.y+combatant.clip.siz.y*0.5f;
						images.back()->anim.fad=0.05f;
						images.back()->anim.scaleX=0.05f;
						images.back()->anim.scaleY=0.05f;

						((SoundEffect*)ResourceManager::get("sound/blockeffect0.wav")->data)->Play();

	#ifdef BACKLOG
						stringstream ss("");
						ss<<playerIndex+1<<" blocks player "<<1-playerIndex+1;
						ss<<"\n   Blockstun:"<<y.blockstun;
						ss<<"\n   Guardeater:"<<y.guardeater;
						ss<<"\n   Chip Damage:"<<y.chip;
						BackLog::post(ss.str().c_str());
	#endif
						combatant.blockstun=y.blockstun;
						if(inputAllowed)
							cData.hp-=y.chip;


						hitFreeze=max( y.freeze, combatant.guard.freeze );

						overrideColor = XMFLOAT4(-0.5f,-0.5f,1,1);
					}
					else{ //HIT

						opponent.state->move->recordedHit.active=true;
						opponent.state->move->recordedHit.pos=x.center;
						if(inputAllowed)
							cData.hp-=y.damage;
						comboShowTime=AFTERCOMBOSHOWTIME;

						if(!comboContinue) {combocount=0;combodamage=0;}
						comboContinue=y.stun;
						combocount++;
						combodamage+=y.damage;

				
						hitFreeze=y.freeze;


						combatant.stun=y.stun;
						combatant.hitProperty = y.prop;

				
						//ChangeAction(getMoveByName("StaggerLight"),getMainCombatant());

						if(y.attach!=nullptr){
							armature->attachTo(y.attach,1,0,0);
						}

		#ifdef BACKLOG
						stringstream ss("");
						ss<<"Hit Confirm: player"<<1-playerIndex+1<<" hits player"<<playerIndex+1;
						ss<<"\n   Damage:"<<y.damage;
						ss<<"\n   Stun:"<<y.stun;
						BackLog::post(ss.str().c_str());
		#endif

						overrideColor = XMFLOAT4(1,-0.5f,-0.5f,1);
					}

					return true;
				}

	}
	return false;
}
void Character::Block(Combatant& opponent){
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		Combatant& combatant = iter->second;
		Armature* armature = iter->first;
		MoveListItem& move = *combatant.state->move;

		if(combatant.clip.intersects(opponent.block)){
			//int direction = GetDirection(directions);
			//if( direction==D_LEFT || direction==D_DOWNLEFT ){
			//	MoveListItem* block = getMoveByName(direction==D_LEFT?"StandingBlock":"CrouchingBlock");
			//	if(block){
			//		//TODO
			//	}
			//}
			combatant.canBlock=true;
		}
		else{
			combatant.canBlock=false;
		}
	}
}
bool Character::CheckForHitConfirm(Combatant& opponent){
	
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		Combatant& combatant = iter->second;
		Armature* armature = iter->first;
		MoveListItem& move = *combatant.state->move;

		if(opponent.hitIn){
			combatant.hitConfirmed=true;
			for(int j=0;j<move.aframes.size();++j){
				float cf = armature->currentFrame;
				if(move.aframes[j].interval.contains(cf)) {
					move.aframes[j].active-=1;
					
					if(opponent.blocking){
						cData.meter+=move.aframes[j].gain/2;
					}
					else{
						cData.meter+=move.aframes[j].gain;
					}

				}
			}
		}
		
		combatant.pushPull=0;
		for(int j=0;j<move.pushPull.size();++j){
			if(move.pushPull[j].active)
			if((move.pushPull[j].frames.begin<0 && combatant.hitConfirmed) || move.pushPull[j].frames.contains(armature->currentFrame)){
				combatant.pushPull+=move.pushPull[j].directionStrength;
				if(move.pushPull[j].frames.begin<0)
					move.pushPull[j].active=false;
			}
		}
		combatant.propel=0;
		for(int j=0;j<move.propel.size();++j){
			if(move.propel[j].active)
			if((move.propel[j].frames.begin<0 && combatant.hitConfirmed) || move.propel[j].frames.contains(armature->currentFrame)){
				combatant.propel+=move.propel[j].directionStrength;
				if(move.propel[j].frames.begin<0)
					move.propel[j].active=false;
			}
		}
	}


	return opponent.hitIn;
}
void Character::Action()
{
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		Combatant& combatant = iter->second;
		Armature* armature = iter->first;

		combatant.clip=cData.clip;
		combatant.clip.pos.x+=combatant.position.x;
		combatant.clip.pos.y+=combatant.position.y;

		combatant.block=Hitbox2D();

		Script_ExecuteAll(*combatant.state->move);
		combatant.state->move->recordedHit.active=false;

		//if(armature->translation.y>ground_height) {
		//	combatant.condition=AIRBORNE;
		//}
		//else combatant.condition=GROUNDED;

		//if((combatant.facing<0 && opponentShared.pos.x>combatant.position.x
		//	|| combatant.facing>0 && opponentShared.pos.x<combatant.position.x)
		//	&& armature->activeAction == combatant.idle) //needs chrouching too!
		//	TurnFacing(armature);

	}
}
void Character::VelocityStep()
{
	static const float GRAVITY = 0.027f;
	static const float DECELERATION_GROUND = 0.9f;
	static const float DECELERATION_AIR = 0.98f;
	
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		Combatant& combatant = iter->second;
		Armature* armature = iter->first;

		XMFLOAT2 newPos = XMFLOAT2( combatant.position.x+combatant.velocity.x,combatant.position.y+combatant.velocity.y );
		XMFLOAT2 newVel = XMFLOAT2(0,0);
		//if(
		//	newX>-XBOUNDS-8
		//	&& newX<XBOUNDS+8
		//  )
		//combatant.position.x=newX;
		//combatant.position.y=newY;
		if(newPos.y>ground_height) newVel.y = combatant.velocity.y-GRAVITY;
		if(newPos.y<ground_height) {newPos.y=ground_height; newVel.y=0;}
		if(newPos.y<=0) newVel.x = combatant.velocity.x*DECELERATION_GROUND;
		if(newPos.y>0) newVel.x = combatant.velocity.x*DECELERATION_AIR;
		if(abs(newVel.x)<0.001f) newVel.x=0.f;
		
		//if(combatant.position.x<-XBOUNDS-6) combatant.position.x=-XBOUNDS-6;
		//if(combatant.position.x>XBOUNDS+6) combatant.position.x=XBOUNDS+6;

		combatant.position = WickedMath::Lerp(combatant.position,newPos,GetGameSpeed());
		combatant.velocity = WickedMath::Lerp(combatant.velocity,newVel,GetGameSpeed());

		
		armature->translation_rest.x=combatant.position.x;
		armature->translation_rest.y=combatant.position.y;
		armature->translation_rest.z=0;
	}
}
void Character::ChangeAction(MoveListItem* move, Combatant& combatant, const string& triggeredInput){
	if(move){

		Armature* armature=move->armature;
		int actionI=move->actionI;

		ChangeArmatureAction(armature,actionI);

		combatant.hitConfirmed=false;

#ifdef BACKLOG
		stringstream ss("");
		ss<<armature->name<<" start new Action: "<<move->name;
		BackLog::post(ss.str().c_str());
#endif
	}
}
void Character::ChangeArmatureAction(Armature* armature, int actionI){
	if(armature!=nullptr
		&& actionI>=0 && actionI<armature->actions.size() ){

		armature->prevAction=armature->activeAction;
		armature->prevActionResolveFrame=armature->currentFrame;
		armature->activeAction=actionI;
		armature->currentFrame=1; //-5 for SMOOTH ACTION TRANSITION!!
	}
}
bool Character::ArmatureInAction(Armature* armature, int action){
	int activeAction = armature->activeAction;
	if(activeAction==action) return true;
	return false;
}
//void Character::ShareData(){
//	shared.pos=getMainCombatant().position;
//	shared.vel=getMainCombatant().velocity;
//	shared.facing=getMainCombatant().facing;
//	shared.clip=getMainCombatant().clip;
//	shared.stunTime-=IsTimeStop()?0:GetGameSpeed();
//	if(shared.stunTime<0) shared.stunTime=0;
//	shared.pushPull=0;
//
//	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
//		Combatant& combatant = iter->second;
//		Armature* armature = iter->first;
//		MoveListItem& move = *combatant.state->move;
//
//		if(!move.armature)
//			break;
//
//		float cf=move.armature->currentFrame;
//		shared.move[armature]=&move;
//		shared.activeSpheres[armature].clear();
//		for(int j=0;j<move.aframes.size();++j){
//			if(move.aframes[j].active){
//				if(move.aframes[j].interval.contains(cf)){
//					int s=move.aframes[j].sphereI;
//					if(spheres[s].TYPE==ATKTYPE){
//						shared.activeSpheres[armature].push_back(ASphere(XMFLOAT4(
//							spheres[s].desc.position.x,spheres[s].desc.position.y,spheres[s].desc.position.z,spheres[s].desc.radius
//							)));
//						shared.activeSpheres[armature].back().damage=move.aframes[j].damage;
//						shared.activeSpheres[armature].back().guardeater=move.aframes[j].guardeater;
//						shared.activeSpheres[armature].back().stun=move.aframes[j].stun;
//						shared.activeSpheres[armature].back().blockstun=move.aframes[j].blockstun;
//						shared.activeSpheres[armature].back().prop=move.aframes[j].prop;
//					}
//				}
//			}
//		}
//
//
//		for(int j=0;j<move.pushPull.size();++j){
//			if((move.pushPull[j].frames.begin<0 && combatant.hitConfirmed) || move.pushPull[j].frames.contains(armature->currentFrame)){
//				shared.pushPull+=move.pushPull[j].directionStrength;
//			}
//		}
//	}
//
//
//}
Character::Combatant& Character::getMainCombatant()
{
	return combatants[e_armatures[0]];
}

void Character::StopTime(){
	timestop=true;
	timestopOverride=true;
}
void Character::UnStopTime(){
	timestop=false;
	timestopOverride=false;
}
bool Character::IsTimeStop(){
	if(timestop){
		if(timestopOverride)
			return false;
		return true;
	}
	return false;
}

void Character::TurnFacing(Armature* armature){
	//if(!combatants[armature].changeFacing){
	//	combatants[armature].changeFacing=true;
	//	ChangeAction(getMoveByName("Turn"),combatants[armature]);
	//}
	//else if(!ArmatureInAction(armature,combatants[armature].turn)){
	//	combatants[armature].changeFacing=false;
	//	combatants[armature].facing *= -1;

		XMVECTOR rotO,rotM,rotF;
		rotO=XMLoadFloat4(&armature->rotation_rest);
		rotM=XMVectorSet(0,1,0,0);
		rotF=XMQuaternionNormalize( XMQuaternionMultiply(rotO,rotM) );
		XMStoreFloat4( &armature->rotation_rest,rotF );
	//}
}

vector<Character::Combatant*> Character::getCombatants()
{
	vector<Combatant*> c(0);
	for(map<Armature*,Combatant>::iterator iter=combatants.begin();iter!=combatants.end();++iter)
		c.push_back(&iter->second);
	return c;
}
void Character::Resolve(Character* p1, Character* p2, Level* level)
{
	vector<Combatant*> p1c=p1->getCombatants(),p2c=p2->getCombatants();
	
#pragma region BLOCKING
	for(Combatant* c : p2c)
		p1->Block(*c);
	for(Combatant* c : p1c)
		p2->Block(*c);
#pragma endregion
	
	p1->UpdateC(true);
	p2->UpdateC(true);

#pragma region HITTING
	if(!hitFreeze){
		for(Combatant* c : p2c)
			p1->CheckForOnHit(true,*c);
		for(Combatant* c : p1c)
			p2->CheckForOnHit(true,*c);
		for(Combatant* c : p2c)
			p1->CheckForHitConfirm(*c);
		for(Combatant* c : p1c)
			p2->CheckForHitConfirm(*c);
	}
#pragma endregion
	
#pragma region CLIPPING
#define REPULSIVENESS 0.35f

	for(Combatant* c1 : p1c){
		for(Combatant* c2 : p2c){

			if(c1->clip.intersects(c2->clip))
			{
				bool p1onleft = (c1->position.x <= c2->position.x);
				float curDis = abs(c1->clip.pos.x - c2->clip.pos.x);
				float minDis = c1->clip.siz.x*0.5f + c2->clip.siz.x*0.5f;
				float overlap = abs(curDis - minDis);
				float ppp=overlap*GetGameSpeed() * (p1onleft?1:-1);

				p1->getMainCombatant().position.x-=ppp*REPULSIVENESS;
				p2->getMainCombatant().position.x+=ppp*REPULSIVENESS;
			}

			
			float p1r = c1->clip.pos.x+c1->clip.siz.x*0.5f;
			float p2r = c2->clip.pos.x+c2->clip.siz.x*0.5f;
			float p1l = c1->clip.pos.x-c1->clip.siz.x*0.5f;
			float p2l = c2->clip.pos.x-c2->clip.siz.x*0.5f;
			float lr = level->playArea.pos.x+level->playArea.siz.x*0.5f;
			float ll = level->playArea.pos.x-level->playArea.siz.x*0.5f;
			float p1diff=0,p2diff=0;

		#pragma region PUSH/PULL AND PROPEL
	
			//if(!hitFreeze)
			{
				float dif = p1->getMainCombatant().position.x-p2->getMainCombatant().position.x;
				float dir = dif/abs(dif);
				float p1push = c2->pushPull*dir;
				float p2push = c1->pushPull*-dir;
				c1->velocity.x = (p1push?p1push:c1->velocity.x);
				c2->velocity.x = (p2push?p2push:c2->velocity.x);
				if(!c1->blocking)
					c1->velocity.y = (c2->propel?c2->propel:c1->velocity.y);
				if(!c2->blocking)
					c2->velocity.y = (c1->propel?c1->propel:c2->velocity.y);
			}

		#pragma endregion

			if(p1r>lr)
				p1diff=lr-p1r;
			if(p1l<ll)
				p1diff=ll-p1l;
			if(p2r>lr)
				p2diff=lr-p2r;
			if(p2l<ll)
				p2diff=ll-p2l;

			if(p1diff)
				c1->position.x+=p1diff;
			if(p2diff)
				c2->position.x+=p2diff;

		}
	}

	
	
#pragma endregion

#pragma region FACING
	if(p1->getMainCombatant().position.x>p2->getMainCombatant().position.x){
		for(Combatant* c1 : p1c){
			c1->facing=-1;
		}
		for(Combatant* c2 : p2c){
			c2->facing=1;
		}
	}
	else{
		for(Combatant* c1 : p1c){
			c1->facing=1;
		}
		for(Combatant* c2 : p2c){
			c2->facing=-1;
		}
	}
#pragma endregion

	
	p1->cData.hp=WickedMath::Clamp(p1->cData.hp,0,p1->cData.hp_max);
	p1->cData.meter=WickedMath::Clamp(p1->cData.meter,0,100);
	p2->cData.hp=WickedMath::Clamp(p2->cData.hp,0,p2->cData.hp_max);
	p2->cData.meter=WickedMath::Clamp(p2->cData.meter,0,100);

}

void Character::ClearBuffer(DWORD buffer[BUFFERSIZE][5])
{
	for(DWORD i=0;i<BUFFERSIZE;i++){
		for(short j=0;j<5;j++) buffer[i][j]=0;
	}
}
//NEEDS CORRECTION!!!
bool Character::InputCompare(const string& toFind, const string& findIn)
{
	/*stringstream aa,bb;
	for(int i=0;i<a.length();++i) if(a[i]=='x' || a[i]=='y' || a[i]=='z' || a[i]=='w') aa<<a[i];
	for(int i=0;i<b.length();++i) if(b[i]=='x' || b[i]=='y' || b[i]=='z' || b[i]=='w') bb<<b[i];
	if(strcmp(aa.str().c_str(),bb.str().c_str())) return -1;

	int ret=0;
	ret=abs((int)(a.length()-b.length()));
	for(int i=0;i< (a.length()<b.length()?a.length():b.length());++i)
		if(a[i]!=b[i]) ++ret;
	return ret;*/
	//return findIn.find(toFind)!=string::npos;

	int i=0;
	int found=0;

	while(i<findIn.length()){
		if(toFind[found]==findIn[i])
			found++;
		if(found>=toFind.length())
			return true;

		i++;
	}
	return false;
}  
void Character::StoreInBuffer()
{
	for(short i=0;i<5;i++) buttonPress[0][i]=buttonPress[1][i];
	buttonPress[1][0]=directions;
	if(corr==CORRECTIONTIME){
		buttonPress[1][1]=buttons[0];
		buttonPress[1][2]=buttons[1];
		buttonPress[1][3]=buttons[2];
		buttonPress[1][4]=buttons[3];
	}

	//storing in input buffer
	PressChange pressCH = checkForPressChange();
	if(pressCH!=PRESSCHANGE_NONE)
	{
		inputTimer=0;
		DWORD NEWBUTTONPRESS[5];
		NEWBUTTONPRESS[0]=NEWBUTTONPRESS[1]=NEWBUTTONPRESS[2]=NEWBUTTONPRESS[3]=NEWBUTTONPRESS[4]=0;
		if(pressCH==PRESSCHANGE_BOTH) memcpy(NEWBUTTONPRESS,buttonPress[1],sizeof(buttonPress[1]));
		else if(pressCH==PRESSCHANGE_DIRECTION) NEWBUTTONPRESS[0]=buttonPress[1][0];
		else if(pressCH==PRESSCHANGE_BUTTON){
			NEWBUTTONPRESS[1]=buttonPress[1][1];
			NEWBUTTONPRESS[2]=buttonPress[1][2];
			NEWBUTTONPRESS[3]=buttonPress[1][3];
			NEWBUTTONPRESS[4]=buttonPress[1][4];
		}

		for(DWORD i=1;i<BUFFERSIZE;i++)
		{
			for(short j=0;j<5;j++) inputBuffer[i-1][j]=inputBuffer[i][j];
		}
		memcpy(inputBuffer[BUFFERSIZE-1],NEWBUTTONPRESS,sizeof(NEWBUTTONPRESS));
		
		//if(inputTimer==0)	inputTimer++;

		onInputChanged=TRUE;
		
		
		int gotfreespaceinspecialbuffer=-1;
		for(int i=0;i<BUFFERSIZE;i++)
			if(!specialBuffer[i][0]) {
				gotfreespaceinspecialbuffer=i;
				break;
			}
		if(gotfreespaceinspecialbuffer>=0)
			memcpy(specialBuffer[gotfreespaceinspecialbuffer],NEWBUTTONPRESS,sizeof(NEWBUTTONPRESS));
		
	}
	else if((inputTimer>INPUTTIMEMAX && nothingPressed()) || specialBuffer[BUFFERSIZE-1][0]){
		noInput=TRUE;
		inputTimer=0;
		ClearBuffer(specialBuffer);
	}

	
	AssembleInputString();

	if(!hitFreeze)
		if(buttonPress[1][1] || buttonPress[1][2] || buttonPress[1][3] || buttonPress[1][4]){
			noInput=TRUE;
			inputTimer=0;
			ClearBuffer(specialBuffer);
		}
}
void Character::AssembleInputString()
{
	//std::stringstream ss("");
	//for(int i=0;i<BUFFERSIZE;++i){
	//	if(specialBuffer[i][0] || specialBuffer[i][1] || specialBuffer[i][2] || specialBuffer[i][3] || specialBuffer[i][4]){
	//		if(specialBuffer[i][0]) {
	//			ss<<GetDirection(specialBuffer[i][0]);
	//		}
	//		if(specialBuffer[i][1]) ss<<'x';
	//		if(specialBuffer[i][2]) ss<<'y';
	//		if(specialBuffer[i][3]) ss<<'z';
	//		if(specialBuffer[i][4]) ss<<'w';
	//	}
	//	else break;
	//}
	//if(!ss.str().empty())
	//	finalInput=ss.str();
	//else
	//	finalInput="";

}
vector<Character::Input> Character::convertInputString(const string& val){
	vector<Input> v(0);
	for(char x: val){
		switch(x){
		case 'x':
			if(v.empty())
				v.push_back(Input());
			v.back().x=true;
			break;
		case 'y':
			if(v.empty())
				v.push_back(Input());
			v.back().y=true;
			break;
		case 'z':
			if(v.empty())
				v.push_back(Input());
			v.back().z=true;
			break;
		case 'w':
			if(v.empty())
				v.push_back(Input());
			v.back().w=true;
			break;
		default:
			v.push_back(Input());
			v.back().directions=atoi(&x);
			break;
		}
	}

	return v;
}
void Character::ClearInput()
{
	ClearBuffer(specialBuffer);
	finalInput="";

	commandBuffer.clear();
	currentInput=Input();
}
int Character::GetDirection(int inDir){
	if(getMainCombatant().facing<0){
		if(inDir==D_LEFTUP) return D_UPRIGHT;
		if(inDir==D_LEFT) return D_RIGHT;
		if(inDir==D_DOWNLEFT) return D_RIGHTDOWN;
		if(inDir==D_UPRIGHT) return D_LEFTUP;
		if(inDir==D_RIGHT) return D_LEFT;
		if(inDir==D_RIGHTDOWN) return D_DOWNLEFT;
	}

	return inDir;
}
Character::PressChange Character::checkForPressChange()
{
	bool directionCH = false;
	bool buttonCH = false;
	if(buttonPress[1][0]!=D_IDLE && buttonPress[1][0]!=buttonPress[0][0]) directionCH=true;
	for(int i=1;i<5;++i)
		if(buttonPress[1][i] && buttonPress[0][i]!=buttonPress[1][i]){buttonCH=true;break;}
	
		if(buttonCH && directionCH) return PRESSCHANGE_BOTH;
		else if(buttonCH) return PRESSCHANGE_BUTTON;
		else if(directionCH) return PRESSCHANGE_DIRECTION;

	return PRESSCHANGE_NONE;
}
bool Character::nothingPressed()
{
	short count=0;
	if(buttonPress[0][0]==D_IDLE)
		count++;
	for(short i=1;i<5;i++)
		if(!buttonPress[0][i]) count++;
	return count==5? true:false;
}


void Character::IncreaseTrails(Object* object, const XMFLOAT3& col){
	if(GetGameSpeed() && object!=nullptr && object->mesh!=nullptr)
	{
		int base = object->mesh->trailInfo.base;
		int tip = object->mesh->trailInfo.tip;


		int x = object->trail.size();

		if(base>=0 && tip>=0 && x<MAX_RIBBONTRAILS-2){
			XMFLOAT2 newBTex = XMFLOAT2(x*GetGameSpeed(),0);
			XMFLOAT2 newTTex = XMFLOAT2(x*GetGameSpeed(),1);
			XMFLOAT4 baseP,tipP;
			XMFLOAT4 newCol = XMFLOAT4(col.x,col.y,col.z,1);
			baseP=TransformVertex(object->mesh,base).pos;
			tipP=TransformVertex(object->mesh,tip).pos;
			XMStoreFloat4( &baseP, XMVector3Transform( XMLoadFloat4(&baseP),XMLoadFloat4x4(&object->world) ) );
			XMStoreFloat4( &tipP, XMVector3Transform( XMLoadFloat4(&tipP),XMLoadFloat4x4(&object->world) ) );
				
			object->trail.push_back( RibbonVertex( XMFLOAT3(baseP.x,baseP.y,baseP.z),newBTex,XMFLOAT4(0,0,0,1) ) );
			object->trail.push_back( RibbonVertex( XMFLOAT3(tipP.x,tipP.y,tipP.z),newTTex,newCol ) );
		}
	}
}
void Character::FadeTrails(){
#define FADEOUT 0.06f
	for(int i=0;i<e_objects.size();++i){
		for(int j=0;j<e_objects[i]->trail.size();++j){
			if(e_objects[i]->trail[j].col.w<=0 /*&& e_objects[i]->trail[j].inc==-1*/) e_objects[i]->trail.pop_front();
			else {
				const float fade = /*e_objects[i]->trail[j].inc**/FADEOUT;
				if(e_objects[i]->trail[j].col.x>0) e_objects[i]->trail[j].col.x=e_objects[i]->trail[j].col.x-fade*(GetGameSpeed()+GetGameSpeed()*(1-j%2)*2);
				else e_objects[i]->trail[j].col.x=0;
				if(e_objects[i]->trail[j].col.y>0) e_objects[i]->trail[j].col.y=e_objects[i]->trail[j].col.y-fade*(GetGameSpeed()+GetGameSpeed()*(1-j%2)*2);
				else e_objects[i]->trail[j].col.y=0;
				if(e_objects[i]->trail[j].col.z>0) e_objects[i]->trail[j].col.z=e_objects[i]->trail[j].col.z-fade*(GetGameSpeed()+GetGameSpeed()*(1-j%2)*2);
				else e_objects[i]->trail[j].col.z=0;
				if(e_objects[i]->trail[j].col.w>0)
					e_objects[i]->trail[j].col.w-=fade*GetGameSpeed();
				else
					e_objects[i]->trail[j].col.w=0;
				/*if(e_objects[i]->trail[j].col.w>=1) 
					e_objects[i]->trail[j].inc=-1;*/
			}
		}
	}
}

void Character::SetUpStatic(){
	ResourceManager::add("images/clipbox.png");
	ResourceManager::add("images/blockbox.png");
	hitFreeze=0;
	timestop=false;
}
void Character::UpdateStatic(){
	if(hitFreeze>0) hitFreeze-=GameSpeed;
	else hitFreeze=0;

	if(hitFreeze!=0) overrideGameSpeed=0;
	else
		overrideGameSpeed=1;
}

void Character::DrawHitbox2D(ID3D11DeviceContext* context)
{
	Image::BatchBegin(context);
	for(map<Armature*,Combatant>::iterator iter=combatants.begin(); iter!=combatants.end(); ++iter){
		Combatant& combatant = iter->second;
		{
			ImageEffects fx(combatant.clip.pos.x,combatant.clip.pos.y,combatant.clip.siz.x,combatant.clip.siz.y);
			fx.blendFlag=BLENDMODE_ALPHA;
			fx.typeFlag=WORLD;
			fx.pivotFlag=CENTER;
			fx.lookAt=XMFLOAT4(0,0,1,1);
			fx.opacity=0.3f;
			Image::Draw((ID3D11ShaderResourceView*)ResourceManager::get("images/clipbox.png")->data,fx,context);
		}
		{
			ImageEffects fx(combatant.block.pos.x,combatant.block.pos.y,combatant.block.siz.x,combatant.block.siz.y);
			fx.blendFlag=BLENDMODE_ALPHA;
			fx.typeFlag=WORLD;
			fx.pivotFlag=CENTER;
			fx.lookAt=XMFLOAT4(0,0,1,1);
			fx.opacity=0.3f;
			Image::Draw((ID3D11ShaderResourceView*)ResourceManager::get("images/blockbox.png")->data,fx,context);
		}
	}

	
}



bool Character::Sphere::intersects(const Sphere& b){
	return WickedMath::Distance(center,b.center)<=radius+b.radius;
}


bool Character::Input::isIdle(){return (directions==D_IDLE && !x && !y && !z && !w);}