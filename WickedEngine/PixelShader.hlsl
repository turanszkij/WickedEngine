struct asd
{
	row_major float4x4 mat;
};

StructuredBuffer<asd> buf;

float4 main(float4 vPos : SV_POSITION) : SV_TARGET
{
	return mul( buf[12].mat, vPos );
}