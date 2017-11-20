#include "ServerConnection.h"

#include <jd-util/Json.h>
#include <QEventLoop>

using namespace JD::Util;

namespace Sportsed {
namespace Client {

FutureImpl::FutureImpl(ServerConnection *conn, const int msgId)
	: m_conn(conn), m_msgId(msgId) {}

FutureImpl::~FutureImpl()
{
	m_conn->futureRemoved(m_msgId);
}

QJsonValue FutureImpl::get()
{
	if (!m_value) {
		m_loop = std::make_unique<QEventLoop>();
		m_loop->exec();
		m_loop.reset();
	}

	const QJsonObject obj = *m_value;
	if (Json::ensureString(obj, "cmd") == "error") {
		throw FutureResultException(Json::ensureString(obj, "data"));
	}
	return Json::ensureValue(obj, "data");
}

void FutureImpl::then(const Callback &cb)
{
	std::shared_ptr<FutureImpl> that = shared_from_this();
	m_cb = [that, cb]() {
		try {
			cb();
		} catch (...) {
			std::shared_ptr<FutureImpl>(that).reset();
			// throw; // do _not_ rethrow since we are likely called through Qt signals/slots
		}
	};
	if (m_value) {
		m_cb();
	}
}

void FutureImpl::receivedMessage(const QJsonObject &obj)
{
	m_value = obj;
	if (m_loop) {
		m_loop->exit();
	}

	if (m_cb) {
		m_cb();
	}
}

Subscribtion::Subscribtion(QObject *parent)
	: QObject(parent) {}

}
}
