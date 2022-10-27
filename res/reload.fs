#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;           // Texture coordinates (sampler2D)
in vec4 fragColor;              // Tint color

// Output fragment color
out vec4 finalColor;            // Output fragment color

// Uniform inputs
uniform vec2 resolution;        // Viewport resolution (in pixels)
uniform vec2 mouse;             // Mouse pixel xy coordinates
uniform float time;             // Total run time (in secods)

uniform sampler2D dataset;

float opUnion( float d1, float d2 ) { return min(d1,d2); }

float opSubtraction( float d1, float d2 ) { return max(-d1,d2); }

float opIntersection( float d1, float d2 ) { return max(d1,d2); }

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float opSmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); }

vec2 sdRotate( in vec2 origin, in float rotation){
    float PI = 3.14159;
    float angle = rotation * (PI / 180);
    float sine = sin(angle), cosine = cos(angle);

    return vec2(
	cosine * origin.x + sine * origin.y, 
	cosine * origin.y - sine * origin.x);
}

float sdCircle( in vec2 p, in float r ) 
{
    return length(p)-r;
}

float sdBox( in vec2 p, in vec2 b )
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

vec2 toScreenCoord( in vec2 p ) 
{
	return p / resolution.y;
}

float toScreenCoord( in float s )
{
	return s / resolution.y;
}

vec2 indexToPos ( in vec2 datasize, in int index )
{
	vec2 pos = 
		vec2(index % int(datasize.x) + 0.5, floor(index / datasize.x) + 0.5)
			/
		datasize;
	pos.y = 1 - pos.y;

	return pos;
}

vec2 colorToVec2 ( in vec4 color )
{
	vec4 pos = vec4(
		color[0] * 255.0, color[1] * 255.0, 
		color[2] * 255.0, color[3] * 255.0
		);

	return vec2(
		int(pos[0]) | (int(pos[1]) << 8), 
		int(pos[2]) | (int(pos[3]) << 8)
	);
}

vec2 vec2FromDatablock ( in sampler2D data, in int index, in int shift ) {
	vec2 datasize = textureSize(data, 0);
	return colorToVec2( 
			texture2D( data, indexToPos(datasize, index + shift) )
		);
}

vec2 posFromDatablock ( in sampler2D data, in int index ) {
	vec2 pos = vec2FromDatablock( data, index, 2 );
	pos.y = resolution.y - pos.y;

	return pos;
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 datasize = textureSize(dataset, 0);

    float d = 65535;

    for (int i = 0; i < datasize.x * datasize.y; i++)
    {
	vec2 datapos = indexToPos(datasize, i);
	vec4 baseargs = texture2D(dataset, datapos);
	vec4 typeargs = texture2D(dataset, indexToPos(datasize, i + 1));

	int datablockSize = int(baseargs[0] * 255);
	
	if ( datablockSize == 0 ) { break; }

	// --- sdf entity

	int shapetype = int(typeargs[0] * 255);

	vec2 pos = posFromDatablock( dataset, i );
	vec2 size = toScreenCoord(vec2FromDatablock( dataset, i, 3 ));
	float rotation = vec2FromDatablock( dataset, i, 4 ).x;

	vec2 screenpos = toScreenCoord( fragCoord - pos );
	vec2 transformed = sdRotate( screenpos, rotation );
	
	float t = 0;
	if ( shapetype == 0 ) {
		t = sdCircle(transformed, size.x);
	} else if ( shapetype == 1 ) {
		t = sdBox(transformed, size);
	}
	d = opSmoothUnion(d, t, 0.01);

	// ---

	i += datablockSize;
    }


    // coloring
    vec3 colora = vec3(0.196,0.239,0.31);
    vec3 colorb = vec3(0.851,0.141,0.235);
    vec3 col = (d>0.0) ?  colora : colorb;
    //distance fade
    col *= 1.0 - exp(-6.0*abs(d));
    // onion radius
    col *= 0.6 + 0.4*cos(200.0*d);
    // outline
    col = mix( col, vec3(1.0, 1.0, 0.961), 1.0-smoothstep(0.001,0.003,abs(d)) );

    finalColor = vec4(col,1.0);
    //finalColor = vec4(texture2D(dataset, fragCoord / resolution).xyz, 1.0);
}
