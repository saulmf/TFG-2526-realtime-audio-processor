#include <iomanip>
#include <iostream>

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

namespace
{
    constexpr int k_ruleWidth = 80;
    constexpr int k_nameColumnWidth = 68;

    void printRule(char c)
    {
        std::cout << std::string(k_ruleWidth, c) << std::endl;
    }
}

class ConsoleUnitTestRunner final : public juce::UnitTestRunner
{
public:
    explicit ConsoleUnitTestRunner(bool verboseIn) : verbose(verboseIn) {}

    void logMessage(const juce::String& message) override
    {
        if (verbose)
            std::cout << message << std::endl;
    }

private:
    bool verbose;
};

int main(int argc, char* argv[])
{
    const juce::ScopedJuceInitialiser_GUI juceInitialiser;

    const bool verbose = juce::Process::isRunningUnderDebugger();
    ConsoleUnitTestRunner runner(verbose);
    runner.setAssertOnFailure(false);

    if (argc > 1)
        runner.runTestsInCategory(argv[1]);
    else
        runner.runAllTests();

    if (verbose)
        std::cout << std::endl;

    printRule('=');
    std::cout << "Unit Test Results" << std::endl;
    printRule('=');

    int totalCases    = 0;
    int totalPasses   = 0;
    int totalFailures = 0;
    juce::String currentCategory;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* r = runner.getResult(i);
        ++totalCases;
        totalPasses   += r->passes;
        totalFailures += r->failures;

        if (r->unitTestName != currentCategory)
        {
            currentCategory = r->unitTestName;
            std::cout << std::endl << currentCategory << std::endl;
            printRule('-');
        }

        const bool passed = r->failures == 0;
        std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] "
                   << std::left << std::setw(k_nameColumnWidth) << r->subcategoryName
                   << std::right << std::setw(5) << (r->passes + r->failures)
                   << " assertion" << ((r->passes + r->failures) == 1 ? "" : "s")
                   << std::endl;

        if (!passed)
            for (const auto& m : r->messages)
                std::cout << "         " << m << std::endl;
    }

    std::cout << std::endl;
    printRule('=');
    std::cout << "Total: " << totalCases << " test case" << (totalCases == 1 ? "" : "s")
               << ", " << (totalPasses + totalFailures) << " assertions, "
               << totalFailures << " failed" << std::endl;
    printRule('=');

    return totalFailures > 0 ? 1 : 0;
}
