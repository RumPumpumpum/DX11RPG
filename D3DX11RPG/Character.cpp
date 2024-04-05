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

	Vector3 center(0.0f, 0.5f, 2.0f);	// ĳ���Ͱ� ������ ��
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

	// ������ ��� �׷��� ����
	// "Motion Graphs" by Kovar et al. ACM SIGGRAPH 2002
	// 
	// ����: frameCount = 0;

	if (state == 0)
	{ // ���� ����
		if (keyPressed[VK_UP])
		{
			state = 1;
			frameCount = 0;
		}
		else if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			frameCount = 0; // ���� ��ȭ ���� �ݺ�
		}
	}
	else if (state == 1) // �ȱ� ����
	{
		if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			state = 2;
			frameCount = 0;
		}
	}
	else if (state == 2) // �ȱ�
	{
		// ĳ������ ���� ���� ����
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

		// ����Ű�� ������ ���� ������ ���� (������ ��� �ȱ�)
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
