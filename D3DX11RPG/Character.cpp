#include "Character.h"


Character::Character()
{

}

void Character::CharacterInit(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context)
{
	m_device = device;
	m_context = context;

	string path = "../Assets/Characters/";
	vector<string> clipNames = {
		"Idle.fbx", "IdleToWalkForward.fbx",
		"WalkForward.fbx", "WalkStop.fbx" };

	AnimationData aniData;

	auto [meshes, _] =
		GeometryGenerator::ReadAnimationFromFile(path, "Character.fbx");

	for (auto& name : clipNames) {
		auto [_, ani] =
			GeometryGenerator::ReadAnimationFromFile(path, name);

		if (aniData.clips.empty()) {
			aniData = ani;
		}
		else {
			aniData.clips.push_back(ani.clips.front());
		}
	}

	Vector3 center(0.0f, 0.5f, 2.0f);	// 캐릭터가 생성될 곳
	m_characterMeshModel =
		make_shared<SkinnedMeshModel>(m_device, m_context, meshes, aniData);
	m_characterMeshModel->m_materialConsts.GetCpu().albedoFactor = Vector3(1.0f);
	m_characterMeshModel->m_materialConsts.GetCpu().roughnessFactor = 0.8f;
	m_characterMeshModel->m_materialConsts.GetCpu().metallicFactor = 0.0f;
	m_characterMeshModel->UpdateWorldRow(Matrix::CreateScale(1.0f) *
		Matrix::CreateTranslation(center));
}

void Character::Move(float dt, bool keyPressed[]) {
	static int frameCount = 0;

	// States
	// 0: idle
	// 1: idle to walk
	// 2: walk forward
	// 3: walk to stop

	static int state = 0;

	// 간단한 모션 그래프 구현
	// "Motion Graphs" by Kovar et al. ACM SIGGRAPH 2002
	// 
	// 주의: frameCount = 0;

	if (state == 0)
	{ // 정지 상태
		if (keyPressed[VK_UP])
		{
			state = 1;
			frameCount = 0;
		}
		else if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			frameCount = 0; // 상태 변화 없이 반복
		}
	}
	else if (state == 1) // 걷기 시작
	{
		if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			state = 2;
			frameCount = 0;
		}
	}
	else if (state == 2) // 걷기
	{
		// 캐릭터의 현재 앞쪽 방향
		Vector3 characterDirection = m_characterMeshModel->m_aniData.accumulatedRootTransform.Forward();

		m_characterMeshModel->m_aniData.accumulatedRootTransform *=
			Matrix::CreateTranslation(characterDirection * 300.0f * dt);

		if (keyPressed[VK_RIGHT])
			m_characterMeshModel->m_aniData.accumulatedRootTransform =
			Matrix::CreateRotationY(3.141592f * 60.0f / 180.0f * dt) *
			m_characterMeshModel->m_aniData.accumulatedRootTransform;
		if (keyPressed[VK_LEFT])
			m_characterMeshModel->m_aniData.accumulatedRootTransform =
			Matrix::CreateRotationY(-3.141592f * 60.0f / 180.0f * dt) *
			m_characterMeshModel->m_aniData.accumulatedRootTransform;

		// 방향키를 누르고 있지 않으면 정지 (누르면 계속 걷기)
		if (!keyPressed[VK_UP])
		{
			state = 3;
			frameCount = 0;
		}

	}
	else if (state == 3)
	{
		if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			state = 0;
			frameCount = 0;
		}
	}

	m_characterMeshModel->UpdateAnimation(m_context, state, frameCount);
	frameCount += 1;
}
