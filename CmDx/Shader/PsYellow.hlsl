
float4 colorScale(float4 clr)
{
	return clr/0.99;
}
 

float4 main(float4 Pos : SV_POSITION) : SV_Target //SV_TARGET
{
	return colorScale(float4( 1.0f, 1.0f, 0.0f, 1.0f ));    // Yellow, with Alpha = 1
}