ClipPosNormal MODEL_VERTEX( in PosNormalTex2d i )
{
    ClipPosNormal Out;

    Out.normal = normalize( i.normal );

    Out.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
    /*Debug*/
/*    
		if(i.pos.x == -0.5 &&	i.pos.y == -0.5)
      Out.clip_pos = float4( i.pos.xy, 0.201217994, 1.0 );    
		if(i.pos.x == -0.5 &&	i.pos.y == 0.5)
      Out.clip_pos = float4( i.pos.xy, 0.548044980, 1.0 );    
		if(i.pos.x == 0.5 &&	i.pos.y == -0.5)
      Out.clip_pos = float4( i.pos.xy, 0.892149985, 1.0 );
      //Out.clip_pos = float4( i.pos.xyz, 1.0 );    
*/
    /*Debug*/

    return Out;
} 
