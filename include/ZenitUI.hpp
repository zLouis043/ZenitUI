#pragma once

/**
 * \file ZenitUI.hpp
 * \brief Umbrella include for the whole framework.
 *
 * Including this header brings in every public ZenitUI symbol. Prefer
 * including the specific header you need in library code (faster compile
 * times, fewer transitive dependencies); use this umbrella in applications
 * and examples.
 *
 * \note `ZenitUIComponents.hpp` is included twice below — harmless because
 *       it is guarded by `#pragma once`, but worth cleaning up. The duplicate
 *       is a known wart of the current include list.
 */

#include "Common.hpp"
#include "Logger.hpp"
#include "CoreTypes.hpp"
#include "UIEnums.hpp"
#include "Easing.hpp"

#include "Layout.hpp"
#include "Style.hpp"
#include "Theme.hpp"

#include "AnimPrimitives.hpp"
#include "AnimationPlayer.hpp"

#include "Bindable.hpp"

#include "Diagnostics.hpp"

#include "InputController.hpp"

#include "Media.hpp"
#include "ScrollController.hpp"
#include "ScrollState.hpp"
#include "SmallVector.hpp"
#include "Style.hpp"           ///< \note Duplicate include; see file-level note.
#include "StyleAttr.hpp"
#include "Theme.hpp"           ///< \note Duplicate include; see file-level note.

#include "ZenitUIComponents.hpp"
#include "UIContext.hpp"
#include "Unit.hpp"
#include "ZMarkup.hpp"
#include "StyleParser.hpp"
#include "FilterRegistry.hpp"
#include "Debug.hpp"