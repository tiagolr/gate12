// Three band splitter - 6dB based of 3-band splitter JSFX by LOSER
// 12dB and 24dB based of Frequency Splitter - Linkwitz-Riley Minimum Phase (lewloiwc)
// Tilr
#pragma once
#include <algorithm>
#include <cmath>

struct SVFAllpass1p
{
	static constexpr float PI = 3.14159265358979323846f;
	float i;
	float c;
	float cut;

	void setFreq(float srate, float cutoff)
	{
		if (cutoff != cut) {
			c = std::tan(PI * (cutoff / srate - 0.25f)) * 0.5f + 0.5f;
			cut = cutoff;
		}
	}

	float process(float x)
	{
		float r = (1 - c) * i + c * x;
		i = 2 * r - i;
		return x - 2 * r;
	}

	void copyFrom(SVFAllpass1p& ap)
	{
		c = ap.c;
		cut = ap.cut;
	}

	void clear()
	{
		i = 0.f;
	}
};

struct SVFAllpass2p
{
	static constexpr float PI = 3.14159265358979323846f;
	float g;
	float k;
	float a1;
	float a2;
	float ic1eq;
	float ic2eq;
	float cut;

	void setFreq(float srate, float cutoff, float Q)
	{
		if (cutoff + Q != cut) {
			g = std::tan(PI * cutoff / srate);
			k = 1 / Q;
			a1 = 1 / (1 + g * (g + k));
			a2 = g * a1;
			cut = cutoff + Q;
		}
	}

	float process(float x)
	{
		float v1 = a1 * ic1eq + a2 * (x - ic2eq);
		float v2 = ic2eq + g * v1;
		ic1eq = 2 * v1 - ic1eq;
		ic2eq = 2 * v2 - ic2eq;

		return x - 2 * k * v1;
	}

	void copyFrom(SVFAllpass2p& ap)
	{
		g = ap.g;
		k = ap.k;
		a1 = ap.a1;
		a2 = ap.a2;
		cut = ap.cut;
	}

	void clear()
	{
		ic1eq = ic2eq = 0.f;
	}
};

struct SVFLow
{
	static constexpr float PI = 3.14159265358979323846f;
	float g;
	float k;
	float a1;
	float a2;
	float ic1eq;
	float ic2eq;
	float cut;

	void setFreq(float srate, float cutoff, float Q)
	{
		if (cutoff + Q != cut) {
			g = std::tan(PI * cutoff / srate);
			k = 1 / Q;
			a1 = 1 / (1 + g * (g + k));
			a2 = g * a1;
			cut = cutoff + Q;
		}
	}

	float process(float x)
	{
		float v1 = a1 * ic1eq + a2 * (x - ic2eq);
		float v2 = ic2eq + g * v1;
		ic1eq = 2 * v1 - ic1eq;
		ic2eq = 2 * v2 - ic2eq;

		return v2;
	}

	void copyFrom(SVFLow& ap)
	{
		g = ap.g;
		k = ap.k;
		a1 = ap.a1;
		a2 = ap.a2;
		cut = ap.cut;
	}

	void clear()
	{
		ic1eq = ic2eq = 0.f;
	}
};

struct SVFHigh
{
	static constexpr float PI = 3.14159265358979323846f;
	float g;
	float k;
	float a1;
	float a2;
	float ic1eq;
	float ic2eq;
	float cut;

	void setFreq(float srate, float cutoff, float Q)
	{
		if (cutoff + Q != cut) {
			g = std::tan(PI * cutoff / srate);
			k = 1 / Q;
			a1 = 1 / (1 + g * (g + k));
			a2 = g * a1;
			cut = cutoff + Q;
		}
	}

	float process(float x)
	{
		float v1 = a1 * ic1eq + a2 * (x - ic2eq);
		float v2 = ic2eq + g * v1;
		ic1eq = 2 * v1 - ic1eq;
		ic2eq = 2 * v2 - ic2eq;

		return x - k * v1 - v2;
	}

	void copyFrom(SVFHigh& ap)
	{
		g = ap.g;
		k = ap.k;
		a1 = ap.a1;
		a2 = ap.a2;
		cut = ap.cut;
	}

	void clear()
	{
		ic1eq = ic2eq = 0.f;
	}
};

struct SVFStack
{
	SVFLow lowa;
	SVFLow lowa2;
	SVFLow lowb;
	SVFLow lowb2;
	SVFHigh mida;
	SVFHigh mida2;
	SVFLow midb;
	SVFLow midb2;
	SVFAllpass1p higha;
	SVFAllpass2p higha2;
	SVFHigh highb;
	SVFHigh highb2;

	void copyFrom(SVFStack b)
	{
		lowa.copyFrom(b.lowa);
		lowa2.copyFrom(b.lowa2);
		lowb.copyFrom(b.lowb);
		lowb2.copyFrom(b.lowb2);
		mida.copyFrom(b.mida);
		mida2.copyFrom(b.mida2);
		midb.copyFrom(b.midb);
		midb2.copyFrom(b.midb2);
		higha.copyFrom(b.higha);
		higha2.copyFrom(b.higha2);
		highb.copyFrom(b.highb);
		highb2.copyFrom(b.highb2);
	}

	void clear()
	{
		lowa.clear();
		lowa2.clear();
		lowb.clear();
		lowb2.clear();
		mida.clear();
		mida2.clear();
		midb.clear();
		midb2.clear();
		higha.clear();
		higha2.clear();
		highb.clear();
		highb2.clear();
	}
};

class Splitter
{
public:
	static constexpr float PI = 3.14159265358979323846f;
	static constexpr float q12 = 0.5f;
	static constexpr float q24 = 0.7071067811865476f;

	float freqHP = 20000.f;
	float freqLP = 20.f;

	Splitter() {}
	~Splitter() {}

	void setFreqs(float srate, float hp, float lp, int slope);
	void processBlock(int slope, const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void processBlock6dB(const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void processBlock12dB(const float* left, const float* right, float* lowl, float* lowr, float* midl, float* midr, float* hil, float* hir, int nsamps);
	void clear();

private:
	SVFStack svfL{};
	SVFStack svfR{};

	// 6dB
	float xHP = 0.f;
	float a0HP = 0.f;
	float b1HP = 0.f;
	float hpL = 0.f;
	float hpR = 0.f;

	float xLP = 0.f;
	float a0LP = 0.f;
	float b1LP = 0.f;
	float lpL = 0.f;
	float lpR = 0.f;
};