#include "TcpMessageSocket.h"

namespace Sportsed {
namespace Common {

TcpMessageSocket::TcpMessageSocket(QObject *parent)
	: QTcpSocket(parent)
{
	connect(this, &QTcpSocket::readyRead, this, &TcpMessageSocket::dataReady);
}

void TcpMessageSocket::send(const QByteArray &msg)
{
	const int size = msg.size();
	QByteArray sizeData(4, 0);
	sizeData[0] = static_cast<char>(size >> 24);
	sizeData[1] = static_cast<char>(size >> 16);
	sizeData[2] = static_cast<char>(size >> 8);
	sizeData[3] = static_cast<char>(size >> 0);
	write(sizeData);
	write(msg);
}

void TcpMessageSocket::dataReady()
{
	while (bytesAvailable() > 0) {
		if (m_currentMessageSize == -1) {
			if (bytesAvailable() < 4) {
				// package size is 4 bytes - if we have less we need to wait
				break;
			} else {
				const QByteArray sizeData = read(4);
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
			m_buffer += read(m_currentMessageSize - m_buffer.size());
		}

		if (m_buffer.size() == m_currentMessageSize) {
			message(m_buffer);
			m_buffer.clear();
		}
	}
}

}
}
