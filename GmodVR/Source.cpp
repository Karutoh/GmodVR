#include "GarrysMod/Lua/Interface.h"
#include "openvr/openvr.h"

unsigned int major = 1;
unsigned int minor = 0;
unsigned int patch = 0;

using namespace GarrysMod::Lua;

vr::TrackedDevicePose_t devices[vr::k_unMaxTrackedDeviceCount];
Vector devPoses[vr::k_unMaxTrackedDeviceCount][4];
vr::IVRSystem *system;

LUA_FUNCTION(GetVersion)
{
	Vector ver = {};
	ver.x = major;
	ver.y = minor;
	ver.z = patch;

	LUA->PushVector(ver);
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

	vr::EVRCompositorError err = com->Submit((vr::EVREye)LUA->GetNumber(3), &tex);
	if (err != vr::EVRCompositorError::VRCompositorError_None)
	{
		LUA->PushNumber(err);
		return -1;
	}

	LUA->PushNumber(err);
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


LUA_FUNCTION(WaitGetPoses)
{
	vr::EVRCompositorError err = vr::EVRCompositorError::VRCompositorError_None;

	if (!system)
	{
		LUA->PushNumber(err);
		return -1;
	}

	err = vr::VRCompositor()->WaitGetPoses(devices, vr::k_unMaxTrackedDeviceCount, 0, 0);
	if (err)
	{
		LUA->PushNumber(err);
		return -1;
	}

	for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		if (!devices[i].bPoseIsValid)
			continue;

		vr::HmdMatrix34_t tmp = devices[i].mDeviceToAbsoluteTracking;

		devPoses[i][0].x = tmp.m[0][0];
		devPoses[i][0].y = tmp.m[1][0];
		devPoses[i][0].z = tmp.m[2][0];

		devPoses[i][1].x = tmp.m[0][1];
		devPoses[i][1].y = tmp.m[1][1];
		devPoses[i][1].z = tmp.m[2][1];

		devPoses[i][2].x = tmp.m[0][2];
		devPoses[i][2].y = tmp.m[1][2];
		devPoses[i][2].z = tmp.m[2][2];

		devPoses[i][3].x = tmp.m[0][3];
		devPoses[i][3].y = tmp.m[1][3];
		devPoses[i][3].z = tmp.m[2][3];
	}

	LUA->PushNumber(err);
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

	LUA->PushCFunction(GetVersion);
	LUA->SetField(-2, "GetVersion");

	LUA->PushCFunction(IsHmdPresent);
	LUA->SetField(-2, "IsHmdPresent");

	LUA->PushCFunction(InitVR);
	LUA->SetField(-2, "InitVR");

	LUA->PushCFunction(CountDevices);
	LUA->SetField(-2, "CountDevices");

	LUA->PushCFunction(GetDeviceClass);
	LUA->SetField(-2, "GetDeviceClass");

	LUA->PushCFunction(GetDeviceRole);
	LUA->SetField(-2, "GetDeviceRole");

	LUA->PushCFunction(Submit);
	LUA->SetField(-2, "Submit");

	LUA->SetField(-2, "GmodVR");
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