#include "Game.h"

#include <DirectXCollision.h> // ���� ���� �浹 ��꿡 ���
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

Game::Game() : AppBase() {}

bool Game::InitScene() {

	AppBase::m_globalConstsCPU.strengthIBL = 1.0f;

	AppBase::m_camera.Reset(Vector3(3.74966f, 5.03645f, -2.54918f), -0.819048f,
		0.741502f);

	AppBase::InitCubemaps(
		L"../Assets/Textures/Cubemaps/HDRI/", L"clear_pureskyEnvHDR.dds",
		L"clear_pureskySpecularHDR.dds", L"clear_pureskyDiffuseHDR.dds",
		L"clear_pureskyBrdf.dds");

	AppBase::InitScene();

	// ���� ����
	{
		// ���� 0�� ����
		m_globalConstsCPU.lights[0].radiance = Vector3(5.0f);
		m_globalConstsCPU.lights[0].position = Vector3(0.0f, 2.0f, 2.0f);
		m_globalConstsCPU.lights[0].direction = Vector3(0.2f, -1.0f, 0.0f);
		m_globalConstsCPU.lights[0].spotPower = 0.0f;
		m_globalConstsCPU.lights[0].radius = 0.1f;
		m_globalConstsCPU.lights[0].type = LIGHT_POINT | LIGHT_SHADOW;

		m_globalConstsCPU.lights[1].type = LIGHT_OFF;
		m_globalConstsCPU.lights[2].type = LIGHT_OFF;
	}

	// �ٴ�(�ſ�)
	{
		// https://freepbr.com/materials/stringy-marble-pbr/
		auto mesh = GeometryGenerator::MakeSquare(5.0);
		string path = "../Assets/Textures/PBR/worn-painted-metal-ue/";
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
		m_ground->m_castShadow = false; // �ٴ��� �׸��� ����� ����

		m_mirrorPlane = SimpleMath::Plane(position, Vector3(0.0f, 1.0f, 0.0f));
		m_mirror = m_ground; // �ٴڿ� �ſ�ó�� �ݻ� ����

		// m_basicList.push_back(m_ground); // �ſ��� ����Ʈ�� ��� X
	}

	character.CharacterInit(m_device, m_context);
	m_characterMeshModel = character.GetCharacterMeshModel();
	m_basicList.push_back(m_characterMeshModel); // ����Ʈ�� ���
	return true;
}

void Game::UpdateLights(float dt) {
	AppBase::UpdateLights(dt);
}

void Game::Update(float dt) {
	AppBase::Update(dt);
	character.Move(dt, AppBase::m_keyPressed);
	
}

void Game::Render() {
	AppBase::Render();
	AppBase::PostRender();
}

void Game::UpdateGUI() {

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
		// ���ǻ� ����� �Է��� �νĵǸ� �ٷ� GPU ���۸� ������Ʈ
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