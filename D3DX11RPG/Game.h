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
#include "Character.h"



using DirectX::BoundingSphere;
using DirectX::SimpleMath::Vector3;

class Game : public AppBase {
public:
	Game();

	virtual bool InitScene() override;

	void UpdateLights(float dt) override;
	void UpdateGUI() override;
	void Update(float dt) override;
	void Render() override;

protected:
	shared_ptr<Model> m_ground;
	Character character;
	shared_ptr<SkinnedMeshModel> m_characterMeshModel;
};

