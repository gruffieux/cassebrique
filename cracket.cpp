#include "cracket.h"

Racket::Racket()
{
	SpinTotal = 0;
	CurrentEffect = NO_EFFECT;
}

Racket::Racket(int Item_width, int Item_height, double Mover_acc, double Mover_dec, Str Object_Name) :
Item(NULL, NULL, Item_width, Item_height, Object_Name),
Mover(0, Mover_acc, Mover_dec)
{
	SpinTotal = 0;
	CurrentEffect = NO_EFFECT;
}
