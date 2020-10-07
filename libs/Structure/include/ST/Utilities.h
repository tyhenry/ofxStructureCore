/*
    Utilities.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

#include <ST/Macros.h>

#include <cmath>
#include <string>

namespace ST
{
    /** @brief Expands defined templates into valid file paths,
     * For example, `resolveSmartPath("[AppDocuments]/data/file.txt");`
       will return "C:\Users\you\Documents\data\file.txt".
        @param inputPath Shorthanded path to a file or directory.
        @return Expanded absolute path to the given inputPath.
    */
    ST_API std::string resolveSmartPath(const std::string& inputPath);

    /** @brief Creates an empty directory at the provided path. Does nothing if the directory already exists.
        @param inputPath The directory to create.
        @return True if the directory was created, otherwise false.
    */
    ST_API bool createDirectories(const std::string& inputPath);

    /** @brief Utility function to get a formatted local time string, using std::put_time.
        @param format The structured format of the output string, see std::put_time for
       more information. The default format looks like: "2017-02-15_11-45-38".
        @return A string detailing the local time using the format specified.
    */
    ST_API std::string formattedStringFromLocaltime(const std::string& format = "%Y-%m-%d_%H-%M-%S");

    /** @brief Get the current time in seconds in the time reference of BridgeEngine.
        Useful for retrieving the current time for pose prediction.
    */
    ST_API double getTimestampNow();

    /** @brief Sets the internal GUI-based debugging verbosity. 1 is default, -1 will disable the HUD entirely. */
    ST_API bool setVisualLoggingVerbosity(int verbosity = 1);

    /** @brief Sets the internal console-based debugging verbosity. 0 is default, 1 will likely slow your application. */
    ST_API bool setConsoleLoggingVerbosity(int verbosity = 0);

    /** @brief Sets the internal console-based debugging/logging system to write out logs to a file. This is disabled by default.
        The logs will be located in [AppDocuments]/occ on Windows and Linux, and "/data/occ" on Android.
    */
    ST_API void setConsoleLoggingToWriteToFile(bool writeLogsToFile = false);

    /** @brief Initialize visual logging. Safe to call multiple times. */
    ST_API void initializeVisualLogging();


    /** @brief Forces all visual logging windows to hide themselves. */
    ST_API void hideAllVisualLoggingWindows();

    /** @brief Forces all visual logging windows to show themselves. */
    ST_API void showAllVisualLoggingWindows();

    /** @brief Returns true if the visual logging windows are visible. */
    ST_API bool areVisualLoggingWindowsShown();

    /** @brief Returns true if the two floats are equal within floating point error. */
    ST_API bool floatEquals(float rhs, float lhs);

    //------------------------------------------------------------------------------


}  // ST namespace
