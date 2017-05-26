#ifndef CRACKET
#define CRACKET

#include <cmover.h>
#include <cvisual.h>
#include "main.h"

class Racket : public Mover, public Visual
{
private:
	int SpinTotal;
	EFFECT CurrentEffect;
	Mover Sight;
public:
	Racket();
	Racket(int Item_width, int Item_height, double Mover_acc, double Mover_dec, Str Object_Name);
	~Racket() {}
	void SetSpinTotal(int Racket_SpinTotal) {SpinTotal = Racket_SpinTotal;}
	void SetCurrentEffect(EFFECT Racket_CurrentEffect) {CurrentEffect = Racket_CurrentEffect;}
	int GetSpinTotal() {return SpinTotal;}
	EFFECT GetCurrentEffect() {return CurrentEffect;}
	Mover * GetSight() {return &Sight;}
};

#endif