#include "ConsoleUtils.h"

#include <iostream>

int ConsoleUtils::OptionsMenu(const std::vector<std::string>& options, const std::string& prompt, bool startAtZero)
{
	for (int ix = 0; ix < options.size(); ix++) {
		std::cout << (startAtZero ? ix : ix + 1) << ": " << options[ix] << std::endl;
	}
	return OptionsMenu(options.size(), prompt, startAtZero);
}

int ConsoleUtils::OptionsMenu(int numOptions, const std::string& prompt, bool startAtZero)
{
	std::cout << prompt;
	int index = -1;
	std::cin >> index;

	int min = startAtZero ? 0 : 1;
	int max = startAtZero ? numOptions - 1 : numOptions;

	bool failed = false;

	while (std::cin.fail() || index < min || index > max) {
		std::cin.clear();
		std::cin.ignore(256, '\n');
		printf("\033[A\33[2K\r");
		if (!failed) {
			failed = true;
			std::cout << "Entry must be a numeric entry on the list" << std::endl;
		}
		std::cout << prompt;
		std::cin >> index;
	}

	return index;
}
