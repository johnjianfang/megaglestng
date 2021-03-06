// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_TILESET_MODEL_TYPE_H_
#define _GLEST_GAME_TILESET_MODEL_TYPE_H_

#ifdef WIN32
    #include <winsock2.h>
    #include <winsock.h>
#endif

#include <vector>
#include "model.h"
#include "vec.h"
#include "leak_dumper.h"
#include "unit_particle_type.h"

using std::vector;

namespace Glest{ namespace Game{

using Shared::Graphics::Model;
using Shared::Graphics::Vec3f;

// =====================================================
// 	class ObjectType  
//
///	Each of the possible objects of the map: trees, stones ...
// =====================================================

typedef vector<ObjectParticleSystemType*> ModelParticleSystemTypes;

class TilesetModelType {
private:
	Model *model;
	ModelParticleSystemTypes particleTypes;
	int height;
	bool rotationAllowed;
	bool smoothTwoFrameAnim;

    int animSpeed;

public:
	TilesetModelType();
	~TilesetModelType();

	void addParticleSystem(ObjectParticleSystemType *particleSystem);
	inline bool hasParticles()	const		{return particleTypes.empty() == false;}
	inline ModelParticleSystemTypes* getParticleTypes()  { return &particleTypes ;}


	inline Model * getModel() const		{return model;}
	inline void setModel(Model *model) 	{this->model=model;}

	inline int getHeight() const			{return height;}
	inline void setHeight(int height) 			{this->height=height;}

	inline bool getRotationAllowed() const			{return rotationAllowed;}
	inline void setRotationAllowed(bool rotationAllowed)	{this->rotationAllowed=rotationAllowed;}

	inline bool getSmoothTwoFrameAnim() const			{return smoothTwoFrameAnim;}
	inline void setSmoothTwoFrameAnim(bool smoothTwoFrameAnim)	{this->smoothTwoFrameAnim=smoothTwoFrameAnim;}

	inline int getAnimSpeed() const			{return animSpeed;}
	inline void setAnimSpeed(int value) {animSpeed = value;}

//	inline int getAnimSpeedVariation() const			{return animVariation;}
//	inline void setAnimSpeedVariation(int value) {animSpeed = valueVariation;}
};

}}//end namespace

#endif
