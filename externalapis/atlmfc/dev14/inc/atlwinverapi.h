// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLWINVERAPI_H__
#define __ATLWINVERAPI_H__

#pragma once

#ifdef _ATL_ALL_WARNINGS
#pragma warning( push )
#endif

#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4949) // pragmas 'managed' and 'unmanaged' are meaningful only when compiled with /clr

#include <atldef.h>
#include <Windows.h>
#include <Ole2.h>
#include <sdkddkver.h>

// This file contains declarations of wrappers for methods used
// in ATL that are only available in later versions of Windows.

// When the minimum supported version of Windows is increased, the
// implementations of these methods just call the real Windows APIs.

// Minimum supported versions of Windows:
// Windows XP SP2 for x86 and x64, Windows 8 for ARM

#if defined(_M_IX86) || defined(_M_X64)
#define _ATL_NTDDI_MIN NTDDI_WINXPSP2
#else
#define _ATL_NTDDI_MIN NTDDI_WIN8
#endif

// Use this macro for loading a local cached function from a DLL that is known to be loaded (e.g. KERNEL32)
#define IFDYNAMICGETCACHEDFUNCTION(libraryname, functionname, functionpointer) \
	static volatile auto functionpointer##_cache = reinterpret_cast<decltype(::functionname)*>(NULL); \
	auto functionpointer = reinterpret_cast<decltype(::functionname)*>(functionpointer##_cache); \
	if (functionpointer == reinterpret_cast<decltype(::functionname)*>(NULL)) \
	{ \
		HINSTANCE hLibrary = GetModuleHandleW(libraryname); \
		if (hLibrary != NULL) \
		{ \
			functionpointer = reinterpret_cast<decltype(::functionname)*>(::GetProcAddress(hLibrary, #functionname)); \
			functionpointer##_cache = reinterpret_cast<decltype(::functionname)*>(::EncodePointer((PVOID)functionpointer)); \
		} \
	} \
	else \
	{ \
		functionpointer = reinterpret_cast<decltype(::functionname)*>(::DecodePointer((PVOID)functionpointer)); \
	} \
	if (functionpointer != reinterpret_cast<decltype(::functionname)*>(NULL))

#define IFDYNAMICGETCACHEDFUNCTIONTYPEDEF(libraryname, functiontypedef, functionname, functionpointer) \
	static volatile auto functionpointer##_cache = reinterpret_cast<functiontypedef>(NULL); \
	auto functionpointer = reinterpret_cast<functiontypedef>(functionpointer##_cache); \
	if (functionpointer == reinterpret_cast<functiontypedef>(NULL)) \
	{ \
		HINSTANCE hLibrary = GetModuleHandleW(libraryname); \
		if (hLibrary != NULL) \
		{ \
			functionpointer = reinterpret_cast<functiontypedef>(::GetProcAddress(hLibrary, functionname)); \
			functionpointer##_cache = reinterpret_cast<functiontypedef>(::EncodePointer((PVOID)functionpointer)); \
		} \
	} \
	else \
	{ \
		functionpointer = reinterpret_cast<functiontypedef>(::DecodePointer((PVOID)functionpointer)); \
	} \
	if (functionpointer != reinterpret_cast<functiontypedef>(NULL))

#pragma pack(push,_ATL_PACKING)
namespace ATL
{

#if TRUE || (NTDDI_VERSION < NTDDI_VISTA) || defined(_USING_V110_SDK71_) || defined(_ATL_XP_TARGETING)
struct _ATL_LCID_TO_LOCALENAME
{
	LCID            lcid;
	const wchar_t*  localeName;
};

struct _ATL_LOCALENAME_INDEX
{
	const wchar_t*  name;
	int             index;
};

// Map of LCID to locale name for Windows XP.
// Data in this table has been obtained from National Language Support (NLS)
// API Reference at http://msdn.microsoft.com/en-us/goglobal/bb896001.aspx
// The table is sorted to improve search performance.
extern "C" const __declspec(selectany) _ATL_LCID_TO_LOCALENAME _pAtlLcidToLocaleNameTable[] = {
	{ 0x0001, L"ar"         },
	{ 0x0002, L"bg"         },
	{ 0x0003, L"ca"         },
	{ 0x0004, L"zh-CHS"     },
	{ 0x0005, L"cs"         },
	{ 0x0006, L"da"         },
	{ 0x0007, L"de"         },
	{ 0x0008, L"el"         },
	{ 0x0009, L"en"         },
	{ 0x000A, L"es"         },
	{ 0x000B, L"fi"         },
	{ 0x000C, L"fr"         },
	{ 0x000D, L"he"         },
	{ 0x000E, L"hu"         },
	{ 0x000F, L"is"         },
	{ 0x0010, L"it"         },
	{ 0x0011, L"ja"         },
	{ 0x0012, L"ko"         },
	{ 0x0013, L"nl"         },
	{ 0x0014, L"no"         },
	{ 0x0015, L"pl"         },
	{ 0x0016, L"pt"         },
	{ 0x0018, L"ro"         },
	{ 0x0019, L"ru"         },
	{ 0x001A, L"hr"         },
	{ 0x001B, L"sk"         },
	{ 0x001C, L"sq"         },
	{ 0x001D, L"sv"         },
	{ 0x001E, L"th"         },
	{ 0x001F, L"tr"         },
	{ 0x0020, L"ur"         },
	{ 0x0021, L"id"         },
	{ 0x0022, L"uk"         },
	{ 0x0023, L"be"         },
	{ 0x0024, L"sl"         },
	{ 0x0025, L"et"         },
	{ 0x0026, L"lv"         },
	{ 0x0027, L"lt"         },
	{ 0x0029, L"fa"         },
	{ 0x002A, L"vi"         },
	{ 0x002B, L"hy"         },
	{ 0x002C, L"az"         },
	{ 0x002D, L"eu"         },
	{ 0x002F, L"mk"         },
	{ 0x0036, L"af"         },
	{ 0x0037, L"ka"         },
	{ 0x0038, L"fo"         },
	{ 0x0039, L"hi"         },
	{ 0x003E, L"ms"         },
	{ 0x003F, L"kk"         },
	{ 0x0040, L"ky"         },
	{ 0x0041, L"sw"         },
	{ 0x0043, L"uz"         },
	{ 0x0044, L"tt"         },
	{ 0x0046, L"pa"         },
	{ 0x0047, L"gu"         },
	{ 0x0049, L"ta"         },
	{ 0x004A, L"te"         },
	{ 0x004B, L"kn"         },
	{ 0x004E, L"mr"         },
	{ 0x004F, L"sa"         },
	{ 0x0050, L"mn"         },
	{ 0x0056, L"gl"         },
	{ 0x0057, L"kok"        },
	{ 0x005A, L"syr"        },
	{ 0x0065, L"div"        },
	{ 0x007f, L""           },
	{ 0x0401, L"ar-SA"      },
	{ 0x0402, L"bg-BG"      },
	{ 0x0403, L"ca-ES"      },
	{ 0x0404, L"zh-TW"      },
	{ 0x0405, L"cs-CZ"      },
	{ 0x0406, L"da-DK"      },
	{ 0x0407, L"de-DE"      },
	{ 0x0408, L"el-GR"      },
	{ 0x0409, L"en-US"      },
	{ 0x040B, L"fi-FI"      },
	{ 0x040C, L"fr-FR"      },
	{ 0x040D, L"he-IL"      },
	{ 0x040E, L"hu-HU"      },
	{ 0x040F, L"is-IS"      },
	{ 0x0410, L"it-IT"      },
	{ 0x0411, L"ja-JP"      },
	{ 0x0412, L"ko-KR"      },
	{ 0x0413, L"nl-NL"      },
	{ 0x0414, L"nb-NO"      },
	{ 0x0415, L"pl-PL"      },
	{ 0x0416, L"pt-BR"      },
	{ 0x0418, L"ro-RO"      },
	{ 0x0419, L"ru-RU"      },
	{ 0x041A, L"hr-HR"      },
	{ 0x041B, L"sk-SK"      },
	{ 0x041C, L"sq-AL"      },
	{ 0x041D, L"sv-SE"      },
	{ 0x041E, L"th-TH"      },
	{ 0x041F, L"tr-TR"      },
	{ 0x0420, L"ur-PK"      },
	{ 0x0421, L"id-ID"      },
	{ 0x0422, L"uk-UA"      },
	{ 0x0423, L"be-BY"      },
	{ 0x0424, L"sl-SI"      },
	{ 0x0425, L"et-EE"      },
	{ 0x0426, L"lv-LV"      },
	{ 0x0427, L"lt-LT"      },
	{ 0x0429, L"fa-IR"      },
	{ 0x042A, L"vi-VN"      },
	{ 0x042B, L"hy-AM"      },
	{ 0x042C, L"az-AZ-Latn" },
	{ 0x042D, L"eu-ES"      },
	{ 0x042F, L"mk-MK"      },
	{ 0x0432, L"tn-ZA"      },
	{ 0x0434, L"xh-ZA"      },
	{ 0x0435, L"zu-ZA"      },
	{ 0x0436, L"af-ZA"      },
	{ 0x0437, L"ka-GE"      },
	{ 0x0438, L"fo-FO"      },
	{ 0x0439, L"hi-IN"      },
	{ 0x043A, L"mt-MT"      },
	{ 0x043B, L"se-NO"      },
	{ 0x043E, L"ms-MY"      },
	{ 0x043F, L"kk-KZ"      },
	{ 0x0440, L"ky-KG"      },
	{ 0x0441, L"sw-KE"      },
	{ 0x0443, L"uz-UZ-Latn" },
	{ 0x0444, L"tt-RU"      },
	{ 0x0445, L"bn-IN"      },
	{ 0x0446, L"pa-IN"      },
	{ 0x0447, L"gu-IN"      },
	{ 0x0449, L"ta-IN"      },
	{ 0x044A, L"te-IN"      },
	{ 0x044B, L"kn-IN"      },
	{ 0x044C, L"ml-IN"      },
	{ 0x044E, L"mr-IN"      },
	{ 0x044F, L"sa-IN"      },
	{ 0x0450, L"mn-MN"      },
	{ 0x0452, L"cy-GB"      },
	{ 0x0456, L"gl-ES"      },
	{ 0x0457, L"kok-IN"     },
	{ 0x045A, L"syr-SY"     },
	{ 0x0465, L"div-MV"     },
	{ 0x046B, L"quz-BO"     },
	{ 0x046C, L"ns-ZA"      },
	{ 0x0481, L"mi-NZ"      },
	{ 0x0801, L"ar-IQ"      },
	{ 0x0804, L"zh-CN"      },
	{ 0x0807, L"de-CH"      },
	{ 0x0809, L"en-GB"      },
	{ 0x080A, L"es-MX"      },
	{ 0x080C, L"fr-BE"      },
	{ 0x0810, L"it-CH"      },
	{ 0x0813, L"nl-BE"      },
	{ 0x0814, L"nn-NO"      },
	{ 0x0816, L"pt-PT"      },
	{ 0x081A, L"sr-SP-Latn" },
	{ 0x081D, L"sv-FI"      },
	{ 0x082C, L"az-AZ-Cyrl" },
	{ 0x083B, L"se-SE"      },
	{ 0x083E, L"ms-BN"      },
	{ 0x0843, L"uz-UZ-Cyrl" },
	{ 0x086B, L"quz-EC"     },
	{ 0x0C01, L"ar-EG"      },
	{ 0x0C04, L"zh-HK"      },
	{ 0x0C07, L"de-AT"      },
	{ 0x0C09, L"en-AU"      },
	{ 0x0C0A, L"es-ES"      },
	{ 0x0C0C, L"fr-CA"      },
	{ 0x0C1A, L"sr-SP-Cyrl" },
	{ 0x0C3B, L"se-FI"      },
	{ 0x0C6B, L"quz-PE"     },
	{ 0x1001, L"ar-LY"      },
	{ 0x1004, L"zh-SG"      },
	{ 0x1007, L"de-LU"      },
	{ 0x1009, L"en-CA"      },
	{ 0x100A, L"es-GT"      },
	{ 0x100C, L"fr-CH"      },
	{ 0x101A, L"hr-BA"      },
	{ 0x103B, L"smj-NO"     },
	{ 0x1401, L"ar-DZ"      },
	{ 0x1404, L"zh-MO"      },
	{ 0x1407, L"de-LI"      },
	{ 0x1409, L"en-NZ"      },
	{ 0x140A, L"es-CR"      },
	{ 0x140C, L"fr-LU"      },
	{ 0x141A, L"bs-BA-Latn" },
	{ 0x143B, L"smj-SE"     },
	{ 0x1801, L"ar-MA"      },
	{ 0x1809, L"en-IE"      },
	{ 0x180A, L"es-PA"      },
	{ 0x180C, L"fr-MC"      },
	{ 0x181A, L"sr-BA-Latn" },
	{ 0x183B, L"sma-NO"     },
	{ 0x1C01, L"ar-TN"      },
	{ 0x1C09, L"en-ZA"      },
	{ 0x1C0A, L"es-DO"      },
	{ 0x1C1A, L"sr-BA-Cyrl" },
	{ 0x1C3B, L"sma-SE"     },
	{ 0x2001, L"ar-OM"      },
	{ 0x2009, L"en-JM"      },
	{ 0x200A, L"es-VE"      },
	{ 0x203B, L"sms-FI"     },
	{ 0x2401, L"ar-YE"      },
	{ 0x2409, L"en-CB"      },
	{ 0x240A, L"es-CO"      },
	{ 0x243B, L"smn-FI"     },
	{ 0x2801, L"ar-SY"      },
	{ 0x2809, L"en-BZ"      },
	{ 0x280A, L"es-PE"      },
	{ 0x2C01, L"ar-JO"      },
	{ 0x2C09, L"en-TT"      },
	{ 0x2C0A, L"es-AR"      },
	{ 0x3001, L"ar-LB"      },
	{ 0x3009, L"en-ZW"      },
	{ 0x300A, L"es-EC"      },
	{ 0x3401, L"ar-KW"      },
	{ 0x3409, L"en-PH"      },
	{ 0x340A, L"es-CL"      },
	{ 0x3801, L"ar-AE"      },
	{ 0x380A, L"es-UY"      },
	{ 0x3C01, L"ar-BH"      },
	{ 0x3C0A, L"es-PY"      },
	{ 0x4001, L"ar-QA"      },
	{ 0x400A, L"es-BO"      },
	{ 0x440A, L"es-SV"      },
	{ 0x480A, L"es-HN"      },
	{ 0x4C0A, L"es-NI"      },
	{ 0x500A, L"es-PR"      },
	{ 0x7C04, L"zh-CHT"     },
	{ 0x7C1A, L"sr"         }
};

// Map of locale name to an index in LcidToLocaleNameTable, for Windows XP.
// Data in this table has been obtained from National Language Support (NLS)
// API Reference at http://msdn.microsoft.com/en-us/goglobal/bb896001.aspx
// The table is sorted to improve search performance.
extern "C" const __declspec(selectany) _ATL_LOCALENAME_INDEX _pAtlLocaleNameToIndexTable[] =
{
	{ L""           , 66  },
	{ L"af"         , 44  },
	{ L"af-za"      , 113 },
	{ L"ar"         , 0   },
	{ L"ar-ae"      , 216 },
	{ L"ar-bh"      , 218 },
	{ L"ar-dz"      , 177 },
	{ L"ar-eg"      , 160 },
	{ L"ar-iq"      , 143 },
	{ L"ar-jo"      , 207 },
	{ L"ar-kw"      , 213 },
	{ L"ar-lb"      , 210 },
	{ L"ar-ly"      , 169 },
	{ L"ar-ma"      , 185 },
	{ L"ar-om"      , 196 },
	{ L"ar-qa"      , 220 },
	{ L"ar-sa"      , 67  },
	{ L"ar-sy"      , 204 },
	{ L"ar-tn"      , 191 },
	{ L"ar-ye"      , 200 },
	{ L"az"         , 41  },
	{ L"az-az-cyrl" , 155 },
	{ L"az-az-latn" , 107 },
	{ L"be"         , 33  },
	{ L"be-by"      , 99  },
	{ L"bg"         , 1   },
	{ L"bg-bg"      , 68  },
	{ L"bn-in"      , 125 },
	{ L"bs-ba-latn" , 183 },
	{ L"ca"         , 2   },
	{ L"ca-es"      , 69  },
	{ L"cs"         , 4   },
	{ L"cs-cz"      , 71  },
	{ L"cy-gb"      , 135 },
	{ L"da"         , 5   },
	{ L"da-dk"      , 72  },
	{ L"de"         , 6   },
	{ L"de-at"      , 162 },
	{ L"de-ch"      , 145 },
	{ L"de-de"      , 73  },
	{ L"de-li"      , 179 },
	{ L"de-lu"      , 171 },
	{ L"div"        , 65  },
	{ L"div-mv"     , 139 },
	{ L"el"         , 7   },
	{ L"el-gr"      , 74  },
	{ L"en"         , 8   },
	{ L"en-au"      , 163 },
	{ L"en-bz"      , 205 },
	{ L"en-ca"      , 172 },
	{ L"en-cb"      , 201 },
	{ L"en-gb"      , 146 },
	{ L"en-ie"      , 186 },
	{ L"en-jm"      , 197 },
	{ L"en-nz"      , 180 },
	{ L"en-ph"      , 214 },
	{ L"en-tt"      , 208 },
	{ L"en-us"      , 75  },
	{ L"en-za"      , 192 },
	{ L"en-zw"      , 211 },
	{ L"es"         , 9   },
	{ L"es-ar"      , 209 },
	{ L"es-bo"      , 221 },
	{ L"es-cl"      , 215 },
	{ L"es-co"      , 202 },
	{ L"es-cr"      , 181 },
	{ L"es-do"      , 193 },
	{ L"es-ec"      , 212 },
	{ L"es-es"      , 164 },
	{ L"es-gt"      , 173 },
	{ L"es-hn"      , 223 },
	{ L"es-mx"      , 147 },
	{ L"es-ni"      , 224 },
	{ L"es-pa"      , 187 },
	{ L"es-pe"      , 206 },
	{ L"es-pr"      , 225 },
	{ L"es-py"      , 219 },
	{ L"es-sv"      , 222 },
	{ L"es-uy"      , 217 },
	{ L"es-ve"      , 198 },
	{ L"et"         , 35  },
	{ L"et-ee"      , 101 },
	{ L"eu"         , 42  },
	{ L"eu-es"      , 108 },
	{ L"fa"         , 38  },
	{ L"fa-ir"      , 104 },
	{ L"fi"         , 10  },
	{ L"fi-fi"      , 76  },
	{ L"fo"         , 46  },
	{ L"fo-fo"      , 115 },
	{ L"fr"         , 11  },
	{ L"fr-be"      , 148 },
	{ L"fr-ca"      , 165 },
	{ L"fr-ch"      , 174 },
	{ L"fr-fr"      , 77  },
	{ L"fr-lu"      , 182 },
	{ L"fr-mc"      , 188 },
	{ L"gl"         , 62  },
	{ L"gl-es"      , 136 },
	{ L"gu"         , 55  },
	{ L"gu-in"      , 127 },
	{ L"he"         , 12  },
	{ L"he-il"      , 78  },
	{ L"hi"         , 47  },
	{ L"hi-in"      , 116 },
	{ L"hr"         , 24  },
	{ L"hr-ba"      , 175 },
	{ L"hr-hr"      , 90  },
	{ L"hu"         , 13  },
	{ L"hu-hu"      , 79  },
	{ L"hy"         , 40  },
	{ L"hy-am"      , 106 },
	{ L"id"         , 31  },
	{ L"id-id"      , 97  },
	{ L"is"         , 14  },
	{ L"is-is"      , 80  },
	{ L"it"         , 15  },
	{ L"it-ch"      , 149 },
	{ L"it-it"      , 81  },
	{ L"ja"         , 16  },
	{ L"ja-jp"      , 82  },
	{ L"ka"         , 45  },
	{ L"ka-ge"      , 114 },
	{ L"kk"         , 49  },
	{ L"kk-kz"      , 120 },
	{ L"kn"         , 58  },
	{ L"kn-in"      , 130 },
	{ L"ko"         , 17  },
	{ L"kok"        , 63  },
	{ L"kok-in"     , 137 },
	{ L"ko-kr"      , 83  },
	{ L"ky"         , 50  },
	{ L"ky-kg"      , 121 },
	{ L"lt"         , 37  },
	{ L"lt-lt"      , 103 },
	{ L"lv"         , 36  },
	{ L"lv-lv"      , 102 },
	{ L"mi-nz"      , 142 },
	{ L"mk"         , 43  },
	{ L"mk-mk"      , 109 },
	{ L"ml-in"      , 131 },
	{ L"mn"         , 61  },
	{ L"mn-mn"      , 134 },
	{ L"mr"         , 59  },
	{ L"mr-in"      , 132 },
	{ L"ms"         , 48  },
	{ L"ms-bn"      , 157 },
	{ L"ms-my"      , 119 },
	{ L"mt-mt"      , 117 },
	{ L"nb-no"      , 85  },
	{ L"nl"         , 18  },
	{ L"nl-be"      , 150 },
	{ L"nl-nl"      , 84  },
	{ L"nn-no"      , 151 },
	{ L"no"         , 19  },
	{ L"ns-za"      , 141 },
	{ L"pa"         , 54  },
	{ L"pa-in"      , 126 },
	{ L"pl"         , 20  },
	{ L"pl-pl"      , 86  },
	{ L"pt"         , 21  },
	{ L"pt-br"      , 87  },
	{ L"pt-pt"      , 152 },
	{ L"quz-bo"     , 140 },
	{ L"quz-ec"     , 159 },
	{ L"quz-pe"     , 168 },
	{ L"ro"         , 22  },
	{ L"ro-ro"      , 88  },
	{ L"ru"         , 23  },
	{ L"ru-ru"      , 89  },
	{ L"sa"         , 60  },
	{ L"sa-in"      , 133 },
	{ L"se-fi"      , 167 },
	{ L"se-no"      , 118 },
	{ L"se-se"      , 156 },
	{ L"sk"         , 25  },
	{ L"sk-sk"      , 91  },
	{ L"sl"         , 34  },
	{ L"sl-si"      , 100 },
	{ L"sma-no"     , 190 },
	{ L"sma-se"     , 195 },
	{ L"smj-no"     , 176 },
	{ L"smj-se"     , 184 },
	{ L"smn-fi"     , 203 },
	{ L"sms-fi"     , 199 },
	{ L"sq"         , 26  },
	{ L"sq-al"      , 92  },
	{ L"sr"         , 227 },
	{ L"sr-ba-cyrl" , 194 },
	{ L"sr-ba-latn" , 189 },
	{ L"sr-sp-cyrl" , 166 },
	{ L"sr-sp-latn" , 153 },
	{ L"sv"         , 27  },
	{ L"sv-fi"      , 154 },
	{ L"sv-se"      , 93  },
	{ L"sw"         , 51  },
	{ L"sw-ke"      , 122 },
	{ L"syr"        , 64  },
	{ L"syr-sy"     , 138 },
	{ L"ta"         , 56  },
	{ L"ta-in"      , 128 },
	{ L"te"         , 57  },
	{ L"te-in"      , 129 },
	{ L"th"         , 28  },
	{ L"th-th"      , 94  },
	{ L"tn-za"      , 110 },
	{ L"tr"         , 29  },
	{ L"tr-tr"      , 95  },
	{ L"tt"         , 53  },
	{ L"tt-ru"      , 124 },
	{ L"uk"         , 32  },
	{ L"uk-ua"      , 98  },
	{ L"ur"         , 30  },
	{ L"ur-pk"      , 96  },
	{ L"uz"         , 52  },
	{ L"uz-uz-cyrl" , 158 },
	{ L"uz-uz-latn" , 123 },
	{ L"vi"         , 39  },
	{ L"vi-vn"      , 105 },
	{ L"xh-za"      , 111 },
	{ L"zh-chs"     , 3   },
	{ L"zh-cht"     , 226 },
	{ L"zh-cn"      , 144 },
	{ L"zh-hk"      , 161 },
	{ L"zh-mo"      , 178 },
	{ L"zh-sg"      , 170 },
	{ L"zh-tw"      , 70  },
	{ L"zu-za"      , 112 }
};

inline wchar_t __cdecl _AtlAsciiToWLower(_In_ wchar_t c)
{
	return ((c >= L'A' && c <= L'Z') ? (c - L'A' + L'a') : c);
}

// Implements wcsncpmp for ASCII chars only.
// We can't use wcsncmp in this context because we may end up trying to modify
// locale data structs or even calling the same function in NLS code.
inline int __cdecl _AtlWcsnicmpAscii(_In_reads_z_(count) const wchar_t* string1, _In_reads_z_(count) const wchar_t* string2, _In_ size_t count)
{
	wchar_t f, l;
	int result = 0;

	if (count)
	{
		do {
			f = _AtlAsciiToWLower(*string1);
			l = _AtlAsciiToWLower(*string2);
			string1++;
			string2++;
		} while ((--count) && f && (f == l));

		result = (int)(f - l);
	}

	return result;
}

// Maps input locale name to the index on LcidToLocaleNameTable
inline int __cdecl _AtlGetTableIndexFromLocaleName(_In_z_ const wchar_t* localeName)
{
	_ATL_LOCALENAME_INDEX *localeNamesIndex = (_ATL_LOCALENAME_INDEX*) _pAtlLocaleNameToIndexTable;
	int bottom = 0;
	int top = _countof(_pAtlLocaleNameToIndexTable) - 1;

	while (bottom <= top)
	{
		int middle = (bottom + top) / 2;
		int testIndex = _AtlWcsnicmpAscii(localeName, localeNamesIndex[middle].name, LOCALE_NAME_MAX_LENGTH);

		if (testIndex == 0)
		{
			return localeNamesIndex[middle].index;
		}

		if (testIndex < 0)
		{
			top = middle - 1;
		}
		else
		{
			bottom = middle + 1;
		}
	}

	return -1;
}

// Maps input LCID to an index in LcidToLocaleNameTable
inline int __cdecl _AtlGetTableIndexFromLcid(_In_ LCID lcid)
{
	int bottom = 0;
	int top = _countof(_pAtlLcidToLocaleNameTable) - 1;

	while (bottom <= top)
	{
		int middle = (bottom + top) / 2;
		int testIndex = lcid - _pAtlLcidToLocaleNameTable[middle].lcid;

		if (testIndex == 0)
		{
			return middle;
		}

		if (testIndex < 0)
		{
			top = middle - 1;
		}
		else
		{
			bottom = middle + 1;
		}
	}

	return -1;
}

inline LCID __cdecl _AtlDownlevelLocaleNameToLCID(_In_z_ LPCWSTR localeName)
{
	if (localeName == NULL)
	{
		return 0;
	}

	int index = _AtlGetTableIndexFromLocaleName(localeName);
	if (index < 0 || (index >= _countof(_pAtlLcidToLocaleNameTable)))
	{
		return 0;
	}

	return _pAtlLcidToLocaleNameTable[index].lcid;
}

_Success_(return != 0) inline int __cdecl _AtlDownlevelLCIDToLocaleName(_In_ LCID lcid, _Out_writes_opt_z_(cchLocaleName) LPWSTR outLocaleName, _In_ int cchLocaleName)
{
	if (lcid == 0 || lcid == LOCALE_USER_DEFAULT || lcid == LOCALE_SYSTEM_DEFAULT)
	{
		return 0;
	}

	if ((outLocaleName == NULL && cchLocaleName > 0) || cchLocaleName < 0)
	{
		return 0;
	}

	int index = _AtlGetTableIndexFromLcid(lcid);
	if (index < 0)
	{
		return 0;
	}

	const wchar_t* buffer = _pAtlLcidToLocaleNameTable[index].localeName;
	size_t count = wcsnlen(buffer, LOCALE_NAME_MAX_LENGTH);

	if (cchLocaleName > 0)
	{
		if ((int)count >= cchLocaleName)
		{
			return 0;
		}

		wcscpy_s(outLocaleName, cchLocaleName, buffer);
	}

	return (int)count + 1;
}
#endif

inline int __cdecl _AtlLCMapStringEx(_In_z_ LPCWSTR lpLocaleName, _In_ DWORD dwMapFlags, _In_reads_(cchSrc) LPCWSTR lpSrcStr, _In_ int cchSrc, _Out_writes_opt_(cchDest) LPWSTR lpDestStr, _In_ int cchDest, _In_opt_ LPNLSVERSIONINFO lpVersionInformation, _In_opt_ LPVOID lpReserved, _In_opt_ LPARAM sortHandle)
{
#if FALSE && (NTDDI_VERSION >= NTDDI_VISTA) && !defined(_USING_V110_SDK71_) && !defined(_ATL_XP_TARGETING)
	return LCMapStringEx(lpLocaleName, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest, lpVersionInformation, lpReserved, sortHandle);
#else
	// use LCMapStringEx if it is available (only on Vista+)...
	typedef int (__stdcall *PFNLCMAPSTRINGEX)(LPCWSTR, DWORD, LPCWSTR, int, LPWSTR, int, LPNLSVERSIONINFO, LPVOID, LPARAM);
	IFDYNAMICGETCACHEDFUNCTIONTYPEDEF(L"kernel32.dll", PFNLCMAPSTRINGEX, "LCMapStringEx", pfLCMapStringEx)
	{
		return (*pfLCMapStringEx)(lpLocaleName, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest, lpVersionInformation, lpReserved, sortHandle);
	}

	// ...otherwise fall back to using LCMapString.
	return LCMapStringW(_AtlDownlevelLocaleNameToLCID(lpLocaleName), dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest);
#endif
}

#pragma managed(push, off)
#ifndef _ATL_STATIC_LIB_IMPL
extern inline BOOL __cdecl _AtlInitializeCriticalSectionEx(_Out_ LPCRITICAL_SECTION lpCriticalSection, _In_ DWORD dwSpinCount, _In_ DWORD Flags)
{
#if (NTDDI_VERSION >= NTDDI_VISTA) && !defined(_USING_V110_SDK71_) && !defined(_ATL_XP_TARGETING)
	// InitializeCriticalSectionEx is available in Vista or later, desktop or store apps
	return ::InitializeCriticalSectionEx(lpCriticalSection, dwSpinCount, Flags);
#else
	UNREFERENCED_PARAMETER(Flags);

	// ...otherwise fall back to using InitializeCriticalSectionAndSpinCount.
	return ::InitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
#endif
}
#else // _ATL_STATIC_LIB_IMPL
BOOL __cdecl _AtlInitializeCriticalSectionEx(_Out_ LPCRITICAL_SECTION lpCriticalSection, _In_ DWORD dwSpinCount, _In_ DWORD Flags);
#endif  // _ATL_STATIC_LIB_IMPL
#pragma managed(pop)

}	// namespace ATL
#pragma pack(pop)

#ifdef _ATL_ALL_WARNINGS
#pragma warning( pop )
#endif

#endif	// __ATLWINVERAPI_H__
