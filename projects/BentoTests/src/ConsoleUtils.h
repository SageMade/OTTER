#pragma once
#include <vector>
#include <string>

class ConsoleUtils {
public:
	static int OptionsMenu(const std::vector<std::string>& options, const std::string& prompt = "> ", bool startAtZero = true);
	static int OptionsMenu(int numOptions, const std::string& prompt = "> ", bool startAtZero = true);
};