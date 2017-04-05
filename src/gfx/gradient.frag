#version 150

uniform float posViewportTop;
uniform vec2 posTopLeft;
uniform vec2 posBottomRight;
uniform int nGradientDirection;
uniform int nGradientSize;
uniform int nGradientShape;
uniform int nStopCount;
uniform vec2 posGradientCentre;
uniform sampler2DRect texStopPositions;
uniform sampler2DRect texStopColours;
in vec4 Color;
out vec4 FragColor;

vec4 gradient_colour(in float n)
{
	int l = 0;
	int r = nStopCount - 1;
	int found = -1;
	float pos = 0.0;
	if (n < 0.0)
		n = 0.0;
	if (n > 1.0)
		n = 1.0;
	while(found == -1)
	{
		int m = (l + r) / 2;
		pos = texelFetch(texStopPositions, ivec2(m, 0)).r;
		if (l > r)
			found = r;
		else
		{
			if (pos < n)
				l = m + 1;
			else if (pos > n)
				r = m - 1;
			else
				found = m;
		}
	}
	if (pos >= n && found != 0)
		--found;
	float firstPos = texelFetch(texStopPositions, ivec2(found, 0)).r;
	float secondPos = texelFetch(texStopPositions, ivec2(found + 1, 0)).r;
	vec4 firstColour = texelFetch(texStopColours, ivec2(found, 0));
	vec4 secondColour = texelFetch(texStopColours, ivec2(found + 1, 0));
	return mix(firstColour, secondColour, (n - firstPos) / (secondPos - firstPos));
}

float ellipse_radius(float cx, float cy, float angle)
{
	return cx * cy / sqrt(cx * cx * sin(angle) * sin(angle) + cy * cy * cos(angle) * cos(angle));
}

void main()
{
	vec2 viewPos = gl_FragCoord.xy;
	viewPos.y = posViewportTop - viewPos.y;
	float gradientPos;
	if (nGradientDirection == 0) /* vertical */
		gradientPos = (viewPos.y - posTopLeft.y) / (posBottomRight.y - posTopLeft.y);
	else if (nGradientDirection == 1) /* horizontal */
		gradientPos = (viewPos.x - posTopLeft.x) / (posBottomRight.x - posTopLeft.x);
	else if (nGradientDirection == 2) /* diagonal - todo */
		gradientPos = (viewPos.x - posTopLeft.x) / (posBottomRight.x - posTopLeft.x);
	else /* radial */
	{
	    vec2 pos = viewPos - posTopLeft;
		vec2 s = posBottomRight - posTopLeft;
		vec2 centre = s / 2.0 * (posGradientCentre + vec2(1.0, 1.0));
		float d = distance(centre, pos);
		vec2 c1 = posTopLeft - posTopLeft;
		vec2 c2 = vec2(posTopLeft.x, posBottomRight.y) - posTopLeft;
		vec2 c3 = posBottomRight - posTopLeft;
		vec2 c4 = vec2(posBottomRight.x, posTopLeft.y) - posTopLeft;
		vec2 nc = c1;
		if (distance(centre, c2) < distance(centre, nc))
			nc = c2;
		if (distance(centre, c3) < distance(centre, nc))
			nc = c3;
		if (distance(centre, c4) < distance(centre, nc))
			nc = c4; 
		vec2 fc = c1;
		if (distance(centre, c2) > distance(centre, fc))
			fc = c2;
		if (distance(centre, c3) > distance(centre, fc))
			fc = c3;
		if (distance(centre, c4) > distance(centre, fc))
			fc = c4;
		float r;
		if (nGradientShape == 0)
		{
			switch(nGradientSize)
			{
			default:
			case 0:
				r = ellipse_radius(min(centre.x, s.x - centre.x), min(centre.y, s.y - centre.y), atan(pos.y - centre.y, pos.x - centre.x));
				break;
			case 1:
				r = ellipse_radius(max(centre.x, s.x - centre.x), max(centre.y, s.y - centre.y), atan(pos.y - centre.y, pos.x - centre.x));
				break;
			case 2:
				r = ellipse_radius(abs(centre.x - nc.x), abs(centre.y - nc.y), atan(pos.y - centre.y, pos.x - centre.x));
				break;
			case 3:
				r = ellipse_radius(abs(centre.x - fc.x), abs(centre.y - fc.y), atan(pos.y - centre.y, pos.x - centre.x));
				break;
			}
		}
		else
		{
			switch(nGradientSize)
			{
			default:
			case 0:
				r = min(centre.x, min(centre.y, min(s.x - centre.x, s.y - centre.y)));
				break;
			case 1:
				r = max(centre.x, max(centre.y, max(s.x - centre.x, s.y - centre.y)));
				break;
			case 2:
				r = distance(nc, centre);
				break;
			case 3:
				r = distance(fc, centre);
				break;
			}
		}
		if (d < r)
			gradientPos = d/r;
		else
			gradientPos = 1.0;
	}

	FragColor = gradient_colour(gradientPos);
}
