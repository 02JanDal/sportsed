#pragma once

#include <jd-util/Json.h>
#include <functional>

class QEventLoop;

namespace Sportsed {
namespace Common {
class ChangeResponse;
}

namespace Client {
class ServerConnection;

DECLARE_EXCEPTION(FutureResult)

// TODO consider replacing by custom ref-counted implementation
class FutureImpl : public std::enable_shared_from_this<FutureImpl>
{
public:
	~FutureImpl();

private:
	friend class ServerConnection;
	template <typename T> friend class Future;

	using Callback = std::function<void()>;

	explicit FutureImpl(ServerConnection *conn, const int msgId);

	QJsonValue get();
	bool hasValue() const { return !!m_value; }
	void then(const Callback &cb);

	void receivedMessage(const QJsonObject &obj);

private:
	ServerConnection *m_conn;
	int m_msgId;
	std::unique_ptr<QEventLoop> m_loop;
	Callback m_cb;

	std::experimental::optional<QJsonObject> m_value;
};

template <typename T>
class Future
{
public:
	explicit Future(const std::shared_ptr<FutureImpl> &impl) : m_impl(impl) {}

	T get() const { return JD::Util::Json::ensureIsType<T>(m_impl->get()); }
	bool hasValue() const { return m_impl->hasValue(); }

	void then(const std::function<void(T)> &cb, const std::function<void(Exception)> &errorCb = {})
	{
		thenImpl(cb, errorCb);
	}
	void then(const std::function<void(T)> &cb, const std::function<void()> &errorCb)
	{
		thenImpl(cb, [errorCb](Exception) { if (errorCb) errorCb(); });
	}
	void then(const std::function<void()> &cb, const std::function<void(Exception)> &errorCb = {})
	{
		thenImpl([cb](T){ if (cb) cb(); }, errorCb);
	}
	void then(const std::function<void()> &cb, const std::function<void()> &errorCb)
	{
		thenImpl([cb](T){ if (cb) cb(); }, [errorCb](Exception) { if (errorCb) errorCb(); });
	}

private:
	std::shared_ptr<FutureImpl> m_impl;

	void thenImpl(const std::function<void(T)> &cb, const std::function<void(Exception)> &errorCb)
	{
		std::shared_ptr<FutureImpl> impl = m_impl;
		m_impl->then([impl, cb, errorCb]() {
			T val;
			bool threw = false;
			try {
				val = JD::Util::Json::ensureIsType<T>(impl->get());
			} catch (Exception e) {
				threw = true;
				if (errorCb) {
					errorCb(e);
				}
			} catch (...) {
				threw = true;
				if (errorCb) {
					errorCb(Exception(QString()));
				}
			}
			if (!threw) {
				if (cb) {
					cb(val);
				}
			}

			std::shared_ptr<FutureImpl>(impl).reset();
		});
	}
};

class Subscribtion : public QObject
{
	Q_OBJECT
public:
	explicit Subscribtion(QObject *parent = nullptr);

signals:
	void triggered(const Common::ChangeResponse &changes);
};

}
}
