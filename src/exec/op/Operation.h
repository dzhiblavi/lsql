#pragma once

#include "exec/op/Explanation.h"
#include "exec/op/Subscriber.h"

#include <memory>

namespace lsql::exec {

class Operation {
 public:
    virtual ~Operation() = default;

    // Subscribe for this operation's output on a given phase.
    // The operation never pushes results concurrently (see Subscriber::consume)
    //
    // Parameters:
    // - sub: the subscriber that records will be passed to
    // - out_phase: the phase at which records will be passsed to the subscriber.
    //
    // pre:  out_phase >= minPhase()
    // post: out_phase <= maxPhase()
    virtual void subscribe(int out_phase, Subscriber* sub) = 0;

    // Min phase on which this operation can produce results
    virtual int minPhase() const = 0;

    // Max phase at which this operation has been requested and will produce results
    virtual int maxPhase() const = 0;

    // Unique operation name
    virtual std::string name() const = 0;

    virtual ExplanationItem explain(ExplanationCtx ctx) const = 0;
};

using OperationPtr = std::shared_ptr<Operation>;

}  // namespace lsql::exec
