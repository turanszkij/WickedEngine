#include "globals.hlsli"

RWRAWBUFFER(arguments, 0);
globallycoherent RWSTRUCTUREDBUFFER(rayCounter, uint, 1);

[numthreads(1, 1, 1)]
void main() {
	uint ray_count = rayCounter[0];

	arguments.Store3(0, uint3((ray_count + 63) / 64, 1, 1));

	rayCounter[0] = 0;
	rayCounter[1] = ray_count;
}
