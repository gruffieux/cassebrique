#ifndef CCAPSULE
#define CCAPSULE

#include <cmover.h>
#include <cvisual.h>
#include "main.h"

class Capsule : public Mover, public Visual
{
private:
	EFFECT Effect;
public:
	Capsule();
	Capsule(EFFECT Capsule_Effect, int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name);
	~Capsule() {}
	EFFECT GetEffect() {return Effect;}
	static Capsule* getCapsuleElement(int index, List *pList) {return dynamic_cast<Capsule*>(pList->GetElement(index));}
};

#endif