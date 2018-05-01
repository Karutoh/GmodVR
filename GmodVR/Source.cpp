#include <stdio.h>
#include <math.h>
#include <float.h>
#include "GarrysMod/Lua/Interface.h"
#include "openvr/openvr.h"

int version = 1;

using namespace GarrysMod::Lua;

vr::IVRSystem *system;

LUA_FUNCTION(GetVersion)
{
	LUA->PushNumber(version);
	return 1;
}

LUA_FUNCTION(IsHmdPresent)
{
	LUA->PushBool(vr::VR_IsHmdPresent());
	return 1;
}

LUA_FUNCTION(InitVR)
{
	vr::EVRInitError eError = vr::VRInitError_None;

	system = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		LUA->PushBool(false);
		return 1;
	}
	LUA->PushBool(true);
	return 1;
}

LUA_FUNCTION(CountDevices)
{
	LUA->PushNumber(vr::k_unMaxTrackedDeviceCount);
	return 1;
}

LUA_FUNCTION(Submit)
{
	if (!system)
		return -1;

	vr::IVRCompositor *com = vr::VRCompositor();

	if (!com)
		return -1;

	vr::Texture_t tex = {};
	tex.eColorSpace = vr::EColorSpace::ColorSpace_Auto;
	tex.eType = (vr::ETextureType)LUA->GetNumber(1);
	tex.handle = (void *)(uintptr_t)LUA->GetNumber(2);

	com->Submit((vr::EVREye)LUA->GetNumber(3), &tex);

	return 1;
}

int ResolveDeviceType(int deviceId) {

	if (!system) {
		return -1;
	}

	vr::ETrackedDeviceClass deviceClass = vr::VRSystem()->GetTrackedDeviceClass(deviceId);

	return static_cast<int>(deviceClass);
}

int ResolveDeviceRole(int deviceId) {

	if (!system) {
		return -1;
	}

	int deviceRole = vr::VRSystem()->GetInt32TrackedDeviceProperty(deviceId, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);

	return static_cast<int>(deviceRole);
}


LUA_FUNCTION(GetDevicePose)
{
	(LUA->CheckType(1, Type::NUMBER));
	int deviceId = static_cast<int>(LUA->GetNumber(1));
	LUA->PushBool(false);
	return 1;
}

LUA_FUNCTION(GetDeviceClass)
{
	(LUA->CheckType(1, Type::NUMBER));
	int deviceId = static_cast<int>(LUA->GetNumber(1));
	int type = ResolveDeviceType(deviceId);
	LUA->PushNumber(type);
	return 1;
}

LUA_FUNCTION(GetDeviceRole)
{
	(LUA->CheckType(1, Type::NUMBER));
	int deviceId = static_cast<int>(LUA->GetNumber(1));
	int type = ResolveDeviceRole(deviceId);
	LUA->PushNumber(type);
	return 1;
}


GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
	LUA->CreateTable();
	LUA->PushCFunction(GetVersion); LUA->SetField(-2, "GetVersion");
	LUA->PushCFunction(IsHmdPresent); LUA->SetField(-2, "IsHmdPresent");
	LUA->PushCFunction(InitVR); LUA->SetField(-2, "InitVR");
	LUA->PushCFunction(CountDevices); LUA->SetField(-2, "CountDevices");
	LUA->PushCFunction(GetDeviceClass); LUA->SetField(-2, "GetDeviceClass");
	LUA->PushCFunction(GetDeviceRole); LUA->SetField(-2, "GetDeviceRole");
	LUA->PushCFunction(Submit); LUA->SetField(-2, "Submit");
	LUA->SetField(-2, "gvr");
	LUA->Pop();
	return 0;
}


GMOD_MODULE_CLOSE()
{
	return 0;
}

void Shutdown()
{
	if (system != NULL)
	{
		vr::VR_Shutdown();
		system = NULL;
	}
}