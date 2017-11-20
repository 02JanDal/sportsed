#include "MessageSocket.h"

namespace Sportsed {
namespace Common {

Q_LOGGING_CATEGORY(messageSocket, "sportsed.common.socket")

MessageSocket::MessageSocket(QIODevice *device)
	: QObject(device)
{
	setDevice(device);
}

void MessageSocket::setDevice(QIODevice *device)
{
	if (m_device) {
		disconnect(m_device, nullptr, this, nullptr);
	}
	m_device = device;
	if (m_device) {
		connect(m_device, &QIODevice::readyRead, this, &MessageSocket::dataReady);
		dataReady();
	}
}

void MessageSocket::send(const QByteArray &msg)
{
	if (!m_device || !m_device->isOpen()) {
		throw SocketNotOpenException();
	}
	const int size = msg.size();
	QByteArray sizeData(4, 0);
	sizeData[0] = static_cast<char>(size >> 24);
	sizeData[1] = static_cast<char>(size >> 16);
	sizeData[2] = static_cast<char>(size >> 8);
	sizeData[3] = static_cast<char>(size >> 0);
	const qint64 sentSize = m_device->write(sizeData) + m_device->write(msg);
	if (sentSize < (size + 4)) {
		throw SocketWriteException("Unable to write data to socket: " + m_device->errorString());
	} else {
		qCDebug(messageSocket) << "sent" << size << "bytes of data:" << msg;
	}
}

void MessageSocket::dataReady()
{
	qDebug() << m_device->bytesAvailable();
	while (m_device->bytesAvailable() > 0) {
		if (m_currentMessageSize == -1) {
			if (m_device->bytesAvailable() < 4) {
				// package size is 4 bytes - if we have less we need to wait
				break;
			} else {
				const QByteArray sizeData = m_device->read(4);
				Q_ASSERT(!sizeData.isNull() && !sizeData.isEmpty());
				m_currentMessageSize =
						(static_cast<unsigned char>(sizeData.at(0)) << 24) |
						(static_cast<unsigned char>(sizeData.at(1)) << 16) |
						(static_cast<unsigned char>(sizeData.at(2)) << 8) |
						(static_cast<unsigned char>(sizeData.at(3)) << 0);
			}
		}

		// not an else to the previous if since we might get a different value within its body
		if (m_currentMessageSize != -1) {
			m_buffer += m_device->read(m_currentMessageSize - m_buffer.size());
		}

		if (m_buffer.size() == m_currentMessageSize) {
			qCDebug(messageSocket) << "received" << m_currentMessageSize << "bytes of data:" << m_buffer;
			message(m_buffer);
			m_buffer.clear();
			m_currentMessageSize = -1;
		}
	}
}

}
}
