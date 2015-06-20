#include "sOHF.hlsli"

[maxvertexcount(1)]
void main(
	point streamOutput input[1], 
	inout PointStream< streamOutput > output
)
{
	output.Append(input[0]);
}