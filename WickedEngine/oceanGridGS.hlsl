#include "globals.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float a : TEXCOORD0;
};

[maxvertexcount(3)]
void main(
	triangle VSOut input[3],
	inout TriangleStream< VSOut > output
)
{
	bool valid = true;
	for (uint i = 0; i < 3; i++)
	{
		if (input[i].a < 0)
		{
			valid = false;
		}
	}

	if (valid)
	{
		for (uint i = 0; i < 3; i++)
		{
			VSOut element;
			element = input[i];
			output.Append(element);
		}
	}
}