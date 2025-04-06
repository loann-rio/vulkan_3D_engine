// Convert light space position to NDC
   vec3 light_space_ndc = light_space_pos.xyz /= light_space_pos.w;
 
   // If the fragment is outside the light's projection then it is outside
   // the light's influence, which means it is in the shadow 
   if (abs(light_space_ndc.x) > 1.0 ||
       abs(light_space_ndc.y) > 1.0 ||
       abs(light_space_ndc.z) > 1.0)
      return 0.0;

   if ((light_space_ndc.x * light_space_ndc.x) + (light_space_ndc.y * light_space_ndc.y) > 1.0)
	return 0.0;
 
   // Translate from NDC to shadow map space
   vec2 shadow_map_coord = light_space_ndc.xy * 0.5 + 0.5;
 
   // Check if the sample is in the light or in the shadow
   //if (light_space_ndc.z > texture(shadow_map, shadow_map_coord.xy).x)
   //   return 0.0; // In the shadow
 
   // In the light
   //return 1.0;

   float offset = 0.002;

   return (float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x, shadow_map_coord.y)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x - offset, shadow_map_coord.y - offset)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x - offset, shadow_map_coord.y         )).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x - offset, shadow_map_coord.y + offset)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x + offset, shadow_map_coord.y - offset)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x + offset, shadow_map_coord.y         )).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x + offset, shadow_map_coord.y + offset)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x         , shadow_map_coord.y - offset)).x) +
   float(light_space_ndc.z < texture(shadow_map, vec2(shadow_map_coord.x         , shadow_map_coord.y + offset)).x)) / 9;