//
// Copyright (c) 2026, Datalogics, Inc. All rights reserved.
//
// MakeAccessible — PDF accessibility tagging pipeline.
//
// This sample demonstrates the two-phase MakeAccessible workflow:
//   1. Extract: generates a JSON page descriptor from a PDF for external analysis
//   2. Apply:   consumes a structure manifest to tag the PDF
//
// Usage:
//   MakeAccessible <input.pdf>                    Extract page descriptor → <input>.json
//   MakeAccessible <input.pdf> <manifest.json>    Apply manifest → <input>_tagged.pdf
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "InitializeLibrary.h"
#include "APDFLDoc.h"

#include "PDCalls.h"
#include "ASCalls.h"
#include "CosCalls.h"
#include "PERCalls.h"
#include "PEWCalls.h"
#include "PagePDECntCalls.h"
#include "PDSReadCalls.h"
#include "PDSWriteCalls.h"

#include "DLMakeAccessible.h"

static void printUsage(const char *prog) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << prog << " <input.pdf>                  Extract page descriptor" << std::endl;
    std::cout << "  " << prog << " <input.pdf> <manifest.json>  Apply manifest and tag PDF" << std::endl;
    std::cout << std::endl;
    std::cout << "Extract mode produces <basename>.json from <basename>.pdf" << std::endl;
    std::cout << "Apply mode produces <basename>_tagged.pdf" << std::endl;
}

/* Strip directory and extension from a file path to get the base name */
static std::string getBaseName(const std::string &path) {
    /* Find last path separator */
    size_t lastSlash = path.find_last_of("/\\");
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);

    /* Strip extension */
    size_t lastDot = filename.rfind('.');
    if (lastDot != std::string::npos)
        filename = filename.substr(0, lastDot);

    return filename;
}

static std::string readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open file: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static bool writeFile(const std::string &path, const std::string &content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: cannot write to file: " << path << std::endl;
        return false;
    }
    file << content;
    return true;
}

int main(int argc, char **argv) {
    APDFLib libInit;
    ASErrorCode errCode = 0;

    if (libInit.isValid() == false) {
        errCode = libInit.getInitError();
        std::cout << "Initialization failed with code " << errCode << std::endl;
        return errCode;
    }

    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string baseName = getBaseName(inputFile);

    DURING

        if (argc == 2) {
            /* --- EXTRACT MODE: just a PDF file --- */
            std::string outputFile = baseName + ".json";

            std::cout << "Extracting page descriptor from: " << inputFile << std::endl;

            APDFLDoc document(inputFile.c_str(), true);

            char *jsonResult = DLMakeAccessibleExtract(document.getPDDoc());
            if (jsonResult) {
                if (writeFile(outputFile, jsonResult)) {
                    std::cout << "Page descriptor written to: " << outputFile << std::endl;
                } else {
                    errCode = 1;
                }
                ASfree(jsonResult);
            } else {
                std::cerr << "Error: extraction failed." << std::endl;
                errCode = 1;
            }

        } else if (argc >= 3) {
            /* --- APPLY MODE: PDF + manifest JSON --- */
            std::string manifestFile = argv[2];
            std::string outputFile = baseName + "_tagged.pdf";

            std::cout << "Input PDF: " << inputFile << std::endl;
            std::cout << "Applying structure manifest from: " << manifestFile << std::endl;

            std::string manifestJSON = readFile(manifestFile);
            if (manifestJSON.empty()) {
                errCode = 1;
            } else {
                APDFLDoc document(inputFile.c_str(), true);

                ASBool success = DLMakeAccessibleApply(document.getPDDoc(), manifestJSON.c_str());
                if (success) {
                    document.saveDoc(outputFile.c_str());
                    std::cout << "Tagged PDF written to: " << outputFile << std::endl;
                } else {
                    std::cerr << "Error: apply failed." << std::endl;
                    errCode = 1;
                }
            }
        }

    HANDLER
        errCode = ERRORCODE;
        libInit.displayError(errCode);
    END_HANDLER

    return errCode;
}
