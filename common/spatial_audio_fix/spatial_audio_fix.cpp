#include <MinHook.h>
#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmreg.h>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <mutex>

//-----------------------------------------------------------------------------

// XInput stand-in for games on Ubisoft's Dare sound engine (Watch_Dogs, Far Cry 4, Far Cry Primal), built once per DLL
// name a game imports (SAF_SYSTEM_DLL and its .def, named by the game's CMakeLists): XInput goes to the system DLL, and
// the output device is reported to XAudio2 2.7 as PCM when Windows spatial sound hands the game a Dolby or DTS bitstream

namespace
{
	struct Xaudio2DeviceDetails
	{
		wchar_t					deviceId[ 256 ];
		wchar_t					displayName[ 256 ];
		UINT32					role;
		WAVEFORMATEXTENSIBLE	outputFormat;
	};

	constexpr GUID	clsidXaudio2 { 0x5a508685, 0xa254, 0x4fba, { 0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf } };
	constexpr GUID	clsidXaudio2Debug { 0xdb05ea35, 0x0329, 0x4d4b, { 0xa5, 0x3a, 0x6d, 0xea, 0xd0, 0x3d, 0x38, 0x52 } };

	using CoCreateInstanceFn = HRESULT ( WINAPI* ) ( REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID* );
	using GetDeviceDetailsFn = HRESULT ( STDMETHODCALLTYPE* ) ( IUnknown*, UINT32, Xaudio2DeviceDetails* );

	HMODULE				self = nullptr;
	CoCreateInstanceFn	realCoCreateInstance = nullptr;
	GetDeviceDetailsFn	realGetDeviceDetails = nullptr;

	void log ( const char* format, ... )
	{
		wchar_t	path[ MAX_PATH ];
		GetModuleFileNameW ( self, path, MAX_PATH );
		if ( auto* slash = std::wcsrchr ( path, L'\\' ) )
			*slash = 0;
		wcscat_s ( path, L"\\spatial-audio-fix.log" );

		FILE*	file = nullptr;
		if ( _wfopen_s ( &file, path, L"a" ) != 0 || file == nullptr )
			return;

		SYSTEMTIME	now;
		GetLocalTime ( &now );
		std::fprintf ( file, "%02u:%02u:%02u.%03u ", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds );

		va_list	args;
		va_start ( args, format );
		std::vfprintf ( file, format, args );
		va_end ( args );

		std::fputc ( '\n', file );
		std::fclose ( file );
	}

	HRESULT STDMETHODCALLTYPE onGetDeviceDetails ( IUnknown* xaudio2, UINT32 index, Xaudio2DeviceDetails* details )
	{
		const auto	hr = realGetDeviceDetails ( xaudio2, index, details );
		if ( FAILED ( hr ) || details == nullptr )
			return hr;

		auto&		format = details->outputFormat;
		const auto&	sub = format.SubFormat;
		const bool	bitstream = format.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE
						&& ! IsEqualGUID ( sub, KSDATAFORMAT_SUBTYPE_PCM ) && ! IsEqualGUID ( sub, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT );

		log ( "device %u \"%ls\": %u channels, %u Hz, %u bits, mask 0x%x, sub-format %08lx-%04hx-%04hx-%02x%02x-%02x%02x%02x%02x%02x%02x: %s",
			  index, details->displayName, format.Format.nChannels, format.Format.nSamplesPerSec, format.Format.wBitsPerSample, format.dwChannelMask,
			  sub.Data1, sub.Data2, sub.Data3, sub.Data4[ 0 ], sub.Data4[ 1 ], sub.Data4[ 2 ], sub.Data4[ 3 ], sub.Data4[ 4 ], sub.Data4[ 5 ], sub.Data4[ 6 ], sub.Data4[ 7 ],
			  bitstream ? "reported as PCM" : "left as is" );

		if ( bitstream )
			format.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

		return hr;
	}

	// GetDeviceDetails is slot 4 of IXAudio2 (xaudio2.h of the DirectX SDK); one hook serves every instance
	void hookXaudio2 ( IUnknown* xaudio2 )
	{
		static std::once_flag	once;

		std::call_once ( once, [ xaudio2 ]
		{
			auto**	vtable = *reinterpret_cast<void***> ( xaudio2 );
			auto	status = MH_CreateHook ( vtable[ 4 ], reinterpret_cast<LPVOID> ( &onGetDeviceDetails ), reinterpret_cast<LPVOID*> ( &realGetDeviceDetails ) );
			if ( status == MH_OK )
				status = MH_EnableHook ( vtable[ 4 ] );

			log ( "XAudio2 2.7 created, GetDeviceDetails hook: MinHook %d", int ( status ) );
		} );
	}

	HRESULT WINAPI onCoCreateInstance ( REFCLSID clsid, LPUNKNOWN outer, DWORD context, REFIID iid, LPVOID* object )
	{
		const auto	hr = realCoCreateInstance ( clsid, outer, context, iid, object );
		if ( SUCCEEDED ( hr ) && object != nullptr && *object != nullptr && ( IsEqualGUID ( clsid, clsidXaudio2 ) || IsEqualGUID ( clsid, clsidXaudio2Debug ) ) )
			hookXaudio2 ( static_cast<IUnknown*> ( *object ) );

		return hr;
	}

	void installHook ()
	{
		auto	ole32 = GetModuleHandleW ( L"ole32.dll" );
		if ( ole32 == nullptr )
			ole32 = LoadLibraryW ( L"ole32.dll" );

		auto*	target = ole32 ? reinterpret_cast<LPVOID> ( GetProcAddress ( ole32, "CoCreateInstance" ) ) : nullptr;
		if ( target == nullptr )
		{
			log ( "CoCreateInstance not found, nothing hooked" );
			return;
		}

		const auto	init = MH_Initialize ();
		const auto	create = MH_CreateHook ( target, reinterpret_cast<LPVOID> ( &onCoCreateInstance ), reinterpret_cast<LPVOID*> ( &realCoCreateInstance ) );
		const auto	enable = MH_EnableHook ( target );
		log ( "CoCreateInstance hook: MinHook init %d create %d enable %d", int ( init ), int ( create ), int ( enable ) );
	}

	//-------------------------------------------------------------------------

	HMODULE systemXinput ()
	{
		static HMODULE	module = []
		{
			wchar_t		path[ MAX_PATH ];
			const auto	length = GetSystemDirectoryW ( path, MAX_PATH );
			wcscpy_s ( path + length, MAX_PATH - length, L"\\" SAF_SYSTEM_DLL );
			return LoadLibraryW ( path );
		} ();

		return module;
	}

	template<typename Fn>
	Fn real ( Fn& slot, LPCSTR name )
	{
		if ( slot == nullptr )
			if ( auto module = systemXinput () )
				slot = reinterpret_cast<Fn> ( GetProcAddress ( module, name ) );

		return slot;
	}
}
//-----------------------------------------------------------------------------

// The system DLLs' exports (each .def picks its own and keeps the ordinals); the structures stay opaque, they only pass
// through

extern "C"
{
	DWORD WINAPI XInputGetState ( DWORD user, void* state )
	{
		static decltype ( &XInputGetState )	fn = nullptr;
		return real ( fn, "XInputGetState" ) ? fn ( user, state ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputSetState ( DWORD user, void* vibration )
	{
		static decltype ( &XInputSetState )	fn = nullptr;
		return real ( fn, "XInputSetState" ) ? fn ( user, vibration ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputGetCapabilities ( DWORD user, DWORD flags, void* capabilities )
	{
		static decltype ( &XInputGetCapabilities )	fn = nullptr;
		return real ( fn, "XInputGetCapabilities" ) ? fn ( user, flags, capabilities ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	void WINAPI XInputEnable ( BOOL enable )
	{
		static decltype ( &XInputEnable )	fn = nullptr;
		if ( real ( fn, "XInputEnable" ) )
			fn ( enable );
	}

	DWORD WINAPI XInputGetDSoundAudioDeviceGuids ( DWORD user, GUID* render, GUID* capture )
	{
		static decltype ( &XInputGetDSoundAudioDeviceGuids )	fn = nullptr;
		return real ( fn, "XInputGetDSoundAudioDeviceGuids" ) ? fn ( user, render, capture ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputGetBatteryInformation ( DWORD user, BYTE deviceType, void* battery )
	{
		static decltype ( &XInputGetBatteryInformation )	fn = nullptr;
		return real ( fn, "XInputGetBatteryInformation" ) ? fn ( user, deviceType, battery ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputGetKeystroke ( DWORD user, DWORD reserved, void* keystroke )
	{
		static decltype ( &XInputGetKeystroke )	fn = nullptr;
		return real ( fn, "XInputGetKeystroke" ) ? fn ( user, reserved, keystroke ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputGetStateEx ( DWORD user, void* state )
	{
		static decltype ( &XInputGetStateEx )	fn = nullptr;
		return real ( fn, MAKEINTRESOURCEA ( 100 ) ) ? fn ( user, state ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputWaitForGuideButton ( DWORD user, DWORD flags, void* unknown )
	{
		static decltype ( &XInputWaitForGuideButton )	fn = nullptr;
		return real ( fn, MAKEINTRESOURCEA ( 101 ) ) ? fn ( user, flags, unknown ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputCancelGuideButtonWait ( DWORD user )
	{
		static decltype ( &XInputCancelGuideButtonWait )	fn = nullptr;
		return real ( fn, MAKEINTRESOURCEA ( 102 ) ) ? fn ( user ) : ERROR_DEVICE_NOT_CONNECTED;
	}

	DWORD WINAPI XInputPowerOffController ( DWORD user )
	{
		static decltype ( &XInputPowerOffController )	fn = nullptr;
		return real ( fn, MAKEINTRESOURCEA ( 103 ) ) ? fn ( user ) : ERROR_DEVICE_NOT_CONNECTED;
	}
}
//-----------------------------------------------------------------------------

BOOL APIENTRY DllMain ( HMODULE module, DWORD reason, LPVOID )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		self = module;
		DisableThreadLibraryCalls ( module );

		wchar_t	exe[ MAX_PATH ];
		GetModuleFileNameW ( nullptr, exe, MAX_PATH );
		log ( "spatial audio fix attached to %ls", exe );

		installHook ();
	}
	else if ( reason == DLL_PROCESS_DETACH )
	{
		MH_DisableHook ( MH_ALL_HOOKS );
		MH_Uninitialize ();
	}

	return TRUE;
}
//-----------------------------------------------------------------------------
