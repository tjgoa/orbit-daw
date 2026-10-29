// Standalone diagnostics utility, deliberately independent of the Orbit app.
#include "../Source/Import/OrionSfsReport.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: inspect-orion-sfs project.sfs\n";
        return 2;
    }
    try
    {
        orbit::import::orion::ImportDocument document;
        orbit::import::orion::SfsInspection inspection;
        std::string error;
        if (! orbit::import::orion::OrionSfsReader::readDocument(argv[1], document, inspection, error))
        {
            std::cerr << error << '\n';
            return 1;
        }
        std::cout << orbit::import::orion::createInspectionReport(document, inspection);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Inspection failed: " << error.what() << '\n';
        return 1;
    }
}
