#pragma once

#include "core/time_formats.h"
#include "core/types.h"

#include <reflex/matcher.h>

namespace lsql {

template <TimeFormat F>
std::string_view timeFormatRegex();

template <TimeFormat F>
timestamp_t timestampFromString(std::string_view s);

template <TimeFormat F>
void reflex_code_FSM(reflex::Matcher& m);

template <TimeFormat F>
extern const char* reflex_pred_FSM;

}  // namespace lsql
