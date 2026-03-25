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
//   MakeAccessible <input.pdf> extract [output.json]
//   MakeAccessible <input.pdf> apply <manifest.json> [output.pdf]
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

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

#define DEF_INPUT "../../../../Resources/Sample_Input/MakeAccessible.pdf"
#define DEF_EXTRACT_OUTPUT "page_descriptor.json"
#define DEF_APPLY_OUTPUT "MakeAccessible-out.pdf"

static void printUsage(const char *prog) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << prog << " <input.pdf> extract [output.json]" << std::endl;
    std::cout << "  " << prog << " <input.pdf> apply <manifest.json> [output.pdf]" << std::endl;
    std::cout << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  extract  Generate a JSON page descriptor for external analysis." << std::endl;
    std::cout << "  apply    Apply a structure manifest to tag the PDF." << std::endl;
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

    /* Parse arguments */
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string inputFile = argv[1];
    std::string mode = argv[2];

    if (mode != "extract" && mode != "apply") {
        std::cerr << "Error: unknown mode '" << mode << "'. Use 'extract' or 'apply'." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    DURING

        if (mode == "extract") {
            /* --- EXTRACT MODE --- */
            std::string outputFile = (argc > 3) ? argv[3] : DEF_EXTRACT_OUTPUT;

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

        } else if (mode == "apply") {
            /* --- APPLY MODE --- */
            if (argc < 4) {
                std::cerr << "Error: apply mode requires a manifest JSON file." << std::endl;
                printUsage(argv[0]);
                errCode = 1;
            } else {
                std::string manifestFile = argv[3];
                std::string outputFile = (argc > 4) ? argv[4] : DEF_APPLY_OUTPUT;

                std::cout << "Applying structure manifest from: " << manifestFile << std::endl;
                std::cout << "Input PDF: " << inputFile << std::endl;

                /* Read the manifest */
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
        }

    HANDLER
        errCode = ERRORCODE;
        libInit.displayError(errCode);
    END_HANDLER

    return errCode;
}
