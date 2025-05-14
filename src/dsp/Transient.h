// Copyright 2025 tilr
// Transient detector
#pragma once

#include <vector>
#include <deque>
#include "../Globals.h"

class Transient
{
public:
	Transient() {};
	~Transient() {};

	void startCooldown();
	bool detect(int algo, double sample, double thres, double sense);
	bool detectSimple(double sample, double thres, double sense);
	bool detectDrums(double sample, double thres, double sense);
	void clear(double srate);
	
	int cooldown = 0; // prevent triggers during cooldown (in samples)
	bool hit = false;

private:
	// simple/envelope algo
	double srate = 44100.0;
	double envelope = 0.0;
	double prevEnvelope = 0.0;
	double attAlpha = 0.99; // env smoothing factor
	double relAlpha = 0.99; // env smoothing factor

	// drums algo
	std::vector<double> drumsBuf;
	int drumsBufIdx = 0;
	double energy = 0.0;
	double prevEnergy = 0.0;
};