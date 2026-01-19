// Three band splitter - based of 3-band splitter JSFX by LOSER
// Tilr
#pragma once
#include <algorithm>
#include <cmath>

class Splitter
{
public:
	double freqHP = 20000.;
	double freqLP = 20.;

	Splitter() {}
	~Splitter() {}

	void setFreqs(double srate, double hp, double lp);
	void processBlock6dB(double* left, double* right, double* lowl, double* lowr, double* midl, double* midr, double* hil, double* hir, int nsamps);
	void processBlock12dB(double* left, double* right, double* lowl, double* lowr, double* midl, double* midr, double* hil, double* hir, int nsamps);
	void clear();

private:
	double xHP = 0.f;
	double a0HP = 0.f;
	double b1HP = 0.f;
	double hpL = 0.f;
	double hpR = 0.f;
	double hpL2 = 0.f;
	double hpR2 = 0.f;

	double xLP = 0.f;
	double a0LP = 0.f;
	double b1LP = 0.f;
	double lpL = 0.f;
	double lpR = 0.f;
	double lpL2 = 0.f;
	double lpR2 = 0.f;
};