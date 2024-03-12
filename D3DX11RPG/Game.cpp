#include "Game.h"

#include "GeometryGenerator.h"
#include "GraphicsCommon.h"


using namespace std;
using namespace DirectX;
using namespace DirectX::SimpleMath;

Game::Game() : AppBase() {}

Game::~Game() {}

bool Game::InitScene() {

	AppBase::m_globalConstsCPU.strengthIBL = 0.1f;
	AppBase::m_globalConstsCPU.lodBias = 0.0f;

	AppBase::m_camera.Reset(Vector3(1.60851f, 0.409084f, 0.560064f), -1.65915f,
		0.0654498f);

	AppBase::InitCubemaps(
		L"../Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds",
		L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds",
		L"clear_pureskyBrdf.dds");

	AppBase::InitScene();

	// 바닥(거울)
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		auto mesh = GeometryGenerator::MakeSquare(5.0, { 10.0f, 10.0f });
		string path = "../Assets/Textures/PBR/stringy-marble-ue/";
		mesh.albedoTextureFilename = path + "stringy_marble_albedo.png";
		mesh.emissiveTextureFilename = "";
		mesh.aoTextureFilename = path + "stringy_marble_ao.png";
		mesh.metallicTextureFilename = path + "stringy_marble_Metallic.png";
		mesh.normalTextureFilename = path + "stringy_marble_Normal-dx.png";
		mesh.roughnessTextureFilename = path + "stringy_marble_Roughness.png";

		auto ground = make_shared<Model>(m_device, m_context, vector{ mesh });
		ground->m_materialConsts.GetCpu().albedoFactor = Vector3(0.2f);
		ground->m_materialConsts.GetCpu().emissionFactor = Vector3(0.0f);
		ground->m_materialConsts.GetCpu().metallicFactor = 0.5f;
		ground->m_materialConsts.GetCpu().roughnessFactor = 0.3f;
		Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
		ground->UpdateWorldRow(Matrix::CreateRotationX(3.141592f * 0.5f) *
			Matrix::CreateTranslation(position));

		m_mirrorPlane = SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		// m_mirror = ground; // 바닥에 거울처럼 반사 구현

		m_basicList.push_back(ground); // 거울은 리스트에 등록 X
	}

	// Main Object
	//{
	//	vector<string> clipNames = { "FightingIdleOnMichelle2.fbx",
	//								"Fireball.fbx" };
	//	string path = "../Assets/Characters/Mixamo/";

	//	AnimationData aniData;

	//	auto [meshes, _] =
	//		GeometryGenerator::ReadAnimationFromFile(path, "character.fbx");

	//	for (auto& name : clipNames) {
	//		auto [_, ani] =
	//			GeometryGenerator::ReadAnimationFromFile(path, name);

	//		if (aniData.clips.empty()) {
	//			aniData = ani;
	//		}
	//		else {
	//			aniData.clips.push_back(ani.clips.front());
	//		}
	//	}

	//	Vector3 center(0.0f, 0.1f, 1.0f);
	//	m_character =
	//		make_shared<SkinnedMeshModel>(m_device, m_context, meshes, aniData);
	//	m_character->m_materialConsts.GetCpu().albedoFactor = Vector3(1.0f);
	//	m_character->m_materialConsts.GetCpu().roughnessFactor = 0.8f;
	//	m_character->m_materialConsts.GetCpu().metallicFactor = 0.0f;
	//	m_character->UpdateWorldRow(Matrix::CreateScale(0.2f) *
	//		Matrix::CreateTranslation(center));

	//	m_basicList.push_back(m_character); // 리스트에 등록
	//	m_pickedModel = m_character;
	//}

	return true;
}

void Game::UpdateLights(float dt) { AppBase::UpdateLights(dt); }

void Game::Update(float dt) {

	AppBase::Update(dt);

	static int frameCount = 0;
	static int state = 0;

	// TODO:

	//m_character->UpdateAnimation(m_context, state, frameCount);

	frameCount += 1;
}

void Game::Render() {
	AppBase::Render();
	AppBase::PostRender();
}

void Game::UpdateGUI() {
	AppBase::UpdateGUI();
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
	if (m_mirror && ImGui::TreeNode("Mirror")) {

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