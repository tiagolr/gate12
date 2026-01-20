#include "Splitter.h"

void Splitter::setFreqs(float srate, float lp, float hp, int slope)
{
    // 6dB
    if (slope == 0) {
	    freqHP = std::clamp(hp, 20.0f, std::min(20000.0f, srate*0.49f));
	    xHP = std::exp(-2.0f*PI*freqHP/srate);
	    a0HP = 1.0f-xHP;
	    b1HP = -xHP;

	    freqLP = std::clamp(lp, 20.f, std::min(20000.f, srate*0.49f));
	    xLP = std::exp(-2.0f*PI*freqLP/srate);
	    a0LP = 1.0f-xLP;
	    b1LP = -xLP;
    }

    // 12dB and more
    else {
        svfL.lowa.setFreq(srate, lp, slope == 1 ? q12 : q24);
        svfL.lowa2.copyFrom(svfL.lowa);
        svfL.lowb.setFreq(srate, hp, slope == 1 ? q12 : q24);
        svfL.lowb2.copyFrom(svfL.lowb);
        svfL.mida.setFreq(srate, lp, slope == 1 ? q12 : q24);
        svfL.mida2.copyFrom(svfL.mida);
        svfL.midb.setFreq(srate, hp, slope == 1 ? q12 : q24);
        svfL.midb2.copyFrom(svfL.midb);
        svfL.higha.setFreq(srate, lp);
        svfL.higha2.setFreq(srate, lp, q24);
        svfL.highb.setFreq(srate, hp, slope == 1 ? q12 : q24);
        svfL.highb2.copyFrom(svfL.highb);
        svfR.copyFrom(svfL);
    }

}

void Splitter::clear()
{
	hpL = 0.f;
	hpR = 0.f;

	lpL = 0.f;
	lpR = 0.f;

    svfL.clear();
    svfR.clear();
}

void Splitter::processBlock(
    int slope, const float* left, const float* right,
    float* lowl, float* lowr, float* midl, float* midr, float* hil,
    float* hir, int nsamps)
{
    if (slope == 0)
        processBlock6dB(left, right, lowl, lowr, midl, midr, hil, hir, nsamps);
    else if (slope == 1)
        processBlock12dB(left, right, lowl, lowr, midl, midr, hil, hir, nsamps);
    //else
    //    processBlock24dB(left, right, lowl, lowr, midl, midr, hil, hir, nsamps);
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
        float l0 = svfL.lowa.process(left[i]);
        l0 = svfL.lowb.process(l0);
        float m0 = svfL.mida.process(-left[i]);
        m0 = svfL.midb.process(m0);
        float h0 = svfL.higha.process(-left[i]);
        h0 = svfL.highb.process(-h0);

        lowl[i] = l0;
        midl[i] = m0;
        hil[i] = h0;

        float l1 = svfR.lowa.process(right[i]);
        l1 = svfR.lowb.process(l1);
        float m1 = svfR.mida.process(-right[i]);
        m1 = svfR.midb.process(m1);
        float h1 = svfR.higha.process(-right[i]);
        h1 = svfR.highb.process(-h1);

        lowr[i] = l1;
        midr[i] = m1;
        hir[i] = h1;
    }
}