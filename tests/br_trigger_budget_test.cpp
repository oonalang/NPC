#include "../src/main/jni/Trigger/BRQueryBudget.h"
#include <cassert>
#include <cmath>
#include <limits>

using namespace PhantomTrigger;

int main() {
    BRQueryBudget budget;
    const ContextIdentity first{1,2,3,4,5};
    const ContextIdentity second{1,2,9,4,5};

    assert(BRTraceDistance(1400.0f,true) == 600.0f);
    assert(BRTraceDistance(1400.0f,false) == 1400.0f);
    assert(BRTraceDistance(500.0f,true) == 500.0f);
    assert(BRTraceDistance(-1.0f,true) == 0.0f);
    assert(BRTraceDistance(std::numeric_limits<float>::infinity(),true) == 0.0f);

    assert(!budget.ShouldTrace(first,100,true,false,false));
    assert(budget.ShouldTrace(first,100,true,true,false));
    budget.Observed(first,100,false,true);
    assert(!budget.ShouldTrace(first,101,true,true,false));
    assert(!budget.ShouldTrace(first,131,true,true,false));
    assert(budget.ShouldTrace(first,132,true,true,false));
    budget.Observed(first,132,false,true);
    assert(!budget.ShouldTrace(first,139,true,true,false));

    assert(budget.ShouldTrace(first,140,true,true,true));
    budget.Observed(first,140,true,true);
    assert(budget.ShouldTrace(first,141,true,true,false));
    budget.Observed(first,141,false,true);
    assert(!budget.ShouldTrace(first,142,true,true,false));
    assert(budget.ShouldTrace(second,142,true,true,false));

    budget.Observed(second,142,false,true);
    assert(budget.ShouldTrace(second,143,false,true,false));
    assert(budget.ShouldTrace(second,144,false,true,false));
    assert(!budget.ShouldTrace(second,145,true,true,false));
    assert(budget.ShouldTrace(second,100,true,true,false));

    budget.Clear();
    assert(budget.ShouldTrace(first,1,true,true,false));
    return 0;
}
