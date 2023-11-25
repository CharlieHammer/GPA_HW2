#version 410

layout (location = 0) in vec2 position;                            
layout (location = 1) in vec2 texc;    

out vec2 texcoord;                                                         
                                                                   
                                                                   
void main(void)                                                    
{                                                                  
    gl_Position = vec4(position,0.0,1.0);							
    texcoord = texc;                                    
}