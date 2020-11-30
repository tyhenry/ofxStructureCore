#pragma once
#include "ofMain.h"

namespace ofx {
namespace structure {

	const std::string map_range_vert_shader = R"(
	#version 150
	uniform mat4 textureMatrix;
	uniform mat4 modelViewProjectionMatrix;

	in vec4 position;
	in vec2 texcoord; 
	out vec2 vTexcoord;

	void main()
	{
		vTexcoord	= (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;
		gl_Position	= modelViewProjectionMatrix * position;
	}
	)";

	const std::string map_range_frag_shader = R"(
	#version 150
	in vec2 vTexcoord;
	out vec4 fragColor;

	uniform vec4 inMin = vec4(0.0);
	uniform vec4 inMax = vec4(1.0);
	uniform vec4 outMin = vec4(0.0);
	uniform vec4 outMax = vec4(1.0);

	uniform sampler2DRect tex0;

	void main()
	{
		vec4 color = texture( tex0, vTexcoord );
		fragColor = (color - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
	}
	)";

	const std::string ir_vert_shader = R"(
	#version 150
	uniform mat4 textureMatrix;
	uniform mat4 modelViewProjectionMatrix;

	in vec4 position;
	in vec2 texcoord; 
	out vec2 vTexcoord;

	void main()
	{
		vTexcoord	= (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;
		gl_Position	= modelViewProjectionMatrix * position;
	}
	)";

	const std::string ir_frag_shader = R"(
	#version 150
	in vec2 vTexcoord;
	out vec4 fragColor;

	uniform sampler2DRect tex0;

	void main()
	{
		vec4 color = texture( tex0, vTexcoord );
		fragColor = vec4( vec3( color.rrr / 1023. ), 1. );
	}
	)";

	const std::string depth_to_points_vert_shader = R"(
	#version 150

	// -----------------------------------------------------------------------
	// point cloud from depth image
	// * transform vertices from depth image
	// * uses camera instrinsics cx, cy, fx, fy (as vec2 uC, uF)
	// -----------------------------------------------------------------------

	// oF input

	in vec4  position;
	uniform mat4 modelViewMatrix;
	uniform mat4 modelViewProjectionMatrix;

	// custom input

	uniform sampler2DRect uDepthTex;		// depth data - GL_R16 / unsigned short millimeters
	uniform ivec2 uDepthDims;				// texture dims
	uniform vec2 uC, uF;					// camera intrinsics: cx,cy,fx,fy

	out vec3 vPosition;
	out vec2 vTexCoord;
	flat out int vValid;

	int is_true(float b){
		return int(abs(sign(b)));	// 1 if b != 0
	}
	int is_false(float b){
		return 1-is_true(b);	// 1 if b == 0
	}

	void main()
	{
		// our texture coordinate in the depth frame
		vTexCoord	= vec2(gl_VertexID % uDepthDims.x, gl_VertexID / uDepthDims.x);

		float depth	= texture(uDepthTex, vTexCoord).r; // * 65535.0;		// Remap 0-1 to float range (millimeters)

		// project depth image into metric space using depth cam intrinsics
		// see: http://nicolas.burrus.name/index.php/Research/KinectCalibration
		vec2 ray = ( vTexCoord - uC ) / uF;
		vec3 pos = vec3( ray, 1. ) * depth;

		// flag invalid data (0 depth)
		vValid = is_true( depth );						// 1 (valid) or 0
	
		// if invalid, this sets posWorld.xyz to (ray.xy, 0):
		pos		*= is_true( vValid );					// zeroes xyz if invalid
		pos.xy	+= ray.xy * is_false( vValid );			// sets xy to ray.xy if invalid

		// Flip XY as OpenGL has different conventions on which direction is positive.
		pos.xy *= -1;

		// our position output, in camera space (mm, world space with kinect as origin)
		vPosition =  pos;	// modelViewMatrix * pos would allow changing depth camera transform 

		gl_Position = modelViewProjectionMatrix * vec4(pos,1.);		// for rendering
	}

)";
}  // namespace structure
}  // namespace ofx
