#ifndef keyenum_h
#define keyenum_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          27/01/2006
 RCS:           $Id: keyenum.h,v 1.6 2009-01-20 04:38:46 cvsranojay Exp $
________________________________________________________________________

-*/

#include "enums.h"

namespace OD
{
    enum ButtonState 
    { //!< Qt's mouse/keyboard state values
			NoButton        = 0x00000000,
			LeftButton      = 0x00000001,
			RightButton     = 0x00000002,
			MidButton       = 0x00000004,
			MouseButtonMask = 0x000000ff,
			ShiftButton     = 0x02000000,
			ControlButton   = 0x04000000,
			AltButton       = 0x08000000,
			MetaButton      = 0x10000000,
			KeyButtonMask   = 0xfe000000,
			Keypad          = 0x20000000
    };

    enum KeyboardKey
    {
	NoKey		= 0x00000000,
	Escape		= 0x01000000,
	Tab		= 0x01000001,
	Backtab		= 0x01000002,
	Backspace	= 0x01000003,
	Return		= 0x01000004,
	Enter		= 0x01000005,
	Insert		= 0x01000006,
	Delete		= 0x01000007,
	Pause		= 0x01000008,
	Print		= 0x01000009,
	SysReq		= 0x0100000a,
	Clear		= 0x0100000b,
	Home		= 0x01000010,
	End		= 0x01000011,
	Left		= 0x01000012,
	Up		= 0x01000013,
	Right		= 0x01000014,
	Down		= 0x01000015,
	PageUp		= 0x01000016,
	PageDown	= 0x01000017,
	Shift		= 0x01000020,
	Control		= 0x01000021, //On Mac OS X, this corresponds to the
				      //Command keys.
	Meta		= 0x01000022, //On Mac OS X, this corresponds to the
				      //Control keys.
	Alt		= 0x01000023,
	AltGr		= 0x01001103,
	CapsLock	= 0x01000024,
	NumLock		= 0x01000025,
	ScrollLock	= 0x01000026,
	F1		= 0x01000030,
	F2		= 0x01000031,
	F3		= 0x01000032,
	F4		= 0x01000033,
	F5		= 0x01000034,
	F6		= 0x01000035,
	F7		= 0x01000036,
	F8		= 0x01000037,
	F9		= 0x01000038,
	F10		= 0x01000039,
	F11		= 0x0100003a,
	F12		= 0x0100003b,
	F13		= 0x0100003c,
	F14		= 0x0100003d,
	F15		= 0x0100003e,
	F16		= 0x0100003f,
	F17		= 0x01000040,
	F18		= 0x01000041,
	F19		= 0x01000042,
	F20		= 0x01000043,
	F21		= 0x01000044,
	F22		= 0x01000045,
	F23		= 0x01000046,
	F24		= 0x01000047,
	F25		= 0x01000048,
	F26		= 0x01000049,
	F27		= 0x0100004a,
	F28		= 0x0100004b,
	F29		= 0x0100004c,
	F30		= 0x0100004d,
	F31		= 0x0100004e,
	F32		= 0x0100004f,
	F33		= 0x01000050,
	F34		= 0x01000051,
	F35		= 0x01000052,
	Super_L		= 0x01000053,
	Super_R		= 0x01000054,
	Menu		= 0x01000055,
	Hyper_L		= 0x01000056,
	Hyper_R		= 0x01000057,
	Help		= 0x01000058,
	Direction_L	= 0x01000059,
	Direction_R	= 0x01000060,
	Space		= 0x20,
	Any		= Space,
	Exclam		= 0x21,
	QuoteDbl	= 0x22,
	NumberSign	= 0x23,
	Dollar		= 0x24,
	Percent		= 0x25,
	Ampersand	= 0x26,
	Apostrophe	= 0x27,
	ParenLeft	= 0x28,
	ParenRight	= 0x29,
	Asterisk	= 0x2a,
	Plus		= 0x2b,
	Comma		= 0x2c,
	Minus		= 0x2d,
	Period		= 0x2e,
	Slash		= 0x2f,
	Zero		= 0x30,
	One		= 0x31,
	Two		= 0x32,
	Three		= 0x33,
	Four		= 0x34,
	Five		= 0x35,
	Six		= 0x36,
	Seven		= 0x37,
	Eight		= 0x38,
	Nine		= 0x39,
	Colon		= 0x3a,
	Semicolon	= 0x3b,
	Less		= 0x3c,
	Equal		= 0x3d,
	Greater		= 0x3e,
	Question	= 0x3f,
	At		= 0x40,
	A		= 0x41,
	B		= 0x42,
	C		= 0x43,
	D		= 0x44,
	E		= 0x45,
	F		= 0x46,
	G		= 0x47,
	H		= 0x48,
	I		= 0x49,
	J		= 0x4a,
	K		= 0x4b,
	L		= 0x4c,
	M		= 0x4d,
	N		= 0x4e,
	O		= 0x4f,
	P		= 0x50,
	Q		= 0x51,
	R		= 0x52,
	S		= 0x53,
	T		= 0x54,
	U		= 0x55,
	V		= 0x56,
	W		= 0x57,
	X		= 0x58,
	Y		= 0x59,
	Z		= 0x5a,
	BracketLeft	= 0x5b,
	Backslash	= 0x5c,
	BracketRight	= 0x5d,
	AsciiCircum	= 0x5e,
	Underscore	= 0x5f,
	QuoteLeft	= 0x60,
	BraceLeft	= 0x7b,
	Bar		= 0x7c,
	BraceRight	= 0x7d,
	AsciiTilde	= 0x7e,
	Nobreakspace	= 0x0a0,
	Exclamdown	= 0x0a1,
	Cent		= 0x0a2,
	Sterling	= 0x0a3,
	Currency	= 0x0a4,
	Yen		= 0x0a5,
	Brokenbar	= 0x0a6,
	Section		= 0x0a7,
	Diaeresis	= 0x0a8,
	Copyright	= 0x0a9,
	Ordfeminine	= 0x0aa,
	Guillemotleft	= 0x0ab,
	Notsign		= 0x0ac,
	Hyphen		= 0x0ad,
	Registered	= 0x0ae,
	Macron		= 0x0af,
	Degree		= 0x0b0,
	Plusminus	= 0x0b1,
	Twosuperior	= 0x0b2,
	Threesuperior	= 0x0b3,
	Acute		= 0x0b4,
	Mu		= 0x0b5,
	Paragraph	= 0x0b6,
	Periodcentered	= 0x0b7,
	Cedilla		= 0x0b8,
	Onesuperior	= 0x0b9,
	Masculine	= 0x0ba,
	Guillemotright	= 0x0bb,
	Onequarter	= 0x0bc,
	Onehalf		= 0x0bd,
	Threequarters	= 0x0be,
	Questiondown	= 0x0bf,
	agrave		= 0x0c0,
	Aacute		= 0x0c1,
	Acircumflex	= 0x0c2,
	Atilde		= 0x0c3,
	Adiaeresis	= 0x0c4,
	Aring		= 0x0c5,
	AE		= 0x0c6,
	Ccedilla	= 0x0c7,
	Egrave		= 0x0c8,
	Eacute		= 0x0c9,
	Ecircumflex	= 0x0ca,
	Ediaeresis	= 0x0cb,
	Igrave		= 0x0cc,
	Iacute		= 0x0cd,
	Icircumflex	= 0x0ce,
	Idiaeresis	= 0x0cf,
	ETH		= 0x0d0,
	NTilde		= 0x0d1,
	Ograve		= 0x0d2,
	Oacute		= 0x0d3,
	Ocircumflex	= 0x0d4,
	Otilde		= 0x0d5,
	Odiaeresis	= 0x0d6,
	Multiply	= 0x0d7,
	Ooblique	= 0x0d8,
	Ugrave		= 0x0d9,
	Uacute		= 0x0da,
	Ucircumflex	= 0x0db,
	Udiaeresis	= 0x0dc,
	Yacute		= 0x0dd,
	THORN		= 0x0de,
	Ssharp		= 0x0df,
	Division	= 0x0f7,
	Ydiaeresis	= 0x0ff,
	Multi_key	= 0x01001120,
	Codeinput	= 0x01001137,
	SingleCandidate	= 0x0100113c,
	MultipleCandidate = 0x0100113d,
	PreviousCandidate = 0x0100113e,
	Mode_switch	= 0x0100117e,
	Kanji		= 0x01001121,
	Muhenkan	= 0x01001122,
	Henkan		= 0x01001123,
	Romaji		= 0x01001124,
	Hiragana	= 0x01001125,
	Katakana	= 0x01001126,
	Hiragana_Katakana = 0x01001127,
	Zenkaku		= 0x01001128,
	Hankaku		= 0x01001129,
	Zenkaku_Hankaku	= 0x0100112a,
	Touroku		= 0x0100112b,
	Massyo		= 0x0100112c,
	Kana_Lock	= 0x0100112d,
	Kana_Shift	= 0x0100112e,
	Eisu_Shift	= 0x0100112f,
	Eisu_toggle	= 0x01001130,
	Hangul		= 0x01001131,
	Hangul_Start	= 0x01001132,
	Hangul_End	= 0x01001133,
	Hangul_Hanja	= 0x01001134,
	Hangul_Jamo	= 0x01001135,
	Hangul_Romaja	= 0x01001136,
	Hangul_Jeonja	= 0x01001138,
	Hangul_Banja	= 0x01001139,
	Hangul_PreHanja	= 0x0100113a,
	Hangul_PostHanja= 0x0100113b,
	Hangul_Special	= 0x0100113f,
	Dead_Grave	= 0x01001250,
	Dead_Acute	= 0x01001251,
	Dead_Circumflex	= 0x01001252,
	Dead_Tilde	= 0x01001253,
	Dead_Macron	= 0x01001254,
	Dead_Breve	= 0x01001255,
	Dead_Abovedot	= 0x01001256,
	Dead_Diaeresis	= 0x01001257,
	Dead_Abovering	= 0x01001258,
	Dead_Doubleacute= 0x01001259,
	Dead_Caron	= 0x0100125a,
	Dead_Cedilla	= 0x0100125b,
	Dead_Ogonek	= 0x0100125c,
	Dead_Iota	= 0x0100125d,
	Dead_Voiced_Sound = 0x0100125e,
	Dead_Semivoiced_Sound	= 0x0100125f,
	Dead_Belowdot	= 0x01001260,
	Dead_Hook	= 0x01001261,
	Dead_Horn	= 0x01001262,
	Back		= 0x01000061,
	Forward		= 0x01000062,
	Stop		= 0x01000063,
	Refresh		= 0x01000064,
	VolumeDown	= 0x01000070,
	VolumeMute	= 0x01000071,
	VolumeUp	= 0x01000072,
	BassBoost	= 0x01000073,
	BassUp		= 0x01000074,
	BassDown	= 0x01000075,
	TrebleUp	= 0x01000076,
	TrebleDown	= 0x01000077,
	MediaPlay	= 0x01000080,
	MediaStop	= 0x01000081,
	MediaPrevious	= 0x01000082,
	MediaNext	= 0x01000083,
	MediaRecord	= 0x01000084,
	HomePage	= 0x01000090,
	Favorites	= 0x01000091,
	Search		= 0x01000092,
	Standby		= 0x01000093,
	OpenUrl		= 0x01000094,
	LaunchMail	= 0x010000a0,
	LaunchMedia	= 0x010000a1,
	Launch0		= 0x010000a2,
	Launch1		= 0x010000a3,
	Launch2		= 0x010000a4,
	Launch3		= 0x010000a5,
	Launch4		= 0x010000a6,
	Launch5		= 0x010000a7,
	Launch6		= 0x010000a8,
	Launch7		= 0x010000a9,
	Launch8		= 0x010000aa,
	Launch9		= 0x010000ab,
	LaunchA		= 0x010000ac,
	LaunchB		= 0x010000ad,
	LaunchC		= 0x010000ae,
	LaunchD		= 0x010000af,
	LaunchE		= 0x010000b0,
	LaunchF		= 0x010000b1,
	MediaLast	= 0x0100ffff,
	unknown		= 0x01ffffff,
	Call		= 0x01100004,
	Context1	= 0x01100000,
	Context2	= 0x01100001,
	Context3	= 0x01100002,
	Context4	= 0x01100003,
	Flip		= 0x01100006,
	Hangup		= 0x01100005,
	No		= 0x01010002,
	Select		= 0x01010000,
	Yes		= 0x01010001,
	Execute		= 0x01020003,
	Printer		= 0x01020002,
	Play		= 0x01020005,
	Sleep		= 0x01020004,
	Zoom		= 0x01020006,
	Cancel		= 0x01020001
    };

    mGlobal const char*		nameOf(ButtonState);
    mGlobal ButtonState		stateOf(const char*);

    mGlobal bool    leftMouseButton(ButtonState);
    mGlobal bool    middleMouseButton(ButtonState);
    mGlobal bool    rightMouseButton(ButtonState);
    mGlobal bool    shiftKeyboardButton(ButtonState);
    mGlobal bool    ctrlKeyboardButton(ButtonState);
    mGlobal bool    altKeyboardButton(ButtonState);
};

#endif
