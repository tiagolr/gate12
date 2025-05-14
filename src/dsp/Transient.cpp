#include "Transient.h"
#include <cmath>

void Transient::clear(double sampleRate)
{
	srate = sampleRate;
	double attTau = 0.1 / 1000.0; // milliseconds
	double relTau = 100.0 / 1000.0; // milliseconds
	attAlpha = std::exp(-1.0 / (attTau * srate));
	relAlpha = std::exp(-1.0 / (relTau * srate));
	envelope = 0.0;
	prevEnvelope = 0.0;
	cooldown = 0;
    drumsBuf.resize((int)(srate * globals::AUDIO_DRUMSBUF_MILLIS / 1000.0), 0.0);
	drumsBufIdx = 0;
	energy = 0.0;
    prevEnergy = 0.0;
}

void Transient::startCooldown()
{
	cooldown = (int)(srate * globals::AUDIO_COOLDOWN_MILLIS / 1000.0);
}

bool Transient::detect(int algo, double sample, double thres, double sense)
{
	return algo == 0 
		? detectSimple(sample, thres, sense)
		: detectDrums(sample, thres, sense);
}

bool Transient::detectSimple(double sample, double thres, double sense)
{
	bool isAttack = std::fabs(sample) > envelope;
    envelope = isAttack 
		? attAlpha * envelope + (1.0 - attAlpha) * std::fabs(sample)
		: relAlpha * envelope + (1.0 - relAlpha) * std::fabs(sample);

    double diff = envelope - prevEnvelope;
	prevEnvelope = envelope;

	diff *= 10; // unscientific method to make diff more sensitive
	if (cooldown)
		cooldown -= 1; 

	hit = !cooldown && diff > sense && std::fabs(sample) > thres;
    return hit;
}

bool Transient::detectDrums(double sample, double thres, double sense)
{
	double energySample = sample * sample;
	const auto size = drumsBuf.size();

	energy += energySample - drumsBuf[drumsBufIdx];
	drumsBuf[drumsBufIdx] = energySample;
	drumsBufIdx = (drumsBufIdx + 1) % size;

	double totalEnergy = std::sqrt(energy / size); // RMS
	double diff = totalEnergy - prevEnergy;
	prevEnergy = totalEnergy;

	diff *= 75; // same story
	if (cooldown)
		cooldown -= 1;

	hit = !cooldown && diff > sense && std::fabs(sample) > thres;
	return hit;
}