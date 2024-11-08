#include "main.h"
#include <fstream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <locale>
#include <codecvt>
#include <string>
#include <array>
#include <locale> 

BlurParams GaussianBlur(int radius)
{
	BlurParams p(radius);

	float twoRadiusSquaredRecip = 1.0f / (2.0f * radius * radius);
	float sqrtTwoPiTimesRadiusRecip = 1.0f / ((float)sqrt(2.0f * D3DX_PI) * radius);
	float radiusModifier = 1.0f;

	float WeightsSum = 0;
	for (int i = 0; i < p.WeightLength; i++)
	{
		double x = (-radius + i) * radiusModifier;
		x *= x;
		p.Weights[i] = sqrtTwoPiTimesRadiusRecip * (float)exp(-x * sqrtTwoPiTimesRadiusRecip);
		WeightsSum += p.Weights[i];
	}

	/* NORMALIZE */
	float div = WeightsSum;
	for (int i = 0; i < p.WeightLength; i++)
		p.Weights[i] /= div;

	return p;
}