#include "Generator.h"

namespace Sportsed {
namespace Client {
namespace Printer {

Generator::Generator(QPrinter *printer)
	: m_printer(printer) {}

void Generator::generate(const Report &report)
{
	Q_UNUSED(report)
	Q_UNUSED(m_printer)
	// TODO: implement generation of documents
}

}
}
}
