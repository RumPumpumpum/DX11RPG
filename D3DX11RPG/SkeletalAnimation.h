#pragma once

#include <algorithm>
#include <directxtk/SimpleMath.h>
#include <iostream>
#include <memory>

#include "AnimationClip.h"
#include "AppBase.h"
#include "GeometryGenerator.h"
#include "ImageFilter.h"
#include "Model.h"
#include "SkinnedMeshModel.h"


using DirectX::BoundingSphere;
using DirectX::SimpleMath::Vector3;

class SkeletalAnimation : public AppBase {
public:
	SkeletalAnimation();

	virtual bool InitScene() override;

	void UpdateLights(float dt) override;
	void UpdateGUI() override;
	void Update(float dt) override;
	void Render() override;

protected:
	shared_ptr<Model> m_ground;

	shared_ptr<SkinnedMeshModel> m_characterMeshModel;
};

