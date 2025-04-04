/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "WaterRendering.h"

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/myGL.h"
#include "RefractWater.h"
#include "Map/MapInfo.h"
#include "Map/ReadMap.h"

#include <bit>

#include "System/Misc/TracyDefs.h"

void CRefractWater::InitResources(bool loadShader)
{
	RECOIL_DETAILED_TRACY_ZONE;
	CAdvWater::InitResources(false);
	LoadGfx();
}

void CRefractWater::FreeResources()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (subSurfaceTex) {
		glDeleteTextures(1, &subSurfaceTex);
		subSurfaceTex = 0;
	}
}

void CRefractWater::LoadGfx()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// valid because GL_TEXTURE_RECTANGLE_ARB = GL_TEXTURE_RECTANGLE_EXT
	if (GLAD_GL_ARB_texture_rectangle) {
		target = GL_TEXTURE_RECTANGLE_ARB;
	} else {
		target = GL_TEXTURE_2D;
	}

	glGenTextures(1, &subSurfaceTex);
	glBindTexture(target, subSurfaceTex);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	if (target == GL_TEXTURE_RECTANGLE_ARB) {
		glTexImage2D(target, 0, 3, globalRendering->viewSizeX, globalRendering->viewSizeY, 0, GL_RGB, GL_INT, 0);
		waterFP = LoadFragmentProgram("ARB/waterRefractTR.fp");
	} else {
		glTexImage2D(target, 0, 3, std::bit_ceil <uint32_t> (globalRendering->viewSizeX), std::bit_ceil <uint32_t> (globalRendering->viewSizeY), 0, GL_RGB, GL_INT, 0);
		waterFP = LoadFragmentProgram("ARB/waterRefractT2D.fp");
	}
}

void CRefractWater::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!waterRendering->forceRendering && !readMap->HasVisibleWater())
		return;

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glBindTexture(target, subSurfaceTex);
	glEnable(target);
	glCopyTexSubImage2D(target, 0, 0, 0, globalRendering->viewPosX, globalRendering->viewPosY, globalRendering->viewSizeX, globalRendering->viewSizeY);

	SetupWaterDepthTex();

	glActiveTextureARB(GL_TEXTURE0_ARB);

	// GL_TEXTURE_RECTANGLE uses texcoord range 0 to width, whereas GL_TEXTURE_2D uses 0 to 1
	if (target == GL_TEXTURE_RECTANGLE_ARB) {
		float v[] = { 10.0f * globalRendering->viewSizeX, 10.0f * globalRendering->viewSizeY, 0.0f, 0.0f };
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, v);
	} else {
		float v[] = { 10.0f, 10.0f, 0.0f, 0.0f };
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 2, v);
		v[0] = 1.0f / std::bit_ceil <uint32_t> (globalRendering->viewSizeX);
		v[1] = 1.0f / std::bit_ceil <uint32_t> (globalRendering->viewSizeY);
		glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 3, v);
	}
	CAdvWater::Draw(false);

	glActiveTextureARB(GL_TEXTURE3_ARB);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);

	glActiveTextureARB(GL_TEXTURE2_ARB);
	glDisable(target);
	glActiveTextureARB(GL_TEXTURE0_ARB);
}

void CRefractWater::SetupWaterDepthTex()
{
	RECOIL_DETAILED_TRACY_ZONE;
	glActiveTextureARB(GL_TEXTURE3_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, readMap->GetShadingTexture()); // the shading texture has water depth encoded in alpha
	glEnable(GL_TEXTURE_GEN_S);
	float splane[] = { 1.0f / (mapDims.mapxp1 * SQUARE_SIZE), 0.0f, 0.0f, 0.0f };
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
	glTexGenfv(GL_S,GL_OBJECT_PLANE,splane);

	glEnable(GL_TEXTURE_GEN_T);
	float tplane[] = { 0.0f, 0.0f, 1.0f / (mapDims.mapyp1 * SQUARE_SIZE), 0.0f};
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
	glTexGenfv(GL_T,GL_OBJECT_PLANE,tplane);
}
