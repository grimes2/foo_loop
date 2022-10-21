#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

#include <helpers/foobar2000+atl.h>

static constexpr const char* component_name = "Loop";

DECLARE_COMPONENT_VERSION(
	component_name,
	"1.3",
	"grimes\n\n"
	"Build: " __TIME__ ", " __DATE__
);

VALIDATE_COMPONENT_FILENAME("foo_loop.dll");

#define ID_TIMER2 1011

UINT_PTR ptr2 = 0;
double loop_position_end;
double loop_position_start;
double loop_length;
bool menu_loop_enabled = false;

VOID CALLBACK LoopTimer(
	HWND hwnd,        // handle to window for timer messages
	UINT message,     // WM_TIMER message
	UINT idEvent1,     // timer identifier
	DWORD dwTime)     // current system time
{
	if (menu_loop_enabled)
	{
		static_api_ptr_t<playback_control>()->playback_seek(loop_position_start);
	}
	else
	{
		KillTimer(NULL, idEvent1);
	}
}

class mainmenu_commands_loop : public mainmenu_commands
{

public:

	// Return the number of commands we provide.
	virtual t_uint32 get_command_count()
	{
		return 3;
	}

	// All commands are identified by a GUID.
	virtual GUID get_command(t_uint32 p_index)
	{
		// {2C77FEA1-C0AC-4A15-8528-793B99D408A5}
		static const GUID guid_main_loop_length_start = { 0x2c77fea1, 0xc0ac, 0x4a15, { 0x85, 0x28, 0x79, 0x3b, 0x99, 0xd4, 0x8, 0xa5 } };
		// {A50DFA39-657B-4293-A504-923FCE45EA1C}
		static const GUID guid_main_loop_length_end = { 0xa50dfa39, 0x657b, 0x4293, { 0xa5, 0x4, 0x92, 0x3f, 0xce, 0x45, 0xea, 0x1c } };
		// {826DCF77-8E94-4D62-BD23-38981A5AC574}
		static const GUID guid_main_loop = { 0x826dcf77, 0x8e94, 0x4d62, { 0xbd, 0x23, 0x38, 0x98, 0x1a, 0x5a, 0xc5, 0x74 } };
		if (p_index == 0)
			return guid_main_loop_length_start;
		if (p_index == 1)
			return guid_main_loop_length_end;
		if (p_index == 2)
			return guid_main_loop;
		return pfc::guid_null;
	}

	// Set p_out to the name of the n-th command.
	// This name is used to identify the command and determines
	// the default position of the command in the menu.
	virtual void get_name(t_uint32 p_index, pfc::string_base& p_out)
	{
		if (p_index == 0)
			p_out = "Loop start playback time";
		if (p_index == 1)
			p_out = "Loop end playback time";
		if (p_index == 2)
			p_out = "Loop";
	}

	// Set p_out to the description for the n-th command.
	virtual bool get_description(t_uint32 p_index, pfc::string_base& p_out)
	{
		if (p_index == 0)
			p_out = "Set start playback time of loop";
		else if (p_index == 1)
			p_out = "Set end playback time of loop";
		else if (p_index == 2)
			p_out = "Toogle loop";
		else
			return false;
		return true;
	}

	// Every set of commands needs to declare which group it belongs to.
	virtual GUID get_parent()
	{
		return mainmenu_groups::playback;
	}

	// Execute n-th command.
	// p_callback is reserved for future use.
	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
	{
		if (p_index == 0)
		{
			if (menu_loop_enabled)
			{
				loop_position_start = static_api_ptr_t<playback_control>()->playback_get_position();
				console::info("Start playback time of loop");
			}
		}
		if (p_index == 1)
		{
			if (menu_loop_enabled && (static_api_ptr_t<playback_control>()->is_paused() || static_api_ptr_t<playback_control>()->is_playing()))
			{
				loop_position_end = static_api_ptr_t<playback_control>()->playback_get_position();
				static_api_ptr_t<playback_control>()->playback_seek(loop_position_start);
				loop_length = loop_position_end - loop_position_start;
				ptr2 = SetTimer(NULL, ID_TIMER2, loop_length * 1000, (TIMERPROC)LoopTimer);
				console::info("End playback time of loop");
			}
		}
		if (p_index == 2)
		{
			menu_loop_enabled = !menu_loop_enabled;
			if (menu_loop_enabled) 
			{
				console::info("Loop on");
			}
			else 
			{
				console::info("Loop off");
			}
		}
	}

	// The standard version of this command does not support checked or disabled
	// commands, so we use our own version.
	virtual bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags)
	{
		if (p_index == 0) {
			get_name(p_index, p_text);
			if (!menu_loop_enabled)
			{
				p_flags |= flag_disabled | flag_defaulthidden;
			}
		}
		if (p_index == 1) {
			get_name(p_index, p_text);
			if (!menu_loop_enabled)
			{
				p_flags |= flag_disabled | flag_defaulthidden;
			}
		}
		if (p_index == 2) {
			if (menu_loop_enabled)
			{
				p_flags |= flag_checked;
			}
			else {
				p_flags = 0;
			}
			get_name(p_index, p_text);
		}
		return true;
	}
	virtual t_uint32 get_sort_priority()
	{
		return sort_priority_dontcare;
	}
	bool is_checked(t_uint32 p_index)
	{
		if (p_index == 2)
			return menu_loop_enabled;
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_loop> g_mainmenu_commands_loop;

class play_callback_seek : public play_callback_static
{
public:
	unsigned get_flags() { return flag_on_playback_stop; }
	virtual void on_playback_seek(double p_time) {}
	virtual void on_playback_new_track(metadb_handle_ptr p_track) {}
	virtual void on_playback_stop(play_control::t_stop_reason p_reason) {
		if (menu_loop_enabled)
			KillTimer(NULL, ptr2);
	}
	virtual void on_playback_pause(bool p_state) {}
	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	virtual void on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void on_playback_dynamic_info(const file_info& info) {}
	virtual void on_playback_dynamic_info_track(const file_info& info) {}
	virtual void on_playback_time(double p_time) {}
	virtual void on_volume_change(float p_new_val) {}
};
static play_callback_static_factory_t<play_callback_seek> g_play_callback_seek;