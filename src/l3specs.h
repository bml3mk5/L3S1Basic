/// @file l3specs.h
///
/// @brief L3固有の設定
///
#ifndef _L3SPECS_H_
#define _L3SPECS_H_

/// マシンタイプ
#define MACHINE_TYPE_L3 1
#define MACHINE_TYPE_S1 2

/// BASICが拡張BASICかどうか
#define IS_EXTENDED_BASIC(basic_type) \
	(basic_type.Find("ROM") >= 0 ? false : true)

/// マシンタイプの判別
#define SET_MACHINE_TYPE(basic_type, machine_type) { \
	if (basic_type.Find("L3") >= 0) { \
		machine_type = MACHINE_TYPE_L3; \
	} else if (basic_type.Find("S1") >= 0) { \
		machine_type = MACHINE_TYPE_S1; \
	} else { \
		machine_type = 0; \
	} \
}

#endif
