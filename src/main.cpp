// MintCap - a ReShade addon that caps the frame rate to a value you choose.
//
// Copyright (c) 2026 Nipeno. All rights reserved.
//
// Source available, NOT open source. Free to use, read, build and modify for
// yourself; redistribution and commercial use need written permission, and the
// licence is revocable on notice. Provided WITHOUT ANY WARRANTY. Full terms in
// the LICENSE file: https://github.com/Nipeno/MintCap/blob/main/LICENSE
//
// Build target: single DLL renamed to MintCap.addon64 (Windows x64).
// Requires the ADDON-ENABLED build of ReShade. Built for FiveM, works in any game ReShade runs on.
//
// How it works:
//  - reshade::addon_event::present fires once per frame. We measure the time since
//    the previous present and, when a cap is set, block until exactly one frame
//    interval has elapsed using a high-resolution waitable timer for the bulk of the
//    wait plus a short busy-wait for sub-millisecond accuracy.
//  - The overlay callback draws a dedicated "MintCap" window. ReShade
//    remembers that window's position, size and dock slot for us, in ReShade.ini.
//  - The chosen cap is persisted via ReShade's own config (no custom file). 0 means
//    unlimited, so a fresh install does nothing until the user asks for a cap.
//  - Startup, initialisation and failures are written to ReShade.log, so a user can
//    send that one file when asking for support. Routine detail is logged at DEBUG;
//    ReShade writes every level, so the level is severity labelling, not filtering.

// Required by reshade_overlay.hpp, which opens with
//   static_assert(sizeof(ImTextureID) == 8, "missing \"#define ImTextureID ImU64\" ...")
// because ReShade passes resource_view handles (uint64_t) through ImTextureID. At our
// pinned ImGui 1.90.4 the default is still void*, so without this define the build
// fails on the first #include <reshade.hpp>. imgui.h guards its typedef with #ifndef.
#define ImTextureID ImU64

#include <imgui.h>          // Must be included BEFORE reshade.hpp so the overlay wrappers compile.
#include <reshade.hpp>

#include <Windows.h>
#include <shellapi.h>       // ShellExecuteA (open the source-code link)
#include <intrin.h>         // _mm_pause (spin-wait hint)
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdarg>          // Variadic logging helper
#include <cstdio>           // vsnprintf, snprintf

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// Upper bound for the cap. Not a technical limit - it just keeps a typo or a
// hand-edited ReShade.ini from handing the pacing loop an absurd interval.
static constexpr int kMaxFps = 1000;

// Last slice we busy-wait instead of sleeping, for sub-millisecond frame accuracy.
// The coarse wait (high-res timer or sleep) lands within ~0.5 ms; the spin nails it.
static constexpr std::chrono::microseconds kSpinMargin{ 500 };

// Brand accent (#93E9BE). Used as text and as a low-alpha hover tint only - it is a
// light mint, so filling a button with it would leave the default label unreadable.
static const ImVec4 kAccent    { 0.576f, 0.914f, 0.745f, 1.00f };
static const ImVec4 kAccentDim { 0.576f, 0.914f, 0.745f, 0.25f };

// Some older SDK headers lack this flag; define it so the build never depends on it.
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

static constexpr const char *kConfigSection = "MintCap";
static constexpr const char *kConfigKey     = "FpsCap";

static constexpr const char *kGithubUrl = "https://github.com/Nipeno/MintCap";

// Injected by CMake (-DMINTCAP_VERSION). Fallback keeps non-CMake/standalone builds compiling.
#ifndef MINTCAP_VERSION_STR
#define MINTCAP_VERSION_STR "0.0.0"
#endif

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

// printf-style wrapper around reshade::log_message; the line lands in ReShade.log,
// prefixed with the add-on name. ReShade has no runtime log level - ReShadeLogMessage
// writes whatever it is given (source/addon.cpp -> reshade::log::message) - so the
// level here labels severity for whoever reads the log, it does not filter anything.
// That is exactly why nothing is logged per frame.
static void mc_log(reshade::log_level level, const char *fmt, ...)
{
	char buf[512];

	va_list args;
	va_start(args, fmt);
	const int written = std::vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	if (written < 0)
		return; // Formatting failed; nothing useful to log.

	reshade::log_message(level, buf);
}

// Human-readable graphics API, so a user's log says which renderer we attached to.
static const char *device_api_name(reshade::api::device_api api)
{
	switch (api)
	{
	case reshade::api::device_api::d3d9:   return "D3D9";
	case reshade::api::device_api::d3d10:  return "D3D10";
	case reshade::api::device_api::d3d11:  return "D3D11";
	case reshade::api::device_api::d3d12:  return "D3D12";
	case reshade::api::device_api::opengl: return "OpenGL";
	case reshade::api::device_api::vulkan: return "Vulkan";
	default:                               return "unknown";
	}
}

// Keep a cap inside [0, kMaxFps]. Applied both to typed input and to whatever the
// config file happens to contain - ReShade.ini is plain text a user can edit.
static int clamp_fps(int fps)
{
	if (fps < 0)
		return 0;
	if (fps > kMaxFps)
		return kMaxFps;
	return fps;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

// Read in the present callback every frame; written from the overlay callback.
// 0 = unlimited (no pacing at all); any positive value = that many frames per second.
static std::atomic<int> g_fps_cap{ 0 };

// Set on the first present we ever see, so the log can prove the pacing callback
// really runs (as opposed to "add-on registered, but events never reach us").
// Atomic because a process can drive more than one swapchain.
static std::atomic<bool> g_first_present_logged{ false };

using clock_type = std::chrono::high_resolution_clock;
static clock_type::time_point g_last_present = clock_type::now();

// High-resolution waitable timer for the coarse wait. Null -> sleep_for fallback
// (older OS or unsupported flag); timeBeginPeriod(1) keeps that path tight.
static HANDLE g_timer = nullptr;

// ---------------------------------------------------------------------------
// Frame pacing
// ---------------------------------------------------------------------------

// Hybrid wait + busy-wait. A plain Sleep() stutters because of Windows timer
// granularity, so we wait until ~0.5 ms before the target with a high-resolution
// waitable timer (cheap, no CPU burn) and spin only the final slice.
static void on_present(reshade::api::command_queue *, reshade::api::swapchain *,
                       const reshade::api::rect *, const reshade::api::rect *,
                       uint32_t, const reshade::api::rect *)
{
	// Read the cap once: the overlay can change it between frames, and every
	// calculation below has to agree on which value this frame is being paced to.
	const int cap = g_fps_cap.load(std::memory_order_relaxed);

	// One line, once per process: past this point the frame pacing path is live.
	if (!g_first_present_logged.exchange(true, std::memory_order_relaxed))
	{
		if (cap > 0)
			mc_log(reshade::log_level::info, "First frame presented; capping at %d FPS.", cap);
		else
			mc_log(reshade::log_level::info, "First frame presented; no cap set (unlimited).");
	}

	if (cap <= 0)
	{
		// No cap: just keep the timestamp fresh so setting one does not over-sleep.
		g_last_present = clock_type::now();
		return;
	}

	// One frame at the current cap. A double divide per frame is not worth caching.
	const std::chrono::duration<double> frame_interval{ 1.0 / static_cast<double>(cap) };

	const clock_type::time_point target     = g_last_present + std::chrono::duration_cast<clock_type::duration>(frame_interval);
	const clock_type::time_point spin_start  = target - std::chrono::duration_cast<clock_type::duration>(kSpinMargin);

	// Coarse phase: block until ~0.5 ms before the target without burning a core.
	for (;;)
	{
		const clock_type::time_point now = clock_type::now();
		if (now >= spin_start)
			break;

		const auto remaining = spin_start - now;
		if (g_timer != nullptr)
		{
			// Negative due time = relative, in 100 ns units. One wait is enough;
			// the loop just re-checks the clock in case of an early wake.
			LARGE_INTEGER due;
			due.QuadPart = -(std::chrono::duration_cast<std::chrono::nanoseconds>(remaining).count() / 100);
			if (due.QuadPart < 0 && SetWaitableTimerEx(g_timer, &due, 0, nullptr, nullptr, nullptr, 0))
				WaitForSingleObject(g_timer, INFINITE);
			else
				break; // Sub-100 ns left, or the timer failed; spin handles it.
		}
		else
		{
			std::this_thread::sleep_for(remaining); // Fallback: timeBeginPeriod(1) keeps this ~1 ms.
		}
	}

	// Fine phase: busy-wait the last slice for frame-accurate pacing.
	while (clock_type::now() < target)
		_mm_pause();

	// Drift compensation: anchor the next frame to the ideal target so per-frame
	// overshoot does not accumulate into a slow drift below the cap. But if we fell
	// more than a full frame behind (alt-tab, hitch, loading stall), drop the debt
	// and re-anchor to now so we never burst-render to "catch up".
	const clock_type::time_point now = clock_type::now();
	const clock_type::time_point one_frame_late = target + std::chrono::duration_cast<clock_type::duration>(frame_interval);
	g_last_present = (now > one_frame_late) ? now : target;
}

// ---------------------------------------------------------------------------
// Config persistence (ReShade config, not a custom file)
// ---------------------------------------------------------------------------

static void on_init_effect_runtime(reshade::api::effect_runtime *runtime)
{
	mc_log(reshade::log_level::info, "Effect runtime initialised (device API: %s).",
	        device_api_name(runtime->get_device()->get_api()));

	int value = 0;
	if (reshade::get_config_value(runtime, kConfigSection, kConfigKey, value))
	{
		const int cap = clamp_fps(value);
		g_fps_cap.store(cap, std::memory_order_relaxed);
		if (cap != value)
			mc_log(reshade::log_level::warning, "Config %s=%d is out of range; clamped to %d.",
			        kConfigKey, value, cap);
		else
			mc_log(reshade::log_level::debug, "Loaded %s=%d from config.", kConfigKey, cap);
	}
	else
	{
		mc_log(reshade::log_level::debug, "No saved config; starting with no cap.");
	}
}

static void on_destroy_effect_runtime(reshade::api::effect_runtime *)
{
	mc_log(reshade::log_level::debug, "Effect runtime destroyed.");
}

// ---------------------------------------------------------------------------
// Overlay window
// ---------------------------------------------------------------------------

static void draw_overlay(reshade::api::effect_runtime *runtime)
{
	// First run only: a deliberate size and spot instead of wherever ImGui lands.
	// ReShade persists this window's position, size, collapsed state and dock slot in
	// ReShade.ini ([OVERLAY] Window=/Docking=, keyed by the window title), and
	// ImGuiCond_FirstUseEver is ignored once that saved entry exists - so whatever the
	// user drags it to, including into ReShade's docked panel, always wins afterwards.
	// x at 45% of the screen keeps it clear of ReShade's own left-hand dock column.
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	// 0 on both axes = auto-fit to the contents. A fixed width would clip the URL
	// label, and by how much depends on the user's ReShade font size.
	ImGui::SetWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowPos(ImVec2(display.x * 0.45f, display.y * 0.20f), ImGuiCond_FirstUseEver);

	// No wordmark here: ReShade already draws the window title (or, when docked, a tab)
	// from the name passed to register_overlay, so a "MintCap" line inside the panel just
	// repeated it. The mint accent lives on the status line and the preset hover instead.
	const int previous = g_fps_cap.load(std::memory_order_relaxed);
	int cap = previous;

	ImGui::SetNextItemWidth(120.0f);
	ImGui::InputInt("FPS cap", &cap);

	// Deliberately body text, not a tooltip: what 0 means has to be readable without
	// the user first guessing that there is something to hover over.
	ImGui::TextDisabled("0 = unlimited (no cap).");
	ImGui::TextDisabled("Any number above 0 = that many FPS.");

	ImGui::Spacing();

	// Every push is popped before this function returns - ReShade draws its own UI in
	// the same ImGui frame, so a leaked style push would tint the rest of the overlay.
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kAccentDim);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, kAccentDim);

	if (ImGui::Button("Off"))
		cap = 0;

	static constexpr int kPresets[] = { 30, 60, 120, 144, 240 };
	for (const int preset : kPresets)
	{
		char label[16];
		std::snprintf(label, sizeof(label), "%d", preset);
		ImGui::SameLine();
		if (ImGui::Button(label))
			cap = preset;
	}

	ImGui::PopStyleColor(2);

	cap = clamp_fps(cap);
	if (cap != previous)
	{
		g_fps_cap.store(cap, std::memory_order_relaxed);
		g_last_present = clock_type::now(); // Reset pacing baseline on change.
		reshade::set_config_value(runtime, kConfigSection, kConfigKey, cap);
	}

	ImGui::Spacing();
	if (cap > 0)
		ImGui::TextColored(kAccent, "Status: capping at %d FPS", cap);
	else
		ImGui::TextColored(kAccent, "Status: unlimited (no cap)");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextUnformatted("Made by Nipeno");
	ImGui::TextDisabled("Version %s", MINTCAP_VERSION_STR);

	ImGui::Spacing();

	// Source code: open the GitHub repo so users can read what they installed.
	if (ImGui::Button("View Source on GitHub"))
		ShellExecuteA(nullptr, "open", kGithubUrl, nullptr, nullptr, SW_SHOWNORMAL);
	ImGui::TextUnformatted(kGithubUrl); // Selectable/copyable fallback.
}

// ---------------------------------------------------------------------------
// Addon metadata (read by ReShade)
// ---------------------------------------------------------------------------

extern "C" __declspec(dllexport) const char *NAME = "MintCap";
extern "C" __declspec(dllexport) const char *DESCRIPTION =
	"Caps the game's frame rate to a limit you choose. Made by Nipeno.";

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		if (!reshade::register_addon(hModule))
			return FALSE;
		timeBeginPeriod(1); // Tighten sleep granularity for the sleep_for fallback path.
		// High-resolution waitable timer (Win10 1803+). Null on older OS -> sleep fallback.
		g_timer = CreateWaitableTimerExW(nullptr, nullptr,
			CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		if (g_timer == nullptr)
			mc_log(reshade::log_level::warning,
			        "High-resolution timer unavailable (error %lu); using the sleep fallback.",
			        GetLastError());
		mc_log(reshade::log_level::info, "MintCap v%s loaded - wait path: %s.",
		        MINTCAP_VERSION_STR, g_timer != nullptr ? "high-resolution timer" : "sleep fallback");
		reshade::register_event<reshade::addon_event::init_effect_runtime>(on_init_effect_runtime);
		reshade::register_event<reshade::addon_event::destroy_effect_runtime>(on_destroy_effect_runtime);
		reshade::register_event<reshade::addon_event::present>(on_present);
		// The title is also the key ReShade stores this window's layout under in
		// ReShade.ini - renaming it throws away every user's saved position/dock slot.
		reshade::register_overlay("MintCap", draw_overlay);
		break;
	case DLL_PROCESS_DETACH:
		mc_log(reshade::log_level::debug, "Unloading.");
		reshade::unregister_addon(hModule);
		if (g_timer != nullptr)
			CloseHandle(g_timer);
		timeEndPeriod(1);
		break;
	}
	return TRUE;
}
