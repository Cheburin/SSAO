float4 VS(uint VertexID : SV_VERTEXID):SV_POSITION
{
  return float4( 0, 0, 0, 1.0 );
} 

ExpandPos _p(float x, float y){
  ExpandPos p;
  p.pos = float4(x, y, 0.5, 1); 
  return p;
}

[maxvertexcount(4)]
void GS(point ExpandPos pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<ExpandPos> triStream )
{
	triStream.Append(_p(-1, -1));
	triStream.Append(_p(-1,  1));
	triStream.Append(_p( 1, -1));
	triStream.Append(_p( 1,  1));
    triStream.RestartStrip();
}