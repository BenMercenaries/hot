/*

             *
            ***
           *****
   *********************       Mercenaries Engineering SARL
     *****************              Copyright (C) 2017
       *************
         *********        http://www.mercenaries-engineering.com
        ***********
       ****     ****
      **           **

*/

#include <memory>

#include <guerilla/shade_op.h>
#include <guerilla/hasher.h>

#include "../Ocean.h"

using namespace GAPI;

// ****************************************************************************

class OceanData
{
public:
	std::unique_ptr<drw::Ocean>			Ocean;
	std::unique_ptr<drw::OceanContext>	OceanContext;
};

// ****************************************************************************

using TOceanMap = std::map<std::string, std::shared_ptr<OceanData>>;

thread_local TOceanMap	LocalOceanMap;
std::mutex	OceanMapMutex;
TOceanMap	OceanMap;

// ****************************************************************************

static std::shared_ptr<OceanData>	getOcean (
	int resolution,
	float s,
	float V,
	float w,
	float l,
	float A,
	int seed,
	bool chop_toggle,
	float chop,
	float damp,
	float wind_align,
	float ocean_depth,
	float time,
	bool interp_toggle,
	bool normals_toggle,
	bool jacobian_toggle
	)
{
	GAPI::Hasher	hr;

	hr.hash (resolution, s, V, w, l, A, seed);
	hr.hash (chop_toggle, chop, damp, wind_align);
	hr.hash (ocean_depth, time, interp_toggle, normals_toggle, jacobian_toggle);

	bool do_chop     = chop_toggle;
	bool do_jacobian = jacobian_toggle;
	bool do_normals  = normals_toggle && !do_chop;

	int gridres  = 1 << int (resolution);
	float stepsize = s / resolution;

	std::string	hash;
	hr.finish (hash);

	auto	it = LocalOceanMap.find (hash);
	if (it == LocalOceanMap.end ())
	{
		std::lock_guard<std::mutex> guard (OceanMapMutex);
		it = OceanMap.find (hash);
		if (it == OceanMap.end ())
		{
			std::shared_ptr<OceanData>	p = std::make_shared<OceanData> ();

			p->Ocean = std::unique_ptr<drw::Ocean> (new drw::Ocean (
				gridres, gridres, stepsize, stepsize,
				V, l, 1.0, w, 1.0f-damp, wind_align,
				ocean_depth, seed));

			float	ocean_scale = p->Ocean->get_height_normalize_factor ();

			p->OceanContext	= std::unique_ptr<drw::OceanContext> (p->Ocean->new_context (true,
				do_chop, do_normals, do_jacobian));

			p->Ocean->update (time, *(p->OceanContext), true,
				do_chop, do_normals, do_jacobian,
				ocean_scale * A, chop),

			it = OceanMap.insert (TOceanMap::value_type (hash, p)).first;
		}
		LocalOceanMap[hash]= it->second;
/*
		std::shared_ptr<OceanData>	p = std::make_shared<OceanData> ();

		p->Ocean = std::unique_ptr<drw::Ocean> (new drw::Ocean (
			gridres, gridres, stepsize, stepsize,
			V, l, 1.0, w, 1.0f-damp, wind_align,
			ocean_depth, seed));

		float	ocean_scale = p->Ocean->get_height_normalize_factor ();

		p->OceanContext	= std::unique_ptr<drw::OceanContext> (p->Ocean->new_context (true,
			do_chop, do_normals, do_jacobian));

		p->Ocean->update (time, *(p->OceanContext), true,
			do_chop, do_normals, do_jacobian,
			ocean_scale * l, chop),

		it = LocalOceanMap.insert (TOceanMap::value_type (hash, p)).first;
*/
	}
	return it->second;
}

// ****************************************************************************

static void	hot (
	GAPI::ShadingCtx *ctx,
	float *result,
	float *P,
	float *resolution,
	float *s,
	float *V,
	float *w,
	float *l,
	float *A,
	float *seed,
	float *chop_toggle,
	float *chop,
	float *damp,
	float *wind_align,
	float *ocean_depth,
	float *time,
	float *interp_toggle,
	float *normals_toggle,
	float *jacobian_toggle
	)
{
	std::shared_ptr<OceanData>	ocean = getOcean (
		int (*resolution),
		*s,
		*V,
		*w,
		*l,
		*A,
		int (*seed),
		*chop_toggle != 0.0f,
		*chop,
		*damp,
		*wind_align,
		*ocean_depth,
		*time,
		*interp_toggle != 0.0f,
		*normals_toggle != 0.0f,
		*jacobian_toggle != 0.0f
		);

	bool	do_chop = chop_toggle;
	bool	do_jacobian = jacobian_toggle;
	bool	do_normals = normals_toggle && !do_chop;

	bool	linterp = (*interp_toggle == 0.0f);

	drw::OceanContext::Outputs	outputs;

	unsigned	stride = ctx->stride ();
	for (unsigned i = 0; i < ctx->count (); ++i)
	{
		float	x = P[i+stride*0];
		float	y = P[i+stride*1];
		float	z = P[i+stride*2];

		if (linterp)
			ocean->OceanContext->eval_xz (x, z, outputs);
		else
			ocean->OceanContext->eval2_xz (x, z, outputs);

		if (do_chop)
		{
			x += outputs.disp[0];
			y += outputs.disp[1];
			z += outputs.disp[2];
		}
		else
		{
			y += outputs.disp[1];
		}

		result[i+stride*0] = x;
		result[i+stride*1] = y;
		result[i+stride*2] = z;
	}
}

// ****************************************************************************
// The plugin module entry point
// ****************************************************************************

extern "C" GUERILLA_PLUGIN_EXPORT	int	init (Guerilla *guerilla, int APIVersion, const char *APIDigest)
{
	CHECK_GUERILLA_API (APIVersion,APIDigest);

	GAPI::registerShadingOp (new GAPI::ShadingOp (
		"hot", (void*)hot,
		GAPI::ShadingOp::Point (),			// return: varying float
		{
			GAPI::ShadingOp::Point (0.0f, 0.0f, 0.0f),		// P: varying point
			GAPI::ShadingOp::Float (8.0f, GAPI_UNIFORM),	// resolution: uniform float
			GAPI::ShadingOp::Float (200.0f, GAPI_UNIFORM),	// s ocean size: uniform float
			GAPI::ShadingOp::Float (30.0f, GAPI_UNIFORM),	// V wind speed: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// w wind direction: uniform float
			GAPI::ShadingOp::Float (1.0f, GAPI_UNIFORM),	// l shortest wavelength: uniform float
			GAPI::ShadingOp::Float (1.0f, GAPI_UNIFORM),	// A approx wavelength: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// seed: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// chop_toggle: uniform float
			GAPI::ShadingOp::Float (1.0f, GAPI_UNIFORM),	// chop: uniform float
			GAPI::ShadingOp::Float (1.0f, GAPI_UNIFORM),	// damp: uniform float
			GAPI::ShadingOp::Float (2.0f, GAPI_UNIFORM),	// wind_align: uniform float
			GAPI::ShadingOp::Float (200.0f, GAPI_UNIFORM),	// ocean_depth: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// time: uniform float
			GAPI::ShadingOp::Float (1.0f, GAPI_UNIFORM),	// interp_toggle: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// normals_toggle: uniform float
			GAPI::ShadingOp::Float (0.0f, GAPI_UNIFORM),	// jacobian_toggle: uniform float
		}));

	return 0;
}

// ****************************************************************************

extern "C" GUERILLA_PLUGIN_EXPORT	int	release (Guerilla *guerilla)
{
	return 0;
}

// ****************************************************************************
