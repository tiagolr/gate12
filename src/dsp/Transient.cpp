#include "Transient.h"
#include <cmath>

void Transient::clear(double srate)
{
	double tau = 2.5 / 1000.0; // seconds
	alpha = std::exp(-1.0 / (tau * srate));
	envelope = 0.0;
	prevEnvelope = 0.0;

    drumsBuf.resize((int)(srate * 0.01), 0.0); // 10ms buffer
    prevEnergy = 0.0;
}

bool Transient::detect(int algo, double sample, double thres, double sense)
{
	return algo == 0 
		? detectSimple(sample, thres, sense)
		: detectDrums(sample, thres, sense);
}

bool Transient::detectSimple(double sample, double thres, double sense)
{
    envelope = alpha * envelope + (1.0 - alpha) * std::fabs(sample);

    double diff = envelope - prevEnvelope;
	prevEnvelope = envelope;

    return diff > sense && std::fabs(sample) > thres;
}

bool Transient::detectDrums(double sample, double thres, double sense)
{
	drumsBuf.push_front(sample);
	drumsBuf.pop_back();
	const auto size = drumsBuf.size();

	double e = 0.0;
	for (int i = 0; i < size; ++i) {
		e += drumsBuf[i] * drumsBuf[i];
	}
	e = std::sqrt(e / size); // RMS
	double diff = e - prevEnergy;
	prevEnergy = e;

	return diff > sense && std::fabs(sample) > thres;
}