#include "ProjectImportPlan.h"
#include <iomanip>
#include <sstream>

namespace orbit::import
{
std::string describeUnconvertedRecord(const UnconvertedSourceRecord& record)
{
    std::ostringstream out;
    out << std::quoted(record.sourceFormat) << " " << std::quoted(record.context);
    if (record.sourceOffset) out << " @" << *record.sourceOffset;
    out << " reason=" << std::quoted(record.reason) << " bytes=";
    for (const auto byte : record.bytes)
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte);
    return out.str();
}
}
