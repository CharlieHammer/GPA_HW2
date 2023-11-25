#version 420

layout (binding = 0) uniform sampler2D tex;
layout (binding = 1) uniform sampler2D noise_tex;
uniform float barPos;
uniform int filterType;
uniform float offset;
uniform vec2 iMouse;
	                                                                               
out vec4 fragColor;                                                                
	                                                                               
in vec2 texcoord;                                                                                                                                   

vec3 GaussianBlur(vec2 fragCoord, float Quality){
	float Pi = 6.28318530718; // Pi*2
    
    // GAUSSIAN BLUR SETTINGS {{{
    float Directions = 16.0; // BLUR DIRECTIONS (Default 16.0 - More is better but slower)
    float Size = 0.005; // BLUR SIZE (Radius)
    // GAUSSIAN BLUR SETTINGS }}}
    
    // Pixel colour
    vec3 fragcolor = texture(tex, fragCoord).rgb;
    
    // Blur calculations
    for( float d=0.0; d<Pi; d+=Pi/Directions)
    {
		for(float i=1.0/Quality; i<1.001; i+=1.0/Quality)
        {
			fragcolor += texture( tex, fragCoord+vec2(cos(d),sin(d))*Size*i).rgb;		
        }
    }
    
    // Output to screen
    fragcolor /= Quality * Directions + 1.0;
    return fragcolor;
}

vec3 quantizeCC(vec3 col, int size, float maximum, float cc){

    
    float increment = maximum / float(size);
    
    vec3 quantized;
    
    for(float i = 0.0; i < float(size); i++){
        if(col.r > i * increment){
            quantized.r = i * increment;
        }
    }
    
    for(float i = 0.0; i < float(size); i++){
        if(col.g > i * increment){
            quantized.g = i * increment;
        }
    }
    
    for(float i = 0.0; i < float(size); i++){
        if(col.b > i * increment){
            quantized.b = i * increment;
        }
    }
    
    
    
    float avg = (quantized.r + quantized.g + quantized.b) / 3.0;
   
   
    // UNCOMMENT FOR SOMETHING COMPLETELY DIFFERENT //
    //quantized *= 1.0 - avg;
    
    
    quantized -= cc * (quantized - avg) * abs(quantized - avg);
    
    return quantized;
    
}

vec3 XDOG(vec2 texcoord){

    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    const float Epsilon = 0.014;
    const float p = 0.97;
    const float Phi = 200.0;

    vec3 blur1 = GaussianBlur(texcoord, 4.0);
    vec3 blur2 = GaussianBlur(texcoord, 2.0);

    blur1 = vec3((blur1 * W).g);
    blur2 = vec3((blur2 * W).g);

    float diff = blur1.r  - p * blur2.r;

    if (diff < Epsilon) {
       diff = 0.0;
    } else {
       diff = 0.5 + tanh(Phi * diff);
    }
    return vec3(diff);
}

vec3 WaterColor(vec2 texcoord)
{
    vec3 blur_color = GaussianBlur(texcoord, 2.0f);
    vec3 noise_color = texture(noise_tex, texcoord).rgb;
    vec2 new_coord = texcoord + noise_color.rg;
    noise_color = texture(noise_tex, new_coord).rgb;
    vec3 quantized_color = quantizeCC((blur_color * 0.8 + noise_color * 0.2), 7, 0.8, 1.0);
    return quantized_color;
}

vec3 Magnifier(vec2 texcoord, vec2 mousecoord){
    const float SqrSize = 0.01;
    const float SqrBorderSize = 0.002;
    const float Zoom = 1.5;

     // Normalized pixel coordinates (from 0 to 1)
    vec2 iResolution = textureSize(tex, 0);
    
    vec2 dis = mousecoord - texcoord; // vector about distantion from uv to mousePoint
    vec2 dist = dis; // save vector without change aspect ratio
    dis.y *= iResolution.y / iResolution.x; // to be circle, not ellipse

    float sqrMagitude = dot(dis,dis);
    
    //flag is uv inside the magnifier
    float magnifierFlag = step(sqrMagitude, SqrSize);
    
    texcoord += (1. - 1./Zoom) * dist * magnifierFlag; //the main magic ;)
    
    
    //flag of white border
    float border = smoothstep(SqrBorderSize, 0., abs(sqrMagitude - SqrSize));
    
    
    return max(texture(tex, texcoord), border).rgb; // drow border;
}

vec3 Bloom(vec2 texcoord){
    vec3 blur1 = GaussianBlur(texcoord, 3.0f);
    vec3 blur2 = GaussianBlur(texcoord, 2.0f);
    vec3 col = texture(tex, texcoord).rgb;
    return col * 0.4 + blur1 * 0.3 + blur2 * 0.3;
}

vec3 Pixelization(vec2 texcoord, int pixelSize){
    vec2 texelcoord = texcoord * textureSize(tex, 0);
    vec2 quantizedcoord = floor(texelcoord / pixelSize) * pixelSize;
    vec3 pixel_color = texture(tex, quantizedcoord / textureSize(tex, 0)).rgb;
    return pixel_color;
}

vec3 SineWave(vec2 texcoord){  

    
    float f = 10.0;
    float a = 10.0;
    
	texcoord.x += 0.8 * sin(texcoord.y * f + offset * 0.002) / a;

	vec3 sinewave_color = texture(tex, texcoord).rgb;
	
	return sinewave_color;
}
				 
void main(void)                                                                
{   
    if(filterType == 3){
        vec3 magnifier_color = Magnifier(texcoord, iMouse);
        fragColor = vec4(magnifier_color, 1.0f);
    } else {
        if(texcoord.x > barPos){
            switch(filterType){
                case 1:
                    vec3 col = texture(tex, texcoord).rgb;
                    vec3 blur_color = GaussianBlur(texcoord, 3.0f);
                    vec3 quantized_color = quantizeCC(col, 4, 0.8, 0.1); 
                    vec3 XDOG_color = XDOG(texcoord);
                    quantized_color = (quantized_color + blur_color) / 2;
                    //fragColor = vec4(quantized_color, 1.0);
                    fragColor  = vec4((quantized_color * 0.8 + XDOG_color * 0.2), 1.0);
                    break;
                case 2:
                    vec3 water_color = WaterColor(texcoord);
                    fragColor = vec4(water_color, 1.0f);
                    break;
                case 4:
                    vec3 bloom_color = Bloom(texcoord);
                    fragColor = vec4(bloom_color, 1.0f);
                    break;
                case 5:
                    vec3 pixel_color = Pixelization(texcoord, 8);
                    fragColor = vec4(pixel_color, 1.0f);
                    break;
                case 6:
                    vec3 sinewave_color = SineWave(texcoord);
                    fragColor = vec4(sinewave_color, 1.0f);
                    break;
                default:
                    fragColor = texture(tex, texcoord);
                    break;
            }
        } else {
            fragColor = texture(tex, texcoord);
        }
    }
    
    
} 
