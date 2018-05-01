#include "GarrysMod/Lua/Interface.h"
#include "openvr/openvr.h"

const unsigned int major = 1;
const unsigned int minor = 0;
const unsigned int patch = 0;

using namespace GarrysMod::Lua;

vr::TrackedDevicePose_t devices[vr::k_unMaxTrackedDeviceCount]{};
Vector devPoses[vr::k_unMaxTrackedDeviceCount][4]{ {} };
unsigned int trackedDevices = 0;
vr::IVRSystem *system = 0;

int ResolveDeviceType(int deviceId) {

	if (!system)
		return -1;

	vr::ETrackedDeviceClass deviceClass = system->GetTrackedDeviceClass(deviceId);

	return static_cast<int>(deviceClass);
}

int ResolveDeviceRole(int deviceId) {

	if (!system)
		return -1;
	
	int deviceRole = system->GetInt32TrackedDeviceProperty(deviceId, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);

	return static_cast<int>(deviceRole);
}

LUA_FUNCTION(GetVersion)
{
	Vector ver = {};
	ver.x = static_cast<float>(major);
	ver.y = static_cast<float>(minor);
	ver.z = static_cast<float>(patch);

	LUA->PushVector(ver);
	return 1;
}

LUA_FUNCTION(IsHmdPresent)
{
	LUA->PushBool(vr::VR_IsHmdPresent());
	return 1;
}

LUA_FUNCTION(Init)
{
	vr::EVRInitError eError = vr::EVRInitError::VRInitError_None;

	if (system)
	{
		LUA->PushNumber(eError);
		return 1;
	}

	system = vr::VR_Init(&eError, vr::EVRApplicationType::VRApplication_Scene);

	if (eError != vr::EVRInitError::VRInitError_None)
	{
		LUA->PushNumber(eError);
		return 1;
	}

	LUA->PushNumber(eError);
	return 1;
}

LUA_FUNCTION(MaxTrackedDevices)
{
	LUA->PushNumber(vr::k_unMaxTrackedDeviceCount);
	return 1;
}

LUA_FUNCTION(TrackedDevices)
{
	LUA->PushNumber(trackedDevices);
	return 1;
}

LUA_FUNCTION(IsDeviceValid)
{
	LUA->CheckType(1, Type::NUMBER);

	LUA->PushBool(devices[static_cast<unsigned int>(LUA->GetNumber(1))].bPoseIsValid);
	return 1;
}

LUA_FUNCTION(Submit)
{
	LUA->CheckType(1, Type::NUMBER);
	LUA->CheckType(2, Type::NUMBER);
	LUA->CheckType(3, Type::NUMBER);

	vr::EVRCompositorError err = vr::EVRCompositorError::VRCompositorError_None;

	if (!system)
	{
		LUA->PushNumber(err);
		return 1;
	}

	vr::IVRCompositor *com = vr::VRCompositor();

	if (!com)
	{
		LUA->PushNumber(err);
		return 1;
	}

	vr::Texture_t tex = {};
	tex.eColorSpace = vr::EColorSpace::ColorSpace_Auto;
	tex.eType = (vr::ETextureType)static_cast<unsigned int>(LUA->GetNumber(1));
	tex.handle = (void *)(uintptr_t)LUA->GetNumber(2);

	err = com->Submit((vr::EVREye)static_cast<unsigned int>(LUA->GetNumber(3)), &tex);
	if (err != vr::EVRCompositorError::VRCompositorError_None)
	{
		LUA->PushNumber(err);
		return 1;
	}

	LUA->PushNumber(err);
	return 1;
}


LUA_FUNCTION(WaitGetPoses)
{
	vr::EVRCompositorError err = vr::EVRCompositorError::VRCompositorError_None;

	if (!system)
	{
		LUA->PushNumber(err);
		return 1;
	}

	err = vr::VRCompositor()->WaitGetPoses(devices, vr::k_unMaxTrackedDeviceCount, 0, 0);
	if (err)
	{
		LUA->PushNumber(err);
		return 1;
	}

	trackedDevices = 0;

	for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		if (!devices[i].bPoseIsValid)
			continue;

		++trackedDevices;

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
	LUA->CheckType(1, Type::NUMBER);

	int deviceId = static_cast<int>(LUA->GetNumber(1));
	int type = ResolveDeviceType(deviceId);
	LUA->PushNumber(type);
	return 1;
}

LUA_FUNCTION(GetDeviceRole)
{
	LUA->CheckType(1, Type::NUMBER);

	int deviceId = static_cast<int>(LUA->GetNumber(1));
	int type = ResolveDeviceRole(deviceId);
	LUA->PushNumber(type);
	return 1;
}


GMOD_MODULE_OPEN()
{
	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->PushString("GmodVR");

	LUA->CreateTable();
	{
		LUA->PushCFunction(GetVersion);
		LUA->SetField(-2, "GetVersion");

		LUA->PushCFunction(IsHmdPresent);
		LUA->SetField(-2, "IsHmdPresent");

		LUA->PushCFunction(MaxTrackedDevices);
		LUA->SetField(-2, "MaxTrackedDevices");

		LUA->PushCFunction(TrackedDevices);
		LUA->SetField(-2, "TrackedDevices");

		LUA->PushCFunction(IsDeviceValid);
		LUA->SetField(-2, "IsDeviceValid");

		LUA->PushCFunction(GetDeviceClass);
		LUA->SetField(-2, "GetDeviceClass");

		LUA->PushCFunction(GetDeviceRole);
		LUA->SetField(-2, "GetDeviceRole");

		LUA->PushCFunction(WaitGetPoses);
		LUA->SetField(-2, "WaitGetPoses");

		LUA->PushCFunction(Submit);
		LUA->SetField(-2, "Submit");

		LUA->PushCFunction(Init);
		LUA->SetField(-2, "Init");
	}

	LUA->Pop();

	return 0;
}


GMOD_MODULE_CLOSE()
{
	if (!system)
		return 0;

	vr::VR_Shutdown();
	system = 0;

	return 0;
}