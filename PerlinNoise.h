#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

#include <vector>
#include <numeric>
#include <random>

// From https://solarianprogrammer.com/2012/07/18/perlin-noise-cpp-11/
// Also https://github.com/sol-prog/Perlin_Noise
// Free to use under the GPL v3 license

class PerlinNoise
{
	std::vector<int> p;
public:
	PerlinNoise();
	PerlinNoise(unsigned int seed);

	double noise(double x, double y, double z);

private:
	double fade(double t);
	double lerp(double t, double a, double b);
	double grad(int hash, double x, double y, double z);
};

#endif