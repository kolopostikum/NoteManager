#include "domain/searchstrategyfactory.h"

#include "domain/substringsearchstrategy.h"

std::shared_ptr<INoteSearchStrategy> createSearchStrategy(const QString &id)
{
    // Only one strategy is shipped; the id is the extension point.
    Q_UNUSED(id)
    return std::make_shared<SubstringSearchStrategy>();
}
