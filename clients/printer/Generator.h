#pragma once

#include "Report.h"

class QPrinter;

namespace Sportsed {
namespace Client {
namespace Printer {

class Generator
{
public:
	explicit Generator(QPrinter *printer);

	void generate(const Report &report);

private:
	QPrinter *m_printer;
};

}
}
}
