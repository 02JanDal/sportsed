#pragma once

#include <QTcpSocket>
#include <QLoggingCategory>

#include <jd-util/Exception.h>

namespace Sportsed {
namespace Common {

DECLARE_EXCEPTION_X(SocketNotOpen, "Unable to write data to closed socket", ::Exception)
DECLARE_EXCEPTION_X(SocketWrite, "Unable to write data to socket", ::Exception)
Q_DECLARE_LOGGING_CATEGORY(messageSocket)

class MessageSocket : public QObject
{
	Q_OBJECT
public:
	explicit MessageSocket(QIODevice *device);

	QIODevice *device() const { return m_device; }

	void setDevice(QIODevice *device);

public slots:
	virtual void send(const QByteArray &msg);

signals:
	void message(const QByteArray &data);

private:
	int m_currentMessageSize = -1;
	QByteArray m_buffer;

	QIODevice *m_device = nullptr;

	void dataReady();
};

}
}
