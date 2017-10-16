#pragma once

#include <QTcpSocket>

namespace Sportsed {
namespace Common {

class TcpMessageSocket : public QTcpSocket
{
	Q_OBJECT
public:
	explicit TcpMessageSocket(QObject *parent = nullptr);

public slots:
	void send(const QByteArray &msg);

signals:
	void message(const QByteArray &data);

private:
	int m_currentMessageSize = -1;
	QByteArray m_buffer;

	void dataReady();
};

}
}
