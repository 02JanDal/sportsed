#pragma once

#include <QTime>

#include "Report.h"

namespace Sportsed {
namespace Client {
namespace Printer {

class Automation
{
public:
	explicit Automation();

	QString brief() const;
	QString fullDescription() const;

	Report report() const { return m_report; }
	void setReport(const Report &report) { m_report = report; }

	bool printedEvery() const { return m_printedEvery; }
	bool printedOnComplete() const { return !m_printedEvery; }
	void setPrintedEvery(const int every) { m_printedEvery = true; m_printedEveryMinutes = every; }
	void setPrintedOnComplete() { m_printedEvery = false; }

	bool isPrintedAfter() const { return !m_printAfter.isNull(); }
	void setPrintedAfter(const QTime &time) { m_printAfter = time; }

	bool isPrintDelayed() const { return m_printedDelay; }
	void setPrintedDelay(const bool delay) { m_printedDelay = delay; }
	void setPrintDelay(const int delay) { m_printDelay = delay; }

private:
	Report m_report;
	bool m_printedEvery = false;
	int m_printedEveryMinutes;

	QTime m_printAfter;
	bool m_printedDelay = false;
	int m_printDelay;
};

}
}
}
