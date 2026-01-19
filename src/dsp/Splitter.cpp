#include "Splitter.h"

void Splitter::setFreqs(float srate, float lp, float hp)
{
	static constexpr float PI = 3.14159265358979323846f;

	freqHP = std::clamp(hp, 20.0f, std::min(20000.0f, srate*0.49f));
	xHP = std::exp(-2.0f*PI*freqHP/srate);
	a0HP = 1.0f-xHP;
	b1HP = -xHP;

	freqLP = std::clamp(lp, 20.f, std::min(20000.f, srate*0.49f));
	xLP = std::exp(-2.0f*PI*freqLP/srate);
	a0LP = 1.0f-xLP;
	b1LP = -xLP;
}

void Splitter::clear()
{
	hpL = 0.f;
	hpR = 0.f;
	hpL2 = 0.f;
	hpR2 = 0.f;

	lpL = 0.f;
	lpR = 0.f;
	lpL2 = 0.f;
	lpR2 = 0.f;
}

void Splitter::processBlock(
    int slope, const float* left, const float* right,
    float* lowl, float* lowr, float* midl, float* midr, float* hil,
    float* hir, int nsamps)
{
    if (slope == 0)
        processBlock6dB(left, right, lowl, lowr, midl, midr, hil, hir, nsamps);
    else
        processBlock12dB(left, right, lowl, lowr, midl, midr, hil, hir, nsamps);
}

void Splitter::processBlock6dB(
    const float* left, const float* right,
    float* lowl, float* lowr,
    float* midl, float* midr,
    float* hil,  float* hir,
    int nsamps)
{
    for (int i = 0; i < nsamps; ++i) {
        const float s0 = left[i];
        const float s1 = right[i];

        lpL = a0LP * s0 - b1LP * lpL;
        lpR = a0LP * s1 - b1LP * lpR;

        lowl[i] = lpL;
        lowr[i] = lpR;

        hpL = a0HP * s0 - b1HP * hpL;
        hpR = a0HP * s1 - b1HP * hpR;

        hil[i] = s0 - hpL;
        hir[i] = s1 - hpR;

        midl[i] = s0 - lowl[i] - hil[i];
        midr[i] = s1 - lowr[i] - hir[i];
    }
}

void Splitter::processBlock12dB(
    const float* left, const float* right,
    float* lowl, float* lowr,
    float* midl, float* midr,
    float* hil, float* hir,
    int nsamps)
{
    for (int i = 0; i < nsamps; ++i) {
        const float s0 = left[i];
        const float s1 = right[i];

        // Lp
        lpL = a0LP * s0 - b1LP * lpL;
        lpR = a0LP * s1 - b1LP * lpR;
        lpL2 = a0LP * lpL - b1LP * lpL2;
        lpR2 = a0LP * lpR - b1LP * lpR2;

        lowl[i] = lpL2;
        lowr[i] = lpR2;

        // Hp
        hpL = a0HP * s0 - b1HP * hpL;
        hpR = a0HP * s1 - b1HP * hpR;
        hpL2 = a0HP * hpL - b1HP * hpL2;
        hpR2 = a0HP * hpR - b1HP * hpR2;

        hil[i] = s0 - hpL2;
        hir[i] = s1 - hpR2;

        // Mid
        midl[i] = s0 - lowl[i] - hil[i];
        midr[i] = s1 - lowr[i] - hir[i];
    }
}