#ifndef CBALL
#define CBALL

#include <cmover.h>
#include <cvisual.h>
#include "main.h"

class Ball : public Mover, public Visual
{
private:
	int SpinCounter, SpinTotal;
	double speed;
	ROTATION Rotation;
	Clock Velocity;
public:
	Ball();
	Ball(int Item_xpos, int Item_ypos, int Item_width, int Item_height, double Mover_angle, double Mover_acc, double Mover_dec, Str Object_Name);
	~Ball() {}
	void InitSpin(int total, ROTATION r);
	bool Spin();
	static Ball* getBallElement(int index, List *pList) {return dynamic_cast<Ball*>(pList->GetElement(index));}
};

#endif