// Three band splitter - based of 3-band splitter JSFX by LOSER
// Tilr
#pragma once
#include <algorithm>
#include <cmath>

class Splitter
{
public:
	float freqHP = 20000.f;
	float freqLP = 20.f;

	Splitter() {}
	~Splitter() {}

	void setFreqs(float srate, float hp, float lp);
	void processBlock(int slope, const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void processBlock6dB(const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void processBlock12dB(const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void clear();

private:
	float xHP = 0.f;
	float a0HP = 0.f;
	float b1HP = 0.f;
	float hpL = 0.f;
	float hpR = 0.f;
	float hpL2 = 0.f;
	float hpR2 = 0.f;

	float xLP = 0.f;
	float a0LP = 0.f;
	float b1LP = 0.f;
	float lpL = 0.f;
	float lpR = 0.f;
	float lpL2 = 0.f;
	float lpR2 = 0.f;
};