// Switches a game's byte patches on and off; the patches are the JSON make_patcher.py appends to this exe
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <commctrl.h>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace
{
	struct Json
	{
		enum class Type { null, boolean, number, string, array, object };

		Type										type = Type::null;
		double										number = 0.0;
		std::string									text;
		std::vector<Json>							items;
		std::vector<std::pair<std::string, Json>>	members;

		const Json& at ( const std::string_view key ) const
		{
			static const Json	none;

			const auto	m = std::ranges::find ( members, key, &std::pair<std::string, Json>::first );

			return m != members.end () ? m->second : none;
		}
	};

	struct JsonReader
	{
		const char*	p = nullptr;
		const char*	end = nullptr;
		bool		failed = false;

		bool eat ( const char c )
		{
			while ( p < end && ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ) )
				++p;

			if ( p < end && *p == c )
			{
				++p;
				return true;
			}

			return false;
		}

		std::string string ()
		{
			std::string	out;

			if ( ! eat ( '"' ) )
			{
				failed = true;
				return out;
			}

			while ( p < end && *p != '"' )
			{
				if ( *p != '\\' )
				{
					out += *p++;
					continue;
				}

				if ( ++p >= end )
					break;

				switch ( const auto c = *p++ )
				{
					case 'n':	out += '\n';	break;
					case 't':	out += '\t';	break;
					case 'r':	out += '\r';	break;
					case 'b':	out += '\b';	break;
					case 'f':	out += '\f';	break;
					case 'u':
					{
						if ( end - p < 4 )
						{
							failed = true;
							return out;
						}

						const auto	cp = std::strtoul ( std::string ( p, 4 ).c_str (), nullptr, 16 );
						p += 4;

						if ( cp < 0x80 )
							out += char ( cp );
						else if ( cp < 0x800 )
							out += { char ( 0xc0 | cp >> 6 ), char ( 0x80 | ( cp & 0x3f ) ) };
						else
							out += { char ( 0xe0 | cp >> 12 ), char ( 0x80 | ( cp >> 6 & 0x3f ) ), char ( 0x80 | ( cp & 0x3f ) ) };
						break;
					}
					default:	out += c;		break;
				}
			}

			if ( ! eat ( '"' ) )
				failed = true;

			return out;
		}

		Json value ()
		{
			Json	v;

			if ( eat ( '{' ) )
			{
				v.type = Json::Type::object;
				if ( eat ( '}' ) )
					return v;

				do
				{
					auto	key = string ();
					if ( ! eat ( ':' ) )
						failed = true;

					v.members.emplace_back ( std::move ( key ), value () );
				}
				while ( ! failed && eat ( ',' ) );

				if ( ! eat ( '}' ) )
					failed = true;
			}
			else if ( eat ( '[' ) )
			{
				v.type = Json::Type::array;
				if ( eat ( ']' ) )
					return v;

				do
					v.items.push_back ( value () );
				while ( ! failed && eat ( ',' ) );

				if ( ! eat ( ']' ) )
					failed = true;
			}
			else if ( p < end && *p == '"' )
			{
				v.type = Json::Type::string;
				v.text = string ();
			}
			else if ( std::string_view ( p, end ).starts_with ( "true" ) || std::string_view ( p, end ).starts_with ( "false" ) )
			{
				v.type = Json::Type::boolean;
				v.number = *p == 't';
				p += *p == 't' ? 4 : 5;
			}
			else if ( std::string_view ( p, end ).starts_with ( "null" ) )
				p += 4;
			else
			{
				char*	next = nullptr;

				v.type = Json::Type::number;
				v.number = std::strtod ( p, &next );
				if ( next == p )
					failed = true;

				p = next;
			}

			return v;
		}
	};

	// Where a patch's value goes in a block's patched bytes: the slider's min..max maps linearly onto from..to
	struct Slot
	{
		size_t		at = 0;
		std::string	type;	// "f32", "i8", "u8", "i16", "u16", "i32", "u32", little endian
		double		from = 0.0;
		double		to = 0.0;

		size_t size () const	{	return type.ends_with ( "32" ) ? 4 : type.ends_with ( "16" ) ? 2 : 1;	}
	};

	struct Block
	{
		std::wstring			file;	// relative to the patcher's folder
		int64_t					offset = 0;
		std::vector<uint8_t>	original;
		std::vector<uint8_t>	patched;
		std::optional<Slot>		slot;
	};

	struct Value
	{
		double			min = 0.0;
		double			max = 100.0;
		double			fallback = 0.0;
		double			step = 1.0;
		std::wstring	unit;
	};

	struct Patch
	{
		std::wstring		what;	// "{}" shows the value there
		std::vector<Block>	blocks;
		std::optional<Value>	value;
		bool				on = false;
		double				chosen = 0.0;
		HWND				check = nullptr;
		HWND				slider = nullptr;
	};

	std::wstring		title;
	std::wstring		exeName;
	std::wstring		folder;
	std::vector<Patch>	patches;
	HWND				window = nullptr;
	HWND				header = nullptr;
	HWND				line = nullptr;		// between header and list
	HWND				list = nullptr;		// the patch rows, scrolled
	HWND				status = nullptr;
	HFONT				font = nullptr;

	// In pixels; set by layout (), scrollY by scrollTo ()
	int					contentHeight = 0;
	int					listTop = 0;
	int					listBottom = 0;
	int					rowHeight = 0;
	int					frameWidth = 0;
	int					minFrameHeight = 0;
	int					maxFrameHeight = 0;
	int					scrollY = 0;

	constexpr auto	headerText = L"Tick a patch to turn it on, untick it to turn it off. The sliders set a patch's value. Every change goes "
								 L"into the game right away. Close ultraPatcher before you start the game.";

	std::wstring widen ( const std::string_view s )
	{
		std::wstring	out ( MultiByteToWideChar ( CP_UTF8, 0, s.data (), int ( s.size () ), nullptr, 0 ), L'\0' );
		MultiByteToWideChar ( CP_UTF8, 0, s.data (), int ( s.size () ), out.data (), int ( out.size () ) );

		return out;
	}

	std::vector<uint8_t> hexBytes ( const std::string& hex )
	{
		std::vector<uint8_t>	out;

		for ( size_t i = 0; i + 1 < hex.size (); i += 2 )
			out.push_back ( uint8_t ( std::strtoul ( hex.substr ( i, 2 ).c_str (), nullptr, 16 ) ) );

		return out;
	}

	std::wstring systemText ( const DWORD error )
	{
		wchar_t*	text = nullptr;
		FormatMessageW ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, 0,
						 reinterpret_cast<wchar_t*> ( &text ), 0, nullptr );

		std::wstring	out = text ? text : L"";
		LocalFree ( text );

		return out;
	}

	void fail ( const std::wstring& text )
	{
		MessageBoxW ( window, text.c_str (), L"ultraPatcher", MB_OK | MB_ICONERROR );
	}

	// The JSON make_patcher.py appended: the text, its size as u32 and "ULTRAPAT", in front of the signature if there is one
	std::string appendedJson ()
	{
		wchar_t	self[ 32768 ];
		GetModuleFileNameW ( nullptr, self, DWORD ( std::size ( self ) ) );

		const auto	file = CreateFileW ( self, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
		if ( file == INVALID_HANDLE_VALUE )
			return {};

		LARGE_INTEGER	size {};
		GetFileSizeEx ( file, &size );

		std::vector<uint8_t>	exe ( size_t ( size.QuadPart ) );
		DWORD					done = 0;
		const auto				read = ReadFile ( file, exe.data (), DWORD ( exe.size () ), &done, nullptr ) && done == exe.size ();
		CloseHandle ( file );

		if ( ! read )
			return {};

		const auto	dos = reinterpret_cast<const IMAGE_DOS_HEADER*> ( exe.data () );
		const auto	nt = reinterpret_cast<const IMAGE_NT_HEADERS*> ( exe.data () + dos->e_lfanew );
		const auto	signature = nt->OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_SECURITY ].VirtualAddress;
		const auto	end = signature ? size_t ( signature ) : exe.size ();

		uint32_t	length = 0;
		if ( end < 12 || std::memcmp ( exe.data () + end - 8, "ULTRAPAT", 8 ) != 0 )
			return {};

		std::memcpy ( &length, exe.data () + end - 12, 4 );
		if ( length > end - 12 )
			return {};

		return { reinterpret_cast<const char*> ( exe.data () ) + end - 12 - length, length };
	}

	std::wstring load ()
	{
		const auto	text = appendedJson ();
		if ( text.empty () )
			return L"This ultraPatcher has no patches inside.";

		JsonReader	reader { text.data (), text.data () + text.size () };
		const auto	root = reader.value ();
		if ( reader.failed )
			return L"The patches inside this ultraPatcher are damaged.";

		title = widen ( root.at ( "title" ).text );
		exeName = widen ( root.at ( "exe" ).text );

		for ( const auto& p : root.at ( "patches" ).items )
		{
			Patch	patch { .what = widen ( p.at ( "what" ).text ) };

			if ( const auto& v = p.at ( "value" ); v.type == Json::Type::object )
			{
				patch.value = Value { v.at ( "min" ).number, v.at ( "max" ).number, v.at ( "default" ).number, v.at ( "step" ).number,
									  widen ( v.at ( "unit" ).text ) };
				patch.chosen = patch.value->fallback;
			}

			for ( const auto& b : p.at ( "blocks" ).items )
			{
				Block	block { widen ( b.at ( "file" ).text ), int64_t ( b.at ( "offset" ).number ), hexBytes ( b.at ( "original" ).text ),
								hexBytes ( b.at ( "patched" ).text ) };

				if ( const auto& s = b.at ( "slot" ); s.type == Json::Type::object )
					block.slot = Slot { size_t ( s.at ( "at" ).number ), s.at ( "type" ).text, s.at ( "from" ).number, s.at ( "to" ).number };

				patch.blocks.push_back ( std::move ( block ) );
			}

			patches.push_back ( std::move ( patch ) );
		}

		return {};
	}

	// The block's patched bytes with value in its slot
	std::vector<uint8_t> patchedBytes ( const Patch& patch, const Block& block, const double value )
	{
		auto	out = block.patched;
		if ( ! block.slot || ! patch.value )
			return out;

		const auto&	s = *block.slot;
		const auto&	v = *patch.value;
		const auto	game = s.from + ( value - v.min ) / ( v.max - v.min ) * ( s.to - s.from );

		if ( s.type == "f32" )
		{
			const auto	f = float ( game );
			std::memcpy ( out.data () + s.at, &f, 4 );
			return out;
		}

		const auto	bits = int ( s.size () * 8 );
		const auto	lo = s.type[ 0 ] == 'i' ? -( int64_t ( 1 ) << ( bits - 1 ) ) : 0;
		const auto	hi = s.type[ 0 ] == 'i' ? ( int64_t ( 1 ) << ( bits - 1 ) ) - 1 : ( int64_t ( 1 ) << bits ) - 1;
		const auto	n = std::clamp ( int64_t ( std::llround ( game ) ), lo, hi );

		for ( size_t i = 0; i < s.size (); ++i )
			out[ s.at + i ] = uint8_t ( uint64_t ( n ) >> ( 8 * i ) );

		return out;
	}

	// The slider value the slot in bytes holds, snapped to the step
	double slotValue ( const Patch& patch, const Block& block, const std::vector<uint8_t>& bytes )
	{
		const auto&	s = *block.slot;
		const auto&	v = *patch.value;
		double		game = 0.0;

		if ( s.type == "f32" )
		{
			float	f = 0.0f;
			std::memcpy ( &f, bytes.data () + s.at, 4 );
			game = f;
		}
		else
		{
			uint64_t	n = 0;
			for ( size_t i = 0; i < s.size (); ++i )
				n |= uint64_t ( bytes[ s.at + i ] ) << ( 8 * i );

			const auto	bits = s.size () * 8;
			game = s.type[ 0 ] == 'i' && ( n >> ( bits - 1 ) ) ? double ( int64_t ( n ) - ( int64_t ( 1 ) << bits ) ) : double ( n );
		}

		const auto	value = v.min + ( game - s.from ) / ( s.to - s.from ) * ( v.max - v.min );

		return std::clamp ( v.min + std::round ( ( value - v.min ) / v.step ) * v.step, v.min, v.max );
	}

	// True if bytes are the block's patched bytes, the slot aside
	bool isPatched ( const Block& block, const std::vector<uint8_t>& bytes )
	{
		for ( size_t i = 0; i < bytes.size (); ++i )
			if ( bytes[ i ] != block.patched[ i ] && ! ( block.slot && i >= block.slot->at && i < block.slot->at + block.slot->size () ) )
				return false;

		return true;
	}

	// The game's files, opened by open () and held until ultraPatcher exits
	std::vector<std::pair<std::wstring, HANDLE>>	files;

	HANDLE handleOf ( const std::wstring& file )
	{
		const auto	f = std::ranges::find ( files, file, &std::pair<std::wstring, HANDLE>::first );

		return f != files.end () ? f->second : INVALID_HANDLE_VALUE;
	}

	// Opens file for writing unless it already is; on failure, why
	std::wstring open ( const std::wstring& file )
	{
		if ( handleOf ( file ) != INVALID_HANDLE_VALUE )
			return {};

		const auto	handle = CreateFileW ( ( folder + file ).c_str (), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
										   0, nullptr );
		if ( handle != INVALID_HANDLE_VALUE )
		{
			files.emplace_back ( file, handle );
			return {};
		}

		const auto	error = GetLastError ();
		if ( error == ERROR_FILE_NOT_FOUND && file == exeName )
			return L"Put ultraPatcher next to " + exeName + L" and start it there.";
		if ( error == ERROR_FILE_NOT_FOUND )
			return file + L" is missing next to ultraPatcher.";
		if ( error == ERROR_SHARING_VIOLATION )
			return L"Close the game first, " + file + L" is in use.";

		return L"Can't open " + file + L": " + systemText ( error );
	}

	bool readAt ( const HANDLE file, const int64_t offset, std::vector<uint8_t>& bytes )
	{
		OVERLAPPED	at { .Offset = DWORD ( offset ), .OffsetHigh = DWORD ( offset >> 32 ) };
		DWORD		done = 0;

		return ReadFile ( file, bytes.data (), DWORD ( bytes.size () ), &done, &at ) && done == bytes.size ();
	}

	bool writeAt ( const HANDLE file, const int64_t offset, const std::vector<uint8_t>& bytes )
	{
		OVERLAPPED	at { .Offset = DWORD ( offset ), .OffsetHigh = DWORD ( offset >> 32 ) };
		DWORD		done = 0;

		return WriteFile ( file, bytes.data (), DWORD ( bytes.size () ), &done, &at ) && done == bytes.size ();
	}

	// Sets each patch's state from the game's bytes; a patch neither fully original nor fully patched is an error
	std::wstring detect ()
	{
		for ( auto& patch : patches )
		{
			bool	allOriginal = true;
			bool	allPatched = true;
			std::optional<double>	slotted;

			for ( const auto& block : patch.blocks )
			{
				if ( auto error = open ( block.file ); ! error.empty () )
					return error;

				std::vector<uint8_t>	bytes ( block.original.size () );
				const auto				read = readAt ( handleOf ( block.file ), block.offset, bytes );

				allOriginal = allOriginal && read && bytes == block.original;
				allPatched = allPatched && read && isPatched ( block, bytes );

				if ( read && block.slot && patch.value && ! slotted )
					slotted = slotValue ( patch, block, bytes );
			}

			if ( ! allOriginal && ! allPatched )
				return L"This ultraPatcher is for a different version of " + title + L". Nothing was changed.";

			patch.on = ! allOriginal;
			if ( patch.on && slotted )
				patch.chosen = *slotted;
		}

		return {};
	}

	// Writes the patch switched to on with value
	bool change ( Patch& patch, const bool on, const double value )
	{
		for ( const auto& block : patch.blocks )
		{
			if ( ! writeAt ( handleOf ( block.file ), block.offset, on ? patchedBytes ( patch, block, value ) : block.original ) )
			{
				fail ( L"Writing " + block.file + L" failed: " + systemText ( GetLastError () ) );
				return false;
			}
		}

		patch.on = on;
		patch.chosen = value;

		return true;
	}

	std::wstring label ( const Patch& patch, const double value )
	{
		if ( ! patch.value )
			return patch.what;

		const auto	decimals = patch.value->step >= 1.0 ? 0 : patch.value->step >= 0.1 ? 1 : 2;
		wchar_t		number[ 32 ];
		swprintf_s ( number, L"%.*f", decimals, value );

		auto	text = patch.what;
		if ( const auto at = text.find ( L"{}" ); at != std::wstring::npos )
			return text.replace ( at, 2, number );

		return text + L" " + number + patch.value->unit;
	}

	double sliderValue ( const Patch& patch )
	{
		return patch.value->min + double ( SendMessageW ( patch.slider, TBM_GETPOS, 0, 0 ) ) * patch.value->step;
	}

	void showState ( const Patch& patch )
	{
		SendMessageW ( patch.check, BM_SETCHECK, patch.on ? BST_CHECKED : BST_UNCHECKED, 0 );
		SetWindowTextW ( patch.check, label ( patch, patch.chosen ).c_str () );

		if ( patch.slider )
		{
			SendMessageW ( patch.slider, TBM_SETPOS, TRUE, std::lround ( ( patch.chosen - patch.value->min ) / patch.value->step ) );
			EnableWindow ( patch.slider, patch.on );
		}
	}

	void scrollTo ( const int y )
	{
		RECT	client {};
		GetClientRect ( list, &client );

		const auto	to = std::clamp ( y, 0, std::max ( 0, contentHeight - int ( client.bottom ) ) );

		ScrollWindowEx ( list, 0, scrollY - to, nullptr, nullptr, nullptr, nullptr, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE );
		scrollY = to;

		const SCROLLINFO	info { sizeof ( info ), SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL, 0, contentHeight - 1, UINT ( client.bottom ), scrollY };
		SetScrollInfo ( list, SB_VERT, &info, TRUE );
	}

	// Places every control for dpi and sizes the window to fit, up to the height of the screen's work area; centred on the
	// screen, or at the top left of suggested if given
	void layout ( const UINT dpi, const RECT* suggested )
	{
		const auto	px = [ dpi ] ( const int v ) { return MulDiv ( v, int ( dpi ), 96 ); };

		NONCLIENTMETRICSW	metrics { .cbSize = sizeof ( metrics ) };
		SystemParametersInfoForDpi ( SPI_GETNONCLIENTMETRICS, sizeof ( metrics ), &metrics, 0, dpi );

		if ( font )
			DeleteObject ( font );
		font = CreateFontIndirectW ( &metrics.lfMessageFont );

		const auto	dc = GetDC ( window );
		const auto	old = SelectObject ( dc, font );
		const auto	textWidth = [ dc ] ( const std::wstring& s )
		{
			SIZE	size {};
			GetTextExtentPoint32W ( dc, s.c_str (), int ( s.size () ), &size );
			return int ( size.cx );
		};

		TEXTMETRICW	tm {};
		GetTextMetricsW ( dc, &tm );

		auto	width = px ( 360 );
		for ( const auto& p : patches )
		{
			width = std::max ( width, px ( 24 ) + textWidth ( label ( p, p.value ? p.value->max : 0.0 ) ) );
			width = std::max ( width, px ( 24 ) + textWidth ( label ( p, p.value ? p.value->min : 0.0 ) ) );
		}

		const auto	margin = px ( 16 );
		const auto	clientWidth = margin + width + margin + GetSystemMetricsForDpi ( SM_CXVSCROLL, dpi );
		const auto	headerWidth = clientWidth - 2 * margin;

		RECT	headerSize { 0, 0, headerWidth, 0 };
		DrawTextW ( dc, headerText, -1, &headerSize, DT_CALCRECT | DT_WORDBREAK );

		SelectObject ( dc, old );
		ReleaseDC ( window, dc );

		auto		y = 0;

		rowHeight = int ( tm.tmHeight ) + px ( 10 );

		const auto	place = [ & ] ( const HWND control, const int x, const int h )
		{
			SendMessageW ( control, WM_SETFONT, WPARAM ( font ), TRUE );
			MoveWindow ( control, margin + x, y, width - x, h, TRUE );
			y += h;
		};

		scrollY = 0;

		for ( const auto& p : patches )
		{
			place ( p.check, 0, rowHeight );
			if ( p.slider )
			{
				SendMessageW ( p.slider, TBM_SETTHUMBLENGTH, px ( 20 ), 0 );
				place ( p.slider, px ( 18 ), px ( 30 ) );
			}
		}

		contentHeight = y;

		SendMessageW ( header, WM_SETFONT, WPARAM ( font ), TRUE );
		MoveWindow ( header, margin, margin, headerWidth, headerSize.bottom, TRUE );
		MoveWindow ( line, margin, margin + headerSize.bottom + px ( 10 ), headerWidth, 2, TRUE );

		RECT	bar {};
		SendMessageW ( status, WM_SETFONT, WPARAM ( font ), FALSE );
		SendMessageW ( status, WM_SIZE, 0, 0 );
		GetWindowRect ( status, &bar );

		listTop = margin + headerSize.bottom + px ( 10 ) + 2 + px ( 10 );
		listBottom = margin + bar.bottom - bar.top;

		const auto	style = DWORD ( GetWindowLongW ( window, GWL_STYLE ) );
		const auto	frameOf = [ & ] ( const int clientHeight )
		{
			RECT	frame { 0, 0, clientWidth, clientHeight };
			AdjustWindowRectExForDpi ( &frame, style, FALSE, 0, dpi );
			return frame;
		};

		frameWidth = frameOf ( 0 ).right - frameOf ( 0 ).left;
		minFrameHeight = frameOf ( listTop + 2 * rowHeight + listBottom ).bottom - frameOf ( 0 ).top;
		maxFrameHeight = frameOf ( listTop + contentHeight + listBottom ).bottom - frameOf ( 0 ).top;

		MONITORINFO	monitor { sizeof ( monitor ) };
		GetMonitorInfoW ( suggested ? MonitorFromRect ( suggested, MONITOR_DEFAULTTONEAREST ) : MonitorFromWindow ( window, MONITOR_DEFAULTTONEAREST ),
						  &monitor );

		const auto&	work = monitor.rcWork;
		const auto	height = std::min ( maxFrameHeight, int ( work.bottom - work.top ) );
		const auto	left = suggested ? suggested->left : ( work.left + work.right - frameWidth ) / 2;
		const auto	top = suggested ? suggested->top : ( work.top + work.bottom - height ) / 2;

		SetWindowPos ( window, nullptr, left, top, frameWidth, height, SWP_NOZORDER | SWP_NOACTIVATE );
		scrollTo ( 0 );
	}

	LRESULT CALLBACK listProc ( const HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam )
	{
		switch ( message )
		{
			case WM_VSCROLL:
			{
				SCROLLINFO	info { sizeof ( info ), SIF_ALL };
				GetScrollInfo ( hwnd, SB_VERT, &info );

				switch ( LOWORD ( wParam ) )
				{
					case SB_LINEUP:			scrollTo ( scrollY - rowHeight );			break;
					case SB_LINEDOWN:		scrollTo ( scrollY + rowHeight );			break;
					case SB_PAGEUP:			scrollTo ( scrollY - int ( info.nPage ) );	break;
					case SB_PAGEDOWN:		scrollTo ( scrollY + int ( info.nPage ) );	break;
					case SB_THUMBTRACK:
					case SB_THUMBPOSITION:	scrollTo ( info.nTrackPos );				break;
					case SB_TOP:			scrollTo ( 0 );								break;
					case SB_BOTTOM:			scrollTo ( contentHeight );					break;
				}
				return 0;
			}

			case WM_MOUSEWHEEL:
				scrollTo ( scrollY - GET_WHEEL_DELTA_WPARAM ( wParam ) * 3 * rowHeight / WHEEL_DELTA );
				return 0;

			case WM_COMMAND:
			case WM_HSCROLL:
			case WM_CTLCOLORSTATIC:
				return SendMessageW ( window, message, wParam, lParam );
		}

		return DefWindowProcW ( hwnd, message, wParam, lParam );
	}

	// Shows text in the status bar for a few seconds, then "Ready" again
	void report ( const std::wstring& text )
	{
		SetWindowTextW ( status, text.c_str () );
		SetTimer ( window, 1, 4000, nullptr );
	}

	LRESULT CALLBACK windowProc ( const HWND hwnd, const UINT message, const WPARAM wParam, const LPARAM lParam )
	{
		switch ( message )
		{
			case WM_COMMAND:
				if ( HIWORD ( wParam ) == BN_CLICKED && LOWORD ( wParam ) >= 1 && LOWORD ( wParam ) <= patches.size () )
				{
					auto&	patch = patches[ LOWORD ( wParam ) - 1 ];
					if ( change ( patch, ! patch.on, patch.chosen ) )
						report ( ( patch.on ? L"Applied: " : L"Removed: " ) + label ( patch, patch.chosen ) );

					showState ( patch );
				}
				return 0;

			case WM_HSCROLL:
				for ( auto& patch : patches )
				{
					if ( patch.slider != HWND ( lParam ) )
						continue;

					const auto	value = sliderValue ( patch );
					SetWindowTextW ( patch.check, label ( patch, value ).c_str () );

					if ( LOWORD ( wParam ) != TB_THUMBTRACK )
					{
						if ( value == patch.chosen || change ( patch, true, value ) )
							report ( L"Applied: " + label ( patch, patch.chosen ) );

						showState ( patch );
					}
				}
				return 0;

			case WM_SIZE:
				SendMessageW ( status, WM_SIZE, 0, 0 );
				MoveWindow ( list, 0, listTop, LOWORD ( lParam ), std::max ( 0, HIWORD ( lParam ) - listTop - listBottom ), TRUE );
				scrollTo ( scrollY );
				return 0;

			case WM_GETMINMAXINFO:
				if ( frameWidth > 0 )
				{
					auto&	info = *reinterpret_cast<MINMAXINFO*> ( lParam );
					info.ptMinTrackSize = { frameWidth, minFrameHeight };
					info.ptMaxTrackSize = { frameWidth, maxFrameHeight };
				}
				return 0;

			case WM_TIMER:
				KillTimer ( hwnd, 1 );
				SetWindowTextW ( status, L"Ready" );
				return 0;

			case WM_CTLCOLORSTATIC:
				return LRESULT ( GetSysColorBrush ( COLOR_WINDOW ) );

			case WM_DPICHANGED:
				layout ( HIWORD ( wParam ), reinterpret_cast<const RECT*> ( lParam ) );
				return 0;

			case WM_DESTROY:
				PostQuitMessage ( 0 );
				return 0;
		}

		return DefWindowProcW ( hwnd, message, wParam, lParam );
	}
}

int WINAPI wWinMain ( const HINSTANCE instance, HINSTANCE, PWSTR, const int show )
{
	const INITCOMMONCONTROLSEX	controls { sizeof ( controls ), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
	InitCommonControlsEx ( &controls );

	wchar_t	self[ 32768 ];
	GetModuleFileNameW ( nullptr, self, DWORD ( std::size ( self ) ) );
	folder = self;
	folder.resize ( folder.find_last_of ( L'\\' ) + 1 );

	if ( auto error = load (); ! error.empty () || ! ( error = detect () ).empty () )
	{
		fail ( error );
		return 1;
	}

	const WNDCLASSW	windowClass { .lpfnWndProc = windowProc, .hInstance = instance, .hIcon = LoadIconW ( nullptr, IDI_APPLICATION ),
								  .hCursor = LoadCursorW ( nullptr, IDC_ARROW ), .hbrBackground = HBRUSH ( COLOR_WINDOW + 1 ),
								  .lpszClassName = L"ultraPatcher" };
	RegisterClassW ( &windowClass );

	const WNDCLASSW	listClass { .lpfnWndProc = listProc, .hInstance = instance, .hCursor = LoadCursorW ( nullptr, IDC_ARROW ),
								.hbrBackground = HBRUSH ( COLOR_WINDOW + 1 ), .lpszClassName = L"ultraPatcherList" };
	RegisterClassW ( &listClass );

	window = CreateWindowExW ( 0, L"ultraPatcher", ( title + L" patches" ).c_str (),
							   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN, CW_USEDEFAULT,
							   CW_USEDEFAULT, 400, 300, nullptr, nullptr, instance, nullptr );

	header = CreateWindowExW ( 0, L"STATIC", headerText, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, window, nullptr, instance, nullptr );
	line = CreateWindowExW ( 0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 0, 0, 0, 0, window, nullptr, instance, nullptr );
	list = CreateWindowExW ( WS_EX_CONTROLPARENT, L"ultraPatcherList", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN, 0, 0, 0,
							 0, window, nullptr, instance, nullptr );
	status = CreateWindowExW ( 0, STATUSCLASSNAMEW, L"Ready", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, window, nullptr, instance, nullptr );

	for ( size_t i = 0; i < patches.size (); ++i )
	{
		auto&	p = patches[ i ];

		p.check = CreateWindowExW ( 0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX, 0, 0, 0, 0, list,
									HMENU ( i + 1 ), instance, nullptr );

		if ( p.value )
		{
			p.slider = CreateWindowExW ( 0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_NOTICKS | TBS_FIXEDLENGTH, 0, 0,
										 0, 0, list, nullptr, instance, nullptr );
			SendMessageW ( p.slider, TBM_SETRANGEMIN, FALSE, 0 );
			SendMessageW ( p.slider, TBM_SETRANGEMAX, FALSE, std::lround ( ( p.value->max - p.value->min ) / p.value->step ) );
		}

		showState ( p );
	}

	layout ( GetDpiForWindow ( window ), nullptr );
	ShowWindow ( window, show );

	MSG	message;
	while ( GetMessageW ( &message, nullptr, 0, 0 ) > 0 )
	{
		if ( message.message == WM_MOUSEWHEEL )
			message.hwnd = list;

		if ( IsDialogMessageW ( window, &message ) )
			continue;

		TranslateMessage ( &message );
		DispatchMessageW ( &message );
	}

	return 0;
}
