float4 MODEL_FRAG(
    in float3 normal         : TEXCOORD0
    //in float4 pos            : SV_POSITION
):SV_TARGET
{ 
/*   
   if(0.202<pos.z&&pos.z<0.203)
     discard;
   else if(0.548<pos.z&&pos.z<0.5489)
     discard;
   else if(0.890<pos.z&&pos.z<0.891)
     discard  ;
*/     
   return float4( normalize(normal), 0.0 );
}
