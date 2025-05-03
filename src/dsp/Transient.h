// Copyright 2025 tilr
// Transient detector
#pragma once

#include <vector>
#include <deque>

class Transient
{
public:
	Transient() {};
	~Transient() {};

	bool detect(int algo, double sample, double thres, double sense);
	bool detectSimple(double sample, double thres, double sense);
	bool detectDrums(double sample, double thres, double sense);
	void clear(double srate);

private:
	// simple algo
	double envelope = 0.0;
	double prevEnvelope = 0.0;
	double alpha = 0.99; // env smoothing factor

	// drums algo
	std::deque<double> drumsBuf;
	double prevEnergy = 0.0;
};