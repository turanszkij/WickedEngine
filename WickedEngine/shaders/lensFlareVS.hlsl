struct VertexOut{
	float4 pos : SV_POSITION;
	nointerpolation uint vid : VERTEXID;
};

VertexOut main(uint vid : SV_VertexID)
{
	VertexOut o = (VertexOut)0;
	o.pos=0;
	o.vid=vid;
	return o;
}