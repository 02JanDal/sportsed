#pragma once

#include <QVector>
#include <commonlib/Record.h>

namespace Sportsed {
namespace Client {
namespace Printer {

class Report
{
public:
	enum Type {
		StartsByClass,
		StartsByTime,
		ResultsPreliminary,
		ResultsOfficial,
		MultiStagePreliminary,
		MultiStageOfficial
	};

	explicit Report(const Type type);

	Type type() const { return m_type; }

	Common::Record start() const { return m_start; }
	void setStart(const Common::Record id) { m_start = id; }

	QVector<Common::Record> classes() const { return m_classes; }
	void setClasses(const QVector<Common::Record> &classes) { m_classes = classes; }

	bool classesTogether() const { return m_classesTogether; }
	void setClassesTogether(const bool together) { m_classesTogether = together; }

private:
	Type m_type;
	Common::Record m_start;
	QVector<Common::Record> m_classes;
	bool m_classesTogether = false;
};

}
}
}
