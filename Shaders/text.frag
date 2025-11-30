#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uTextColor;

void main()
{
    float alpha = texture(uTexture, TexCoord).r;
    FragColor = vec4(uTextColor.rgb, uTextColor.a * alpha);
}
