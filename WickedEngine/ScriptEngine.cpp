#include "Renderer.h"



int Character::Script_getParamCount(const char* script){
	if(!strcmp("move",script)) return 6;
	else if(!strcmp("movemul",script)) return 3;
	else if(!strcmp("dash",script)) return 3;
	else if(!strcmp("toggleAction",script)) return 2;
	else if(!strcmp("onHitAction",script)) return 1;
	else if(!strcmp("ribbonTrail",script)) return 6;
	else if(!strcmp("clip",script)) return 6;
	else if(!strcmp("block",script)) return 6;
	else if(!strcmp("image",script)) return -1; //-1 : read block
	else if(!strcmp("sound",script)) return 2;
	else if(!strcmp("cam",script)) return 5;
	else if(!strcmp("burst",script)) return 3;
	else if(!strcmp("emit",script)) return 3;
	else if(!strcmp("opacity",script)) return 3;
	else if(!strcmp("detach",script)) return 2;
	else if(!strcmp("decal",script)) return 8;
	return 0;
}
void Character::Script_Remove(MoveListItem& move, int scriptI){
	//move.scripts.erase(move.scripts.begin()+scriptI);
	move.scripts[scriptI].Deactivate();
}

void Character::Script_ExecuteAll(MoveListItem& move){
	Armature* armature = move.armature;
	if(armature!=nullptr){
		float cf = armature->currentFrame;
		for(int i=0;i<move.scripts.size();++i){
			if(move.scripts[i].active){
				string script = move.scripts[i].name;
				vector<std::string> params = move.scripts[i].params;
		
				if(		!script.compare("move")) Script_Move(move,i,cf);
				else if(!script.compare("movemul")) Script_MoveMul(move,i,cf);
				else if(!script.compare("dash")) Script_Dash(move,i,cf);
				else if(!script.compare("toggleAction")) Script_ToggleAction(move,i,cf);
				else if(!script.compare("toggleAction")) Script_OnHitAction(move,i,cf);
				else if(!script.compare("ribbonTrail")) Script_RibbonTrail(move,i,cf);
				else if(!script.compare("clip")) Script_Clip(move,i,cf);
				else if(!script.compare("block")) Script_Block(move,i,cf);
				else if(!script.compare("image")) Script_Image(move,i,cf);
				else if(!script.compare("sound")) Script_Sound(move,i,cf);
				else if(!script.compare("cam")) Script_Cam(move,i,cf);
				else if(!script.compare("burst")) Script_Burst(move,i,cf);
				else if(!script.compare("emit")) Script_Emit(move,i,cf);
				else if(!script.compare("opacity")) Script_Opacity(move,i,cf);
				else if(!script.compare("detach")) Script_Detach(move,i,cf);
				else if(!script.compare("decal")) Script_Decal(move,i,cf);
			}
		}
	}
	//move.scripts.clear();
}

void Character::Script_Move(const MoveListItem& move, int scriptI, float currentFrame){
	if(currentFrame>=atoi(move.scripts[scriptI].params[4].c_str())
		&& currentFrame<=atoi(move.scripts[scriptI].params[5].c_str())){
			XMFLOAT2 vector = XMFLOAT2(atof(move.scripts[scriptI].params[0].c_str()),atof(move.scripts[scriptI].params[1].c_str()));
			float mod = (combatants[move.armature].prevFacing<0)?(-1.0f):(1.0f);

			combatants[move.armature].velocity.x+=vector.x*mod;
			combatants[move.armature].velocity.y+=vector.y;
			
			float maxX=atof(move.scripts[scriptI].params[2].c_str());
			float maxY=atof(move.scripts[scriptI].params[3].c_str());
			if( abs(combatants[move.armature].velocity.x)>abs(maxX) )
				combatants[move.armature].velocity.x=maxX*mod;
			if( abs(combatants[move.armature].velocity.y)>abs(maxY) )
				combatants[move.armature].velocity.y=maxY;
	}
}
void Character::Script_MoveMul(MoveListItem& move, int scriptI, float currentFrame){
	int frame = atoi(move.scripts[scriptI].params[2].c_str());
	if((frame>=0 && currentFrame>=frame) || (frame<0 && move.recordedHit.active) ){
		XMFLOAT2 vector = XMFLOAT2(atof(move.scripts[scriptI].params[0].c_str()),atof(move.scripts[scriptI].params[1].c_str()));
		combatants[move.armature].velocity.x*=vector.x;
		combatants[move.armature].velocity.y*=vector.y;
		Script_Remove(move,scriptI);
	}
}
void Character::Script_Dash(MoveListItem& move, int scriptI, float currentFrame){
	if(currentFrame>=atoi(move.scripts[scriptI].params[2].c_str())){
		XMFLOAT2 vector = XMFLOAT2(atof(move.scripts[scriptI].params[0].c_str()),atof(move.scripts[scriptI].params[1].c_str()));
		float mod = (combatants[move.armature].prevFacing<0)?(-1.0f):(1.0f);
		combatants[move.armature].velocity.x+=vector.x*mod;
		combatants[move.armature].velocity.y+=vector.y;
		Script_Remove(move,scriptI);
	}
}
void Character::Script_ToggleAction(MoveListItem& move, int scriptI, float currentFrame){
	if(atoi(move.scripts[scriptI].params[1].c_str())==(int)currentFrame){
		MoveListItem* a = getMoveByName(move.scripts[scriptI].params[0]);
		if(a){
			move = *a;
			ChangeArmatureAction(move.armature,move.actionI);
		}
		//Script_Remove(move,scriptI);
	}
}
void Character::Script_OnHitAction(MoveListItem& move, int scriptI, float currentFrame){
	
}
void Character::Script_RibbonTrail(MoveListItem& move, int scriptI, float currentFrame){
	if(currentFrame>=atoi(move.scripts[scriptI].params[1].c_str())
		&& currentFrame<=atoi(move.scripts[scriptI].params[2].c_str())){
			stringstream objectName("");
			objectName<<move.scripts[scriptI].params[0]<<identifier;
			Object* trailObject = nullptr;
			for(Object* object : e_objects){
				if(!object->name.compare(objectName.str())){
					trailObject=object;
					break;
				}
			}

			if(trailObject!=nullptr){
				IncreaseTrails(
					trailObject,
					XMFLOAT3( 
							atof(move.scripts[scriptI].params[3].c_str()) / 256.0f
							,atof(move.scripts[scriptI].params[4].c_str()) / 256.0f
							,atof(move.scripts[scriptI].params[5].c_str()) / 256.0f
							) 
					);
			}
	}
	else if(currentFrame > atoi(move.scripts[scriptI].params[2].c_str()))
		Script_Remove(move,scriptI);
}
void Character::Script_Clip(MoveListItem& move, int scriptI, float currentFrame){
	int frame0 = atoi(move.scripts[scriptI].params[0].c_str());
	int frame1 = atoi(move.scripts[scriptI].params[1].c_str());
	if(currentFrame>=frame0
		&& currentFrame<=frame1){
			XMFLOAT4 box = XMFLOAT4(atof(move.scripts[scriptI].params[2].c_str()),atof(move.scripts[scriptI].params[3].c_str())
				,atof(move.scripts[scriptI].params[4].c_str()),atof(move.scripts[scriptI].params[5].c_str()));
			combatants[move.armature].clip.pos.x+=combatants[move.armature].prevFacing*box.x;
			combatants[move.armature].clip.pos.y+=box.y;
			combatants[move.armature].clip.siz.x=box.z;
			combatants[move.armature].clip.siz.y=box.w;
	}
	else if(currentFrame > frame1)
		Script_Remove(move,scriptI);
}
void Character::Script_Block(MoveListItem& move, int scriptI, float currentFrame){
	if(currentFrame>=atoi(move.scripts[scriptI].params[0].c_str())
		&& currentFrame<=atoi(move.scripts[scriptI].params[1].c_str())){
			XMFLOAT4 box = XMFLOAT4(atof(move.scripts[scriptI].params[2].c_str()),atof(move.scripts[scriptI].params[3].c_str())
				,atof(move.scripts[scriptI].params[4].c_str()),atof(move.scripts[scriptI].params[5].c_str()));
			Combatant& combatant = combatants[move.armature];
			combatant.block=Hitbox2D();
			combatant.block.pos.x=combatants[move.armature].position.x;
			combatant.block.pos.y=combatants[move.armature].position.y;
			combatant.block.pos.x+=combatants[move.armature].prevFacing*box.x;
			combatant.block.pos.y+=box.y;
			combatant.block.siz.x=box.z;
			combatant.block.siz.y=box.w;
	}
	else if(currentFrame > atoi(move.scripts[scriptI].params[1].c_str()))
		Script_Remove(move,scriptI);
}
void Character::Script_Image(MoveListItem& move, int scriptI, float currentFrame){
	Combatant& combatant = combatants[move.armature];
	int paramC = move.scripts[scriptI].params.size();
	bool activate = false;
	bool toRemove = false;
	for(int i=0;i<paramC;++i){ //Check if it will be triggering now
		if(!strcmp(move.scripts[scriptI].params[i].c_str(),"trigger")){
			int onFrame = atoi(move.scripts[scriptI].params[i+1].c_str());
			if(onFrame < 0){
				if(move.recordedHit.active){
					activate = true;
					break;
				}
			}
			else if(onFrame == (int)currentFrame){
				activate = true;
				toRemove = true;
				break;
			}
		}
	}

	if(activate){ //Trigger
		oImage* image = new oImage();
		image->effects.pos=move.recordedHit.pos;
		if(combatant.facing<0){
		}
		string texture="",mask="",normal="";

		int i=0;
		while(i<paramC){
			if(!strcmp(move.scripts[scriptI].params[i].c_str(),"image")){ texture=move.scripts[scriptI].params[++i].c_str(); }
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"normal")){ normal=move.scripts[scriptI].params[++i].c_str(); }
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"mask")){ mask=move.scripts[scriptI].params[++i].c_str(); }
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"blendMode")){ 
				++i;
				if(!strcmp(move.scripts[scriptI].params[i].c_str(),"alpha")) image->effects.blendFlag=BLENDMODE::BLENDMODE_ALPHA;
				else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"additive")) image->effects.blendFlag=BLENDMODE::BLENDMODE_ADDITIVE;
				else image->effects.blendFlag=BLENDMODE::BLENDMODE_OPAQUE;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"size")){ 
				image->effects.siz=XMFLOAT2(atof(move.scripts[scriptI].params[i+1].c_str()),atof(move.scripts[scriptI].params[i+2].c_str()));
				i+=2;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"scale")){ 
				image->effects.scale=XMFLOAT2(atof(move.scripts[scriptI].params[i+1].c_str()),atof(move.scripts[scriptI].params[i+2].c_str())); 
				i+=2;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"drawRec")){ 
				image->effects.drawRec=XMFLOAT4(
					atof(move.scripts[scriptI].params[i+1].c_str()),atof(move.scripts[scriptI].params[i+2].c_str())
					,atof(move.scripts[scriptI].params[i+3].c_str()),atof(move.scripts[scriptI].params[i+4].c_str())
					); 
				i+=4;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"position")){ 
				image->effects.pos=XMFLOAT3(
					combatant.position.x+atof(move.scripts[scriptI].params[i+1].c_str()),combatant.position.y+atof(move.scripts[scriptI].params[i+2].c_str())
					,atof(move.scripts[scriptI].params[i+3].c_str())
					); 
				i+=3;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"rotation")){ 
				if(!strcmp(move.scripts[scriptI].params[i+1].c_str(),"random"))
					image->effects.rotation = rand()%360;
				else image->effects.rotation=atoi(move.scripts[scriptI].params[i+1].c_str());
				i+=1;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"sphere")){ 
				HitSphere* sph = getSphereByName(move.scripts[scriptI].params[++i].c_str());
				if(sph!=nullptr) 
					image->effects.pos=sph->translation;
			}
			else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"animation")){ 
				stack<char> blocks;
				if(!strcmp(move.scripts[scriptI].params[++i].c_str(),"{")) blocks.push('{');
				while(!blocks.empty()){
					++i;
					if(!strcmp(move.scripts[scriptI].params[i].c_str(),"}")){ blocks.pop(); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"opa")){ image->anim.opa=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"fad")){ image->anim.fad=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"repeatable")){ image->anim.repeatable=atoi(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"velocity")){ 
						image->anim.vel=XMFLOAT3(atof(move.scripts[scriptI].params[++i].c_str()),atof(move.scripts[scriptI].params[++i].c_str()),atof(move.scripts[scriptI].params[++i].c_str()));
					}
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"rot")){ image->anim.rot=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"opa")){ image->anim.opa=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"scaleX")){ image->anim.scaleX=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"scaleY")){ image->anim.scaleY=atof(move.scripts[scriptI].params[++i].c_str()); }
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"movingTexAnim")){
						stack<char> blockM;
						if(!strcmp(move.scripts[scriptI].params[++i].c_str(),"{")) blockM.push('{');
						while(!blockM.empty()){
							++i;
							if(!strcmp(move.scripts[scriptI].params[i].c_str(),"}")) blockM.pop();
							if(!strcmp(move.scripts[scriptI].params[i].c_str(),"speedX")){ image->anim.movingTexAnim.speedX=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"speedY")){ image->anim.movingTexAnim.speedY=atof(move.scripts[scriptI].params[++i].c_str()); }
						}
					}
					else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"drawRecAnim")){
						stack<char> blockD;
						if(!strcmp(move.scripts[scriptI].params[++i].c_str(),"{")) blockD.push('{');
						while(!blockD.empty()){
							++i;
							if(!strcmp(move.scripts[scriptI].params[i].c_str(),"}")) blockD.pop();
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"frameCount")){ image->anim.drawRecAnim.frameCount=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"jumpX")){ image->anim.drawRecAnim.jumpX=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"jumpY")){ image->anim.drawRecAnim.jumpY=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"onFrameChangeWait")){ image->anim.drawRecAnim.onFrameChangeWait=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"sizX")){ image->anim.drawRecAnim.sizX=atof(move.scripts[scriptI].params[++i].c_str()); }
							else if(!strcmp(move.scripts[scriptI].params[i].c_str(),"sizY")){ image->anim.drawRecAnim.sizY=atof(move.scripts[scriptI].params[++i].c_str()); }
						}
					}
				}
			}

			++i;
		}

		//if(move.armature)
		//	image->effects.mirror=combatants[move.armature].facing;
		image->effects.typeFlag=WORLD;
		image->effects.pivotFlag=Pivot::CENTER;
		image->effects.sampleFlag=SAMPLEMODE_WRAP;
		stringstream texF,masF,norF;
		texF<<"images/"<<texture;
		masF<<"images/"<<mask;
		norF<<"images/"<<normal;
		image->CreateReference(texF.str(),masF.str(),norF.str());
		images.push_back(image);
		if(toRemove) Script_Remove(move,scriptI);
	}
	else return;
}
void Character::Script_Sound(MoveListItem& move, int scriptI, float currentFrame){
	int trigger = atoi(move.scripts[scriptI].params[0].c_str());
	if(trigger>=0 && currentFrame>=trigger){
		((Sound*)ResourceManager::add(move.scripts[scriptI].params[1]))->Play();
		Script_Remove(move,scriptI);
	}
	else if(trigger<0 && move.recordedHit.active){
		((Sound*)ResourceManager::add(move.scripts[scriptI].params[1]))->Play();
	}
}
void Character::Script_Cam(MoveListItem& move, int scriptI, float currentFrame){
	static Camera* camMemory = new Camera(1280,720,Renderer::cam->zNearP,Renderer::cam->zFarP,XMVectorSet(0,0,0,1));

	string cn = move.scripts[scriptI].params[0];
	int f1s = atoi(move.scripts[scriptI].params[1].c_str());
	int f1e = atoi(move.scripts[scriptI].params[2].c_str());
	int f2s = atoi(move.scripts[scriptI].params[3].c_str());
	int f2e = atoi(move.scripts[scriptI].params[4].c_str());

	int ai = 0, bi = 0;
	int camI = -1;
	int i=0;
	static ActionCamera cc;
	for(ActionCamera& c : cameras)
		if(!cn.compare(c.name)){
			cc = c;
			camI=1;
		}

	if(camI>=0){
		XMVECTOR eye,at,up;
		eye=XMVectorSet(0,0,0,1);
		at=XMVectorSet(0,-1,0,1);
		up=XMVectorSet(0,0,1,1);
		if((int)currentFrame<=f1s){ //SAVE CAMERA
			camMemory->Eye=Renderer::cam->Eye;
			camMemory->refEye=Renderer::cam->refEye;
			camMemory->leftrightRot=Renderer::cam->leftrightRot;
			camMemory->updownRot=Renderer::cam->updownRot;
			camMemory->Up=Renderer::cam->Up;
			StopTime();
		}
		else if(currentFrame>f1s && currentFrame<f2e){
			cc.getTransform();
			XMVECTOR ROT = XMLoadFloat4(&cc.rotation);
			XMVECTOR POS = XMLoadFloat3(&cc.translation);
			XMVECTOR MIDPOS = POS;
			XMVECTOR MIDROT = ROT;
			XMMATRIX MR;
			float ip = 0;
			if(currentFrame>f1s && currentFrame<=f1e){ //FOCUS
				ip = (float)(currentFrame-f1s)/(float)(f1e-f1s);
				MIDPOS = XMVectorLerp(camMemory->Eye,POS,ip);
				MIDROT = XMQuaternionSlerp(XMVectorSet(-0.707f,0,0,0.707f),ROT,ip);
			}
			else if(currentFrame>f1e && currentFrame<f2s){ //FOLLOW
				//ip=1;
			}
			else if(currentFrame>=f2s && currentFrame<f2e){ //DEFOCUS
				ip = (float)(currentFrame-f2s)/(float)(f2e-f2s);
				MIDPOS = XMVectorLerp(POS,camMemory->Eye,ip);
				MIDROT = XMQuaternionSlerp(ROT,XMVectorSet(-0.707f,0,0,0.707f),ip);
			}
			
			MR = XMMatrixRotationQuaternion( MIDROT );
			eye=MIDPOS;
			up=XMVector3Transform(up, MR );
			at=XMVector3Transform(at, MR );
			at+=eye;
			
			Renderer::cam->View = XMMatrixLookAtLH(eye,at,up);
			Renderer::cam->Eye=eye;
			XMVECTOR refEye = XMVectorMultiply(eye,XMVectorSet(1,-1,1,1));
			XMVECTOR refAt = XMVectorMultiply(at,XMVectorSet(1,-1,1,1));
			XMVECTOR camRight = XMVector3Transform(XMVectorSet(1,0,0,0),MR);
			XMVECTOR invUp = XMVector3Cross(camRight,XMVectorSubtract(refAt,refEye));
			Renderer::cam->refView = XMMatrixLookAtLH(refEye,refAt,up);
		}
		else{ //RESTORE CAMERA
			Renderer::cam->Eye=camMemory->Eye;
			Renderer::cam->refEye=camMemory->refEye;
			Renderer::cam->leftrightRot=camMemory->leftrightRot;
			Renderer::cam->updownRot=camMemory->updownRot;
			Renderer::cam->Up=camMemory->Up;

			UnStopTime();
			Script_Remove(move,scriptI);
		}
	}
	else 
		return;
}
void Character::Script_Burst(MoveListItem& move, int scriptI, float currentFrame){
	stringstream systemName("");
	systemName<<move.scripts[scriptI].params[0]<<identifier;
	float burstNum = atof(move.scripts[scriptI].params[1].c_str());
	int frame = atoi(move.scripts[scriptI].params[2].c_str());

	if( (frame>=0 && currentFrame>=frame) || (frame<0 && move.recordedHit.active) ){
		
		map<string,vector<EmittedParticle*>>::iterator iter=emitterSystems.find(systemName.str());
		if(iter!=emitterSystems.end()){
			for(EmittedParticle* e:iter->second){
				e->Burst(burstNum);
				Script_Remove(move,scriptI);
			}
		}

		//bool found = false;
		//for(Object* o : e_objects){
		//	for(int j=0;j<o->eParticleSystems.size();++j){
		//		if(!o->eParticleSystems[j]->name.compare(particleSystem)){
		//			o->eParticleSystems[j]->Burst(burstNum);
		//			Script_Remove(move,scriptI);
		//			found = true;
		//			break;
		//		}
		//	}
		//	if(found)
		//		break;
		//}
	}
}
void Character::Script_Emit(MoveListItem& move, int scriptI, float currentFrame){
	stringstream systemName("");
	systemName<<move.scripts[scriptI].params[0]<<identifier;
	float burstNum = atof(move.scripts[scriptI].params[1].c_str());
	Interval<int> interval(move.scripts[scriptI].params[2]);

	if( GetGameSpeed() && currentFrame-(int)currentFrame<=GetGameSpeed() && interval.contains(currentFrame) ){
		
		map<string,vector<EmittedParticle*>>::iterator iter=emitterSystems.find(systemName.str());
		if(iter!=emitterSystems.end()){
			for(EmittedParticle* e:iter->second){
				e->Burst(burstNum);
			}
		}

		//bool found = false;
		//for(Object* o : e_objects){
		//	for(int j=0;j<o->eParticleSystems.size();++j){
		//		if(!o->eParticleSystems[j]->name.compare(particleSystem)){
		//			o->eParticleSystems[j]->Burst(burstNum);
		//			//Script_Remove(move,scriptI);
		//			found = true;
		//			break;
		//		}
		//	}
		//	if(found)
		//		break;
		//}
	}
}
void Character::Script_Opacity(MoveListItem& move, int scriptI, float currentFrame){
	stringstream materialName("");
	materialName<<move.scripts[scriptI].params[0]<<identifier;
	Interval<float> frames(move.scripts[scriptI].params[1]);
	Interval<float> values(move.scripts[scriptI].params[2]);

	if(frames.contains(currentFrame) ){
		
		MaterialCollection::iterator iter=materials.find(materialName.str());
		if(iter!=materials.end()){
			iter->second->alpha=values.end;
			//iter->second->alpha = WickedMath::Lerp(values.begin,values.end,(currentFrame-frames.begin)/abs(frames.end-frames.begin));
		}

	}
}
void Character::Script_Detach(MoveListItem& move, int scriptI, float currentFrame){
	int frame = 0;
	if(!move.scripts[scriptI].params[1].compare("end"))
		frame=move.armature->actions[move.armature->activeAction].frameCount-1;
	else
		frame = atoi( move.scripts[scriptI].params[1].c_str() );
	stringstream node("");
	node<<move.scripts[scriptI].params[0]<<identifier;

	if(frame<=currentFrame){
		if(transforms.find(node.str())!=transforms.end()){
			Transform* parentNode = transforms[node.str()];
			parentNode->detachChild();
		}
		Script_Remove(move,scriptI);
	}
}
void Character::Script_Decal(MoveListItem& move, int scriptI, float currentFrame){
	int frame = atoi(move.scripts[scriptI].params[0].c_str());

	if( (frame>=0 && currentFrame>=frame) || (frame<0 && move.recordedHit.active) ){

		string& texture = move.scripts[scriptI].params[1];
		XMFLOAT3 armaturePos = move.armature->translation_rest;
		XMFLOAT3 translation,scale;
		XMFLOAT4 rotation;
		
		translation=XMFLOAT3(armaturePos.x+(getMainCombatant().facing)*atof(move.scripts[scriptI].params[2].c_str()),armaturePos.y+atof(move.scripts[scriptI].params[3].c_str()),armaturePos.z+atof(move.scripts[scriptI].params[4].c_str()));
		scale=XMFLOAT3(atof(move.scripts[scriptI].params[5].c_str()),atof(move.scripts[scriptI].params[6].c_str()),atof(move.scripts[scriptI].params[7].c_str()));
		XMStoreFloat4(&rotation,XMQuaternionRotationRollPitchYaw(XM_PI*0.5f,0,XM_PI*((rand()%1000)*0.001f)));

		Decal* decal = new Decal(translation,scale,rotation,texture);
		decal->life=800;
		decal->fadeStart=40;
		decals.push_back(decal);

		Script_Remove(move,scriptI);
	}
}

