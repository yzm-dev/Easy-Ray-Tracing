#version 420 core

void main()
{
    // Fullscreen triangle using gl_VertexID
    // verts: ( -1,-1 ), ( 3,-1 ), ( -1, 3 ) to cover the screen
    vec2 pos = vec2(
        (gl_VertexID == 0) ? -1.0 : ((gl_VertexID == 1) ? 3.0 : -1.0),
        (gl_VertexID == 0) ? -1.0 : ((gl_VertexID == 1) ? -1.0 : 3.0)
    );
    gl_Position = vec4(pos, 0.0, 1.0);
}