#include "Renderer.h"


Character::Req_input::Req_input(Character* c, const string& newData):Requirement(c,"input"){
	data.push_back(newData);
	input=newData;
	overrider = !input.compare("0");
	special = convertInputString(input).size()>1;
}
bool Character::Req_input::validate(){
	if(overrider)
		return true;

	vector<Input> v = convertInputString(input);

	if(!special)
		return InputCompare(input,c->directinput);
	return InputCompare(input,c->finalInput);

	//if(input.length()<=1)
	//	return (!c->directinput.compare(input));
	//return (c->finalInput.find(input)!=string::npos);
}

Character::Req_timeout::Req_timeout(Character* c, const string& newData, Combatant& newCombatant):Requirement(c,"input"),combatant(newCombatant){ //type must be "input" because it overrides input if no input is captured
	data.push_back(newData);
	timeout=atoi(newData.c_str());
}
bool Character::Req_timeout::validate(){
	return combatant.state->move->timer>=timeout;
}

Character::Req_frame::Req_frame(Character* c, const string& newData, MoveListItem* newMove):Requirement(c,"frame"){
	data.push_back(newData);
	move=newMove;
	if(!newData.empty()){
		if(!newData.compare("end") && move!=nullptr && move->armature!=nullptr){
			interval=Interval<int>(move->armature->actions[move->actionI].frameCount);
		}
		else{
			interval=Interval<int>(newData);
		}
	}
}
bool Character::Req_frame::validate(){
	if(move==nullptr || move->armature==nullptr)
		return false;

	return interval.contains(move->armature->currentFrame);
}

Character::Req_height::Req_height(Character* c, const string& newData, MoveListItem* newMove):Requirement(c,"height"){
	data.push_back(newData);
	move=newMove;
	interval=Interval<float>(newData);
}
bool Character::Req_height::validate(){
	return interval.contains(move->armature->translation.y);
}

Character::Req_hit::Req_hit(Character* c, Combatant& newCombatant):Requirement(c,"hit"),combatant(newCombatant){
}
bool Character::Req_hit::validate(){
	return combatant.hitConfirmed;
}

Character::Req_nostun::Req_nostun(Character* c, Combatant& newCombatant):Requirement(c,"nostun"),combatant(newCombatant){
}
bool Character::Req_nostun::validate(){
	return combatant.stun<=0;
}

Character::Req_onstun::Req_onstun(Character* c, Combatant& newCombatant):Requirement(c,"onstun"),combatant(newCombatant){
}
bool Character::Req_onstun::validate(){
	return combatant.stun>0;
}

Character::Req_onhit::Req_onhit(Character* c, Combatant& newCombatant):Requirement(c,"onhit"),combatant(newCombatant){
}
bool Character::Req_onhit::validate(){
	return combatant.hitIn;
}

Character::Req_noblockstun::Req_noblockstun(Character* c, Combatant& newCombatant):Requirement(c,"noblockstun"),combatant(newCombatant){
}
bool Character::Req_noblockstun::validate(){
	return combatant.blockstun<=0;
}

Character::Req_onblockstun::Req_onblockstun(Character* c, Combatant& newCombatant):Requirement(c,"onblockstun"),combatant(newCombatant){
}
bool Character::Req_onblockstun::validate(){
	return combatant.blockstun>0;
}

Character::Req_health::Req_health(Character* c, const string& newData):Requirement(c,"health"){
	data.push_back(newData);
	health=atoi(newData.c_str());
}
bool Character::Req_health::validate(){
	return c->cData.hp>health;
}
void Character::Req_health::activate(){
	c->cData.hp -= health;
}

Character::Req_meter::Req_meter(Character* c, const string& newData):Requirement(c,"meter"){
	data.push_back(newData);
	meter=atoi(newData.c_str());
}
bool Character::Req_meter::validate(){
	return c->cData.meter>=meter;
}
void Character::Req_meter::activate(){
	c->cData.meter -= meter;
}


Character::Req_facing::Req_facing(Character* c, Combatant& newCombatant):Requirement(c,"facing"),combatant(newCombatant){
}
bool Character::Req_facing::validate(){
	return combatant.facing!=combatant.prevFacing;
}
void Character::Req_facing::activate(){
	combatant.prevFacing=combatant.facing;
	c->TurnFacing(combatant.state->move->armature);
}

Character::Req_died::Req_died(Character* c):Requirement(c,"died"){
}
bool Character::Req_died::validate(){
	return c->cData.hp<=0;
}

Character::Req_block::Req_block(Character* c, Combatant& newCombatant):Requirement(c,"block"),combatant(newCombatant){
}
bool Character::Req_block::validate(){
	return combatant.canBlock;
}

Character::Req_falling::Req_falling(Character* c, Combatant& newCombatant):Requirement(c,"falling"),combatant(newCombatant){
}
bool Character::Req_falling::validate(){
	return combatant.velocity.y<=0;
}

Character::Req_hitproperty::Req_hitproperty(Character* c, Combatant& newCombatant, const string& prop):Requirement(c,"hitproperty"),combatant(newCombatant),hitproperty(prop){
}
bool Character::Req_hitproperty::validate(){
	return !combatant.hitProperty.compare(hitproperty);
}




