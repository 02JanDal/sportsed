#include "InterfaceSync.h"

#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include <QDateTimeEdit>
#include <QTimeEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QRadioButton>
#include <QPushButton>
#include <QMessageBox>
#include <QApplication>
#include <QAbstractProxyModel>

#include <jd-util-gui/ModelUtil.h>
#include <jd-util/MultiLevelModel.h>
#include <jd-util/Util.h>
#include <jd-util/Functional.h>

#include "RecordObject.h"
#include "AbstractRecordModel.h"
#include "ClientMainWindow.h"

namespace Sportsed {
namespace Client {

namespace detail {
static void setWidgetValue(QComboBox *widget, const QVariant &value)
{
	if (widget->isEditable()) {
		widget->setCurrentText(value.toString());
	} else {
		const int index = widget->findData(value);
		if (index != -1) {
			widget->setCurrentIndex(index);
		}
	}
}

static inline std::tuple<JD::Util::MultiLevelModel *, AbstractRecordModel *> extractModels(QAbstractItemModel *leaf)
{
	using JD::Util::MultiLevelModel;

	MultiLevelModel *mlm;
	AbstractRecordModel *model;
	if (leaf->property("multiLevelModel").isValid()) {
		// case B: part of a MultiLevelModel
		mlm = leaf->property("multiLevelModel").value<MultiLevelModel *>();
		model = qobject_cast<AbstractRecordModel *>(mlm->source());
		qDebug() << model << mlm << mlm->source();
	} else if (leaf->metaObject()->inherits(&QAbstractProxyModel::staticMetaObject)) {
		// case A: generic proxy model
		QAbstractProxyModel *proxy = qobject_cast<QAbstractProxyModel *>(leaf);
		mlm = new MultiLevelModel(proxy->sourceModel());
		mlm->append(proxy);
		model = qobject_cast<AbstractRecordModel *>(proxy->sourceModel());
	} else {
		// case C: plain model
		mlm = new MultiLevelModel(leaf);
		model = qobject_cast<AbstractRecordModel *>(leaf);
	}
	Q_ASSERT_X(model, "ViewSync::buttons", "view with invalid type of model given");
	return std::make_tuple(mlm, model);
}
static inline void buttonsInternal(QAbstractItemModel *leaf, QWidget *widget, QPushButton *add)
{
	QObject::connect(add, &QPushButton::clicked, leaf, [leaf, widget]() {
		JD::Util::MultiLevelModel *mlm;
		AbstractRecordModel *model;
		std::tie(mlm, model) = extractModels(leaf);

		ClientMainWindow::waitFor(model->add(),
								  QApplication::translate("ViewSync", "Adding record..."),
								  widget);
	});
}

template <typename Widget, typename Getter, typename Setter>
static inline void setupViewSync(QAbstractItemView *view, const int role, Widget *widget, Getter getter, Setter setter)
{
	auto currentIndex = [view]() {
		return view->currentIndex().sibling(view->currentIndex().row(), 0);
	};
	setter(currentIndex().data(role));
	QObject::connect(view->model(), &QAbstractItemModel::dataChanged, widget, [view, role, setter, currentIndex](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles) {
		if (roles.contains(role) && view->currentIndex().row() >= tl.row() && view->currentIndex().row() <= br.row()) {
			setter(currentIndex().data(role));
		}
	});
	QObject::connect(view->selectionModel(), &QItemSelectionModel::currentRowChanged, widget, [view, role, getter, setter](const QModelIndex &current, const QModelIndex &previous) {
		if (previous.isValid()) {
			view->model()->setData(previous, getter(), role);
		}
		if (current.isValid()) {
			setter(current.data(role));
		} else {
			setter(QVariant());
		}
	});
}
template <typename Widget, typename Getter, typename Setter, typename Signal>
static inline void setupViewSync(QAbstractItemView *view, const int role, Widget *widget, Getter getter, Setter setter, Signal signal)
{
	setupViewSync(view, role, widget, getter, setter);

	QObject::connect(widget, signal, view->model(), [view, getter, role]() {
		view->model()->setData(view->currentIndex().sibling(view->currentIndex().row(), 0),
							   getter(), role);
	});
}

template <typename Widget, typename Getter, typename Setter>
static inline void setupComboBoxSync(QComboBox *view, const int role, Widget *widget, Getter getter, Setter setter)
{
	setter(view->currentData(role));
	QObject::connect(view->model(), &QAbstractItemModel::dataChanged, widget, [view, role, setter](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles) {
		if (roles.contains(role) && view->currentIndex() >= tl.row() && view->currentIndex() <= br.row()) {
			setter(view->currentData(role));
		}
	});
	QObject::connect(view, QOverload<int>::of(&QComboBox::currentIndexChanged), widget, [view, role, getter, setter]() {
		setter(view->currentData(role));
	});
}
template <typename Widget, typename Getter, typename Setter, typename Signal>
static inline void setupComboBoxSync(QComboBox *view, const int role, Widget *widget, Getter getter, Setter setter, Signal signal)
{
	setupComboBoxSync(view, role, widget, getter, setter);

	QObject::connect(widget, signal, view, [view, getter, role]() {
		view->setItemData(view->currentIndex(), getter(), role);
	});
}
}

namespace ViewSync {
void sync(QAbstractItemView *view, const int role, QLabel *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->text(); },
				[widget](const QVariant &val) { widget->setText(val.toString()); }
	);
}
void sync(QAbstractItemView *view, const int role, QLineEdit *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->text(); },
				[widget](const QVariant &val) { widget->setText(val.toString()); },
				&QLineEdit::textEdited
	);
}
void sync(QAbstractItemView *view, const int role, QDateTimeEdit *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->dateTime(); },
				[widget](const QVariant &val) { widget->setDateTime(val.toDateTime()); },
				&QDateTimeEdit::dateTimeChanged
	);
}
void sync(QAbstractItemView *view, const int role, QSpinBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->value(); },
				[widget](const QVariant &val) { widget->setValue(val.toInt()); },
				QOverload<int>::of(&QSpinBox::valueChanged)
	);
}
void sync(QAbstractItemView *view, const int role, QComboBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->isEditable() ? widget->currentText() : widget->currentData(); },
				[widget](const QVariant &val) {
					if (widget->isEditable()) {
						widget->setCurrentText(val.toString());
					} else {
						widget->setCurrentIndex(widget->findData(val));
					}
				},
				&QComboBox::currentTextChanged
	);
}
void sync(QAbstractItemView *view, const int role, QCheckBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupViewSync(
				view, role, widget,
				[widget]() { return widget->isChecked(); },
				[widget](const QVariant &val) { widget->setChecked(val.toBool()); },
				&QCheckBox::toggled
	);
}

void sync(QComboBox *view, const int role, QLabel *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->text(); },
				[widget](const QVariant &val) { widget->setText(val.toString()); }
	);
}
void sync(QComboBox *view, const int role, QLineEdit *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->text(); },
				[widget](const QVariant &val) { widget->setText(val.toString()); },
				&QLineEdit::textEdited
	);
}
void sync(QComboBox *view, const int role, QDateTimeEdit *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->dateTime(); },
				[widget](const QVariant &val) { widget->setDateTime(val.toDateTime()); },
				&QDateTimeEdit::dateTimeChanged
	);
}
void sync(QComboBox *view, const int role, QSpinBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->value(); },
				[widget](const QVariant &val) { widget->setValue(val.toInt()); },
				QOverload<int>::of(&QSpinBox::valueChanged)
	);
}
void sync(QComboBox *view, const int role, QComboBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->isEditable() ? widget->currentText() : widget->currentData(); },
				[widget](const QVariant &val) {
					if (widget->isEditable()) {
						widget->setCurrentText(val.toString());
					} else {
						widget->setCurrentIndex(widget->findData(val));
					}
				},
				&QComboBox::currentTextChanged
	);
}
void sync(QComboBox *view, const int role, QCheckBox *widget)
{
	JD::Util::enableIfHasSelection(view, widget);
	detail::setupComboBoxSync(
				view, role, widget,
				[widget]() { return widget->isChecked(); },
				[widget](const QVariant &val) { widget->setChecked(val.toBool()); },
				&QCheckBox::toggled
	);
}

class RadioButtonsProxy : public QObject
{
	Q_OBJECT
public:
	explicit RadioButtonsProxy(const QHash<QRadioButton *, QVariant> &widgets, QObject *parent = nullptr)
		: QObject(parent), m_widgets(widgets)
	{
		for (QRadioButton *btn : widgets.keys()) {
			connect(btn, &QRadioButton::toggled, this, &RadioButtonsProxy::changed);
		}
	}

	QVariant value() const
	{
		for (auto it = m_widgets.cbegin(); it != m_widgets.cend(); ++it) {
			if (it.key()->isChecked()) {
				return it.value();
			}
		}
		return QVariant();
	}

public slots:
	void setValue(const QVariant &val)
	{
		m_widgets.key(val)->setChecked(true);
	}

signals:
	void changed();

private:
	QHash<QRadioButton *, QVariant> m_widgets;
};

void sync(QAbstractItemView *view, const int role, const QHash<QRadioButton *, QVariant> &widgets)
{
	for (QRadioButton *widget : widgets.keys()) {
		JD::Util::enableIfHasSelection(view, widget);
	}

	RadioButtonsProxy *proxy = new RadioButtonsProxy(widgets, view);

	detail::setupViewSync(
				view, role, proxy,
				[proxy]() { return proxy->value(); },
				[proxy](const QVariant &val) { proxy->setValue(val); },
				&RadioButtonsProxy::changed
	);
}
void sync(QComboBox *view, const int role, const QHash<QRadioButton *, QVariant> &widgets)
{
	for (QRadioButton *widget : widgets.keys()) {
		JD::Util::enableIfHasSelection(view, widget);
	}

	RadioButtonsProxy *proxy = new RadioButtonsProxy(widgets, view);

	detail::setupComboBoxSync(
				view, role, proxy,
				[proxy]() { return proxy->value(); },
				[proxy](const QVariant &val) { proxy->setValue(val); },
				&RadioButtonsProxy::changed
	);
}

void buttons(QAbstractItemView *view, QPushButton *add, QPushButton *remove)
{
	JD::Util::enableIfHasSelection(view, remove);

	detail::buttonsInternal(view->model(), view, add);
	QObject::connect(remove, &QPushButton::clicked, view, [view]() {
		const int numRecords = view->selectionModel()->selectedRows().size();
		const int ans = QMessageBox::question(view,
											  QApplication::translate("ViewSync", "Really remove?"),
											  QApplication::translate("ViewSync",
																	  "You are about to remove %n record(s). Proceed?",
																	  nullptr,
																	  numRecords));
		if (ans == QMessageBox::Yes) {
			JD::Util::MultiLevelModel *mlm;
			AbstractRecordModel *model;
			std::tie(mlm, model) = detail::extractModels(view->model());

			const auto indices =
					JD::Util::Functional::map2<QVector<QPersistentModelIndex>>(view->selectionModel()->selectedRows(),
																			   [](const QModelIndex &i) { return QPersistentModelIndex(i); });
			for (const QPersistentModelIndex &index : indices) {
				model->remove(mlm->mapToSource(index));
			}
		}
	});
}
void buttons(QComboBox *view, QPushButton *add, QPushButton *remove)
{
	JD::Util::enableIfHasSelection(view, remove);

	detail::buttonsInternal(view->model(), view, add);
	QObject::connect(remove, &QPushButton::clicked, view, [view]() {
		const int ans = QMessageBox::question(view,
											  QApplication::translate("ViewSync", "Really remove?"),
											  QApplication::translate("ViewSync", "You are about to remove a record. Proceed?"));
		if (ans == QMessageBox::Yes) {
			JD::Util::MultiLevelModel *mlm;
			AbstractRecordModel *model;
			std::tie(mlm, model) = detail::extractModels(view->model());

			model->remove(mlm->mapToSource(view->model()->index(view->currentIndex(), 0)));
		}
	});
}
}

namespace ItemSync {
void sync(RecordObject *obj, const QString &field, QLabel *widget)
{
	widget->setText(obj->value<QString>(field));
	QObject::connect(obj, &RecordObject::updated, widget, [obj, field, widget](const QVector<QString> &fields) {
		if (fields.contains(field)) {
			widget->setText(obj->value<QString>(field));
		}
	});
}
void sync(RecordObject *obj, const QString &field, QLineEdit *widget)
{
	widget->setText(obj->value<QString>(field));
	QObject::connect(obj, &RecordObject::updated, widget, [obj, field, widget](const QVector<QString> &fields) {
		if (fields.contains(field)) {
			widget->setText(obj->value<QString>(field));
		}
	});
	QObject::connect(widget, &QLineEdit::textEdited, obj, [obj, field](const QString &text) {
		obj->setValue(field, text);
	});
}
void sync(RecordObject *obj, const QString &field, QDateTimeEdit *widget)
{
	widget->setDateTime(obj->value<QDateTime>(field));
	QObject::connect(obj, &RecordObject::updated, widget, [obj, field, widget](const QVector<QString> &fields) {
		if (fields.contains(field)) {
			widget->setDateTime(obj->value<QDateTime>(field));
		}
	});
	QObject::connect(widget, &QDateTimeEdit::dateTimeChanged, obj, [obj, field](const QDateTime &date) {
		obj->setValue(field, date);
	});
}
void sync(RecordObject *obj, const QString &field, QSpinBox *widget)
{
	widget->setValue(obj->value<int>(field));
	QObject::connect(obj, &RecordObject::updated, widget, [obj, field, widget](const QVector<QString> &fields) {
		if (fields.contains(field)) {
			widget->setValue(obj->value<int>(field));
		}
	});
	QObject::connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), obj, [obj, field](const int value) {
		obj->setValue(field, value);
	});
}
void sync(RecordObject *obj, const QString &field, QComboBox *widget)
{
	detail::setWidgetValue(widget, obj->value(field));
	QObject::connect(obj, &RecordObject::updated, widget, [obj, field, widget](const QVector<QString> &fields) {
		if (fields.contains(field)) {
			detail::setWidgetValue(widget, obj->value(field));
		}
	});
	QObject::connect(widget, &QComboBox::currentTextChanged, obj, [obj, field, widget]() {
		if (widget->isEditable()) {
			obj->setValue(field, widget->currentText());
		} else {
			obj->setValue(field, widget->currentData());
		}
	});
}
}

}
}

#include "InterfaceSync.moc"
