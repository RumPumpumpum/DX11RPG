#pragma once

#include "AppBase.h"
#include "SkinnedMeshModel.h"

// �⺻ ������ SnippetHelloWorld.cpp
// ������ ������ SnippetHelloWorldRender.cpp

#define PX_RELEASE(x)                                                          \
    if (x) {                                                                   \
        x->release();                                                          \
        x = NULL;                                                              \
    }

#define PVD_HOST "127.0.0.1"
#define MAX_NUM_ACTOR_SHAPES 128


class Game : public AppBase {
public:
	Game();

	~Game();

	bool InitScene() override;

	void UpdateLights(float dt) override;
	void UpdateGUI() override;
	void Update(float dt) override;
	void Render() override;

public:
	shared_ptr<SkinnedMeshModel> m_character;
};

