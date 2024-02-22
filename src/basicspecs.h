/// @file basicspecs.h
///
/// @brief BASICの設定
///
#ifndef BASICSPECS_H
#define BASICSPECS_H

//#include "l3specs.h"
//#include "msxspecs.h"
#include <wx/string.h>


class BasicSpecs
{
public:
	BasicSpecs() {}
	virtual ~BasicSpecs() {}
	/// BASICが拡張BASICかどうか
	virtual bool IsExtendedBasic(const wxString &basic_type) = 0;
	/// マシンタイプの判別
	virtual void SetMachineType(const wxString &basic_type, int &machine_type) = 0;
};

#endif /* BASICSPECS_H */
