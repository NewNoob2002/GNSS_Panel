/*------------------------------------------------------------------------------
SparkFun_Extensible_Message_Parser.cpp

Parse messages from GNSS radios

License: MIT. Please see LICENSE.md for more details
------------------------------------------------------------------------------*/

#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SparkFun_Extensible_Message_Parser.h"

#if defined(SEMP_CRC24_CONFIG_ON)
#include "semp_crc24q.h"
#endif
#if defined(SEMP_CRC32_CONFIG_ON)
#include "semp_crc32.h" // IWYU pragma: keep
#endif
//----------------------------------------
// Constants
//----------------------------------------

#define SEMP_ALIGNMENT_MASK 7

//----------------------------------------
// Macros
//----------------------------------------

#define SEMP_ALIGN(x)       ((x + SEMP_ALIGNMENT_MASK) & (~SEMP_ALIGNMENT_MASK))

__attribute__((weak)) void
debugPrintCallback(const char* format, ...) {
    (void)format;
}

__attribute__((weak)) void
debugErrorCallback(const char* format, ...) {
    (void)format;
}

//----------------------------------------
// Support routines
//----------------------------------------

// Allocate the parse structure
SEMP_PARSE_STATE*
sempAllocateParseStructure(const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK printDebug, uint16_t scratchPadBytes,
                           int bufferLength) {
    SEMP_PARSE_STATE* parse = nullptr;

    // Print the scratchPad area size
    sempPrintf(printDebug, "scratchPadBytes: 0x%04x (%d) bytes", scratchPadBytes, scratchPadBytes);

    // Align the scratch patch area
    if (scratchPadBytes < SEMP_ALIGN(scratchPadBytes)) {
        scratchPadBytes = SEMP_ALIGN(scratchPadBytes);
        sempPrintf(printDebug, "scratchPadBytes: 0x%04x (%d) bytes after alignment", scratchPadBytes, scratchPadBytes);
    }

    // Determine the minimum length for the scratch pad
    int length = SEMP_ALIGN(sizeof(SEMP_SCRATCH_PAD));
    if (scratchPadBytes < length) {
        scratchPadBytes = length;
        sempPrintf(printDebug, "scratchPadBytes: 0x%04x (%d) bytes after mimimum size adjustment", scratchPadBytes,
                   scratchPadBytes);
    }
    constexpr int parseBytes = SEMP_ALIGN(sizeof(SEMP_PARSE_STATE));
    sempPrintf(printDebug, "parseBytes: 0x%04x (%d)", parseBytes, parseBytes);

    // Verify the minimum bufferLength
    if (bufferLength < SEMP_MINIMUM_BUFFER_LENGTH) {
        sempPrintf(printDebug, "SEMP: Increasing bufferLength from %ld to %d bytes, minimum size adjustment",
                   bufferLength, SEMP_MINIMUM_BUFFER_LENGTH);
        bufferLength = SEMP_MINIMUM_BUFFER_LENGTH;
    }

    // Allocate the parser
    length = parseBytes + scratchPadBytes;
    uint8_t* rawMemory = new (std::nothrow) uint8_t[length + bufferLength];
    if (rawMemory) {
        parse = reinterpret_cast<SEMP_PARSE_STATE*>(rawMemory);
    }
    sempPrintf(printDebug, "parse: %p", static_cast<void*>(parse));

    // Initialize the parse structure
    if (parse) {
        // Zero the parse structure
        memset(parse, 0, length);

        // Set the scratch pad area address
        parse->scratchPad = reinterpret_cast<uint8_t*>(parse) + parseBytes;
        parse->printDebug = printDebug;
        sempPrintf(parse->printDebug, "parse->scratchPad: %p", parse->scratchPad);

        // Set the buffer address and length
        parse->bufferLength = bufferLength;
        parse->buffer = (static_cast<uint8_t*>(parse->scratchPad) + scratchPadBytes);
        sempPrintf(parse->printDebug, "parse->buffer: %p", parse->buffer);
    }
    return parse;
}

// Convert nibble to ASCII
int
sempAsciiToNibble(int data) {
    // Convert the value to lower case
    data |= 0x20;
    if ((data >= 'a') && (data <= 'f')) {
        return data - 'a' + 10;
    }
    if ((data >= '0') && (data <= '9')) {
        return data - '0';
    }
    return -1;
}

// Translate the type value into an ASCII type name
const char*
sempGetTypeName(SEMP_PARSE_STATE* parse, const uint16_t type) {
    auto name = "Unknown parser";

    if (parse) {
        if (type == parse->parserCount) {
            name = "No active parser, scanning for preamble";
        } else if (parse->parserNames && (type < parse->parserCount)) {
            name = parse->parserNames[type];
        }
    }
    return name;
}

// Print the parser's configuration
void
sempPrintParserConfiguration(SEMP_PARSE_STATE* parse, const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK print) {
    if (print && parse) {
        sempPrintln(print, "SparkFun Extensible Message Parser");
        sempPrintf(print, "    Name: %p (%s)", parse->parserName, parse->parserName);
        sempPrintf(print, "    parsers: %p", (void*)parse->parsers);
        sempPrintf(print, "    parserNames: %p", (void*)parse->parserNames);
        sempPrintf(print, "    parserCount: %d", parse->parserCount);
        sempPrintf(print, "    printError: %p", parse->printError);
        sempPrintf(print, "    printDebug: %p", parse->printDebug);
        sempPrintf(print, "    Scratch Pad: %p (%ld bytes)", static_cast<void*>(parse->scratchPad),
                   parse->buffer - static_cast<uint8_t*>(parse->scratchPad));
        sempPrintf(print, "    computeCrc: %p", (void*)parse->computeCrc);
        sempPrintf(print, "    crc: 0x%08x", parse->crc);
        sempPrintf(print, "    State: %p%s", (void*)parse->state,
                   (parse->state == sempFirstByte) ? " (sempFirstByte)" : "");
        sempPrintf(print, "    EomCallback: %p", (void*)parse->eomCallback);
        sempPrintf(print, "    Buffer: %p (%d bytes)", static_cast<void*>(parse->buffer), parse->bufferLength);
        sempPrintf(print, "    length: %d message bytes", parse->length);
        sempPrintf(print, "    type: %d (%s)", parse->type, sempGetTypeName(parse, parse->type));
    }
}

// Format and print a line of text
void
sempPrintf(const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK print, const char* format, ...) {
    if (print && format) {
        // 诊断日志限长，避免通信错误路径反复分配堆内存。
        char buffer[256];
        va_list args;
        va_start(args, format);
        const int length = vsnprintf(buffer, sizeof(buffer) - 2U, format, args);
        va_end(args);
        if (length < 0) {
            return;
        }
        const size_t used =
            static_cast<size_t>(length) < sizeof(buffer) - 3U ? static_cast<size_t>(length) : sizeof(buffer) - 3U;
        buffer[used] = '\r';
        buffer[used + 1U] = '\n';
        buffer[used + 2U] = '\0';
        print("%s", buffer);
    }
}

// Print a line of error text
void
sempPrintln(const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK print, const char* string) {
    if (print) {
        print("%s", string);
    }
}

// Translates state value into an ASCII state name
const char*
sempGetStateName(const SEMP_PARSE_STATE* parse) {
    if (parse && (parse->state == sempFirstByte)) {
        return "sempFirstByte";
    }
    return "Unknown state";
}

// Disable debug output
void
sempDisableDebugOutput(SEMP_PARSE_STATE* parse) {
    if (parse) {
        parse->printDebug = nullptr;
    }
}

// Enable debug output
void
sempEnableDebugOutput(SEMP_PARSE_STATE* parse, const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK print) {
    if (parse) {
        parse->printDebug = print;
    }
}

// Disable error output
void
sempDisableErrorOutput(SEMP_PARSE_STATE* parse) {
    if (parse) {
        parse->printError = nullptr;
    }
}

// Enable error output
void
sempEnableErrorOutput(SEMP_PARSE_STATE* parse, const SEMP_CUSTOM_ERROR_PRINT_CALLBACK print) {
    if (parse) {
        parse->printError = print;
    }
}

//----------------------------------------
// Parse routines
//----------------------------------------

// Initialize the parser
SEMP_PARSE_STATE*
sempBeginParser(const SEMP_PARSE_ROUTINE* parseTable, const uint16_t parserCount, const char* const* parserNameTable,
                const uint16_t parserNameCount, const uint16_t scratchPadBytes, const int bufferLength,
                const SEMP_EOM_CALLBACK eomCallback, const char* name,
                const SEMP_CUSTOM_ERROR_PRINT_CALLBACK printError, const SEMP_CUSTOM_DEBUG_PRINT_CALLBACK printDebug,
                const SEMP_BAD_CRC_CALLBACK badCrcCallback) {
    SEMP_PARSE_STATE* parse = nullptr;

    do {
        // Validate the parse type names table
        if (parserCount != parserNameCount) {
            sempPrintln(printError, "SEMP: Please fix parserTable and parserNameTable parserCount != parserNameCount");
            break;
        }

        // Validate the parserTable address is not nullptr
        if (!parseTable) {
            sempPrintln(printError, "SEMP: Please specify a parserTable data structure");
            break;
        }

        // Validate the parserNameTable address is not nullptr
        if (!parserNameTable) {
            sempPrintln(printError, "SEMP: Please specify a parserNameTable data structure");
            break;
        }

        // Validate the end-of-message callback routine address is not nullptr
        if (!eomCallback) {
            sempPrintln(printError, "SEMP: Please specify an eomCallback routine");
            break;
        }

        // Verify the parser name
        if ((!name) || (!strlen(name))) {
            sempPrintln(printError, "SEMP: Please provide a name for the parser");
            break;
        }

        // Verify that there is at least one parser in the table
        if (!parserCount) {
            sempPrintln(printError, "SEMP: Please provide at least one parser in parserTable");
            break;
        }

        // Validate the parser address is not nullptr
        parse = sempAllocateParseStructure(printDebug, scratchPadBytes, bufferLength);
        if (!parse) {
            sempPrintln(printError, "SEMP: Failed to allocate the parse structure");
            break;
        }

        // Initialize the parser
        parse->printError = printError;
        parse->parsers = parseTable;
        parse->parserCount = parserCount;
        parse->parserNames = parserNameTable;
        parse->state = sempFirstByte;
        parse->eomCallback = eomCallback;
        parse->parserName = name;
        parse->badCrc = badCrcCallback;

        // Display the parser configuration
        sempPrintParserConfiguration(parse, parse->printDebug);
    } while (false);

    // Return the parse structure address
    return parse;
}

// Wait for the first byte in the GPS message
bool
sempFirstByte(SEMP_PARSE_STATE* parse, const uint8_t data) {
    if (parse) {
        // Add this byte to the buffer
        parse->crc = 0;
        parse->computeCrc = nullptr;
        parse->length = 0;
        parse->type = parse->parserCount;
        parse->buffer[parse->length++] = data;

        // Walk through the parse table
        for (int index = 0; index < parse->parserCount; index++) {
            const SEMP_PARSE_ROUTINE parseRoutine = parse->parsers[index];
            if (parseRoutine(parse, data)) {
                parse->type = index;
                return true;
            }
        }

        // Preamble byte not found, continue searching for a preamble byte
        parse->state = sempFirstByte;
    }
    return false;
}

// Parse the next byte
void
sempParseNextByte(SEMP_PARSE_STATE* parse, const uint8_t data) {
    if (parse) {
        // Verify that enough space exists in the buffer
        if (parse->length >= parse->bufferLength) {
            // Message too long
            sempPrintf(parse->printError, "SEMP %s: Message too long, increase the buffer size > %d\r\n",
                       parse->parserName, parse->bufferLength);

            // Start searching for a preamble byte
            sempFirstByte(parse, data);
            return;
        }

        // Save the data byte
        parse->buffer[parse->length++] = data;

        // Compute the CRC value for the message
        if (parse->computeCrc) {
            parse->crc = parse->computeCrc(parse, data);
        }

        // Update the parser state based on the incoming byte
        parse->state(parse, data);
    }
}

// Shutdown the parser
void
sempStopParser(SEMP_PARSE_STATE** parse) {
    // Free the parse structure if it was specified
    if (parse && *parse) {
        delete[] reinterpret_cast<uint8_t*>(*parse);
        *parse = nullptr;
    }
}
