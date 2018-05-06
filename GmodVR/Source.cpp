#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "openvr/openvr.h"

const unsigned int major = 1;
const unsigned int minor = 0;
const unsigned int patch = 0;

using namespace GarrysMod::Lua;

vr::TrackedDevicePose_t devices[vr::k_unMaxTrackedDeviceCount]{};
Vector devPoses[vr::k_unMaxTrackedDeviceCount][4]{{}};
Vector devVel[vr::k_unMaxTrackedDeviceCount][2]{{}};
unsigned int trackedDevices = 0;
vr::IVRSystem *system = nullptr;

vr::ETrackedDeviceClass RetreiveDeviceClass(int deviceId)
{
	if (!system)
		return vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid;

	return system->GetTrackedDeviceClass(deviceId);
}

vr::ETrackedControllerRole RetreiveDeviceRole(int deviceId)
{
	if (!system)
		return vr::ETrackedControllerRole::TrackedControllerRole_Invalid;

	return (vr::ETrackedControllerRole)system->GetInt32TrackedDeviceProperty(deviceId, vr::ETrackedDeviceProperty::Prop_ControllerRoleHint_Int32);
}

LUA_FUNCTION(GetVersion)
{
	LUA->PushVector({ static_cast<float>(major), static_cast<float>(minor), static_cast<float>(patch) });
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

	unsigned int dIndex = static_cast<unsigned int>(LUA->GetNumber(1));

	if (dIndex >= vr::k_unMaxTrackedDeviceCount)
	{
		LUA->PushBool(false);
		return 1;
	}

	LUA->PushBool(devices[dIndex].bPoseIsValid);
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

		devPoses[i][0] = { tmp.m[0][0], tmp.m[1][0], tmp.m[2][0] };
		devPoses[i][1] = { tmp.m[0][1], tmp.m[1][1], tmp.m[2][1] };
		devPoses[i][2] = { tmp.m[0][2], tmp.m[1][2], tmp.m[2][2] };
		devPoses[i][3] = { tmp.m[0][3], tmp.m[1][3], tmp.m[2][3] };

		vr::HmdVector3_t angVel = devices[i].vAngularVelocity;

		devVel[i][0] = {angVel.v[0], angVel.v[1], angVel.v[2]};

		vr::HmdVector3_t vel = devices[i].vVelocity;

		devVel[i][1] = { vel.v[0], vel.v[1], vel.v[2] };
	}

	LUA->PushNumber(err);
	return 1;
}

LUA_FUNCTION(GetDevicePose)
{
	LUA->CheckType(1, Type::NUMBER);

	unsigned int dIndex = static_cast<unsigned int>(LUA->GetNumber(1));

	if (dIndex >= vr::k_unMaxTrackedDeviceCount)
	{
		LUA->PushVector({ 1.0f, 0.0f, 0.0f });
		LUA->PushVector({ 0.0f, 1.0f, 0.0f });
		LUA->PushVector({ 0.0f, 0.0f, 1.0f });
		LUA->PushVector({ 0.0f, 0.0f, 0.0f });

		return 4;
	}

	LUA->PushVector(devPoses[dIndex][0]);
	LUA->PushVector(devPoses[dIndex][1]);
	LUA->PushVector(devPoses[dIndex][2]);
	LUA->PushVector(devPoses[dIndex][3]);

	return 4;
}

LUA_FUNCTION(GetDeviceVel)
{
	LUA->CheckType(1, Type::NUMBER);

	unsigned int dIndex = static_cast<unsigned int>(LUA->GetNumber(1));

	if (dIndex >= vr::k_unMaxTrackedDeviceCount)
	{
		LUA->PushVector({ 0.0f, 0.0f, 0.0f });
		LUA->PushVector({ 0.0f, 0.0f, 0.0f });

		return 2;
	}

	LUA->PushVector(devVel[dIndex][0]);
	LUA->PushVector(devVel[dIndex][1]);

	return 2;
}

LUA_FUNCTION(GetDeviceClass)
{
	LUA->CheckType(1, Type::NUMBER);

	unsigned int dIndex = static_cast<unsigned int>(LUA->GetNumber(1));

	if (dIndex >= vr::k_unMaxTrackedDeviceCount)
	{
		LUA->PushNumber(vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid);
		return 1;
	}

	LUA->PushNumber(RetreiveDeviceClass(dIndex));
	return 1;
}

LUA_FUNCTION(GetDeviceRole)
{
	LUA->CheckType(1, Type::NUMBER);

	unsigned int dIndex = static_cast<unsigned int>(LUA->GetNumber(1));

	if (dIndex >= vr::k_unMaxTrackedDeviceCount)
	{
		LUA->PushNumber(vr::ETrackedControllerRole::TrackedControllerRole_Invalid);
		return 1;
	}

	LUA->PushNumber(RetreiveDeviceRole(dIndex));
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

		LUA->PushCFunction(GetDevicePose);
		LUA->SetField(-2, "GetDevicePose");

		LUA->PushCFunction(GetDeviceVel);
		LUA->SetField(-2, "GetDeviceVel");

		LUA->PushCFunction(Submit);
		LUA->SetField(-2, "Submit");

		LUA->PushCFunction(Init);
		LUA->SetField(-2, "Init");
	}

	LUA->SetTable(-3);
	LUA->Pop();

	return 0;
}


GMOD_MODULE_CLOSE()
{
	if (!system)
		return 0;

	vr::VR_Shutdown();
	system = nullptr;

	return 0;
}