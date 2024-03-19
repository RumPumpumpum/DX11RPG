#include "SkeletalAnimation.h"

#include <DirectXCollision.h> // 구와 광선 충돌 계산에 사용
#include <directxtk/DDSTextureLoader.h>
#include <directxtk/SimpleMath.h>
#include <tuple>
#include <vector>

#include "GeometryGenerator.h"
#include "GraphicsCommon.h"
#include "SkinnedMeshModel.h"


using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

SkeletalAnimation::SkeletalAnimation() : AppBase() {}

bool SkeletalAnimation::InitScene() {

	AppBase::m_globalConstsCPU.strengthIBL = 1.0f;

	AppBase::m_camera.Reset(Vector3(3.74966f, 5.03645f, -2.54918f), -0.819048f,
		0.741502f);

	AppBase::InitCubemaps(
		L"../Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds",
		L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds",
		L"clear_pureskyBrdf.dds");

	AppBase::InitScene();

	// 조명 설정
	{
		// 조명 0은 고정
		m_globalConstsCPU.lights[0].radiance = Vector3(5.0f);
		m_globalConstsCPU.lights[0].position = Vector3(0.0f, 2.0f, 2.0f);
		m_globalConstsCPU.lights[0].direction = Vector3(0.2f, -1.0f, 0.0f);
		m_globalConstsCPU.lights[0].spotPower = 0.0f;
		m_globalConstsCPU.lights[0].radius = 0.1f;
		m_globalConstsCPU.lights[0].type = LIGHT_POINT | LIGHT_SHADOW;

		m_globalConstsCPU.lights[1].type = LIGHT_OFF;
		m_globalConstsCPU.lights[2].type = LIGHT_OFF;
	}

	// 바닥(거울)
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		auto mesh = GeometryGenerator::MakeSquare(5.0);
		string path = "../Assets/Textures/PBR/stringy-marble-ue/";
		mesh.albedoTextureFilename = path + "stringy_marble_albedo.png";
		mesh.emissiveTextureFilename = "";
		mesh.aoTextureFilename = path + "stringy_marble_ao.png";
		mesh.metallicTextureFilename = path + "stringy_marble_Metallic.png";
		mesh.normalTextureFilename = path + "stringy_marble_Normal-dx.png";
		mesh.roughnessTextureFilename = path + "stringy_marble_Roughness.png";

		m_ground = make_shared<Model>(m_device, m_context, vector{ mesh });
		m_ground->m_materialConsts.GetCpu().albedoFactor = Vector3(0.7f);
		m_ground->m_materialConsts.GetCpu().emissionFactor = Vector3(0.0f);
		m_ground->m_materialConsts.GetCpu().metallicFactor = 0.5f;
		m_ground->m_materialConsts.GetCpu().roughnessFactor = 0.3f;

		Vector3 position = Vector3(0.0f, 0.0f, 2.0f);
		m_ground->UpdateWorldRow(Matrix::CreateRotationX(3.141592f * 0.5f) *
			Matrix::CreateTranslation(position));
		m_ground->m_castShadow = false; // 바닥은 그림자 만들기 생략

		m_mirrorPlane = SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_mirror = m_ground; // 바닥에 거울처럼 반사 구현

		// m_basicList.push_back(m_ground); // 거울은 리스트에 등록 X
	}

	// Main Object
	{
		string path = "../Assets/Characters/";
		vector<string> clipNames = {
			"Idle.fbx", "IdleToWalkForward.fbx",
			"WalkForward.fbx", "WalkStop.fbx" };

		AnimationData aniData;

		auto [meshes, _] =
			GeometryGenerator::ReadAnimationFromFile(path, "character.fbx");

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

		Vector3 center(0.0f, 0.5f, 2.0f);
		m_characterMeshModel =
			make_shared<SkinnedMeshModel>(m_device, m_context, meshes, aniData);
		m_characterMeshModel->m_materialConsts.GetCpu().albedoFactor = Vector3(1.0f);
		m_characterMeshModel->m_materialConsts.GetCpu().roughnessFactor = 0.8f;
		m_characterMeshModel->m_materialConsts.GetCpu().metallicFactor = 0.0f;
		m_characterMeshModel->UpdateWorldRow(Matrix::CreateScale(1.0f) *
			Matrix::CreateTranslation(center));

		m_basicList.push_back(m_characterMeshModel); // 리스트에 등록
	}

	return true;
}

void SkeletalAnimation::UpdateLights(float dt) {
	AppBase::UpdateLights(dt);
}

void SkeletalAnimation::Update(float dt) {
	AppBase::Update(dt);

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
		if (AppBase::m_keyPressed[VK_UP])
		{
			state = 1;
			frameCount = 0;
		}
		else if (frameCount ==
			m_characterMeshModel->m_aniData.clips[state].keys[0].size() ||
			AppBase::m_keyPressed[VK_UP])
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
		if (AppBase::m_keyPressed[VK_RIGHT])
			m_characterMeshModel->m_aniData.accumulatedRootTransform =
				Matrix::CreateRotationY(3.141592f * 60.0f / 180.0f * dt) *
				m_characterMeshModel->m_aniData.accumulatedRootTransform;
		if (AppBase::m_keyPressed[VK_LEFT])
			m_characterMeshModel->m_aniData.accumulatedRootTransform =
				Matrix::CreateRotationY(-3.141592f * 60.0f / 180.0f * dt) *
				m_characterMeshModel->m_aniData.accumulatedRootTransform;
		if (frameCount == m_characterMeshModel->m_aniData.clips[state].keys[0].size())
		{
			// 방향키를 누르고 있지 않으면 정지 (누르면 계속 걷기)
			if (!AppBase::m_keyPressed[VK_UP])
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

void SkeletalAnimation::Render() {
	AppBase::Render();
	AppBase::PostRender();
}

void SkeletalAnimation::UpdateGUI() {

	ImGui::SetNextItemOpen(false, ImGuiCond_Once);
	if (ImGui::TreeNode("General")) {
		ImGui::Checkbox("Use FPV", &m_camera.m_useFirstPersonView);
		ImGui::Checkbox("Wireframe", &m_drawAsWire);
		ImGui::Checkbox("DrawOBB", &m_drawOBB);
		ImGui::Checkbox("DrawBSphere", &m_drawBS);
		if (ImGui::Checkbox("MSAA ON", &m_useMSAA)) {
			CreateBuffers();
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Skybox")) {
		ImGui::SliderFloat("Strength", &m_globalConstsCPU.strengthIBL, 0.0f,
			0.5f);
		ImGui::RadioButton("Env", &m_globalConstsCPU.textureToDraw, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Specular", &m_globalConstsCPU.textureToDraw, 1);
		ImGui::SameLine();
		ImGui::RadioButton("Irradiance", &m_globalConstsCPU.textureToDraw, 2);
		ImGui::SliderFloat("EnvLodBias", &m_globalConstsCPU.envLodBias, 0.0f,
			10.0f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Post Effects")) {
		int flag = 0;
		flag += ImGui::RadioButton("Render", &m_postEffectsConstsCPU.mode, 1);
		ImGui::SameLine();
		flag += ImGui::RadioButton("Depth", &m_postEffectsConstsCPU.mode, 2);
		flag += ImGui::SliderFloat(
			"DepthScale", &m_postEffectsConstsCPU.depthScale, 0.0, 1.0);
		flag += ImGui::SliderFloat("Fog", &m_postEffectsConstsCPU.fogStrength,
			0.0, 10.0);

		if (flag)
			D3D11Utils::UpdateBuffer(m_context, m_postEffectsConstsCPU,
				m_postEffectsConstsGPU);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Post Processing")) {
		int flag = 0;
		flag += ImGui::SliderFloat(
			"Bloom Strength",
			&m_postProcess.m_combineFilter.m_constData.strength, 0.0f, 1.0f);
		flag += ImGui::SliderFloat(
			"Exposure", &m_postProcess.m_combineFilter.m_constData.option1,
			0.0f, 10.0f);
		flag += ImGui::SliderFloat(
			"Gamma", &m_postProcess.m_combineFilter.m_constData.option2, 0.1f,
			5.0f);
		// 편의상 사용자 입력이 인식되면 바로 GPU 버퍼를 업데이트
		if (flag) {
			m_postProcess.m_combineFilter.UpdateConstantBuffers(m_context);
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Mirror")) {

		ImGui::SliderFloat("Alpha", &m_mirrorAlpha, 0.0f, 1.0f);
		const float blendColor[4] = { m_mirrorAlpha, m_mirrorAlpha,
									 m_mirrorAlpha, 1.0f };
		if (m_drawAsWire)
			Graphics::mirrorBlendWirePSO.SetBlendFactor(blendColor);
		else
			Graphics::mirrorBlendSolidPSO.SetBlendFactor(blendColor);

		ImGui::SliderFloat("Metallic",
			&m_mirror->m_materialConsts.GetCpu().metallicFactor,
			0.0f, 1.0f);
		ImGui::SliderFloat("Roughness",
			&m_mirror->m_materialConsts.GetCpu().roughnessFactor,
			0.0f, 1.0f);

		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light")) {
		// ImGui::SliderFloat3("Position",
		// &m_globalConstsCPU.lights[0].position.x,
		//                     -5.0f, 5.0f);
		ImGui::SliderFloat("Halo Radius",
			&m_globalConstsCPU.lights[1].haloRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("Halo Strength",
			&m_globalConstsCPU.lights[1].haloStrength, 0.0f,
			1.0f);
		ImGui::SliderFloat("Radius", &m_globalConstsCPU.lights[1].radius, 0.0f,
			0.5f);
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Material")) {
		ImGui::SliderFloat("LodBias", &m_globalConstsCPU.lodBias, 0.0f, 10.0f);

		int flag = 0;

		if (m_pickedModel) {
			flag += ImGui::SliderFloat(
				"Metallic",
				&m_pickedModel->m_materialConsts.GetCpu().metallicFactor, 0.0f,
				1.0f);
			flag += ImGui::SliderFloat(
				"Roughness",
				&m_pickedModel->m_materialConsts.GetCpu().roughnessFactor, 0.0f,
				1.0f);
			flag += ImGui::CheckboxFlags(
				"AlbedoTexture",
				&m_pickedModel->m_materialConsts.GetCpu().useAlbedoMap, 1);
			flag += ImGui::CheckboxFlags(
				"EmissiveTexture",
				&m_pickedModel->m_materialConsts.GetCpu().useEmissiveMap, 1);
			flag += ImGui::CheckboxFlags(
				"Use NormalMapping",
				&m_pickedModel->m_materialConsts.GetCpu().useNormalMap, 1);
			flag += ImGui::CheckboxFlags(
				"Use AO", &m_pickedModel->m_materialConsts.GetCpu().useAOMap,
				1);
			flag += ImGui::CheckboxFlags(
				"Use HeightMapping",
				&m_pickedModel->m_meshConsts.GetCpu().useHeightMap, 1);
			flag += ImGui::SliderFloat(
				"HeightScale",
				&m_pickedModel->m_meshConsts.GetCpu().heightScale, 0.0f, 0.1f);
			flag += ImGui::CheckboxFlags(
				"Use MetallicMap",
				&m_pickedModel->m_materialConsts.GetCpu().useMetallicMap, 1);
			flag += ImGui::CheckboxFlags(
				"Use RoughnessMap",
				&m_pickedModel->m_materialConsts.GetCpu().useRoughnessMap, 1);
			if (flag) {
				m_pickedModel->UpdateConstantBuffers(m_device, m_context);
			}
			ImGui::Checkbox("Draw Normals", &m_pickedModel->m_drawNormals);
		}

		ImGui::TreePop();
	}
}

