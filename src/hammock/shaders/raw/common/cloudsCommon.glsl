#ifndef CLOUDS_COMMON
#define CLOUDS_COMMON

// MATH
#define PI 3.14159265359
#define THREE_OVER_SIXTEEN_PI 0.05968310365946075
#define ONE_OVER_FOUR_PI 0.07957747154594767
#define E 2.718281828459

// EARTH
#define EARTH_RADIUS 6371000.0 // earth's actual radius in km = 6371
#define ATMOSPHERE_RADIUS_INNER (EARTH_RADIUS + 7500.0) //paper suggests values of 15000-35000m above
#define ATMOSPHERE_RADIUS_OUTER (EARTH_RADIUS + 20000.0)
#define ATMOSPHERE_THICKNESS (ATMOSPHERE_RADIUS_OUTER - ATMOSPHERE_RADIUS_INNER)

// WIND
#define WIND_DIRECTION vec3(1.0,0.0,0.0)
#define CLOUD_SPEED 0.080
#define CLOUD_TOP_OFFSET 1.0// this offset pushes the tops of the clouds along this wind direction by this many units

// SUN
#define SUN_LOCATION vec3(0.0, ATMOSPHERE_RADIUS_OUTER * 0.9, -ATMOSPHERE_RADIUS_OUTER * 0.9)
#define BACKGROUND_SKY_SUN_LOCATION vec3(0.0, EARTH_RADIUS * 2.0, -EARTH_RADIUS * 10.0)
#define SUN_COLOR vec3(1.0, 1.0, 1.0)
#define SUN_INTENSITY 0.780

// COLORS
#define BLACK vec3(0,0,0)
#define BLUE vec3(0.529, 0.808, 0.922)
#define WHITE vec3(1,1,1)
#define RED vec3(1,0,0)

// POST
#define NUM_MOTION_BLUR_SAMPLES 10

// TOOLBOX

//This bit of code is for converting a 32 bit float to store as 4 8 bit floats
const vec4 bitEnc = vec4(1.,255.,65025.,16581375.);
vec4 EncodeFloatRGBA (float v) {
    vec4 enc = bitEnc * v;
    enc = fract(enc);
    enc -= enc.yzww * vec2(1./255., 0.).xxxy;
    return enc;
}

// Remaps value from one range to another
float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

// remap clamped after
float remapc(in float value, in float original_min, in float original_max, in float new_min, in float new_max)
{
    float t = new_min + ( ((value - original_min) / (original_max - original_min)) * (new_max - new_min) );
    return clamp(t, new_min, new_max);
}

// remap clamped before and after
float remapcc(in float value, in float original_min, in float original_max, in float new_min, in float new_max)
{
    value = clamp(value, original_min, original_max);
    float t = new_min + ( ((value - original_min) / (original_max - original_min)) * (new_max - new_min) );
    return clamp(t, new_min, new_max);
}

float getRelativeAtmosphericHeight(in vec3 point, in vec3 earthCenter, in vec3 startPosOnInnerShell, in vec3 rayDir, in vec3 eye)
{
    float lengthOfRayfromCamera = length(point - eye);
    float lengthOfRayToInnerShell = length(startPosOnInnerShell - eye);
    vec3 pointToEarthDir = normalize(point - earthCenter);
    // assuming RayDir is normalised
    float cosTheta = dot(rayDir, pointToEarthDir);

    // CosTheta is an approximation whose error gets relatively big near the horizon and could lead to problems.
    // However, the actual calculationis involve a lot of trig and thats expensive;
    // No longer drawing clouds that close to the horizon and so the cosTheta Approximation is fine

    float numerator = abs(cosTheta * (lengthOfRayfromCamera - lengthOfRayToInnerShell));
    return numerator/ATMOSPHERE_THICKNESS;
    // return clamp( length(point.y - projectedPos.y) / ATMOSPHERE_THICKNESS, 0.0, 1.0);
}

vec3 getRelativeAtmosphericPosition(in vec3 pos, in vec3 earthCenter)
{
    return vec3( ( pos - vec3(earthCenter.x, ATMOSPHERE_RADIUS_INNER - EARTH_RADIUS, earthCenter.z) )/ ATMOSPHERE_THICKNESS );
}

// LIGHTING
/*
    Note:
        - Apply this result whenever you calculate radiance of your sample

    Functionality:
        - Combine 2 HG functions with max() to retain baseline forward scattering and achieve silver lining highlights

        eccentricity = 0.6

        silver_intensity = user controlled param [0, 1]
                            Controls intensity of the effect of using 2 HG functions and the spread away from the sun
                            Increase this to add intensity on clouds near the sun

        silver_spread = user contorlled param [0, 1]
                            Decrease this to increase brightness that's spread throughout clouds away from the sun
*/
float HenyeyGreenstein(float cos_angle, float eccentricity)
{
    float numerator =  1.0 - eccentricity * eccentricity;
    float denominator = pow((1.0 + eccentricity * eccentricity - 2.0 * eccentricity * cos_angle), 1.5);
    return (numerator / denominator) * ONE_OVER_FOUR_PI;
}

float HenyeyGreensteinArtDirected(float cos_angle, float eccentricity, float silver_intensity, float silver_spread)
{
    return max( HenyeyGreenstein(cos_angle, eccentricity),
                silver_intensity * HenyeyGreenstein(cos_angle, 0.99 - silver_spread) );
}


/*
    Note:
        - Only do this when you look away from sun
        - Ramp down this affect as angle b/w viewRay and lightRay decrease
        - attenuation_reduction_factor = 0.25;
    	- influence_reduction_factor = 0.7;

    Functionality:
        - Attenuation value for the second function was reduced to push light further into the cloud
        - Reduce its influence so to not overpower the result.
        - density_along_light_ray comes from cone sampling
*/

float BeerLambertModified(float density_along_light_ray, float attenuation_reduction_factor, float influence_reduction_factor)
{
    return max( exp(density_along_light_ray) ,
                exp(density_along_light_ray * attenuation_reduction_factor) * influence_reduction_factor );
}


/*
	Notes:
		- dl is the density sampled along the light ray for the given sample position.
		- ds_loded is the low lod sample of density at the given sample position.
*/
float GetLightEnergy(float height_fraction, float dl, float ds_loded, float phase_probability, float cos_angle, float step_size, float brightness)
{
    // Attenuation – difference from slides – reduce the secondary component when we look toward the sun.
    float primary_attenuation = exp(-dl);


    // NOTE: in the slides, seconary_attenuation was "secondary_intensity_curve", and primary_attenuation was "primary_intensity_curve". UNSURE IF SAME
    // FIRST INSTANCE
    // float secondary_attenuation = exp(-dl * 0.25) * 0.7;
//    float secondary_attenuation = exp(-dl);
//    float attenuation_probability = max(
//        remap(cos_angle, 0.7, 1.0, secondary_attenuation, secondary_attenuation * 0.25),
//        primary_attenuation);

    // --------------------------------------------------------------------------------------------------------------------

    // // SECOND INSTANCE -------> DARKER THAN THE FIRST INSTANCE
     float beerLambertModified = BeerLambertModified(-dl, 0.25, 0.7);
     float attenuation_probability = mix(primary_attenuation, beerLambertModified, -cos_angle * 0.5 + 0.5);


    // In-scattering – one difference from presentation slides – we also reduce this effect once light has attenuated to make it directional.

    // // FIRST INSTANCE -----> THIS PRODUCES MORE BANDING EFFECTS
    // float depth_probability = mix( 0.05 + pow(ds_loded,
    //                                           remap(height_fraction, 0.3, 0.85, 0.5, 2.0))
    //                              , 1.0, clamp( dl / step_size, 0.0, 1.0));

    // --------------------------------------------------------------------------------------------------------------------

    // // SECOND INSTANCE
    // float depth_probability = mix(0.05 + pow(ds_loded, clamp(
    // 														remap(height_fraction, 0.3, 0.85, 0.5, 2.0),
    // 														0.6, 2.0)),
    // 							1.0,
    // 							clamp(dl / step_size, 0.0, 1.0));


    // float vertical_probability = pow(clamp(
    // 										remap(height_fraction, 0.07, 0.14, 0.1, 1.0),
    // 										0.1, 1.0),
    // 								0.8 );


    // THIRD INSTANCE ------> LOOKS ESSENTIALLY SAME AS SECOND INSTANCE

    // MANIPULATE ME
    float depth_probability = 0.05 + pow(ds_loded, clamp(remap(height_fraction * 0.125, 0.3, 0.85, 0.5, 2.0), 0.5, 2.0));
    float vertical_probability = pow(clamp(remap(height_fraction * 1.5, 0.07, 0.34, 0.1, 1.0), 0.1, 1.0), 0.8);

    // MANIPULATE ME
    float in_scatter_probability = depth_probability * vertical_probability;

    float light_energy = attenuation_probability * in_scatter_probability * phase_probability * brightness;						// ORIGINAL (LIGHTEST)
    //float light_energy = attenuation_probability * primary_attenuation * in_scatter_probability * phase_probability * brightness;	// MEDIUM
    // float light_energy = primary_attenuation * secondary_attenuation * in_scatter_probability * phase_probability * brightness;	// DARKEST
    return light_energy;
}




#endif