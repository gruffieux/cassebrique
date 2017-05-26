#include "ccapsule.h"

Capsule::Capsule()
{
	Effect = NO_EFFECT;
}

Capsule::Capsule(EFFECT Capsule_Effect, int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name) :
Item(Item_xpos, Item_ypos, Item_width, Item_height, Object_Name),
Mover(Mover_angle, Mover_acc, Mover_dec)
{
	Effect = Capsule_Effect;
}
