#ifndef CHARACTER
#define CHARACTER

#include "Hitbox2D.h"
#include "WickedLoader.h"


class Character:public Entity{
private:
#define BUFFERSIZE			24
#define INPUTTIMEMAX		14
#define CORRECTIONTIME		2
#define AFTERCOMBOSHOWTIME	200

	static const int INPUTWINDOW = 30;
	static const int HITFREEZETIME = 12;
	static const int CHARGETIME = 50;

	static float hitFreeze;
	static bool timestop;
	bool timestopOverride;
	void StopTime();
	void UnStopTime();
	bool IsTimeStop();
	
	static void ClearBuffer(DWORD buffer[BUFFERSIZE][5]);
	static bool InputCompare(const string&, const string&);

	void IncreaseTrails(Object* object, const XMFLOAT3& col);
	void FadeTrails();

	
	struct State;
	struct Requirement;
	struct Req_input;
	struct Req_frame;
	struct Req_height;
	typedef vector<Requirement*> Requirements;
	struct StateTransition;
	struct Combatant;

	
	template <typename T>
	struct Interval{
		T begin,end;
		Interval(){begin=end=(T)0;}
		Interval(T a,T b){
			begin=a;
			end=b;
		}
		Interval(T a){
			begin=a;
			end=a;
		}
		Interval(const string& a){
			if(!a.empty()){
				if(a[0]=='('){
					int comma=a.find(',');
					begin=(T)atof(a.substr(1,comma).c_str());
					string e=a.substr(comma+1,a.length()-comma-1);
					end=(T)atof(e.c_str());
				}
				else{
					begin=end=(T)atof(a.c_str());
				}
			}
		}
		bool contains(T value){
			return ((T)begin<=value && value<=(T)end);
		}
	};

	
	enum PressChange{
		PRESSCHANGE_NONE,
		PRESSCHANGE_DIRECTION,
		PRESSCHANGE_BUTTON,
		PRESSCHANGE_BOTH,
	};
	PressChange checkForPressChange();
	struct Input{
		long frame, held;
		short directions;
		bool x,y,z,w,taunt;

		Input(){
			frame=held=0;
			directions=0;
			x=y=z=w=taunt=false;
		}
		Input(long frame,short directions,bool buttons[5]):frame(frame),directions(directions)
			,x(buttons[0]),y(buttons[1]),z(buttons[2]),w(buttons[3]),taunt(buttons[4]),held(0){}
		Input(long frame, long held, short directions,bool buttons[5]):frame(frame),directions(directions)
			,x(buttons[0]),y(buttons[1]),z(buttons[2]),w(buttons[3]),taunt(buttons[4]),held(held){}
		bool isIdle();
		bool equals(const Input& b){
			return (directions==b.directions && x==b.x && y==b.y && z==b.z && w==b.w && taunt==b.taunt);
		}
		PressChange compare(const Input& b){
			if(this->equals(b))
				return PRESSCHANGE_NONE;
			if(directions==b.directions)
				return PRESSCHANGE_BUTTON;
			if(x==b.x && y==b.y && z==b.z && w==b.w && taunt==b.taunt)
				return PRESSCHANGE_DIRECTION;
			return PRESSCHANGE_BOTH;
		}
	};

	struct Data
	{
		float		hp_max, hp, hp_prev;
		int meter;
		Hitbox2D		clip;
		Data()		{hp_max=hp=hp_prev=0;meter=0;clip=Hitbox2D();}
		void Load(const string& dir, const string& chname){
			std::stringstream filename("");
			filename<<dir<<chname<<".data";
			std::ifstream file(filename.str().c_str());
			if(file) {
				std::string voidStr;
				file>>voidStr>>hp_max;
				file>>voidStr>>clip.pos.x>>clip.pos.y>>clip.siz.x>>clip.siz.y;
			}
			file.close();
			hp=hp_prev=hp_max;
		}
		void Reset(){hp=hp_prev=hp_max;meter=0;}
	};
	struct Sphere{
		float radius;
		XMFLOAT3 center;
		Sphere(){radius=0;center=XMFLOAT3(0,0,0);}
		Sphere(const XMFLOAT3& c, float r):center(c),radius(r){}
		bool intersects(const Sphere& b);
	};
	struct ASphere : Sphere{
		int damage;
		string prop;
		string height;
		int stun;
		int blockstun;
		int guardeater;
		int chip;
		int freeze;
		Transform* attach;

		ASphere(){center=XMFLOAT3(0,0,0);radius=0;damage=chip=0;prop="default";height="high";stun=0;blockstun=0;guardeater=0;attach=nullptr;}
		ASphere(const XMFLOAT3& c, float r):Sphere(c,r){damage=chip=0;prop="default";height="high";stun=0;blockstun=0;guardeater=0;attach=nullptr;}
	};

	struct ActiveFrame{
		string sphereP;
		HitSphere* sphere;
		Interval<int> interval;
		float radius;
		int active,repeat;
		int damage,chip,freeze;
		string prop;
		string height;
		int stun;
		int blockstun;
		int guardeater;
		int gain;
		bool attach;

		ActiveFrame(){};
		ActiveFrame(const string& newSphereP,const string& frame, float newRadius, int newDamage, int newChip, const string& newProp, int newStun
			,int newBlockStun, int newGuardEater, int newGain, int newFreeze, int newRepeat, const string newHeight, bool newAttach){
				sphereP = newSphereP;
				sphere=nullptr;
				//frame[0]=frame0;
				//frame[1]=frame1;
				interval=Interval<int>(frame);
				radius=newRadius;
				damage=newDamage;
				chip=newChip;
				prop=newProp;
				stun=newStun;
				blockstun=newBlockStun;
				guardeater=newGuardEater;
				gain=newGain;
				freeze=newFreeze;
				repeat=newRepeat;
				height=newHeight;
				attach=newAttach;
				Reset();
		}
		void Reset(){
			active=repeat;
		}
	};
	struct GuardFrame{
		string height;
		int gain;
		int freeze;
		Interval<int> interval;

		GuardFrame(){
			interval=Interval<int>(0,0);
			gain=0;
			freeze=0;
			height="";
		}
		GuardFrame(const string& newFrame, const string& newheight, int newGain, int newFreeze){
			interval=Interval<int>(newFrame);
			height=newheight;
			gain=newGain;
			freeze=newFreeze;
		}
	};
	struct ScriptItem{
		bool active;
		string name;
		vector<std::string> params;

		ScriptItem(){name="";params.resize(0);}
		ScriptItem(std::string newName){name=newName;params.resize(0);Reset();}
		void Reset(){active=true;}
		void Deactivate(){active=false;}
	};
	struct RecordedHit{
		bool active;
		XMFLOAT3 pos;
		RecordedHit(){active=false;pos=XMFLOAT3(0,0,0);}
	};
	struct PushPull{
		Interval<int> frames;
		float directionStrength;
		bool active;
		PushPull():frames(Interval<int>()),directionStrength(0),active(true){}
		PushPull(const string& frames, float stren):frames(Interval<int>(frames)),directionStrength(stren),active(true){}
		void Reset(){active=true;}
	};
	struct MoveListItem{
		std::string name;
		std::string armatureP;
		Armature* armature;
		std::string actionP;
		int actionI;
		Requirements requirements;
		string triggeredInput; //stores the input on which this was triggered
		RecordedHit recordedHit;
		vector<Interval<int> > invincible;
		vector<ActiveFrame> aframes;
		vector<GuardFrame> gframes;
		vector<ScriptItem> scripts;
		vector<PushPull> pushPull,propel;
		float timer;
		
		MoveListItem(){
			name="";
			triggeredInput="";
			armatureP="";
			actionP="";
			actionI=-1;
			aframes.resize(0);
			gframes.resize(0);
			scripts.resize(0);
			recordedHit=RecordedHit();
			invincible.resize(0);
			pushPull.resize(0);
			Reset();
		};
		MoveListItem(const string& newName,const string& newArmatureP,const string& newActionP){
			name=newName;
			armatureP=newArmatureP;
			actionP=newActionP;
			aframes.clear();
			gframes.clear();
			invincible.clear();
			pushPull.clear();
			scripts.clear();
			Reset();
		}
		
		void CleanUp(){
			aframes.clear();
			scripts.clear();
			pushPull.clear();
			invincible.clear();
			for(int i=0;i<requirements.size();++i)
				delete requirements[i];
			requirements.clear();
		}
		void Reset(){
			for(ActiveFrame& af : aframes){
				af.Reset();
			}
			for(ScriptItem& si : scripts){
				si.Reset();
			}
			for(PushPull& pu : pushPull){
				pu.Reset();
			}
			for(PushPull& pr : propel){
				pr.Reset();
			}
			timer=0;
		}
	};
	map<string,MoveListItem*> movelist;
	void LoadMoveList();
	MoveListItem* getMoveByName(const string&);

	static int Script_getParamCount(const char* script);
	void Script_Move(const MoveListItem& move, int scriptI, float currentFrame);
	void Script_MoveMul(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Dash(MoveListItem& move, int scriptI, float currentFrame);
	void Script_ToggleAction(MoveListItem& move, int scriptI, float currentFrame);
	void Script_OnHitAction(MoveListItem& move, int scriptI, float currentFrame);
	void Script_RibbonTrail(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Clip(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Block(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Image(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Sound(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Cam(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Burst(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Emit(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Opacity(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Detach(MoveListItem& move, int scriptI, float currentFrame);
	void Script_Decal(MoveListItem& move, int scriptI, float currentFrame);
	void Script_ExecuteAll(MoveListItem& move);
	void Script_Remove(MoveListItem& move, int scriptI);

	
	void ReadInput();
	int GetDirection(int);
	bool CheckForOnHit(bool inputAllowed, Combatant& opponent);
	void Block(Combatant& opponent);
	bool CheckForHitConfirm(Combatant& opponent);
	void Action();
	void VelocityStep();
	static void ChangeArmatureAction(Armature* armature, int actionI);
	bool ArmatureInAction(Armature* armature, int action);
	//void ShareData();

	bool nothingPressed();
	void StoreInBuffer();
	void AssembleInputString();
	static vector<Input> convertInputString(const string& val);

	short playerIndex;

	short corr;
	bool buttons[5];
	DWORD directions;
	DWORD buttonPress[2][5];

	float startPosition;
	float ground_height;

	void SetUpBaseActions();
	void ActivateFrames(Armature* armature);
	void TurnFacing(Armature* armature);

	struct Combatant{
		State* startState,*state;
		//Condition condition;
		XMFLOAT2 position;
		XMFLOAT2 velocity;
		float facing,prevFacing;
		bool hitConfirmed;
		float stun, blockstun;
		vector<ASphere> activeSpheres;
		vector<Sphere> hitSpheres;
		bool hitIn;
		bool canBlock, blocking;
		GuardFrame guard;
		Hitbox2D clip,block;
		float pushPull,propel;
		string hitProperty;

		Combatant(){
			//condition=GROUNDED;
			position=velocity=XMFLOAT2(0,0);
			facing=prevFacing=1;
			clip=block=Hitbox2D();
			startState=state=nullptr;
			hitConfirmed=hitIn=blocking=false;
			stun=blockstun=pushPull=0.0f;
			hitProperty="default";
		}
		void ActivateState(State* newState){
			//if(state!=newState /*|| (state && state->move && !state->move->requirements.empty())*/)
			{
				if(state && state->move)
					state->move->Reset();
				state = newState;
				ChangeAction(state->move,*this,"???");

				//canBlock=blocking=false;
			}
		}
	};
public:
	map<Armature*,Combatant> combatants;
	vector<Combatant*> getCombatants();
	Combatant& getMainCombatant();
	XMFLOAT4 overrideColor;

	deque<Input> inputs;
	Input currentInput;
	deque<Input> commandBuffer;

public:
	Character(const string& name, const string& newIdentifier, short pIndex, float pos, int costume=1);
	void CompleteLoading();
	void CleanUp();
	static void SetUpStatic();
	static void UpdateStatic();

	void* operator new(size_t);
	void operator delete(void*);

	void Reset();
	void ClearInput();
	void UpdateC(bool inputAllowed);
	void PreUpdateC();
	void PostUpdateC();
	DWORD inputBuffer[BUFFERSIZE][5];
	DWORD specialBuffer[BUFFERSIZE][5];
	string finalInput,directinput;
	float inputTimer;
	bool onInputChanged, noInput;

	Data cData;
	//SharedData shared;
	string Name;
	string portraitName;
	ID3D11ShaderResourceView *portrait;
	
	unsigned int combocount,combodamage;
	float comboShowTime;
	float comboContinue;

	static void Resolve(Character*, Character*, Level*);

	void DrawHitbox2D(ID3D11DeviceContext* context);

	static bool celShaded;
	void updateCelShading();

private:
	static void ChangeAction(MoveListItem* move, Combatant& combatant, const string& triggeredInput="");


#pragma region REQUIREMENTS
	struct Requirement{
		string type;
		vector<string> data;
		Character* c;

		Requirement(Character* c, const string& type):c(c),type(type){}

		virtual bool validate(){
			return true;
		}
		virtual void activate(){}
	};
	struct Req_input : Requirement{
		string input;
		bool overrider, special;

		Req_input(Character* c, const string& newData);
		bool validate();
	};
	struct Req_timeout : Requirement{
		int timeout;
		Combatant& combatant;

		Req_timeout(Character* c, const string& newData, Combatant& newCombatant);
		bool validate();
	};
	struct Req_frame : Requirement{
		Interval<int> interval;
		MoveListItem* move;
		
		Req_frame(Character* c, const string& newData, MoveListItem* newMove);
		bool validate();
	};
	struct Req_height : Requirement{
		Interval<float> interval;
		MoveListItem* move;
		
		Req_height(Character* c, const string& newData, MoveListItem* newMove);
		bool validate();
	};
	struct Req_hit : Requirement{
		Combatant& combatant;
		
		Req_hit(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_nostun : Requirement{
		Combatant& combatant;
		
		Req_nostun(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_onstun : Requirement{
		Combatant& combatant;
		
		Req_onstun(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_onhit : Requirement{
		Combatant& combatant;
		
		Req_onhit(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_noblockstun : Requirement{
		Combatant& combatant;
		
		Req_noblockstun(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_onblockstun : Requirement{
		Combatant& combatant;
		
		Req_onblockstun(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_health : Requirement{
		int health;

		Req_health(Character* c, const string& newData);
		bool validate();
		void activate();
	};
	struct Req_meter : Requirement{
		int meter;

		Req_meter(Character* c, const string& newData);
		bool validate();
		void activate();
	};
	struct Req_facing : Requirement{
		Combatant& combatant;
		
		Req_facing(Character* c, Combatant& newCombatant);
		bool validate();
		void activate();
	};
	struct Req_died : Requirement{
		Req_died(Character* c);
		bool validate();
	};
	struct Req_block : Requirement{
		Combatant& combatant;

		Req_block(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_falling : Requirement{
		Combatant& combatant;

		Req_falling(Character* c, Combatant& newCombatant);
		bool validate();
	};
	struct Req_hitproperty : Requirement{
		Combatant& combatant;
		string hitproperty;

		Req_hitproperty(Character* c, Combatant& newCombatant, const string& prop);
		bool validate();
	};
#pragma endregion
	struct StateTransition{
		State* state;
		Requirements requirements;

		StateTransition(State* newState, const Requirements& newRequs){
			state = newState;
			requirements.insert(requirements.end(),state->move->requirements.begin(),state->move->requirements.end());
			requirements.insert(requirements.end(),newRequs.begin(),newRequs.end());
		}
	};
	struct State{
		string name;
		MoveListItem* move;
		vector<StateTransition> transitions;

		State(const string& newName, MoveListItem* newMove){
			name=newName;
			move=newMove;
		}
	};
	map<string,State*> stateCollection;
	State* Character::getStateByName(const string& get);

	void CreateStateMachine();
	void StepStateMachine();
	bool ParseRequirement(MoveListItem* move, Requirements& requirements, const string& s, ifstream& file);
};

#endif
