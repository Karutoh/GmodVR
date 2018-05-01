#include <stdio.h>
#include <math.h>
#include <float.h>
#include "GarrysMod/Lua/Interface.h"
#include "openvr/openvr.h"

int version = 1; //every release this will be incremented, in lua, the user will be warned to update the dll if they have a lua version ahead of the module.

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
	system = vr::VR_Init(&eError, vr::VRApplication_Background);
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