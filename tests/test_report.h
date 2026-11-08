#pragma once

#include <string>

struct TestResult
{
    std::string Suite;
    std::string Name;
    std::string Status;
    std::string Category;
    std::string File;
    unsigned int Line = 0;
    std::string Message;
};

void RecordTestResult(const TestResult& result);
