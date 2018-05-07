#include "GarrysMod/Lua/Interface.h"
#include "GarrysMod/Lua/SourceCompat.h"
#include "openvr/openvr.h"

const unsigned int major = 1;
const unsigned int minor = 0;
const unsigned int patch = 0;

using namespace GarrysMod::Lua;

enum ButtonState
{
	BS_RELEASED,
	BS_PRESSED,
	BS_ONCE
};

struct Button
{
	ButtonState state;
	bool wasPressed;
} ButtonDef = { BS_RELEASED, false };

struct DeviceData
{
	Button buttons[vr::EVRButtonId::k_EButton_Max] = {};
	Vector matrix[4] = {};
	bool connected;
	Vector angVel;
	Vector vel;
} DeviceDataDef = { { ButtonDef }, { { 0.0f, 0.0f, 0.0f } }, false, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

vr::TrackedDevicePose_t devices[vr::k_unMaxTrackedDeviceCount]{};
DeviceData devData[vr::k_unMaxTrackedDeviceCount]{ DeviceDataDef };
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

	for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		if (!system->IsTrackedDeviceConnected(i))
			continue;

		devData[i].connected = true;
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
	unsigned int count = 0;
	for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		if (!devData[i].connected)
			continue;

		++count;
	}

	LUA->PushNumber(count);
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

LUA_FUNCTION(Update)
{
	if (!system)
		return 0;

	vr::VREvent_t e;
	while (system->PollNextEvent(&e, sizeof(e)))
	{
		if (e.eventType == vr::VREvent_TrackedDeviceActivated)
			devData[e.trackedDeviceIndex].connected = true;

		if (e.eventType == vr::VREvent_TrackedDeviceDeactivated)
			devData[e.trackedDeviceIndex].connected = false;

		if (e.eventType == vr::VREvent_ButtonPress)
			devData[e.trackedDeviceIndex].buttons[e.data.controller.button].wasPressed = true;

		if (e.eventType == vr::VREvent_ButtonUnpress)
			devData[e.trackedDeviceIndex].buttons[e.data.controller.button].wasPressed = false;

		if (e.eventType == vr::VREvent_ButtonTouch)
			devData[e.trackedDeviceIndex].buttons[e.data.controller.button].wasPressed = true;

		if (e.eventType == vr::VREvent_ButtonUntouch)
			devData[e.trackedDeviceIndex].buttons[e.data.controller.button].wasPressed = false;
	}

	for (unsigned int d = 0; d < vr::k_unMaxTrackedDeviceCount; ++d)
	{
		if (!devData[d].connected)
			continue;

		for (unsigned int b = 0; b < vr::EVRButtonId::k_EButton_Max; ++b)
		{
			if (devData[d].buttons[b].wasPressed)
			{
				if (devData[d].buttons[b].state == BS_RELEASED)
					devData[d].buttons[b].state = BS_ONCE;
				else
					devData[d].buttons[b].state = BS_PRESSED;
			}
			else
				devData[d].buttons[b].state = BS_RELEASED;
		}
	}

	return 0;
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

	for (unsigned int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		if (!devices[i].bPoseIsValid)
			continue;

		vr::HmdMatrix34_t tmp = devices[i].mDeviceToAbsoluteTracking;

		devData[i].matrix[0] = { tmp.m[0][0], tmp.m[1][0], tmp.m[2][0] };
		devData[i].matrix[1] = { tmp.m[0][1], tmp.m[1][1], tmp.m[2][1] };
		devData[i].matrix[2] = { tmp.m[0][2], tmp.m[1][2], tmp.m[2][2] };
		devData[i].matrix[3] = { tmp.m[0][3], tmp.m[1][3], tmp.m[2][3] };

		vr::HmdVector3_t angVel = devices[i].vAngularVelocity;

		devData[i].angVel = { angVel.v[0], angVel.v[1], angVel.v[2] };

		vr::HmdVector3_t vel = devices[i].vVelocity;

		devData[i].vel = { vel.v[0], vel.v[1], vel.v[2] };
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

	LUA->PushVector(devData[dIndex].matrix[0]);
	LUA->PushVector(devData[dIndex].matrix[1]);
	LUA->PushVector(devData[dIndex].matrix[2]);
	LUA->PushVector(devData[dIndex].matrix[3]);

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

	LUA->PushVector(devData[dIndex].angVel);
	LUA->PushVector(devData[dIndex].vel);

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

		LUA->PushCFunction(Update);
		LUA->SetField(-2, "Update");

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