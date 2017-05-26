#include "cball.h"

Ball::Ball()
{
	SpinCounter = SpinTotal = 0;
	speed = 0;
	Rotation = LIFT;
}

Ball::Ball(int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name) :
Item(Item_xpos, Item_ypos, Item_width, Item_height, Object_Name),
Mover(Mover_angle, Mover_acc, Mover_dec)
{
	SpinCounter = SpinTotal = 0;
	speed = 0;
	Rotation = LIFT;
}

void Ball::InitSpin(int total, ROTATION r)
{
	SpinCounter = 0;
	SpinTotal = total;
	speed = 100 - SpinTotal;
	Rotation = r;
}

bool Ball::Spin()
{
	if (SpinCounter == SpinTotal)
		return true;

	if (Velocity.goCounter(speed, 0, 1, false))
		return false;

	SpinCounter++;

	switch (Rotation)
	{
	case LIFT:
		DecreaseAngle(0, PI * 2, 180);
		break;
	case CUT:
		IncreaseAngle(0, PI * 2, 180);
		break;
	}

	return true;
}
