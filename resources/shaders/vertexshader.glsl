uniform mat4 mvp_matrix;

attribute vec3 a_position;
attribute vec2 a_texcoord;

varying vec2 v_texcoord;

void main()
{
    gl_Position = mvp_matrix * vec4(a_position, 1);
    v_texcoord = a_texcoord;
}
