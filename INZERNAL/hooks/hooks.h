#pragma once
#include <windows.h>
#include <string.h>
#include <core/minhook/hook.h>
#include <proton/Variant.h>
#include <core/globals.h>

typedef void(__cdecl* ENetPacketFreeCallback) (struct _ENetPacket*);

typedef struct _ENetPacket {
	size_t reference_count;
	int flags;
	uint8_t* data;
	size_t dataLength;
	ENetPacketFreeCallback freeCallback;
	void* userData;
} ENetPacket;


#define TYPE(x) extern types::x x

namespace hooks {

	namespace orig {
		TYPE(App_GetVersion);
		TYPE(BaseApp_SetFPSLimit);
		TYPE(LogMsg);
		TYPE(NetAvatar_CanMessageT4);
		TYPE(CanPunchOrBuildNow);
		TYPE(ObjectMap_HandlePacket);
		TYPE(SendPacketRaw);
		TYPE(HandleTouch);


		extern WNDPROC wndproc;
	}

	void init();
	void destroy();

	// clang-format off

	float	__cdecl		App_GetVersion(App* app);
	void	__cdecl		BaseApp_SetFPSLimit(BaseApp* ba, float fps);
	int		__cdecl		LogMsg(const char* msg, ...);
	bool	__cdecl		NetAvatar_CanMessageT4(NetAvatar* player);
	bool	__cdecl		CanPunchOrBuildNow(AvatarRenderData* render_data);
	bool	__cdecl		ObjectMap_HandlePacket(WorldObjectMap* map, GameUpdatePacket* packet);
	void	__cdecl		SendPacketRaw(int type, void* data, int size, void* a4, void* peer, int flag);
	void	__cdecl		HandleTouch(LevelTouchComponent* touch, CL_Vec2f pos, bool started);

	// clang-format on


	LRESULT __stdcall hooked_wndproc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

}