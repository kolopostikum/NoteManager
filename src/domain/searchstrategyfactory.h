#pragma once

#include <QString>
#include <memory>

#include "domain/inotesearchstrategy.h"

/// Creates a search strategy by its identifier. Unknown ids fall back to the
/// default "substring" strategy.
std::shared_ptr<INoteSearchStrategy> createSearchStrategy(const QString &id);
