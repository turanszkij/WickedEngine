#ifndef DEFINED_FRAMESKIP
#define DEFINED_FRAMESKIP

//MUST CALL FRAMESKIP_END !
#define FRAMESKIP_START(enabled,idlethreshold) const bool frameskip = enabled; static Timer timer = Timer(); static const double dt = 1.0 / 60.0; static double accumulator = 0.0; accumulator += timer.elapsed() / 1000.0;if (accumulator>idlethreshold)accumulator = 0;timer.record();while (accumulator >= dt){
#define FRAMESKIP_END accumulator -= dt;if (!frameskip) break; }

#endif