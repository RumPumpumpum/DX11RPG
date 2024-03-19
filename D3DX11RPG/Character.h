#pragma once

#include "SkinnedMeshModel.h"

#include <directxtk/SimpleMath.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::vector;

class Character
{
public:
	Character();
	void CharacterInit(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> context);

	void Move(float dt, bool keyPressed[]);
private:
	shared_ptr<SkinnedMeshModel> m_characterMeshModel;
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_context;
public:
	inline shared_ptr<SkinnedMeshModel> GetCharacterMeshModel() {return m_characterMeshModel;};
};