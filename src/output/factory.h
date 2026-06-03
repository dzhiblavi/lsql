#pragma once

#include "output/Consumer.h"
#include "output/Formats.h"
#include "output/Sink.h"

#include "output/CSVHeaderFormatter.h"
#include "output/JSONFormatter.h"
#include "output/TSKVFormatter.h"

#include "core/types.h"

namespace lsql::output {

template <Sink S>
Box<Consumer> makeConsumer(Format format, ConstFieldBindingPtr binding) {
    switch (format) {
        case output::Format::JSON:
            return box<output::JSONFormatter<S>>(S{}, binding);
        case output::Format::TSKV:
            return box<output::TSKVFormatter<S>>(S{}, binding);
        case output::Format::CSVHeader:
            return box<output::CSVHeaderFormatter<S>>(S{}, binding);
    }
}

}  // namespace lsql::output
